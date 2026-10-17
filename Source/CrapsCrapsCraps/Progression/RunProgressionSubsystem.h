// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Progression/RunProgressionTypes.h"
#include "Progression/PDA_RunProgressionConfig.h"
#include "RunProgressionSubsystem.generated.h"

class ACrapsGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRunStateChanged, ERunProgressionState, NewState, ERunProgressionState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnteProgressChanged, int64, CurrentProgress, int64, TargetQuota);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRollsRemainingChanged, int32, RollsRemaining, int32, MaxRolls);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnteAdvanced, int32, NewAnteIndex, const FAnteConfig&, NewAnteConfig);

/**
 * World Subsystem orchestrating the overarching Roguelike Run Progression loop (Milestone B1).
 * Owns Ante state (RunStart -> ActiveAnte -> AnteCleared -> BetweenAntes -> RunVictory / RunDefeat),
 * roll allowance consumption, and qualifying winnings quota tracking.
 *
 * Strictly decoupled from table-level craps state machine (ACrapsGameState).
 */
UCLASS(BlueprintType)
class CRAPSCRAPSCRAPS_API URunProgressionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Starts or restarts a run from Ante 1 using InConfig (or default if null). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Progression")
	void StartNewRun(UPDA_RunProgressionConfig* InConfig = nullptr);

	/** Manually credits secured qualifying winnings toward Ante Progress. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Progression")
	void AwardQualifyingWinnings(int64 Amount);

	/** Consumes one valid physical roll from the allowance. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Progression")
	void ConsumeRoll();

	/** Advances from AnteCleared / BetweenAntes into the next ActiveAnte or RunVictory. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Progression")
	bool AdvanceToNextAnte();

	/** Binds event listeners to the authoritative CrapsGameState. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Progression")
	void BindToGameState(ACrapsGameState* InGameState);

	// --- Public Accessors ---

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	ERunProgressionState GetRunState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int32 GetCurrentAnteIndex() const { return CurrentAnteIndex; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int64 GetCurrentAnteProgress() const { return CurrentAnteProgress; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int64 GetCurrentAnteTargetQuota() const { return CurrentAnteConfig.TargetWinningsQuota; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int32 GetRollsRemaining() const { return RollsRemaining; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int32 GetRollAllowance() const { return CurrentAnteConfig.RollAllowance; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	const FAnteConfig& GetCurrentAnteConfig() const { return CurrentAnteConfig; }

	UFUNCTION(BlueprintPure, Category = "Craps|Progression")
	int32 GetTotalAntesCount() const;

	UPDA_RunProgressionConfig* GetActiveConfig() const { return ActiveConfig; }

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Craps|Progression")
	FOnRunStateChanged OnRunStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Progression")
	FOnAnteProgressChanged OnAnteProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Progression")
	FOnRollsRemainingChanged OnRollsRemainingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Progression")
	FOnAnteAdvanced OnAnteAdvanced;

protected:
	UFUNCTION()
	void HandleWinningsSecured(int64 SecuredAmount);

	UFUNCTION()
	void HandleRollSettledAuthoritative(int32 Die1Val, int32 Die2Val, int64 Payout);

	void TransitionToState(ERunProgressionState NewState);
	void CheckAnteCompletionOrDefeat();

	UPROPERTY(Transient)
	TObjectPtr<UPDA_RunProgressionConfig> ActiveConfig;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACrapsGameState> BoundGameState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Progression")
	ERunProgressionState CurrentState = ERunProgressionState::RunStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Progression")
	int32 CurrentAnteIndex = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Progression")
	int64 CurrentAnteProgress = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Progression")
	int32 RollsRemaining = 15;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Progression")
	FAnteConfig CurrentAnteConfig;

	UPROPERTY(Transient)
	bool bIsAdvancing = false;
};
