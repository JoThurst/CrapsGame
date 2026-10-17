// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Hazards/TableHazard.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Progression/RunProgressionSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FTestWorldPhysicsScope
{
	UWorld* World = nullptr;

	FTestWorldPhysicsScope()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FTestWorldPhysicsScope()
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}
};

// 1. Test Pinball Bumper Per-Die Debounce and Retrigger Protection
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsPinballBumperDebounceTest, "Craps.Physics.PinballBumperDebounceAndPerDie", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsPinballBumperDebounceTest::RunTest(const FString& Parameters)
{
	FTestWorldPhysicsScope Scope;

	APinballBumperHazard* Bumper = Scope.World->SpawnActor<APinballBumperHazard>();
	TestNotNull(TEXT("Pinball bumper spawned"), Bumper);

	ADieBase* Die1 = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die1 spawned"), Die1);

	ADieBase* Die2 = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die2 spawned"), Die2);

	TestEqual(TEXT("Initial hit count for Die1 is 0"), Bumper->GetDieHitCount(Die1), 0);
	TestEqual(TEXT("Initial hit count for Die2 is 0"), Bumper->GetDieHitCount(Die2), 0);

	// Case A: Below minimum trigger speed (gentle resting contact) -> MUST NOT trigger
	UStaticMeshComponent* Mesh1 = Die1->DieMesh;
	TestNotNull(TEXT("Die1 mesh exists"), Mesh1);
	Mesh1->SetSimulatePhysics(true);
	Mesh1->SetPhysicsLinearVelocity(FVector(50.0f, 0.0f, 0.0f)); // < 120 cm/s threshold

	FHitResult FakeHit;
	FakeHit.ImpactPoint = Bumper->GetActorLocation();
	FakeHit.ImpactNormal = FVector(-1.0f, 0.0f, 0.0f);

	// Trigger component hit
	UFunction* HitFunc = Bumper->FindFunction(TEXT("HandleBumperHit"));
	TestNotNull(TEXT("Bumper has HandleBumperHit"), HitFunc);

	struct FBumperHitParams
	{
		UPrimitiveComponent* HitComponent;
		AActor* OtherActor;
		UPrimitiveComponent* OtherComp;
		FVector NormalImpulse;
		FHitResult Hit;
	};
	FBumperHitParams Params;
	Params.HitComponent = nullptr;
	Params.OtherActor = Die1;
	Params.OtherComp = Mesh1;
	Params.NormalImpulse = FVector::ZeroVector;
	Params.Hit = FakeHit;

	Bumper->ProcessEvent(HitFunc, &Params);
	TestEqual(TEXT("Sub-threshold speed does not increment Die1 hit count"), Bumper->GetDieHitCount(Die1), 0);

	// Case B: High-speed strike (350 cm/s) -> Triggers hit 1
	Mesh1->SetPhysicsLinearVelocity(FVector(350.0f, 0.0f, 0.0f));
	Bumper->ProcessEvent(HitFunc, &Params);
	TestEqual(TEXT("Die1 hit count is now 1"), Bumper->GetDieHitCount(Die1), 1);

	// Case C: Immediate rapid retrigger (cooldown violation) -> Blocked
	Bumper->ProcessEvent(HitFunc, &Params);
	TestEqual(TEXT("Immediate re-strike blocked by cooldown, hit count stays 1"), Bumper->GetDieHitCount(Die1), 1);

	// Case D: Separate Die2 strikes -> Independent per-die tracking
	UStaticMeshComponent* Mesh2 = Die2->DieMesh;
	TestNotNull(TEXT("Die2 mesh exists"), Mesh2);
	Mesh2->SetSimulatePhysics(true);
	Mesh2->SetPhysicsLinearVelocity(FVector(400.0f, 0.0f, 0.0f));
	Params.OtherActor = Die2;
	Params.OtherComp = Mesh2;

	Bumper->ProcessEvent(HitFunc, &Params);
	TestEqual(TEXT("Die2 independently triggers hit 1"), Bumper->GetDieHitCount(Die2), 1);
	TestEqual(TEXT("Die1 hit count still 1"), Bumper->GetDieHitCount(Die1), 1);

	// Case E: Reset for new throw clears all per-die records
	Bumper->ResetForNewThrow();
	TestEqual(TEXT("Die1 hit count reset to 0 after ResetForNewThrow"), Bumper->GetDieHitCount(Die1), 0);
	TestEqual(TEXT("Die2 hit count reset to 0 after ResetForNewThrow"), Bumper->GetDieHitCount(Die2), 0);

	return true;
}

