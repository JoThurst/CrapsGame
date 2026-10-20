// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Table/CrapsGameState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Table/BetZone.h"
#include "Items/TrinketInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

ACrapsGameState::ACrapsGameState()
{
}

void ACrapsGameState::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UScoringQueueSubsystem* ScoringQueue = World->GetSubsystem<UScoringQueueSubsystem>())
		{
			ScoringQueue->OnScoringSequenceFinished.AddDynamic(this, &ACrapsGameState::HandleScoringQueueFinished);
		}

		// Discover any pre-existing ABetZone actors in the level
		for (TActorIterator<ABetZone> It(World); It; ++It)
		{
			RegisterBetZone(*It);
		}
	}
}

void ACrapsGameState::RegisterBetZone(ABetZone* BetZone)
{
	if (BetZone && !ActiveBetZones.Contains(BetZone))
	{
		ActiveBetZones.Add(BetZone);
	}
}

void ACrapsGameState::BindTrinketInventory(UTrinketInventoryComponent* InTrinketInventory)
{
	BoundTrinketInventory = InTrinketInventory;
}

UTrinketInventoryComponent* ACrapsGameState::GetActiveTrinketInventory() const
{
	if (BoundTrinketInventory.IsValid())
	{
		return BoundTrinketInventory.Get();
	}
	if (UWorld* World = GetWorld())
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			return PlayerPawn->FindComponentByClass<UTrinketInventoryComponent>();
		}
	}
	return nullptr;
}

