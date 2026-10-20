// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Table/CrapsFlyingChipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
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

	UStaticMesh* SelectedMesh = nullptr;
	if (ChipMeshFinder.Succeeded() && ChipMeshFinder.Object)
	{
		SelectedMesh = ChipMeshFinder.Object;
	}
	else if (FallbackMeshFinder.Succeeded() && FallbackMeshFinder.Object)
	{
		SelectedMesh = FallbackMeshFinder.Object;
	}

	if (SelectedMesh)
	{
		ChipMeshComponent->SetStaticMesh(SelectedMesh);
		const FBoxSphereBounds Bounds = SelectedMesh->GetBounds();
		const float DesiredRadius = 18.0f; // 36cm diameter (10x casino scale)
		const float ScaleXY = (Bounds.BoxExtent.X > KINDA_SMALL_NUMBER) ? (DesiredRadius / Bounds.BoxExtent.X) : 1.0f;
		const float DesiredHalfHeight = 1.7f; // 3.4cm thickness
		const float ScaleZ = (Bounds.BoxExtent.Z > KINDA_SMALL_NUMBER) ? (DesiredHalfHeight / Bounds.BoxExtent.Z) : ScaleXY;
		ChipMeshComponent->SetRelativeScale3D(FVector(ScaleXY, ScaleXY, ScaleZ));
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
	TotalDuration = FMath::Max(0.2f, InDuration);
	ElapsedTime = 0.0f;
	bIsFlying = true;

	SetActorLocation(StartLocation);

	// Gentle random tumble spin
	TumbleAngularVelocity = FVector(
		FMath::FRandRange(-540.0f, 540.0f),
		FMath::FRandRange(-540.0f, 540.0f),
		FMath::FRandRange(-360.0f, 360.0f));

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		const int32 NumSlots = FMath::Max(1, ChipMeshComponent->GetNumMaterials());
		for (int32 Slot = 0; Slot < NumSlots; ++Slot)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				FLinearColor SlotColor = InColor;
				if (Slot == 1)
				{
					// Inlay face: slightly brighter
					SlotColor = FMath::Lerp(InColor, FLinearColor::White, 0.4f);
				}
				MID->SetVectorParameterValue(TEXT("Color"), SlotColor);
				ChipMeshComponent->SetMaterial(Slot, MID);
			}
		}
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

	const int32 SafeCount = FMath::Clamp(ChipCount, 4, 24);
	for (int32 i = 0; i < SafeCount; ++i)
	{
		const FVector StaggeredOrigin = OriginPos + FVector(
			FMath::FRandRange(-50.0f, 50.0f),
			FMath::FRandRange(-50.0f, 50.0f),
			FMath::FRandRange(0.0f, 20.0f));

		const FVector StaggeredDest = DestinationPos + FVector(
			FMath::FRandRange(-20.0f, 20.0f),
			FMath::FRandRange(-20.0f, 20.0f),
			FMath::FRandRange(-10.0f, 10.0f));

		const float Arc = FMath::FRandRange(55.0f, 85.0f);
		const float Duration = FMath::FRandRange(0.60f, 0.85f);

		SpawnChip(World, StaggeredOrigin, StaggeredDest, Arc, Duration, Color);
	}
}