// 2. Test Cocked Die Alignment Detection
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsCockedDieRejectionTest, "Craps.Physics.CockedDieRejection", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsCockedDieRejectionTest::RunTest(const FString& Parameters)
{
	FTestWorldPhysicsScope Scope;

	ADieBase* Die = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die spawned"), Die);

	// Flat on felt: Pitch=0, Roll=0, Yaw=0 -> +Z points straight up
	Die->SetActorRotation(FRotator::ZeroRotator);
	float TopDot = 0.0f;
	EDieFaceDirection TopDir = Die->GetTopFaceDirection(TopDot);
	TestEqual(TEXT("Top face direction is PlusZ when flat"), (uint8)TopDir, (uint8)EDieFaceDirection::PlusZ);
	TestTrue(TEXT("TopDot is nearly 1.0 when flat"), TopDot >= 0.99f);
	TestFalse(TEXT("Die is not cocked when flat"), TopDot < Die->CockedAlignmentThreshold);

	// Tilted 35 degrees against a hazard or rail: Cos(35 deg) = ~0.819 < 0.92 threshold
	Die->SetActorRotation(FRotator(35.0f, 0.0f, 0.0f));
	TopDir = Die->GetTopFaceDirection(TopDot);
	TestTrue(TEXT("TopDot is < CockedAlignmentThreshold (0.92) when tilted 35 degrees"), TopDot < Die->CockedAlignmentThreshold);

	// Tilted 45 degrees: Cos(45 deg) = ~0.707 -> strictly cocked
	Die->SetActorRotation(FRotator(45.0f, 0.0f, 0.0f));
	TopDir = Die->GetTopFaceDirection(TopDot);
	TestTrue(TEXT("TopDot is ~0.707 at 45 degree tilt"), TopDot < Die->CockedAlignmentThreshold);

	return true;
}

// 3. Test No-Roll Preserves Wagers, Bankroll, and Progression Rolls
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsNoRollPreservesWagersAndRollsTest, "Craps.Physics.NoRollPreservesWagersAndRolls", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsNoRollPreservesWagersAndRollsTest::RunTest(const FString& Parameters)
{
	FTestWorldPhysicsScope Scope;

	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	TestNotNull(TEXT("GameState spawned"), GS);

	ABetZone* PassZone = Scope.World->SpawnActor<ABetZone>();
	PassZone->InitializeZone(ECrapsBetType::PassLine, TEXT("PASS LINE"), 2.0f, 50);
	GS->RegisterBetZone(PassZone);

	// Setup Progression Subsystem
	URunProgressionSubsystem* Progression = Scope.World->GetSubsystem<URunProgressionSubsystem>();
	TestNotNull(TEXT("RunProgressionSubsystem exists"), Progression);
	Progression->BindToGameState(GS);
	Progression->StartNewRun();

	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(150);
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);

	// Place 100 on Pass Line during ComeOutRoll: bankroll becomes 400
	TestTrue(TEXT("Place 100 on Pass Line"), GS->PlaceWager(PassZone, 100));
	TestEqual(TEXT("Bankroll debited to 400"), GS->GetPlayerBankroll(), 400LL);
	TestEqual(TEXT("Pass Line wager is 100"), PassZone->GetPlacedWager(), 100LL);
	TestEqual(TEXT("Unbanked pool is 150"), GS->GetUnbankedPool(), 150LL);

	// Advance table to PointEstablished (Point = 6)
	GS->SetTablePhase(ECrapsTablePhase::PointEstablished, 6);

	const int32 InitialRolls = Progression->GetRollsRemaining();
	const int64 InitialProgress = Progression->GetCurrentAnteProgress();

	// Trigger explicit No-Roll
	GS->HandleNoRoll(TEXT("Short Throw — Failed to cross mid-table"));

	// Verify complete preservation
	TestEqual(TEXT("Player bankroll remains 400"), GS->GetPlayerBankroll(), 400LL);
	TestEqual(TEXT("Unbanked pool remains 150"), GS->GetUnbankedPool(), 150LL);
	TestEqual(TEXT("Pass Line wager remains 100 on the felt"), PassZone->GetPlacedWager(), 100LL);
	TestEqual(TEXT("Table phase remains PointEstablished"), (uint8)GS->GetTablePhase(), (uint8)ECrapsTablePhase::PointEstablished);
	TestEqual(TEXT("Established Point remains 6"), GS->GetEstablishedPoint(), 6);
	TestEqual(TEXT("Roll allowance was NOT consumed"), Progression->GetRollsRemaining(), InitialRolls);
	TestEqual(TEXT("Ante progress was NOT altered"), Progression->GetCurrentAnteProgress(), InitialProgress);
	TestTrue(TEXT("Banner indicates No-Roll"), GS->GetLastRollOutcomeBanner().Contains(TEXT("NO ROLL!")));

	return true;
}

