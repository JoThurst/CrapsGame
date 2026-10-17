// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/CrapsTypes.h"
#include "TableHazard.generated.h"

class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Base class for physical Craps table hazards and targets (Task 2.3).
 * Injects real-time mid-roll FScoringStep entries into UScoringQueueSubsystem when struck by dice.
 */
UCLASS(Abstract, Blueprintable)
class CRAPSCRAPSCRAPS_API ATableHazard : public AActor
{
	GENERATED_BODY()

public:
	ATableHazard();

	virtual void Tick(float DeltaTime) override;

	/** Resets per-throw trigger cooldowns or re-stacks scattered chips when a new throw starts. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Hazard")
	virtual void ResetForNewThrow();

protected:
	virtual void BeginPlay() override;

	/** Helper to push a live mid-roll scoring step into UScoringQueueSubsystem. */
	void PushMidRollScoringStep(
		const FString& Label,
		EScoringLayer Layer,
		int64 ChipsDelta,
		float ThrowMultDelta,
		const FVector& WorldLoc);

	/** Visual pulse countdown after being triggered by a die. */
	float HitFlashTimer = 0.0f;
};

/**
 * Physical Pinball Bumper planted on the Craps felt.
 * Imparts a radial velocity kick when struck by a die and awards +35 Base Chips & +0.5x Throw Mult.
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API APinballBumperHazard : public ATableHazard
{
	GENERATED_BODY()

public:
	APinballBumperHazard();

	virtual void Tick(float DeltaTime) override;
	virtual void ResetForNewThrow() override;

	/** Returns how many times the specified die has triggered this bumper during the current throw. */
	UFUNCTION(BlueprintPure, Category = "Craps|Hazard")
	int32 GetDieHitCount(AActor* DieActor) const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBumperHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BumperCylinderMesh = nullptr;

	/** Radial repulse speed imparted to any die that strikes the bumper. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	float BumperKickStrength = 650.0f;

	/** Base Chips awarded per bumper strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	int64 BumperChipBonus = 35;

	/** Layer 2 Throw Multiplier added per bumper strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	float BumperThrowMultBonus = 0.5f;

	/** Maximum times a single die can trigger this bumper in a single throw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	int32 MaxHitsPerDiePerThrow = 2;

	/** Minimum cooldown seconds before the same die can retrigger this bumper. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	float RetriggerCooldownSeconds = 0.45f;

	/** Minimum impact speed (cm/s) required to trigger a bounce and bonus (prevents resting contact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	float MinTriggerImpactSpeed = 120.0f;

private:
	struct FDieBumperHitRecord
	{
		int32 HitCount = 0;
		double LastHitTime = -10.0;
	};

	TMap<TWeakObjectPtr<AActor>, FDieBumperHitRecord> PerDieHitRecords;
};

/**
 * Laser Tripwire Gate spanning across the Craps pit.
 * Detects dice slicing through its beam and awards +1.0x Throw Mult (Magenta).
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ALaserGateHazard : public ATableHazard
{
	GENERATED_BODY()

public:
	ALaserGateHazard();

	virtual void Tick(float DeltaTime) override;
	virtual void ResetForNewThrow() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeamOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> LaserBeamTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftPylonMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightPylonMesh = nullptr;

	/** Layer 2 Throw Multiplier added when a die slices through the laser gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Hazard")
	float LaserThrowMultBonus = 1.0f;

private:
	UPROPERTY()
	TSet<TObjectPtr<AActor>> TriggeredDiceThisThrow;
};

/**
 * Scatterable 3D Physics Chip Stack sitting on the Craps felt.
 * Uses bUseCCD = true on each chip cylinder so high-speed 20cm dice scatter them physically across the pit,
 * awarding +20 Base Chips + +0.25x Throw Mult when bowled over!
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API AChipStackHazard : public ATableHazard
{
	GENERATED_BODY()

public:
	AChipStackHazard();

	virtual void Tick(float DeltaTime) override;
	virtual void ResetForNewThrow() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleChipHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	/** Individual physics-simulated chip meshes in the stack (all with bUseCCD = true). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> ChipMeshes;

	/** Relative resting transforms for re-stacking the chips between throws. */
	TArray<FTransform> InitialChipTransforms;

	bool bScatteredThisThrow = false;
};