void ACrapsGameState::ResolveSettledRoll(
	int32 Die1FaceValue,
	int32 Die1ChipYield,
	const FVector& Die1Location,
	int32 Die2FaceValue,
	int32 Die2ChipYield,
	const FVector& Die2Location,
	bool bHitBackWall,
	int32 TotalWallBounces)
{
	bIsResolvingRoll = true;

	const int32 RollSum = Die1FaceValue + Die2FaceValue;
	const bool bIsHardway = (Die1FaceValue == Die2FaceValue);
	const ECrapsTablePhase PreRollPhase = CurrentPhase;
	const int32 PreRollPoint = EstablishedPoint;

	UWorld* World = GetWorld();
	UScoringQueueSubsystem* ScoringQueue = World ? World->GetSubsystem<UScoringQueueSubsystem>() : nullptr;

	// Build contextual roll state for Trinket and Synergy evaluation
	FRollEvaluationContext Context;
	Context.Die1Value = Die1FaceValue;
	Context.Die2Value = Die2FaceValue;
	Context.RollSum = RollSum;
	Context.bIsHardway = bIsHardway;
	Context.bHitBackWall = bHitBackWall;
	Context.TotalBounces = TotalWallBounces;
	Context.TablePhase = PreRollPhase;
	Context.EstablishedPoint = PreRollPoint;
	Context.bSevenOut = (PreRollPhase == ECrapsTablePhase::PointEstablished && RollSum == 7);
	Context.bPointHit = (PreRollPhase == ECrapsTablePhase::PointEstablished && RollSum == PreRollPoint);
	Context.bNatural = (PreRollPhase == ECrapsTablePhase::ComeOutRoll && (RollSum == 7 || RollSum == 11));
	Context.bCraps = (PreRollPhase == ECrapsTablePhase::ComeOutRoll && (RollSum == 2 || RollSum == 3 || RollSum == 12));
	Context.MidRollHazardsHit = ScoringQueue ? ((ScoringQueue->RunningBaseChips > 0) ? FMath::Max(1, ScoringQueue->RunningBaseChips / 30) : 0) : 0;

	if (RollSum % 2 != 0)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnOddSum"), false));
	}
	else
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnEvenSum"), false));
	}
	if (bIsHardway)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnHardway"), false));
	}
	if (Context.bNatural)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnNatural"), false));
	}
	if (Context.bCraps)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnCraps"), false));
	}
	if (Context.bPointHit)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnPointHit"), false));
	}
	if (RollSum == 7)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnSeven"), false));
	}
	if (bHitBackWall)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnWallBounce"), false));
	}
	if (TotalWallBounces > 0)
	{
		Context.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnAnyBounce"), false));
	}

	UTrinketInventoryComponent* TrinketComp = GetActiveTrinketInventory();

	// Check for Seven-Out during PointEstablished
	if (PreRollPhase == ECrapsTablePhase::PointEstablished && RollSum == 7)
	{
		// Check for Miracle Seven aversion via equipped Trinket (e.g. Onyx Claw)
		FScoringStep MiracleStep;
		int32 MiracleChips = 0;
		const bool bMiracleAverted = TrinketComp && TrinketComp->CheckMiracleSevenAverted(Context, MiracleStep, MiracleChips);

		if (!bMiracleAverted)
		{
			// 1. Wipe exposed unbanked combo pool from previous rolls
			UnbankedComboPool = 0;
			ConsecutivePointsHit = 0;
			CurrentPhase = ECrapsTablePhase::ComeOutRoll;
			EstablishedPoint = 0;

			int64 DontPassWinnings = 0;
			TArray<FScoringStep> DontPassSteps;

			// 2. Evaluate all active bets; losing bets clear, winning Don't Pass pays and clears
			for (ABetZone* Zone : ActiveBetZones)
			{
				if (!Zone)
				{
					continue;
				}

				FScoringStep ZoneStep;
				bool bShouldClear = false;
				const bool bWon = Zone->EvaluateRoll(Die1FaceValue, Die2FaceValue, PreRollPhase, PreRollPoint, ZoneStep, bShouldClear);
				if (bWon && Zone->GetBetType() == ECrapsBetType::DontPass)
				{
					DontPassWinnings += ZoneStep.BaseChipDelta;
					DontPassSteps.Add(ZoneStep);
				}

				if (bShouldClear)
				{
					Zone->ClearWager();
				}
			}

			// 3. Authoritatively preserve valid Don't Pass winnings in the pool
			UnbankedComboPool = DontPassWinnings;

			LastRollOutcomeBanner = FString::Printf(
				TEXT("SEVEN-OUT! (%d+%d=7) — Exposed Pool Wiped! Don't Pass Won: +%lld Chips. Puck -> OFF"),
				Die1FaceValue,
				Die2FaceValue,
				DontPassWinnings);

			// 4. Enqueue presentation steps for visual feedback
			if (ScoringQueue)
			{
				FScoringStep SevenOutStep;
				SevenOutStep.StepLabel = TEXT("SEVEN-OUT! EXPOSED POOL WIPED");
				SevenOutStep.Layer = EScoringLayer::SevenOut;
				SevenOutStep.PopupWorldLocation = (Die1Location + Die2Location) * 0.5f + FVector(0.0f, 0.0f, 40.0f);
				ScoringQueue->EnqueueScoringStep(SevenOutStep);

				for (const FScoringStep& Step : DontPassSteps)
				{
					ScoringQueue->EnqueueScoringStep(Step);
				}

				ScoringQueue->StartEndOfRollResolution(true);
			}

			bIsResolvingRoll = false;
			OnTablePhaseChanged.Broadcast(CurrentPhase, EstablishedPoint);
			OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
			OnRollSettledAuthoritative.Broadcast(Die1FaceValue, Die2FaceValue, DontPassWinnings);
			return;
		}
		else
		{
			// Miracle Seven! Puck stays ON, Point remains established, wagers not wiped
			LastRollOutcomeBanner = FString::Printf(
				TEXT("MIRACLE SEVEN! (+%d Chips) — Seven-Out Averted! Chasing Point [%d]"),
				MiracleChips, PreRollPoint);

			if (ScoringQueue)
			{
				MiracleStep.PopupWorldLocation = (Die1Location + Die2Location) * 0.5f + FVector(0.0f, 0.0f, 40.0f);
				ScoringQueue->EnqueueScoringStep(MiracleStep);
			}

			Die1ChipYield += (MiracleChips / 2);
			Die2ChipYield += (MiracleChips - (MiracleChips / 2));
		}
	}

	// --- 1. Compute Authoritative Base Chips and Throw Multipliers ---
	int32 AuthBaseChips = Die1ChipYield + Die2ChipYield;
	float AuthThrowMult = 1.0f;
	if (bHitBackWall)
	{
		AuthThrowMult += 0.5f;
	}
	if (bIsHardway)
	{
		AuthThrowMult += 0.5f;
	}

	// Integrate mid-roll hazard chips & throw multipliers earned during flight
	if (ScoringQueue)
	{
		AuthBaseChips += ScoringQueue->RunningBaseChips;
		if (ScoringQueue->RunningThrowMult > 1.0f)
		{
			AuthThrowMult += (ScoringQueue->RunningThrowMult - 1.0f);
		}
	}
	float AuthBetMult = 1.0f;

	// Integrate Left-Hand Trinket synergies (Milestone 3.1 & 3.2)
	int32 TrinketBonusChips = 0;
	float TrinketBonusThrowMult = 0.0f;
	float TrinketBetMultFactor = 1.0f;
	if (TrinketComp)
	{
		TrinketComp->EvaluateTrinketsForRoll(
			Context,
			ScoringQueue,
			TrinketBonusChips,
			TrinketBonusThrowMult,
			TrinketBetMultFactor);
	}

	AuthBaseChips += TrinketBonusChips;
	AuthThrowMult += TrinketBonusThrowMult;
	AuthBetMult *= TrinketBetMultFactor;

	// Enqueue Presentation Layer 1 & Layer 2 steps
	if (ScoringQueue)
	{
		FScoringStep Die1Step;
		Die1Step.StepLabel = FString::Printf(TEXT("DIE 1 FACE [%d] (+%d CHIPS)"), Die1FaceValue, Die1ChipYield);
		Die1Step.Layer = EScoringLayer::BaseChips;
		Die1Step.BaseChipDelta = Die1ChipYield;
		Die1Step.PopupWorldLocation = Die1Location + FVector(0.0f, 0.0f, 30.0f);
		ScoringQueue->EnqueueScoringStep(Die1Step);

		FScoringStep Die2Step;
		Die2Step.StepLabel = FString::Printf(TEXT("DIE 2 FACE [%d] (+%d CHIPS)"), Die2FaceValue, Die2ChipYield);
		Die2Step.Layer = EScoringLayer::BaseChips;
		Die2Step.BaseChipDelta = Die2ChipYield;
		Die2Step.PopupWorldLocation = Die2Location + FVector(0.0f, 0.0f, 30.0f);
		ScoringQueue->EnqueueScoringStep(Die2Step);

		if (bHitBackWall)
		{
			FScoringStep WallStep;
			WallStep.StepLabel = TEXT("ALLIGATOR WALL BONUS (+0.5x THROW MULT)");
			WallStep.Layer = EScoringLayer::ThrowMult;
			WallStep.ThrowMultDelta = 0.5f;
			WallStep.PopupWorldLocation = (Die1Location + Die2Location) * 0.5f + FVector(0.0f, 0.0f, 45.0f);
			ScoringQueue->EnqueueScoringStep(WallStep);
		}

		if (bIsHardway)
		{
			FScoringStep HardPairStep;
			HardPairStep.StepLabel = FString::Printf(TEXT("SYMMETRIC PAIR %d-%d (+0.5x THROW MULT)"), Die1FaceValue, Die2FaceValue);
			HardPairStep.Layer = EScoringLayer::ThrowMult;
			HardPairStep.ThrowMultDelta = 0.5f;
			HardPairStep.PopupWorldLocation = (Die1Location + Die2Location) * 0.5f + FVector(0.0f, 0.0f, 60.0f);
			ScoringQueue->EnqueueScoringStep(HardPairStep);
		}
	}

	// --- 2. Evaluate Active ABetZone Actors ---
	int32 WinningZonesCount = 0;
	int64 AuthBetChipsTotal = 0;

	for (ABetZone* Zone : ActiveBetZones)
	{
		if (!Zone)
		{
			continue;
		}

		FScoringStep ZoneStep;
		bool bShouldClear = false;
		if (Zone->EvaluateRoll(Die1FaceValue, Die2FaceValue, PreRollPhase, PreRollPoint, ZoneStep, bShouldClear))
		{
			++WinningZonesCount;
			AuthBetChipsTotal += ZoneStep.BaseChipDelta;
			if (ScoringQueue)
			{
				ScoringQueue->EnqueueScoringStep(ZoneStep);
			}
		}

		// Fix: If a bet resolves (lost or won single-roll), clear the wager
		if (bShouldClear)
		{
			Zone->ClearWager();
		}
	}

	// --- 3. Advance Craps Table Phase State Machine & Streak Bonuses ---
	if (PreRollPhase == ECrapsTablePhase::ComeOutRoll)
	{
		if (RollSum == 7 || RollSum == 11)
		{
			LastRollOutcomeBanner = FString::Printf(TEXT("COME-OUT NATURAL %d! WINNER! (Puck stays OFF)"), RollSum);
		}
		else if (RollSum == 2 || RollSum == 3 || RollSum == 12)
		{
			LastRollOutcomeBanner = FString::Printf(TEXT("CRAPS %d ON COME-OUT! (Field/Horn Pay, Pass Resets)"), RollSum);
		}
		else
		{
			// 4, 5, 6, 8, 9, 10 establishes the Point
			CurrentPhase = ECrapsTablePhase::PointEstablished;
			EstablishedPoint = RollSum;
			LastRollOutcomeBanner = FString::Printf(TEXT("POINT ESTABLISHED: [%d] — Puck ON!"), EstablishedPoint);
			OnTablePhaseChanged.Broadcast(CurrentPhase, EstablishedPoint);
		}
	}
	else if (PreRollPhase == ECrapsTablePhase::PointEstablished)
	{
		if (RollSum == PreRollPoint)
		{
			++ConsecutivePointsHit;
			LastRollOutcomeBanner = FString::Printf(
				TEXT("POINT [%d] HIT! (Hot Streak x%d) — Puck returns OFF!"),
				PreRollPoint,
				ConsecutivePointsHit);

			const int32 StreakChips = 100 * ConsecutivePointsHit;
			const float StreakMultFactor = 1.5f + (0.25f * ConsecutivePointsHit);
			AuthBaseChips += StreakChips;
			AuthBetMult *= StreakMultFactor;

			if (ScoringQueue)
			{
				FScoringStep StreakStep;
				StreakStep.StepLabel = FString::Printf(TEXT("HOT SHOOTER STREAK x%d"), ConsecutivePointsHit);
				StreakStep.Layer = EScoringLayer::PointBonus;
				StreakStep.BaseChipDelta = StreakChips;
				StreakStep.MultiplierFactor = StreakMultFactor;
				StreakStep.PopupWorldLocation = (Die1Location + Die2Location) * 0.5f + FVector(0.0f, 0.0f, 75.0f);
				ScoringQueue->EnqueueScoringStep(StreakStep);
			}

			CurrentPhase = ECrapsTablePhase::ComeOutRoll;
			EstablishedPoint = 0;
			OnTablePhaseChanged.Broadcast(CurrentPhase, EstablishedPoint);
		}
		else
		{
			LastRollOutcomeBanner = FString::Printf(
				TEXT("ROLLED %d (%d Winning Bets) — Chasing Point [%d]"),
				RollSum,
				WinningZonesCount,
				EstablishedPoint);
		}
	}

	// --- 4. Authoritatively Commit Roll Payout to Economy ---
	const int64 AuthTotalChips = static_cast<int64>(AuthBaseChips) + AuthBetChipsTotal;
	const double RawTotal = static_cast<double>(AuthTotalChips) * static_cast<double>(AuthThrowMult) * static_cast<double>(AuthBetMult);
	const int64 AuthoritativePayout = FMath::Max<int64>(0, FMath::RoundToInt64(RawTotal));

	UnbankedComboPool += AuthoritativePayout;
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);

	// Start presentation animation in the scoring queue
	if (ScoringQueue)
	{
		ScoringQueue->StartEndOfRollResolution(false);
	}

	bIsResolvingRoll = false;
	OnRollSettledAuthoritative.Broadcast(Die1FaceValue, Die2FaceValue, AuthoritativePayout);
}

