// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/CrapsTableGrayboxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Table/CrapsGameState.h"
#include "Table/CrapsFlyingChipActor.h"
#include "Pawn/CrapsPawn.h"
#include "Camera/CameraComponent.h"
#include "Progression/RunProgressionSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACrapsTableGrayboxActor::ACrapsTableGrayboxActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TableRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TableRoot"));
	SetRootComponent(TableRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;

	// Use custom imported /Game/Meshes/SM_AlligatorWallStrip if available
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AlligatorFinder(TEXT("/Game/Meshes/SM_AlligatorWallStrip.SM_AlligatorWallStrip"));
	UStaticMesh* AlligatorMesh = AlligatorFinder.Succeeded() ? AlligatorFinder.Object : nullptr;

	// 1. Matte-black velvet felt bed: X from -150..1650 (center 750, length 1800), Y from -440..+440 (width 880), top surface Z = 0.0f
	// 1. Matte-black velvet felt bed: X from -150..1650 (center 720, length 1800), Y from -440..+440 (width 880), top surface Z = 0.0f
	FeltBedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeltBedMesh"));
	FeltBedMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		FeltBedMesh->SetStaticMesh(CubeMesh);
	}
	FeltBedMesh->SetRelativeLocation(FVector(720.0f, 0.0f, -30.0f));
	FeltBedMesh->SetRelativeScale3D(FVector(18.0f, 8.8f, 0.6f));
	FeltBedMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 2. Realistic Casino Wall Height (120cm visible wall + flush padded leather top rail)
	// Felt interior: X = -100 .. 1540 (length 1640, center X = 720), Y = -410 .. +410 (width 820, center Y = 0), top surface Z = 0
	// Back Wall at North end (X = 1570, Y = 0, Z = 50, Scale = (0.6, 8.8, 1.4) -> top at Z = 120cm)
	BackWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWallMesh"));
	BackWallMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		BackWallMesh->SetStaticMesh(CubeMesh);
	}
	BackWallMesh->SetRelativeLocation(FVector(1570.0f, 0.0f, 50.0f));
	BackWallMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BackWallMesh->SetRelativeScale3D(FVector(0.6f, 8.8f, 1.4f));
	BackWallMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 2b. Flush Padded Leather Top Rail on Back Wall (Z = 126cm, flat FRotator::ZeroRotator)
	BackOverhangRailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackOverhangRailMesh"));
	BackOverhangRailMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		BackOverhangRailMesh->SetStaticMesh(CubeMesh);
	}
	BackOverhangRailMesh->SetRelativeLocation(FVector(1575.0f, 0.0f, 126.0f));
	BackOverhangRailMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BackOverhangRailMesh->SetRelativeScale3D(FVector(0.70f, 8.8f, 0.14f));
	BackOverhangRailMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 3. Dual Alligator Diamond Pyramid Rubber Strips on BOTH North back wall (X = 1535) and South end wall (X = -95)
	UStaticMeshComponent* AlligatorWallLinerNorth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AlligatorWallLinerNorth"));
	AlligatorWallLinerNorth->SetupAttachment(TableRoot);
	if (AlligatorMesh)
	{
		AlligatorWallLinerNorth->SetStaticMesh(AlligatorMesh);
	}
	AlligatorWallLinerNorth->SetRelativeLocation(FVector(1535.0f, 0.0f, 0.0f));
	AlligatorWallLinerNorth->SetRelativeRotation(FRotator::ZeroRotator);
	AlligatorWallLinerNorth->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	AlligatorPyramidMeshes.Add(AlligatorWallLinerNorth);

	UStaticMeshComponent* AlligatorWallLinerSouth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AlligatorWallLinerSouth"));
	AlligatorWallLinerSouth->SetupAttachment(TableRoot);
	if (AlligatorMesh)
	{
		AlligatorWallLinerSouth->SetStaticMesh(AlligatorMesh);
	}
	AlligatorWallLinerSouth->SetRelativeLocation(FVector(-95.0f, 0.0f, 0.0f));
	AlligatorWallLinerSouth->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	AlligatorWallLinerSouth->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	AlligatorPyramidMeshes.Add(AlligatorWallLinerSouth);

	// 4. Front Shooter Wall (South end wall) at X = -130cm (Z = 50, Scale = (0.6, 8.8, 1.4) -> top at Z = 120cm)
	FrontWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontWallMesh"));
	FrontWallMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		FrontWallMesh->SetStaticMesh(CubeMesh);
	}
	FrontWallMesh->SetRelativeLocation(FVector(-130.0f, 0.0f, 50.0f));
	FrontWallMesh->SetRelativeRotation(FRotator::ZeroRotator);
	FrontWallMesh->SetRelativeScale3D(FVector(0.6f, 8.8f, 1.4f));
	FrontWallMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 5. Left Rail Wall at Y = -440cm (Z = 50, Scale = (17.0, 0.6, 1.4) -> top at Z = 120cm) + Flush Top Rail (Z = 126cm)
	LeftWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWallMesh"));
	LeftWallMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		LeftWallMesh->SetStaticMesh(CubeMesh);
	}
	LeftWallMesh->SetRelativeLocation(FVector(720.0f, -440.0f, 50.0f));
	LeftWallMesh->SetRelativeRotation(FRotator::ZeroRotator);
	LeftWallMesh->SetRelativeScale3D(FVector(17.0f, 0.6f, 1.4f));
	LeftWallMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	LeftOverhangRailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftOverhangRailMesh"));
	LeftOverhangRailMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		LeftOverhangRailMesh->SetStaticMesh(CubeMesh);
	}
	LeftOverhangRailMesh->SetRelativeLocation(FVector(720.0f, -445.0f, 126.0f));
	LeftOverhangRailMesh->SetRelativeRotation(FRotator::ZeroRotator);
	LeftOverhangRailMesh->SetRelativeScale3D(FVector(17.6f, 0.70f, 0.14f));
	LeftOverhangRailMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 6. Right Rail Wall at Y = +440cm (Z = 50, Scale = (17.0, 0.6, 1.4) -> top at Z = 120cm) + Flush Top Rail (Z = 126cm)
	RightWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWallMesh"));
	RightWallMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		RightWallMesh->SetStaticMesh(CubeMesh);
	}
	RightWallMesh->SetRelativeLocation(FVector(720.0f, 440.0f, 50.0f));
	RightWallMesh->SetRelativeRotation(FRotator::ZeroRotator);
	RightWallMesh->SetRelativeScale3D(FVector(17.0f, 0.6f, 1.4f));
	RightWallMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	RightOverhangRailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightOverhangRailMesh"));
	RightOverhangRailMesh->SetupAttachment(TableRoot);
	if (CubeMesh)
	{
		RightOverhangRailMesh->SetStaticMesh(CubeMesh);
	}
	RightOverhangRailMesh->SetRelativeLocation(FVector(720.0f, 445.0f, 126.0f));
	RightOverhangRailMesh->SetRelativeRotation(FRotator::ZeroRotator);
	RightOverhangRailMesh->SetRelativeScale3D(FVector(17.6f, 0.70f, 0.14f));
	RightOverhangRailMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// 7. Invisible Ceiling Containment Box at Z = 420cm
	PitCeilingContainmentBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PitCeilingContainmentBox"));
	PitCeilingContainmentBox->SetupAttachment(TableRoot);
	PitCeilingContainmentBox->SetRelativeLocation(FVector(720.0f, 0.0f, 420.0f));
	PitCeilingContainmentBox->SetBoxExtent(FVector(920.0f, 460.0f, 25.0f));
	PitCeilingContainmentBox->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PitCeilingContainmentBox->SetHiddenInGame(true);

	// 7b. Four invisible (bHiddenInGame = true) upper containment wall boxes from Z = 120cm to Z = 420cm (height 300cm, center Z = 270cm, half-extent Z = 150cm)
	// Flush with inner wall planes: North X = 1540, South X = -100, Left Y = -410, Right Y = +410
	auto CreateUpperContainment = [this](const TCHAR* Name, const FVector& Loc, const FVector& Extent) -> UBoxComponent*
	{
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetupAttachment(TableRoot);
		Box->SetRelativeLocation(Loc);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box->SetHiddenInGame(true);
		UpperContainmentBoxes.Add(Box);
		return Box;
	};
	CreateUpperContainment(TEXT("UpperContainment_North"), FVector(1550.0f,    0.0f, 270.0f), FVector(10.0f, 430.0f, 150.0f));
	CreateUpperContainment(TEXT("UpperContainment_South"), FVector( -110.0f,    0.0f, 270.0f), FVector(10.0f, 430.0f, 150.0f));
	CreateUpperContainment(TEXT("UpperContainment_Left"),  FVector(  720.0f, -420.0f, 270.0f), FVector(840.0f, 10.0f, 150.0f));
	CreateUpperContainment(TEXT("UpperContainment_Right"), FVector(  720.0f,  420.0f, 270.0f), FVector(840.0f, 10.0f, 150.0f));

	// 8. Four chamfered corner walls that round off the pit (rounded tub): (1460, ±330, 50) and (-20, ±330, 50), Scale = (2.4, 0.5, 1.4), Yaw = ±45°
	struct FChamferDef { FVector Loc; float Yaw; };
	const FChamferDef Chamfers[4] = {
		{ FVector(1460.0f,  330.0f, 50.0f), -45.0f }, // back, +Y
		{ FVector(1460.0f, -330.0f, 50.0f),  45.0f }, // back, -Y
		{ FVector( -20.0f,  330.0f, 50.0f),  45.0f }, // front, +Y
		{ FVector( -20.0f, -330.0f, 50.0f), -45.0f }, // front, -Y
	};
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Chamfer = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("CornerChamfer_%d"), i));
		Chamfer->SetupAttachment(TableRoot);
		if (CubeMesh)
		{
			Chamfer->SetStaticMesh(CubeMesh);
		}
		Chamfer->SetRelativeLocation(Chamfers[i].Loc);
		Chamfer->SetRelativeRotation(FRotator(0.0f, Chamfers[i].Yaw, 0.0f));
		Chamfer->SetRelativeScale3D(FVector(2.4f, 0.5f, 1.4f));
		Chamfer->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		CornerChamferMeshes.Add(Chamfer);
	}

	// 9. Authentic Casino Rail-Standing Shooter Stations
	auto MakeCasinoStation = [](
		const TCHAR* Name,
		const FVector& Loc,
		float Yaw,
		const FVector& Staging,
		const FVector& Stickman,
		const FVector& TargetWall,
		const FRotator& SettingCamRot,
		const FRotator& ThrowCamRot)
	{
		FCrapsShooterStation S;
		S.StationName = Name;
		S.PawnLocalLocation = Loc;
		S.PawnLocalRotation = FRotator(0.0f, Yaw, 0.0f);
		S.DiceStagingLocalPos = Staging;
		S.StickmanOriginLocalPos = Stickman;
		S.TargetBackWallLocalCenter = TargetWall;
		S.SettingCameraLocalRot = SettingCamRot;
		S.ThrowCameraLocalRot = ThrowCamRot;
		return S;
	};

	const FVector StickmanShared(720.0f, -40.0f, 18.0f);
	const FVector NorthBackWallCenter(1500.0f, 0.0f, 25.0f);

	// Station 0: South End Center (-> North Wall) - Classic Head of Table
	ShooterStations.Add(MakeCasinoStation(
		TEXT("South End Center (-> North Wall)"),
		FVector(-155.0f, 0.0f, 0.0f),
		0.0f,
		FVector(35.0f, 0.0f, 11.0f),
		StickmanShared,
		NorthBackWallCenter,
		FRotator(-27.0f, 0.0f, 0.0f),
		FRotator(-27.0f, 0.0f, 0.0f)
	));

	// Station 1: South Left-Center (-> North Wall)
	ShooterStations.Add(MakeCasinoStation(
		TEXT("South Left-Center (-> North Wall)"),
		FVector(-155.0f, -90.0f, 0.0f),
		3.5f,
		FVector(35.0f, -80.0f, 11.0f),
		StickmanShared,
		NorthBackWallCenter,
		FRotator(-27.0f, 0.0f, 0.0f),
		FRotator(-27.0f, 0.0f, 0.0f)
	));

	// Station 2: South Right-Center (-> North Wall)
	ShooterStations.Add(MakeCasinoStation(
		TEXT("South Right-Center (-> North Wall)"),
		FVector(-155.0f, 90.0f, 0.0f),
		-3.5f,
		FVector(35.0f, 80.0f, 11.0f),
		StickmanShared,
		NorthBackWallCenter,
		FRotator(-27.0f, 0.0f, 0.0f),
		FRotator(-27.0f, 0.0f, 0.0f)
	));

	// Station 3: South Hook Left (-> North Wall)
	ShooterStations.Add(MakeCasinoStation(
		TEXT("South Hook Left (-> North Wall)"),
		FVector(-155.0f, -180.0f, 0.0f),
		7.0f,
		FVector(35.0f, -160.0f, 11.0f),
		StickmanShared,
		NorthBackWallCenter,
		FRotator(-27.0f, 0.0f, 0.0f),
		FRotator(-27.0f, 0.0f, 0.0f)
	));

	// Station 4: South Hook Right (-> North Wall)
	ShooterStations.Add(MakeCasinoStation(
		TEXT("South Hook Right (-> North Wall)"),
		FVector(-155.0f, 180.0f, 0.0f),
		-7.0f,
		FVector(35.0f, 160.0f, 11.0f),
		StickmanShared,
		NorthBackWallCenter,
		FRotator(-27.0f, 0.0f, 0.0f),
		FRotator(-27.0f, 0.0f, 0.0f)
	));
}

void ACrapsTableGrayboxActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureLayoutResolved();

	FeltPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("PM_CrapsFelt"));
	FeltPhysicalMaterial->Friction = 0.52f;
	FeltPhysicalMaterial->StaticFriction = 0.62f;
	FeltPhysicalMaterial->Restitution = 0.38f;

	BackWallPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("PM_AlligatorWall"));
	BackWallPhysicalMaterial->Friction = 0.48f;
	BackWallPhysicalMaterial->Restitution = 0.68f;
	BackWallPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
	BackWallPhysicalMaterial->bOverrideRestitutionCombineMode = true;

	if (FeltBedMesh)
	{
		FeltBedMesh->SetPhysMaterialOverride(FeltPhysicalMaterial);
	}
	if (BackWallMesh)
	{
		BackWallMesh->SetPhysMaterialOverride(BackWallPhysicalMaterial);
	}
	if (BackOverhangRailMesh)
	{
		BackOverhangRailMesh->SetPhysMaterialOverride(FeltPhysicalMaterial);
	}
	if (FrontWallMesh)
	{
		FrontWallMesh->SetPhysMaterialOverride(BackWallPhysicalMaterial);
	}
	if (LeftWallMesh)
	{
		LeftWallMesh->SetPhysMaterialOverride(BackWallPhysicalMaterial);
	}
	if (LeftOverhangRailMesh)
	{
		LeftOverhangRailMesh->SetPhysMaterialOverride(FeltPhysicalMaterial);
	}
	if (RightWallMesh)
	{
		RightWallMesh->SetPhysMaterialOverride(BackWallPhysicalMaterial);
	}
	if (RightOverhangRailMesh)
	{
		RightOverhangRailMesh->SetPhysMaterialOverride(FeltPhysicalMaterial);
	}
	for (UStaticMeshComponent* Pyramid : AlligatorPyramidMeshes)
	{
		if (Pyramid)
		{
			Pyramid->SetPhysMaterialOverride(BackWallPhysicalMaterial);
		}
	}
	for (UStaticMeshComponent* Chamfer : CornerChamferMeshes)
	{
		if (Chamfer)
		{
			Chamfer->SetPhysMaterialOverride(BackWallPhysicalMaterial);
		}
	}

	// Move the 3D dealer puck & update the CRT Scoreboard whenever the table phase or bankroll changes
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			BindToGameState(GS);
		}
		else
		{
			// GameState might not be spawned yet on this frame; queue a NextTick retry
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (UWorld* NextTickWorld = GetWorld())
				{
					if (ACrapsGameState* GS = NextTickWorld->GetGameState<ACrapsGameState>())
					{
						BindToGameState(GS);
					}
				}
			}));
		}

		if (UScoringQueueSubsystem* ScoringQueue = World->GetSubsystem<UScoringQueueSubsystem>())
		{
			ScoringQueue->OnScoringStepExecuted.RemoveDynamic(this, &ACrapsTableGrayboxActor::HandleScoringStepExecuted);
			ScoringQueue->OnScoringStepExecuted.AddDynamic(this, &ACrapsTableGrayboxActor::HandleScoringStepExecuted);

			ScoringQueue->OnScoringSequenceFinished.RemoveDynamic(this, &ACrapsTableGrayboxActor::HandleScoringSequenceFinished);
			ScoringQueue->OnScoringSequenceFinished.AddDynamic(this, &ACrapsTableGrayboxActor::HandleScoringSequenceFinished);
		}
	}
}

