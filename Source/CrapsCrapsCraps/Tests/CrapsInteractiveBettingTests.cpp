// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Pawn/CrapsPawn.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "UI/CrapsDebugHUD.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

// 1. Test Targeted Bet Zone Placement & Stacking
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTargetedBetPlacementTest, "Craps.InteractiveBetting.TargetedZonePlacement", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTargetedBetPlacementTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	ABetZone* FieldZone = NewObject<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 2.0f, 50);

	GS->RegisterBetZone(PassZone);
	GS->RegisterBetZone(FieldZone);
	GS->SetPlayerBankroll(500);

	// Targeted stacking on FieldZone alone
	bool bPlaced1 = GS->PlaceWager(FieldZone, 50);
	TestTrue(TEXT("First $50 placement on Field should succeed"), bPlaced1);
	TestEqual(TEXT("Field wager is now $50"), FieldZone->GetPlacedWager(), 50LL);
	TestEqual(TEXT("Pass Line wager remains $0"), PassZone->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll debited to $450"), GS->GetPlayerBankroll(), 450LL);

	// Stack a second chip onto FieldZone
	bool bPlaced2 = GS->PlaceWager(FieldZone, 50);
	TestTrue(TEXT("Second $50 placement on Field should succeed"), bPlaced2);
	TestEqual(TEXT("Field wager is now stacked to $100"), FieldZone->GetPlacedWager(), 100LL);
	TestEqual(TEXT("Pass Line wager is still $0"), PassZone->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll debited to $400"), GS->GetPlayerBankroll(), 400LL);

	return true;
}

// 2. Test Individual Zone Refund & Contract Lock Enforcement
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsIndividualZoneRefundTest, "Craps.InteractiveBetting.IndividualZoneRefundAndContractLock", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsIndividualZoneRefundTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	ABetZone* PassZone = NewObject<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	ABetZone* FieldZone = NewObject<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 2.0f, 50);

	GS->RegisterBetZone(PassZone);
	GS->RegisterBetZone(FieldZone);
	GS->SetPlayerBankroll(500);

	// Place $50 on Pass and $50 on Field during Come-Out
	GS->PlaceWager(PassZone, 50);
	GS->PlaceWager(FieldZone, 50);
	TestEqual(TEXT("Bankroll is $400"), GS->GetPlayerBankroll(), 400LL);

	// Refund $50 from Field (allowed during Come-Out)
	int64 Refunded = 0;
	bool bRefundField = GS->RemoveWager(FieldZone, 50, Refunded);
	TestTrue(TEXT("Field removal should succeed"), bRefundField);
	TestEqual(TEXT("Field refunded $50"), Refunded, 50LL);
	TestEqual(TEXT("Field wager back to $0"), FieldZone->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll refunded to $450"), GS->GetPlayerBankroll(), 450LL);
	TestEqual(TEXT("Pass Line wager remains $50"), PassZone->GetPlacedWager(), 50LL);

	// Establish Point 6
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 6);

	// Attempt to remove Pass Line during Point phase (Contract Lock: MUST FAIL)
	Refunded = 0;
	bool bRefundPassBlocked = GS->RemoveWager(PassZone, 50, Refunded);
	TestFalse(TEXT("Pass Line removal during PointEstablished should be blocked"), bRefundPassBlocked);
	TestEqual(TEXT("No chips refunded from Pass"), Refunded, 0LL);
	TestEqual(TEXT("Pass Line wager remains $50"), PassZone->GetPlacedWager(), 50LL);
	TestEqual(TEXT("Bankroll remains $450"), GS->GetPlayerBankroll(), 450LL);

	// Place and remove on Field during Point phase (Single-roll: Allowed)
	GS->PlaceWager(FieldZone, 50);
	TestEqual(TEXT("Field wager is $50"), FieldZone->GetPlacedWager(), 50LL);
	bool bRefundFieldPoint = GS->RemoveWager(FieldZone, 50, Refunded);
	TestTrue(TEXT("Field removal during Point phase should succeed"), bRefundFieldPoint);
	TestEqual(TEXT("Bankroll restored to $450"), GS->GetPlayerBankroll(), 450LL);

	return true;
}

// 3. Test Chip Denomination Cycling & Management
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsDenominationCyclingTest, "Craps.InteractiveBetting.DenominationCycling", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsDenominationCyclingTest::RunTest(const FString& Parameters)
{
	ACrapsPawn* Pawn = NewObject<ACrapsPawn>();
	TestEqual(TEXT("Default denomination is 25"), Pawn->SelectedChipDenomination, 25LL);

	// Cycle forward (25 -> 100 -> 500 -> 1000 -> 1 -> 5 -> 25)
	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward to 100"), Pawn->SelectedChipDenomination, 100LL);

	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward to 500"), Pawn->SelectedChipDenomination, 500LL);

	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward to 1000"), Pawn->SelectedChipDenomination, 1000LL);

	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward wraps to 1"), Pawn->SelectedChipDenomination, 1LL);

	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward to 5"), Pawn->SelectedChipDenomination, 5LL);

	Pawn->CycleChipDenomination(1);
	TestEqual(TEXT("Cycle forward to 25"), Pawn->SelectedChipDenomination, 25LL);

	// Cycle backward (25 -> 5 -> 1 -> 1000)
	Pawn->CycleChipDenomination(-1);
	TestEqual(TEXT("Cycle backward to 5"), Pawn->SelectedChipDenomination, 5LL);

	Pawn->CycleChipDenomination(-1);
	TestEqual(TEXT("Cycle backward to 1"), Pawn->SelectedChipDenomination, 1LL);

	Pawn->CycleChipDenomination(-1);
	TestEqual(TEXT("Cycle backward wraps to 1000"), Pawn->SelectedChipDenomination, 1000LL);

	// Direct assignment
	Pawn->SetChipDenomination(250);
	TestEqual(TEXT("Direct assignment to 250"), Pawn->SelectedChipDenomination, 250LL);

	return true;
}

