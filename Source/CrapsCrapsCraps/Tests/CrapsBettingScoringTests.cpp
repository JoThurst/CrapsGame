// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Core/CrapsTypes.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

// 1. Test Wager Debit and Bankroll Limit
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsWagerDebitAndLimitTest, "Craps.Betting.WagerDebitAndLimit", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsWagerDebitAndLimitTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	ABetZone* Zone = NewObject<ABetZone>();
	Zone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(Zone);

	GS->SetPlayerBankroll(100);
	TestEqual(TEXT("Initial bankroll is 100"), GS->GetPlayerBankroll(), 100LL);
	TestEqual(TEXT("Initial zone wager is 0"), Zone->GetPlacedWager(), 0LL);

	// Place 60 chips (should succeed)
	bool bPlaced = GS->PlaceWager(Zone, 60);
	TestTrue(TEXT("Placing 60 chips should succeed"), bPlaced);
	TestEqual(TEXT("Bankroll should be debited to 40"), GS->GetPlayerBankroll(), 40LL);
	TestEqual(TEXT("Zone placed wager should be 60"), Zone->GetPlacedWager(), 60LL);

	// Place 50 chips (should fail: bankroll is only 40)
	bool bFailedPlacement = GS->PlaceWager(Zone, 50);
	TestFalse(TEXT("Placing 50 chips with 40 bankroll should fail"), bFailedPlacement);
	TestEqual(TEXT("Bankroll should remain 40"), GS->GetPlayerBankroll(), 40LL);
	TestEqual(TEXT("Zone placed wager should remain 60"), Zone->GetPlacedWager(), 60LL);

	// Place remaining 40 chips (should succeed)
	bool bPlaceRemaining = GS->PlaceWager(Zone, 40);
	TestTrue(TEXT("Placing remaining 40 chips should succeed"), bPlaceRemaining);
	TestEqual(TEXT("Bankroll should be 0"), GS->GetPlayerBankroll(), 0LL);
	TestEqual(TEXT("Zone placed wager should be 100"), Zone->GetPlacedWager(), 100LL);

	return true;
}

// 2. Test Wager Removal and Contract Lock
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsWagerRemovalAndContractLockTest, "Craps.Betting.WagerRemovalAndContractLock", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsWagerRemovalAndContractLockTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(PassZone);

	ABetZone* FieldZone = NewObject<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);
	GS->RegisterBetZone(FieldZone);

	GS->SetPlayerBankroll(200);

	// Place 50 on Field
	TestTrue(TEXT("Place 50 on Field"), GS->PlaceWager(FieldZone, 50));
	TestEqual(TEXT("Bankroll is 150"), GS->GetPlayerBankroll(), 150LL);

	// Remove 30 from Field (uncommitted eligible bet)
	int64 Refund = 0;
	TestTrue(TEXT("Remove 30 from Field should succeed"), GS->RemoveWager(FieldZone, 30, Refund));
	TestEqual(TEXT("Refunded amount is 30"), Refund, 30LL);
	TestEqual(TEXT("Bankroll is refunded to 180"), GS->GetPlayerBankroll(), 180LL);
	TestEqual(TEXT("Field wager is 20"), FieldZone->GetPlacedWager(), 20LL);

	// Place 50 on Pass Line during ComeOutRoll
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);
	TestTrue(TEXT("Place 50 on Pass Line during ComeOut"), GS->PlaceWager(PassZone, 50));
	TestEqual(TEXT("Bankroll is 130"), GS->GetPlayerBankroll(), 130LL);

	// Advance phase to PointEstablished (Point = 6)
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 6);

	// Attempt to remove Pass Line during Point phase (Contract Bet!)
	Refund = 0;
	bool bRemovedContract = GS->RemoveWager(PassZone, 50, Refund);
	TestFalse(TEXT("Pass Line removal during PointEstablished MUST fail"), bRemovedContract);
	TestEqual(TEXT("Refund should be 0"), Refund, 0LL);
	TestEqual(TEXT("Bankroll should stay 130"), GS->GetPlayerBankroll(), 130LL);
	TestEqual(TEXT("Pass Line wager should remain 50"), PassZone->GetPlacedWager(), 50LL);

	// Field bet can still be removed during Point phase
	Refund = 0;
	TestTrue(TEXT("Field removal during PointEstablished succeeds"), GS->RemoveWager(FieldZone, 20, Refund));
	TestEqual(TEXT("Field fully refunded"), Refund, 20LL);
	TestEqual(TEXT("Bankroll is 150"), GS->GetPlayerBankroll(), 150LL);
	TestEqual(TEXT("Field wager is 0"), FieldZone->GetPlacedWager(), 0LL);

	return true;
}

