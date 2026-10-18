// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Dice/PDA_DiceFace.h"
#include "Dice/DicePhysicsTestingHarness.h"
#include "Pawn/ThrowStyles/ThrowStyles_Concrete.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FTestWorldDiceScope
{
	UWorld* World = nullptr;

	FTestWorldDiceScope()
	{
		UWorld::InitializationValues IVS;
		IVS.CreatePhysicsScene(true).ShouldSimulatePhysics(true).EnableTraceCollision(true);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &IVS);
		World->bShouldSimulatePhysics = true;
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FTestWorldDiceScope()
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}
};

// 1. Center of Mass Offset Calculation and Clamping
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsCenterOfMassTest, "Craps.Dice.CenterOfMassOffsetCalculation", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsCenterOfMassTest::RunTest(const FString& Parameters)
{
	FTestWorldDiceScope Scope;

	ADieBase* Die = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die spawned successfully"), Die);

	// Baseline: unmodified die has ZeroVector COM offset
	TestEqual(TEXT("Initial aggregate COM offset is zero"), Die->CalculateAggregateCenterOfMassOffset(), FVector::ZeroVector);
	TestEqual(TEXT("Initial local COM offset is zero"), Die->GetLocalCenterOfMass(), FVector::ZeroVector);

	// Direct AdditionalLocalCOMOffset check
	Die->AdditionalLocalCOMOffset = FVector(0.0f, 0.0f, -2.0f);
	TestEqual(TEXT("Aggregate includes AdditionalLocalCOMOffset"), Die->CalculateAggregateCenterOfMassOffset(), FVector(0.0f, 0.0f, -2.0f));

	Die->UpdateMassAndCenterOfMass();
	const FVector AppliedCOM = Die->GetLocalCenterOfMass();
	TestTrue(TEXT("Applied local COM matches additional offset"), AppliedCOM.Equals(FVector(0.0f, 0.0f, -2.0f), 0.01f));

	// Max allowable clamping check: 10cm offset clamped to MaxAllowedCOMOffsetCm (3.5cm)
	Die->AdditionalLocalCOMOffset = FVector(10.0f, 0.0f, 0.0f);
	const FVector ClampedCOM = Die->CalculateAggregateCenterOfMassOffset();
	TestTrue(TEXT("Large COM offset is clamped to MaxAllowedCOMOffsetCm"), FMath::IsNearlyEqual(ClampedCOM.Size(), Die->MaxAllowedCOMOffsetCm, 0.01f));
	TestEqual(TEXT("Clamped COM direction is along +X"), ClampedCOM.GetSafeNormal(), FVector::ForwardVector);

	// Weighted face offset check: FaceUpBiasCm on PlusZ shifts COM toward MinusZ (-Z)
	Die->AdditionalLocalCOMOffset = FVector::ZeroVector;
	UPDA_DiceFace* WeightedFace = NewObject<UPDA_DiceFace>(Die);
	WeightedFace->FaceUpBiasCm = 1.5f;

	Die->EquipFaceAsset(EDieFaceDirection::PlusZ, WeightedFace);
	const FVector FaceCOM = Die->CalculateAggregateCenterOfMassOffset();
	TestTrue(TEXT("PlusZ face up bias shifts COM toward -Z"), FaceCOM.Equals(FVector(0.0f, 0.0f, -1.5f), 0.01f));

	// Removing face restores zero offset
	Die->UnequipFaceAsset(EDieFaceDirection::PlusZ);
	TestEqual(TEXT("Unequipping face restores zero COM offset"), Die->CalculateAggregateCenterOfMassOffset(), FVector::ZeroVector);

	return true;
}

// 2. Equip and Unequip Face Assets Dynamically
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsEquipFaceAssetTest, "Craps.Dice.EquipAndUnequipFaceAsset", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsEquipFaceAssetTest::RunTest(const FString& Parameters)
{
	FTestWorldDiceScope Scope;

	ADieBase* Die = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die spawned successfully"), Die);

	// Default casino values
	TestEqual(TEXT("Default +Z is 1"), Die->GetFaceValueForDirection(EDieFaceDirection::PlusZ), 1);
	TestEqual(TEXT("Default -Z is 6"), Die->GetFaceValueForDirection(EDieFaceDirection::MinusZ), 6);
	TestEqual(TEXT("Default +X is 2"), Die->GetFaceValueForDirection(EDieFaceDirection::PlusX), 2);
	TestEqual(TEXT("Default -X is 5"), Die->GetFaceValueForDirection(EDieFaceDirection::MinusX), 5);
	TestEqual(TEXT("Default +Y is 3"), Die->GetFaceValueForDirection(EDieFaceDirection::PlusY), 3);
	TestEqual(TEXT("Default -Y is 4"), Die->GetFaceValueForDirection(EDieFaceDirection::MinusY), 4);

	// Equip custom face on +Z with FaceValue = 6
	UPDA_DiceFace* CustomFace = NewObject<UPDA_DiceFace>(Die);
	CustomFace->FaceValue = 6;
	CustomFace->FaceUpBiasCm = 1.0f;

	Die->EquipFaceAsset(EDieFaceDirection::PlusZ, CustomFace);
	TestEqual(TEXT("Equipped +Z now reports custom value 6"), Die->GetFaceValueForDirection(EDieFaceDirection::PlusZ), 6);
	TestTrue(TEXT("Equipped face asset is retrieved"), Die->EquippedFaceAssets.FindRef(EDieFaceDirection::PlusZ) == CustomFace);

	// Clear all faces
	Die->ClearAllEquippedFaces();
	TestEqual(TEXT("After ClearAll, +Z reverts to default 1"), Die->GetFaceValueForDirection(EDieFaceDirection::PlusZ), 1);
	TestEqual(TEXT("EquippedFaceAssets is empty"), Die->EquippedFaceAssets.Num(), 0);

	return true;
}

