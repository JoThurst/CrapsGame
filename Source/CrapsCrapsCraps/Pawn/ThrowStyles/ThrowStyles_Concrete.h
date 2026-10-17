// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Pawn/ThrowStyles/ThrowStyleInterface.h"
#include "ThrowStyles_Concrete.generated.h"

/**
 * F1: The Table-Skimmer (Shuffleboard/Curling Style).
 * Dice never leave the cloth: zero vertical launch pitch, high horizontal momentum,
 * ultra-low pitch/roll tumble to preserve top-face stability while plowing through stacks.
 */
UCLASS(ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UThrowStyle_TableSkimmer : public UThrowStyleComponentBase
{
	GENERATED_BODY()

public:
	UThrowStyle_TableSkimmer();

protected:
	virtual FVector ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const override;
};

/**
 * F2: The Parabolic Lob (Helicopter/Soft Toss).
 * High trajectory arc (48 deg) over front-table hazards directly onto target zones or bumpers,
 * with helicopter Z-axis spin for soft-landing stabilization.
 */
UCLASS(ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UThrowStyle_ParabolicLob : public UThrowStyleComponentBase
{
	GENERATED_BODY()

public:
	UThrowStyle_ParabolicLob();

protected:
	virtual FVector ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const override;
};

/**
 * F3: The Back-Wall Whip (Fastball Flick).
 * High-velocity line drive straight into the back alligator wall for maximum bounce/collision
 * multipliers and explosive scatter.
 */
UCLASS(ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UThrowStyle_BackWallWhip : public UThrowStyleComponentBase
{
	GENERATED_BODY()

public:
	UThrowStyle_BackWallWhip();
};

/**
 * F4: The Mechanical Plunger (Cybernetic Spring Hand).
 * Pull-back spring plunger resting on the felt behind the dice.
 * Delivers 100% repeatable velocity and straight-line trajectory with zero random angular variance.
 */
UCLASS(ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UThrowStyle_MechanicalPlunger : public UThrowStyleComponentBase
{
	GENERATED_BODY()

public:
	UThrowStyle_MechanicalPlunger();

protected:
	virtual FVector ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const override;
};