// 4. Test Voiding Mid-Roll Hazard Scoring on Interrupted / Invalid Throws
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsVoidMidRollScoringTest, "Craps.Physics.VoidMidRollScoringOnInterruptedThrow", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsVoidMidRollScoringTest::RunTest(const FString& Parameters)
{
	FTestWorldPhysicsScope Scope;

	UScoringQueueSubsystem* ScoringQueue = Scope.World->GetSubsystem<UScoringQueueSubsystem>();
	TestNotNull(TEXT("ScoringQueue exists"), ScoringQueue);

	ACrapsGameState* GS = Scope.World->SpawnActor<ACrapsGameState>();
	TestNotNull(TEXT("GameState spawned"), GS);
	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(0);

	ScoringQueue->BeginNewThrowScoringSession();

	// Simulate die slicing through laser gate and hitting bumper during flight
	ScoringQueue->EnqueueMidRollCollisionStep(nullptr, FVector::ZeroVector, TEXT("LASER TRIPWIRE"), 25, 1.0f);
	ScoringQueue->EnqueueMidRollCollisionStep(nullptr, FVector::ZeroVector, TEXT("PINBALL BUMPER"), 35, 0.5f);

	TestEqual(TEXT("RunningBaseChips accumulated to 60"), ScoringQueue->RunningBaseChips, 60);
	TestEqual(TEXT("RunningThrowMult accumulated to 2.5x"), ScoringQueue->RunningThrowMult, 2.5f);

	// Throw is interrupted by shooter or flagged as short throw No-Roll
	GS->HandleNoRoll(TEXT("Shooter Interrupted Throw"));

	// Verify scoring was completely voided
	TestEqual(TEXT("RunningBaseChips purged to 0"), ScoringQueue->RunningBaseChips, 0);
	TestEqual(TEXT("RunningThrowMult reset to 1.0f"), ScoringQueue->RunningThrowMult, 1.0f);
	TestEqual(TEXT("No unbanked pool awarded to player"), GS->GetUnbankedPool(), 0LL);
	TestEqual(TEXT("Player bankroll remains 500"), GS->GetPlayerBankroll(), 500LL);

	return true;
}

// 5. Test AvoidSevenOut PD Damping and Partner Stability Guard
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsAvoidSevenOutStabilizationTest, "Craps.Physics.AvoidSevenOutStabilization", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsAvoidSevenOutStabilizationTest::RunTest(const FString& Parameters)
{
	FTestWorldPhysicsScope Scope;

	ADieBase* Die1 = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die1 spawned"), Die1);

	ADieBase* Die2 = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die2 spawned"), Die2);

	UDiceRiggingComponent* Rigging = Die1->DiceRiggingComponent;
	TestNotNull(TEXT("Die1 has DiceRiggingComponent"), Rigging);

	Rigging->RiggingMode = EDiceRiggingMode::AvoidSevenOut;
	Rigging->DiceControlStat = 0.8f;
	Rigging->PartnerDie = Die2;

	TestTrue(TEXT("MaxAngularVelocityRadPerSec has a stable limit"), Rigging->MaxAngularVelocityRadPerSec > 0.0f);
	TestTrue(TEXT("AngularStabilizationDamping is active"), Rigging->AngularStabilizationDamping > 0.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
