// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Table/BetZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

ABetZone::ABetZone()
{
	PrimaryActorTick.bCanEverTick = true;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	SetRootComponent(ZoneBounds);
	ZoneBounds->SetBoxExtent(FVector(55.0f, 110.0f, 4.0f));
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ZonePadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZonePadMesh"));
	ZonePadMesh->SetupAttachment(ZoneBounds);
	ZonePadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZonePadMesh->SetRelativeScale3D(FVector(1.05f, 2.15f, 0.02f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	ZoneBorderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneBorderMesh"));
	ZoneBorderMesh->SetupAttachment(ZoneBounds);
	ZoneBorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneTitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneTitleText"));
	ZoneTitleText->SetupAttachment(ZoneBounds);
	ZoneTitleText->SetHorizontalAlignment(EHTA_Center);
	ZoneTitleText->SetVerticalAlignment(EVRTA_TextCenter);

	ZoneOddsText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneOddsText"));
	ZoneOddsText->SetupAttachment(ZoneBounds);
	ZoneOddsText->SetHorizontalAlignment(EHTA_Center);
	ZoneOddsText->SetVerticalAlignment(EVRTA_TextCenter);

	ChipStackMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ChipStackMesh"));
	ChipStackMesh->SetupAttachment(ZoneBounds);
	ChipStackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChipStackMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChipMeshFinder(
		TEXT("/Game/Meshes/SM_CasinoChip_v1.SM_CasinoChip_v1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	if (ChipMeshFinder.Succeeded() && ChipMeshFinder.Object)
	{
		ChipStackMesh->SetStaticMesh(ChipMeshFinder.Object);
	}
	else if (CylinderMeshFinder.Succeeded() && CylinderMeshFinder.Object)
	{
		ChipStackMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	if (CubeMeshFinder.Succeeded())
	{
		ZonePadMesh->SetStaticMesh(CubeMeshFinder.Object);
		ZoneBorderMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

void ABetZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisuals();
}

void ABetZone::SetZoneHalfExtent(float HalfExtentX, float HalfExtentY)
{
	ZoneHalfExtent = FVector2D(FMath::Max(8.0f, HalfExtentX), FMath::Max(8.0f, HalfExtentY));
	RefreshVisuals();
}

void ABetZone::RefreshVisuals()
{
	if (!ZoneBounds || !ZonePadMesh || !ZoneBorderMesh || !ZoneTitleText || !ZoneOddsText)
	{
		return;
	}

	ZoneBounds->SetBoxExtent(FVector(ZoneHalfExtent.X, ZoneHalfExtent.Y, 4.0f));

	// Cube mesh is 100cm, so scale = fullSize / 100. Border sits just under the pad and peeks out as an outline.
	const FVector PadScale(ZoneHalfExtent.X * 2.0f / 100.0f * 0.94f, ZoneHalfExtent.Y * 2.0f / 100.0f * 0.94f, 0.004f);
	const FVector BorderScale(ZoneHalfExtent.X * 2.0f / 100.0f, ZoneHalfExtent.Y * 2.0f / 100.0f, 0.003f);
	ZonePadMesh->SetRelativeScale3D(PadScale);
	ZonePadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.5f));
	ZoneBorderMesh->SetRelativeScale3D(BorderScale);
	ZoneBorderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.3f));

	const bool bHasWager = PlacedWager > 0;
	const FLinearColor Cyan(0.0f, 0.96f, 0.83f);
	const FLinearColor Gold(1.0f, 0.72f, 0.01f);
	FLinearColor AccentColor = bHasWager ? Gold : Cyan;
	if (bIsHovered)
	{
		AccentColor = bHasWager ? FLinearColor(1.0f, 0.95f, 0.45f) : FLinearColor(0.5f, 1.0f, 0.95f);
	}

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (!PadMID)
		{
			PadMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			ZonePadMesh->SetMaterial(0, PadMID);
		}
		if (!BorderMID)
		{
			BorderMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			ZoneBorderMesh->SetMaterial(0, BorderMID);
		}
		const FLinearColor PadColor = bIsHovered ? FLinearColor(0.03f, 0.07f, 0.08f) : FLinearColor(0.01f, 0.04f, 0.04f);
		PadMID->SetVectorParameterValue(TEXT("Color"), PadColor);
		BorderMID->SetVectorParameterValue(TEXT("Color"), AccentColor);
	}

	// Text lies flat on the felt, readable from the +X shooter end (reads along +Y, top toward +X).
	// Long, thin zones read along X instead so the label fits.
	const bool bReadAlongX = ZoneHalfExtent.X > ZoneHalfExtent.Y * 1.5f;
	const FQuat BaseTextQuat = FRotator(90.0f, 180.0f, 0.0f).Quaternion();
	const FQuat TextQuat = bReadAlongX ? (FRotator(0.0f, -90.0f, 0.0f).Quaternion() * BaseTextQuat) : BaseTextQuat;
	const float AvailableWidth = (bReadAlongX ? ZoneHalfExtent.X : ZoneHalfExtent.Y) * 2.0f * 0.9f;

	const FString TitleStr = ZoneDisplayName;
	const FString OddsStr = FString::Printf(TEXT("$%lld | %.1fx"), PlacedWager, GetEffectiveBetMultiplier());
	const float TitleSize = FMath::Clamp(AvailableWidth / (FMath::Max(1, TitleStr.Len()) * 0.62f), 8.0f, 22.0f);
	const float OddsSize = FMath::Clamp(AvailableWidth / (FMath::Max(1, OddsStr.Len()) * 0.62f), 6.0f, 16.0f);

	ZoneTitleText->SetText(FText::FromString(TitleStr));
	ZoneTitleText->SetWorldSize(TitleSize);
	ZoneTitleText->SetTextRenderColor(AccentColor.ToFColor(true));
	ZoneTitleText->SetRelativeRotation(TextQuat);
	ZoneTitleText->SetRelativeLocation(FVector(bReadAlongX ? 0.0f : TitleSize * 0.6f, bReadAlongX ? -TitleSize * 0.6f : 0.0f, 1.2f));

	ZoneOddsText->SetText(FText::FromString(OddsStr));
	ZoneOddsText->SetWorldSize(OddsSize);
	ZoneOddsText->SetTextRenderColor(FColor(235, 240, 250));
	ZoneOddsText->SetRelativeRotation(TextQuat);
	ZoneOddsText->SetRelativeLocation(FVector(bReadAlongX ? 0.0f : -OddsSize * 0.7f, bReadAlongX ? OddsSize * 0.7f : 0.0f, 1.2f));

	if (ChipStackMesh)
	{
		ChipStackMesh->ClearInstances();

		if (PlacedWager > 0 && ChipStackMesh->GetStaticMesh())
		{
			UStaticMesh* Mesh = ChipStackMesh->GetStaticMesh();
			const FBoxSphereBounds Bounds = Mesh->GetBounds();
			const float DesiredRadius = 14.0f; // 28cm diameter stack, fits cleanly on all bet zone pads
			const float ScaleXY = (Bounds.BoxExtent.X > KINDA_SMALL_NUMBER) ? (DesiredRadius / Bounds.BoxExtent.X) : 1.0f;
			const float DesiredHalfHeight = 1.4f; // 2.8cm per chip
			const float ScaleZ = (Bounds.BoxExtent.Z > KINDA_SMALL_NUMBER) ? (DesiredHalfHeight / Bounds.BoxExtent.Z) : ScaleXY;
			const FVector ChipScale(ScaleXY, ScaleXY, ScaleZ);

			const int32 NumChips = FMath::Clamp(static_cast<int32>(PlacedWager / 25), 1, 8);
			const float StepZ = DesiredHalfHeight * 2.0f;

			for (int32 i = 0; i < NumChips; ++i)
			{
				const float JitterYaw = static_cast<float>((i * 37) % 360);
				const FTransform InstXf(
					FRotator(0.0f, JitterYaw, 0.0f),
					FVector(0.0f, 0.0f, 1.5f + (i * StepZ) + DesiredHalfHeight),
					ChipScale);
				ChipStackMesh->AddInstance(InstXf);
			}

			if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
			{
				const int32 NumMats = FMath::Max(1, Mesh->GetStaticMaterials().Num());
				for (int32 Slot = 0; Slot < NumMats; ++Slot)
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
					if (MID)
					{
						FLinearColor ChipColor = AccentColor;
						if (Slot == 1)
						{
							ChipColor = FMath::Lerp(AccentColor, FLinearColor::White, 0.4f);
						}
						MID->SetVectorParameterValue(TEXT("Color"), ChipColor);
						ChipStackMesh->SetMaterial(Slot, MID);
					}
				}
			}
		}
	}
}

