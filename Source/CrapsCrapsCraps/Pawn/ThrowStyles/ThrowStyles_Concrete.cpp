// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawn/ThrowStyles/ThrowStyles_Concrete.h"

// ============================================================================
// F1: Table-Skimmer
// ============================================================================
UThrowStyle_TableSkimmer::UThrowStyle_TableSkimmer()
{
	Archetype = EThrowArchetype::TableSkimmer;
	DisplayName = FText::FromString(TEXT("F1: Table-Skimmer (Low Felt Skip & Slide)"));
	MinLaunchSpeedCmPerSec = 1050.0f;
	MaxLaunchSpeedCmPerSec = 2350.0f;
	LaunchPitchDegrees = 3.5f; // Skips low across the felt into the bottom alligator pyramids
	BaseAngularTumbleDegPerSec = 320.0f;
	RandomAngularVarianceDegPerSec = 95.0f;
}

FVector UThrowStyle_TableSkimmer::ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const
{
	const float DieSign = (DieIndex == 0) ? 1.0f : -1.0f;
	const float YawSpin = (LaunchParams.LateralSpin * 260.0f) + FMath::FRandRange(-RandomAngularVarianceDegPerSec, RandomAngularVarianceDegPerSec);
	const float RollSpin = (DieSign * 145.0f) + FMath::FRandRange(-RandomAngularVarianceDegPerSec, RandomAngularVarianceDegPerSec);
	const float PitchSpin = BaseAngularTumbleDegPerSec + FMath::FRandRange(-RandomAngularVarianceDegPerSec, RandomAngularVarianceDegPerSec);
	return FVector(RollSpin, PitchSpin, YawSpin);
}

// ============================================================================
// F2: Parabolic Lob
// ============================================================================
UThrowStyle_ParabolicLob::UThrowStyle_ParabolicLob()
{
	Archetype = EThrowArchetype::ParabolicLob;
	DisplayName = FText::FromString(TEXT("F2: Parabolic Lob (Helicopter Soft Toss)"));
	// At 36 deg pitch, v=1100 lands at X=1380cm and bounces cleanly into the 1538cm alligator wall!
	MinLaunchSpeedCmPerSec = 780.0f;
	MaxLaunchSpeedCmPerSec = 1380.0f;
	LaunchPitchDegrees = 36.0f;
	BaseAngularTumbleDegPerSec = 440.0f;
	RandomAngularVarianceDegPerSec = 125.0f;
}

FVector UThrowStyle_ParabolicLob::ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const
{
	const float HeliSpinZ = (DieIndex == 0 ? 520.0f : -520.0f);
	const float Variance = LaunchParams.bSweetSpotRelease ? (RandomAngularVarianceDegPerSec * 0.35f) : RandomAngularVarianceDegPerSec;
	return FVector(
		(DieIndex == 0 ? 210.0f : -190.0f) + FMath::FRandRange(-Variance, Variance),
		-BaseAngularTumbleDegPerSec + FMath::FRandRange(-Variance, Variance),
		HeliSpinZ + FMath::FRandRange(-Variance, Variance)
	);
}

// ============================================================================
// F3: Back-Wall Whip
// ============================================================================
UThrowStyle_BackWallWhip::UThrowStyle_BackWallWhip()
{
	Archetype = EThrowArchetype::BackWallWhip;
	DisplayName = FText::FromString(TEXT("F3: Back-Wall Whip (Fastball Line Drive)"));
	MinLaunchSpeedCmPerSec = 1650.0f;
	MaxLaunchSpeedCmPerSec = 3100.0f;
	LaunchPitchDegrees = 6.5f; // Low fastball straight into the middle alligator pyramid row
	BaseAngularTumbleDegPerSec = 760.0f;
	RandomAngularVarianceDegPerSec = 240.0f;
}

// ============================================================================
// F4: Mechanical Plunger
// ============================================================================
UThrowStyle_MechanicalPlunger::UThrowStyle_MechanicalPlunger()
{
	Archetype = EThrowArchetype::MechanicalPlunger;
	DisplayName = FText::FromString(TEXT("F4: Mechanical Plunger (Cybernetic Spring)"));
	MinLaunchSpeedCmPerSec = 1200.0f;
	MaxLaunchSpeedCmPerSec = 2500.0f;
	LaunchPitchDegrees = 4.0f;
	BaseAngularTumbleDegPerSec = 480.0f;
	RandomAngularVarianceDegPerSec = 0.0f; // 100% deterministic repeatable release
}

FVector UThrowStyle_MechanicalPlunger::ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const
{
	return FVector(110.0f, BaseAngularTumbleDegPerSec, 0.0f);
}


