// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Items/PDA_Trinket.h"
#include "Items/TrinketInventoryComponent.h"
#include "Table/CrapsGameState.h"
#include "Core/CrapsTypes.h"
#include "Pawn/CrapsPawn.h"

#if WITH_DEV_AUTOMATION_TESTS

// 1. Test Trinket Equip & Slot Capacity Constraints
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketSlotCapacityTest, "Craps.Trinkets.SlotCapacityConstraints", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketSlotCapacityTest::RunTest(const FString& Parameters)
{
	UTrinketInventoryComponent* Inv = NewObject<UTrinketInventoryComponent>();
	TestEqual(TEXT("Initial equipped count is 0"), Inv->GetEquippedCount(), 0);

	// Create 5 finger rings
	TArray<UPDA_Trinket*> Rings;
	for (int32 i = 0; i < 5; ++i)
	{
		UPDA_Trinket* Ring = NewObject<UPDA_Trinket>();
		Ring->SlotType = ETrinketSlotType::FingerRing;
		Ring->TrinketName = FText::FromString(FString::Printf(TEXT("Ring_%d"), i));
		Rings.Add(Ring);
	}

	// Equip 4 rings (max capacity)
	TestTrue(TEXT("Equip Ring 0"), Inv->EquipTrinket(Rings[0]));
	TestTrue(TEXT("Equip Ring 1"), Inv->EquipTrinket(Rings[1]));
	TestTrue(TEXT("Equip Ring 2"), Inv->EquipTrinket(Rings[2]));
	TestTrue(TEXT("Equip Ring 3"), Inv->EquipTrinket(Rings[3]));
	TestEqual(TEXT("Equipped count is now 4"), Inv->GetEquippedCount(), 4);

	// 5th ring should fail due to full ring slots
	TestFalse(TEXT("5th ring should fail when slots are full"), Inv->EquipTrinket(Rings[4]));
	TestEqual(TEXT("Equipped count remains 4"), Inv->GetEquippedCount(), 4);

	// Wristwear (Capacity = 1)
	UPDA_Trinket* Wrist1 = NewObject<UPDA_Trinket>();
	Wrist1->SlotType = ETrinketSlotType::WristwearActive;
	TestTrue(TEXT("Equip Wrist 1"), Inv->EquipTrinket(Wrist1));
	TestEqual(TEXT("Equipped count is now 5 (4 rings + 1 wrist)"), Inv->GetEquippedCount(), 5);

	UPDA_Trinket* Wrist2 = NewObject<UPDA_Trinket>();
	Wrist2->SlotType = ETrinketSlotType::WristwearActive;
	TestTrue(TEXT("Equip Wrist 2 replaces Wrist 1"), Inv->EquipTrinket(Wrist2));
	TestEqual(TEXT("Equipped count is still 5"), Inv->GetEquippedCount(), 5);
	TestTrue(TEXT("Inv has Wrist 2"), Inv->HasTrinket(Wrist2));
	TestFalse(TEXT("Inv no longer has Wrist 1"), Inv->HasTrinket(Wrist1));

	// Rail Props (Capacity = 2)
	UPDA_Trinket* Prop1 = NewObject<UPDA_Trinket>();
	Prop1->SlotType = ETrinketSlotType::RailProp;
	UPDA_Trinket* Prop2 = NewObject<UPDA_Trinket>();
	Prop2->SlotType = ETrinketSlotType::RailProp;
	UPDA_Trinket* Prop3 = NewObject<UPDA_Trinket>();
	Prop3->SlotType = ETrinketSlotType::RailProp;

	TestTrue(TEXT("Equip Prop 1"), Inv->EquipTrinket(Prop1));
	TestTrue(TEXT("Equip Prop 2"), Inv->EquipTrinket(Prop2));
	TestEqual(TEXT("Equipped count is now 7 (4 rings + 1 wrist + 2 props)"), Inv->GetEquippedCount(), 7);
	TestFalse(TEXT("3rd prop fails"), Inv->EquipTrinket(Prop3));

	// Unequip test
	TestTrue(TEXT("Unequip Ring 0"), Inv->UnequipTrinket(Rings[0]));
	TestEqual(TEXT("Equipped count is now 6"), Inv->GetEquippedCount(), 6);
	TestFalse(TEXT("Ring 0 is no longer equipped"), Inv->HasTrinket(Rings[0]));

	// Clear all
	Inv->ClearAllTrinkets();
	TestEqual(TEXT("All cleared to 0"), Inv->GetEquippedCount(), 0);

	return true;
}

