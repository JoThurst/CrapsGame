#include "Pawn/CrapsPawn.h"
#include "Pawn/ThrowStyles/ThrowStyles_Concrete.h"
#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Table/CrapsFlyingChipActor.h"
#include "Progression/RunProgressionSubsystem.h"
#include "Items/TrinketInventoryComponent.h"
#include "Items/PDA_Trinket.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/CrapsDebugHUD.h"
#include "Hazards/TableHazard.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

ACrapsPawn::ACrapsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	PawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PawnRoot"));
	SetRootComponent(PawnRoot);

	// 10x Scale POV Shooter Camera positioned at the shooter's eye level looking down into the pit
	POVCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("POVCamera"));
	POVCamera->SetupAttachment(PawnRoot);
	POVCamera->SetRelativeLocation(FVector(25.0f, 0.0f, 175.0f));
	POVCamera->SetRelativeRotation(FRotator(-27.0f, 0.0f, 0.0f));
	POVCamera->FieldOfView = 82.0f;

	// Left-Hand Anchor resting visibly on the bottom-left leather rail in front of the camera (Milestone 3.1 & 3.2)
	LeftHandRailAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandRailAnchor"));
	LeftHandRailAnchor->SetupAttachment(PawnRoot);
	LeftHandRailAnchor->SetRelativeLocation(FVector(85.0f, -24.0f, 132.0f));
	LeftHandRailAnchor->SetRelativeRotation(FRotator(10.0f, 22.0f, -6.0f));

	// Right-Hand Shooter Anchor above the staging zone
	RightHandShooterAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("RightHandShooterAnchor"));
	RightHandShooterAnchor->SetupAttachment(PawnRoot);
	RightHandShooterAnchor->SetRelativeLocation(FVector(165.0f, 30.0f, 35.0f));
	RightHandShooterAnchor->SetRelativeRotation(FRotator(0.0f, -15.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CyberArmFinder(TEXT("/Game/Meshes/SM_CyberArm_Left.SM_CyberArm_Left"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WhiskeyTumblerFinder(TEXT("/Game/Meshes/SM_RailProp_WhiskeyTumbler.SM_RailProp_WhiskeyTumbler"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SignetRingFinder(TEXT("/Game/Meshes/SM_TrinketRing_Signet.SM_TrinketRing_Signet"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Legacy LeftHandVisualMesh kept hidden
	LeftHandVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHandVisualMesh"));
	LeftHandVisualMesh->SetupAttachment(LeftHandRailAnchor);
	LeftHandVisualMesh->SetVisibility(false);
	LeftHandVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const bool bHasCustomCyberArm = CyberArmFinder.Succeeded();

	// 1. Forearm (Sculpted Synth-Noir Cybernetic Left Arm)
	LeftForearmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftForearmMesh"));
	LeftForearmMesh->SetupAttachment(LeftHandRailAnchor);
	if (bHasCustomCyberArm)
	{
		LeftForearmMesh->SetStaticMesh(CyberArmFinder.Object);
		LeftForearmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		LeftForearmMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		LeftForearmMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}
	else
	{
		if (CubeFinder.Succeeded()) LeftForearmMesh->SetStaticMesh(CubeFinder.Object);
		LeftForearmMesh->SetRelativeLocation(FVector(-22.0f, -4.0f, 6.0f));
		LeftForearmMesh->SetRelativeScale3D(FVector(0.38f, 0.16f, 0.14f));
	}
	LeftForearmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. Wristwear Cyber-Chrono Cuff
	// Integrated into SM_CyberArm_Left when present; fallback cylinder is kept small and non-obstructive
	LeftWristMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWristMesh"));
	LeftWristMesh->SetupAttachment(LeftHandRailAnchor);
	if (CylinderFinder.Succeeded()) LeftWristMesh->SetStaticMesh(CylinderFinder.Object);
	LeftWristMesh->SetRelativeLocation(FVector(-2.0f, 0.0f, 4.0f));
	LeftWristMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
	LeftWristMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.015f));
	LeftWristMesh->SetVisibility(false);
	LeftWristMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. Palm resting flat on the table rail (fallback if no custom mesh)
	LeftPalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPalmMesh"));
	LeftPalmMesh->SetupAttachment(LeftHandRailAnchor);
	if (CubeFinder.Succeeded()) LeftPalmMesh->SetStaticMesh(CubeFinder.Object);
	LeftPalmMesh->SetRelativeLocation(FVector(6.0f, 0.0f, 0.0f));
	LeftPalmMesh->SetRelativeScale3D(FVector(0.18f, 0.16f, 0.07f));
	LeftPalmMesh->SetVisibility(!bHasCustomCyberArm);
	LeftPalmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. Fingers & Knuckle Rings
	const float FingerOffsetsY[4] = { 4.2f, 1.4f, -1.4f, -4.2f };
	const float FingerOffsetsX[4] = { 12.8f, 13.8f, 12.8f, 11.0f };
	const float FingerOffsetsZ[4] = { 2.8f, 3.0f, 2.8f, 2.4f };

	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* FingerMesh = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("LeftFingerMesh_%d"), i));
		FingerMesh->SetupAttachment(LeftHandRailAnchor);
		if (CylinderFinder.Succeeded()) FingerMesh->SetStaticMesh(CylinderFinder.Object);
		FingerMesh->SetRelativeLocation(FVector(FingerOffsetsX[i] + 4.0f, FingerOffsetsY[i], FingerOffsetsZ[i] - 1.5f));
		FingerMesh->SetRelativeRotation(FRotator(25.0f, 0.0f, 0.0f));
		FingerMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.10f));
		FingerMesh->SetVisibility(!bHasCustomCyberArm);
		FingerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftFingerMeshes.Add(FingerMesh);

		UStaticMeshComponent* RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("LeftRingMesh_%d"), i));
		RingMesh->SetupAttachment(LeftHandRailAnchor);
		if (SignetRingFinder.Succeeded())
		{
			RingMesh->SetStaticMesh(SignetRingFinder.Object);
			RingMesh->SetRelativeLocation(FVector(FingerOffsetsX[i] - 0.2f, FingerOffsetsY[i], FingerOffsetsZ[i] + 0.1f));
			RingMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
			RingMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
		else
		{
			if (CylinderFinder.Succeeded()) RingMesh->SetStaticMesh(CylinderFinder.Object);
			RingMesh->SetRelativeLocation(FVector(FingerOffsetsX[i], FingerOffsetsY[i], FingerOffsetsZ[i]));
			RingMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
			RingMesh->SetRelativeScale3D(FVector(0.045f, 0.045f, 0.025f));
		}
		RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftRingMeshes.Add(RingMesh);
	}

	// 5. Rail Prop (Whiskey tumbler resting neatly down on the drink rail armrest, clear of the hand)
	LeftRailPropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRailPropMesh"));
	LeftRailPropMesh->SetupAttachment(LeftHandRailAnchor);
	if (WhiskeyTumblerFinder.Succeeded())
	{
		LeftRailPropMesh->SetStaticMesh(WhiskeyTumblerFinder.Object);
		LeftRailPropMesh->SetRelativeLocation(FVector(10.0f, -38.0f, -2.0f));
		LeftRailPropMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
		LeftRailPropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}
	else
	{
		if (CylinderFinder.Succeeded()) LeftRailPropMesh->SetStaticMesh(CylinderFinder.Object);
		LeftRailPropMesh->SetRelativeLocation(FVector(10.0f, -38.0f, -2.0f));
		LeftRailPropMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.10f));
	}
	LeftRailPropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Visible Graybox Right Hand (Active Shooter Hand hovering beside the dice)
	RightHandVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHandVisualMesh"));
	RightHandVisualMesh->SetupAttachment(RightHandShooterAnchor);
	if (SphereFinder.Succeeded())
	{
		RightHandVisualMesh->SetStaticMesh(SphereFinder.Object);
	}
	RightHandVisualMesh->SetRelativeScale3D(FVector(0.50f, 0.40f, 0.22f));
	RightHandVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Synth-Noir Overhead Pit Spotlight so the table, hands, and 20cm dice are brightly illuminated in any map
	PitOverheadLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PitOverheadLight"));
	PitOverheadLight->SetupAttachment(PawnRoot);
	PitOverheadLight->SetRelativeLocation(FVector(550.0f, 0.0f, 650.0f));
	PitOverheadLight->Intensity = 65000.0f;
	PitOverheadLight->AttenuationRadius = 4500.0f;
	PitOverheadLight->LightColor = FColor(210, 250, 255);

	// Instantiate all 4 hot-swappable Throw Archetypes (F1-F4)
	ThrowStyle_TableSkimmer = CreateDefaultSubobject<UThrowStyle_TableSkimmer>(TEXT("ThrowStyle_TableSkimmer"));
	ThrowStyle_ParabolicLob = CreateDefaultSubobject<UThrowStyle_ParabolicLob>(TEXT("ThrowStyle_ParabolicLob"));
	ThrowStyle_BackWallWhip = CreateDefaultSubobject<UThrowStyle_BackWallWhip>(TEXT("ThrowStyle_BackWallWhip"));
	ThrowStyle_MechanicalPlunger = CreateDefaultSubobject<UThrowStyle_MechanicalPlunger>(TEXT("ThrowStyle_MechanicalPlunger"));

	// Left-Hand Trinket Inventory
	TrinketInventory = CreateDefaultSubobject<UTrinketInventoryComponent>(TEXT("TrinketInventory"));

	DieClass = ADieBase::StaticClass();

	EnableRemotePythonBridge();
}

