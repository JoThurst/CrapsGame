// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/CrapsTypes.h"
#include "Items/PDA_Trinket.h"
#include "TrinketInventoryComponent.generated.h"

class UScoringQueueSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTrinketEquippedSignature, UPDA_Trinket*, Trinket, ETrinketSlotType, SlotType, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTrinketUnequippedSignature, UPDA_Trinket*, Trinket, ETrinketSlotType, SlotType, int32, SlotIndex);

/**
 * Authoritative Left-Hand Trinket Inventory Component (Milestones 3.1 & 3.2).
 * Manages 4 Finger Rings, 1 Wristwear (Active), and 2 Rail Props.
 * Evaluates equipped trinket synergies during roll resolution and coordinates hand visuals.
 */
UCLASS(ClassGroup=(Craps), meta=(BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UTrinketInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTrinketInventoryComponent();

	static constexpr int32 MaxFingerRings = 4;
	static constexpr int32 MaxWristwear = 1;
	static constexpr int32 MaxRailProps = 2;

	/** Authoritative equip method. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	bool EquipTrinket(UPDA_Trinket* Trinket, int32 SlotIndex = -1);

	/** Authoritative unequip method by data asset pointer. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	bool UnequipTrinket(UPDA_Trinket* Trinket);

	/** Authoritative unequip method by slot type and index. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	bool UnequipSlot(ETrinketSlotType SlotType, int32 SlotIndex = 0);

	/** Clears all equipped trinkets across all slots. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	void ClearAllTrinkets();

	/** Returns true if this specific trinket is currently equipped in any slot. */
	UFUNCTION(BlueprintPure, Category = "Craps|Trinkets")
	bool HasTrinket(const UPDA_Trinket* Trinket) const;

	/** Returns all non-null equipped trinkets. */
	UFUNCTION(BlueprintPure, Category = "Craps|Trinkets")
	TArray<UPDA_Trinket*> GetAllEquippedTrinkets() const;

	/** Returns the trinket at a specific slot (or nullptr if empty/invalid). */
	UFUNCTION(BlueprintPure, Category = "Craps|Trinkets")
	UPDA_Trinket* GetEquippedTrinket(ETrinketSlotType SlotType, int32 SlotIndex = 0) const;

	/** Returns the number of currently equipped trinkets. */
	UFUNCTION(BlueprintPure, Category = "Craps|Trinkets")
	int32 GetEquippedCount() const;

	/**
	 * Evaluates all equipped trinkets against the current settled roll context,
	 * enqueuing presentation scoring steps into UScoringQueueSubsystem and returning aggregated bonuses.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	void EvaluateTrinketsForRoll(
		const FRollEvaluationContext& Context,
		UScoringQueueSubsystem* ScoringQueue,
		int32& OutTotalBonusChips,
		float& OutTotalBonusThrowMult,
		float& OutTotalBetMultFactor);

	/** Checks if any equipped trinket averts a Seven-Out (e.g. Onyx Claw Miracle Seven). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Trinkets")
	bool CheckMiracleSevenAverted(
		const FRollEvaluationContext& Context,
		FScoringStep& OutScoringStep,
		int32& OutBonusBaseChips) const;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Trinkets|Events")
	FOnTrinketEquippedSignature OnTrinketEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Trinkets|Events")
	FOnTrinketUnequippedSignature OnTrinketUnequipped;

protected:
	virtual void BeginPlay() override;

	/** Sockets: 4 Finger Rings */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Trinkets|Slots")
	TArray<TObjectPtr<UPDA_Trinket>> EquippedFingerRings;

	/** Sockets: 1 Wristwear (Active) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Trinkets|Slots")
	TObjectPtr<UPDA_Trinket> EquippedWristwear = nullptr;

	/** Sockets: 2 Rail Props */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Trinkets|Slots")
	TArray<TObjectPtr<UPDA_Trinket>> EquippedRailProps;
};
