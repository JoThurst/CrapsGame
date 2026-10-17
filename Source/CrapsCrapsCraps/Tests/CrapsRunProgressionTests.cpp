// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Progression/RunProgressionSubsystem.h"
#include "Progression/RunProgressionTypes.h"
#include "Progression/PDA_RunProgressionConfig.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Core/CrapsTypes.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

// Helper to construct a clean test world
struct FTestWorldProgressionScope
{
	UWorld* World = nullptr;

	FTestWorldProgressionScope()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FTestWorldProgressionScope()
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}
};

// 1. Test Initial Run State and Configuration
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionInitialStateTest, "Craps.Progression.InitialRunState", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionInitialStateTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	TestNotNull(TEXT("RunProgressionSubsystem exists"), Subsystem);

	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	TestNotNull(TEXT("CrapsGameState spawned"), GS);

	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	TestEqual(TEXT("Run state is ActiveAnte"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::ActiveAnte);
	TestEqual(TEXT("Current ante is 1"), Subsystem->GetCurrentAnteIndex(), 1);
	TestEqual(TEXT("Starting progress is 0"), Subsystem->GetCurrentAnteProgress(), 0LL);
	TestEqual(TEXT("Starting bankroll is 500"), GS->GetPlayerBankroll(), 500LL);
	TestEqual(TEXT("Ante 1 Target Quota is 1000"), Subsystem->GetCurrentAnteTargetQuota(), 1000LL);
	TestEqual(TEXT("Ante 1 Roll Allowance is 15"), Subsystem->GetRollsRemaining(), 15);

	return true;
}

// 2. Test Roll Consumption Per Resolved Physical Throw
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionRollConsumptionTest, "Craps.Progression.RollConsumptionPerSettledThrow", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionRollConsumptionTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	TestEqual(TEXT("Initial rolls remaining is 15"), Subsystem->GetRollsRemaining(), 15);

	// Resolve a throw (e.g. 4 + 2 = 6, Point Established)
	GS->ResolveSettledRoll(4, 0, FVector::ZeroVector, 2, 0, FVector::ZeroVector, false, 0);
	TestEqual(TEXT("Rolls remaining decrements to 14 after first throw"), Subsystem->GetRollsRemaining(), 14);

	// Resolve a second throw
	GS->ResolveSettledRoll(3, 0, FVector::ZeroVector, 5, 0, FVector::ZeroVector, false, 0);
	TestEqual(TEXT("Rolls remaining decrements to 13 after second throw"), Subsystem->GetRollsRemaining(), 13);

	return true;
}

// 3. Test Banking Secures Winnings For Ante Progress
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionBankingSecuresWinningsTest, "Craps.Progression.BankingSecuresWinningsForAnteProgress", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionBankingSecuresWinningsTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	// Inject unbanked pool chips on the table
	GS->SetUnbankedPool(400);
	TestEqual(TEXT("Unbanked pool is 400"), GS->GetUnbankedPool(), 400LL);
	TestEqual(TEXT("Ante progress remains 0 while unbanked"), Subsystem->GetCurrentAnteProgress(), 0LL);

	// Manual Banking: Cash out pool via BankUnbankedPool()
	const int64 Banked = GS->BankUnbankedPool();
	TestEqual(TEXT("Banked amount is 400"), Banked, 400LL);
	TestEqual(TEXT("Unbanked pool is emptied to 0"), GS->GetUnbankedPool(), 0LL);
	TestEqual(TEXT("Player bankroll increases by 400 (500 + 400 = 900)"), GS->GetPlayerBankroll(), 900LL);
	TestEqual(TEXT("Ante progress awarded 400 qualifying winnings"), Subsystem->GetCurrentAnteProgress(), 400LL);

	// If player has unbanked chips and a Seven-Out occurs, unbanked pool is wiped and NO ante progress is gained
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 8);
	GS->SetUnbankedPool(300);
	TestEqual(TEXT("New unbanked pool is 300"), GS->GetUnbankedPool(), 300LL);

	// Roll 7 -> Seven-Out!
	GS->ResolveSettledRoll(4, 0, FVector::ZeroVector, 3, 0, FVector::ZeroVector, false, 0);
	TestEqual(TEXT("Unbanked pool wiped to 0 on Seven-Out"), GS->GetUnbankedPool(), 0LL);
	TestEqual(TEXT("Ante progress stayed at 400; lost unbanked pool did not count"), Subsystem->GetCurrentAnteProgress(), 400LL);

	return true;
}