// 2. Test Odd / Even Sum Splitting Synergy (Neon Signet Pattern)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketOddEvenSplitTest, "Craps.Trinkets.OddEvenSplitSynergy", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketOddEvenSplitTest::RunTest(const FString& Parameters)
{
	UPDA_Trinket* NeonSignet = NewObject<UPDA_Trinket>();
	NeonSignet->TrinketName = FText::FromString(TEXT("Neon Signet"));
	NeonSignet->bSplitOddEven = true;
	NeonSignet->OddSumThrowMult = 4.0f;
	NeonSignet->EvenSumBaseChips = 40;

	FScoringStep Step;
	int32 BonusChips = 0;
	float BonusMult = 0.0f;
	float BetMult = 1.0f;

	// Odd roll: 5 + 4 = 9
	FRollEvaluationContext OddContext;
	OddContext.RollSum = 9;
	bool bTriggeredOdd = NeonSignet->EvaluateTrinket(OddContext, Step, BonusChips, BonusMult, BetMult);
	TestTrue(TEXT("Triggers on odd roll 9"), bTriggeredOdd);
	TestEqual(TEXT("Odd roll awards +4.0x throw mult"), BonusMult, 4.0f);
	TestEqual(TEXT("Odd roll awards 0 base chips"), BonusChips, 0);

	// Even roll: 4 + 4 = 8
	FRollEvaluationContext EvenContext;
	EvenContext.RollSum = 8;
	bool bTriggeredEven = NeonSignet->EvaluateTrinket(EvenContext, Step, BonusChips, BonusMult, BetMult);
	TestTrue(TEXT("Triggers on even roll 8"), bTriggeredEven);
	TestEqual(TEXT("Even roll awards +40 chips"), BonusChips, 40);
	TestEqual(TEXT("Even roll awards 0 throw mult"), BonusMult, 0.0f);

	return true;
}

// 3. Test GameplayTag Matching Synergy (Mobius Band / Cobalt Band Pattern)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketTagMatchingHardwayTest, "Craps.Trinkets.TagMatchingHardwaySynergy", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketTagMatchingHardwayTest::RunTest(const FString& Parameters)
{
	UPDA_Trinket* CobaltBand = NewObject<UPDA_Trinket>();
	CobaltBand->TrinketName = FText::FromString(TEXT("Cobalt Band"));
	CobaltBand->TriggerConditionTag = FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnHardway"), false);
	CobaltBand->TargetScoringLayer = EScoringLayer::BetMult;
	CobaltBand->BetMultFactor = 2.0f;

	FScoringStep Step;
	int32 BonusChips = 0;
	float BonusMult = 0.0f;
	float BetMult = 1.0f;

	// Hardway roll: 4-4
	FRollEvaluationContext HardwayContext;
	HardwayContext.bIsHardway = true;
	HardwayContext.TriggerTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnHardway"), false));

	bool bTriggeredHard = CobaltBand->EvaluateTrinket(HardwayContext, Step, BonusChips, BonusMult, BetMult);
	TestTrue(TEXT("Triggers on Hardway"), bTriggeredHard);
	TestEqual(TEXT("Awards 2.0x Bet Mult Factor"), BetMult, 2.0f);

	// Non-hardway roll: 5-3
	FRollEvaluationContext EasyContext;
	EasyContext.bIsHardway = false;
	bool bTriggeredEasy = CobaltBand->EvaluateTrinket(EasyContext, Step, BonusChips, BonusMult, BetMult);
	TestFalse(TEXT("Does not trigger on non-hardway"), bTriggeredEasy);

	return true;
}

// 4. Test Table Bounce Scaling Synergy (Brass Knuckles Pattern)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketBounceScalingTest, "Craps.Trinkets.BounceScalingKnuckles", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketBounceScalingTest::RunTest(const FString& Parameters)
{
	UPDA_Trinket* Knuckles = NewObject<UPDA_Trinket>();
	Knuckles->TrinketName = FText::FromString(TEXT("Brass Knuckle Band"));
	Knuckles->bScaleWithBounces = true;
	Knuckles->ChipsPerBounce = 20;
	Knuckles->ThrowMultPerBounce = 0.25f;

	FScoringStep Step;
	int32 BonusChips = 0;
	float BonusMult = 0.0f;
	float BetMult = 1.0f;

	// 3 bounces
	FRollEvaluationContext BounceContext;
	BounceContext.TotalBounces = 3;
	bool bTriggered = Knuckles->EvaluateTrinket(BounceContext, Step, BonusChips, BonusMult, BetMult);
	TestTrue(TEXT("Triggers on bounces"), bTriggered);
	TestEqual(TEXT("3 bounces * 20 chips = 60 chips"), BonusChips, 60);
	TestEqual(TEXT("3 bounces * 0.25 mult = +0.75x mult"), BonusMult, 0.75f);

	// 0 bounces
	FRollEvaluationContext ZeroBounceContext;
	ZeroBounceContext.TotalBounces = 0;
	bool bTriggeredZero = Knuckles->EvaluateTrinket(ZeroBounceContext, Step, BonusChips, BonusMult, BetMult);
	TestFalse(TEXT("Does not trigger on 0 bounces"), bTriggeredZero);

	return true;
}

