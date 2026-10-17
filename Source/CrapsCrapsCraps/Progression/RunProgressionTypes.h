// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunProgressionTypes.generated.h"

/**
 * State machine phases for the overarching roguelike run progression.
 * Kept strictly decoupled from table-level craps phases (ComeOutRoll vs PointEstablished).
 */
UENUM(BlueprintType)
enum class ERunProgressionState : uint8
{
	RunStart     UMETA(DisplayName = "Run Start"),
	ActiveAnte   UMETA(DisplayName = "Active Ante"),
	AnteCleared  UMETA(DisplayName = "Ante Cleared"),
	BetweenAntes UMETA(DisplayName = "Between Antes"),
	RunVictory   UMETA(DisplayName = "Run Victory"),
	RunDefeat    UMETA(DisplayName = "Run Defeat")
};

/**
 * Configuration definition for a single Ante tier in a run.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FAnteConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	int32 AnteIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	FString AnteTitle = TEXT("Ante 1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	FString AnteDescription = TEXT("Low-Stakes Warmup");

	/** Qualifying net winnings required to clear this ante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	int64 TargetWinningsQuota = 1000;

	/** Total physical throw allowance granted for this ante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	int32 RollAllowance = 15;
};