void ACrapsPawn::EnableRemotePythonBridge()
{
	if (UClass* RCSettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/RemoteControlCommon.RemoteControlSettings")))
	{
		if (UObject* CDO = RCSettingsClass->GetDefaultObject())
		{
			if (FBoolProperty* RestrictProp = FindFProperty<FBoolProperty>(RCSettingsClass, TEXT("bRestrictServerAccess")))
			{
				RestrictProp->SetPropertyValue_InContainer(CDO, true);
			}
			if (FBoolProperty* PyProp = FindFProperty<FBoolProperty>(RCSettingsClass, TEXT("bEnableRemotePythonExecution")))
			{
				PyProp->SetPropertyValue_InContainer(CDO, true);
			}
		}
	}
}

void ACrapsPawn::BeginPlay()
{
	Super::BeginPlay();

	// Seat the pawn at the first shooter station of the table (falls back to origin when no table exists)
	if (UWorld* World = GetWorld())
	{
		TableActor = Cast<ACrapsTableGrayboxActor>(UGameplayStatics::GetActorOfClass(World, ACrapsTableGrayboxActor::StaticClass()));
		if (TableActor && TableActor->ShooterStations.Num() > 0)
		{
			ApplyShooterStation(0);
		}
		else if (TableActor)
		{
			SetActorLocationAndRotation(TableActor->GetActorLocation(), TableActor->GetActorRotation());
			EnsureDiceSpawned();
			RequestStickmanPushBack();
		}
		else
		{
			SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			EnsureDiceSpawned();
			RequestStickmanPushBack();
		}
	}

	// Configure PlayerController to show mouse cursor for Testing Pit interaction while capturing clicks
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	InitializeDefaultTrinketLoadout();

	if (TrinketInventory)
	{
		TrinketInventory->OnTrinketEquipped.AddDynamic(this, &ACrapsPawn::HandleTrinketEquippedVisual);
		TrinketInventory->OnTrinketUnequipped.AddDynamic(this, &ACrapsPawn::HandleTrinketUnequippedVisual);
	}
	UpdateTrinketVisuals();
}

void ACrapsPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent)
	{
		return;
	}

	// Direct key bindings so the Graybox Testing Pit works out-of-the-box without requiring manual InputAction assets
	// Stickman Recall / Reset
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACrapsPawn::RequestStickmanPushBack);

	// Cycle Setting Method (Q) & Select Left/Right/Both Die (E) & Cycle Throw Input Mode (Y)
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ACrapsPawn::CycleDiceSetMethod);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACrapsPawn::Input_SelectDieCycle);
	PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ACrapsPawn::CycleThrowInputMode);

	// Interactive Synth-Noir Betting Board Toggle (B & Tab)
	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &ACrapsPawn::Input_ToggleBettingBoard);
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ACrapsPawn::Input_ToggleBettingBoard);

	// On-Felt Finger-Tip 90-Degree Rotations (WASD / Arrow Keys / Z / C)
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ACrapsPawn::Input_PitchUp);
	PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ACrapsPawn::Input_PitchUp);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ACrapsPawn::Input_PitchDown);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ACrapsPawn::Input_PitchDown);
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ACrapsPawn::Input_YawLeft);
	PlayerInputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ACrapsPawn::Input_YawLeft);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ACrapsPawn::Input_YawRight);
	PlayerInputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ACrapsPawn::Input_YawRight);
	PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ACrapsPawn::Input_RollLeft);
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ACrapsPawn::Input_RollRight);

	// V-Set Locked-Axis Mouse Wheel Scroll
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ACrapsPawn::Input_MouseWheelUp);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ACrapsPawn::Input_MouseWheelDown);

	// Holographic Preset Snaps (V / K / N keys, freeing 1/2/3/4 for Throw Archetypes)
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ACrapsPawn::Input_Preset1);
	PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &ACrapsPawn::Input_Preset2);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &ACrapsPawn::Input_Preset3);

	// 4 Hot-Swappable Throw Archetypes bound to BOTH 1/2/3/4 AND NumPad 1/2/3/4 (and F1-F4 for standalone builds)
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF4);
	PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF1);
	PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF2);
	PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF3);
	PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF4);
	PlayerInputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF1);
	PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF2);
	PlayerInputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF3);
	PlayerInputComponent->BindKey(EKeys::F4, IE_Pressed, this, &ACrapsPawn::Input_ArchetypeF4);

	// Throw Charge & Release (Left Mouse Button or SpaceBar)
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACrapsPawn::Input_ThrowPress);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ACrapsPawn::Input_ThrowRelease);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ACrapsPawn::Input_RightClickPress);
	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ACrapsPawn::Input_CycleDenominationUp);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACrapsPawn::Input_ThrowPress);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &ACrapsPawn::Input_ThrowRelease);

	// Physics Rigging Slider ([ and ] keys) & Rigging Mode Toggle (G key)
	PlayerInputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &ACrapsPawn::Input_RiggingDecrease);
	PlayerInputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &ACrapsPawn::Input_RiggingIncrease);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &ACrapsPawn::ToggleRiggingMode);

	// Table Surface Friction Slider (- and = keys)
	PlayerInputComponent->BindKey(EKeys::Hyphen, IE_Pressed, this, &ACrapsPawn::Input_FrictionDecrease);
	PlayerInputComponent->BindKey(EKeys::Equals, IE_Pressed, this, &ACrapsPawn::Input_FrictionIncrease);

	// Milestone 2: Bank Unbanked Combo Pool (F key) & Re-seed Default Spread Bets (X key)
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ACrapsPawn::Input_BankPool);
	PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &ACrapsPawn::Input_RebetAll);

	// Milestone B1: Progression Advance (Enter key) & Restart Run (R key)
	PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACrapsPawn::Input_AdvanceAnte);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACrapsPawn::Input_RestartRun);

	// Toggle Debug HUD (H key)
	PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &ACrapsPawn::Input_ToggleHUDHelp);

	// Milestone 3: Toggle / Re-equip Starter Trinket Loadout (J key)
	PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &ACrapsPawn::Input_ToggleStarterTrinkets);

	// Clear uncommitted wagers back to bankroll (Backspace / Delete)
	PlayerInputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &ACrapsPawn::Input_ClearBets);
	PlayerInputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &ACrapsPawn::Input_ClearBets);

	// Shooter station POV cycling (PageUp/PageDown or M)
	PlayerInputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &ACrapsPawn::Input_StationNext);
	PlayerInputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &ACrapsPawn::Input_StationPrev);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ACrapsPawn::Input_StationNext);

	// Camera Free-Look / Glance Mode (Hold Middle Mouse Button or Left Alt)
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ACrapsPawn::Input_FreeLookPress);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ACrapsPawn::Input_FreeLookRelease);
	PlayerInputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &ACrapsPawn::Input_FreeLookPress);
	PlayerInputComponent->BindKey(EKeys::LeftAlt, IE_Released, this, &ACrapsPawn::Input_FreeLookRelease);
}

void ACrapsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ---- Free-Look Mouse Input Accumulation & Smoothing (MMB / Alt Glance) ----
	if (bIsFreeLooking)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			float MouseDeltaX = 0.0f;
			float MouseDeltaY = 0.0f;
			PC->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

			const float LookSensitivity = 0.55f;
			FreeLookYaw = FMath::Clamp(FreeLookYaw + (MouseDeltaX * LookSensitivity), -55.0f, 55.0f);
			// Inverted sign: Moving mouse forward/up pitches up; pulling back/down pitches down
			FreeLookPitch = FMath::Clamp(FreeLookPitch + (MouseDeltaY * LookSensitivity), -45.0f, 35.0f);
		}
	}
	else
	{
		// Smoothly and swiftly spring back to neutral perspective when glance key is released
		FreeLookYaw = FMath::FInterpTo(FreeLookYaw, 0.0f, DeltaTime, 12.0f);
		FreeLookPitch = FMath::FInterpTo(FreeLookPitch, 0.0f, DeltaTime, 12.0f);
	}

	// ---- Dynamic Look-Down / Pan / Zoom Camera Behavior (Task 3.2) ----
	if (POVCamera && TableActor && TableActor->ShooterStations.IsValidIndex(CurrentStationIndex))
	{
		const FCrapsShooterStation& Station = TableActor->ShooterStations[CurrentStationIndex];
		FRotator TargetCamRot = Station.SettingCameraLocalRot;
		float TargetFOV = 82.0f;

		switch (CurrentPhase)
		{
		case ECrapsThrowPhase::StickmanReturn:
		case ECrapsThrowPhase::OnFeltSetting:
		{
			// Natural first-person view of the table, near rail, and staged dice
			TargetCamRot = Station.SettingCameraLocalRot;
			TargetFOV = 82.0f;
			break;
		}

		case ECrapsThrowPhase::ThrowCharging:
		{
			// Pan toward the target back wall with aim yaw offset
			TargetCamRot = Station.ThrowCameraLocalRot + FRotator(0.0f, CurrentLaunchParams.AimYawOffsetDegrees, 0.0f);
			TargetFOV = 82.0f;
			break;
		}

		case ECrapsThrowPhase::DiceInFlight:
		case ECrapsThrowPhase::RollResolved:
		{
			// Follow dice midpoint and zoom in
			if (ActiveDice.Num() >= 2 && ActiveDice[0] && ActiveDice[1])
			{
				const FVector DiceMidpoint = 0.5f * (ActiveDice[0]->GetActorLocation() + ActiveDice[1]->GetActorLocation());
				const FVector CamWorldPos = POVCamera->GetComponentLocation();
				const FRotator WorldLookAt = (DiceMidpoint - CamWorldPos).Rotation();
				// Convert to pawn-local rotation
				TargetCamRot = (GetActorQuat().Inverse() * WorldLookAt.Quaternion()).Rotator();
			}
			TargetFOV = 48.0f;
			break;
		}
		}

		// Apply Free-Look Glance offset
		TargetCamRot += FRotator(FreeLookPitch, FreeLookYaw, 0.0f);

		const float InterpSpeed = (CurrentPhase == ECrapsThrowPhase::DiceInFlight) ? 7.5f : 8.0f;
		const float FOVInterpSpeed = (CurrentPhase == ECrapsThrowPhase::DiceInFlight) ? 4.5f : 8.0f;
		POVCamera->SetRelativeRotation(FMath::RInterpTo(POVCamera->GetRelativeRotation(), TargetCamRot, DeltaTime, InterpSpeed));
		POVCamera->FieldOfView = FMath::FInterpTo(POVCamera->FieldOfView, TargetFOV, DeltaTime, FOVInterpSpeed);
	}

	switch (CurrentPhase)
	{
	case ECrapsThrowPhase::StickmanReturn:
		UpdateStickmanPushBack(DeltaTime);
		break;

	case ECrapsThrowPhase::OnFeltSetting:
		UpdateOnFeltDiceRotationSlerp(DeltaTime);
		UpdateThrowChargingAndTrajectory(DeltaTime);
		UpdateCursorHoverRaycast();
		break;

	case ECrapsThrowPhase::ThrowCharging:
		UpdateOnFeltDiceRotationSlerp(DeltaTime);
		UpdateThrowChargingAndTrajectory(DeltaTime);
		break;

	case ECrapsThrowPhase::DiceInFlight:
	{
		FlightTimeoutTimer += DeltaTime;
		const FVector PawnOrigin = GetActorLocation();
		bool bAllSettled = (ActiveDice.Num() >= 2);
		for (ADieBase* Die : ActiveDice)
		{
			if (!Die)
			{
				continue;
			}

			const FTransform TableXf = TableActor ? TableActor->GetActorTransform() : FTransform(GetActorRotation(), PawnOrigin);
			FVector LocalLoc = TableXf.InverseTransformPosition(Die->GetActorLocation());
			MaxThrowDistanceReached = FMath::Max(MaxThrowDistanceReached, LocalLoc.X);

			// Anti-Wall-Stick Repulsor (Task 1.3):
			// Only apply after initial throw launch (FlightTimeoutTimer > 0.6s) to settled/slow dice resting against perimeter walls
			if (FlightTimeoutTimer > 0.6f && Die->DieMesh && Die->DieMesh->IsSimulatingPhysics())
			{
				const bool bNearPerimeter = (LocalLoc.X > 1500.0f || LocalLoc.X < -60.0f || FMath::Abs(LocalLoc.Y) > 375.0f);
				if (bNearPerimeter)
				{
					const float Speed = Die->DieMesh->GetPhysicsLinearVelocity().Size();
					float TopFaceDot = 1.0f;
					Die->GetTopFaceDirection(TopFaceDot);

					if ((Speed > 5.0f && Speed < 90.0f) || (Speed < 10.0f && TopFaceDot < 0.85f))
					{
						const float TargetCenterX = (LocalLoc.X < 720.0f) ? 350.0f : 1090.0f;
						const FVector2D TowardCenter = FVector2D(TargetCenterX - LocalLoc.X, -LocalLoc.Y).GetSafeNormal();
						const FVector PushVelocityTable(TowardCenter.X * 140.0f, TowardCenter.Y * 140.0f, 25.0f);
						Die->DieMesh->AddImpulse(TableXf.TransformVectorNoScale(PushVelocityTable), NAME_None, true);
					}
				}
			}

			// Safety containment net (table-local: bounds widened so it only catches true out-of-bounds tunneling)
			FVector LocalVelDelta = FVector::ZeroVector;
			bool bClamped = false;
			if (LocalLoc.X > 1580.0f) { LocalLoc.X = 1530.0f; LocalVelDelta.X = -1.0f; bClamped = true; }
			if (LocalLoc.X < -140.0f) { LocalLoc.X = -90.0f;  LocalVelDelta.X = 1.0f;  bClamped = true; }
			if (LocalLoc.Y > 450.0f)  { LocalLoc.Y = 400.0f;  LocalVelDelta.Y = -1.0f; bClamped = true; }
			if (LocalLoc.Y < -450.0f) { LocalLoc.Y = -400.0f; LocalVelDelta.Y = 1.0f;  bClamped = true; }
			if (LocalLoc.Z < 9.5f)    { LocalLoc.Z = 10.5f;   bClamped = true; }
			if (bClamped && Die->DieMesh)
			{
				Die->SetActorLocation(TableXf.TransformPosition(LocalLoc), false, nullptr, ETeleportType::TeleportPhysics);
				FVector LocalVel = TableXf.InverseTransformVectorNoScale(Die->DieMesh->GetPhysicsLinearVelocity());
				if (LocalVelDelta.X != 0.0f) { LocalVel.X = LocalVelDelta.X * FMath::Abs(LocalVel.X) * 0.45f; }
				if (LocalVelDelta.Y != 0.0f) { LocalVel.Y = LocalVelDelta.Y * FMath::Abs(LocalVel.Y) * 0.45f; }
				Die->DieMesh->SetPhysicsLinearVelocity(TableXf.TransformVectorNoScale(LocalVel));
			}

			if (!Die->HasSettled())
			{
				bAllSettled = false;
			}
		}

		// Settle resolution or timeout triggers
		if (bAllSettled || FlightTimeoutTimer >= 5.5f)
		{
			// 1. Timeout failsafe check: if timer reached 5.5s and dice never properly settled, declare No-Roll
			if (!bAllSettled)
			{
				HandleNoRollThrow(TEXT("Dice Unresolved / Flight Timeout"));
				break;
			}

			// 2. Cocked dice check: ensure neither die is resting at an illegal angle on a wall/obstacle
			bool bAnyCocked = false;
			for (ADieBase* Die : ActiveDice)
			{
				if (Die)
				{
					float Dot = 1.0f;
					Die->GetTopFaceDirection(Dot);
					if (Die->IsCocked() || Dot < Die->CockedAlignmentThreshold)
					{
						bAnyCocked = true;
						break;
					}
				}
			}
			if (bAnyCocked)
			{
				HandleNoRollThrow(TEXT("Cocked Dice"));
				break;
			}

			// 3. Short throw check: dice must travel past mid-table into the far half of the table
			if (MaxThrowDistanceReached < ShortThrowMinTableX)
			{
				HandleNoRollThrow(TEXT("Short Throw — Failed to cross mid-table"));
				break;
			}

			// 4. Valid roll: all dice cleanly settled, not cocked, and passed distance threshold
			CurrentPhase = ECrapsThrowPhase::RollResolved;
			if (ActiveDice.Num() >= 2 && ActiveDice[0] && ActiveDice[1])
			{
				LastDie1Value = ActiveDice[0]->GetTopFaceValue();
				LastDie2Value = ActiveDice[1]->GetTopFaceValue();
				LastRollSum = LastDie1Value + LastDie2Value;
				OnRollResolved.Broadcast(LastDie1Value, LastDie2Value, LastRollSum);

				if (UWorld* World = GetWorld())
				{
					if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
					{
						const int32 TotalBounces = ActiveDice[0]->GetCurrentThrowBounceCount() + ActiveDice[1]->GetCurrentThrowBounceCount();
						const FTransform TableXf = TableActor ? TableActor->GetActorTransform() : FTransform::Identity;
						const FVector WallTarget = (TableActor && TableActor->ShooterStations.IsValidIndex(CurrentStationIndex))
							? TableXf.TransformPosition(TableActor->ShooterStations[CurrentStationIndex].TargetBackWallLocalCenter)
							: GetActorLocation() + GetActorForwardVector() * 1540.0f;
						const bool bHitFarEnd = (FVector::Dist2D(ActiveDice[0]->GetActorLocation(), WallTarget) < 400.0f)
							|| (FVector::Dist2D(ActiveDice[1]->GetActorLocation(), WallTarget) < 400.0f)
							|| (TotalBounces >= 3);
						GS->ResolveSettledRoll(
							LastDie1Value,
							LastDie1Value * 10,
							ActiveDice[0]->GetActorLocation(),
							LastDie2Value,
							LastDie2Value * 10,
							ActiveDice[1]->GetActorLocation(),
							bHitFarEnd,
							TotalBounces);
					}
				}
			}
		}
		break;
	}

	case ECrapsThrowPhase::RollResolved:
		break;
	}

	// Render & Animate Two-Handed POV Synth-Noir Vector Hands (Left Anchor + Right Active Shooter Rig)
	// All offsets are pawn-local (+X = throw direction) so every shooter station renders correctly.
	if (UWorld* World = GetWorld())
	{
		const FQuat PawnQ = GetActorQuat();
		const float ChargePull = bIsChargingThrow ? CurrentLaunchParams.NormalizedPower : 0.0f;
		const float AimYaw = CurrentLaunchParams.AimYawOffsetDegrees;
		const FVector Fwd = PawnQ.GetForwardVector();
		const FVector Right = PawnQ.GetRightVector();

		// 1. LEFT HAND ANCHOR (Bottom-Left Rail): 3D Mesh Cyber-Rig (Milestone 3.1 & 3.2)
		const FQuat LeftHandQ = LeftHandRailAnchor ? LeftHandRailAnchor->GetComponentQuat() : (PawnQ * FQuat(FRotator(0.0f, 15.0f, 0.0f)));
		const FVector LeftWristPos = GetWristWorldLocation();

		// Optional socket glow highlights for equipped Trinkets
		if (TrinketInventory)
		{
			// Wristwear
			if (UPDA_Trinket* WristTrinket = TrinketInventory->GetEquippedTrinket(ETrinketSlotType::WristwearActive, 0))
			{
				DrawDebugSphere(World, LeftWristPos + FVector(0.0f, 0.0f, 4.0f), 6.0f, 16, FColor(255, 215, 0), false, -1.0f, 0, 2.0f);
			}

			// 4 Finger Rings
			for (int32 FingerIdx = 0; FingerIdx < 4; ++FingerIdx)
			{
				if (UPDA_Trinket* RingTrinket = TrinketInventory->GetEquippedTrinket(ETrinketSlotType::FingerRing, FingerIdx))
				{
					const FVector KnucklePos = GetFingerKnuckleWorldLocation(FingerIdx);
					DrawDebugSphere(World, KnucklePos, 4.0f, 12, FColor(0, 245, 212), false, -1.0f, 0, 2.0f);
				}
			}

			// 2 Rail Props
			for (int32 PropIdx = 0; PropIdx < 2; ++PropIdx)
			{
				if (UPDA_Trinket* PropTrinket = TrinketInventory->GetEquippedTrinket(ETrinketSlotType::RailProp, PropIdx))
				{
					const FVector PropPos = GetRailPropWorldLocation(PropIdx);
					DrawDebugBox(World, PropPos, FVector(5.0f, 5.0f, 6.0f), LeftHandQ, FColor(255, 0, 128), false, -1.0f, 0, 2.0f);
				}
			}
		}

		// 2. RIGHT HAND ACTIVE SHOOTER RIG (transforms per archetype and pulls back with throw power)
		const FVector StagingCenter = PawnLocalToWorld(ShooterStagingWorldPos + FVector(0.0f, AimYaw * 2.5f, 0.0f));
		const FQuat AimQ = PawnQ * FQuat(FRotator(0.0f, AimYaw, 0.0f));

		switch (ActiveThrowArchetype)
		{
		case EThrowArchetype::TableSkimmer:
		{
			const FVector PusherPos = StagingCenter + Fwd * (-26.0f - (ChargePull * 65.0f)) + FVector(0.0f, 0.0f, 4.0f);
			DrawDebugBox(World, PusherPos, FVector(8.0f, 38.0f, 10.0f), AimQ, FColor(0, 245, 212), false, -1.0f, 0, 3.0f);
			DrawDebugString(World, PusherPos + FVector(0.0f, 0.0f, 22.0f), TEXT("[1] SKIMMER BLADE"), nullptr, FColor(0, 245, 212), 0.0f, true, 1.0f);
			break;
		}
		case EThrowArchetype::ParabolicLob:
		{
			const FVector CradlePos = StagingCenter + Fwd * (-18.0f - (ChargePull * 45.0f)) + FVector(0.0f, 0.0f, 32.0f + (ChargePull * 35.0f));
			DrawDebugSphere(World, CradlePos, 24.0f, 14, FColor(255, 183, 3), false, -1.0f, 0, 2.5f);
			DrawDebugCircle(World, CradlePos, 32.0f, 24, FColor(255, 0, 127), false, -1.0f, 0, 2.5f, Fwd, Right, false);
			DrawDebugString(World, CradlePos + FVector(0.0f, 0.0f, 28.0f), TEXT("[2] HELI-LOB CRADLE"), nullptr, FColor(255, 183, 3), 0.0f, true, 1.0f);
			break;
		}
		case EThrowArchetype::BackWallWhip:
		{
			const FVector WhipPos = StagingCenter + Fwd * (-30.0f - (ChargePull * 85.0f)) + Right * 42.0f + FVector(0.0f, 0.0f, 18.0f);
			DrawDebugCone(World, WhipPos, -Fwd, 55.0f, 0.35f, 0.35f, 12, FColor(255, 30, 66), false, -1.0f, 0, 2.8f);
			DrawDebugString(World, WhipPos + FVector(0.0f, 0.0f, 26.0f), TEXT("[3] FASTBALL WHIP"), nullptr, FColor(255, 30, 66), 0.0f, true, 1.0f);
			break;
		}
		case EThrowArchetype::MechanicalPlunger:
		{
			const FVector PlungerHead = StagingCenter + Fwd * (-22.0f - (ChargePull * 75.0f)) + FVector(0.0f, 0.0f, 6.0f);
			const FVector PlungerBase = StagingCenter + Fwd * -115.0f + FVector(0.0f, 0.0f, 6.0f);
			DrawDebugBox(World, PlungerHead, FVector(6.0f, 34.0f, 12.0f), PawnQ, FColor(255, 183, 3), false, -1.0f, 0, 3.2f);
			DrawDebugCylinder(World, PlungerBase, PlungerHead, 8.0f, 12, FColor(0, 245, 212), false, -1.0f, 0, 2.5f);
			DrawDebugString(World, PlungerHead + FVector(0.0f, 0.0f, 24.0f), TEXT("[4] MECH PLUNGER"), nullptr, FColor(255, 183, 3), 0.0f, true, 1.0f);
			break;
		}
		}
	}
}