// 5. Test Miracle Seven Aversion (Onyx Claw Pattern)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketMiracleSevenTest, "Craps.Trinkets.MiracleSevenAversion", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketMiracleSevenTest::RunTest(const FString& Parameters)
{
	UPDA_Trinket* OnyxClaw = NewObject<UPDA_Trinket>();
	OnyxClaw->TrinketName = FText::FromString(TEXT("Onyx Claw"));
	OnyxClaw->bIsMiracleSevenTrinket = true;
	OnyxClaw->MinHazardsForMiracleSeven = 2;
	OnyxClaw->MiracleSevenBonusChips = 150;

	FScoringStep Step;
	int32 BonusChips = 0;

	// Valid Miracle: Roll 7 during PointEstablished with 2 hazards hit
	FRollEvaluationContext MiracleContext;
	MiracleContext.RollSum = 7;
	MiracleContext.TablePhase = ECrapsTablePhase::PointEstablished;
	MiracleContext.MidRollHazardsHit = 2;

	bool bAverted = OnyxClaw->CheckMiracleSevenAverted(MiracleContext, Step, BonusChips);
	TestTrue(TEXT("Seven-Out averted with 2 hazards hit"), bAverted);
	TestEqual(TEXT("Miracle Seven awards +150 chips"), BonusChips, 150);

	// Insufficient hazards: only 1 hazard hit
	FRollEvaluationContext LowHazardsContext;
	LowHazardsContext.RollSum = 7;
	LowHazardsContext.TablePhase = ECrapsTablePhase::PointEstablished;
	LowHazardsContext.MidRollHazardsHit = 1;

	bool bNotAverted = OnyxClaw->CheckMiracleSevenAverted(LowHazardsContext, Step, BonusChips);
	TestFalse(TEXT("Seven-Out NOT averted with only 1 hazard hit"), bNotAverted);

	// Come-Out roll: rolling 7 is a natural, not Seven-Out
	FRollEvaluationContext ComeOutContext;
	ComeOutContext.RollSum = 7;
	ComeOutContext.TablePhase = ECrapsTablePhase::ComeOutRoll;
	ComeOutContext.MidRollHazardsHit = 2;

	bool bComeOutNotAverted = OnyxClaw->CheckMiracleSevenAverted(ComeOutContext, Step, BonusChips);
	TestFalse(TEXT("Does not trigger on Come-Out natural 7"), bComeOutNotAverted);

	return true;
}

// 6. Test Authoritative GameState Multiplier Stacking with Trinkets
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsTrinketGameStateIntegrationTest, "Craps.Trinkets.GameStateMultiplierStacking", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsTrinketGameStateIntegrationTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	UTrinketInventoryComponent* Inv = NewObject<UTrinketInventoryComponent>();
	GS->BindTrinketInventory(Inv);

	// Equip Neon Signet (+4.0x mult on odd sum)
	UPDA_Trinket* NeonSignet = NewObject<UPDA_Trinket>();
	NeonSignet->TrinketName = FText::FromString(TEXT("Neon Signet"));
	NeonSignet->SlotType = ETrinketSlotType::FingerRing;
	NeonSignet->bSplitOddEven = true;
	NeonSignet->OddSumThrowMult = 4.0f;
	NeonSignet->EvenSumBaseChips = 40;
	Inv->EquipTrinket(NeonSignet);

	// Equip Pyrite Ring (+1.5x mult on back wall bounce)
	UPDA_Trinket* PyriteRing = NewObject<UPDA_Trinket>();
	PyriteRing->TrinketName = FText::FromString(TEXT("Pyrite Ring"));
	PyriteRing->SlotType = ETrinketSlotType::FingerRing;
	PyriteRing->TriggerConditionTag = FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnWallBounce"), false);
	PyriteRing->TargetScoringLayer = EScoringLayer::ThrowMult;
	PyriteRing->BonusThrowMult = 1.5f;
	Inv->EquipTrinket(PyriteRing);

	// Roll 4 + 5 = 9 (Odd sum), hit back wall (bHitBackWall = true)
	// Base face yields: 40 + 50 = 90 chips
	// Throw mult: 1.0 (base) + 0.5 (alligator wall) + 4.0 (Neon Signet odd) + 1.5 (Pyrite Ring wall) = 7.0x!
	// Payout = 90 * 7.0 = 630 chips
	GS->ResolveSettledRoll(4, 40, FVector::ZeroVector, 5, 50, FVector::ZeroVector, true, 1);

	TestEqual(TEXT("Unbanked pool received 630 chips from stacked trinket multipliers"), GS->GetUnbankedPool(), 630LL);

	// Unequip trinkets and roll another 9 under same wall conditions on a fresh Come-Out
	Inv->ClearAllTrinkets();
	GS->SetUnbankedPool(0);
	GS->SetTablePhase(ECrapsTablePhase::ComeOutRoll, 0);

	// Base 90 chips * (1.0 base + 0.5 wall) = 135 chips (Standard payout without trinkets)
	GS->ResolveSettledRoll(4, 40, FVector::ZeroVector, 5, 50, FVector::ZeroVector, true, 1);
	TestEqual(TEXT("Standard roll without trinkets pays 135 chips"), GS->GetUnbankedPool(), 135LL);

	return true;
}