// 4. Test Wager Refunds Do Not Count as Progress
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionWagerRefundsNoProgressTest, "Craps.Progression.WagerRefundsDoNotCountAsProgress", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionWagerRefundsNoProgressTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	ABetZone* Zone = Scope.World->SpawnActor<ABetZone>();
	Zone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);
	GS->RegisterBetZone(Zone);

	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	TestEqual(TEXT("Starting bankroll is 500"), GS->GetPlayerBankroll(), 500LL);
	TestEqual(TEXT("Starting ante progress is 0"), Subsystem->GetCurrentAnteProgress(), 0LL);

	// Place 100 on Field
	TestTrue(TEXT("Place 100 on Field"), GS->PlaceWager(Zone, 100));
	TestEqual(TEXT("Bankroll debited to 400"), GS->GetPlayerBankroll(), 400LL);
	TestEqual(TEXT("Ante progress is still 0"), Subsystem->GetCurrentAnteProgress(), 0LL);

	// Remove/Refund 100 from Field
	int64 Refunded = 0;
	TestTrue(TEXT("Remove 100 from Field"), GS->RemoveWager(Zone, 100, Refunded));
	TestEqual(TEXT("Refunded amount is 100"), Refunded, 100LL);
	TestEqual(TEXT("Bankroll restored to 500"), GS->GetPlayerBankroll(), 500LL);
	TestEqual(TEXT("Ante progress remains strictly 0 after wager refund"), Subsystem->GetCurrentAnteProgress(), 0LL);

	return true;
}