// 3. Throw Style Parameter Validity and Differentiation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsThrowStyleParameterValidityTest, "Craps.Dice.ThrowStyleParameterValidity", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsThrowStyleParameterValidityTest::RunTest(const FString& Parameters)
{
	UThrowStyle_TableSkimmer* Skimmer = NewObject<UThrowStyle_TableSkimmer>();
	UThrowStyle_ParabolicLob* Lob = NewObject<UThrowStyle_ParabolicLob>();
	UThrowStyle_BackWallWhip* Whip = NewObject<UThrowStyle_BackWallWhip>();
	UThrowStyle_MechanicalPlunger* Plunger = NewObject<UThrowStyle_MechanicalPlunger>();

	TestNotNull(TEXT("Skimmer created"), Skimmer);
	TestNotNull(TEXT("Lob created"), Lob);
	TestNotNull(TEXT("Whip created"), Whip);
	TestNotNull(TEXT("Plunger created"), Plunger);

	// Archetype Enum tags
	TestEqual(TEXT("Skimmer enum"), IThrowStyleInterface::Execute_GetThrowArchetype(Skimmer), EThrowArchetype::TableSkimmer);
	TestEqual(TEXT("Lob enum"), IThrowStyleInterface::Execute_GetThrowArchetype(Lob), EThrowArchetype::ParabolicLob);
	TestEqual(TEXT("Whip enum"), IThrowStyleInterface::Execute_GetThrowArchetype(Whip), EThrowArchetype::BackWallWhip);
	TestEqual(TEXT("Plunger enum"), IThrowStyleInterface::Execute_GetThrowArchetype(Plunger), EThrowArchetype::MechanicalPlunger);

	// Speed ranges valid
	TestTrue(TEXT("Skimmer speed range valid"), Skimmer->MaxLaunchSpeedCmPerSec > Skimmer->MinLaunchSpeedCmPerSec && Skimmer->MinLaunchSpeedCmPerSec > 500.0f);
	TestTrue(TEXT("Lob speed range valid"), Lob->MaxLaunchSpeedCmPerSec > Lob->MinLaunchSpeedCmPerSec && Lob->MinLaunchSpeedCmPerSec > 500.0f);
	TestTrue(TEXT("Whip speed range valid"), Whip->MaxLaunchSpeedCmPerSec > Whip->MinLaunchSpeedCmPerSec && Whip->MinLaunchSpeedCmPerSec > 1000.0f);
	TestTrue(TEXT("Plunger speed range valid"), Plunger->MaxLaunchSpeedCmPerSec > Plunger->MinLaunchSpeedCmPerSec && Plunger->MinLaunchSpeedCmPerSec > 800.0f);

	// Pitch angles differentiated
	TestTrue(TEXT("Skimmer has low pitch (<= 5 deg)"), Skimmer->LaunchPitchDegrees <= 5.0f);
	TestTrue(TEXT("Lob has high pitch (>= 30 deg)"), Lob->LaunchPitchDegrees >= 30.0f);
	TestTrue(TEXT("Whip has line-drive pitch"), Whip->LaunchPitchDegrees >= 5.0f && Whip->LaunchPitchDegrees <= 12.0f);

	// Mechanical plunger has zero random angular variance (100% deterministic)
	TestEqual(TEXT("Plunger has 0.0 random angular variance"), Plunger->RandomAngularVarianceDegPerSec, 0.0f);
	TestTrue(TEXT("Skimmer has non-zero angular variance"), Skimmer->RandomAngularVarianceDegPerSec > 0.0f);
	TestTrue(TEXT("Whip has highest angular variance"), Whip->RandomAngularVarianceDegPerSec > Skimmer->RandomAngularVarianceDegPerSec);

	// Trajectory preview calculation returns valid points
	FThrowLaunchParams LaunchParams;
	LaunchParams.NormalizedPower = 0.5f;
	TArray<FVector> Trajectory;
	FVector LandingSpot;

	IThrowStyleInterface::Execute_ComputePreviewTrajectory(Skimmer, FVector(190.0f, 0.0f, 11.0f), LaunchParams, Trajectory, LandingSpot);
	TestTrue(TEXT("Skimmer preview generates points"), Trajectory.Num() >= 2);
	TestTrue(TEXT("Skimmer landing spot is forward of start"), LandingSpot.X > 190.0f);

	return true;
}