void ACrapsPawn::EnsureDiceSpawned()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ActiveDice.RemoveAll([](const TObjectPtr<ADieBase>& D) { return D == nullptr; });

	UClass* SpawnClass = DieClass ? *DieClass : ADieBase::StaticClass();
	while (ActiveDice.Num() < 2)
	{
		const int32 Index = ActiveDice.Num();
		const float LateralOffset = (Index == 0 ? -0.5f : 0.5f) * DiceLateralSpacingCm;
		const FVector SpawnLoc = PawnLocalToWorld(ShooterStagingWorldPos + FVector(0.0f, LateralOffset, 0.0f));

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (ADieBase* NewDie = World->SpawnActor<ADieBase>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams))
		{
			NewDie->OnDieSettled.AddDynamic(this, &ACrapsPawn::HandleSingleDieSettled);
			ActiveDice.Add(NewDie);
		}
	}

	DesiredDieOrientations.SetNum(ActiveDice.Num());
	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		if (ActiveDice[i])
		{
			DesiredDieOrientations[i] = GetActorQuat().Inverse() * ActiveDice[i]->GetActorQuat();
		}
	}

	// Link PartnerDie references for AvoidSevenOut rigging mode
	if (ActiveDice.Num() >= 2 && ActiveDice[0] && ActiveDice[1])
	{
		if (ActiveDice[0]->DiceRiggingComponent)
		{
			ActiveDice[0]->DiceRiggingComponent->PartnerDie = ActiveDice[1];
		}
		if (ActiveDice[1]->DiceRiggingComponent)
		{
			ActiveDice[1]->DiceRiggingComponent->PartnerDie = ActiveDice[0];
		}
	}

	SyncRiggingSettingsToDice();
}

void ACrapsPawn::SyncRiggingSettingsToDice()
{
	for (ADieBase* Die : ActiveDice)
	{
		if (Die && Die->DiceRiggingComponent)
		{
			Die->DiceRiggingComponent->DiceControlStat = GlobalDiceControlStat;
			Die->DiceRiggingComponent->RiggingMode = GlobalRiggingMode;
		}
	}
}

void ACrapsPawn::RequestStickmanPushBack()
{
	EnsureDiceSpawned();

	const bool bWasInFlight = (CurrentPhase == ECrapsThrowPhase::DiceInFlight);
	if (bWasInFlight)
	{
		if (UWorld* World = GetWorld())
		{
			if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
			{
				GS->HandleNoRoll(TEXT("Shooter Recalled Dice Mid-Throw"));
			}
			for (TActorIterator<ATableHazard> It(World); It; ++It)
			{
				if (ATableHazard* Hazard = *It)
				{
					Hazard->ResetForNewThrow();
				}
			}
		}
	}
	else
	{
		// Auto-bank unbanked combo winnings before the dice are returned to the shooter
		if (UWorld* World = GetWorld())
		{
			if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
			{
				if (GS->bAutoBankOnNextRoll && GS->GetUnbankedPool() > 0)
				{
					GS->BankUnbankedPool();
				}
			}
		}
	}

	bIsChargingThrow = false;
	StickmanPushElapsed = 0.0f;
	FlightTimeoutTimer = 0.0f;
	MaxThrowDistanceReached = 0.0f;
	CurrentPhase = ECrapsThrowPhase::StickmanReturn;

	// Reset camera FOV to 75.0f look-down on push-back
	if (POVCamera)
	{
		POVCamera->FieldOfView = 75.0f;
		if (TableActor && TableActor->ShooterStations.IsValidIndex(CurrentStationIndex))
		{
			POVCamera->SetRelativeRotation(TableActor->ShooterStations[CurrentStationIndex].SettingCameraLocalRot);
		}
	}

	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		if (ADieBase* Die = ActiveDice[i])
		{
			Die->EnterOnFeltSettingState();
			const float LateralOffset = (i == 0 ? -0.5f : 0.5f) * DiceLateralSpacingCm;
			Die->SetActorLocation(PawnLocalToWorld(StickmanOriginWorldPos + FVector(0.0f, LateralOffset, 0.0f)));
			DesiredDieOrientations[i] = GetActorQuat().Inverse() * Die->GetActorQuat();
		}
	}
}

void ACrapsPawn::HandleNoRollThrow(const FString& Reason)
{
	FlightTimeoutTimer = 0.0f;
	MaxThrowDistanceReached = 0.0f;

	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			GS->HandleNoRoll(Reason);
		}
		for (TActorIterator<ATableHazard> It(World); It; ++It)
		{
			if (ATableHazard* Hazard = *It)
			{
				Hazard->ResetForNewThrow();
			}
		}
	}

	for (ADieBase* Die : ActiveDice)
	{
		if (Die)
		{
			Die->EnterOnFeltSettingState();
		}
	}

	RequestStickmanPushBack();
}

void ACrapsPawn::UpdateStickmanPushBack(float DeltaTime)
{
	StickmanPushElapsed += DeltaTime;
	const float RawAlpha = FMath::Clamp(StickmanPushElapsed / FMath::Max(0.05f, StickmanPushDuration), 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseOut(0.0f, 1.0f, RawAlpha, 2.5f);

	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		if (ADieBase* Die = ActiveDice[i])
		{
			const float LateralOffset = (i == 0 ? -0.5f : 0.5f) * DiceLateralSpacingCm;
			const FVector StartPos = PawnLocalToWorld(StickmanOriginWorldPos + FVector(0.0f, LateralOffset, 0.0f));
			const FVector EndPos = PawnLocalToWorld(ShooterStagingWorldPos + FVector(0.0f, LateralOffset, 0.0f));
			Die->SetActorLocation(FMath::Lerp(StartPos, EndPos, SmoothAlpha));
		}
	}

	if (RawAlpha >= 1.0f)
	{
		CurrentPhase = ECrapsThrowPhase::OnFeltSetting;
	}
}

void ACrapsPawn::UpdateOnFeltDiceRotationSlerp(float DeltaTime)
{
	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		if (ADieBase* Die = ActiveDice[i])
		{
			const FQuat CurrentQuat = Die->GetActorQuat();
			const FQuat TargetQuat = DesiredDieOrientations.IsValidIndex(i) ? (GetActorQuat() * DesiredDieOrientations[i]) : CurrentQuat;
			const FQuat NewQuat = FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, DiceSetRotationInterpSpeed);
			Die->SetActorRotation(NewQuat);
		}
	}
}

void ACrapsPawn::RotateDiceOnFelt90Degrees(const FVector& WorldAxis, float AngleSign)
{
	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting && CurrentPhase != ECrapsThrowPhase::ThrowCharging)
	{
		return;
	}

	const FQuat DeltaRot(WorldAxis.GetSafeNormal(), FMath::DegreesToRadians(90.0f * AngleSign));

	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		const bool bShouldRotate = (SelectedDieIndex == 2) || (SelectedDieIndex == i);
		if (bShouldRotate && DesiredDieOrientations.IsValidIndex(i))
		{
			DesiredDieOrientations[i] = (DeltaRot * DesiredDieOrientations[i]).GetNormalized();
		}
	}
}