// 5. Test Ante Cleared Condition Upon Reaching Target Quota
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionAnteClearedTest, "Craps.Progression.AnteClearedOnReachingTarget", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionAnteClearedTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	TestEqual(TEXT("Run state is ActiveAnte"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::ActiveAnte);
	TestEqual(TEXT("Target quota is 1000"), Subsystem->GetCurrentAnteTargetQuota(), 1000LL);

	// Bank 999 chips (not yet reached)
	GS->SetUnbankedPool(999);
	GS->BankUnbankedPool();
	TestEqual(TEXT("Progress is 999"), Subsystem->GetCurrentAnteProgress(), 999LL);
	TestEqual(TEXT("State still ActiveAnte at 999 / 1000"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::ActiveAnte);

	// Bank 1 chip (meets 1000 quota)
	GS->SetUnbankedPool(1);
	GS->BankUnbankedPool();
	TestEqual(TEXT("Progress reached 1000"), Subsystem->GetCurrentAnteProgress(), 1000LL);
	TestEqual(TEXT("State transitioned to AnteCleared"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::AnteCleared);

	return true;
}

// 6. Test Advancement to Next Ante and Wager Sweeping
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionAdvanceToNextAnteTest, "Craps.Progression.AdvanceToNextAnte", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionAdvanceToNextAnteTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	ABetZone* FieldZone = Scope.World->SpawnActor<ABetZone>();
	FieldZone->InitializeZone(ECrapsBetType::Field, TEXT("FIELD"), 1.5f, 50);
	GS->RegisterBetZone(FieldZone);

	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	// Place 50 on Field
	GS->PlaceWager(FieldZone, 50);
	TestEqual(TEXT("Bankroll debited to 450"), GS->GetPlayerBankroll(), 450LL);

	// Meet Ante 1 quota (1000)
	GS->SetUnbankedPool(1000);
	GS->BankUnbankedPool();
	TestEqual(TEXT("Bankroll is 1450"), GS->GetPlayerBankroll(), 1450LL);
	TestEqual(TEXT("State is AnteCleared"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::AnteCleared);

	// Advance to Ante 2
	TestTrue(TEXT("AdvanceToNextAnte succeeds"), Subsystem->AdvanceToNextAnte());

	TestEqual(TEXT("New Ante is 2"), Subsystem->GetCurrentAnteIndex(), 2);
	TestEqual(TEXT("State is back to ActiveAnte"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::ActiveAnte);
	TestEqual(TEXT("Ante 2 target quota loaded (2500)"), Subsystem->GetCurrentAnteTargetQuota(), 2500LL);
	TestEqual(TEXT("Ante 2 roll allowance loaded (15)"), Subsystem->GetRollsRemaining(), 15);
	TestEqual(TEXT("Ante progress reset to 0 for Ante 2"), Subsystem->GetCurrentAnteProgress(), 0LL);

	// Field wager of 50 was legitimately refunded on ante advancement
	TestEqual(TEXT("Field wager reset to 0"), FieldZone->GetPlacedWager(), 0LL);
	TestEqual(TEXT("Bankroll preserved and refunded (1450 + 50 = 1500)"), GS->GetPlayerBankroll(), 1500LL);

	return true;
}

// 7. Test Run Victory Upon Completing Final Ante
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionRunVictoryTest, "Craps.Progression.RunVictoryOnFinalAnte", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionRunVictoryTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();

	// Create custom 2-ante config for deterministic victory test
	UPDA_RunProgressionConfig* TestConfig = NewObject<UPDA_RunProgressionConfig>(Scope.World);
	TestConfig->Antes.Empty();

	FAnteConfig A1;
	A1.AnteIndex = 1;
	A1.TargetWinningsQuota = 100;
	A1.RollAllowance = 10;
	TestConfig->Antes.Add(A1);

	FAnteConfig A2;
	A2.AnteIndex = 2;
	A2.TargetWinningsQuota = 200;
	A2.RollAllowance = 10;
	TestConfig->Antes.Add(A2);

	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun(TestConfig);

	// Complete Ante 1
	GS->SetUnbankedPool(100);
	GS->BankUnbankedPool();
	TestEqual(TEXT("Ante 1 Cleared"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::AnteCleared);
	Subsystem->AdvanceToNextAnte();
	TestEqual(TEXT("Advanced to Ante 2"), Subsystem->GetCurrentAnteIndex(), 2);

	// Complete Final Ante (Ante 2)
	GS->SetUnbankedPool(200);
	GS->BankUnbankedPool();
	TestEqual(TEXT("Ante 2 Cleared"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::AnteCleared);

	// Advance from final ante -> RunVictory!
	TestTrue(TEXT("Advance on final ante succeeds"), Subsystem->AdvanceToNextAnte());
	TestEqual(TEXT("State transitioned to RunVictory"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::RunVictory);

	return true;
}

// 8. Test Run Defeat When Roll Allowance is Exhausted
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionRunDefeatTest, "Craps.Progression.RunDefeatOnRollExhaustion", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionRunDefeatTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	TestEqual(TEXT("Initial rolls is 15"), Subsystem->GetRollsRemaining(), 15);

	// First roll establishes Point 6 (1 + 5 = 6) with 0 chip yield and no bets
	GS->ResolveSettledRoll(1, 0, FVector::ZeroVector, 5, 0, FVector::ZeroVector, false, 0);
	TestEqual(TEXT("Point 6 established"), GS->GetEstablishedPoint(), 6);
	TestEqual(TEXT("Unbanked pool is 0"), GS->GetUnbankedPool(), 0LL);
	TestEqual(TEXT("Rolls remaining is 14"), Subsystem->GetRollsRemaining(), 14);

	// Exhaust next 13 rolls with non-point rolls (1 + 2 = 3) yielding 0 chips
	for (int32 i = 0; i < 13; ++i)
	{
		GS->ResolveSettledRoll(1, 0, FVector::ZeroVector, 2, 0, FVector::ZeroVector, false, 0);
	}
	TestEqual(TEXT("Rolls remaining is 1"), Subsystem->GetRollsRemaining(), 1);
	TestEqual(TEXT("State is still ActiveAnte"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::ActiveAnte);
	TestEqual(TEXT("Unbanked pool is 0"), GS->GetUnbankedPool(), 0LL);

	// 15th throw exhausts rolls (with progress 0 < 1000 and unbanked pool 0)
	GS->ResolveSettledRoll(1, 0, FVector::ZeroVector, 2, 0, FVector::ZeroVector, false, 0);
	TestEqual(TEXT("Rolls remaining is 0"), Subsystem->GetRollsRemaining(), 0);
	TestEqual(TEXT("State transitioned to RunDefeat"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::RunDefeat);

	return true;
}

// 9. Test Idempotent Advancement (No Duplicate Transitions)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsProgressionIdempotentAdvancementTest, "Craps.Progression.IdempotentAdvancement", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsProgressionIdempotentAdvancementTest::RunTest(const FString& Parameters)
{
	FTestWorldProgressionScope Scope;
	URunProgressionSubsystem* Subsystem = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	Subsystem->BindToGameState(GS);
	Subsystem->StartNewRun();

	// Advance while in ActiveAnte (must fail gracefully)
	TestFalse(TEXT("Cannot advance while in ActiveAnte"), Subsystem->AdvanceToNextAnte());
	TestEqual(TEXT("Ante index remains 1"), Subsystem->GetCurrentAnteIndex(), 1);

	// Meet quota
	GS->SetUnbankedPool(1000);
	GS->BankUnbankedPool();
	TestEqual(TEXT("State is AnteCleared"), (uint8)Subsystem->GetRunState(), (uint8)ERunProgressionState::AnteCleared);

	// Advance once
	TestTrue(TEXT("First advance succeeds"), Subsystem->AdvanceToNextAnte());
	TestEqual(TEXT("Ante index is 2"), Subsystem->GetCurrentAnteIndex(), 2);

	// Immediate second advance must fail because state is now ActiveAnte
	TestFalse(TEXT("Immediate second advance fails"), Subsystem->AdvanceToNextAnte());
	TestEqual(TEXT("Ante index stays 2"), Subsystem->GetCurrentAnteIndex(), 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
