// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dice/DicePhysicsTestingHarness.h"
#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Dice/PDA_DiceFace.h"
#include "Pawn/ThrowStyles/ThrowStyles_Concrete.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PBDRigidsSolver.h"
#include "Async/TaskGraphInterfaces.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

FString FDiceHarnessSummary::ToFormattedLog() const
{
	FString Log;
	Log += FString::Printf(TEXT("=== DICE PHYSICS TELEMETRY SUMMARY ===\n"));
	Log += FString::Printf(TEXT("Total Throws: %d | Valid Throws: %d | No-Rolls: %d\n"), TotalThrows, ValidThrows, NoRollThrows);
	Log += FString::Printf(TEXT("Seven-Out Count: %d | Seven Frequency: %.2f%% (Fair Baseline: 16.67%%)\n"), SevenCount, SevenFrequency * 100.0f);
	Log += FString::Printf(TEXT("Hardways: %d | Avg Duration: %.2fs | Avg Bounces: %.1f | Back-Wall Hits: %d\n"),
		HardwayCount, AverageDurationSeconds, AverageBounces, BackWallHitCount);

	Log += TEXT("\n--- SUM DISTRIBUTION (2..12) ---\n");
	for (int32 Sum = 2; Sum <= 12; ++Sum)
	{
		const int32 Count = SumDistribution.FindRef(Sum);
		const float Pct = ValidThrows > 0 ? (float)Count / (float)ValidThrows * 100.0f : 0.0f;
		Log += FString::Printf(TEXT("Sum %2d: %3d (%5.1f%%) | %s\n"), Sum, Count, Pct, *FString::ChrN(FMath::RoundToInt(Pct / 2.0f), TEXT('#')));
	}

	Log += TEXT("\n--- INDIVIDUAL FACE DISTRIBUTIONS (1..6) ---\n");
	for (int32 Pip = 1; Pip <= 6; ++Pip)
	{
		const int32 C1 = Die1FaceDistribution.FindRef(Pip);
		const int32 C2 = Die2FaceDistribution.FindRef(Pip);
		const float Pct1 = ValidThrows > 0 ? (float)C1 / (float)ValidThrows * 100.0f : 0.0f;
		const float Pct2 = ValidThrows > 0 ? (float)C2 / (float)ValidThrows * 100.0f : 0.0f;
		Log += FString::Printf(TEXT("Face [%d] - Die1: %3d (%5.1f%%) | Die2: %3d (%5.1f%%)\n"), Pip, C1, Pct1, C2, Pct2);
	}

	return Log;
}

FString FDiceHarnessSummary::ToCsvString() const
{
	FString Csv = TEXT("ThrowIndex,Valid,Die1,Die2,Sum,SevenOut,Hardway,NoRollReason,DurationSec,Bounces,HitBackWall,Die1X,Die1Y,Die2X,Die2Y\n");
	for (const FSingleThrowTelemetry& T : Throws)
	{
		Csv += FString::Printf(TEXT("%d,%s,%d,%d,%d,%s,%s,\"%s\",%.3f,%d,%s,%.1f,%.1f,%.1f,%.1f\n"),
			T.ThrowIndex,
			T.bIsNoRoll ? TEXT("0") : TEXT("1"),
			T.Die1Value,
			T.Die2Value,
			T.TotalValue,
			T.bIsSevenOut ? TEXT("1") : TEXT("0"),
			T.bIsHardway ? TEXT("1") : TEXT("0"),
			*T.NoRollReason,
			T.DurationSeconds,
			T.TotalBounces,
			T.bHitBackWall ? TEXT("1") : TEXT("0"),
			T.Die1FinalLocation.X, T.Die1FinalLocation.Y,
			T.Die2FinalLocation.X, T.Die2FinalLocation.Y
		);
	}
	return Csv;
}

