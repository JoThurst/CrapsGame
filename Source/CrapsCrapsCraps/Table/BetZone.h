// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "Core/CrapsTypes.h"
#include "BetZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

/**
 * Data Asset defining a Balatro-style upgrade applied to a specific Craps bet archetype
 * (e.g., "Pass Line Level 3", "Iron Cross Field", "Supercharged Hardways").
 */
UCLASS(BlueprintType, Const)
class CRAPSCRAPSCRAPS_API UPDA_BetUpgrade : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Target bet archetype upgraded by this asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BetUpgrade")
	ECrapsBetType TargetBetType = ECrapsBetType::PassLine;

	/** Display name of the bet upgrade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BetUpgrade")
	FText UpgradeTitle = FText::FromString(TEXT("Upgraded Pass Line"));

	/** Upgrade level (1 = Base, 2+ = Upgraded). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BetUpgrade", meta = (ClampMin = "1"))
	int32 UpgradeLevel = 1;

	/** Bonus Layer 1 Base Chips added whenever this bet hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BetUpgrade|Scoring")
	int64 BonusBaseChips = 25;

	/** Bonus Layer 3 Bet Multiplier added to the base payout odds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BetUpgrade|Scoring")
	float BonusBetMultiplier = 1.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("BetUpgrade"), GetFName());
	}
};

/**
 * Interactive 3D Felt Betting Zone on the Craps table.
 * Evaluates standard Craps rules + roguelike Bet Upgrades against settled dice rolls,
 * generating Layer 3 (Bet Mult - Gold #FFB703) and Layer 1 (Chip Bonus - Cyan #00F5D4) scoring steps.
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ABetZone : public AActor
{
	GENERATED_BODY()

public:
	ABetZone();

	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Configures this bet zone's archetype, title, default payout odds, and starting chip stack. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void InitializeZone(ECrapsBetType InBetType, const FString& InDisplayName, float InBaseMultiplier, int64 InInitialWager);

	/** Places additional chips onto this betting zone. */
	/** Resizes the felt zone (half extents in cm along X/Y) and refreshes its visuals. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void SetZoneHalfExtent(float HalfExtentX, float HalfExtentY);

	/** Rebuilds pad, border and text so the zone reads correctly in the editor and in game. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void RefreshVisuals();

	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void AddWager(int64 AdditionalChips);

	/** Sets placed wager directly and refreshes visuals. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void SetPlacedWager(int64 InWager);

	/** Clears all wagered chips from this zone (e.g., on Seven-Out or losing one-roll bet). */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void ClearWager();

	/**
	 * Evaluates a settled roll (Die1 + Die2) against this Bet Zone's rules for the current Table Phase.
	 * @param Die1Val          Top face value of Die 1 (1-6)
	 * @param Die2Val          Top face value of Die 2 (1-6)
	 * @param CurrentPhase     ComeOutRoll or PointEstablished
	 * @param EstablishedPoint Current table point (4, 5, 6, 8, 9, 10) if in PointEstablished
	 * @param OutScoringStep   Populated FScoringStep if this bet hits
	 * @param bOutClearBet     True if this bet lost or resolved and should be cleared/reset
	 * @return True if this bet won and generated a valid OutScoringStep
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	bool EvaluateRoll(
		int32 Die1Val,
		int32 Die2Val,
		ECrapsTablePhase CurrentPhase,
		int32 EstablishedPoint,
		FScoringStep& OutScoringStep,
		bool& bOutClearBet);

	/** Triggers a transient high-intensity visual pulse when this Bet Zone scores. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void TriggerWinPulse();

	// --- Public Accessors ---

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	ECrapsBetType GetBetType() const { return BetType; }

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	int64 GetPlacedWager() const { return PlacedWager; }

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	int64 GetDefaultWager() const { return DefaultWager; }

	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void SetDefaultWager(int64 InDefaultWager);

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	FString GetZoneName() const { return ZoneDisplayName; }

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	float GetEffectiveBetMultiplier() const;

	/** Sets whether the player's cursor is currently hovering over this bet zone. */
	UFUNCTION(BlueprintCallable, Category = "Craps|BetZone")
	void SetHovered(bool bInHovered);

	UFUNCTION(BlueprintPure, Category = "Craps|BetZone")
	bool IsHovered() const { return bIsHovered; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Craps|BetZone")
	bool bIsHovered = false;

	/** Collision bounds for clicking/placing chips and visualizing the felt zone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ZoneBounds = nullptr;

	/** Thin glowing felt pad mesh representing the zone on the table. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ZonePadMesh = nullptr;

	/** Slightly larger glowing frame under the pad so the zone outline is visible on the felt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ZoneBorderMesh = nullptr;

	/** Flat felt label showing the zone name (visible in the editor before Play). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> ZoneTitleText = nullptr;

	/** Flat felt label showing wager and payout multiplier. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> ZoneOddsText = nullptr;

	/** Physical 3D visual chip stack rendered on the felt when PlacedWager > 0. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UInstancedStaticMeshComponent> ChipStackMesh = nullptr;

	/** Half extents (cm) of the felt zone on the table plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	FVector2D ZoneHalfExtent = FVector2D(55.0f, 110.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PadMID = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BorderMID = nullptr;

	/** Bet archetype represented by this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	ECrapsBetType BetType = ECrapsBetType::PassLine;

	/** Human-readable label rendered on the synth-noir vector felt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	FString ZoneDisplayName = TEXT("PASS LINE");

	/** Base Layer 3 Bet Multiplier when this zone wins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	float BaseBetMultiplier = 2.0f;

	/** Current chips wagered on this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	int64 PlacedWager = 0;

	/** Target/default wager for this zone when default spread bets are placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	int64 DefaultWager = 50;

	/** Optional data asset upgrading this bet zone's level and payout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|BetZone")
	TObjectPtr<UPDA_BetUpgrade> ActiveUpgrade = nullptr;

private:
	/** Remaining seconds of gold win-pulse highlight. */
	float WinPulseTimer = 0.0f;
};