void ACrapsGameState::HandleNoRoll(const FString& Reason)
{
	bIsResolvingRoll = false;

	LastRollOutcomeBanner = FString::Printf(TEXT("NO ROLL! [%s] — Wagers & Rolls Preserved!"), *Reason);

	if (UWorld* World = GetWorld())
	{
		if (UScoringQueueSubsystem* ScoringQueue = World->GetSubsystem<UScoringQueueSubsystem>())
		{
			ScoringQueue->VoidCurrentThrowScoring();
		}
	}

	// Preserves PlayerBankroll, UnbankedComboPool, ActiveBetZones, and CurrentPhase intact.
	// Does NOT broadcast OnRollSettledAuthoritative, so URunProgressionSubsystem will not consume a roll.
	OnTablePhaseChanged.Broadcast(CurrentPhase, EstablishedPoint);
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
}

void ACrapsGameState::HandleScoringQueueFinished(int64 FinalRollScore, bool bWasSevenOut)
{
	// Authoritative scoring is committed in ResolveSettledRoll.
	// This event handles visual presentation sequence completion.
}

int64 ACrapsGameState::BankUnbankedPool()
{
	if (UnbankedComboPool <= 0)
	{
		return 0;
	}

	const int64 BankedScore = UnbankedComboPool;

	// Option B: Score Chips vs. Spendable Bankroll Separation.
	// 1. Authoritative combo score chips are deposited into Ante Progress
	OnWinningsSecured.Broadcast(BankedScore);

	// 2. Spendable Bankroll calculation:
	int64 CashAwarded = 0;
	if (bEnableScoreCashSeparation)
	{
		// Option B: Controlled cash dividend to prevent 10k wager spam runaway
		CashAwarded = FMath::Max<int64>(MinCashDividendPerBank, FMath::RoundToInt64(BankedScore * ScoreToCashDividendRatio));
		PlayerBankroll += CashAwarded;
		LastRollOutcomeBanner = FString::Printf(TEXT("BANKED +%lld SCORE! (+$%lld Cash) Bankroll: $%lld"),
			BankedScore, CashAwarded, PlayerBankroll);
	}
	else
	{
		// Option A: Unified Bankroll (Context Preserved: Table Wager Caps would clamp wagers)
		CashAwarded = BankedScore;
		PlayerBankroll += BankedScore;
		LastRollOutcomeBanner = FString::Printf(TEXT("BANKED +%lld CHIPS! Bankroll: $%lld / %lld"),
			BankedScore, PlayerBankroll, CurrentAnteQuota);
	}

	UnbankedComboPool = 0;
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
	OnBankedPayoutVisuals.Broadcast(BankedScore, CashAwarded);

	return BankedScore;
}