// 4. Rigging Toggle and Stat Scaling
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsRiggingToggleTest, "Craps.Dice.RiggingToggleAndStatScaling", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsRiggingToggleTest::RunTest(const FString& Parameters)
{
	FTestWorldDiceScope Scope;

	ADieBase* Die = Scope.World->SpawnActor<ADieBase>();
	TestNotNull(TEXT("Die spawned"), Die);

	UDiceRiggingComponent* Rigging = Die->DiceRiggingComponent;
	TestNotNull(TEXT("DiceRiggingComponent exists"), Rigging);

	// When bRiggingEnabled is false, IsActivelyRigging is false
	Rigging->bRiggingEnabled = false;
	Rigging->DiceControlStat = 0.5f;
	Rigging->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Rigging disabled -> IsActivelyRigging is false"), Rigging->IsActivelyRigging());

	// When DiceControlStat is 0.0, IsActivelyRigging is false
	Rigging->bRiggingEnabled = true;
	Rigging->DiceControlStat = 0.0f;
	Rigging->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Stat at 0.0 -> IsActivelyRigging is false"), Rigging->IsActivelyRigging());

	return true;
}

// 5. Physics Simulation Telemetry Baseline (Chaos Physics Batch)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsPhysicsTelemetryBaselineTest, "Craps.Dice.PhysicsSimulationTelemetryBaseline", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsPhysicsTelemetryBaselineTest::RunTest(const FString& Parameters)
{
	FTestWorldDiceScope Scope;

	FDiceHarnessConfig Config;
	Config.ThrowArchetype = EThrowArchetype::TableSkimmer;
	Config.NormalizedPower = 0.70f;
	Config.NumThrows = 8;
	Config.RandomSeed = 1001;

	FDiceHarnessSummary Summary = UDicePhysicsTestingHarness::RunSimulationBatch(Scope.World, Config);

	TestEqual(TEXT("Total throws matches config"), Summary.TotalThrows, 8);
	TestTrue(TEXT("Valid throws produced (> 0)"), Summary.ValidThrows > 0);
	TestTrue(TEXT("Average duration is within realistic bounds [0.5s .. 5.5s]"), Summary.AverageDurationSeconds >= 0.5f && Summary.AverageDurationSeconds <= 5.5f);

	// Export CSV test
	const bool bExportOk = UDicePhysicsTestingHarness::ExportTelemetryToCsv(Summary, TEXT("TestBaselineTelemetry.csv"));
	TestTrue(TEXT("CSV export succeeded"), bExportOk);

	return true;
}

// 6. Physics Simulation Telemetry with Weighted Faces
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrapsPhysicsTelemetryWeightedTest, "Craps.Dice.PhysicsSimulationTelemetryWeighted", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCrapsPhysicsTelemetryWeightedTest::RunTest(const FString& Parameters)
{
	FTestWorldDiceScope Scope;

	// Create loaded face biasing face 6 (on -Z) to land UP with FaceUpBiasCm = 2.0f
	UPDA_DiceFace* LoadedSix = NewObject<UPDA_DiceFace>(Scope.World);
	LoadedSix->FaceValue = 6;
	LoadedSix->FaceUpBiasCm = 2.0f;

	FDiceHarnessConfig Config;
	Config.ThrowArchetype = EThrowArchetype::TableSkimmer;
	Config.NormalizedPower = 0.65f;
	Config.NumThrows = 8;
	Config.RandomSeed = 2002;
	Config.Die1EquippedFaces.Add(EDieFaceDirection::MinusZ, LoadedSix);
	Config.Die2EquippedFaces.Add(EDieFaceDirection::MinusZ, LoadedSix);

	FDiceHarnessSummary Summary = UDicePhysicsTestingHarness::RunSimulationBatch(Scope.World, Config);

	TestEqual(TEXT("Weighted batch total throws matches config"), Summary.TotalThrows, 8);
	TestTrue(TEXT("Weighted batch produces valid throws"), Summary.ValidThrows > 0);
	TestTrue(TEXT("Weighted dice settle cleanly without infinite timeout"), Summary.NoRollThrows < Summary.TotalThrows);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
