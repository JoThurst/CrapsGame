// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/CrapsTypes.h"
#include "Dice/DieBase.h"
#include "DicePhysicsTestingHarness.generated.h"

class UWorld;
class UPDA_DiceFace;
class ACrapsTableGrayboxActor;

/**
 * Configuration options for automated physical throw batch simulation.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FDiceHarnessConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness")
	EThrowArchetype ThrowArchetype = EThrowArchetype::TableSkimmer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness")
	float NormalizedPower = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness")
	float AimYawOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness")
	float LateralSpin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness")
	bool bSweetSpotRelease = false;

	//~ Rigging Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Rigging")
	bool bEnableRigging = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Rigging")
	float DiceControlStat = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Rigging")
	EDiceRiggingMode RiggingMode = EDiceRiggingMode::TargetSetFace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Rigging")
	EDieFaceDirection DesiredTopFace = EDieFaceDirection::PlusZ;

	//~ Weighted Faces / COM Manipulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Weighting")
	TMap<EDieFaceDirection, TObjectPtr<UPDA_DiceFace>> Die1EquippedFaces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Weighting")
	TMap<EDieFaceDirection, TObjectPtr<UPDA_DiceFace>> Die2EquippedFaces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Weighting")
	FVector Die1AdditionalCOMOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Weighting")
	FVector Die2AdditionalCOMOffset = FVector::ZeroVector;

	//~ Simulation batch settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Batch")
	int32 NumThrows = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Batch")
	float MaxFlightTimePerThrowSeconds = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Batch")
	float FixedSimulationTimeStep = 0.016667f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Harness|Batch")
	int32 RandomSeed = 42;
};

/**
 * Detailed telemetry collected for an individual physical throw.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FSingleThrowTelemetry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 ThrowIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 Die1Value = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 Die2Value = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 TotalValue = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	bool bIsSevenOut = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	bool bIsHardway = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	bool bIsNoRoll = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	FString NoRollReason;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	float DurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 TotalBounces = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	bool bHitBackWall = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	FVector Die1FinalLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	FVector Die2FinalLocation = FVector::ZeroVector;
};

/**
 * Aggregated statistics and distributions over a simulated batch of throws.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FDiceHarnessSummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 TotalThrows = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 ValidThrows = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 NoRollThrows = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 SevenCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	float SevenFrequency = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 HardwayCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	float AverageDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	float AverageBounces = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	int32 BackWallHitCount = 0;

	/** Sum distribution: maps sum (2..12) to count. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	TMap<int32, int32> SumDistribution;

	/** Individual face distribution (1..6) for Die 1. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	TMap<int32, int32> Die1FaceDistribution;

	/** Individual face distribution (1..6) for Die 2. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	TMap<int32, int32> Die2FaceDistribution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Harness")
	TArray<FSingleThrowTelemetry> Throws;

	FString ToFormattedLog() const;
	FString ToCsvString() const;
};

/**
 * Developer-only physical throw simulation and telemetry harness.
 * Drives real Chaos physics simulations to measure throw archetypes,
 * weighted dice biases, and rigging balance without manual input.
 */
UCLASS(BlueprintType)
class CRAPSCRAPSCRAPS_API UDicePhysicsTestingHarness : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Executes repeated physical throws inside World using real Chaos physics simulation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|Harness")
	static FDiceHarnessSummary RunSimulationBatch(UWorld* World, const FDiceHarnessConfig& Config);

	/**
	 * Exports the batch summary to a CSV file in Saved/Profiling/.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|Harness")
	static bool ExportTelemetryToCsv(const FDiceHarnessSummary& Summary, const FString& Filename);
};