// 7. Test Left-Hand 3D Rig Positioning, Components & Free-Look Mode
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsLeftHandVisualRigTest, "Craps.Trinkets.LeftHandRigAndFreeLook", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsLeftHandVisualRigTest::RunTest(const FString& Parameters)
{
	ACrapsPawn* Pawn = NewObject<ACrapsPawn>();
	TestNotNull(TEXT("Pawn instantiated"), Pawn);
	if (!Pawn)
	{
		return false;
	}

	// 1. Verify Left-Hand Anchor is positioned in front of the default camera (X=85, Y=-24, Z=132)
	TestNotNull(TEXT("LeftHandRailAnchor exists"), Pawn->LeftHandRailAnchor.Get());
	if (Pawn->LeftHandRailAnchor)
	{
		const FVector RelLoc = Pawn->LeftHandRailAnchor->GetRelativeLocation();
		TestEqual(TEXT("Anchor X is 85cm"), RelLoc.X, 85.0);
		TestEqual(TEXT("Anchor Y is -24cm"), RelLoc.Y, -24.0);
		TestEqual(TEXT("Anchor Z is 132cm"), RelLoc.Z, 132.0);
	}

	// 2. Verify Hand Sub-object Components exist
	TestNotNull(TEXT("LeftForearmMesh exists"), Pawn->LeftForearmMesh.Get());
	TestNotNull(TEXT("LeftWristMesh exists"), Pawn->LeftWristMesh.Get());
	TestNotNull(TEXT("LeftPalmMesh exists"), Pawn->LeftPalmMesh.Get());
	TestNotNull(TEXT("LeftRailPropMesh exists"), Pawn->LeftRailPropMesh.Get());
	TestEqual(TEXT("LeftFingerMeshes count is 4"), Pawn->LeftFingerMeshes.Num(), 4);
	TestEqual(TEXT("LeftRingMeshes count is 4"), Pawn->LeftRingMeshes.Num(), 4);

	// 3. Verify Free-Look Toggle
	TestFalse(TEXT("bIsFreeLooking is initially false"), Pawn->IsFreeLooking());
	Pawn->Input_FreeLookPress();
	TestTrue(TEXT("bIsFreeLooking is true after press"), Pawn->IsFreeLooking());
	Pawn->Input_FreeLookRelease();
	TestFalse(TEXT("bIsFreeLooking is false after release"), Pawn->IsFreeLooking());

	// 4. Verify Socket World Locations are forward of origin (+X > 0)
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector KnuckleLoc = Pawn->GetFingerKnuckleWorldLocation(i);
		TestTrue(TEXT("Knuckle location is forward of pawn"), KnuckleLoc.X > 0.0f);
	}
	const FVector WristLoc = Pawn->GetWristWorldLocation();
	TestTrue(TEXT("Wrist location is forward of pawn"), WristLoc.X > 0.0f);

	const FVector PropLoc = Pawn->GetRailPropWorldLocation(0);
	TestTrue(TEXT("Prop location is forward of pawn"), PropLoc.X > 0.0f);

	return true;
}