// 3. Test No Free Money on Default Spread Bets
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsNoFreeMoneyDefaultSpreadTest, "Craps.Betting.NoFreeMoneyDefaultSpread", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsNoFreeMoneyDefaultSpreadTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	ABetZone* Zone1 = NewObject<ABetZone>();
	Zone1->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(Zone1);

	ABetZone* Zone2 = NewObject<ABetZone>();
	Zone2->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);
	GS->RegisterBetZone(Zone2);

	ABetZone* Zone3 = NewObject<ABetZone>();
	Zone3->InitializeZone(ECrapsBetType::Place6, TEXT("PLACE 6"), 1.4f, 50);
	GS->RegisterBetZone(Zone3);

	// Case A: 0 Bankroll -> No default bets placed at all
	GS->SetPlayerBankroll(0);
	GS->PlaceDefaultSpreadBets();
	TestEqual(TEXT("Zone1 has 0 wager when bankroll is 0"), Zone1->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Zone2 has 0 wager when bankroll is 0"), Zone2->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Zone3 has 0 wager when bankroll is 0"), Zone3->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll remains 0"), GS->GetPlayerBankroll(), 0LL);

	// Case B: Partial Bankroll of 75 -> Places 50 on Zone1, 25 on Zone2, 0 on Zone3
	GS->SetPlayerBankroll(75);
	GS->PlaceDefaultSpreadBets();
	TestEqual(TEXT("Zone1 gets 50 default wager"), Zone1->GetPlacedWager(), 50LL);
	TestEqual(TEXT("Zone2 gets remaining 25 wager"), Zone2->GetPlacedWager(), 25LL);
	TestEqual(TEXT("Zone3 gets 0 wager"), Zone3->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll is fully exhausted to 0"), GS->GetPlayerBankroll(), 0LL);

	return true;
}

// 4. Test Winning Bet Odds Do Not Compound Across Bets
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsWinningOddsDoNotCompoundTest, "Craps.Scoring.WinningOddsDoNotMultiplyAcrossBets", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsWinningOddsDoNotCompoundTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);

	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(PassZone);

	ABetZone* FieldZone = NewObject<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);
	GS->RegisterBetZone(FieldZone);

	GS->SetPlayerBankroll(200);
	GS->PlaceWager(PassZone, 50);  // 50 @ 2.0x = 100 payout
	GS->PlaceWager(FieldZone, 50); // 50 @ 1.5x = 75 payout

	// Roll 11 on ComeOut (5 + 6 = 11) -> Pass Line wins (100 chips) and Field wins (75 chips)
	// Base dice yield is 0, no back wall, not hardway
	GS->ResolveSettledRoll(5, 0, FVector::ZeroVector, 6, 0, FVector::ZeroVector, false, 0);

	// Total payout MUST be 100 + 75 = 175 chips (NOT 50*2.0*1.5 + 50*2.0*1.5 = 300)
	TestEqual(TEXT("Authoritative unbanked pool is 175 chips (100 Pass + 75 Field)"), GS->GetUnbankedPool(), 175LL);

	return true;
}

