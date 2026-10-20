// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Table/CrapsFlyingChipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACrapsFlyingChipActor::ACrapsFlyingChipActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ChipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChipMesh"));
	SetRootComponent(ChipMeshComponent);
	ChipMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChipMeshComponent->SetCastShadow(false);

	// Load SM_CasinoChip_v1 if available, fallback to Engine Cylinder
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChipMeshFinder(
		TEXT("/Game/Meshes/SM_CasinoChip_v1.SM_CasinoChip_v1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FallbackMeshFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	if (ChipMeshFinder.Succeeded())
	{
		ChipMeshComponent->SetStaticMesh(ChipMeshFinder.Object);
		ChipMeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}
	else if (FallbackMeshFinder.Succeeded())
	{
		ChipMeshComponent->SetStaticMesh(FallbackMeshFinder.Object);
		ChipMeshComponent->SetRelativeScale3D(FVector(0.26f, 0.26f, 0.035f));
	}
}

void ACrapsFlyingChipActor::Launch(
	const FVector& InStartPos,
	const FVector& InTargetPos,
	float InArcHeight,
	float InDuration,
	const FLinearColor& InColor)
{
	StartLocation = InStartPos;
	TargetLocation = InTargetPos;
	ArcHeight = InArcHeight;
	TotalDuration = FMath::Max(0.1f, InDuration);
	ElapsedTime = 0.0f;
	bIsFlying = true;

	SetActorLocation(StartLocation);

	// Random tumble spin
	TumbleAngularVelocity = FVector(
		FMath::FRandRange(-720.0f, 720.0f),
		FMath::FRandRange(-720.0f, 720.0f),
		FMath::FRandRange(-360.0f, 360.0f));

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (!ChipMID)
		{
			ChipMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			ChipMeshComponent->SetMaterial(0, ChipMID);
		}
		ChipMID->SetVectorParameterValue(TEXT("Color"), InColor);
	}
}

void ACrapsFlyingChipActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsFlying)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(ElapsedTime / TotalDuration, 0.0f, 1.0f);

	// Quadratic Bezier arc
	const FVector MidPoint = 0.5f * (StartLocation + TargetLocation) + FVector(0.0f, 0.0f, ArcHeight);
	const FVector P01 = FMath::Lerp(StartLocation, MidPoint, Alpha);
	const FVector P12 = FMath::Lerp(MidPoint, TargetLocation, Alpha);
	const FVector CurrentPos = FMath::Lerp(P01, P12, Alpha);

	SetActorLocation(CurrentPos);
	AddActorLocalRotation(FRotator(
		TumbleAngularVelocity.Y * DeltaTime,
		TumbleAngularVelocity.Z * DeltaTime,
		TumbleAngularVelocity.X * DeltaTime));

	if (Alpha >= 1.0f)
	{
		bIsFlying = false;
		Destroy();
	}
}

ACrapsFlyingChipActor* ACrapsFlyingChipActor::SpawnChip(
	UWorld* World,
	const FVector& StartPos,
	const FVector& TargetPos,
	float ArcHeight,
	float Duration,
	const FLinearColor& Color)
{
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACrapsFlyingChipActor* Chip = World->SpawnActor<ACrapsFlyingChipActor>(
		ACrapsFlyingChipActor::StaticClass(), StartPos, FRotator::ZeroRotator, SpawnParams);

	if (Chip)
	{
		Chip->Launch(StartPos, TargetPos, ArcHeight, Duration, Color);
	}
	return Chip;
}

void ACrapsFlyingChipActor::SpawnPayoutCascade(
	UWorld* World,
	const FVector& OriginPos,
	const FVector& DestinationPos,
	int32 ChipCount,
	const FLinearColor& Color)
{
	if (!World)
	{
		return;
	}

	const int32 SafeCount = FMath::Clamp(ChipCount, 3, 20);
	for (int32 i = 0; i < SafeCount; ++i)
	{
		const FVector StaggeredOrigin = OriginPos + FVector(
			FMath::FRandRange(-40.0f, 40.0f),
			FMath::FRandRange(-40.0f, 40.0f),
			FMath::FRandRange(0.0f, 15.0f));

		const FVector StaggeredDest = DestinationPos + FVector(
			FMath::FRandRange(-25.0f, 25.0f),
			FMath::FRandRange(-25.0f, 25.0f),
			FMath::FRandRange(-10.0f, 10.0f));

		const float Arc = FMath::FRandRange(35.0f, 65.0f);
		const float Duration = FMath::FRandRange(0.35f, 0.58f);

		SpawnChip(World, StaggeredOrigin, StaggeredDest, Arc, Duration, Color);
	}
}
