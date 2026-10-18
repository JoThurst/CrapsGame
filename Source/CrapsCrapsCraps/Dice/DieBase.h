// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/CrapsTypes.h"
#include "DieBase.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPDA_DiceFace;
class UDiceRiggingComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnDieSettledSignature,
	ADieBase*, Die,
	EDieFaceDirection, TopDirection,
	int32, TopValue,
	UPDA_DiceFace*, TopFaceData
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDieImpactSignature,
	ADieBase*, Die,
	AActor*, OtherActor,
	float, ImpactSpeed
);

/**
 * 20cm x 20cm x 20cm (10x Scale) modular physical Craps die (Task 1.1).
 * Features 6 directional face sockets (+X, -X, +Y, -Y, +Z, -Z), CCD enabled by default,
 * Dot-Product top-face detection against FVector::UpVector, and UDiceRiggingComponent.
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ADieBase : public AActor
{
	GENERATED_BODY()

public:
	ADieBase();

	virtual void Tick(float DeltaTime) override;

	/** 10x Scale Die Constant: 20cm side length (10cm half-extent). */
	static constexpr float DieSideLengthCm = 20.0f;
	static constexpr float DieHalfExtentCm = 10.0f;

	//~ Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Components")
	TObjectPtr<UStaticMeshComponent> DieMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Components")
	TObjectPtr<UDiceRiggingComponent> DiceRiggingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_PlusZ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_MinusZ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_PlusX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_MinusX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_PlusY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Sockets")
	TObjectPtr<USceneComponent> Socket_MinusY;

	//~ Data-Driven Faces & Default Fallbacks
	/** Optional data assets equipped on each of the 6 directional sockets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Faces")
	TMap<EDieFaceDirection, TObjectPtr<UPDA_DiceFace>> EquippedFaceAssets;

	/** Default integer pip values (opposite faces sum to 7: +Z=1, -Z=6, +X=2, -X=5, +Y=3, -Y=4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Faces")
	TMap<EDieFaceDirection, int32> DefaultFaceValues;

	//~ Settle Detection Tuning (10x Scale)
	/** Linear speed threshold (cm/s) below which the die is considered nearly stopped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics")
	float SettledLinearSpeedThreshold = 8.0f;

	/** Angular speed threshold (deg/s) below which the die is considered nearly stopped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics")
	float SettledAngularSpeedThreshold = 12.0f;

	/** Time in seconds the die must stay below velocity thresholds to broadcast OnDieSettled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics")
	float SettledHoldTimeRequired = 0.18f;

	/** Minimum Dot Product alignment with UpVector (0..1) before a settled die is flagged as "Cocked". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics")
	float CockedAlignmentThreshold = 0.92f;

	/** Maximum allowable aggregate COM offset (cm) to prevent unnatural or tumbling glitch artifacts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MaxAllowedCOMOffsetCm = 3.5f;

	/** Additional manual / debug COM offset in die local coordinates (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Physics")
	FVector AdditionalLocalCOMOffset = FVector::ZeroVector;

	/** Debug draw pip numbers and socket axes above the die in the Testing Pit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Die|Debug")
	bool bDrawDebugFaceOverlay = true;

	//~ Events
	UPROPERTY(BlueprintAssignable, Category = "Craps|Die|Events")
	FOnDieSettledSignature OnDieSettled;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Die|Events")
	FOnDieImpactSignature OnDieImpact;

	//~ Blueprint Visual/Audio Hooks
	UFUNCTION(BlueprintImplementableEvent, Category = "Craps|Die|Events")
	void BP_OnDieSettled(EDieFaceDirection TopDirection, int32 TopValue, UPDA_DiceFace* TopFaceData, bool bWasCocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "Craps|Die|Events")
	void BP_OnDieBounceImpact(const FVector& ImpactLocation, const FVector& ImpactNormal, float ImpactSpeed, AActor* HitActor);

	//~ Public API
	/** Returns the local unit normal vector for a given face direction. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	static FVector GetLocalNormalForDirection(EDieFaceDirection Direction);

	/** Returns the opposite face direction on a cube (+Z <-> -Z, etc.). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	static EDieFaceDirection GetOppositeDirection(EDieFaceDirection Direction);

	/** Returns the world-space normal vector for a given face direction. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	FVector GetWorldNormalForDirection(EDieFaceDirection Direction) const;

	/** Returns the scene component socket for a given face direction. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	USceneComponent* GetFaceSocket(EDieFaceDirection Direction) const;

	/**
	 * Determines the current top-facing direction via maximum Dot Product against FVector::UpVector (0, 0, 1).
	 * Outputs OutMaxDotProduct so callers can detect cocked dice.
	 */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	EDieFaceDirection GetTopFaceDirection(float& OutMaxDotProduct) const;

	/** Returns the integer value of a specific face direction (queries EquippedFaceAssets first, then DefaultFaceValues). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	int32 GetFaceValueForDirection(EDieFaceDirection Direction) const;

	/** Returns the integer value of the current top face. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	int32 GetTopFaceValue() const;

	/** Returns the UPDA_DiceFace equipped on the current top face (or nullptr if using default integer face). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	UPDA_DiceFace* GetTopFaceData() const;

	/** Finds which EDieFaceDirection holds the specified integer pip value (1-6). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	bool FindDirectionForFaceValue(int32 TargetValue, EDieFaceDirection& OutDirection) const;

	/** Computes aggregate local-space Center of Mass offset (cm) from all currently equipped faces + AdditionalLocalCOMOffset. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die|Physics")
	FVector CalculateAggregateCenterOfMassOffset() const;

	/** Refreshes and applies the aggregate center-of-mass offset to DieMesh's BodyInstance in Chaos physics. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die|Physics")
	void UpdateMassAndCenterOfMass();

	/** Equips a UPDA_DiceFace onto the specified directional socket and updates physical mass properties. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die|Faces")
	void EquipFaceAsset(EDieFaceDirection Direction, UPDA_DiceFace* FaceAsset);

	/** Removes the equipped UPDA_DiceFace from the specified directional socket and restores default mass properties. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die|Faces")
	void UnequipFaceAsset(EDieFaceDirection Direction);

	/** Clears all equipped face assets, restoring pure casino standard dice behavior. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die|Faces")
	void ClearAllEquippedFaces();

	/** Returns the current physical Center of Mass offset in local actor space (cm). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die|Physics")
	FVector GetLocalCenterOfMass() const;

	/** Computes the world rotation that places DesiredTopFace pointing up (+Z) and DesiredFrontFace pointing forward (+X). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	static FQuat MakeDieOrientation(EDieFaceDirection DesiredTopFace, EDieFaceDirection DesiredFrontFace, float TableYawDegrees = 0.0f);

	/** Prepares the die for On-Felt Setting (disables physics simulation, zeroes velocity, clears settle state). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die")
	void EnterOnFeltSettingState();

	/** Launches the die with physical linear velocity (cm/s) and angular velocity (deg/s). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Die")
	void LaunchDie(const FVector& LinearVelocityCmPerSec, const FVector& AngularVelocityDegPerSec);

	/** Returns true if the die is currently in flight / rolling and has not yet settled. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	bool IsRolling() const { return bIsRolling; }

	/** Returns true if the die has finished rolling and settled. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	bool HasSettled() const { return bHasSettled; }

	/** Returns true if the die is currently cocked (leaning against an obstacle or wall with top alignment < CockedAlignmentThreshold). */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	bool IsCocked() const { return bIsCocked; }

	/** Returns the number of physical hazard/wall bounces accumulated during the current throw. */
	UFUNCTION(BlueprintPure, Category = "Craps|Die")
	int32 GetCurrentThrowBounceCount() const { return CurrentThrowBounceCount; }

	/** True if the die settled in a cocked position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Die|Physics")
	bool bIsCocked = false;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleComponentHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

private:
	void DrawDebugDieFaces() const;

	bool bIsRolling = false;
	bool bHasSettled = false;
	float SettledTimer = 0.0f;
	int32 CurrentThrowBounceCount = 0;
	double LastBounceTime = 0.0;
};
