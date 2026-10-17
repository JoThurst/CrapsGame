// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dice/DiceRiggingComponent.h"
#include "Dice/DieBase.h"
#include "Components/StaticMeshComponent.h"

UDiceRiggingComponent::UDiceRiggingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UDiceRiggingComponent::CapturePreThrowDesiredFace()
{
	if (const ADieBase* OwnerDie = Cast<ADieBase>(GetOwner()))
	{
		float DummyDot = 1.0f;
		DesiredTopFace = OwnerDie->GetTopFaceDirection(DummyDot);
	}
}

void UDiceRiggingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bIsActivelyRigging = false;

	if (DiceControlStat <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ADieBase* OwnerDie = Cast<ADieBase>(GetOwner());
	if (!OwnerDie || !OwnerDie->IsRolling() || OwnerDie->HasSettled() || !OwnerDie->DieMesh)
	{
		return;
	}

	UStaticMeshComponent* Mesh = OwnerDie->DieMesh;
	if (!Mesh->IsSimulatingPhysics())
	{
		return;
	}

	const FVector LinearVel = Mesh->GetPhysicsLinearVelocity();
	const float Speed = LinearVel.Size();

	// Only engage the Torque Magnet inside the settling velocity window [MinActiveVelocityThreshold .. SettlingVelocityThreshold]
	if (Speed < MinActiveVelocityThreshold || Speed > SettlingVelocityThreshold)
	{
		return;
	}

	bIsActivelyRigging = true;

	// Normalized factor peaking in the middle of the settling window
	const float SettlingAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(MinActiveVelocityThreshold, SettlingVelocityThreshold),
		FVector2D(1.0f, 0.25f),
		Speed
	);

	const float ControlStrength = FMath::Clamp(DiceControlStat, 0.0f, 1.0f) * SettlingAlpha;

	if (RiggingMode == EDiceRiggingMode::TargetSetFace)
	{
		// Compute torque axis that rotates DesiredTopFace's world normal toward FVector::UpVector (0, 0, 1)
		const FVector CurrentFaceNormal = OwnerDie->GetWorldNormalForDirection(DesiredTopFace);
		const FVector TorqueAxis = FVector::CrossProduct(CurrentFaceNormal, FVector::UpVector);
		const float SinAngle = TorqueAxis.Size();

		if (SinAngle > 0.01f)
		{
			const FVector NormalizedAxis = TorqueAxis / SinAngle;
			const float DotAlignment = FVector::DotProduct(CurrentFaceNormal, FVector::UpVector);
			const float AngleErrorRadians = FMath::Acos(FMath::Clamp(DotAlignment, -1.0f, 1.0f));

			// Proportional-Derivative (PD) angular controller so the die locks onto the target face without overshooting
			const FVector CurrentAngVelRad = Mesh->GetPhysicsAngularVelocityInRadians();
			const FVector DesiredTorque =
				(NormalizedAxis * (AngleErrorRadians * MaxTorqueRadiansPerSecSq * ControlStrength))
				- (CurrentAngVelRad * (AngularStabilizationDamping * ControlStrength));

			Mesh->AddTorqueInRadians(DesiredTorque, NAME_None, true);
		}
	}
	else if (RiggingMode == EDiceRiggingMode::AvoidSevenOut)
	{
		// Dynamically determine which face on this die would sum to 7 with PartnerDie's top face.
		// Only consider PartnerDie if it has settled or decelerated into late settling to avoid frantic jitter.
		TSet<EDieFaceDirection> ActiveForbiddenFaces = ForbiddenTopFaces;
		if (PartnerDie != nullptr)
		{
			const bool bPartnerStable = PartnerDie->HasSettled() ||
				(PartnerDie->DieMesh && PartnerDie->DieMesh->GetPhysicsLinearVelocity().Size() < SettlingVelocityThreshold * 0.75f);
			if (bPartnerStable)
			{
				const int32 PartnerVal = PartnerDie->GetTopFaceValue();
				const int32 ComplementForSeven = 7 - PartnerVal;
				EDieFaceDirection SevenDirection;
				if (OwnerDie->FindDirectionForFaceValue(ComplementForSeven, SevenDirection))
				{
					ActiveForbiddenFaces.Add(SevenDirection);
				}
			}
		}

		const FVector CurrentAngVelRad = Mesh->GetPhysicsAngularVelocityInRadians();
		const float CurrentAngSpeed = CurrentAngVelRad.Size();

		for (EDieFaceDirection ForbiddenDir : ActiveForbiddenFaces)
		{
			const FVector ForbiddenWorldNormal = OwnerDie->GetWorldNormalForDirection(ForbiddenDir);
			const float DotToUp = FVector::DotProduct(ForbiddenWorldNormal, FVector::UpVector);

			// If the forbidden face is rotating into the top danger cone (Dot > 0.55), apply smooth repulsive roll torque
			if (DotToUp > 0.55f)
			{
				FVector RepulseAxis = FVector::CrossProduct(FVector::UpVector, ForbiddenWorldNormal);
				if (RepulseAxis.SizeSquared() < 0.01f)
				{
					// Directly vertical: tip along travel direction or RightVector
					RepulseAxis = FVector::CrossProduct(LinearVel.GetSafeNormal(0.1f, FVector::ForwardVector), FVector::UpVector).GetSafeNormal();
					if (RepulseAxis.IsNearlyZero())
					{
						RepulseAxis = FVector::RightVector;
					}
				}
				else
				{
					RepulseAxis.Normalize();
				}

				// Smooth cosine/cubic curve to avoid sudden unnatural physics jerks
				const float NormalizedDanger = FMath::Clamp((DotToUp - 0.55f) / 0.45f, 0.0f, 1.0f);
				const float DangerIntensity = FMath::SmoothStep(0.0f, 1.0f, NormalizedDanger) * 1.15f;

				// Attenuate extra rotational acceleration as angular speed approaches MaxAngularVelocityRadPerSec
				const float SpinAttenuation = FMath::Clamp(1.0f - (CurrentAngSpeed / FMath::Max(1.0f, MaxAngularVelocityRadPerSec)), 0.0f, 1.0f);

				// Proportional-Derivative (PD) controller: repulsive roll nudge + stabilization damping
				const FVector RepulseTorque = (RepulseAxis * (MaxTorqueRadiansPerSecSq * ControlStrength * DangerIntensity * SpinAttenuation))
					- (CurrentAngVelRad * (AngularStabilizationDamping * ControlStrength * 0.6f));

				Mesh->AddTorqueInRadians(RepulseTorque, NAME_None, true);
			}
		}
	}
}