FDiceHarnessSummary UDicePhysicsTestingHarness::RunSimulationBatch(UWorld* World, const FDiceHarnessConfig& Config)
{
	FDiceHarnessSummary Summary;
	Summary.TotalThrows = Config.NumThrows;

	if (!World)
	{
		return Summary;
	}

	World->bShouldSimulatePhysics = true;


	Chaos::EThreadingModeTemp OriginalThreadingMode = Chaos::EThreadingModeTemp::TaskGraph;
	Chaos::FPhysicsSolver* Solver = nullptr;
	if (FPhysScene* PhysScene = World->GetPhysicsScene())
	{
		Solver = PhysScene->GetSolver();
		if (Solver)
		{
			OriginalThreadingMode = Solver->GetThreadingMode();
			Solver->SetThreadingMode_External(Chaos::EThreadingModeTemp::SingleThread);
		}
	}

	if (Config.RandomSeed != 0)
	{
		FMath::RandInit(Config.RandomSeed);
	}

	// 1. Ensure Craps Table exists in World
	ACrapsTableGrayboxActor* Table = nullptr;
	for (TActorIterator<ACrapsTableGrayboxActor> It(World); It; ++It)
	{
		Table = *It;
		break;
	}
	if (!Table)
	{
		FActorSpawnParameters TableParams;
		Table = World->SpawnActor<ACrapsTableGrayboxActor>(FVector::ZeroVector, FRotator::ZeroRotator, TableParams);
	}

	// 2. Ensure two ADieBase actors exist in World
	TArray<ADieBase*> Dice;
	for (TActorIterator<ADieBase> It(World); It; ++It)
	{
		Dice.Add(*It);
		if (Dice.Num() >= 2)
		{
			break;
		}
	}
	while (Dice.Num() < 2)
	{
		FActorSpawnParameters DieParams;
		DieParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ADieBase* NewDie = World->SpawnActor<ADieBase>(DieParams);
		if (NewDie)
		{
			Dice.Add(NewDie);
		}
		else
		{
			break;
		}
	}

	if (Dice.Num() < 2 || !Dice[0] || !Dice[1])
	{
		return Summary;
	}

	ADieBase* Die1 = Dice[0];
	ADieBase* Die2 = Dice[1];

	// Connect partner references for AvoidSevenOut rigging mode
	if (Die1->DiceRiggingComponent)
	{
		Die1->DiceRiggingComponent->PartnerDie = Die2;
	}
	if (Die2->DiceRiggingComponent)
	{
		Die2->DiceRiggingComponent->PartnerDie = Die1;
	}

	// 3. Create active throw style component
	UThrowStyleComponentBase* StyleComp = nullptr;
	switch (Config.ThrowArchetype)
	{
	case EThrowArchetype::TableSkimmer:
		StyleComp = NewObject<UThrowStyle_TableSkimmer>(GetTransientPackage());
		break;
	case EThrowArchetype::ParabolicLob:
		StyleComp = NewObject<UThrowStyle_ParabolicLob>(GetTransientPackage());
		break;
	case EThrowArchetype::BackWallWhip:
		StyleComp = NewObject<UThrowStyle_BackWallWhip>(GetTransientPackage());
		break;
	case EThrowArchetype::MechanicalPlunger:
		StyleComp = NewObject<UThrowStyle_MechanicalPlunger>(GetTransientPackage());
		break;
	default:
		StyleComp = NewObject<UThrowStyle_TableSkimmer>(GetTransientPackage());
		break;
	}

	// 4. Initialize face distribution maps
	for (int32 Sum = 2; Sum <= 12; ++Sum)
	{
		Summary.SumDistribution.Add(Sum, 0);
	}
	for (int32 Pip = 1; Pip <= 6; ++Pip)
	{
		Summary.Die1FaceDistribution.Add(Pip, 0);
		Summary.Die2FaceDistribution.Add(Pip, 0);
	}

	float TotalDuration = 0.0f;
	int32 TotalBouncesSum = 0;

	// 5. Execute Simulation Batch
	for (int32 ThrowIdx = 0; ThrowIdx < Config.NumThrows; ++ThrowIdx)
	{
		FSingleThrowTelemetry Throw;
		Throw.ThrowIndex = ThrowIdx + 1;

		// Staging positions: 190cm table-X, spaced 34cm apart along Y, 11cm height on felt
		Die1->EnterOnFeltSettingState();
		Die2->EnterOnFeltSettingState();
		Die1->SetActorLocation(FVector(190.0f, -17.0f, 11.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Die2->SetActorLocation(FVector(190.0f,  17.0f, 11.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Die1->SetActorRotation(FRotator::ZeroRotator, ETeleportType::TeleportPhysics);
		Die2->SetActorRotation(FRotator::ZeroRotator, ETeleportType::TeleportPhysics);

		// Apply face weighting / COM configurations
		Die1->EquippedFaceAssets = Config.Die1EquippedFaces;
		Die1->AdditionalLocalCOMOffset = Config.Die1AdditionalCOMOffset;
		Die1->UpdateMassAndCenterOfMass();

		Die2->EquippedFaceAssets = Config.Die2EquippedFaces;
		Die2->AdditionalLocalCOMOffset = Config.Die2AdditionalCOMOffset;
		Die2->UpdateMassAndCenterOfMass();

		// Apply rigging settings
		if (Die1->DiceRiggingComponent)
		{
			Die1->DiceRiggingComponent->bRiggingEnabled = Config.bEnableRigging;
			Die1->DiceRiggingComponent->DiceControlStat = Config.DiceControlStat;
			Die1->DiceRiggingComponent->RiggingMode = Config.RiggingMode;
			Die1->DiceRiggingComponent->DesiredTopFace = Config.DesiredTopFace;
		}
		if (Die2->DiceRiggingComponent)
		{
			Die2->DiceRiggingComponent->bRiggingEnabled = Config.bEnableRigging;
			Die2->DiceRiggingComponent->DiceControlStat = Config.DiceControlStat;
			Die2->DiceRiggingComponent->RiggingMode = Config.RiggingMode;
			Die2->DiceRiggingComponent->DesiredTopFace = Config.DesiredTopFace;
		}

		// Launch dice
		FThrowLaunchParams LaunchParams;
		LaunchParams.NormalizedPower = Config.NormalizedPower;
		LaunchParams.AimYawOffsetDegrees = Config.AimYawOffsetDegrees;
		LaunchParams.LateralSpin = Config.LateralSpin;
		LaunchParams.bSweetSpotRelease = Config.bSweetSpotRelease;
		TArray<ADieBase*> DiceToThrow = {Die1, Die2};
		IThrowStyleInterface::Execute_ExecuteThrow(StyleComp, LaunchParams, DiceToThrow);

		// Simulate forward frame-by-frame until settlement or timeout
		float FlightTime = 0.0f;
		const float Dt = Config.FixedSimulationTimeStep;
		float MaxX = 190.0f;

		while (FlightTime < Config.MaxFlightTimePerThrowSeconds)
		{
			World->Tick(ELevelTick::LEVELTICK_All, Dt);
			GFrameCounter++;
			GFrameNumber++;

			Die1->Tick(Dt);
			Die2->Tick(Dt);

			FlightTime += Dt;

			MaxX = FMath::Max(MaxX, FMath::Max(Die1->GetActorLocation().X, Die2->GetActorLocation().X));

			if (Die1->HasSettled() && Die2->HasSettled())
			{
				break;
			}
		}

		Throw.DurationSeconds = FlightTime;
		Throw.Die1FinalLocation = Die1->GetActorLocation();
		Throw.Die2FinalLocation = Die2->GetActorLocation();
		Throw.TotalBounces = Die1->GetCurrentThrowBounceCount() + Die2->GetCurrentThrowBounceCount();
		Throw.bHitBackWall = (Die1->GetActorLocation().X > 1400.0f || Die2->GetActorLocation().X > 1400.0f || Throw.TotalBounces >= 3);

		UE_LOG(LogTemp, Display, TEXT("HARNESS THROW %d: FlightTime=%.3f, MaxX=%.1f, Settled1=%d, Settled2=%d, Loc1=%s, Loc2=%s, Vel1=%s, Vel2=%s"),
			ThrowIdx, FlightTime, MaxX, Die1->HasSettled() ? 1 : 0, Die2->HasSettled() ? 1 : 0,
			*Die1->GetActorLocation().ToString(), *Die2->GetActorLocation().ToString(),
			Die1->DieMesh ? *Die1->DieMesh->GetPhysicsLinearVelocity().ToString() : TEXT("None"),
			Die2->DieMesh ? *Die2->DieMesh->GetPhysicsLinearVelocity().ToString() : TEXT("None"));

		// Validate throw outcomes
		const bool bSettled = Die1->HasSettled() && Die2->HasSettled();
		const bool bCocked = Die1->IsCocked() || Die2->IsCocked();
		const bool bShortThrow = MaxX < 650.0f;

		if (!bSettled)
		{
			Throw.bIsNoRoll = true;
			Throw.NoRollReason = TEXT("Simulation Timeout Unresolved");
			++Summary.NoRollThrows;
		}
		else if (bCocked)
		{
			Throw.bIsNoRoll = true;
			Throw.NoRollReason = TEXT("Cocked Die Alignment");
			++Summary.NoRollThrows;
		}
		else if (bShortThrow)
		{
			Throw.bIsNoRoll = true;
			Throw.NoRollReason = TEXT("Short Throw");
			++Summary.NoRollThrows;
		}
		else
		{
			// Valid roll
			Throw.bIsNoRoll = false;
			Throw.Die1Value = Die1->GetTopFaceValue();
			Throw.Die2Value = Die2->GetTopFaceValue();
			Throw.TotalValue = Throw.Die1Value + Throw.Die2Value;
			Throw.bIsSevenOut = (Throw.TotalValue == 7);
			Throw.bIsHardway = (Throw.Die1Value == Throw.Die2Value &&
				(Throw.TotalValue == 4 || Throw.TotalValue == 6 || Throw.TotalValue == 8 || Throw.TotalValue == 10));

			++Summary.ValidThrows;
			if (Throw.bIsSevenOut)
			{
				++Summary.SevenCount;
			}
			if (Throw.bIsHardway)
			{
				++Summary.HardwayCount;
			}
			if (Throw.bHitBackWall)
			{
				++Summary.BackWallHitCount;
			}

			Summary.SumDistribution.FindOrAdd(Throw.TotalValue)++;
			Summary.Die1FaceDistribution.FindOrAdd(Throw.Die1Value)++;
			Summary.Die2FaceDistribution.FindOrAdd(Throw.Die2Value)++;

			TotalDuration += Throw.DurationSeconds;
			TotalBouncesSum += Throw.TotalBounces;
		}

		Summary.Throws.Add(Throw);
	}

	if (Summary.ValidThrows > 0)
	{
		Summary.SevenFrequency = (float)Summary.SevenCount / (float)Summary.ValidThrows;
		Summary.AverageDurationSeconds = TotalDuration / (float)Summary.ValidThrows;
		Summary.AverageBounces = (float)TotalBouncesSum / (float)Summary.ValidThrows;
	}

	if (Solver)
	{
		Solver->SetThreadingMode_External(OriginalThreadingMode);
	}

	return Summary;
}

bool UDicePhysicsTestingHarness::ExportTelemetryToCsv(const FDiceHarnessSummary& Summary, const FString& Filename)
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Profiling");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	const FString FullPath = Directory / Filename;
	return FFileHelper::SaveStringToFile(Summary.ToCsvString(), *FullPath);
}
