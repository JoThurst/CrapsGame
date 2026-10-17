// Copyright Epic Games, Inc. All Rights Reserved.

#include "Scoring/ScoringQueueSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UScoringQueueSubsystem::UScoringQueueSubsystem()
{
	// Optional finders: missing engine sounds degrade silently instead of logging load errors.
	static ConstructorHelpers::FObjectFinderOptional<USoundBase> ClickSound(TEXT("/Engine/VREditor/Sounds/VR_click1.VR_click1"));
	static ConstructorHelpers::FObjectFinderOptional<USoundBase> FallbackSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
	if (ClickSound.Get())
	{
		StepPopSound = ClickSound.Get();
	}
	else if (FallbackSound.Get())
	{
		StepPopSound = FallbackSound.Get();
	}
}

TStatId UScoringQueueSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UScoringQueueSubsystem, STATGROUP_Tickables);
}

void UScoringQueueSubsystem::FlushQueue()
{
	while (PendingSteps.Num() > 0)
	{
		PopAndExecuteNextStep();
	}
	if (bIsResolvingEndOfRoll)
	{
		bIsResolvingEndOfRoll = false;
		const int64 FinalPayout = bPendingSevenOutWipe ? 0 : GetCurrentThrowTotalPayout();
		OnScoringSequenceFinished.Broadcast(FinalPayout, bPendingSevenOutWipe);
	}
}

void UScoringQueueSubsystem::BeginNewThrowScoringSession()
{
	if (bIsResolvingEndOfRoll && PendingSteps.Num() > 0)
	{
		FlushQueue();
	}

	PendingSteps.Reset();
	RunningBaseChips = 0;
	RunningThrowMult = 1.0f;
	RunningBetMult = 1.0f;
	CurrentPitchSemitoneShift = 0;
	PoppedStepCountThisThrow = 0;
	CurrentStepInterval = BaseStepIntervalSeconds;
	StepTimer = 0.0f;
	bIsResolvingEndOfRoll = false;
	bPendingSevenOutWipe = false;
	LastExecutedStep = FScoringStep();
	UpdateMultiplierIntensityScalar();
}

void UScoringQueueSubsystem::VoidCurrentThrowScoring()
{
	PendingSteps.Reset();
	ActivePopups.Empty();
	RunningBaseChips = 0;
	RunningThrowMult = 1.0f;
	RunningBetMult = 1.0f;
	CurrentPitchSemitoneShift = 0;
	PoppedStepCountThisThrow = 0;
	CurrentStepInterval = BaseStepIntervalSeconds;
	StepTimer = 0.0f;
	bIsResolvingEndOfRoll = false;
	bPendingSevenOutWipe = false;
	LastExecutedStep = FScoringStep();
	UpdateMultiplierIntensityScalar();
}

void UScoringQueueSubsystem::EnqueueScoringStep(const FScoringStep& Step)
{
	PendingSteps.Add(Step);
}

void UScoringQueueSubsystem::EnqueueMidRollCollisionStep(
	AActor* SourceActor,
	const FVector& ImpactLocation,
	const FString& Label,
	int32 ChipDelta,
	float ThrowMultDelta
)
{
	FScoringStep Step;
	Step.SourceActor = SourceActor;
	Step.PopupWorldLocation = ImpactLocation;
	Step.StepLabel = Label;
	Step.Layer = (ThrowMultDelta > 0.0f) ? EScoringLayer::ThrowMult : EScoringLayer::BaseChips;
	Step.BaseChipDelta = ChipDelta;
	Step.ThrowMultDelta = ThrowMultDelta;
	Step.MultiplierFactor = 1.0f;

	// Mid-roll physical impacts pop immediately during flight so the player gets instant tactile feedback!
	ApplyStepMath(Step);
}

void UScoringQueueSubsystem::StartEndOfRollResolution(bool bIsSevenOut)
{
	bIsResolvingEndOfRoll = true;
	bPendingSevenOutWipe = bIsSevenOut;
	CurrentStepInterval = BaseStepIntervalSeconds;
	StepTimer = BaseStepIntervalSeconds; // Pop the first settled step right away
}

int64 UScoringQueueSubsystem::GetCurrentThrowTotalPayout() const
{
	const double RawTotal = static_cast<double>(RunningBaseChips) * static_cast<double>(RunningThrowMult) * static_cast<double>(RunningBetMult);
	return FMath::Max<int64>(0, FMath::RoundToInt64(RawTotal));
}

void UScoringQueueSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();

	// 1. Update & render 3D floating popups in the pit
	for (int32 i = ActivePopups.Num() - 1; i >= 0; --i)
	{
		FActiveScoringPopup& Popup = ActivePopups[i];
		Popup.RemainingTime -= DeltaTime;
		Popup.WorldLocation.Z += DeltaTime * 55.0f; // Float upward

		if (Popup.RemainingTime <= 0.0f)
		{
			ActivePopups.RemoveAt(i);
			continue;
		}

		if (World)
		{
			FColor PopupColor = FColor(0, 245, 212); // Cyan Base Chips
			switch (Popup.Layer)
			{
			case EScoringLayer::BaseChips:  PopupColor = FColor(0, 245, 212); break;
			case EScoringLayer::ThrowMult:  PopupColor = FColor(255, 0, 127); break;
			case EScoringLayer::BetMult:    PopupColor = FColor(255, 183, 3); break;
			case EScoringLayer::SevenOut:   PopupColor = FColor(255, 30, 66); break;
			case EScoringLayer::PointBonus: PopupColor = FColor(255, 215, 0); break;
			}
			DrawDebugString(World, Popup.WorldLocation, Popup.Text, nullptr, PopupColor, 0.0f, true, 1.35f);
		}
	}

	// 2. Pop sequential end-of-roll steps on the accelerating timer (NEVER an instant synchronous loop)
	if (bIsResolvingEndOfRoll)
	{
		if (PendingSteps.Num() > 0)
		{
			StepTimer += DeltaTime;
			if (StepTimer >= CurrentStepInterval)
			{
				StepTimer = 0.0f;
				PopAndExecuteNextStep();
				CurrentStepInterval = FMath::Max(MinStepIntervalSeconds, CurrentStepInterval * StepAccelerationFactor);
			}
		}
		else
		{
			bIsResolvingEndOfRoll = false;
			const int64 FinalPayout = bPendingSevenOutWipe ? 0 : GetCurrentThrowTotalPayout();
			OnScoringSequenceFinished.Broadcast(FinalPayout, bPendingSevenOutWipe);
		}
	}
	else
	{
		// Decay CRT overdrive intensity gently when idle
		NormalizedMultiplierIntensity = FMath::FInterpTo(NormalizedMultiplierIntensity, 0.05f, DeltaTime, 1.5f);
	}
}

