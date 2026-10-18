// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/CrapsTypes.h"
#include "DiceRiggingComponent.generated.h"

class ADieBase;

/**
 * Hybrid "Torque Magnet" rigging component (Task 1.2).
 * Applies deterministic angular torque (AddTorqueInRadians) as linear velocity drops below
 * SettlingVelocityThreshold, nudging the die toward DesiredTopFace or away from a fatal 7-Out.
 */
UCLASS(ClassGroup = (Craps), meta = (BlueprintSpawnableComponent))
class CRAPSCRAPSCRAPS_API UDiceRiggingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDiceRiggingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Master toggle enabling or disabling physics torque rigging assistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	bool bRiggingEnabled = true;

	/**
	 * Player's Dice Control Stat [0.0 = Pure Chaos RNG, 1.0 = Full Deterministic Torque Lock].
	 * Controlled by the Testing Pit slider or character/trinket progression.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiceControlStat = 0.35f;

	/** Rigging mode: torque toward pre-set top face OR torque away from faces that sum to 7. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	EDiceRiggingMode RiggingMode = EDiceRiggingMode::TargetSetFace;

	/**
	 * Linear velocity threshold (cm/s at 10x scale) below which the Torque Magnet engages.
	 * Default 650 cm/s corresponds to final settling bounces on the felt.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	float SettlingVelocityThreshold = 650.0f;

	/** Minimum linear velocity (cm/s) below which torque stops so resting dice never jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	float MinActiveVelocityThreshold = 15.0f;

	/** Maximum angular acceleration torque (radians/sec^2) applied at 100% DiceControlStat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	float MaxTorqueRadiansPerSecSq = 55.0f;

	/** Angular damping assist factor applied while rigging is active to stabilize wobbling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	float AngularStabilizationDamping = 4.5f;

	/** Maximum angular speed (rad/s) allowed before torque magnet ceases rotational acceleration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	float MaxAngularVelocityRadPerSec = 12.0f;

	/** The face direction the player set on top during On-Felt Setting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	EDieFaceDirection DesiredTopFace = EDieFaceDirection::PlusZ;

	/** Optional forbidden face directions (e.g., the complement face that would make a 7-Out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Rigging")
	TSet<EDieFaceDirection> ForbiddenTopFaces;

	/** Reference to the partner die on the table so AvoidSevenOut mode can dynamically read its top face. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Craps|Rigging")
	TObjectPtr<ADieBase> PartnerDie = nullptr;

	/** Captures the owning die's current top face as DesiredTopFace right before launch. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Rigging")
	void CapturePreThrowDesiredFace();

	/** Returns true if the Torque Magnet is actively applying torque on the current frame. */
	UFUNCTION(BlueprintPure, Category = "Craps|Rigging")
	bool IsActivelyRigging() const { return bIsActivelyRigging; }

private:
	bool bIsActivelyRigging = false;
};