void ACrapsPawn::ScrollVSetAxis(float ScrollDirection)
{
	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting && CurrentPhase != ECrapsThrowPhase::ThrowCharging)
	{
		return;
	}

	ActiveDiceSetMethod = EDiceSetMethod::VSetScroll;
	const FQuat AxialPitchStep(FVector::RightVector, FMath::DegreesToRadians(90.0f * FMath::Sign(ScrollDirection)));

	for (int32 i = 0; i < DesiredDieOrientations.Num(); ++i)
	{
		DesiredDieOrientations[i] = (AxialPitchStep * DesiredDieOrientations[i]).GetNormalized();
	}
}

void ACrapsPawn::ApplyPresetDiceSnap(int32 PresetIndex)
{
	if (CurrentPhase == ECrapsThrowPhase::DiceInFlight)
	{
		return;
	}

	if (CurrentPhase == ECrapsThrowPhase::RollResolved)
	{
		RequestStickmanPushBack();
	}

	ActiveDiceSetMethod = EDiceSetMethod::PresetSnap;
	EnsureDiceSpawned();

	if (DesiredDieOrientations.Num() < 2)
	{
		return;
	}

	switch (PresetIndex)
	{
	case 1:
		// Preset 1: Hardways Set (Both dice 4 on top [-Y], 2 on front [+X])
		DesiredDieOrientations[0] = ADieBase::MakeDieOrientation(EDieFaceDirection::MinusY, EDieFaceDirection::PlusX, 0.0f);
		DesiredDieOrientations[1] = ADieBase::MakeDieOrientation(EDieFaceDirection::MinusY, EDieFaceDirection::PlusX, 0.0f);
		break;

	case 2:
		// Preset 2: All-Sevens / Come-Out Natural Set (Left die 3 on top [+Y], Right die 4 on top [-Y], 2 & 5 front)
		DesiredDieOrientations[0] = ADieBase::MakeDieOrientation(EDieFaceDirection::PlusY, EDieFaceDirection::PlusX, 0.0f);
		DesiredDieOrientations[1] = ADieBase::MakeDieOrientation(EDieFaceDirection::MinusY, EDieFaceDirection::MinusX, 0.0f);
		break;

	case 3:
	default:
		// Preset 3: 3-V Point-Hunter Set (Both dice 3 on top [+Y], V-angled outward by +/-15 deg)
		DesiredDieOrientations[0] = ADieBase::MakeDieOrientation(EDieFaceDirection::PlusY, EDieFaceDirection::PlusX, -12.0f);
		DesiredDieOrientations[1] = ADieBase::MakeDieOrientation(EDieFaceDirection::PlusY, EDieFaceDirection::MinusZ, 12.0f);
		break;
	}
}

void ACrapsPawn::SetActiveThrowArchetype(EThrowArchetype NewArchetype)
{
	ActiveThrowArchetype = NewArchetype;
}

void ACrapsPawn::CycleDiceSetMethod()
{
	const uint8 Next = (static_cast<uint8>(ActiveDiceSetMethod) + 1) % 3;
	ActiveDiceSetMethod = static_cast<EDiceSetMethod>(Next);
}

void ACrapsPawn::CycleThrowInputMode()
{
	const uint8 Next = (static_cast<uint8>(ActiveInputMode) + 1) % 3;
	ActiveInputMode = static_cast<EThrowInputMode>(Next);
	CurrentLaunchParams.InputMode = ActiveInputMode;
}

void ACrapsPawn::AdjustDiceControlStat(float Delta)
{
	GlobalDiceControlStat = FMath::Clamp(GlobalDiceControlStat + Delta, 0.0f, 1.0f);
	SyncRiggingSettingsToDice();
}

void ACrapsPawn::ToggleRiggingMode()
{
	GlobalRiggingMode = (GlobalRiggingMode == EDiceRiggingMode::TargetSetFace)
		? EDiceRiggingMode::AvoidSevenOut
		: EDiceRiggingMode::TargetSetFace;
	SyncRiggingSettingsToDice();
}

void ACrapsPawn::AdjustTableFriction(float Delta)
{
	TableSurfaceFriction = FMath::Clamp(TableSurfaceFriction + Delta, 0.0f, 1.0f);

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundTables;
		UGameplayStatics::GetAllActorsOfClass(World, ACrapsTableGrayboxActor::StaticClass(), FoundTables);
		for (AActor* Actor : FoundTables)
		{
			if (ACrapsTableGrayboxActor* Table = Cast<ACrapsTableGrayboxActor>(Actor))
			{
				Table->SetFeltSurfaceFriction(TableSurfaceFriction);
			}
		}
	}
}

UThrowStyleComponentBase* ACrapsPawn::GetActiveThrowStyleComponent() const
{
	switch (ActiveThrowArchetype)
	{
	case EThrowArchetype::TableSkimmer:      return ThrowStyle_TableSkimmer;
	case EThrowArchetype::ParabolicLob:      return ThrowStyle_ParabolicLob;
	case EThrowArchetype::BackWallWhip:      return ThrowStyle_BackWallWhip;
	case EThrowArchetype::MechanicalPlunger: return ThrowStyle_MechanicalPlunger;
	default:                                 return ThrowStyle_TableSkimmer;
	}
}

void ACrapsPawn::UpdateThrowChargingAndTrajectory(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	FVector2D CurrentMousePos = FVector2D::ZeroVector;
	if (PC)
	{
		PC->GetMousePosition(CurrentMousePos.X, CurrentMousePos.Y);
	}

	if (DeltaTime > KINDA_SMALL_NUMBER)
	{
		const FVector2D InstantVel = (CurrentMousePos - PrevMousePos) / DeltaTime;
		SmoothedMouseVelocity = FMath::Vector2DInterpTo(SmoothedMouseVelocity, InstantVel, DeltaTime, 18.0f);
	}
	PrevMousePos = CurrentMousePos;

	CurrentLaunchParams.InputMode = ActiveInputMode;

	if (bIsChargingThrow)
	{
		if (ActiveInputMode == EThrowInputMode::ArcadeMeter)
		{
			ArcadeMeterPhase += DeltaTime * 2.4f;
			// Ping-pong normalized power between 0.1 and 1.0
			CurrentLaunchParams.NormalizedPower = 0.1f + 0.9f * FMath::Abs(FMath::Sin(ArcadeMeterPhase));
			CurrentLaunchParams.bSweetSpotRelease = (CurrentLaunchParams.NormalizedPower >= 0.78f && CurrentLaunchParams.NormalizedPower <= 0.90f);
		}
		else if (ActiveInputMode == EThrowInputMode::SlingshotPull)
		{
			// Dragging mouse downward (positive Y screen delta) increases pull power; horizontal delta steers aim
			const FVector2D DragDelta = CurrentMousePos - ThrowPressMousePos;
			CurrentLaunchParams.NormalizedPower = FMath::Clamp(DragDelta.Y / 280.0f, 0.10f, 1.0f);
			CurrentLaunchParams.AimYawOffsetDegrees = FMath::Clamp(-DragDelta.X * 0.08f, -22.0f, 22.0f);
		}
		else if (ActiveInputMode == EThrowInputMode::MouseFlick)
		{
			// Forward mouse flick is negative Y screen velocity
			const float ForwardFlickSpeed = FMath::Max(0.0f, -SmoothedMouseVelocity.Y);
			CurrentLaunchParams.NormalizedPower = FMath::Clamp(ForwardFlickSpeed / 1800.0f, 0.15f, 1.0f);
			CurrentLaunchParams.AimYawOffsetDegrees = FMath::Clamp(SmoothedMouseVelocity.X * 0.012f, -22.0f, 22.0f);
			CurrentLaunchParams.LateralSpin = FMath::Clamp(SmoothedMouseVelocity.X / 1500.0f, -1.0f, 1.0f);
		}
	}

	// Draw live trajectory preview and Target Ring on the felt
	if (UThrowStyleComponentBase* StyleComp = GetActiveThrowStyleComponent())
	{
		TArray<FVector> TrajectoryPts;
		FVector LandingSpot = FVector::ZeroVector;
		const FVector StagingWorldStart = PawnLocalToWorld(ShooterStagingWorldPos);
		IThrowStyleInterface::Execute_ComputePreviewTrajectory(StyleComp, StagingWorldStart, CurrentLaunchParams, TrajectoryPts, LandingSpot);
		CurrentLaunchParams.TargetWorldLocation = LandingSpot;

		if (UWorld* World = GetWorld())
		{
			const FColor ArcColor = bIsChargingThrow ? FColor(255, 0, 127) : FColor(0, 245, 212);
			for (int32 i = 0; i + 1 < TrajectoryPts.Num(); ++i)
			{
				DrawDebugLine(World, TrajectoryPts[i], TrajectoryPts[i + 1], ArcColor, false, -1.0f, 0, 2.5f);
			}

			// Draw Target Ring on the felt at LandingSpot
			DrawDebugCircle(
				World,
				FVector(LandingSpot.X, LandingSpot.Y, GetActorLocation().Z + 2.0f),
				95.0f,
				36,
				FColor(255, 183, 3),
				false,
				-1.0f,
				0,
				3.5f,
				FVector(1.0f, 0.0f, 0.0f),
				FVector(0.0f, 1.0f, 0.0f),
				false
			);
		}
	}
}

void ACrapsPawn::CommitThrow()
{
	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting && CurrentPhase != ECrapsThrowPhase::ThrowCharging)
	{
		return;
	}

	// Safety check: ensure any unbanked pool is authoritatively secured before dice leave the shooter's hand
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			if (GS->bAutoBankOnNextRoll && GS->GetUnbankedPool() > 0)
			{
				GS->BankUnbankedPool();
			}
		}
	}

	EnsureDiceSpawned();
	SyncRiggingSettingsToDice();

	// Snap dice to exact DesiredDieOrientations right before physics handover
	for (int32 i = 0; i < ActiveDice.Num(); ++i)
	{
		if (ActiveDice[i] && DesiredDieOrientations.IsValidIndex(i))
		{
			ActiveDice[i]->SetActorRotation(GetActorQuat() * DesiredDieOrientations[i]);
		}
	}

	if (UThrowStyleComponentBase* StyleComp = GetActiveThrowStyleComponent())
	{
		// Reset sequential scoring queue and table hazards for the new throw
		if (UWorld* World = GetWorld())
		{
			if (UScoringQueueSubsystem* ScoringQueue = World->GetSubsystem<UScoringQueueSubsystem>())
			{
				ScoringQueue->BeginNewThrowScoringSession();
			}

			for (TActorIterator<ATableHazard> It(World); It; ++It)
			{
				if (ATableHazard* Hazard = *It)
				{
					Hazard->ResetForNewThrow();
				}
			}
		}

		TArray<ADieBase*> RawDice;
		for (ADieBase* Die : ActiveDice)
		{
			if (Die)
			{
				RawDice.Add(Die);
			}
		}

		FlightTimeoutTimer = 0.0f;
		MaxThrowDistanceReached = 0.0f;
		CurrentPhase = ECrapsThrowPhase::DiceInFlight;
		IThrowStyleInterface::Execute_ExecuteThrow(StyleComp, CurrentLaunchParams, RawDice);
	}
}