int64 ACrapsGameState::RefundAllEligibleWagers()
{
	int64 TotalRefunded = 0;
	for (ABetZone* Zone : ActiveBetZones)
	{
		if (Zone && Zone->GetPlacedWager() > 0)
		{
			int64 Refund = 0;
			if (RemoveWager(Zone, Zone->GetPlacedWager(), Refund))
			{
				TotalRefunded += Refund;
			}
		}
	}
	return TotalRefunded;
}

bool ACrapsGameState::CanModifyWager(const ABetZone* Zone, bool bIsRemoval) const
{
	if (!Zone)
	{
		return false;
	}

	if (bIsResolvingRoll)
	{
		return false;
	}

	const ECrapsBetType BType = Zone->GetBetType();

	if (bIsRemoval)
	{
		if (Zone->GetPlacedWager() <= 0)
		{
			return false;
		}

		// Contract bets: Pass Line is locked in once the Point is established
		if (BType == ECrapsBetType::PassLine && CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			return false;
		}

		return true;
	}
	else
	{
		if (PlayerBankroll <= 0)
		{
			return false;
		}

		// Cannot place a new Pass Line bet after Point is established
		if (BType == ECrapsBetType::PassLine && CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			return false;
		}

		return true;
	}
}

bool ACrapsGameState::PlaceWager(ABetZone* Zone, int64 Amount)
{
	if (!Zone || Amount <= 0)
	{
		return false;
	}

	if (!CanModifyWager(Zone, false))
	{
		return false;
	}

	if (PlayerBankroll < Amount)
	{
		return false;
	}

	PlayerBankroll -= Amount;
	Zone->AddWager(Amount);
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
	return true;
}

