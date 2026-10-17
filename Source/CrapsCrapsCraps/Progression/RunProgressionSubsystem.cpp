// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Progression/RunProgressionSubsystem.h"
#include "Table/CrapsGameState.h"
#include "Engine/World.h"

void URunProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!ActiveConfig)
	{
		ActiveConfig = NewObject<UPDA_RunProgressionConfig>(this);
	}

	CurrentAnteConfig = ActiveConfig->GetAnteConfig(1);
}

void URunProgressionSubsystem::Deinitialize()
{
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnWinningsSecured.RemoveAll(this);
		BoundGameState->OnRollSettledAuthoritative.RemoveAll(this);
		BoundGameState.Reset();
	}

	Super::Deinitialize();
}

void URunProgressionSubsystem::BindToGameState(ACrapsGameState* InGameState)
{
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnWinningsSecured.RemoveAll(this);
		BoundGameState->OnRollSettledAuthoritative.RemoveAll(this);
	}

	BoundGameState = InGameState;

	if (InGameState)
	{
		InGameState->OnWinningsSecured.AddDynamic(this, &URunProgressionSubsystem::HandleWinningsSecured);
		InGameState->OnRollSettledAuthoritative.AddDynamic(this, &URunProgressionSubsystem::HandleRollSettledAuthoritative);
	}
}

void URunProgressionSubsystem::StartNewRun(UPDA_RunProgressionConfig* InConfig)
{
	if (InConfig)
	{
		ActiveConfig = InConfig;
	}
	else if (!ActiveConfig)
	{
		ActiveConfig = NewObject<UPDA_RunProgressionConfig>(this);
	}

	CurrentAnteIndex = 1;
	CurrentAnteConfig = ActiveConfig->GetAnteConfig(CurrentAnteIndex);
	CurrentAnteProgress = 0;
	RollsRemaining = CurrentAnteConfig.RollAllowance;
	bIsAdvancing = false;

	if (BoundGameState.IsValid())
	{
		BoundGameState->SetPlayerBankroll(ActiveConfig->StartingSpendableBankroll);
		BoundGameState->SetUnbankedPool(0);
		BoundGameState->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);
		BoundGameState->RefundAllEligibleWagers();
	}

	TransitionToState(ERunProgressionState::ActiveAnte);

	OnAnteAdvanced.Broadcast(CurrentAnteIndex, CurrentAnteConfig);
	OnAnteProgressChanged.Broadcast(CurrentAnteProgress, CurrentAnteConfig.TargetWinningsQuota);
	OnRollsRemainingChanged.Broadcast(RollsRemaining, CurrentAnteConfig.RollAllowance);
}

void URunProgressionSubsystem::AwardQualifyingWinnings(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	if (CurrentState != ERunProgressionState::ActiveAnte && CurrentState != ERunProgressionState::AnteCleared)
	{
		return;
	}

	CurrentAnteProgress += Amount;
	OnAnteProgressChanged.Broadcast(CurrentAnteProgress, CurrentAnteConfig.TargetWinningsQuota);

	CheckAnteCompletionOrDefeat();
}

void URunProgressionSubsystem::ConsumeRoll()
{
	if (CurrentState != ERunProgressionState::ActiveAnte)
	{
		return;
	}

	RollsRemaining = FMath::Max(0, RollsRemaining - 1);
	OnRollsRemainingChanged.Broadcast(RollsRemaining, CurrentAnteConfig.RollAllowance);

	CheckAnteCompletionOrDefeat();
}

void URunProgressionSubsystem::CheckAnteCompletionOrDefeat()
{
	if (CurrentState == ERunProgressionState::ActiveAnte)
	{
		// 1. Check for Ante Quota Met
		if (CurrentAnteProgress >= CurrentAnteConfig.TargetWinningsQuota)
		{
			TransitionToState(ERunProgressionState::AnteCleared);
			return;
		}

		// 2. Check for Roll Allowance Exhaustion
		if (RollsRemaining <= 0)
		{
			// Give the player an opportunity to bank if unbanked pool could fulfill the quota
			const int64 Unbanked = BoundGameState.IsValid() ? BoundGameState->GetUnbankedPool() : 0;
			if (CurrentAnteProgress + Unbanked < CurrentAnteConfig.TargetWinningsQuota)
			{
				TransitionToState(ERunProgressionState::RunDefeat);
			}
		}
	}
}

bool URunProgressionSubsystem::AdvanceToNextAnte()
{
	if (bIsAdvancing)
	{
		return false;
	}

	if (CurrentState != ERunProgressionState::AnteCleared && CurrentState != ERunProgressionState::BetweenAntes)
	{
		return false;
	}

	bIsAdvancing = true;

	// 1. Refund legitimately refundable active wagers (Place, Field, Hardway, or ComeOut Pass/DontPass)
	if (BoundGameState.IsValid())
	{
		BoundGameState->RefundAllEligibleWagers();
		BoundGameState->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);
	}

	// 2. Check for Run Victory (completed all configured antes)
	const int32 TotalAntes = GetTotalAntesCount();
	if (CurrentAnteIndex >= TotalAntes)
	{
		TransitionToState(ERunProgressionState::RunVictory);
		bIsAdvancing = false;
		return true;
	}

	// 3. Advance to next Ante
	CurrentAnteIndex++;
	CurrentAnteConfig = ActiveConfig->GetAnteConfig(CurrentAnteIndex);
	CurrentAnteProgress = 0;
	RollsRemaining = CurrentAnteConfig.RollAllowance;

	TransitionToState(ERunProgressionState::ActiveAnte);

	OnAnteAdvanced.Broadcast(CurrentAnteIndex, CurrentAnteConfig);
	OnAnteProgressChanged.Broadcast(CurrentAnteProgress, CurrentAnteConfig.TargetWinningsQuota);
	OnRollsRemainingChanged.Broadcast(RollsRemaining, CurrentAnteConfig.RollAllowance);

	bIsAdvancing = false;
	return true;
}

void URunProgressionSubsystem::TransitionToState(ERunProgressionState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	const ERunProgressionState OldState = CurrentState;
	CurrentState = NewState;

	OnRunStateChanged.Broadcast(NewState, OldState);
}

void URunProgressionSubsystem::HandleWinningsSecured(int64 SecuredAmount)
{
	AwardQualifyingWinnings(SecuredAmount);
}

void URunProgressionSubsystem::HandleRollSettledAuthoritative(int32 Die1Val, int32 Die2Val, int64 Payout)
{
	ConsumeRoll();
}

int32 URunProgressionSubsystem::GetTotalAntesCount() const
{
	return ActiveConfig ? ActiveConfig->GetTotalAntes() : 8;
}