void ACrapsPawn::HandleSingleDieSettled(ADieBase* Die, EDieFaceDirection TopDirection, int32 TopValue, UPDA_DiceFace* TopFaceData)
{
	// Handled in Tick once both dice have settled
}

// ============================================================================
// Input Callbacks
// ============================================================================
void ACrapsPawn::Input_PitchUp()    { RotateDiceOnFelt90Degrees(FVector::RightVector, 1.0f); }
void ACrapsPawn::Input_PitchDown()  { RotateDiceOnFelt90Degrees(FVector::RightVector, -1.0f); }
void ACrapsPawn::Input_YawLeft()    { RotateDiceOnFelt90Degrees(FVector::UpVector, -1.0f); }
void ACrapsPawn::Input_YawRight()   { RotateDiceOnFelt90Degrees(FVector::UpVector, 1.0f); }
void ACrapsPawn::Input_RollLeft()   { RotateDiceOnFelt90Degrees(FVector::ForwardVector, -1.0f); }
void ACrapsPawn::Input_RollRight()  { RotateDiceOnFelt90Degrees(FVector::ForwardVector, 1.0f); }

void ACrapsPawn::Input_MouseWheelUp()
{
	bool bBoardOpen = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			bBoardOpen = HUD->IsBettingBoardOpen();
		}
	}

	if (bBoardOpen || HoveredBetZone || CurrentPhase == ECrapsThrowPhase::OnFeltSetting)
	{
		CycleChipDenomination(1);
		return;
	}
	ScrollVSetAxis(1.0f);
}

void ACrapsPawn::Input_MouseWheelDown()
{
	bool bBoardOpen = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			bBoardOpen = HUD->IsBettingBoardOpen();
		}
	}

	if (bBoardOpen || HoveredBetZone || CurrentPhase == ECrapsThrowPhase::OnFeltSetting)
	{
		CycleChipDenomination(-1);
		return;
	}
	ScrollVSetAxis(-1.0f);
}

void ACrapsPawn::Input_Preset1() { ApplyPresetDiceSnap(1); }
void ACrapsPawn::Input_Preset2() { ApplyPresetDiceSnap(2); }
void ACrapsPawn::Input_Preset3() { ApplyPresetDiceSnap(3); }

void ACrapsPawn::Input_SelectDieCycle()
{
	SelectedDieIndex = (SelectedDieIndex + 1) % 3;
}

void ACrapsPawn::Input_ArchetypeF1() { SetActiveThrowArchetype(EThrowArchetype::TableSkimmer); }
void ACrapsPawn::Input_ArchetypeF2() { SetActiveThrowArchetype(EThrowArchetype::ParabolicLob); }
void ACrapsPawn::Input_ArchetypeF3() { SetActiveThrowArchetype(EThrowArchetype::BackWallWhip); }
void ACrapsPawn::Input_ArchetypeF4() { SetActiveThrowArchetype(EThrowArchetype::MechanicalPlunger); }

void ACrapsPawn::Input_ThrowPress()
{
	// Suppress throw interactions while actively using Free-Look / Glance camera
	if (bIsFreeLooking)
	{
		return;
	}

	// If the Betting Board is open, route click to HUD overlay (prevents throwing)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			if (HUD->IsBettingBoardOpen())
			{
				FVector2D MousePos;
				if (PC->GetMousePosition(MousePos.X, MousePos.Y))
				{
					if (HUD->HandleBettingBoardClick(MousePos, false))
					{
						return; // Consumed by Betting Board!
					}
				}
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (URunProgressionSubsystem* Progression = World->GetSubsystem<URunProgressionSubsystem>())
		{
			if (Progression->GetRunState() != ERunProgressionState::ActiveAnte)
			{
				return;
			}
		}
	}

	if (CurrentPhase == ECrapsThrowPhase::RollResolved)
	{
		RequestStickmanPushBack();
		return;
	}

	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting)
	{
		return;
	}

	// Targeted betting: clicking on an interactive bet zone places chips on that specific zone instead of throwing!
	if (HoveredBetZone)
	{
		if (UWorld* World = GetWorld())
		{
			if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
			{
				if (GS->CanModifyWager(HoveredBetZone, false))
				{
					if (GS->PlaceWager(HoveredBetZone, SelectedChipDenomination))
					{
						HoveredBetZone->TriggerWinPulse();
						const FVector CamLoc = POVCamera ? POVCamera->GetComponentLocation() : GetActorLocation() + FVector(0.0f, 0.0f, 160.0f);
						const FVector CamFwd = POVCamera ? POVCamera->GetForwardVector() : GetActorForwardVector();
						const FVector CamRight = POVCamera ? POVCamera->GetRightVector() : GetActorRightVector();
						const FVector CamUp = POVCamera ? POVCamera->GetUpVector() : FVector::UpVector;
						const FVector PlayerPos = CamLoc + (CamFwd * 55.0f) + (CamRight * 28.0f) - (CamUp * 16.0f);
						const FVector ZonePos = HoveredBetZone->GetActorLocation() + FVector(0.0f, 0.0f, 14.0f);
						ACrapsFlyingChipActor::SpawnChip(World, PlayerPos, ZonePos, 65.0f, 0.58f, FLinearColor(0.0f, 0.96f, 0.83f), SelectedChipDenomination);

						GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("BET +$%lld ON %s! Bankroll: $%lld"),
							SelectedChipDenomination, *HoveredBetZone->GetZoneName(), GS->GetPlayerBankroll()));
					}
				}
				else
				{
					GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("CANNOT BET ON %s IN CURRENT PHASE"),
						*HoveredBetZone->GetZoneName()));
				}
			}
		}
		return;
	}

	bIsChargingThrow = true;
	CurrentPhase = ECrapsThrowPhase::ThrowCharging;
	ArcadeMeterPhase = 0.0f;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->GetMousePosition(ThrowPressMousePos.X, ThrowPressMousePos.Y);
		PrevMousePos = ThrowPressMousePos;
	}
}

void ACrapsPawn::Input_ThrowRelease()
{
	if (!bIsChargingThrow)
	{
		return;
	}

	bIsChargingThrow = false;
	CommitThrow();
}

void ACrapsPawn::Input_RiggingDecrease()  { AdjustDiceControlStat(-0.10f); }
void ACrapsPawn::Input_RiggingIncrease()  { AdjustDiceControlStat(0.10f); }
void ACrapsPawn::Input_FrictionDecrease() { AdjustTableFriction(-0.10f); }
void ACrapsPawn::Input_FrictionIncrease() { AdjustTableFriction(0.10f); }

void ACrapsPawn::Input_BankPool()
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			GS->BankUnbankedPool();
		}
	}
}

void ACrapsPawn::Input_RebetAll()
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			GS->PlaceDefaultSpreadBets();
		}
	}
}

void ACrapsPawn::Input_AdvanceAnte()
{
	if (UWorld* World = GetWorld())
	{
		if (URunProgressionSubsystem* Progression = World->GetSubsystem<URunProgressionSubsystem>())
		{
			if (Progression->GetRunState() == ERunProgressionState::AnteCleared || Progression->GetRunState() == ERunProgressionState::BetweenAntes)
			{
				Progression->AdvanceToNextAnte();
				RequestStickmanPushBack();
			}
		}
	}
}

void ACrapsPawn::Input_RestartRun()
{
	if (UWorld* World = GetWorld())
	{
		if (URunProgressionSubsystem* Progression = World->GetSubsystem<URunProgressionSubsystem>())
		{
			if (Progression->GetRunState() == ERunProgressionState::RunDefeat || Progression->GetRunState() == ERunProgressionState::RunVictory)
			{
				Progression->StartNewRun();
				RequestStickmanPushBack();
			}
		}
	}
}

void ACrapsPawn::Input_ToggleHUDHelp()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			HUD->ToggleDebugControls();
		}
	}
}

void ACrapsPawn::Input_ClearBets()
{
	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			const int64 Refunded = GS->RefundAllEligibleWagers();
			if (Refunded > 0)
			{
				GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("REFUNDED %lld CHIPS FROM ELIGIBLE BETS"), Refunded));
			}
		}
	}
}

// ============================================================================
// Shooter Stations (multi-POV)
// ============================================================================
FVector ACrapsPawn::PawnLocalToWorld(const FVector& LocalOffset) const
{
	return GetActorLocation() + GetActorQuat().RotateVector(LocalOffset);
}

FString ACrapsPawn::GetCurrentStationName() const
{
	if (TableActor && TableActor->ShooterStations.IsValidIndex(CurrentStationIndex))
	{
		return TableActor->ShooterStations[CurrentStationIndex].StationName;
	}
	return TEXT("Default");
}

void ACrapsPawn::ApplyShooterStation(int32 StationIndex)
{
	if (!TableActor || TableActor->ShooterStations.Num() == 0)
	{
		return;
	}

	CurrentStationIndex = (StationIndex % TableActor->ShooterStations.Num() + TableActor->ShooterStations.Num()) % TableActor->ShooterStations.Num();
	const FCrapsShooterStation& Station = TableActor->ShooterStations[CurrentStationIndex];
	const FTransform TableXf = TableActor->GetActorTransform();

	SetActorLocationAndRotation(
		TableXf.TransformPosition(Station.PawnLocalLocation),
		TableXf.GetRotation() * Station.PawnLocalRotation.Quaternion());
	CurrentLaunchParams.BaseYawDegrees = GetActorRotation().Yaw;

	// Staging center is always directly in front of the shooter along the forward throw vector
	ShooterStagingWorldPos = FVector(190.0f, 0.0f, 11.0f);

	// Convert table-local stickman position into pawn-local coordinates for slide-in animation
	const FTransform PawnXf = GetActorTransform();
	const FVector StickmanWorld = TableXf.TransformPosition(Station.StickmanOriginLocalPos);
	StickmanOriginWorldPos = PawnXf.InverseTransformPosition(StickmanWorld);

	// Re-stage the dice at the new station (also resets the throw phase)
	RequestStickmanPushBack();
}