bool ACrapsGameState::RemoveWager(ABetZone* Zone, int64 Amount, int64& OutRefundedAmount)
{
	OutRefundedAmount = 0;
	if (!Zone || Amount <= 0)
	{
		return false;
	}

	if (!CanModifyWager(Zone, true))
	{
		return false;
	}

	const int64 Available = Zone->GetPlacedWager();
	if (Available <= 0)
	{
		return false;
	}

	const int64 ActualRefund = FMath::Min(Amount, Available);
	Zone->AddWager(-ActualRefund);
	PlayerBankroll += ActualRefund;
	OutRefundedAmount = ActualRefund;
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
	return true;
}

void ACrapsGameState::SetPlayerBankroll(int64 NewBankroll)
{
	PlayerBankroll = FMath::Max<int64>(0, NewBankroll);
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
}

void ACrapsGameState::SetUnbankedPool(int64 NewPool)
{
	UnbankedComboPool = FMath::Max<int64>(0, NewPool);
	OnBankrollChanged.Broadcast(PlayerBankroll, UnbankedComboPool);
}

void ACrapsGameState::SetTablePhase(ECrapsTablePhase NewPhase, int32 NewPoint)
{
	CurrentPhase = NewPhase;
	EstablishedPoint = (NewPhase == ECrapsTablePhase::PointEstablished) ? NewPoint : 0;
	OnTablePhaseChanged.Broadcast(CurrentPhase, EstablishedPoint);
}

