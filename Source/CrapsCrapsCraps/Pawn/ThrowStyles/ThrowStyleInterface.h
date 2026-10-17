// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Components/ActorComponent.h"
#include "Core/CrapsTypes.h"
#include "ThrowStyleInterface.generated.h"

class ADieBase;

UINTERFACE(MinimalAPI, BlueprintType)
class UThrowStyleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Modular interface for Right-Hand active throw archetypes (Task 1.4).
 * Hot-swappable at runtime in the Testing Pit via F1-F4.
 */
class CRAPSCRAPSCRAPS_API IThrowStyleInterface
{
	GENERATED_BODY()

public:
	/** Executes the physical throw on the pair of dice using the given launch parameters. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Craps|ThrowStyle")
	void ExecuteThrow(const FThrowLaunchParams& LaunchParams, const TArray<ADieBase*>& Dice);

	/** Computes trajectory preview points in world space for the HUD/felt aiming line. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Craps|ThrowStyle")
	void ComputePreviewTrajectory(
		const FVector& StartLocation,
		const FThrowLaunchParams& LaunchParams,
		TArray<FVector>& OutTrajectoryPoints,
		FVector& OutPredictedLandingSpot
	) const;

	/** Returns the enum identifier for this throw archetype. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Craps|ThrowStyle")
	EThrowArchetype GetThrowArchetype() const;

	/** Returns a human-readable display name for the Debug HUD. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Craps|ThrowStyle")
	FText GetThrowStyleDisplayName() const;
};

/**
 * Base component implementing IThrowStyleInterface so all 4 styles can live on ACrapsPawn.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UThrowStyleComponentBase : public UActorComponent, public IThrowStyleInterface
{
	GENERATED_BODY()

public:
	UThrowStyleComponentBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	EThrowArchetype Archetype = EThrowArchetype::TableSkimmer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	FText DisplayName;

	/** Minimum linear launch speed (cm/s at 10x scale) at 0% power. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	float MinLaunchSpeedCmPerSec = 1400.0f;

	/** Maximum linear launch speed (cm/s at 10x scale) at 100% power. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	float MaxLaunchSpeedCmPerSec = 3600.0f;

	/** Upward launch pitch angle in degrees (0 for Table-Skimmer, ~48 for Parabolic Lob, ~8 for Back-Wall Whip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	float LaunchPitchDegrees = 0.0f;

	/** Base angular tumble speed in deg/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	float BaseAngularTumbleDegPerSec = 180.0f;

	/** Random angular variance in deg/s (0.0 for Mechanical Plunger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|ThrowStyle")
	float RandomAngularVarianceDegPerSec = 45.0f;

	//~ Begin IThrowStyleInterface
	virtual void ExecuteThrow_Implementation(const FThrowLaunchParams& LaunchParams, const TArray<ADieBase*>& Dice) override;
	virtual void ComputePreviewTrajectory_Implementation(
		const FVector& StartLocation,
		const FThrowLaunchParams& LaunchParams,
		TArray<FVector>& OutTrajectoryPoints,
		FVector& OutPredictedLandingSpot
	) const override;
	virtual EThrowArchetype GetThrowArchetype_Implementation() const override { return Archetype; }
	virtual FText GetThrowStyleDisplayName_Implementation() const override { return DisplayName; }
	//~ End IThrowStyleInterface

protected:
	/** Calculates initial velocity vector (cm/s) for the current launch params. */
	virtual FVector ComputeInitialVelocity(const FThrowLaunchParams& LaunchParams) const;

	/** Calculates initial angular velocity (deg/s) for a specific die index (0=Left, 1=Right). */
	virtual FVector ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const;
};