void ACrapsTableGrayboxActor::BindToGameState(ACrapsGameState* GS)
{
	if (!GS)
	{
		return;
	}

	GS->OnTablePhaseChanged.RemoveDynamic(this, &ACrapsTableGrayboxActor::HandleTablePhaseChanged);
	GS->OnTablePhaseChanged.AddDynamic(this, &ACrapsTableGrayboxActor::HandleTablePhaseChanged);

	GS->OnBankrollChanged.RemoveDynamic(this, &ACrapsTableGrayboxActor::HandleBankrollChanged);
	GS->OnBankrollChanged.AddDynamic(this, &ACrapsTableGrayboxActor::HandleBankrollChanged);

	GS->OnBankedPayoutVisuals.RemoveDynamic(this, &ACrapsTableGrayboxActor::HandleBankedPayoutVisuals);
	GS->OnBankedPayoutVisuals.AddDynamic(this, &ACrapsTableGrayboxActor::HandleBankedPayoutVisuals);

	int64 DisplayAnteQuota = GS->GetCurrentAnteQuota();
	if (UWorld* World = GetWorld())
	{
		if (const URunProgressionSubsystem* Prog = World->GetSubsystem<URunProgressionSubsystem>())
		{
			DisplayAnteQuota = Prog->GetCurrentAnteTargetQuota();
		}
	}

	HandleTablePhaseChanged(GS->GetTablePhase(), GS->GetEstablishedPoint());
	UpdateScoreboardReadouts(
		GS->GetPlayerBankroll(),
		DisplayAnteQuota,
		GS->GetUnbankedPool(),
		0,
		1.0f,
		1.0f,
		GS->GetTablePhase(),
		GS->GetEstablishedPoint());
}

void ACrapsTableGrayboxActor::HandleTablePhaseChanged(ECrapsTablePhase NewPhase, int32 NewPoint)
{
	SetDealerPuckState(NewPhase == ECrapsTablePhase::PointEstablished, NewPoint);

	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			const UScoringQueueSubsystem* SQ = World->GetSubsystem<UScoringQueueSubsystem>();
			UpdateScoreboardReadouts(
				GS->GetPlayerBankroll(),
				GS->GetCurrentAnteQuota(),
				GS->GetUnbankedPool(),
				SQ ? SQ->RunningBaseChips : 0,
				SQ ? SQ->RunningThrowMult : 1.0f,
				SQ ? SQ->RunningBetMult : 1.0f,
				NewPhase,
				NewPoint);
		}
	}
}

void ACrapsTableGrayboxActor::HandleBankrollChanged(int64 NewBankroll, int64 NewUnbankedPool)
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			const UScoringQueueSubsystem* SQ = World->GetSubsystem<UScoringQueueSubsystem>();
			int64 DisplayAnteQuota = GS->GetCurrentAnteQuota();
			if (const URunProgressionSubsystem* Prog = World->GetSubsystem<URunProgressionSubsystem>())
			{
				DisplayAnteQuota = Prog->GetCurrentAnteTargetQuota();
			}

			UpdateScoreboardReadouts(
				NewBankroll,
				DisplayAnteQuota,
				NewUnbankedPool,
				SQ ? SQ->RunningBaseChips : 0,
				SQ ? SQ->RunningThrowMult : 1.0f,
				SQ ? SQ->RunningBetMult : 1.0f,
				GS->GetTablePhase(),
				GS->GetEstablishedPoint());
		}
	}
}

void ACrapsTableGrayboxActor::HandleBankedPayoutVisuals(int64 ScoreAmount, int64 CashAmount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	FVector Destination = GetActorLocation() + FVector(0.0f, 0.0f, 160.0f);
	if (ACrapsPawn* CPawn = Cast<ACrapsPawn>(PlayerPawn))
	{
		if (CPawn->POVCamera)
		{
			const FVector CamLoc = CPawn->POVCamera->GetComponentLocation();
			const FVector CamFwd = CPawn->POVCamera->GetForwardVector();
			const FVector CamRight = CPawn->POVCamera->GetRightVector();
			const FVector CamUp = CPawn->POVCamera->GetUpVector();
			Destination = CamLoc + (CamFwd * 50.0f) - (CamRight * 15.0f) - (CamUp * 12.0f);
		}
		else
		{
			Destination = CPawn->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
		}
	}

	const FVector Origin = GetActorTransform().TransformPosition(FVector(650.0f, 0.0f, 15.0f));
	const int32 VisualChips = FMath::Clamp(static_cast<int32>(ScoreAmount / 40) + 6, 8, 22);
	ACrapsFlyingChipActor::SpawnPayoutCascade(
		World,
		Origin,
		Destination,
		VisualChips,
		FLinearColor(1.0f, 0.72f, 0.01f));
}

