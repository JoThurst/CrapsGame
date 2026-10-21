// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CrapsTypes.generated.h"

class ADieBase;
class UPDA_DiceFace;

/**
 * 6 local-space face directions on a 20cm x 20cm x 20cm 10x scale Craps die.
 */
UENUM(BlueprintType)
enum class EDieFaceDirection : uint8
{
	PlusZ  UMETA(DisplayName = "+Z (Top / Default 1)"),
	MinusZ UMETA(DisplayName = "-Z (Bottom / Default 6)"),
	PlusX  UMETA(DisplayName = "+X (Forward / Default 2)"),
	MinusX UMETA(DisplayName = "-X (Backward / Default 5)"),
	PlusY  UMETA(DisplayName = "+Y (Right / Default 3)"),
	MinusY UMETA(DisplayName = "-Y (Left / Default 4)")
};

/**
 * Right-Hand active throw archetypes (hot-swappable via F1-F4 in the Testing Pit).
 */
UENUM(BlueprintType)
enum class EThrowArchetype : uint8
{
	TableSkimmer      UMETA(DisplayName = "F1: Table-Skimmer (Shuffleboard Slide)"),
	ParabolicLob      UMETA(DisplayName = "F2: Parabolic Lob (Helicopter Soft Toss)"),
	BackWallWhip      UMETA(DisplayName = "F3: Back-Wall Whip (Fastball Line Drive)"),
	MechanicalPlunger UMETA(DisplayName = "F4: Mechanical Plunger (100% Repeatable Spring)")
};

/**
 * Input charging/release modes for the Testing Pit debug overlay.
 */
UENUM(BlueprintType)
enum class EThrowInputMode : uint8
{
	MouseFlick    UMETA(DisplayName = "Mouse Flick (Gesture Velocity)"),
	ArcadeMeter   UMETA(DisplayName = "Arcade Meter (Oscillating Power Bar)"),
	SlingshotPull UMETA(DisplayName = "Slingshot Pull (Drag Back & Release)")
};

/**
 * On-Felt pre-throw dice setting methods.
 */
UENUM(BlueprintType)
enum class EDiceSetMethod : uint8
{
	FingerTip  UMETA(DisplayName = "Finger-Tip (90-Degree Click Rotation)"),
	VSetScroll UMETA(DisplayName = "V-Set Slide (Locked-Axis Pair Scroll)"),
	PresetSnap UMETA(DisplayName = "Holographic Preset Snap (1/2/3 Keys)")
};

/**
 * Rigging behavior mode for UDiceRiggingComponent.
 */
UENUM(BlueprintType)
enum class EDiceRiggingMode : uint8
{
	TargetSetFace     UMETA(DisplayName = "Target Pre-Set Top Face"),
	AvoidSevenOut     UMETA(DisplayName = "Anti-Seven Survival (Avoid Sum of 7)")
};

/**
 * Shooter POV state machine phases.
 */
UENUM(BlueprintType)
enum class ECrapsThrowPhase : uint8
{
	StickmanReturn UMETA(DisplayName = "Stickman Push-Back"),
	OnFeltSetting  UMETA(DisplayName = "On-Felt Dice Setting"),
	ThrowCharging  UMETA(DisplayName = "Throw Charging / Aiming"),
	DiceInFlight   UMETA(DisplayName = "Dice In Flight / Settling"),
	RollResolved   UMETA(DisplayName = "Roll Resolved")
};

/**
 * Parameters passed into IThrowStyleInterface::ExecuteThrow and GetPreviewTrajectory.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FThrowLaunchParams
{
	GENERATED_BODY()

	/** Normalized throw power [0.0 .. 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	float NormalizedPower = 0.65f;

	/** Horizontal aim yaw offset in degrees relative to table forward (+X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	float AimYawOffsetDegrees = 0.0f;

	/** World yaw of the active shooter station (0 = throws toward +X, 180 = toward -X). Applied before aim offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	float BaseYawDegrees = 0.0f;

	/** Lateral spin / curve input [-1.0 .. 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	float LateralSpin = 0.0f;

	/** World-space target ring location on the felt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	FVector TargetWorldLocation = FVector(1800.0f, 0.0f, 100.0f);

	/** True if released inside the Arcade Meter / Metronome sweet spot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	bool bSweetSpotRelease = false;

	/** Active input mode used to generate this throw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Throw")
	EThrowInputMode InputMode = EThrowInputMode::SlingshotPull;
};

/**
 * Semantic 3-Layer Synergy Engine layer for FScoringStep (Task 2.1).
 * Payout = (Base Chips [Cyan]) * (Throw Mult [Magenta]) * (Bet Mult [Gold])
 */
UENUM(BlueprintType)
enum class EScoringLayer : uint8
{
	BaseChips   UMETA(DisplayName = "Layer 1: Base Chips (Cyan #00F5D4)"),
	ThrowMult   UMETA(DisplayName = "Layer 2: Throw Mult (Magenta #FF007F)"),
	BetMult     UMETA(DisplayName = "Layer 3: Bet Mult (Gold #FFB703)"),
	SevenOut    UMETA(DisplayName = "Danger: Seven-Out Wipe (Crimson #FF1E42)"),
	PointBonus  UMETA(DisplayName = "Jackpot: Point Hit Cashout (Gold #FFB703)")
};

