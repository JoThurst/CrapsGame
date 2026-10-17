// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Progression/RunProgressionTypes.h"
#include "PDA_RunProgressionConfig.generated.h"

/**
 * Primary Data Asset configuring the 8-Ante progression ladder, roll limits,
 * quotas, and starting parameters for a roguelike run.
 */
UCLASS(BlueprintType)
class CRAPSCRAPSCRAPS_API UPDA_RunProgressionConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPDA_RunProgressionConfig();

	/** Default starting spendable bankroll for a fresh run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	int64 StartingSpendableBankroll = 500;

	/** Ordered sequence of Ante tiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Progression")
	TArray<FAnteConfig> Antes;

	/** Returns the Ante config for the given 1-based index, or a fallback if out of bounds. */
	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	FAnteConfig GetAnteConfig(int32 AnteIndex) const;

	/** Total number of configured antes. */
	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int32 GetTotalAntes() const { return Antes.Num(); }
};