// 4. Test Dealer Puck Movable Mobility & Position Setting
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsDealerPuckMobilityTest, "Craps.Table.DealerPuckMovableMobility", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsDealerPuckMobilityTest::RunTest(const FString& Parameters)
{
	ACrapsTableGrayboxActor* Table = NewObject<ACrapsTableGrayboxActor>();
	Table->EnsureLayoutResolved();

	// Discover DealerPuckMesh from components
	TArray<UStaticMeshComponent*> MeshComps;
	Table->GetComponents<UStaticMeshComponent>(MeshComps);
	UStaticMeshComponent* PuckMesh = nullptr;
	for (UStaticMeshComponent* SMC : MeshComps)
	{
		if (SMC && SMC->ComponentHasTag(TEXT("DealerPuckMesh")))
		{
			PuckMesh = SMC;
			break;
		}
	}

	TestNotNull(TEXT("DealerPuckMesh should exist"), PuckMesh);
	if (PuckMesh)
	{
		TestEqual(TEXT("DealerPuckMesh mobility MUST be Movable"), PuckMesh->Mobility.GetValue(), EComponentMobility::Movable);

		// Set point to 8 (ON)
		Table->SetDealerPuckState(true, 8);
		const FVector LocOn8 = PuckMesh->GetRelativeLocation();
		TestEqual(TEXT("Puck X coordinate for Point 8 is 305.0"), LocOn8.X, 305.0);
		TestEqual(TEXT("Puck Y coordinate is 180.0"), LocOn8.Y, 180.0);

		// Set point to OFF (Don't Come Bar parked)
		Table->SetDealerPuckState(false, 0);
		const FVector LocOff = PuckMesh->GetRelativeLocation();
		TestEqual(TEXT("Puck X coordinate parked is 65.0"), LocOff.X, 65.0);
		TestEqual(TEXT("Puck Y coordinate parked is 180.0"), LocOff.Y, 180.0);
	}

	return true;
}

// 5. Test Option B Economy Separation (Score vs. Cash Bankroll)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsOptionBEconomySeparationTest, "Craps.InteractiveBetting.OptionBEconomySeparation", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsOptionBEconomySeparationTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(10000); // Massive 10k combo roll

	// Option B enabled (default): 5% cash dividend, min $25
	GS->bEnableScoreCashSeparation = true;
	GS->ScoreToCashDividendRatio = 0.05f;
	GS->MinCashDividendPerBank = 25;

	const int64 SecuredScore = GS->BankUnbankedPool();

	TestEqual(TEXT("Secured score for Ante Progress is full 10,000 chips"), SecuredScore, 10000LL);
	TestEqual(TEXT("Unbanked pool is reset to 0"), GS->GetUnbankedPool(), 0LL);
	// Bankroll was 500 + 5% of 10000 (500) = 1000 (NOT 10500!)
	TestEqual(TEXT("Bankroll increased by 5% cash dividend (500 + 500 = 1000)"), GS->GetPlayerBankroll(), 1000LL);

	// Test Option A fallback mode (Context Preserved)
	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(2000);
	GS->bEnableScoreCashSeparation = false;

	GS->BankUnbankedPool();
	TestEqual(TEXT("Option A unified bankroll receives 100% of score (500 + 2000 = 2500)"), GS->GetPlayerBankroll(), 2500LL);

	return true;
}

// 6. Test Betting Board Open/Close Toggle State
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsBettingBoardToggleTest, "Craps.InteractiveBetting.BettingBoardToggle", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsBettingBoardToggleTest::RunTest(const FString& Parameters)
{
	ACrapsDebugHUD* HUD = NewObject<ACrapsDebugHUD>();
	TestFalse(TEXT("Betting board defaults to closed"), HUD->IsBettingBoardOpen());

	HUD->ToggleBettingBoard();
	TestTrue(TEXT("Betting board toggles to open"), HUD->IsBettingBoardOpen());

	HUD->SetBettingBoardOpen(false);
	TestFalse(TEXT("Betting board sets to closed"), HUD->IsBettingBoardOpen());

	return true;
}

// 7. Test AutoBankOnNextRoll Economy Flag & Safe Redundant Banking
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsAutoBankEconomyFlagTest, "Craps.InteractiveBetting.AutoBankEconomyFlag", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsAutoBankEconomyFlagTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	TestTrue(TEXT("AutoBankOnNextRoll defaults to true"), GS->bAutoBankOnNextRoll);

	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(600);

	// Bank pool
	const int64 Banked = GS->BankUnbankedPool();
	TestEqual(TEXT("Secured full 600 score"), Banked, 600LL);
	TestEqual(TEXT("Pool is now 0"), GS->GetUnbankedPool(), 0LL);
	TestEqual(TEXT("Bankroll increased by min dividend 25 or 5% (500 + 30 = 530)"), GS->GetPlayerBankroll(), 530LL);

	// Subsequent redundant bank call while pool is 0 returns 0 safely
	const int64 RedundantBanked = GS->BankUnbankedPool();
	TestEqual(TEXT("Redundant bank returns 0"), RedundantBanked, 0LL);
	TestEqual(TEXT("Bankroll remains intact at 530"), GS->GetPlayerBankroll(), 530LL);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