void ABetZone::BeginPlay()
{
	Super::BeginPlay();
}

void ABetZone::InitializeZone(ECrapsBetType InBetType, const FString& InDisplayName, float InBaseMultiplier, int64 InInitialWager)
{
	BetType = InBetType;
	ZoneDisplayName = InDisplayName;
	BaseBetMultiplier = InBaseMultiplier;
	DefaultWager = InInitialWager;
	PlacedWager = 0;
	RefreshVisuals();
}

void ABetZone::AddWager(int64 AdditionalChips)
{
	PlacedWager = FMath::Max<int64>(0, PlacedWager + AdditionalChips);
	RefreshVisuals();
}

void ABetZone::SetPlacedWager(int64 InWager)
{
	PlacedWager = FMath::Max<int64>(0, InWager);
	RefreshVisuals();
}

void ABetZone::SetDefaultWager(int64 InDefaultWager)
{
	DefaultWager = FMath::Max<int64>(0, InDefaultWager);
}

void ABetZone::ClearWager()
{
	PlacedWager = 0;
	RefreshVisuals();
}

void ABetZone::SetHovered(bool bInHovered)
{
	if (bIsHovered != bInHovered)
	{
		bIsHovered = bInHovered;
		RefreshVisuals();
	}
}

float ABetZone::GetEffectiveBetMultiplier() const
{
	const float UpgradeBonus = (ActiveUpgrade != nullptr) ? ActiveUpgrade->BonusBetMultiplier : 0.0f;
	return BaseBetMultiplier + UpgradeBonus;
}

