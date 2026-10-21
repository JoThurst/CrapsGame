// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/CrapsTypes.h"
#include "CrapsGameState.generated.h"

class ABetZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTablePhaseChanged, ECrapsTablePhase, NewPhase, int32, NewPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBankrollChanged, int64, NewBankroll, int64, NewUnbankedPool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRollSettledAuthoritative, int32, Die1Value, int32, Die2Value, int64, AuthoritativePayout);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWinningsSecured, int64, SecuredWinningsAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBankedPayoutVisualsSignature, int64, ScoreAmount, int64, CashAmount);

/**
 * Authoritative Craps Table State Machine and Economy Tracker.
 * Orchestrates Come-Out Roll -> Point Established -> Point Hit / Seven-Out transitions,
 * evaluates all active ABetZone actors on the felt, and pushes sequential FScoringSteps
 * into UScoringQueueSubsystem.
 */
UCLASS(BlueprintType, Blueprintable)
class CRAPSCRAPSCRAPS_API ACrapsGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACrapsGameState();

	/**
	 * Evaluates a completed 2-die throw against Craps rules and all active ABetZone actors,
	 * queueing Layer 1 -> Layer 2 -> Layer 3 FScoringSteps into UScoringQueueSubsystem.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void ResolveSettledRoll(
		int32 Die1FaceValue,
		int32 Die1ChipYield,
		const FVector& Die1Location,
		int32 Die2FaceValue,
		int32 Die2ChipYield,
		const FVector& Die2Location,
		bool bHitBackWall,
		int32 TotalWallBounces);

	/**
	 * Explicit No-Roll handling (cocked dice, short throws, timeout, throw interrupted).
	 * Voids mid-roll hazard gains, preserves wagers, preserves roll allowance and bankroll.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void HandleNoRoll(const FString& Reason);

	/**
	 * Cashes out the Unbanked Combo Pool into the Player Bankroll ("Press Your Luck" cashout).
	 * Returns the amount banked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	int64 BankUnbankedPool();

	/** Restores standard default wagers across all active BetZones on the table without creating free money. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void PlaceDefaultSpreadBets();

	/** Authoritative wager placement. Debits PlayerBankroll by Amount and credits BetZone. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	bool PlaceWager(ABetZone* Zone, int64 Amount);

	/** Authoritative wager removal. Refunds uncommitted eligible wagers back to PlayerBankroll. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	bool RemoveWager(ABetZone* Zone, int64 Amount, int64& OutRefundedAmount);

	/** Returns true if the wager on Zone can be placed or removed given the table phase and bet type. */
	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	bool CanModifyWager(const ABetZone* Zone, bool bIsRemoval) const;

	/** Authoritative setter for testing and initialization. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void SetPlayerBankroll(int64 NewBankroll);

	/** Authoritative setter for testing and initialization. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void SetUnbankedPool(int64 NewPool);

	/** Authoritative state transition helper. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void SetTablePhase(ECrapsTablePhase NewPhase, int32 NewPoint);

	/** Registers a spawned ABetZone with the table state machine. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void RegisterBetZone(ABetZone* BetZone);

	/** Binds a specific TrinketInventoryComponent (used by tests or pawn initialization). */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void BindTrinketInventory(class UTrinketInventoryComponent* InTrinketInventory);

	/** Returns active TrinketInventoryComponent (either bound or resolved from PlayerPawn). */
	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	class UTrinketInventoryComponent* GetActiveTrinketInventory() const;

	// --- Public Accessors ---

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	ECrapsTablePhase GetTablePhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	int32 GetEstablishedPoint() const { return EstablishedPoint; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	int64 GetPlayerBankroll() const { return PlayerBankroll; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	int64 GetUnbankedPool() const { return UnbankedComboPool; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	int64 GetCurrentAnteQuota() const { return CurrentAnteQuota; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	int32 GetConsecutivePointsHit() const { return ConsecutivePointsHit; }

	UFUNCTION(BlueprintPure, Category = "Craps|GameState")
	FString GetLastRollOutcomeBanner() const { return LastRollOutcomeBanner; }

	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	void SetLastRollOutcomeBanner(const FString& InBanner) { LastRollOutcomeBanner = InBanner; }

	/** Returns the structured Balatro-style scoring breakdown for the last settled roll. */
	UFUNCTION(BlueprintPure, Category = "Craps|Scoring")
	const FRollScoringBreakdown& GetLastRollBreakdown() const { return LastRollBreakdown; }

	/** Returns the structured financial breakdown for spendable cash bankroll. */
	UFUNCTION(BlueprintPure, Category = "Craps|Economy")
	const FRollBankrollBreakdown& GetLastBankrollBreakdown() const { return LastBankrollBreakdown; }

	const TArray<TObjectPtr<ABetZone>>& GetActiveBetZones() const { return ActiveBetZones; }

	/** Refunds only legitimately refundable active wagers back to PlayerBankroll. */
	UFUNCTION(BlueprintCallable, Category = "Craps|GameState")
	int64 RefundAllEligibleWagers();

	UPROPERTY(BlueprintAssignable, Category = "Craps|GameState")
	FOnTablePhaseChanged OnTablePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craps|GameState")
	FOnBankrollChanged OnBankrollChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craps|GameState")
	FOnRollSettledAuthoritative OnRollSettledAuthoritative;

	UPROPERTY(BlueprintAssignable, Category = "Craps|GameState")
	FOnWinningsSecured OnWinningsSecured;

	UPROPERTY(BlueprintAssignable, Category = "Craps|GameState")
	FOnBankedPayoutVisualsSignature OnBankedPayoutVisuals;

	/**
	 * Option B Economy Toggle (Default = true):
	 * Separates Balatro combo score (filling Ante Progress) from Spendable Cash ($).
	 * If set to false, Option A (Unified Bankroll) is active where all combo chips are spendable.
	 * Option A is preserved in docs and comments for future exploration / high-roller mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	bool bEnableScoreCashSeparation = true;

	/** Cash dividend percentage (e.g. 0.05 = 5% of combo score) awarded to spendable bankroll on banking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	float ScoreToCashDividendRatio = 0.05f;

	/** Minimum cash dividend awarded when banking a winning unbanked pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	int64 MinCashDividendPerBank = 25;

	/**
	 * If true, any unbanked combo pool is automatically secured into Ante Progress and Cash Bankroll
	 * when the stickman pushes the dice back to the shooter (or before committing a new throw).
	 * Eliminates cognitive friction and prevents accidental roll wipeout while preserving manual [F] banking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	bool bAutoBankOnNextRoll = true;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleScoringQueueFinished(int64 FinalRollScore, bool bWasSevenOut);

	/** Current phase of the Craps table state machine (Puck OFF vs Puck ON). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	ECrapsTablePhase CurrentPhase = ECrapsTablePhase::ComeOutRoll;

	/** Active Point number (4, 5, 6, 8, 9, or 10) when CurrentPhase == PointEstablished; 0 when OFF. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	int32 EstablishedPoint = 0;

	/** Safe banked chips towards the current Pit Boss Ante Quota. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	int64 PlayerBankroll = 500;

	/** Structured Balatro-style scoring breakdown for the last settled roll. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Scoring")
	FRollScoringBreakdown LastRollBreakdown;

	/** Structured financial breakdown for spendable cash bankroll. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Economy")
	FRollBankrollBreakdown LastBankrollBreakdown;

	/** Unbanked combo pool riding on the table (wiped on Seven-Out!). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Economy")
	int64 UnbankedComboPool = 0;

	/** Target score required to defeat the current Pit Boss blind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Economy")
	int64 CurrentAnteQuota = 2500;

	/** Consecutive points hit during the current shooter's hot streak. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	int32 ConsecutivePointsHit = 0;

	/** Narrative outcome banner from the most recent roll. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	FString LastRollOutcomeBanner = TEXT("COME-OUT ROLL — Puck OFF");

	/** All active 3D BetZone actors registered on the table felt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	TArray<TObjectPtr<ABetZone>> ActiveBetZones;

	/** True while a roll is actively resolving. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|State")
	bool bIsResolvingRoll = false;

	/** Bound Left-Hand Trinket Inventory component (Milestone 3.1 & 3.2). */
	UPROPERTY(Transient)
	TWeakObjectPtr<class UTrinketInventoryComponent> BoundTrinketInventory;
};