void ACrapsTableGrayboxActor::HandleScoringStepExecuted(
	const FScoringStep& Step,
	int32 RunningBaseChips,
	float RunningThrowMult,
	float RunningBetMult,
	float NormalizedMultiplierIntensity)
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			UpdateScoreboardReadouts(
				GS->GetPlayerBankroll(),
				GS->GetCurrentAnteQuota(),
				GS->GetUnbankedPool(),
				RunningBaseChips,
				RunningThrowMult,
				RunningBetMult,
				GS->GetTablePhase(),
				GS->GetEstablishedPoint());
		}
	}
}

void ACrapsTableGrayboxActor::HandleScoringSequenceFinished(int64 FinalThrowPayout, bool bWasSevenOut)
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			const UScoringQueueSubsystem* SQ = World->GetSubsystem<UScoringQueueSubsystem>();
			UpdateScoreboardReadouts(
				GS->GetPlayerBankroll(),
				GS->GetCurrentAnteQuota(),
				GS->GetUnbankedPool(),
				SQ ? SQ->RunningBaseChips : 0,
				SQ ? SQ->RunningThrowMult : 1.0f,
				SQ ? SQ->RunningBetMult : 1.0f,
				GS->GetTablePhase(),
				GS->GetEstablishedPoint());
		}
	}
}

void ACrapsTableGrayboxActor::SetFeltSurfaceFriction(float NormalizedFriction)
{
	const float Clamped = FMath::Clamp(NormalizedFriction, 0.0f, 1.0f);
	if (!FeltPhysicalMaterial)
	{
		FeltPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("PM_CrapsFelt"));
	}

	FeltPhysicalMaterial->Friction = FMath::Lerp(0.02f, 1.25f, Clamped);
	FeltPhysicalMaterial->StaticFriction = FMath::Lerp(0.02f, 1.40f, Clamped);

	if (FeltBedMesh)
	{
		FeltBedMesh->SetPhysMaterialOverride(FeltPhysicalMaterial);
	}
}

// ============================================================================
// Editor-visible felt layout & CRT Scoreboard (built in OnConstruction)
// ============================================================================
void ACrapsTableGrayboxActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ClearGeneratedLayout();
	CachedCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CachedCylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CachedScoreboardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_CRT_Scoreboard_v1.SM_CRT_Scoreboard_v1"));
	if (!CachedCubeMesh || !CachedCylinderMesh)
	{
		return;
	}

	BuildFeltLayout();
	BuildTubDressing();
	BuildCRTScoreboard();
}

void ACrapsTableGrayboxActor::ClearGeneratedLayout()
{
	for (UActorComponent* Comp : GeneratedLayoutComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	GeneratedLayoutComponents.Reset();
	ColorMIDCache.Reset();
	DealerPuckMesh = nullptr;
	DealerPuckText = nullptr;
	ScoreboardEnclosureMesh = nullptr;
	BankrollValueText = nullptr;
	AnteValueText = nullptr;
	UnbankedPoolValueText = nullptr;
	MultiplierValueText = nullptr;
	MultiplierSubText = nullptr;
	PointBoxCenters.Reset();
}

FVector2D ACrapsTableGrayboxActor::HalfToTable(float U, float V, bool bNorth)
{
	// Felt interior spans X = -100 .. 1540. South half grows +X from the front wall, north half mirrors from the back wall.
	return bNorth ? FVector2D(1540.0f - U, V) : FVector2D(-100.0f + U, V);
}

UMaterialInstanceDynamic* ACrapsTableGrayboxActor::GetColorMID(const FLinearColor& Color)
{
	const FColor Key = Color.ToFColor(false);
	const uint32 Hash = (uint32(Key.R) << 16) | (uint32(Key.G) << 8) | uint32(Key.B);
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = ColorMIDCache.Find(Hash))
	{
		return *Found;
	}

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	ColorMIDCache.Add(Hash, MID);
	return MID;
}

UStaticMeshComponent* ACrapsTableGrayboxActor::AddLayoutMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, bool bCollide)
{
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
	Comp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Comp->SetStaticMesh(Mesh);
	Comp->SetupAttachment(TableRoot);
	Comp->SetRelativeLocationAndRotation(Location, Rotation);
	Comp->SetRelativeScale3D(Scale);
	Comp->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	if (UMaterialInstanceDynamic* MID = GetColorMID(Color))
	{
		Comp->SetMaterial(0, MID);
	}
	Comp->RegisterComponent();
	GeneratedLayoutComponents.Add(Comp);
	return Comp;
}

void ACrapsTableGrayboxActor::AddFeltLine(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Width)
{
	const FVector2D Delta = B - A;
	const float Length = Delta.Size();
	if (Length < 1.0f)
	{
		return;
	}
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const FVector2D Mid = (A + B) * 0.5f;
	// Felt top is Z = 0; a 0.6cm slab centered at 0.3 sits flush on the surface.
	AddLayoutMesh(CachedCubeMesh, FVector(Mid.X, Mid.Y, 0.3f), FRotator(0.0f, Yaw, 0.0f), FVector(Length / 100.0f, Width / 100.0f, 0.006f), Color);
}

void ACrapsTableGrayboxActor::AddFeltRect(const FVector2D& Center, const FVector2D& Size, const FLinearColor& Color, float Width)
{
	const FVector2D H = Size * 0.5f;
	AddFeltLine(Center + FVector2D(-H.X, -H.Y), Center + FVector2D( H.X, -H.Y), Color, Width);
	AddFeltLine(Center + FVector2D( H.X, -H.Y), Center + FVector2D( H.X,  H.Y), Color, Width);
	AddFeltLine(Center + FVector2D( H.X,  H.Y), Center + FVector2D(-H.X,  H.Y), Color, Width);
	AddFeltLine(Center + FVector2D(-H.X,  H.Y), Center + FVector2D(-H.X, -H.Y), Color, Width);
}

void ACrapsTableGrayboxActor::AddFeltText(const FVector2D& Center, const FString& Text, float WorldSize, const FLinearColor& Color, bool bNorthFacing, bool bAlongX)
{
	UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
	Label->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Label->SetupAttachment(TableRoot);
	Label->SetText(FText::FromString(Text));
	Label->SetWorldSize(WorldSize);
	Label->SetTextRenderColor(Color.ToFColor(true));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);

	// Text lies flat on the felt. South-facing: top toward +X, reads along +Y. North-facing is rotated 180 degrees.
	const FQuat BaseQuat = bNorthFacing ? FRotator(90.0f, 0.0f, 0.0f).Quaternion() : FRotator(90.0f, 180.0f, 0.0f).Quaternion();
	const float AlongXYaw = bNorthFacing ? 90.0f : -90.0f;
	const FQuat FinalQuat = bAlongX ? (FRotator(0.0f, AlongXYaw, 0.0f).Quaternion() * BaseQuat) : BaseQuat;
	Label->SetRelativeRotation(FinalQuat);
	Label->SetRelativeLocation(FVector(Center.X, Center.Y, 0.9f));
	Label->RegisterComponent();
	GeneratedLayoutComponents.Add(Label);
}