void ABetZone::TriggerWinPulse()
{
	WinPulseTimer = 1.25f;
}

bool ABetZone::EvaluateRoll(
	int32 Die1Val,
	int32 Die2Val,
	ECrapsTablePhase CurrentPhase,
	int32 EstablishedPoint,
	FScoringStep& OutScoringStep,
	bool& bOutClearBet)
{
	bOutClearBet = false;

	if (PlacedWager <= 0)
	{
		return false;
	}

	const int32 RollSum = Die1Val + Die2Val;
	const bool bIsHardway = (Die1Val == Die2Val);
	bool bWon = false;
	float PayoutMult = GetEffectiveBetMultiplier();
	FString WinReason;

	switch (BetType)
	{
	case ECrapsBetType::PassLine:
		if (CurrentPhase == ECrapsTablePhase::ComeOutRoll)
		{
			if (RollSum == 7 || RollSum == 11)
			{
				bWon = true;
				WinReason = FString::Printf(TEXT("PASS LINE NATURAL (%d)"), RollSum);
				bOutClearBet = true; // Natural win resolves and clears Pass Line
			}
			else if (RollSum == 2 || RollSum == 3 || RollSum == 12)
			{
				bOutClearBet = true; // Craps out on Come-Out loses Pass Line
			}
		}
		else if (CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			if (RollSum == EstablishedPoint)
			{
				bWon = true;
				PayoutMult += 0.5f; // Point hit bonus
				WinReason = FString::Printf(TEXT("PASS LINE POINT HIT (%d)"), RollSum);
				bOutClearBet = true; // Point hit resolves and clears Pass Line
			}
			else if (RollSum == 7)
			{
				bOutClearBet = true; // Seven-Out clears Pass Line
			}
		}
		break;

	case ECrapsBetType::DontPass:
		if (CurrentPhase == ECrapsTablePhase::ComeOutRoll)
		{
			if (RollSum == 2 || RollSum == 3)
			{
				bWon = true;
				WinReason = FString::Printf(TEXT("DON'T PASS CRAPS (%d)"), RollSum);
				bOutClearBet = true; // Won on Come-Out craps
			}
			else if (RollSum == 7 || RollSum == 11)
			{
				bOutClearBet = true; // Natural 7/11 loses Don't Pass
			}
			// Note: 12 is a Push (Bar 12) per Craps rules; neither wins nor clears
		}
		else if (CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			if (RollSum == 7)
			{
				bWon = true;
				WinReason = TEXT("DON'T PASS SEVEN-OUT WIN");
				bOutClearBet = true; // Seven-Out wins and clears Don't Pass
			}
			else if (RollSum == EstablishedPoint)
			{
				bOutClearBet = true; // Point hit loses Don't Pass
			}
		}
		break;

	case ECrapsBetType::Field:
		// One-roll bet: wins on 2, 3, 4, 9, 10, 11, 12 (2x on 2, 3x on 12); clears every roll
		bOutClearBet = true;
		if (RollSum == 2 || RollSum == 3 || RollSum == 4 || RollSum == 9 || RollSum == 10 || RollSum == 11 || RollSum == 12)
		{
			bWon = true;
			if (RollSum == 2)
			{
				PayoutMult *= 2.0f;
				WinReason = TEXT("FIELD DOUBLE (SNAKE EYES 2)");
			}
			else if (RollSum == 12)
			{
				PayoutMult *= 3.0f;
				WinReason = TEXT("FIELD TRIPLE (BOXCARS 12)");
			}
			else
			{
				WinReason = FString::Printf(TEXT("FIELD HIT (%d)"), RollSum);
			}
		}
		break;

	case ECrapsBetType::Place4:
	case ECrapsBetType::Place5:
	case ECrapsBetType::Place6:
	case ECrapsBetType::Place8:
	case ECrapsBetType::Place9:
	case ECrapsBetType::Place10:
	{
		int32 TargetNum = 0;
		if (BetType == ECrapsBetType::Place4) TargetNum = 4;
		else if (BetType == ECrapsBetType::Place5) TargetNum = 5;
		else if (BetType == ECrapsBetType::Place6) TargetNum = 6;
		else if (BetType == ECrapsBetType::Place8) TargetNum = 8;
		else if (BetType == ECrapsBetType::Place9) TargetNum = 9;
		else if (BetType == ECrapsBetType::Place10) TargetNum = 10;

		if (RollSum == TargetNum)
		{
			bWon = true;
			WinReason = FString::Printf(TEXT("PLACE %d HIT"), TargetNum);
		}
		else if (RollSum == 7 && CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			bOutClearBet = true;
		}
		break;
	}

	case ECrapsBetType::Hard4:
	case ECrapsBetType::Hard6:
	case ECrapsBetType::Hard8:
	case ECrapsBetType::Hard10:
	{
		int32 TargetHard = 0;
		if (BetType == ECrapsBetType::Hard4) TargetHard = 4;
		else if (BetType == ECrapsBetType::Hard6) TargetHard = 6;
		else if (BetType == ECrapsBetType::Hard8) TargetHard = 8;
		else if (BetType == ECrapsBetType::Hard10) TargetHard = 10;

		if (RollSum == TargetHard && bIsHardway)
		{
			bWon = true;
			WinReason = FString::Printf(TEXT("HARD %d (%d-%d)"), TargetHard, Die1Val, Die2Val);
		}
		else if (RollSum == TargetHard && !bIsHardway)
		{
			// Easy way loses the hardway bet
			bOutClearBet = true;
		}
		else if (RollSum == 7 && CurrentPhase == ECrapsTablePhase::PointEstablished)
		{
			// 7 during point phase clears hardway
			bOutClearBet = true;
		}
		break;
	}

	case ECrapsBetType::Horn:
		// One-roll bet: wins on 2, 3, 11, 12; loses on all other numbers; clears every roll
		bOutClearBet = true;
		if (RollSum == 2 || RollSum == 3 || RollSum == 11 || RollSum == 12)
		{
			bWon = true;
			if (RollSum == 2 || RollSum == 12)
			{
				PayoutMult *= 2.0f;
			}
			WinReason = FString::Printf(TEXT("HORN BET HIT (%d)"), RollSum);
		}
		break;
	}

	if (bWon)
	{
		TriggerWinPulse();
		const int64 BetPayout = FMath::RoundToInt64(static_cast<double>(PlacedWager) * PayoutMult) + ((ActiveUpgrade != nullptr) ? ActiveUpgrade->BonusBaseChips : 0);
		OutScoringStep.SourceActor = this;
		OutScoringStep.StepLabel = FString::Printf(TEXT("%s (+%lld CHIPS)"), *WinReason, BetPayout);
		OutScoringStep.Layer = EScoringLayer::BetMult;
		OutScoringStep.BaseChipDelta = static_cast<int32>(BetPayout);
		OutScoringStep.ThrowMultDelta = 0.0f;
		OutScoringStep.BetMultAdditiveDelta = 0.0f;
		OutScoringStep.MultiplierFactor = 1.0f; // Fix: ordinary craps odds do not multiply across unrelated winning bets!
		OutScoringStep.PopupWorldLocation = GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);
		return true;
	}

	return false;
}

void ABetZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (WinPulseTimer > 0.0f)
	{
		WinPulseTimer = FMath::Max(0.0f, WinPulseTimer - DeltaTime);
	}

	UWorld* World = GetWorld();
	if (!World || !ZoneBounds)
	{
		return;
	}

	const FVector Loc = GetActorLocation();
	const FVector Extent = ZoneBounds->GetScaledBoxExtent();
	const FQuat Rot = GetActorQuat();

	// Synth-Noir Vector Felt Outlines:
	// Gold (#FFB703) when pulsing or heavily wagered, Cyan (#00F5D4) for standard active bet zones
	const bool bIsPulsing = (WinPulseTimer > 0.0f);
	const FColor BorderColor = bIsPulsing
		? FColor(255, 255, 255)
		: (PlacedWager > 0 ? FColor(255, 183, 3) : FColor(0, 245, 212));

	DrawDebugBox(World, Loc + FVector(0.0f, 0.0f, 2.0f), Extent, Rot, BorderColor, false, -1.0f, 0, bIsPulsing ? 4.5f : 2.0f);

	// Draw 3D stacked vector chips inside the BetZone if a wager is placed
	if (PlacedWager > 0)
	{
		const int32 NumVisualChips = FMath::Clamp<int32>(static_cast<int32>(PlacedWager / 25) + 1, 1, 8);
		const FVector StackBase = Loc + FVector(0.0f, 0.0f, 3.0f);
		for (int32 i = 0; i < NumVisualChips; ++i)
		{
			const FVector ChipPos = StackBase + FVector(0.0f, 0.0f, i * 3.5f);
			DrawDebugCylinder(
				World,
				ChipPos,
				ChipPos + FVector(0.0f, 0.0f, 2.5f),
				14.0f,
				12,
				bIsPulsing ? FColor(0, 245, 212) : FColor(255, 183, 3),
				false,
				-1.0f,
				0,
				1.6f);
		}
	}
}