// 8. Test Balatro-Style Roll Scoring Breakdown Data Structure & Accuracy
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsBalatroRollBreakdownTest, "Craps.Trinkets.BalatroRollBreakdownTransparency", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsBalatroRollBreakdownTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	TestNotNull(TEXT("GameState instantiated"), GS);
	if (!GS)
	{
		return false;
	}

	UTrinketInventoryComponent* Inv = NewObject<UTrinketInventoryComponent>(GS);
	GS->BindTrinketInventory(Inv);

	// Equip Neon Signet (Odd sum -> +4.0x Throw Mult)
	UPDA_Trinket* NeonSignet = NewObject<UPDA_Trinket>();
	NeonSignet->TrinketName = FText::FromString(TEXT("Neon Signet"));
	NeonSignet->SlotType = ETrinketSlotType::FingerRing;
	NeonSignet->bSplitOddEven = true;
	NeonSignet->OddSumThrowMult = 4.0f;
	NeonSignet->EvenSumBaseChips = 40;
	Inv->EquipTrinket(NeonSignet, 0);

	// Roll 4 + 5 = 9 (Odd sum) with BackWall = true
	// Base chips: 40 + 50 = 90
	// Throw mult: 1.0 (base) + 0.5 (wall) + 4.0 (Neon Signet) = 5.5x
	// Expected payout: 90 * 5.5 = 495
	GS->ResolveSettledRoll(4, 40, FVector::ZeroVector, 5, 50, FVector::ZeroVector, true, 0);

	const FRollScoringBreakdown& BD = GS->GetLastRollBreakdown();
	TestTrue(TEXT("Breakdown has data"), BD.bHasData);
	TestEqual(TEXT("Die1Value is 4"), BD.Die1Value, 4);
	TestEqual(TEXT("Die2Value is 5"), BD.Die2Value, 5);
	TestEqual(TEXT("RollSum is 9"), BD.RollSum, 9);
	TestEqual(TEXT("TotalBaseChips is 90"), BD.TotalBaseChips, 90LL);
	TestEqual(TEXT("FinalThrowMult is 5.5x"), BD.FinalThrowMult, 5.5f);
	TestEqual(TEXT("TotalPayout is 495 chips"), BD.TotalPayout, 495LL);
	TestTrue(TEXT("Steps array is populated with contributing bonuses"), BD.Steps.Num() >= 3);

	// Verify step contents
	bool bFoundNeonSignet = false;
	bool bFoundWall = false;
	for (const FRollBreakdownItem& Step : BD.Steps)
	{
		if (Step.SourceName.Contains(TEXT("Neon Signet")))
		{
			bFoundNeonSignet = true;
			TestEqual(TEXT("Neon Signet multiplier is +4.0x"), Step.MultiplierDelta, 4.0f);
		}
		if (Step.SourceName.Contains(TEXT("BACK-WALL")))
		{
			bFoundWall = true;
			TestEqual(TEXT("Wall multiplier is +0.5x"), Step.MultiplierDelta, 0.5f);
		}
	}
	TestTrue(TEXT("Breakdown contains Neon Signet step"), bFoundNeonSignet);
	TestTrue(TEXT("Breakdown contains Back-Wall step"), bFoundWall);

	return true;
}

// 9. Test Bankroll Financial Ledger Breakdown & Cash Dividend Transparency
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsBankrollCashBreakdownTest, "Craps.Trinkets.BankrollCashBreakdownTransparency", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsBankrollCashBreakdownTest::RunTest(const FString& Parameters)
{
	ACrapsGameState* GS = NewObject<ACrapsGameState>();
	TestNotNull(TEXT("GameState instantiated"), GS);
	if (!GS)
	{
		return false;
	}

	GS->SetPlayerBankroll(500);
	GS->SetUnbankedPool(1000);

	// Bank the 1,000 combo score: with 5% dividend, awards +$50 cash
	GS->BankUnbankedPool();

	const FRollBankrollBreakdown& BBD = GS->GetLastBankrollBreakdown();
	TestTrue(TEXT("Bankroll breakdown has data"), BBD.bHasData);
	TestEqual(TEXT("Starting cash was 500"), BBD.StartingCash, 500LL);
	TestEqual(TEXT("Cash dividend awarded was 50"), BBD.CashDividendAwarded, 50LL);
	TestEqual(TEXT("Ending cash is 550"), BBD.EndingCash, 550LL);
	TestEqual(TEXT("Unbanked score is 0 after banking"), BBD.UnbankedScore, 0LL);
	TestTrue(TEXT("Action summary explains banking event"), BBD.ActionSummary.Contains(TEXT("Banked")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