void ACrapsTableGrayboxActor::BuildFeltLayout()
{
	const FLinearColor Cyan(0.0f, 0.96f, 0.83f);
	const FLinearColor Gold(1.0f, 0.72f, 0.01f);
	const FLinearColor Magenta(1.0f, 0.16f, 0.52f);
	const FLinearColor White(0.88f, 0.92f, 1.0f);
	const FLinearColor Red(0.95f, 0.08f, 0.15f);

	// ---- Two mirrored tubs: South Tub (Half 0: X = -100 + U), North Tub (Half 1: X = 1540 - U) ----
	static const TCHAR* PointNames[6] = { TEXT("4"), TEXT("5"), TEXT("SIX"), TEXT("8"), TEXT("NINE"), TEXT("10") };
	for (int32 Half = 0; Half < 2; ++Half)
	{
		const bool bNorth = (Half == 1);

		// 1. PASS LINE Track:
		// Player Side strip: Y = -365 .. -305, U = 80 .. 550
		AddFeltRect(HalfToTable(315.0f, -335.0f, bNorth), FVector2D(470.0f, 60.0f), White, 3.0f);
		AddFeltText(HalfToTable(315.0f, -335.0f, bNorth), TEXT("PASS LINE"), 16.0f, White, bNorth, true);

		// End Wall strip: U = 20 .. 80, Y = -305 .. +280
		AddFeltRect(HalfToTable(50.0f, -12.5f, bNorth), FVector2D(60.0f, 585.0f), White, 3.0f);
		AddFeltText(HalfToTable(50.0f, -12.5f, bNorth), TEXT("PASS LINE"), 18.0f, White, bNorth, false);

		// 2. DON'T PASS BAR 6-6:
		// Player Side strip: Y = -305 .. -250, U = 175 .. 550
		AddFeltRect(HalfToTable(362.5f, -277.5f, bNorth), FVector2D(375.0f, 55.0f), Gold, 2.5f);
		AddFeltText(HalfToTable(362.5f, -277.5f, bNorth), TEXT("DON'T PASS BAR"), 13.0f, Gold, bNorth, true);

		// End Wall strip: U = 80 .. 135, Y = -180 .. +280
		AddFeltRect(HalfToTable(107.5f, 50.0f, bNorth), FVector2D(55.0f, 460.0f), Gold, 2.5f);
		AddFeltText(HalfToTable(107.5f, 50.0f, bNorth), TEXT("DON'T PASS BAR"), 14.0f, Gold, bNorth, false);

		// 3. Corner BIG 6 | BIG 8 Diamond Box (U = 80 .. 175, Y = -305 .. -175):
		AddFeltRect(HalfToTable(127.5f, -240.0f, bNorth), FVector2D(95.0f, 130.0f), Magenta, 3.0f);
		// Diagonal divider line from (80, -305) to (175, -175)
		AddFeltLine(HalfToTable(80.0f, -305.0f, bNorth), HalfToTable(175.0f, -175.0f, bNorth), Magenta, 2.5f);
		AddFeltText(HalfToTable(105.0f, -260.0f, bNorth), TEXT("BIG 6"), 16.0f, Magenta, bNorth);
		AddFeltText(HalfToTable(150.0f, -220.0f, bNorth), TEXT("BIG 8"), 16.0f, Magenta, bNorth);

		// 4. FIELD Box (U = 175 .. 550, Y = -250 .. -95):
		AddFeltRect(HalfToTable(362.5f, -172.5f, bNorth), FVector2D(375.0f, 155.0f), Gold, 3.0f);
		AddFeltText(HalfToTable(362.5f, -150.0f, bNorth), TEXT("FIELD"), 32.0f, Gold, bNorth);
		AddFeltText(HalfToTable(362.5f, -185.0f, bNorth), TEXT("2 \u2022 3 \u2022 4 \u2022 9 \u2022 10 \u2022 11 \u2022 12"), 14.0f, Gold, bNorth);
		AddFeltText(HalfToTable(220.0f, -150.0f, bNorth), TEXT("2 PAYS DOUBLE"), 9.5f, White, bNorth);
		AddFeltText(HalfToTable(505.0f, -150.0f, bNorth), TEXT("12 PAYS TRIPLE"), 9.5f, White, bNorth);

		// 5. COME Box (U = 175 .. 550, Y = -95 .. +80):
		AddFeltRect(HalfToTable(362.5f, -7.5f, bNorth), FVector2D(375.0f, 175.0f), Magenta, 3.5f);
		AddFeltText(HalfToTable(362.5f, -7.5f, bNorth), TEXT("COME"), 58.0f, Magenta, bNorth);

		// 6. Top Row along Dealer Side (Y = +80 .. +280): 7 Columns
		// Column 0 (U = 135 .. 195): DON'T COME BAR
		AddFeltRect(HalfToTable(165.0f, 180.0f, bNorth), FVector2D(60.0f, 200.0f), White, 3.0f);
		AddFeltText(HalfToTable(165.0f, 180.0f, bNorth), TEXT("DON'T\nCOME\nBAR"), 16.0f, White, bNorth);

		// Columns 1..6 (U = 195 .. 555): 4, 5, SIX, 8, NINE, 10
		for (int32 i = 0; i < 6; ++i)
		{
			const float UCol = 195.0f + i * 60.0f + 30.0f;
			const FVector2D Center = HalfToTable(UCol, 180.0f, bNorth);
			AddFeltRect(Center, FVector2D(60.0f, 200.0f), Gold, 3.0f);
			// Internal horizontal dividers: top line (Buy/Lay) at Y = +235, bottom line (Place) at Y = +125
			AddFeltLine(HalfToTable(UCol - 30.0f, 235.0f, bNorth), HalfToTable(UCol + 30.0f, 235.0f, bNorth), Gold, 2.0f);
			AddFeltLine(HalfToTable(UCol - 30.0f, 125.0f, bNorth), HalfToTable(UCol + 30.0f, 125.0f, bNorth), Gold, 2.0f);
			// Large point number centered in middle zone (Y = 180)
			AddFeltText(Center, PointNames[i], FCString::Strlen(PointNames[i]) > 2 ? 26.0f : 34.0f, Gold, bNorth);

			if (!bNorth)
			{
				PointBoxCenters.Add(Center);
			}
		}
	}

	// ---- Center PROPOSITION BOX between the two tubs (X = 590 .. 850) ----
	{
		// Stickman / Player Side: X = 590 .. 850, Y = -355 .. +40
		AddFeltRect(FVector2D(720.0f, -157.5f), FVector2D(260.0f, 395.0f), Cyan, 3.5f);

		// Top banner (Y = +5 .. +40): ANY SEVEN 4 TO 1
		AddFeltRect(FVector2D(720.0f, 22.5f), FVector2D(250.0f, 35.0f), Red, 2.5f);
		AddFeltText(FVector2D(720.0f, 22.5f), TEXT("ANY SEVEN  4 TO 1"), 18.0f, Red);

		// Middle 2x2 grid (Y = -195 .. +5): Hardways
		static const TCHAR* HardwayNames[4] = {
			TEXT("HARD 4 (7 TO 1)"), TEXT("HARD 10 (7 TO 1)"),
			TEXT("HARD 6 (9 TO 1)"), TEXT("HARD 8 (9 TO 1)")
		};
		const float GridX[2] = { 655.0f, 785.0f };
		const float GridY[2] = { -45.0f, -145.0f };
		int32 HardIdx = 0;
		for (int32 gx = 0; gx < 2; ++gx)
		{
			for (int32 gy = 0; gy < 2; ++gy)
			{
				const FVector2D CellCenter(GridX[gx], GridY[gy]);
				AddFeltRect(CellCenter, FVector2D(120.0f, 90.0f), Cyan, 2.0f);
				AddFeltText(CellCenter, HardwayNames[HardIdx++], 13.0f, Cyan);
			}
		}

		// Lower section (Y = -305 .. -195): HORN BET 2 • 3 • 11 • 12
		AddFeltRect(FVector2D(720.0f, -250.0f), FVector2D(250.0f, 100.0f), Cyan, 2.5f);
		AddFeltText(FVector2D(720.0f, -250.0f), TEXT("HORN BET  2 \u2022 3 \u2022 11 \u2022 12"), 15.0f, Cyan);

		// Bottom banner (Y = -355 .. -305): ANY CRAPS 7 TO 1
		AddFeltRect(FVector2D(720.0f, -330.0f), FVector2D(250.0f, 45.0f), Cyan, 2.5f);
		AddFeltText(FVector2D(720.0f, -330.0f), TEXT("ANY CRAPS  7 TO 1"), 16.0f, Cyan);
	}

	// ---- 3D DEALER PUCK (ON = white, OFF = black). Moves when the table phase changes. ----
	{
		DealerPuckMesh = AddLayoutMesh(CachedCylinderMesh, FVector(PointBoxCenters.IsValidIndex(0) ? PointBoxCenters[0].X : 225.0f, 180.0f, 3.0f), FRotator::ZeroRotator, FVector(0.34f, 0.34f, 0.05f), FLinearColor(0.02f, 0.02f, 0.02f));
		DealerPuckMesh->SetMobility(EComponentMobility::Movable);
		DealerPuckMesh->ComponentTags.AddUnique(TEXT("DealerPuckMesh"));

		UTextRenderComponent* PuckLabel = NewObject<UTextRenderComponent>(this);
		PuckLabel->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		PuckLabel->SetMobility(EComponentMobility::Movable);
		PuckLabel->SetupAttachment(DealerPuckMesh);
		PuckLabel->SetText(FText::FromString(TEXT("OFF")));
		PuckLabel->SetWorldSize(11.0f);
		PuckLabel->SetTextRenderColor(FColor::White);
		PuckLabel->SetHorizontalAlignment(EHTA_Center);
		PuckLabel->SetVerticalAlignment(EVRTA_TextCenter);
		PuckLabel->SetWorldRotation(FRotator(90.0f, 180.0f, 0.0f));
		PuckLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
		PuckLabel->SetWorldScale3D(FVector(1.0f));
		PuckLabel->ComponentTags.AddUnique(TEXT("DealerPuckText"));
		PuckLabel->RegisterComponent();
		GeneratedLayoutComponents.Add(PuckLabel);
		DealerPuckText = PuckLabel;
		SetDealerPuckState(false, 0);
	}
}

