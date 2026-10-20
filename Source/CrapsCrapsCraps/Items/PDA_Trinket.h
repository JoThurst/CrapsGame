// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/CrapsTypes.h"
#include "PDA_Trinket.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 * Data Asset defining a Left-Hand collectible trinket (Rings, Wristwear, Rail Props).
 * Trinkets grant passive or active synergy bonuses to UScoringQueueSubsystem (Task 3.1 & 3.2).
 */
UCLASS(BlueprintType, Const)
class CRAPSCRAPSCRAPS_API UPDA_Trinket : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPDA_Trinket();

	/** Display name of the trinket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|General")
	FText TrinketName = FText::FromString(TEXT("New Trinket"));

	/** Narrative description and mechanical explanation of this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|General", meta = (MultiLine = "true"))
	FText Description = FText::FromString(TEXT("Trinket description"));

	/** Slot type on the shooter rig (Finger Ring, Active Wristwear, Rail Prop). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|General")
	ETrinketSlotType SlotType = ETrinketSlotType::FingerRing;

	/** GameplayTag condition required to activate this trinket during rolls (e.g. Craps.Trigger.OnHardway, Craps.Trigger.OnWallBounce). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Trigger")
	FGameplayTag TriggerConditionTag;

	/** Which scoring layer of the Balatro equation this trinket primarily modifies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Scoring")
	EScoringLayer TargetScoringLayer = EScoringLayer::ThrowMult;

	/** Flat bonus chips awarded when triggered (Layer 1 - Cyan). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Scoring")
	int32 BonusBaseChips = 0;

	/** Additive throw multiplier awarded when triggered (Layer 2 - Magenta). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Scoring")
	float BonusThrowMult = 0.0f;

	/** Multiplicative factor applied to bet payout or throw mult (Layer 3 - Gold). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Scoring")
	float BetMultFactor = 1.0f;

	/** Visual mesh attached to the hand/ring socket or rail prop socket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Visuals")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	/** UI Icon for shop inspection and inventory HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Visuals")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** If true, this trinket applies distinct bonuses for Odd vs. Even roll sums (e.g. Neon Signet). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	bool bSplitOddEven = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	float OddSumThrowMult = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	int32 EvenSumBaseChips = 0;

	/** If true, this trinket scales bonuses per table/bumper bounce (e.g. Brass Knuckles). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	bool bScaleWithBounces = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	int32 ChipsPerBounce = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	float ThrowMultPerBounce = 0.0f;

	/** If true, rolling 7 with enough hazards converts Seven-Out into a Miracle Seven (e.g. Onyx Claw). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	bool bIsMiracleSevenTrinket = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	int32 MinHazardsForMiracleSeven = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinket|Special")
	int32 MiracleSevenBonusChips = 150;

	/** Evaluates whether this trinket triggers under the current roll context, producing an FScoringStep if so. */
	virtual bool EvaluateTrinket(
		const FRollEvaluationContext& Context,
		FScoringStep& OutScoringStep,
		int32& OutBonusBaseChips,
		float& OutBonusThrowMult,
		float& OutBetMultFactor) const;

	/** Checks if this trinket averts Seven-Out (Miracle Seven). */
	virtual bool CheckMiracleSevenAverted(
		const FRollEvaluationContext& Context,
		FScoringStep& OutScoringStep,
		int32& OutBonusBaseChips) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Trinket"), GetFName());
	}
};
