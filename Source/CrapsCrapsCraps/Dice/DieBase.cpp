#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Dice/PDA_DiceFace.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ADieBase::ADieBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create 20cm x 20cm x 20cm (10x Scale) Die Root Mesh
	DieMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DieMesh"));
	SetRootComponent(DieMesh);

	// Use Engine Cube (100x100x100cm) scaled to 0.2 -> exactly 20x20x20cm in Unreal units
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		DieMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	DieMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));

	// Non-negotiable 10x Chaos Physics rules: Enable CCD and RigidBody Collision events
	DieMesh->SetSimulatePhysics(false);
	DieMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DieMesh->SetCollisionObjectType(ECC_PhysicsBody);
	DieMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DieMesh->BodyInstance.bUseCCD = true;
	DieMesh->SetNotifyRigidBodyCollision(true);
	DieMesh->SetLinearDamping(0.25f);
	DieMesh->SetAngularDamping(0.35f);
	DieMesh->BodyInstance.bOverrideMass = true;
	DieMesh->BodyInstance.SetMassOverride(0.45f); // Scaled acrylic die mass for snappy Chaos response

	// Create 6 directional face sockets at the surface centers of the 20cm cube
	// Since parent DieMesh has 0.2 relative scale, 50.0 local units * 0.2 = 10.0cm world offset!
	constexpr float LocalSocketOffset = 50.0f;

	Socket_PlusZ = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_PlusZ"));
	Socket_PlusZ->SetupAttachment(DieMesh);
	Socket_PlusZ->SetRelativeLocation(FVector(0.0f, 0.0f, LocalSocketOffset));
	Socket_PlusZ->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));

	Socket_MinusZ = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_MinusZ"));
	Socket_MinusZ->SetupAttachment(DieMesh);
	Socket_MinusZ->SetRelativeLocation(FVector(0.0f, 0.0f, -LocalSocketOffset));
	Socket_MinusZ->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	Socket_PlusX = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_PlusX"));
	Socket_PlusX->SetupAttachment(DieMesh);
	Socket_PlusX->SetRelativeLocation(FVector(LocalSocketOffset, 0.0f, 0.0f));
	Socket_PlusX->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	Socket_MinusX = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_MinusX"));
	Socket_MinusX->SetupAttachment(DieMesh);
	Socket_MinusX->SetRelativeLocation(FVector(-LocalSocketOffset, 0.0f, 0.0f));
	Socket_MinusX->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	Socket_PlusY = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_PlusY"));
	Socket_PlusY->SetupAttachment(DieMesh);
	Socket_PlusY->SetRelativeLocation(FVector(0.0f, LocalSocketOffset, 0.0f));
	Socket_PlusY->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	Socket_MinusY = CreateDefaultSubobject<USceneComponent>(TEXT("Socket_MinusY"));
	Socket_MinusY->SetupAttachment(DieMesh);
	Socket_MinusY->SetRelativeLocation(FVector(0.0f, -LocalSocketOffset, 0.0f));
	Socket_MinusY->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	// Attach Torque Magnet Rigging Component
	DiceRiggingComponent = CreateDefaultSubobject<UDiceRiggingComponent>(TEXT("DiceRiggingComponent"));

	// Populate Western casino opposite-faces-sum-to-7 defaults
	DefaultFaceValues.Add(EDieFaceDirection::PlusZ, 1);
	DefaultFaceValues.Add(EDieFaceDirection::MinusZ, 6);
	DefaultFaceValues.Add(EDieFaceDirection::PlusX, 2);
	DefaultFaceValues.Add(EDieFaceDirection::MinusX, 5);
	DefaultFaceValues.Add(EDieFaceDirection::PlusY, 3);
	DefaultFaceValues.Add(EDieFaceDirection::MinusY, 4);
}

