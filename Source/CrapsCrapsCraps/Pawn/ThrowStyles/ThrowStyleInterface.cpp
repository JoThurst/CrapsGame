#include "Pawn/ThrowStyles/ThrowStyleInterface.h"
#include "Dice/DieBase.h"
#include "Engine/World.h"

UThrowStyleComponentBase::UThrowStyleComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
	DisplayName = FText::FromString(TEXT("Base Throw Style"));
}

FVector UThrowStyleComponentBase::ComputeInitialVelocity(const FThrowLaunchParams& LaunchParams) const
{
	const float ClampedPower = FMath::Clamp(LaunchParams.NormalizedPower, 0.12f, 1.0f);
	const float Speed = FMath::Lerp(MinLaunchSpeedCmPerSec, MaxLaunchSpeedCmPerSec, ClampedPower);

	const FRotator LaunchRot(LaunchPitchDegrees, LaunchParams.BaseYawDegrees + LaunchParams.AimYawOffsetDegrees, 0.0f);
	return LaunchRot.Vector() * Speed;
}

FVector UThrowStyleComponentBase::ComputeInitialAngularVelocity(const FThrowLaunchParams& LaunchParams, int32 DieIndex) const
{
	const float SweetSpotReduction = LaunchParams.bSweetSpotRelease ? 0.40f : 1.0f;
	const float Variance = RandomAngularVarianceDegPerSec * SweetSpotReduction;

	const FVector RandomNoise(
		FMath::FRandRange(-Variance, Variance),
		FMath::FRandRange(-Variance, Variance),
		FMath::FRandRange(-Variance, Variance)
	);

	// Real 3D craps tumble: strong pitch roll around Y, plus edge-catching roll (X) and yaw spin (Z)
	const float DieAsymmetry = (DieIndex == 0) ? 1.0f : -0.85f;
	const FVector BaseSpin(
		(BaseAngularTumbleDegPerSec * 0.42f * DieAsymmetry) + (LaunchParams.LateralSpin * 160.0f),
		BaseAngularTumbleDegPerSec,
		(BaseAngularTumbleDegPerSec * 0.28f * -DieAsymmetry) + (LaunchParams.LateralSpin * 220.0f)
	);

	return BaseSpin + RandomNoise;
}

void UThrowStyleComponentBase::ExecuteThrow_Implementation(const FThrowLaunchParams& LaunchParams, const TArray<ADieBase*>& Dice)
{
	const FVector BaseVel = ComputeInitialVelocity(LaunchParams);

	for (int32 Index = 0; Index < Dice.Num(); ++Index)
	{
		if (ADieBase* Die = Dice[Index])
		{
			// Lift dice slightly (+3cm) off the staging felt at the moment of hand release so corners can rotate freely
			FVector ReleaseLoc = Die->GetActorLocation();
			ReleaseLoc.Z += 3.0f;
			Die->SetActorLocation(ReleaseLoc, false, nullptr, ETeleportType::TeleportPhysics);

			// Slight convergence/divergence between the two fingers so the dice can kiss/collide mid-roll
			const float SpreadSign = (Index == 0) ? 1.0f : -1.0f;
			const float SpeedJitter = (RandomAngularVarianceDegPerSec > 0.1f) ? FMath::FRandRange(0.96f, 1.04f) : 1.0f;
			const FVector LateralNudge(
				0.0f,
				SpreadSign * FMath::FRandRange(-45.0f, 85.0f) * (RandomAngularVarianceDegPerSec > 0.1f ? 1.0f : 0.0f),
				FMath::FRandRange(0.0f, 65.0f) * (RandomAngularVarianceDegPerSec > 0.1f ? 1.0f : 0.0f)
			);

			const FRotator StationYaw(0.0f, LaunchParams.BaseYawDegrees, 0.0f);
			const FVector FinalLinearVel = (BaseVel * SpeedJitter) + StationYaw.RotateVector(LateralNudge);
			const FVector FinalAngVel = StationYaw.RotateVector(ComputeInitialAngularVelocity(LaunchParams, Index));
			Die->LaunchDie(FinalLinearVel, FinalAngVel);
		}
	}
}

void UThrowStyleComponentBase::ComputePreviewTrajectory_Implementation(
	const FVector& StartLocation,
	const FThrowLaunchParams& LaunchParams,
	TArray<FVector>& OutTrajectoryPoints,
	FVector& OutPredictedLandingSpot
) const
{
	OutTrajectoryPoints.Reset();
	const FVector InitialVel = ComputeInitialVelocity(LaunchParams);
	const FVector Gravity(0.0f, 0.0f, -980.0f);
	const float FeltHeightZ = StartLocation.Z; // Center of 20cm die resting on the felt

	constexpr int32 NumSteps = 32;
	constexpr float StepDt = 0.032f;

	FVector PrevPos = StartLocation;
	OutTrajectoryPoints.Add(PrevPos);
	OutPredictedLandingSpot = StartLocation;

	for (int32 Step = 1; Step <= NumSteps; ++Step)
	{
		const float Time = Step * StepDt;
		FVector Pos;

		if (LaunchPitchDegrees <= 3.0f)
		{
			// Low-skim glide preview with felt deceleration
			const float FrictionDecel = FMath::Max(0.20f, 1.0f - (Time * 0.38f));
			Pos = StartLocation + (InitialVel * Time * FrictionDecel);
			Pos.Z = FeltHeightZ;
		}
		else
		{
			// Ballistic arc
			Pos = StartLocation + (InitialVel * Time) + (0.5f * Gravity * Time * Time);
			if (Pos.Z <= FeltHeightZ)
			{
				Pos.Z = FeltHeightZ;
				OutTrajectoryPoints.Add(Pos);
				OutPredictedLandingSpot = Pos;
				return;
			}
		}

		OutTrajectoryPoints.Add(Pos);
		OutPredictedLandingSpot = Pos;
	}
}