void ACrapsPawn::CycleShooterStation(int32 Direction)
{
	if (CurrentPhase == ECrapsThrowPhase::DiceInFlight || bIsChargingThrow)
	{
		return;
	}
	ApplyShooterStation(CurrentStationIndex + (Direction >= 0 ? 1 : -1));
}

void ACrapsPawn::Input_StationNext() { CycleShooterStation(1); }
void ACrapsPawn::Input_StationPrev() { CycleShooterStation(-1); }

void ACrapsPawn::Input_ToggleBettingBoard()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			HUD->ToggleBettingBoard();
		}
	}
}

void ACrapsPawn::Input_RightClickPress()
{
	// Suppress table refund interactions while actively using Free-Look / Glance camera
	if (bIsFreeLooking)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			if (HUD->IsBettingBoardOpen())
			{
				FVector2D MousePos;
				if (PC->GetMousePosition(MousePos.X, MousePos.Y))
				{
					if (HUD->HandleBettingBoardClick(MousePos, true))
					{
						return; // Consumed by Betting Board!
					}
				}
			}
		}
	}

	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting || !HoveredBetZone)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			if (GS->CanModifyWager(HoveredBetZone, true))
			{
				int64 Refunded = 0;
				if (GS->RemoveWager(HoveredBetZone, SelectedChipDenomination, Refunded))
				{
					const FVector CamLoc = POVCamera ? POVCamera->GetComponentLocation() : GetActorLocation() + FVector(0.0f, 0.0f, 160.0f);
					const FVector CamFwd = POVCamera ? POVCamera->GetForwardVector() : GetActorForwardVector();
					const FVector CamRight = POVCamera ? POVCamera->GetRightVector() : GetActorRightVector();
					const FVector CamUp = POVCamera ? POVCamera->GetUpVector() : FVector::UpVector;
					const FVector PlayerPos = CamLoc + (CamFwd * 55.0f) + (CamRight * 28.0f) - (CamUp * 16.0f);
					const FVector ZonePos = HoveredBetZone->GetActorLocation() + FVector(0.0f, 0.0f, 14.0f);
					ACrapsFlyingChipActor::SpawnChip(World, ZonePos, PlayerPos, 65.0f, 0.58f, FLinearColor(1.0f, 0.72f, 0.01f), Refunded);

					GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("REFUNDED $%lld FROM %s! Bankroll: $%lld"),
						Refunded, *HoveredBetZone->GetZoneName(), GS->GetPlayerBankroll()));
				}
			}
			else
			{
				GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("CANNOT REMOVE %s (CONTRACT LOCKED)"),
					*HoveredBetZone->GetZoneName()));
			}
		}
	}
}

void ACrapsPawn::CycleChipDenomination(int32 Direction)
{
	static const int64 Denominations[] = { 1, 5, 25, 100, 500, 1000 };
	const int32 Count = UE_ARRAY_COUNT(Denominations);
	int32 CurrentIndex = 2; // Default 25
	for (int32 i = 0; i < Count; ++i)
	{
		if (Denominations[i] == SelectedChipDenomination)
		{
			CurrentIndex = i;
			break;
		}
	}

	if (Direction > 0)
	{
		CurrentIndex = (CurrentIndex + 1) % Count;
	}
	else
	{
		CurrentIndex = (CurrentIndex - 1 + Count) % Count;
	}

	SelectedChipDenomination = Denominations[CurrentIndex];

	if (UWorld* World = GetWorld())
	{
		if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
		{
			GS->SetLastRollOutcomeBanner(FString::Printf(TEXT("CHIP SELECTED: [$%lld]"), SelectedChipDenomination));
		}
	}
}

void ACrapsPawn::SetChipDenomination(int64 NewDenomination)
{
	SelectedChipDenomination = FMath::Max<int64>(1, NewDenomination);
}

void ACrapsPawn::Input_CycleDenominationUp()
{
	CycleChipDenomination(1);
}

void ACrapsPawn::Input_CycleDenominationDown()
{
	CycleChipDenomination(-1);
}

void ACrapsPawn::UpdateCursorHoverRaycast()
{
	if (bIsFreeLooking)
	{
		if (HoveredBetZone)
		{
			HoveredBetZone->SetHovered(false);
			HoveredBetZone = nullptr;
		}
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());

	// If betting board is open, do not raycast or hover 3D table zones
	if (PC)
	{
		if (ACrapsDebugHUD* HUD = Cast<ACrapsDebugHUD>(PC->GetHUD()))
		{
			if (HUD->IsBettingBoardOpen())
			{
				if (HoveredBetZone)
				{
					HoveredBetZone->SetHovered(false);
					HoveredBetZone = nullptr;
				}
				return;
			}
		}
	}

	if (CurrentPhase != ECrapsThrowPhase::OnFeltSetting)
	{
		if (HoveredBetZone)
		{
			HoveredBetZone->SetHovered(false);
			HoveredBetZone = nullptr;
		}
		return;
	}

	if (!PC)
	{
		return;
	}

	FHitResult HitResult;
	const bool bHit = PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, HitResult);

	ABetZone* NewHoveredZone = nullptr;
	if (bHit && HitResult.GetActor())
	{
		NewHoveredZone = Cast<ABetZone>(HitResult.GetActor());
	}

	if (HoveredBetZone != NewHoveredZone)
	{
		if (HoveredBetZone)
		{
			HoveredBetZone->SetHovered(false);
		}
		HoveredBetZone = NewHoveredZone;
		if (HoveredBetZone)
		{
			HoveredBetZone->SetHovered(true);
		}
	}
}

FVector ACrapsPawn::GetFingerKnuckleWorldLocation(int32 FingerIndex) const
{
	const int32 ClampedIndex = FMath::Clamp(FingerIndex, 0, 3);
	if (LeftRingMeshes.IsValidIndex(ClampedIndex) && LeftRingMeshes[ClampedIndex] && LeftRingMeshes[ClampedIndex]->IsRegistered())
	{
		return LeftRingMeshes[ClampedIndex]->GetComponentLocation();
	}
	if (LeftHandRailAnchor)
	{
		const FTransform AnchorLocalXf(LeftHandRailAnchor->GetRelativeRotation(), LeftHandRailAnchor->GetRelativeLocation());
		const FVector RelOffset = (LeftRingMeshes.IsValidIndex(ClampedIndex) && LeftRingMeshes[ClampedIndex])
			? LeftRingMeshes[ClampedIndex]->GetRelativeLocation()
			: FVector(15.0f, -1.5f, 0.5f);
		return PawnLocalToWorld(AnchorLocalXf.TransformPosition(RelOffset));
	}
	return PawnLocalToWorld(FVector(85.0f, -24.0f, 132.0f));
}

FVector ACrapsPawn::GetWristWorldLocation() const
{
	if (LeftWristMesh && LeftWristMesh->IsRegistered())
	{
		return LeftWristMesh->GetComponentLocation();
	}
	if (LeftHandRailAnchor)
	{
		const FTransform AnchorLocalXf(LeftHandRailAnchor->GetRelativeRotation(), LeftHandRailAnchor->GetRelativeLocation());
		const FVector RelOffset = LeftWristMesh ? LeftWristMesh->GetRelativeLocation() : FVector(-4.0f, 0.0f, 3.5f);
		return PawnLocalToWorld(AnchorLocalXf.TransformPosition(RelOffset));
	}
	return PawnLocalToWorld(FVector(80.0f, -24.0f, 135.0f));
}

FVector ACrapsPawn::GetRailPropWorldLocation(int32 PropIndex) const
{
	if (LeftRailPropMesh && LeftRailPropMesh->IsRegistered())
	{
		return LeftRailPropMesh->GetComponentLocation();
	}
	if (LeftHandRailAnchor)
	{
		const FTransform AnchorLocalXf(LeftHandRailAnchor->GetRelativeRotation(), LeftHandRailAnchor->GetRelativeLocation());
		const FVector RelOffset = LeftRailPropMesh ? LeftRailPropMesh->GetRelativeLocation() : FVector(6.0f, -14.0f, 3.0f);
		return PawnLocalToWorld(AnchorLocalXf.TransformPosition(RelOffset));
	}
	return PawnLocalToWorld(FVector(90.0f, -38.0f, 135.0f));
}

void ACrapsPawn::Input_FreeLookPress()
{
	bIsFreeLooking = true;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

void ACrapsPawn::Input_FreeLookRelease()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		const bool bStillHolding = PC->IsInputKeyDown(EKeys::MiddleMouseButton) || PC->IsInputKeyDown(EKeys::LeftAlt);
		if (bStillHolding)
		{
			return;
		}
	}

	bIsFreeLooking = false;
	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void ACrapsPawn::HandleTrinketEquippedVisual(UPDA_Trinket* /*Trinket*/, ETrinketSlotType /*SlotType*/, int32 /*SlotIndex*/)
{
	UpdateTrinketVisuals();
}

void ACrapsPawn::HandleTrinketUnequippedVisual(UPDA_Trinket* /*Trinket*/, ETrinketSlotType /*SlotType*/, int32 /*SlotIndex*/)
{
	UpdateTrinketVisuals();
}