void ACrapsTableGrayboxActor::EnsureLayoutResolved()
{
	// 1. Ensure deterministic point box centers (Point 4, 5, 6, 8, 9, 10)
	if (PointBoxCenters.Num() != 6)
	{
		PointBoxCenters = {
			FVector2D(125.0f, 180.0f), // Point 4
			FVector2D(185.0f, 180.0f), // Point 5
			FVector2D(245.0f, 180.0f), // Point 6
			FVector2D(305.0f, 180.0f), // Point 8
			FVector2D(365.0f, 180.0f), // Point 9
			FVector2D(425.0f, 180.0f)  // Point 10
		};
	}

	// 2. Ensure basic meshes are loaded
	if (!CachedCubeMesh)
	{
		CachedCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	if (!CachedCylinderMesh)
	{
		CachedCylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	if (!CachedScoreboardMesh)
	{
		CachedScoreboardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_CRT_Scoreboard_v1.SM_CRT_Scoreboard_v1"));
	}

	// 3. Resolve Dealer Puck Component references if not already set (e.g. across PIE duplication)
	if (!DealerPuckMesh)
	{
		for (UActorComponent* Comp : GetComponents())
		{
			if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
			{
				if (SMC->ComponentHasTag(TEXT("DealerPuckMesh")) ||
					(FMath::IsNearlyEqual(SMC->GetRelativeScale3D().X, 0.34f, 0.05f) &&
					 FMath::IsNearlyEqual(SMC->GetRelativeScale3D().Z, 0.05f, 0.03f)))
				{
					DealerPuckMesh = SMC;
					DealerPuckMesh->SetMobility(EComponentMobility::Movable);
					break;
				}
			}
		}
	}

	if (DealerPuckMesh && !DealerPuckText)
	{
		for (USceneComponent* Child : DealerPuckMesh->GetAttachChildren())
		{
			if (UTextRenderComponent* TRC = Cast<UTextRenderComponent>(Child))
			{
				DealerPuckText = TRC;
				DealerPuckText->SetMobility(EComponentMobility::Movable);
				break;
			}
		}
	}

	// 4. Runtime fallback: If no dealer puck exists on the actor at all, create it dynamically
	if (!DealerPuckMesh && CachedCylinderMesh)
	{
		DealerPuckMesh = NewObject<UStaticMeshComponent>(this, TEXT("RuntimeDealerPuckMesh"));
		DealerPuckMesh->SetMobility(EComponentMobility::Movable);
		DealerPuckMesh->SetStaticMesh(CachedCylinderMesh);
		DealerPuckMesh->SetupAttachment(TableRoot ? TableRoot.Get() : GetRootComponent());
		DealerPuckMesh->SetRelativeLocation(FVector(65.0f, 180.0f, 3.0f));
		DealerPuckMesh->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.05f));
		DealerPuckMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DealerPuckMesh->SetCastShadow(false);
		DealerPuckMesh->ComponentTags.AddUnique(TEXT("DealerPuckMesh"));
		if (GetWorld())
		{
			DealerPuckMesh->RegisterComponent();
		}
	}

	if (DealerPuckMesh && !DealerPuckText)
	{
		DealerPuckText = NewObject<UTextRenderComponent>(this, TEXT("RuntimeDealerPuckText"));
		DealerPuckText->SetMobility(EComponentMobility::Movable);
		DealerPuckText->SetupAttachment(DealerPuckMesh);
		DealerPuckText->SetText(FText::FromString(TEXT("OFF")));
		DealerPuckText->SetWorldSize(11.0f);
		DealerPuckText->SetTextRenderColor(FColor::White);
		DealerPuckText->SetHorizontalAlignment(EHTA_Center);
		DealerPuckText->SetVerticalAlignment(EVRTA_TextCenter);
		DealerPuckText->SetWorldRotation(FRotator(90.0f, 180.0f, 0.0f));
		DealerPuckText->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
		DealerPuckText->SetWorldScale3D(FVector(1.0f));
		DealerPuckText->ComponentTags.AddUnique(TEXT("DealerPuckText"));
		if (GetWorld())
		{
			DealerPuckText->RegisterComponent();
		}
	}

	// 5. Ensure CRT Scoreboard component references are resolved
	ResolveScoreboardReferences();
}

