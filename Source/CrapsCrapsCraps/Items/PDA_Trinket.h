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

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Trinket"), GetFName());
	}
};