void ACrapsPawn::UpdateTrinketVisuals()
{
	static UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto SetCompColorSlot = [this](UStaticMeshComponent* Comp, int32 SlotIdx, const FLinearColor& Color)
	{
		if (!Comp)
		{
			return;
		}
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(SlotIdx)))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
		else if (BaseMat)
		{
			UMaterialInstanceDynamic* NewMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			NewMID->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(SlotIdx, NewMID);
		}
	};

	// 1. Cybernetic Hand Finish
	const bool bWristEquipped = TrinketInventory && (TrinketInventory->GetEquippedTrinket(ETrinketSlotType::WristwearActive, 0) != nullptr);

	if (LeftForearmMesh && LeftForearmMesh->GetNumMaterials() >= 4)
	{
		// Slot 0: Matte Charcoal Titanium Armor
		SetCompColorSlot(LeftForearmMesh, 0, FLinearColor(0.035f, 0.040f, 0.045f));
		// Slot 1: Brushed Gunmetal Joint Hydraulics
		SetCompColorSlot(LeftForearmMesh, 1, FLinearColor(0.080f, 0.085f, 0.095f));
		// Slot 2: Glowing Neon Power Conduit & Chrono Screen (Amber/Gold when wristwear active, otherwise Cyan)
		SetCompColorSlot(LeftForearmMesh, 2, bWristEquipped ? FLinearColor(1.0f, 0.65f, 0.08f) : FLinearColor(0.0f, 0.85f, 1.0f));
		// Slot 3: Brushed Brass / Gold Trim
		SetCompColorSlot(LeftForearmMesh, 3, FLinearColor(1.0f, 0.78f, 0.12f));
	}
	else
	{
		SetCompColorSlot(LeftForearmMesh, 0, FLinearColor(0.04f, 0.045f, 0.05f));
		SetCompColorSlot(LeftPalmMesh, 0, FLinearColor(0.05f, 0.055f, 0.06f));
		for (UStaticMeshComponent* FingerMesh : LeftFingerMeshes)
		{
			SetCompColorSlot(FingerMesh, 0, FLinearColor(0.08f, 0.085f, 0.09f));
		}
	}

	// 2. Knuckle Signet Rings (0 = Neon Cyan, 1 = Royal Cobalt, 2 = Synth Violet, 3 = Brilliant Gold)
	const FLinearColor RingColors[4] = {
		FLinearColor(0.0f, 0.85f, 1.0f),    // Index: Neon Signet (Cyan)
		FLinearColor(0.12f, 0.40f, 1.0f),   // Middle: Cobalt Band (Royal Blue)
		FLinearColor(0.75f, 0.15f, 0.95f),  // Ring: Amethyst / Synergies (Violet)
		FLinearColor(1.0f, 0.78f, 0.10f)    // Pinky: High Roller (Brilliant Gold)
	};

	for (int32 i = 0; i < LeftRingMeshes.Num(); ++i)
	{
		if (!LeftRingMeshes[i])
		{
			continue;
		}

		const bool bEquipped = TrinketInventory && (TrinketInventory->GetEquippedTrinket(ETrinketSlotType::FingerRing, i) != nullptr);
		LeftRingMeshes[i]->SetVisibility(bEquipped);
		if (bEquipped)
		{
			// Slot 0: Polished Vegas Gold Band; Slot 1: Emissive Cut Gemstone Face
			SetCompColorSlot(LeftRingMeshes[i], 0, FLinearColor(1.0f, 0.80f, 0.15f));
			SetCompColorSlot(LeftRingMeshes[i], 1, RingColors[FMath::Clamp(i, 0, 3)]);
		}
	}

	// 3. Wristwear Cyber-Chrono Cuff (integrated into SM_CyberArm_Left when custom mesh is loaded)
	if (LeftWristMesh)
	{
		// Only display separate fallback cylinder if no custom cyber-arm exists
		const bool bShowFallbackWrist = bWristEquipped && (LeftForearmMesh && LeftForearmMesh->GetNumMaterials() < 4);
		LeftWristMesh->SetVisibility(bShowFallbackWrist);
		if (bShowFallbackWrist)
		{
			SetCompColorSlot(LeftWristMesh, 0, FLinearColor(0.95f, 0.55f, 0.10f));
		}
	}

	// 4. Rail Prop (Whiskey Tumbler - Faceted Crystal Glass + Glowing Bourbon)
	if (LeftRailPropMesh)
	{
		const bool bPropEquipped = TrinketInventory && (TrinketInventory->GetEquippedTrinket(ETrinketSlotType::RailProp, 0) != nullptr);
		LeftRailPropMesh->SetVisibility(bPropEquipped);
		if (bPropEquipped)
		{
			// Slot 0: Crystal glass
			SetCompColorSlot(LeftRailPropMesh, 0, FLinearColor(0.85f, 0.90f, 0.95f));
			// Slot 1: Amber bourbon liquid
			SetCompColorSlot(LeftRailPropMesh, 1, FLinearColor(1.0f, 0.55f, 0.05f));
			// Slot 2: Floating ice cube
			SetCompColorSlot(LeftRailPropMesh, 2, FLinearColor(0.75f, 0.90f, 1.0f));
		}
	}
}

void ACrapsPawn::Input_ToggleStarterTrinkets()
{
	if (!TrinketInventory)
	{
		return;
	}

	if (TrinketInventory->GetEquippedCount() > 0)
	{
		TrinketInventory->ClearAllTrinkets();
		UpdateTrinketVisuals();
		if (UWorld* World = GetWorld())
		{
			if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
			{
				GS->SetLastRollOutcomeBanner(TEXT("TRINKETS UNEQUIPPED — Press [J] to Re-equip Starter Rig"));
			}
		}
	}
	else
	{
		InitializeDefaultTrinketLoadout();
		UpdateTrinketVisuals();
		if (UWorld* World = GetWorld())
		{
			if (ACrapsGameState* GS = World->GetGameState<ACrapsGameState>())
			{
				GS->SetLastRollOutcomeBanner(TEXT("STARTER TRINKETS EQUIPPED! (Neon Signet • Cobalt Band • Brass Knuckles • Whiskey)"));
			}
		}
	}
}

void ACrapsPawn::InitializeDefaultTrinketLoadout()
{
	if (!TrinketInventory || TrinketInventory->GetEquippedCount() > 0)
	{
		return;
	}

	// 1. Neon Signet (Ring 0): Odd sum -> +4.0x Throw Mult, Even sum -> +40 Base Chips
	UPDA_Trinket* NeonSignet = Cast<UPDA_Trinket>(StaticLoadObject(UPDA_Trinket::StaticClass(), nullptr, TEXT("/Game/Data/Trinkets/DA_Trinket_NeonSignet")));
	if (!NeonSignet)
	{
		NeonSignet = NewObject<UPDA_Trinket>(this, TEXT("Starter_NeonSignet"));
		NeonSignet->TrinketName = FText::FromString(TEXT("Neon Signet"));
		NeonSignet->Description = FText::FromString(TEXT("Odd sums grant +4.0x Throw Mult; Even sums grant +40 Base Chips."));
		NeonSignet->SlotType = ETrinketSlotType::FingerRing;
		NeonSignet->bSplitOddEven = true;
		NeonSignet->OddSumThrowMult = 4.0f;
		NeonSignet->EvenSumBaseChips = 40;
	}
	TrinketInventory->EquipTrinket(NeonSignet, 0);

	// 2. Cobalt Band (Ring 1): Symmetric Hardway pairs double active Horn & Field multipliers
	UPDA_Trinket* CobaltBand = Cast<UPDA_Trinket>(StaticLoadObject(UPDA_Trinket::StaticClass(), nullptr, TEXT("/Game/Data/Trinkets/DA_Trinket_CobaltBand")));
	if (!CobaltBand)
	{
		CobaltBand = NewObject<UPDA_Trinket>(this, TEXT("Starter_CobaltBand"));
		CobaltBand->TrinketName = FText::FromString(TEXT("Cobalt Band"));
		CobaltBand->Description = FText::FromString(TEXT("Symmetric Hardways double active Horn and Field multipliers (x2.0 Bet Mult)."));
		CobaltBand->SlotType = ETrinketSlotType::FingerRing;
		CobaltBand->TriggerConditionTag = FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnHardway"), false);
		CobaltBand->TargetScoringLayer = EScoringLayer::BetMult;
		CobaltBand->BetMultFactor = 2.0f;
	}
	TrinketInventory->EquipTrinket(CobaltBand, 1);

	// 3. Brass Knuckle Band (Wristwear): +20 chips, +0.25x mult per bounce
	UPDA_Trinket* BrassKnuckles = Cast<UPDA_Trinket>(StaticLoadObject(UPDA_Trinket::StaticClass(), nullptr, TEXT("/Game/Data/Trinkets/DA_Trinket_BrassKnuckleBand")));
	if (!BrassKnuckles)
	{
		BrassKnuckles = NewObject<UPDA_Trinket>(this, TEXT("Starter_BrassKnuckles"));
		BrassKnuckles->TrinketName = FText::FromString(TEXT("Brass Knuckle Band"));
		BrassKnuckles->Description = FText::FromString(TEXT("+20 Base Chips and +0.25x Throw Mult per table bounce."));
		BrassKnuckles->SlotType = ETrinketSlotType::WristwearActive;
		BrassKnuckles->bScaleWithBounces = true;
		BrassKnuckles->ChipsPerBounce = 20;
		BrassKnuckles->ThrowMultPerBounce = 0.25f;
	}
	TrinketInventory->EquipTrinket(BrassKnuckles, 0);

	// 4. Whiskey Tumbler (Rail Prop 0): Point streak multiplier booster
	UPDA_Trinket* Whiskey = Cast<UPDA_Trinket>(StaticLoadObject(UPDA_Trinket::StaticClass(), nullptr, TEXT("/Game/Data/Trinkets/DA_Trinket_WhiskeyTumbler")));
	if (!Whiskey)
	{
		Whiskey = NewObject<UPDA_Trinket>(this, TEXT("Starter_WhiskeyTumbler"));
		Whiskey->TrinketName = FText::FromString(TEXT("Whiskey Tumbler"));
		Whiskey->Description = FText::FromString(TEXT("Hot Shooter point streaks have their bonus multiplier increased by +50%."));
		Whiskey->SlotType = ETrinketSlotType::RailProp;
		Whiskey->TriggerConditionTag = FGameplayTag::RequestGameplayTag(TEXT("Craps.Trigger.OnPointHit"), false);
		Whiskey->TargetScoringLayer = EScoringLayer::PointBonus;
		Whiskey->BetMultFactor = 1.5f;
	}
	TrinketInventory->EquipTrinket(Whiskey, 0);
}