void ACrapsGameState::PlaceDefaultSpreadBets()
{
	if (PlayerBankroll <= 0)
	{
		return;
	}

	// 1. Check if any active eligible zone is below its default wager
	bool bAnyBelowDefault = false;
	for (ABetZone* Zone : ActiveBetZones)
	{
		if (Zone && CanModifyWager(Zone, false))
		{
			const int64 TargetWager = Zone->GetDefaultWager() > 0 ? Zone->GetDefaultWager() : 50;
			if (Zone->GetPlacedWager() < TargetWager)
			{
				bAnyBelowDefault = true;
				break;
			}
		}
	}

	if (bAnyBelowDefault)
	{
		// Top up each eligible zone to its default wager
		for (ABetZone* Zone : ActiveBetZones)
		{
			if (!Zone || !CanModifyWager(Zone, false))
			{
				continue;
			}

			const int64 TargetWager = Zone->GetDefaultWager() > 0 ? Zone->GetDefaultWager() : 50;
			const int64 CurrentWager = Zone->GetPlacedWager();
			if (CurrentWager < TargetWager)
			{
				const int64 Needed = TargetWager - CurrentWager;
				const int64 ToPlace = FMath::Min(Needed, PlayerBankroll);
				if (ToPlace > 0)
				{
					PlaceWager(Zone, ToPlace);
				}
			}

			if (PlayerBankroll <= 0)
			{
				break;
			}
		}

		LastRollOutcomeBanner = FString::Printf(TEXT("SEEDED DEFAULT BETS! Bankroll: $%lld"), PlayerBankroll);
	}
	else
	{
		// All zones are already at or above default: PRESS / STACK all eligible bets by +1 default unit!
		int64 TotalPressed = 0;
		for (ABetZone* Zone : ActiveBetZones)
		{
			if (!Zone || !CanModifyWager(Zone, false))
			{
				continue;
			}

			const int64 Unit = Zone->GetDefaultWager() > 0 ? Zone->GetDefaultWager() : 50;
			const int64 ToPlace = FMath::Min(Unit, PlayerBankroll);
			if (ToPlace > 0)
			{
				if (PlaceWager(Zone, ToPlace))
				{
					TotalPressed += ToPlace;
				}
			}

			if (PlayerBankroll <= 0)
			{
				break;
			}
		}

		if (TotalPressed > 0)
		{
			LastRollOutcomeBanner = FString::Printf(TEXT("PRESSED ALL BETS (+%lld CHIPS)! Bankroll: $%lld"), TotalPressed, PlayerBankroll);
		}
	}
}
