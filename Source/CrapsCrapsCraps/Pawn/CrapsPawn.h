// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/CrapsTypes.h"
#include "CrapsPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputMappingContext;
class UInputAction;
class ADieBase;
class UThrowStyleComponentBase;
class UThrowStyle_TableSkimmer;
class UThrowStyle_ParabolicLob;
class UThrowStyle_BackWallWhip;
class UThrowStyle_MechanicalPlunger;
class ACrapsTableGrayboxActor;
class ABetZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnRollResolvedSignature,
	int32, Die1Value,
	int32, Die2Value,
	int32, RollSum
);

/**
 * First-Person POV Shooter Pawn for CRT Synth-Noir Craps (Tasks 1.3 & 1.4).
 * Handles Stickman push-back, 3 On-Felt Dice Setting methods (Finger-Tip, V-Set Scroll, 1/2/3 Preset Snap),
 * 4 hot-swappable Throw Archetypes (F1-F4), and 3 Input Modes (Mouse Flick, Arcade Meter, Slingshot Pull).
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ACrapsPawn : public APawn
{
	GENERATED_BODY()

public:
	ACrapsPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//~ Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<USceneComponent> PawnRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<UCameraComponent> POVCamera;

	/** Left-Hand anchor scene root resting on the bottom-left leather rail (socket parent for Milestone 3). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<USceneComponent> LeftHandRailAnchor;

	/** Right-Hand shooter anchor above the staging felt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<USceneComponent> RightHandShooterAnchor;

	/** Graybox Left-Hand visual mesh resting on the left leather rail. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<class UStaticMeshComponent> LeftHandVisualMesh;

	/** Graybox Right-Hand visual mesh hovering beside the staged dice. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<class UStaticMeshComponent> RightHandVisualMesh;

	/** Overhead pit light so the table, hands, and 20cm dice are clearly lit in any map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Components")
	TObjectPtr<class UPointLightComponent> PitOverheadLight;

	//~ 4 Modular Throw Style Components (Hot-Swappable via F1-F4)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|ThrowStyles")
	TObjectPtr<UThrowStyle_TableSkimmer> ThrowStyle_TableSkimmer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|ThrowStyles")
	TObjectPtr<UThrowStyle_ParabolicLob> ThrowStyle_ParabolicLob;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|ThrowStyles")
	TObjectPtr<UThrowStyle_BackWallWhip> ThrowStyle_BackWallWhip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|ThrowStyles")
	TObjectPtr<UThrowStyle_MechanicalPlunger> ThrowStyle_MechanicalPlunger;

	//~ Optional Enhanced Input Assets (with automatic C++ fallback bindings if unassigned)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	//~ State & Configuration (10x Scale)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|State")
	ECrapsThrowPhase CurrentPhase = ECrapsThrowPhase::StickmanReturn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Setting")
	EDiceSetMethod ActiveDiceSetMethod = EDiceSetMethod::FingerTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Throw")
	EThrowArchetype ActiveThrowArchetype = EThrowArchetype::ParabolicLob;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Throw")
	EThrowInputMode ActiveInputMode = EThrowInputMode::SlingshotPull;

	/** Index of selected die for single-die Finger-Tip rotation: 0 = Left Die, 1 = Right Die, 2 = Both Dice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Setting")
	int32 SelectedDieIndex = 2;

	/** Class to spawn for the pair of 20cm dice if none are present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Dice")
	TSubclassOf<ADieBase> DieClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Dice")
	TArray<TObjectPtr<ADieBase>> ActiveDice;

	/** Center-table location where the Stickman pushes the dice from (10x scale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Staging")
	FVector StickmanOriginWorldPos = FVector(850.0f, 0.0f, 11.0f);

	/** Shooter's pre-throw on-felt staging center directly in front of the POV camera (10x scale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Staging")
	FVector ShooterStagingWorldPos = FVector(190.0f, 0.0f, 11.0f);

	/** Lateral spacing between the two 20cm dice on the felt (default 34cm center-to-center). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Staging")
	float DiceLateralSpacingCm = 34.0f;

	/** Duration of the Stickman push-back slide animation in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Staging")
	float StickmanPushDuration = 0.45f;

	/** Duration of the smooth 90-degree on-felt rotation slerp in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Setting")
	float DiceSetRotationInterpSpeed = 16.0f;

	/** Live Physics Rigging Slider [0.0 .. 1.0] synced to both dice's UDiceRiggingComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlobalDiceControlStat = 0.35f;

	/** Active Rigging Mode synced to both dice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Debug")
	EDiceRiggingMode GlobalRiggingMode = EDiceRiggingMode::TargetSetFace;

	/** Live Table Surface Friction Slider [0.0 = Ice Felt .. 1.0 = Heavy Velvet]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TableSurfaceFriction = 0.55f;

	/** Current launch parameters being charged/aimed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Throw")
	FThrowLaunchParams CurrentLaunchParams;

	/** Last resolved roll values for the HUD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|State")
	int32 LastDie1Value = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|State")
	int32 LastDie2Value = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|State")
	int32 LastRollSum = 0;

	UPROPERTY(BlueprintAssignable, Category = "Craps|Pawn|Events")
	FOnRollResolvedSignature OnRollResolved;

	//~ Public Callable Actions (Tasks 1.3 & 1.4)
	/** Spawns or recalls the two 20cm dice via the Stickman push-back across the black velvet felt. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn")
	void RequestStickmanPushBack();

	/** Returns whether the player is currently actively charging a throw. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Throw")
	bool IsChargingThrow() const { return bIsChargingThrow; }

	/** Minimum forward table-local distance (cm) dice must reach for a throw to be valid (prevents short throws). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Rules")
	float ShortThrowMinTableX = 650.0f;

	/** Handles a No-Roll outcome: voids scoring, preserves wagers/rolls, and recalls dice cleanly. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn")
	void HandleNoRollThrow(const FString& Reason);

	/** Rotates the selected die(s) on the felt by 90-degree increments around Pitch/Yaw/Roll. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Setting")
	void RotateDiceOnFelt90Degrees(const FVector& WorldAxis, float AngleSign = 1.0f);

	/** V-Set Locked-Axis Scroll: rotates both dice synchronously along their shared horizontal pitch axis. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Setting")
	void ScrollVSetAxis(float ScrollDirection);

	/**
	 * Holographic Preset Snap (Keys 1, 2, 3):
	 * PresetIndex 1 = Hardways (4-4 Top, 2-2 Front)
	 * PresetIndex 2 = All-Sevens / Natural (3-4 Top, 2-5 Front)
	 * PresetIndex 3 = 3-V Point Hunter (3-3 Top, V-angled Front)
	 */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Setting")
	void ApplyPresetDiceSnap(int32 PresetIndex);

	/** Hot-swaps the active Right-Hand Throw Archetype (F1-F4). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Throw")
	void SetActiveThrowArchetype(EThrowArchetype NewArchetype);

	/** Cycles the active On-Felt Dice Setting method (Q key). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Setting")
	void CycleDiceSetMethod();

	/** Cycles the active Throw Input Mode (Tab key: Mouse Flick / Arcade Meter / Slingshot Pull). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Throw")
	void CycleThrowInputMode();

	/** Adjusts the GlobalDiceControlStat rigging slider by Delta ([-1.0 .. +1.0]) and updates both dice. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Debug")
	void AdjustDiceControlStat(float Delta);

	/** Toggles between TargetSetFace and AvoidSevenOut rigging modes (G key). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Debug")
	void ToggleRiggingMode();

	/** Adjusts the Table Surface Friction slider by Delta and updates the Graybox Table's PhysicalMaterial. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Debug")
	void AdjustTableFriction(float Delta);

	/** Returns the currently active UThrowStyleComponentBase implementing IThrowStyleInterface. */
	UFUNCTION(BlueprintPure, Category = "Craps|Pawn|Throw")
	UThrowStyleComponentBase* GetActiveThrowStyleComponent() const;

	/** Fires the dice using the active IThrowStyleInterface component and CurrentLaunchParams. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Throw")
	void CommitThrow();

	/** Enables Remote Python Execution on URemoteControlSettings CDO at runtime so MCP can run Python scripts without Editor restart. */
	UFUNCTION(BlueprintCallable, Category = "Craps|MCP")
	void EnableRemotePythonBridge();

	/** Index into ACrapsTableGrayboxActor::ShooterStations the pawn currently occupies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Pawn|Station")
	int32 CurrentStationIndex = 0;

	/** Table the pawn is seated at (resolved in BeginPlay). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Craps|Pawn|Station")
	TObjectPtr<ACrapsTableGrayboxActor> TableActor;

	/** Moves to the next/previous shooter station (PageUp/PageDown/M). Ignored while dice are in flight. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Station")
	void CycleShooterStation(int32 Direction);

	/** Teleports the pawn to a station, rotates it to face the opposite wall, and re-stages the dice. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Station")
	void ApplyShooterStation(int32 StationIndex);

	UFUNCTION(BlueprintPure, Category = "Craps|Pawn|Station")
	FString GetCurrentStationName() const;

	/** Converts a pawn-local offset (+X = throw direction) to world space. */
	FVector PawnLocalToWorld(const FVector& LocalOffset) const;

	//~ Targeted POV Bet Placement & Chip Denominations
	/** Returns the currently hovered bet zone under the mouse cursor. */
	UFUNCTION(BlueprintPure, Category = "Craps|Pawn|Betting")
	ABetZone* GetHoveredBetZone() const { return HoveredBetZone; }

	/** Currently selected chip denomination for placement ($10, $25, $50, $100, $500). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Pawn|Betting")
	int64 SelectedChipDenomination = 50;

	/** Cycles through available chip denominations: $10, $25, $50, $100, $500. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Betting")
	void CycleChipDenomination(int32 Direction = 1);

	/** Sets active chip denomination directly. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Pawn|Betting")
	void SetChipDenomination(int64 NewDenomination);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Craps|Pawn|Betting")
	TObjectPtr<ABetZone> HoveredBetZone = nullptr;

	UFUNCTION()
	void HandleSingleDieSettled(ADieBase* Die, EDieFaceDirection TopDirection, int32 TopValue, UPDA_DiceFace* TopFaceData);

private:
	void EnsureDiceSpawned();
	void SyncRiggingSettingsToDice();
	void UpdateStickmanPushBack(float DeltaTime);
	void UpdateOnFeltDiceRotationSlerp(float DeltaTime);
	void UpdateThrowChargingAndTrajectory(float DeltaTime);
	void UpdateCursorHoverRaycast();

	// Input Handlers
	void Input_PitchUp();
	void Input_PitchDown();
	void Input_YawLeft();
	void Input_YawRight();
	void Input_RollLeft();
	void Input_RollRight();
	void Input_MouseWheel(const struct FInputActionValue& Value);
	void Input_MouseWheelUp();
	void Input_MouseWheelDown();
	void Input_RightClickPress();
	void Input_CycleDenominationUp();
	void Input_CycleDenominationDown();
	void Input_Preset1();
	void Input_Preset2();
	void Input_Preset3();
	void Input_SelectDieCycle();
	void Input_ArchetypeF1();
	void Input_ArchetypeF2();
	void Input_ArchetypeF3();
	void Input_ArchetypeF4();
	void Input_ThrowPress();
	void Input_ThrowRelease();
	void Input_RiggingDecrease();
	void Input_RiggingIncrease();
	void Input_FrictionDecrease();
	void Input_FrictionIncrease();
	void Input_BankPool();
	void Input_StationNext();
	void Input_StationPrev();
	void Input_RebetAll();
	void Input_AdvanceAnte();
	void Input_RestartRun();
	void Input_ToggleHUDHelp();
	void Input_ClearBets();

	float StickmanPushElapsed = 0.0f;
	TArray<FQuat> DesiredDieOrientations;
	bool bIsChargingThrow = false;
	float ArcadeMeterPhase = 0.0f;
	FVector2D ThrowPressMousePos = FVector2D::ZeroVector;
	FVector2D PrevMousePos = FVector2D::ZeroVector;
	FVector2D SmoothedMouseVelocity = FVector2D::ZeroVector;
	float FlightTimeoutTimer = 0.0f;
	float MaxThrowDistanceReached = 0.0f;
};
