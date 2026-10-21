// Copyright CrapsCrapsCraps. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrapsFlyingChipActor.generated.h"

class UStaticMeshComponent;

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

	/** Returns standard casino color palette (Base, Inlay, Stripes) for a given denomination. */
	UFUNCTION(BlueprintPure, Category = "Craps|Chips")
	static void GetDenominationColors(int64 Denomination, FLinearColor& OutBase, FLinearColor& OutInlay, FLinearColor& OutStripe);

	/** Initializes flight trajectory and appearance. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Chips")
	void Launch(
		const FVector& InStartPos,
		const FVector& InTargetPos,
		float InArcHeight = 60.0f,
		float InDuration = 0.58f,
		const FLinearColor& InColor = FLinearColor(0.0f, 0.96f, 0.83f),
		int64 InDenomination = 0);

	/** Spawns a single flying chip from Start to Target. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Chips", meta = (WorldContext = "World"))
	static ACrapsFlyingChipActor* SpawnChip(
		UWorld* World,
		const FVector& StartPos,
		const FVector& TargetPos,
		float ArcHeight = 60.0f,
		float Duration = 0.58f,
		const FLinearColor& Color = FLinearColor(0.0f, 0.96f, 0.83f),
		int64 Denomination = 0);

	/** Spawns a fountain cascade of glowing chips flying from table toward player POV on banking. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Chips", meta = (WorldContext = "World"))
	static void SpawnPayoutCascade(
		UWorld* World,
		const FVector& OriginPos,
		const FVector& DestinationPos,
		int32 ChipCount = 10,
		const FLinearColor& Color = FLinearColor(1.0f, 0.72f, 0.01f));

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChipMeshComponent = nullptr;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector TumbleAngularVelocity = FVector::ZeroVector;
	float ArcHeight = 60.0f;
	float TotalDuration = 0.58f;
	float ElapsedTime = 0.0f;
	bool bIsFlying = false;
};