void ACrapsTableGrayboxActor::ResolveScoreboardReferences()
{
	if (BankrollValueText && AnteValueText && UnbankedPoolValueText && MultiplierValueText && MultiplierSubText)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || Actor == this)
			{
				continue;
			}
			if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Enclosure")))
			{
				// Found enclosure actor
			}
			else if (UTextRenderComponent* TextComp = Actor->FindComponentByClass<UTextRenderComponent>())
			{
				if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Bankroll")))
				{
					BankrollValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Ante")))
				{
					AnteValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Unbanked")))
				{
					UnbankedPoolValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Mult")))
				{
					MultiplierValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Hdr_Mult")))
				{
					MultiplierSubText = TextComp;
				}
			}
		}
	}
}

void ACrapsTableGrayboxActor::SetDealerPuckState(bool bPuckOn, int32 PointNumber)
{
	EnsureLayoutResolved();

	if (!DealerPuckMesh)
	{
		return;
	}

	int32 BoxIndex = INDEX_NONE;
	switch (PointNumber)
	{
	case 4:  BoxIndex = 0; break;
	case 5:  BoxIndex = 1; break;
	case 6:  BoxIndex = 2; break;
	case 8:  BoxIndex = 3; break;
	case 9:  BoxIndex = 4; break;
	case 10: BoxIndex = 5; break;
	default: break;
	}

	const FLinearColor PuckColor = (bPuckOn && BoxIndex != INDEX_NONE) ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(0.02f, 0.02f, 0.02f);
	if (bPuckOn && PointBoxCenters.IsValidIndex(BoxIndex))
	{
		const FVector2D P = PointBoxCenters[BoxIndex];
		DealerPuckMesh->SetRelativeLocation(FVector(P.X, P.Y, 3.0f));
	}
	else
	{
		// Parked in the South Don't Come Bar when OFF (U = 165, Y = 180 -> X = 65, Y = 180)
		DealerPuckMesh->SetRelativeLocation(FVector(65.0f, 180.0f, 3.0f));
	}

	if (UMaterialInstanceDynamic* MID = GetColorMID(PuckColor))
	{
		DealerPuckMesh->SetMaterial(0, MID);
	}
	if (DealerPuckText)
	{
		const bool bIsOn = bPuckOn && BoxIndex != INDEX_NONE;
		DealerPuckText->SetText(FText::FromString(bIsOn ? TEXT("ON") : TEXT("OFF")));
		DealerPuckText->SetTextRenderColor(bIsOn ? FColor::Black : FColor::White);
	}
}

void ACrapsTableGrayboxActor::BuildTubDressing()
{
	const FLinearColor Leather(0.18f, 0.04f, 0.02f);
	const FLinearColor Groove(0.005f, 0.005f, 0.008f);
	const FLinearColor Tan(0.35f, 0.17f, 0.05f);

	// Flush padded leather front rail cap on the South Wall (X = -135, Y = 0, Z = 126, thickness 0.14)
	AddLayoutMesh(CachedCubeMesh, FVector(-135.0f, 0.0f, 126.0f), FRotator::ZeroRotator, FVector(0.70f, 8.8f, 0.14f), Leather);

	// Wooden chip rack grooves flush on the outer edge of the Z = 126cm rails (Y = ±460)
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 g = 0; g < 2; ++g)
		{
			AddLayoutMesh(CachedCubeMesh, FVector(720.0f, Side * (455.0f + g * 12.0f), 127.0f), FRotator::ZeroRotator, FVector(14.0f, 0.07f, 0.03f), Groove);
		}
	}

	// Stickman's rattan stick reaching from (720, -370, 15) into the center of the table (720, -20, 3)
	{
		const FVector A(720.0f, -370.0f, 15.0f);
		const FVector B(720.0f, -20.0f, 3.0f);
		const FVector Dir = B - A;
		AddLayoutMesh(CachedCylinderMesh, (A + B) * 0.5f, FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator(), FVector(0.03f, 0.03f, Dir.Size() / 100.0f), Tan);
	}

	// Chip bank stacks neatly along Y = +340 (X = 610 .. 830) on the Boxman / Dealer Side
	static const FLinearColor ChipColors[4] = {
		FLinearColor(1.0f, 0.72f, 0.01f), FLinearColor(0.0f, 0.96f, 0.83f), FLinearColor(1.0f, 0.16f, 0.52f), FLinearColor(0.9f, 0.92f, 1.0f)
	};
	for (int32 i = 0; i < 8; ++i)
	{
		const float Height = 14.0f + (i % 4) * 5.0f;
		AddLayoutMesh(CachedCylinderMesh, FVector(610.0f + i * 31.0f, 340.0f, Height * 0.5f), FRotator::ZeroRotator, FVector(0.26f, 0.26f, Height / 100.0f), ChipColors[i % 4]);
	}
}