void ADieBase::BeginPlay()
{
	Super::BeginPlay();

	if (DieMesh)
	{
		DieMesh->BodyInstance.bUseCCD = true;
		DieMesh->OnComponentHit.AddDynamic(this, &ADieBase::HandleComponentHit);

		// Give the acrylic die its own lively PhysicalMaterial so corners catch and bounce crisply
		UPhysicalMaterial* DiePhysMat = NewObject<UPhysicalMaterial>(this, TEXT("PM_AcrylicDie"));
		DiePhysMat->Friction = 0.38f;
		DiePhysMat->StaticFriction = 0.45f;
		DiePhysMat->Restitution = 0.56f;
		DiePhysMat->RestitutionCombineMode = EFrictionCombineMode::Max;
		DiePhysMat->bOverrideRestitutionCombineMode = true;
		DieMesh->SetPhysMaterialOverride(DiePhysMat);
	}
}

void ADieBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDrawDebugFaceOverlay)
	{
		DrawDebugDieFaces();
	}

	if (bIsRolling && !bHasSettled && DieMesh && DieMesh->IsSimulatingPhysics())
	{
		const float LinearSpeed = DieMesh->GetPhysicsLinearVelocity().Size();
		const float AngularSpeed = DieMesh->GetPhysicsAngularVelocityInDegrees().Size();

		if (LinearSpeed <= SettledLinearSpeedThreshold && AngularSpeed <= SettledAngularSpeedThreshold)
		{
			SettledTimer += DeltaTime;
			if (SettledTimer >= SettledHoldTimeRequired)
			{
				bIsRolling = false;
				bHasSettled = true;

				// Put physics to rest cleanly
				DieMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
				DieMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

				float MaxDot = 1.0f;
				const EDieFaceDirection TopDir = GetTopFaceDirection(MaxDot);
				const int32 TopVal = GetFaceValueForDirection(TopDir);
				UPDA_DiceFace* TopAsset = GetTopFaceData();
				bIsCocked = (MaxDot < CockedAlignmentThreshold);

				OnDieSettled.Broadcast(this, TopDir, TopVal, TopAsset);
				BP_OnDieSettled(TopDir, TopVal, TopAsset, bIsCocked);
			}
		}
		else
		{
			SettledTimer = 0.0f;
		}
	}
}

FVector ADieBase::GetLocalNormalForDirection(EDieFaceDirection Direction)
{
	switch (Direction)
	{
	case EDieFaceDirection::PlusZ:  return FVector(0.0f, 0.0f, 1.0f);
	case EDieFaceDirection::MinusZ: return FVector(0.0f, 0.0f, -1.0f);
	case EDieFaceDirection::PlusX:  return FVector(1.0f, 0.0f, 0.0f);
	case EDieFaceDirection::MinusX: return FVector(-1.0f, 0.0f, 0.0f);
	case EDieFaceDirection::PlusY:  return FVector(0.0f, 1.0f, 0.0f);
	case EDieFaceDirection::MinusY: return FVector(0.0f, -1.0f, 0.0f);
	default:                        return FVector::UpVector;
	}
}

EDieFaceDirection ADieBase::GetOppositeDirection(EDieFaceDirection Direction)
{
	switch (Direction)
	{
	case EDieFaceDirection::PlusZ:  return EDieFaceDirection::MinusZ;
	case EDieFaceDirection::MinusZ: return EDieFaceDirection::PlusZ;
	case EDieFaceDirection::PlusX:  return EDieFaceDirection::MinusX;
	case EDieFaceDirection::MinusX: return EDieFaceDirection::PlusX;
	case EDieFaceDirection::PlusY:  return EDieFaceDirection::MinusY;
	case EDieFaceDirection::MinusY: return EDieFaceDirection::PlusY;
	default:                        return EDieFaceDirection::MinusZ;
	}
}

FVector ADieBase::GetWorldNormalForDirection(EDieFaceDirection Direction) const
{
	const FQuat WorldQuat = DieMesh ? DieMesh->GetComponentQuat() : GetActorQuat();
	return WorldQuat.RotateVector(GetLocalNormalForDirection(Direction)).GetSafeNormal();
}

