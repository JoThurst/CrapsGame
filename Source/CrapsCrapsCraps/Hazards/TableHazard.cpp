// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Hazards/TableHazard.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dice/DieBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATableHazard::ATableHazard()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATableHazard::BeginPlay()
{
	Super::BeginPlay();
}

void ATableHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HitFlashTimer > 0.0f)
	{
		HitFlashTimer = FMath::Max(0.0f, HitFlashTimer - DeltaTime);
	}
}

void ATableHazard::ResetForNewThrow()
{
	HitFlashTimer = 0.0f;
}

void ATableHazard::PushMidRollScoringStep(
	const FString& Label,
	EScoringLayer Layer,
	int64 ChipsDelta,
	float ThrowMultDelta,
	const FVector& WorldLoc)
{
	HitFlashTimer = 0.85f;

	if (UWorld* World = GetWorld())
	{
		if (UScoringQueueSubsystem* ScoringQueue = World->GetSubsystem<UScoringQueueSubsystem>())
		{
			ScoringQueue->EnqueueMidRollCollisionStep(
				this,
				WorldLoc,
				Label,
				static_cast<int32>(ChipsDelta),
				ThrowMultDelta);
		}
	}
}

// ============================================================================
// APinballBumperHazard
// ============================================================================

APinballBumperHazard::APinballBumperHazard()
{
	BumperCylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BumperCylinderMesh"));
	SetRootComponent(BumperCylinderMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BumperCylinderMesh->SetStaticMesh(CylinderFinder.Object);
	}

	// 48cm wide, 28cm tall glowing synth-noir arcade bumper
	BumperCylinderMesh->SetRelativeScale3D(FVector(0.48f, 0.48f, 0.28f));
	BumperCylinderMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BumperCylinderMesh->SetNotifyRigidBodyCollision(true);
}

void APinballBumperHazard::BeginPlay()
{
	Super::BeginPlay();
	if (BumperCylinderMesh)
	{
		BumperCylinderMesh->OnComponentHit.AddDynamic(this, &APinballBumperHazard::HandleBumperHit);
	}
}

void APinballBumperHazard::ResetForNewThrow()
{
	Super::ResetForNewThrow();
	PerDieHitRecords.Empty();
}

int32 APinballBumperHazard::GetDieHitCount(AActor* DieActor) const
{
	if (!DieActor)
	{
		return 0;
	}
	if (const FDieBumperHitRecord* Found = PerDieHitRecords.Find(DieActor))
	{
		return Found->HitCount;
	}
	return 0;
}

void APinballBumperHazard::HandleBumperHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	ADieBase* HitDie = Cast<ADieBase>(OtherActor);
	if (!HitDie || !OtherComp || !OtherComp->IsSimulatingPhysics())
	{
		return;
	}

	// Reject resting or gentle rubbing against the bumper
	const float ImpactSpeed = OtherComp->GetPhysicsLinearVelocity().Size();
	if (ImpactSpeed < MinTriggerImpactSpeed)
	{
		return;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FDieBumperHitRecord& Record = PerDieHitRecords.FindOrAdd(HitDie);

	// Per-die, per-throw retrigger and count protection
	if (Record.HitCount >= MaxHitsPerDiePerThrow)
	{
		return;
	}
	if (Now - Record.LastHitTime < static_cast<double>(RetriggerCooldownSeconds))
	{
		return;
	}

	Record.HitCount++;
	Record.LastHitTime = Now;

	// Impart crisp radial pinball kick away from the bumper center
	FVector KickDir = (HitDie->GetActorLocation() - GetActorLocation());
	KickDir.Z = 0.22f;
	KickDir = KickDir.GetSafeNormal();
	OtherComp->AddImpulse(KickDir * BumperKickStrength, NAME_None, true);

	PushMidRollScoringStep(
		FString::Printf(TEXT("PINBALL BUMPER (+%lld CHIPS / +%.1fx MULT)"), BumperChipBonus, BumperThrowMultBonus),
		EScoringLayer::ThrowMult,
		BumperChipBonus,
		BumperThrowMultBonus,
		GetActorLocation() + FVector(0.0f, 0.0f, 35.0f));
}

void APinballBumperHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Loc = GetActorLocation();
	const bool bFlashing = (HitFlashTimer > 0.0f);
	const FColor RingColor = bFlashing ? FColor(255, 255, 255) : FColor(255, 42, 133); // Hot Magenta #FF2A85

	DrawDebugCylinder(
		World,
		Loc - FVector(0.0f, 0.0f, 12.0f),
		Loc + FVector(0.0f, 0.0f, 16.0f),
		bFlashing ? 30.0f : 25.0f,
		16,
		RingColor,
		false,
		-1.0f,
		0,
		bFlashing ? 4.0f : 2.2f);
}

// ============================================================================
// ALaserGateHazard
// ============================================================================

ALaserGateHazard::ALaserGateHazard()
{
	LaserBeamTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("LaserBeamTrigger"));
	SetRootComponent(LaserBeamTrigger);
	LaserBeamTrigger->SetBoxExtent(FVector(8.0f, 135.0f, 22.0f));
	LaserBeamTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LaserBeamTrigger->SetCollisionResponseToAllChannels(ECR_Overlap);
	LaserBeamTrigger->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	LeftPylonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPylonMesh"));
	LeftPylonMesh->SetupAttachment(LaserBeamTrigger);
	LeftPylonMesh->SetRelativeLocation(FVector(0.0f, -140.0f, 0.0f));
	LeftPylonMesh->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.42f));
	LeftPylonMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	RightPylonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPylonMesh"));
	RightPylonMesh->SetupAttachment(LaserBeamTrigger);
	RightPylonMesh->SetRelativeLocation(FVector(0.0f, 140.0f, 0.0f));
	RightPylonMesh->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.42f));
	RightPylonMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	if (CylinderFinder.Succeeded())
	{
		LeftPylonMesh->SetStaticMesh(CylinderFinder.Object);
		RightPylonMesh->SetStaticMesh(CylinderFinder.Object);
	}
}

