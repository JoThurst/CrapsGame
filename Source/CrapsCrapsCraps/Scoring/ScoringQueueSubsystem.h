// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/CrapsTypes.h"
#include "ScoringQueueSubsystem.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnScoringStepExecutedSignature,
	const FScoringStep&, Step,
	int32, RunningBaseChips,
	float, RunningThrowMult,
	float, RunningBetMult,
	float, NormalizedMultiplierIntensity
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnScoringSequenceFinishedSignature,
	int64, FinalThrowPayout,
	bool, bWasSevenOut
);

/**
 * Active floating 3D popup rendered in the pit & HUD when each FScoringStep pops.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FActiveScoringPopup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Craps|Scoring")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "Craps|Scoring")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Craps|Scoring")
	EScoringLayer Layer = EScoringLayer::BaseChips;

	UPROPERTY(BlueprintReadOnly, Category = "Craps|Scoring")
	float RemainingTime = 1.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Craps|Scoring")
	int32 PitchSemitone = 0;
};

/**
 * Sequential Balatro-Style Scoring Queue Subsystem (Task 2.1).
 *
 * Non-negotiable architecture rules from GEMINI.md & DESIGN.md:
 * - NEVER resolve end-of-roll scoring in an instant synchronous for-loop.
 * - Every triggered modifier (Die Face, Hazard Impact, Felt Bet Zone, Left-Hand Trinket)
 *   pushes an FScoringStep into UScoringQueueSubsystem.
 * - Pops and executes steps on an accelerating timer (0.25s base interval, accelerating per step)
 *   to compute: Total Payout = (Base Chips [Cyan]) * (Throw Mult [Magenta]) * (Bet Mult [Gold]).
 * - Updates NormalizedMultiplierIntensity [0.0 .. 1.0] in real time for M_PP_CRTOverdrive
 *   and increments PitchSemitoneShift (+1.0 semitone per trigger, clamping/wrapping at +24).
 */
UCLASS()
class CRAPSCRAPSCRAPS_API UScoringQueueSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UScoringQueueSubsystem();

	//~ Begin UTickableWorldSubsystem Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }
	//~ End UTickableWorldSubsystem Interface

	//~ Queue Timing Configuration
	/** Base interval (seconds) between popped FScoringSteps at the start of a sequence (GEMINI.md: 0.25s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float BaseStepIntervalSeconds = 0.25f;

	/** Acceleration factor applied per popped step so long combo chains speed up excitingly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float StepAccelerationFactor = 0.90f;

	/** Minimum interval floor (seconds) at maximum combo acceleration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float MinStepIntervalSeconds = 0.065f;

	/** Sound played on every popped step; pitch climbs +1 semitone per step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring|Audio")
	TObjectPtr<USoundBase> StepPopSound;

	//~ Live Running Totals for the Current Throw
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	int32 RunningBaseChips = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	float RunningThrowMult = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	float RunningBetMult = 1.0f;

	/** Current step pitch shift in semitones (+1 per step, wrapping at +24 per DESIGN.md). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	int32 CurrentPitchSemitoneShift = 0;

	/** Normalized scalar [0.0 .. 1.0] driving M_PP_CRTOverdrive shader tiers (Tier 1: 0-0.2, Tier 2: 0.2-0.6, Tier 3: 0.6-1.0). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	float NormalizedMultiplierIntensity = 0.0f;

	/** Most recently popped FScoringStep for HUD highlight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	FScoringStep LastExecutedStep;

	/** Active floating 3D popups for HUD/viewport display. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	TArray<FActiveScoringPopup> ActivePopups;

	//~ Delegates
	UPROPERTY(BlueprintAssignable, Category = "Craps|Scoring|Events")
	FOnScoringStepExecutedSignature OnScoringStepExecuted;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Scoring|Events")
	FOnScoringSequenceFinishedSignature OnScoringSequenceFinished;

	//~ Public API
	/** Resets the current throw's running Base Chips (0), Throw Mult (1.0), Bet Mult (1.0), and clears the queue. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void BeginNewThrowScoringSession();

	/** Voids and purges all in-flight scoring steps, popups, and multipliers from an interrupted or invalid throw. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void VoidCurrentThrowScoring();

	/** Enqueues a single FScoringStep to be popped sequentially on the dynamic timer. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void EnqueueScoringStep(const FScoringStep& Step);

	/** Convenience helper to enqueue a mid-roll collision step (Base Chips or Throw Mult) and execute immediately or via queue. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void EnqueueMidRollCollisionStep(
		AActor* SourceActor,
		const FVector& ImpactLocation,
		const FString& Label,
		int32 ChipDelta,
		float ThrowMultDelta
	);

	/** Starts resolving the queued end-of-roll sequence once dice have settled. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void StartEndOfRollResolution(bool bIsSevenOut);

	/** Flushes all remaining pending steps in the queue immediately, finishing resolution. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Scoring")
	void FlushQueue();

	/** Returns the current calculated payout: Round((Base Chips) * (Throw Mult) * (Bet Mult)). */
	UFUNCTION(BlueprintPure, Category = "Craps|Scoring")
	int64 GetCurrentThrowTotalPayout() const;

	/** Returns the normalized CRT overdrive shader scalar [0.0 .. 1.0] based on combined multiplier. */
	UFUNCTION(BlueprintPure, Category = "Craps|Scoring")
	float GetNormalizedIntensity() const { return NormalizedMultiplierIntensity; }

	/** Returns true while the end-of-roll sequential queue is actively popping steps. */
	UFUNCTION(BlueprintPure, Category = "Craps|Scoring")
	bool IsResolvingQueue() const { return bIsResolvingEndOfRoll && PendingSteps.Num() > 0; }

	/** Returns the number of steps waiting in the queue. */
	UFUNCTION(BlueprintPure, Category = "Craps|Scoring")
	int32 GetPendingStepCount() const { return PendingSteps.Num(); }

private:
	void PopAndExecuteNextStep();
	void ApplyStepMath(FScoringStep& Step);
	void UpdateMultiplierIntensityScalar();

	TArray<FScoringStep> PendingSteps;
	bool bIsResolvingEndOfRoll = false;
	bool bPendingSevenOutWipe = false;
	float StepTimer = 0.0f;
	float CurrentStepInterval = 0.25f;
	int32 PoppedStepCountThisThrow = 0;
};