USceneComponent* ADieBase::GetFaceSocket(EDieFaceDirection Direction) const
{
	switch (Direction)
	{
	case EDieFaceDirection::PlusZ:  return Socket_PlusZ;
	case EDieFaceDirection::MinusZ: return Socket_MinusZ;
	case EDieFaceDirection::PlusX:  return Socket_PlusX;
	case EDieFaceDirection::MinusX: return Socket_MinusX;
	case EDieFaceDirection::PlusY:  return Socket_PlusY;
	case EDieFaceDirection::MinusY: return Socket_MinusY;
	default:                        return Socket_PlusZ;
	}
}

EDieFaceDirection ADieBase::GetTopFaceDirection(float& OutMaxDotProduct) const
{
	static const EDieFaceDirection AllDirections[6] = {
		EDieFaceDirection::PlusZ,
		EDieFaceDirection::MinusZ,
		EDieFaceDirection::PlusX,
		EDieFaceDirection::MinusX,
		EDieFaceDirection::PlusY,
		EDieFaceDirection::MinusY
	};

	EDieFaceDirection BestDirection = EDieFaceDirection::PlusZ;
	float BestDot = -2.0f;

	for (EDieFaceDirection Dir : AllDirections)
	{
		const FVector WorldNormal = GetWorldNormalForDirection(Dir);
		const float Dot = FVector::DotProduct(WorldNormal, FVector::UpVector);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestDirection = Dir;
		}
	}

	OutMaxDotProduct = BestDot;
	return BestDirection;
}

int32 ADieBase::GetFaceValueForDirection(EDieFaceDirection Direction) const
{
	if (const TObjectPtr<UPDA_DiceFace>* FoundAsset = EquippedFaceAssets.Find(Direction))
	{
		if (*FoundAsset != nullptr)
		{
			return (*FoundAsset)->FaceValue;
		}
	}

	if (const int32* FoundDefault = DefaultFaceValues.Find(Direction))
	{
		return *FoundDefault;
	}

	return 1;
}

int32 ADieBase::GetTopFaceValue() const
{
	float DummyDot = 1.0f;
	const EDieFaceDirection TopDir = GetTopFaceDirection(DummyDot);
	return GetFaceValueForDirection(TopDir);
}

UPDA_DiceFace* ADieBase::GetTopFaceData() const
{
	float DummyDot = 1.0f;
	const EDieFaceDirection TopDir = GetTopFaceDirection(DummyDot);
	if (const TObjectPtr<UPDA_DiceFace>* FoundAsset = EquippedFaceAssets.Find(TopDir))
	{
		return *FoundAsset;
	}
	return nullptr;
}

bool ADieBase::FindDirectionForFaceValue(int32 TargetValue, EDieFaceDirection& OutDirection) const
{
	static const EDieFaceDirection AllDirections[6] = {
		EDieFaceDirection::PlusZ,
		EDieFaceDirection::MinusZ,
		EDieFaceDirection::PlusX,
		EDieFaceDirection::MinusX,
		EDieFaceDirection::PlusY,
		EDieFaceDirection::MinusY
	};

	for (EDieFaceDirection Dir : AllDirections)
	{
		if (GetFaceValueForDirection(Dir) == TargetValue)
		{
			OutDirection = Dir;
			return true;
		}
	}
	return false;
}