void ACrapsTableGrayboxActor::BuildCRTScoreboard()
{
	if (!CachedScoreboardMesh)
	{
		return;
	}

	// If a standalone CRT_Scoreboard_Enclosure StaticMeshActor is already placed in the level, bind to its text actors
	// so we never spawn a duplicate mesh and live PIE gameplay still drives the 4 CRT readouts.
	if (UWorld* World = GetWorld())
	{
		bool bFoundLevelScoreboard = false;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || Actor == this)
			{
				continue;
			}
			if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Enclosure")))
			{
				bFoundLevelScoreboard = true;
			}
			else if (UTextRenderComponent* TextComp = Actor->FindComponentByClass<UTextRenderComponent>())
			{
				if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Bankroll")))
				{
					BankrollValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Ante")))
				{
					AnteValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Unbanked")))
				{
					UnbankedPoolValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Val_Mult")))
				{
					MultiplierValueText = TextComp;
				}
				else if (Actor->ActorHasTag(TEXT("CRT_Scoreboard_Hdr_Mult")))
				{
					MultiplierSubText = TextComp;
				}
			}
		}

		if (bFoundLevelScoreboard)
		{
			// Still build the two dark-steel & brass support pylons rising from the North Back Rail into the enclosure mounts
			const FLinearColor DarkSteel(0.022f, 0.025f, 0.032f);
			const FLinearColor TarnishedBrass(0.42f, 0.27f, 0.09f);
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float PylonY = Side * 182.0f;
				AddLayoutMesh(CachedCubeMesh, FVector(1572.0f, PylonY, 172.0f), FRotator::ZeroRotator, FVector(0.18f, 0.14f, 0.96f), DarkSteel);
				AddLayoutMesh(CachedCubeMesh, FVector(1572.0f, PylonY, 128.5f), FRotator::ZeroRotator, FVector(0.28f, 0.24f, 0.05f), TarnishedBrass);
			}
			return;
		}
	}

	// 1. Mount SM_CRT_Scoreboard_v1 above the North Back Wall (X = 1565, Y = 0, Z = 152, Yaw = 180, Pitch = -8 deg)
	// Its +X front face points -X toward the South Shooter stations, tilted 8 deg down toward the player.
	ScoreboardEnclosureMesh = NewObject<UStaticMeshComponent>(this, TEXT("CRTScoreboardEnclosureMesh"));
	ScoreboardEnclosureMesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	ScoreboardEnclosureMesh->SetStaticMesh(CachedScoreboardMesh);
	ScoreboardEnclosureMesh->SetupAttachment(TableRoot);
	ScoreboardEnclosureMesh->SetRelativeLocationAndRotation(
		FVector(1565.0f, 0.0f, 152.0f),
		FRotator(-8.0f, 180.0f, 0.0f));
	ScoreboardEnclosureMesh->SetRelativeScale3D(FVector(1.0f));
	ScoreboardEnclosureMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ScoreboardEnclosureMesh->SetCastShadow(true);
	ScoreboardEnclosureMesh->RegisterComponent();
	GeneratedLayoutComponents.Add(ScoreboardEnclosureMesh);

	// 2. Heavy Dark-Steel & Tarnished-Brass Support Pylons anchoring the scoreboard to the North Back Rail
	const FLinearColor DarkSteel(0.022f, 0.025f, 0.032f);
	const FLinearColor TarnishedBrass(0.42f, 0.27f, 0.09f);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const float PylonY = Side * 182.0f;
		// Vertical steel I-beam pylon rising from North Back Wall rail (Z = 126) up into the side trunnion mounts (Z = 225)
		AddLayoutMesh(CachedCubeMesh, FVector(1572.0f, PylonY, 172.0f), FRotator::ZeroRotator, FVector(0.18f, 0.14f, 0.96f), DarkSteel);
		// Brass base flange bolted into the North rail
		AddLayoutMesh(CachedCubeMesh, FVector(1572.0f, PylonY, 128.5f), FRotator::ZeroRotator, FVector(0.28f, 0.24f, 0.05f), TarnishedBrass);
	}

	// 3. Helper to create CRT phosphor text readouts attached directly to ScoreboardEnclosureMesh
	auto AddScreenReadout = [this](const FVector& LocalPos, const FString& InitialText, float WorldSize, const FColor& PhosphorColor) -> UTextRenderComponent*
	{
		UTextRenderComponent* Readout = NewObject<UTextRenderComponent>(this);
		Readout->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Readout->SetupAttachment(ScoreboardEnclosureMesh);
		Readout->SetText(FText::FromString(InitialText));
		Readout->SetWorldSize(WorldSize);
		Readout->SetTextRenderColor(PhosphorColor);
		Readout->SetHorizontalAlignment(EHTA_Center);
		Readout->SetVerticalAlignment(EVRTA_TextCenter);
		Readout->SetRelativeLocationAndRotation(LocalPos, FRotator::ZeroRotator);
		Readout->RegisterComponent();
		GeneratedLayoutComponents.Add(Readout);
		return Readout;
	};

	const FColor PhosphorCyan(0, 245, 212);
	const FColor HighVoltageAmber(255, 183, 3);
	const FColor NeonMagenta(255, 42, 133);
	const FColor WarmHeader(210, 175, 110);

	// --- Compartment 1: BANKROLL (Top-Left to Viewer -> Local Y = +81.0, Z = 99.0) ---
	AddScreenReadout(FVector(15.2f, 81.0f, 113.5f), TEXT("BANKROLL // SAFE CREDITS"), 7.5f, PhosphorCyan);
	BankrollValueText = AddScreenReadout(FVector(15.5f, 81.0f, 94.0f), TEXT("$ 500"), 24.0f, PhosphorCyan);

	// --- Compartment 2: ANTE QUOTA (Top-Right to Viewer -> Local Y = -83.0, Z = 98.5) ---
	AddScreenReadout(FVector(15.2f, -83.0f, 112.5f), TEXT("PIT BOSS ANTE // PUCK OFF"), 7.2f, HighVoltageAmber);
	AnteValueText = AddScreenReadout(FVector(15.5f, -83.0f, 93.5f), TEXT("$ 2,500"), 22.0f, HighVoltageAmber);

	// --- Compartment 3: UNBANKED POOL (Bottom-Left to Viewer -> Local Y = +96.0, Z = 40.0) ---
	AddScreenReadout(FVector(15.2f, 96.0f, 53.0f), TEXT("UNBANKED POOL [AT RISK]"), 6.5f, PhosphorCyan);
	UnbankedPoolValueText = AddScreenReadout(FVector(15.5f, 96.0f, 35.5f), TEXT("+ 0"), 20.5f, PhosphorCyan);

	// --- Compartment 4: MULTIPLIER CHAIN (Bottom-Right to Viewer -> Local Y = -68.0, Z = 40.0) ---
	MultiplierSubText = AddScreenReadout(FVector(15.2f, -68.0f, 53.0f), TEXT("CHIPS 0  x  THROW 1.0x  x  BET 1.0x"), 6.5f, WarmHeader);
	MultiplierValueText = AddScreenReadout(FVector(15.5f, -68.0f, 35.5f), TEXT("1.0x MULT"), 21.0f, NeonMagenta);
}

void ACrapsTableGrayboxActor::UpdateScoreboardReadouts(
	int64 Bankroll,
	int64 AnteQuota,
	int64 UnbankedPool,
	int32 BaseChips,
	float ThrowMult,
	float BetMult,
	ECrapsTablePhase Phase,
	int32 PointNumber)
{
	if (BankrollValueText)
	{
		BankrollValueText->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *FText::AsNumber(Bankroll).ToString())));
	}

	if (AnteValueText)
	{
		const FString PhaseTag = (Phase == ECrapsTablePhase::PointEstablished && PointNumber > 0)
			? FString::Printf(TEXT("  [PT %d]"), PointNumber)
			: TEXT("  [COME-OUT]");
		AnteValueText->SetText(FText::FromString(FString::Printf(TEXT("$ %s%s"), *FText::AsNumber(AnteQuota).ToString(), *PhaseTag)));
	}

	if (UnbankedPoolValueText)
	{
		UnbankedPoolValueText->SetText(FText::FromString(FString::Printf(TEXT("+ %s"), *FText::AsNumber(UnbankedPool).ToString())));
	}

	if (MultiplierValueText)
	{
		const float CombinedMult = ThrowMult * BetMult;
		MultiplierValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx MULT"), CombinedMult)));
	}

	if (MultiplierSubText)
	{
		MultiplierSubText->SetText(FText::FromString(FString::Printf(
			TEXT("CHIPS %d  x  THROW %.1fx  x  BET %.1fx"),
			BaseChips,
			ThrowMult,
			BetMult)));
	}
}