void ALaserGateHazard::BeginPlay()
{
	Super::BeginPlay();
	if (LaserBeamTrigger)
	{
		LaserBeamTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALaserGateHazard::HandleBeamOverlap);
	}
}

void ALaserGateHazard::ResetForNewThrow()
{
	Super::ResetForNewThrow();
	TriggeredDiceThisThrow.Empty();
}

void ALaserGateHazard::HandleBeamOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ADieBase* Die = Cast<ADieBase>(OtherActor);
	if (!Die || TriggeredDiceThisThrow.Contains(Die))
	{
		return;
	}

	TriggeredDiceThisThrow.Add(Die);

	PushMidRollScoringStep(
		FString::Printf(TEXT("LASER GATE SLICED (+%.1fx THROW MULT)"), LaserThrowMultBonus),
		EScoringLayer::ThrowMult,
		25,
		LaserThrowMultBonus,
		Die->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f));
}

void ALaserGateHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector LeftPos = LeftPylonMesh ? LeftPylonMesh->GetComponentLocation() : GetActorLocation() - FVector(0.0f, 140.0f, 0.0f);
	const FVector RightPos = RightPylonMesh ? RightPylonMesh->GetComponentLocation() : GetActorLocation() + FVector(0.0f, 140.0f, 0.0f);
	const FColor BeamColor = (HitFlashTimer > 0.0f) ? FColor(0, 245, 212) : FColor(255, 42, 133);

	// Draw triple horizontal synth-laser scanlines between the pylons
	for (int32 BeamIdx = -1; BeamIdx <= 1; ++BeamIdx)
	{
		const FVector Offset(0.0f, 0.0f, BeamIdx * 8.0f + 4.0f);
		DrawDebugLine(World, LeftPos + Offset, RightPos + Offset, BeamColor, false, -1.0f, 0, (HitFlashTimer > 0.0f) ? 4.2f : 2.2f);
	}
}

// ============================================================================
// AChipStackHazard
// ============================================================================

AChipStackHazard::AChipStackHazard()
{
	USceneComponent* StackRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StackRoot"));
	SetRootComponent(StackRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	constexpr int32 NumChipsInStack = 6;
	for (int32 i = 0; i < NumChipsInStack; ++i)
	{
		const FName CompName(*FString::Printf(TEXT("PhysicsChip_%d"), i));
		UStaticMeshComponent* ChipComp = CreateDefaultSubobject<UStaticMeshComponent>(CompName);
		ChipComp->SetupAttachment(StackRoot);

		if (CylinderFinder.Succeeded())
		{
			ChipComp->SetStaticMesh(CylinderFinder.Object);
		}

		// 28cm diameter, 5cm thick heavy casino chip
		ChipComp->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.05f));
		ChipComp->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f + i * 5.4f));
		ChipComp->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		ChipComp->SetSimulatePhysics(true);
		ChipComp->BodyInstance.bUseCCD = true; // Mandatory CCD per GEMINI.md!
		ChipComp->SetNotifyRigidBodyCollision(true);

		ChipMeshes.Add(ChipComp);
	}
}

void AChipStackHazard::BeginPlay()
{
	Super::BeginPlay();

	InitialChipTransforms.Empty();
	for (UStaticMeshComponent* ChipComp : ChipMeshes)
	{
		if (ChipComp)
		{
			InitialChipTransforms.Add(ChipComp->GetComponentTransform());
			ChipComp->OnComponentHit.AddDynamic(this, &AChipStackHazard::HandleChipHit);
		}
	}
}

void AChipStackHazard::ResetForNewThrow()
{
	Super::ResetForNewThrow();
	bScatteredThisThrow = false;

	for (int32 i = 0; i < ChipMeshes.Num(); ++i)
	{
		if (ChipMeshes[i] && InitialChipTransforms.IsValidIndex(i))
		{
			ChipMeshes[i]->SetPhysicsLinearVelocity(FVector::ZeroVector);
			ChipMeshes[i]->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			ChipMeshes[i]->SetWorldTransform(InitialChipTransforms[i], false, nullptr, ETeleportType::ResetPhysics);
		}
	}
}

void AChipStackHazard::HandleChipHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bScatteredThisThrow)
	{
		return;
	}

	ADieBase* HitDie = Cast<ADieBase>(OtherActor);
	if (!HitDie)
	{
		return;
	}

	bScatteredThisThrow = true;

	PushMidRollScoringStep(
		TEXT("CHIP STACK SCATTERED! (+45 CHIPS / +0.3x MULT)"),
		EScoringLayer::BaseChips,
		45,
		0.3f,
		GetActorLocation() + FVector(0.0f, 0.0f, 30.0f));
}

void AChipStackHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor ChipHighlight = bScatteredThisThrow ? FColor(0, 245, 212) : FColor(255, 183, 3);
	for (UStaticMeshComponent* ChipComp : ChipMeshes)
	{
		if (ChipComp)
		{
			const FVector Pos = ChipComp->GetComponentLocation();
			DrawDebugCircle(
				World,
				Pos + FVector(0.0f, 0.0f, 2.5f),
				14.5f,
				14,
				ChipHighlight,
				false,
				-1.0f,
				0,
				1.5f,
				FVector(1.0f, 0.0f, 0.0f),
				FVector(0.0f, 1.0f, 0.0f),
				false);
		}
	}
}