/**
 * Craps table state machine phase (Task 2.2).
 */
UENUM(BlueprintType)
enum class ECrapsTablePhase : uint8
{
	ComeOutRoll      UMETA(DisplayName = "Come-Out Roll (Puck OFF)"),
	PointEstablished UMETA(DisplayName = "Point Phase (Puck ON: 4,5,6,8,9,10)")
};

/**
 * Felt bet zone types on the Craps table (Task 2.2).
 */
UENUM(BlueprintType)
enum class ECrapsBetType : uint8
{
	PassLine UMETA(DisplayName = "Pass Line"),
	DontPass UMETA(DisplayName = "Don't Pass Bar"),
	Field    UMETA(DisplayName = "The Field (2, 3, 4, 9, 10, 11, 12)"),
	Place4   UMETA(DisplayName = "Place 4"),
	Place5   UMETA(DisplayName = "Place 5"),
	Place6   UMETA(DisplayName = "Place 6"),
	Place8   UMETA(DisplayName = "Place 8"),
	Place9   UMETA(DisplayName = "Place 9"),
	Place10  UMETA(DisplayName = "Place 10"),
	Hard4    UMETA(DisplayName = "Hard 4 (2+2)"),
	Hard6    UMETA(DisplayName = "Hard 6 (3+3)"),
	Hard8    UMETA(DisplayName = "Hard 8 (4+4)"),
	Hard10   UMETA(DisplayName = "Hard 10 (5+5)"),
	Horn     UMETA(DisplayName = "Horn Bet (2, 3, 11, 12)")
};

/**
 * Single sequential scoring step pushed into UScoringQueueSubsystem (Task 2.1).
 * NEVER resolve end-of-roll scoring in an instant synchronous loop!
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FScoringStep
{
	GENERATED_BODY()

	/** Actor that triggered this scoring step (ADieBase, ABetZone, ATableHazard, or Left-Hand Trinket). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** World location where the floating score popup & voxel burst should appear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FVector PopupWorldLocation = FVector::ZeroVector;

	/** Optional socket name on SourceActor for visual pulse/trail attachment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FName VisualSocketName = NAME_None;

	/** Short label shown in the Balatro-style popup (e.g., "+40 CHIPS [DIE 1]", "+1.5x THROW [BUMPER]", "x2.0 BET [HARD 8]"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FString StepLabel;

	/** Which layer of the 3-layer equation this step modifies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	EScoringLayer Layer = EScoringLayer::BaseChips;

	/** Additive delta to Base Chips (Cyan). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int32 BaseChipDelta = 0;

	/** Additive delta to Throw Mult (Magenta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float ThrowMultDelta = 0.0f;

	/** Additive delta to Bet Mult (Gold). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float BetMultAdditiveDelta = 0.0f;

	/** Multiplicative factor applied to Bet Mult or Throw Mult (1.0f = no multiplicative scaling, 2.0f = x2 Mult). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float MultiplierFactor = 1.0f;

	/** Sequential audio pitch index (+1 semitone per step, wrapping/clamping at +24). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int32 AudioPitchIndex = 0;
};

/**
 * Contextual state of a settled throw passed into Trinkets and Scoring steps for synergy evaluation.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FRollEvaluationContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 Die1Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 Die2Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 RollSum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bIsHardway = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bHitBackWall = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 TotalBounces = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 MidRollHazardsHit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	ECrapsTablePhase TablePhase = ECrapsTablePhase::ComeOutRoll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	int32 EstablishedPoint = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bPointHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bSevenOut = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bNatural = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	bool bCraps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Evaluation")
	FGameplayTagContainer TriggerTags;
};


/**
 * Trinket slot categories on the Left-Hand anchor (Task 3.1 / 3.2).
 */
UENUM(BlueprintType)
enum class ETrinketSlotType : uint8
{
	FingerRing      UMETA(DisplayName = "Finger Ring"),
	WristwearActive UMETA(DisplayName = "Wristwear (Active)"),
	RailProp        UMETA(DisplayName = "Rail Prop")
};

/**
 * A shooter position around the table. Local values are relative to the table actor.
 * The pawn faces PawnLocalRotation and throws toward TargetBackWallLocalCenter.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FCrapsShooterStation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FString StationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FVector PawnLocalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FRotator PawnLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FVector DiceStagingLocalPos = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FVector StickmanOriginLocalPos = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FVector TargetBackWallLocalCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FRotator SettingCameraLocalRot = FRotator(-40.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Station")
	FRotator ThrowCameraLocalRot = FRotator(-22.0f, 0.0f, 0.0f);
};

/**
 * Detailed step contributor in the Balatro-style roll scoring breakdown.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FRollBreakdownItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FString SourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FString Detail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float MultiplierDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float MultiplierFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int64 ChipDelta = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	FLinearColor AccentColor = FLinearColor::White;
};

/**
 * Full Balatro-style mathematical scoring breakdown for the settled roll.
 */
USTRUCT(BlueprintType)
struct CRAPSCRAPSCRAPS_API FRollScoringBreakdown
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int32 Die1Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int32 Die2Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int32 RollSum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int64 TotalBaseChips = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float FinalThrowMult = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float FinalBetMult = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	float CombinedMult = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	int64 TotalPayout = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	TArray<FRollBreakdownItem> Steps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Scoring")
	bool bHasData = false;
};
