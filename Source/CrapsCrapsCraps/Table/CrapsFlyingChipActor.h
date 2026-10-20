// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrapsFlyingChipActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 3D Visual flying casino chip for tactile chip tossing and payout cascades.
 * Interpolates along a smooth parabolic arc with tumbling rotation.
 */
UCLASS()
class CRAPSCRAPSCRAPS_API ACrapsFlyingChipActor : public AActor
{
	GENERATED_BODY()

public:
	ACrapsFlyingChipActor();

	virtual void Tick(float DeltaTime) override;

	/** Initializes flight trajectory and appearance. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Chips")
	void Launch(
		const FVector& InStartPos,
		const FVector& InTargetPos,
		float InArcHeight = 45.0f,
		float InDuration = 0.42f,
		const FLinearColor& InColor = FLinearColor(0.0f, 0.96f, 0.83f));

	/** Spawns a single flying chip from Start to Target. */
	static ACrapsFlyingChipActor* SpawnChip(
		UWorld* World,
		const FVector& StartPos,
		const FVector& TargetPos,
		float ArcHeight = 45.0f,
		float Duration = 0.42f,
		const FLinearColor& Color = FLinearColor(0.0f, 0.96f, 0.83f));

	/** Spawns a fountain cascade of glowing chips flying from table toward player POV on banking. */
	static void SpawnPayoutCascade(
		UWorld* World,
		const FVector& OriginPos,
		const FVector& DestinationPos,
		int32 ChipCount = 8,
		const FLinearColor& Color = FLinearColor(1.0f, 0.72f, 0.01f));

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChipMeshComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ChipMID = nullptr;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector TumbleAngularVelocity = FVector::ZeroVector;
	float ArcHeight = 45.0f;
	float TotalDuration = 0.42f;
	float ElapsedTime = 0.0f;
	bool bIsFlying = false;
};