void UScoringQueueSubsystem::PopAndExecuteNextStep()
{
	if (PendingSteps.Num() == 0)
	{
		return;
	}

	FScoringStep NextStep = PendingSteps[0];
	PendingSteps.RemoveAt(0);
	ApplyStepMath(NextStep);
}

void UScoringQueueSubsystem::ApplyStepMath(FScoringStep& Step)
{
	++PoppedStepCountThisThrow;
	// DESIGN.md: +1.0 semitone per trigger, wrapping/clamping at +24
	CurrentPitchSemitoneShift = FMath::Clamp(PoppedStepCountThisThrow, 1, 24);
	Step.AudioPitchIndex = CurrentPitchSemitoneShift;

	// Rising +1 semitone per popped step (12th root of 2 per semitone)
	if (StepPopSound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), StepPopSound, 0.85f, FMath::Pow(2.0f, CurrentPitchSemitoneShift / 12.0f));
	}

	RunningBaseChips = FMath::Max(0, RunningBaseChips + Step.BaseChipDelta);
	RunningThrowMult = FMath::Max(1.0f, RunningThrowMult + Step.ThrowMultDelta);
	RunningBetMult   = FMath::Max(1.0f, RunningBetMult + Step.BetMultAdditiveDelta);

	if (Step.MultiplierFactor > 0.0f && !FMath::IsNearlyEqual(Step.MultiplierFactor, 1.0f))
	{
		if (Step.Layer == EScoringLayer::ThrowMult)
		{
			RunningThrowMult *= Step.MultiplierFactor;
		}
		else
		{
			RunningBetMult *= Step.MultiplierFactor;
		}
	}

	if (Step.Layer == EScoringLayer::SevenOut)
	{
		RunningBaseChips = 0;
		RunningThrowMult = 1.0f;
		RunningBetMult = 1.0f;
	}

	UpdateMultiplierIntensityScalar();
	LastExecutedStep = Step;

	// Spawn floating 3D popup at source location
	FActiveScoringPopup Popup;
	Popup.Text = FString::Printf(TEXT("%s  [+%d st]"), *Step.StepLabel, CurrentPitchSemitoneShift);
	Popup.WorldLocation = Step.PopupWorldLocation.IsNearlyZero() && Step.SourceActor
		? (Step.SourceActor->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f))
		: (Step.PopupWorldLocation + FVector(0.0f, 0.0f, 25.0f));
	Popup.Layer = Step.Layer;
	Popup.RemainingTime = 1.45f;
	Popup.PitchSemitone = CurrentPitchSemitoneShift;
	ActivePopups.Add(Popup);

	// Highlight visual pulse around the SourceActor in the viewport
	if (UWorld* World = GetWorld())
	{
		if (Step.SourceActor)
		{
			FColor PulseColor = FColor(0, 245, 212);
			if (Step.Layer == EScoringLayer::ThrowMult) PulseColor = FColor(255, 0, 127);
			else if (Step.Layer == EScoringLayer::BetMult || Step.Layer == EScoringLayer::PointBonus) PulseColor = FColor(255, 183, 3);
			else if (Step.Layer == EScoringLayer::SevenOut) PulseColor = FColor(255, 30, 66);

			DrawDebugSphere(World, Step.SourceActor->GetActorLocation(), 36.0f, 16, PulseColor, false, 0.35f, 0, 3.0f);
		}
	}

	OnScoringStepExecuted.Broadcast(Step, RunningBaseChips, RunningThrowMult, RunningBetMult, NormalizedMultiplierIntensity);
}

void UScoringQueueSubsystem::UpdateMultiplierIntensityScalar()
{
	// DESIGN.md:
	// Tier 1 (0.0 - 0.2): 1x - 10x Mult
	// Tier 2 (0.2 - 0.6): 50x - 500x Mult
	// Tier 3 (0.6 - 1.0): 1,000x+ "Breaking the Table"
	const float CombinedMult = RunningThrowMult * RunningBetMult;
	if (CombinedMult <= 10.0f)
	{
		NormalizedMultiplierIntensity = FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 10.0f), FVector2D(0.05f, 0.20f), CombinedMult);
	}
	else if (CombinedMult <= 500.0f)
	{
		NormalizedMultiplierIntensity = FMath::GetMappedRangeValueClamped(FVector2D(10.0f, 500.0f), FVector2D(0.20f, 0.60f), CombinedMult);
	}
	else
	{
		NormalizedMultiplierIntensity = FMath::GetMappedRangeValueClamped(FVector2D(500.0f, 1500.0f), FVector2D(0.60f, 1.00f), CombinedMult);
	}
}