// 5. Test Seven-Out Handling Preserves Don't Pass Winnings While Wiping Exposed Pool
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsSevenOutPreservesDontPassTest, "Craps.Rules.SevenOutPreservesDontPass", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsSevenOutPreservesDontPassTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 8);
	GS->SetUnbankedPool(1500); // Exposed pool accumulated from previous rolls

	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(PassZone);

	ABetZone* DontPassZone = NewObject<ABetZone>();
	DontPassZone->InitializeZone(ECrapsBetType::DontPass, TEXT("DON'T PASS"), 2.0f, 50);
	GS->RegisterBetZone(DontPassZone);

	ABetZone* Place6Zone = NewObject<ABetZone>();
	Place6Zone->InitializeZone(ECrapsBetType::Place6, TEXT("PLACE 6"), 1.4f, 50);
	GS->RegisterBetZone(Place6Zone);

	GS->SetPlayerBankroll(500);
	// Place wagers manually on zones for testing
	PassZone->SetPlacedWager(50);
	DontPassZone->SetPlacedWager(50); // 50 @ 2.0x = 100 payout on Seven-Out
	Place6Zone->SetPlacedWager(50);

	// Roll 7 during PointEstablished (4 + 3 = 7 -> Seven-Out!)
	GS->ResolveSettledRoll(4, 0, FVector::ZeroVector, 3, 0, FVector::ZeroVector, false, 0);

	// 1. Prior 1500 unbanked pool must be wiped
	// 2. Pass Line and Place 6 lose and clear
	TestEqual(TEXT("Pass Line wager cleared on Seven-Out"), PassZone->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Place 6 wager cleared on Seven-Out"), Place6Zone->GetPlacedWager(), 0LL);

	// 3. Don't Pass wager clears upon win
	TestEqual(TEXT("Don't Pass wager cleared upon win"), DontPassZone->GetPlacedWager(), 0LL);

	// 4. Valid Don't Pass winnings (100 chips) must be preserved in UnbankedComboPool!
	TestEqual(TEXT("Unbanked pool preserved Don't Pass winnings (100 chips)"), GS->GetUnbankedPool(), 100LL);

	// 5. Table phase returned to ComeOutRoll and point is 0
	TestEqual(TEXT("Phase is ComeOutRoll"), (uint8)GS->GetTablePhase(), (uint8)ECrapsTablePhase::ComeOutRoll);
	TestEqual(TEXT("EstablishedPoint is 0"), GS->GetEstablishedPoint(), 0);

	return true;
}

// 6. Test Single-Roll Wagers Clear on Both Loss and Win (Field and Horn)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsSingleRollClearingTest, "Craps.Rules.SingleRollClearingFieldAndHorn", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsSingleRollClearingTest::RunTest(const FString& Parameters)
{
	ABetZone* FieldZone = NewObject<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);

	ABetZone* HornZone = NewObject<ABetZone>();
	HornZone->InitializeZone(ECrapsBetType::Horn, TEXT("HORN"), 3.5f, 25);

	FScoringStep Step;
	bool bShouldClear = false;

	// Field on roll 5 (loses)
	FieldZone->SetPlacedWager(50);
	bool bWon = FieldZone->EvaluateRoll(2, 3, ECrapsTablePhase::ComeOutRoll, 0, Step, bShouldClear);
	TestFalse(TEXT("Field loses on 5"), bWon);
	TestTrue(TEXT("Field bShouldClear is true on loss"), bShouldClear);

	// Field on roll 4 (wins)
	bShouldClear = false;
	FieldZone->SetPlacedWager(50);
	bWon = FieldZone->EvaluateRoll(1, 3, ECrapsTablePhase::ComeOutRoll, 0, Step, bShouldClear);
	TestTrue(TEXT("Field wins on 4"), bWon);
	TestTrue(TEXT("Field bShouldClear is true on win (single-roll bet)"), bShouldClear);
	TestEqual(TEXT("Field payout is 75 chips (50 * 1.5)"), Step.BaseChipDelta, 75);

	// Horn on roll 6 (loses)
	bShouldClear = false;
	HornZone->SetPlacedWager(25);
	bWon = HornZone->EvaluateRoll(2, 4, ECrapsTablePhase::ComeOutRoll, 0, Step, bShouldClear);
	TestFalse(TEXT("Horn loses on 6"), bWon);
	TestTrue(TEXT("Horn bShouldClear is true on loss"), bShouldClear);

	// Horn on roll 11 (wins)
	bShouldClear = false;
	HornZone->SetPlacedWager(25);
	bWon = HornZone->EvaluateRoll(5, 6, ECrapsTablePhase::ComeOutRoll, 0, Step, bShouldClear);
	TestTrue(TEXT("Horn wins on 11"), bWon);
	TestTrue(TEXT("Horn bShouldClear is true on win (single-roll bet)"), bShouldClear);

	return true;
}