FQuat ADieBase::MakeDieOrientation(EDieFaceDirection DesiredTopFace, EDieFaceDirection DesiredFrontFace, float TableYawDegrees)
{
	const FVector LocalTop = GetLocalNormalForDirection(DesiredTopFace);
	FVector LocalFront = GetLocalNormalForDirection(DesiredFrontFace);

	// Ensure DesiredFrontFace is orthogonal to DesiredTopFace; if parallel, pick a valid orthogonal face
	if (FMath::Abs(FVector::DotProduct(LocalTop, LocalFront)) > 0.9f)
	{
		LocalFront = (FMath::Abs(LocalTop.X) < 0.5f) ? FVector::ForwardVector : FVector::RightVector;
	}

	const FVector LocalRight = FVector::CrossProduct(LocalTop, LocalFront).GetSafeNormal();

	// Local-to-Desired basis matrix: maps LocalFront -> +X, LocalRight -> +Y, LocalTop -> +Z
	FMatrix LocalBasis(LocalFront, LocalRight, LocalTop, FVector::ZeroVector);
	const FQuat BaseOrientation = LocalBasis.GetTransposed().ToQuat();
	const FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(TableYawDegrees));
	return (YawRotation * BaseOrientation).GetNormalized();
}

void ADieBase::EnterOnFeltSettingState()
{
	bIsRolling = false;
	bHasSettled = false;
	bIsCocked = false;
	SettledTimer = 0.0f;
	CurrentThrowBounceCount = 0;

	if (DieMesh)
	{
		DieMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		DieMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		DieMesh->SetSimulatePhysics(false);
		DieMesh->BodyInstance.bUseCCD = true;
	}
}

void ADieBase::LaunchDie(const FVector& LinearVelocityCmPerSec, const FVector& AngularVelocityDegPerSec)
{
	if (!DieMesh)
	{
		return;
	}

	if (DiceRiggingComponent)
	{
		DiceRiggingComponent->CapturePreThrowDesiredFace();
	}

	bIsRolling = true;
	bHasSettled = false;
	bIsCocked = false;
	SettledTimer = 0.0f;
	CurrentThrowBounceCount = 0;

	DieMesh->BodyInstance.bUseCCD = true;
	DieMesh->SetSimulatePhysics(true);
	DieMesh->WakeRigidBody();
	DieMesh->SetPhysicsLinearVelocity(LinearVelocityCmPerSec);
	DieMesh->SetPhysicsAngularVelocityInDegrees(AngularVelocityDegPerSec);
}

void ADieBase::HandleComponentHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (!bIsRolling || !DieMesh)
	{
		return;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (CurrentTime - LastBounceTime < 0.04)
	{
		return;
	}

	const float ImpactSpeed = DieMesh->GetPhysicsLinearVelocity().Size();
	if (ImpactSpeed > 60.0f)
	{
		LastBounceTime = CurrentTime;
		++CurrentThrowBounceCount;
		OnDieImpact.Broadcast(this, OtherActor, ImpactSpeed);
		BP_OnDieBounceImpact(Hit.ImpactPoint, Hit.ImpactNormal, ImpactSpeed, OtherActor);
	}
}

void ADieBase::DrawDebugDieFaces() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float MaxDot = 1.0f;
	const EDieFaceDirection TopDir = GetTopFaceDirection(MaxDot);

	static const EDieFaceDirection AllDirections[6] = {
		EDieFaceDirection::PlusZ,
		EDieFaceDirection::MinusZ,
		EDieFaceDirection::PlusX,
		EDieFaceDirection::MinusX,
		EDieFaceDirection::PlusY,
		EDieFaceDirection::MinusY
	};

	const FVector DieCenter = GetActorLocation();

	for (EDieFaceDirection Dir : AllDirections)
	{
		const FVector Normal = GetWorldNormalForDirection(Dir);
		const FVector FacePos = DieCenter + Normal * (DieHalfExtentCm + 2.5f);
		const int32 FaceVal = GetFaceValueForDirection(Dir);
		const bool bIsTop = (Dir == TopDir);

		const FColor PipColor = bIsTop ? FColor(0, 245, 212) : FColor(255, 0, 127);
		DrawDebugString(World, FacePos, FString::Printf(TEXT("[%d]"), FaceVal), nullptr, PipColor, 0.0f, true, bIsTop ? 1.35f : 0.95f);
	}
}