// 7. Test Hardway Losing Conditions (Easy Way and 7-Out)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsHardwayLosingConditionsTest, "Craps.Rules.HardwayEasyWayAndSevenLosing", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsHardwayLosingConditionsTest::RunTest(const FString& Parameters)
{
	ABetZone* Hard4 = NewObject<ABetZone>();
	Hard4->InitializeZone(ECrapsBetType::Hard4, TEXT("HARD 4"), 3.0f, 25);

	FScoringStep Step;
	bool bShouldClear = false;

	// Easy 4 (3 + 1 = 4): MUST lose and clear!
	Hard4->SetPlacedWager(25);
	bool bWon = Hard4->EvaluateRoll(3, 1, ECrapsTablePhase::PointEstablished, 6, Step, bShouldClear);
	TestFalse(TEXT("Hard 4 does not win on easy 4 (3+1)"), bWon);
	TestTrue(TEXT("Hard 4 MUST clear on easy 4"), bShouldClear);

	// Hard 4 (2 + 2 = 4): Wins!
	bShouldClear = false;
	Hard4->SetPlacedWager(25);
	bWon = Hard4->EvaluateRoll(2, 2, ECrapsTablePhase::PointEstablished, 6, Step, bShouldClear);
	TestTrue(TEXT("Hard 4 wins on 2-2"), bWon);

	// Seven on PointEstablished: Hard 4 MUST clear
	bShouldClear = false;
	Hard4->SetPlacedWager(25);
	bWon = Hard4->EvaluateRoll(4, 3, ECrapsTablePhase::PointEstablished, 6, Step, bShouldClear);
	TestFalse(TEXT("Hard 4 does not win on 7"), bWon);
	TestTrue(TEXT("Hard 4 clears on 7 during PointEstablished"), bShouldClear);

	return true;
}

// 8. Test Authoritative Scoring Independent of Animation Timing
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsAuthoritativeScoringTest, "Craps.Scoring.AuthoritativeIndependentOfAnimation", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsAuthoritativeScoringTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);

	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(PassZone);

	GS->SetPlayerBankroll(500);
	GS->PlaceWager(PassZone, 50);
	TestEqual(TEXT("Bankroll is 450 after wager"), GS->GetPlayerBankroll(), 450LL);

	// Roll natural 7 on Come-Out: Die1 yields 25 chips, Die2 yields 25 chips, Pass Line pays 100 chips
	// Authoritative score: (25 + 25 + 100) * 1.0 * 1.0 = 150 chips
	GS->ResolveSettledRoll(3, 25, FVector::ZeroVector, 4, 25, FVector::ZeroVector, false, 0);

	// Check that UnbankedComboPool was updated immediately and authoritatively
	TestEqual(TEXT("Authoritative pool updated immediately to 150"), GS->GetUnbankedPool(), 150LL);

	// Bank the pool immediately without waiting for any animation
	int64 Banked = GS->BankUnbankedPool();
	TestEqual(TEXT("Banked amount is 150"), Banked, 150LL);
	TestEqual(TEXT("Bankroll is 450 + 150 = 600"), GS->GetPlayerBankroll(), 600LL);
	TestEqual(TEXT("Unbanked pool is now 0"), GS->GetUnbankedPool(), 0LL);

	return true;
}

// 9. Test Dealer Puck Movement and State Tracking
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsDealerPuckMovementTest, "Craps.Table.DealerPuckMovement", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsDealerPuckMovementTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	FURL URL;
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();

	ACrapsTableGrayboxActor* TableActor = World->SpawnActor<ACrapsTableGrayboxActor>();
	TestNotNull(TEXT("TableActor spawned in test world"), TableActor);
	TableActor->EnsureLayoutResolved();

	// Initially Come-Out / OFF position: (65, 180, 3)
	TableActor->SetDealerPuckState(false, 0);
	UStaticMeshComponent* PuckMesh = nullptr;
	for (UActorComponent* Comp : TableActor->GetComponents())
	{
		if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
		{
			if (SMC->ComponentHasTag(TEXT("DealerPuckMesh")))
			{
				PuckMesh = SMC;
				break;
			}
		}
	}
	TestNotNull(TEXT("DealerPuckMesh exists on table"), PuckMesh);
	if (PuckMesh)
	{
		TestEqual(TEXT("Puck OFF X position is 65"), PuckMesh->GetRelativeLocation().X, 65.0);
		TestEqual(TEXT("Puck OFF Y position is 180"), PuckMesh->GetRelativeLocation().Y, 180.0);

		// Test Point 4: X = 125, Y = 180
		TableActor->SetDealerPuckState(true, 4);
		TestEqual(TEXT("Point 4 Puck X position is 125"), PuckMesh->GetRelativeLocation().X, 125.0);
		TestEqual(TEXT("Point 4 Puck Y position is 180"), PuckMesh->GetRelativeLocation().Y, 180.0);

		// Test Point 5: X = 185
		TableActor->SetDealerPuckState(true, 5);
		TestEqual(TEXT("Point 5 Puck X position is 185"), PuckMesh->GetRelativeLocation().X, 185.0);

		// Test Point 6: X = 245
		TableActor->SetDealerPuckState(true, 6);
		TestEqual(TEXT("Point 6 Puck X position is 245"), PuckMesh->GetRelativeLocation().X, 245.0);

		// Test Point 8: X = 305
		TableActor->SetDealerPuckState(true, 8);
		TestEqual(TEXT("Point 8 Puck X position is 305"), PuckMesh->GetRelativeLocation().X, 305.0);

		// Test Point 9: X = 365
		TableActor->SetDealerPuckState(true, 9);
		TestEqual(TEXT("Point 9 Puck X position is 365"), PuckMesh->GetRelativeLocation().X, 365.0);

		// Test Point 10: X = 425
		TableActor->SetDealerPuckState(true, 10);
		TestEqual(TEXT("Point 10 Puck X position is 425"), PuckMesh->GetRelativeLocation().X, 425.0);

		// Test Seven-Out / Return to Come-Out: returns to OFF (65, 180)
		TableActor->SetDealerPuckState(false, 0);
		TestEqual(TEXT("Puck returned to OFF X position 65"), PuckMesh->GetRelativeLocation().X, 65.0);
	}

	// Verify UFunction reflection
	UFunction* PhaseFunc = TableActor->FindFunction(TEXT("HandleTablePhaseChanged"));
	TestNotNull(TEXT("TableActor has HandleTablePhaseChanged UFunction"), PhaseFunc);

	// Test GameState delegate synchronization
	ACrapsGameState* GS = World->SpawnActor<ACrapsGameState>();
	TestNotNull(TEXT("GS spawned in test world"), GS);
	TableActor->BindToGameState(GS);
	TestTrue(TEXT("OnTablePhaseChanged is bound"), GS->OnTablePhaseChanged.IsBound());

	TableActor->HandleTablePhaseChanged(ECrapsTablePhase::PointEstablished, 6);
	if (PuckMesh)
	{
		TestEqual(TEXT("Direct HandleTablePhaseChanged moves to Point 6 (245)"), PuckMesh->GetRelativeLocation().X, 245.0);
	}

	// Establish point 8 via GameState
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 8);
	if (PuckMesh)
	{
		TestEqual(TEXT("GameState delegate moves puck to Point 8 (305)"), PuckMesh->GetRelativeLocation().X, 305.0);
	}

	// Return to ComeOutRoll
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);
	if (PuckMesh)
	{
		TestEqual(TEXT("GameState delegate moves puck back to OFF (65)"), PuckMesh->GetRelativeLocation().X, 65.0);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
