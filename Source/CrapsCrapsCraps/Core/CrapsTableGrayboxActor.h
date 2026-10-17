// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/CrapsTypes.h"
#include "CrapsTableGrayboxActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UBoxComponent;
class UPhysicalMaterial;
class UMaterialInstanceDynamic;
class ACrapsGameState;

/**
 * 10x Scale Craps Table Pit Graybox Actor for /Game/Maps/L_TestingPit.
 * Features:
 * - Flush, gap-free felt bed (X: -150..1650, Y: -440..+440, Top Surface Z = 0)
 * - Deep bathtub side/front/back walls + Top Padded Rail Overhang Lip + Invisible Ceiling Containment Box
 * - Real 3D Alligator Pyramid Rubber Diamond rows along the back wall
 * - Chamfered (rounded-tub) corner walls, double-tier leather rails and chip-rack grooves
 * - Editor-visible casino felt layout (Pass Line, Don't Pass, Field, Come, Big 6/8, Point Boxes,
 *   center proposition box) built in OnConstruction, a 3D ON/OFF dealer puck, stick and chip banks
 * - Shooter stations around the perimeter (the pawn throws toward the opposite wall)
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ACrapsTableGrayboxActor : public AActor
{
	GENERATED_BODY()

public:
	ACrapsTableGrayboxActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<USceneComponent> TableRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> FeltBedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> BackWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> BackOverhangRailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> FrontWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> LeftWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> LeftOverhangRailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> RightWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UStaticMeshComponent> RightOverhangRailMesh;

	/** 3D diamond pyramid bumper ribs across the back wall ("Alligator Padding"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> AlligatorPyramidMeshes;

	/** Four angled corner wall segments that round off the pit ends (rounded tub). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> CornerChamferMeshes;

	/** Invisible ceiling containment collider so extreme lobs/bounces stay inside the pit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TObjectPtr<UBoxComponent> PitCeilingContainmentBox;

	/** Four invisible upper containment wall colliders (Z: 120..420cm) preventing out-of-bounds throws. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Components")
	TArray<TObjectPtr<UBoxComponent>> UpperContainmentBoxes;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Craps|Table|Physics")
	TObjectPtr<UPhysicalMaterial> FeltPhysicalMaterial;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Craps|Table|Physics")
	TObjectPtr<UPhysicalMaterial> BackWallPhysicalMaterial;

	/** Shooter positions around the table. Local to this actor; the pawn faces PawnLocalRotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|Table|Stations")
	TArray<FCrapsShooterStation> ShooterStations;

	/** Updates the felt surface friction in real time [0.0 = Ice Felt .. 1.0 = Heavy Velvet]. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Physics")
	void SetFeltSurfaceFriction(float NormalizedFriction);

	/** Moves the 3D dealer puck to the given point box (0 = OFF / come-out). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Layout")
	void SetDealerPuckState(bool bPuckOn, int32 PointNumber);

	/** Ensures point box centers, meshes, dealer puck, and scoreboard references are resolved and valid. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Layout")
	void EnsureLayoutResolved();

	/** Binds table delegates to the authoritative CrapsGameState and synchronizes visual state. */
	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Layout")
	void BindToGameState(ACrapsGameState* GS);

	/** Updates the 4 CRT scoreboard displays (Bankroll, Ante, Unbanked Pool, Multiplier). */
	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Scoreboard")
	void UpdateScoreboardReadouts(int64 Bankroll, int64 AnteQuota, int64 UnbankedPool, int32 BaseChips, float ThrowMult, float BetMult, ECrapsTablePhase Phase, int32 PointNumber);

	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Delegates")
	void HandleTablePhaseChanged(ECrapsTablePhase NewPhase, int32 NewPoint);

	UFUNCTION(BlueprintCallable, Category = "Craps|Table|Delegates")
	void HandleBankrollChanged(int64 NewBankroll, int64 NewUnbankedPool);

	UFUNCTION()
	void HandleScoringStepExecuted(const FScoringStep& Step, int32 RunningBaseChips, float RunningThrowMult, float RunningBetMult, float NormalizedMultiplierIntensity);

	UFUNCTION()
	void HandleScoringSequenceFinished(int64 FinalThrowPayout, bool bWasSevenOut);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Layout")
	TObjectPtr<UStaticMeshComponent> DealerPuckMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Layout")
	TObjectPtr<UTextRenderComponent> DealerPuckText;

	// ---- Overhead CRT Scoreboard Mesh & 4 Live Display Bays ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UStaticMeshComponent> ScoreboardEnclosureMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UTextRenderComponent> BankrollValueText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UTextRenderComponent> AnteValueText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UTextRenderComponent> UnbankedPoolValueText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UTextRenderComponent> MultiplierValueText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Scoreboard")
	TObjectPtr<UTextRenderComponent> MultiplierSubText;

	/** Table-local XY center of each point box on the south half (4, 5, SIX, 8, NINE, 10). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craps|Table|Layout")
	TArray<FVector2D> PointBoxCenters;

private:
	// ---- Editor-visible felt layout & CRT scoreboard (rebuilt in OnConstruction) ----
	void ClearGeneratedLayout();
	void BuildFeltLayout();
	void BuildTubDressing();
	void BuildCRTScoreboard();
	void ResolveScoreboardReferences();

	UStaticMeshComponent* AddLayoutMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, bool bCollide = false);
	void AddFeltLine(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Width = 3.0f);
	void AddFeltRect(const FVector2D& Center, const FVector2D& Size, const FLinearColor& Color, float Width = 3.0f);
	void AddFeltText(const FVector2D& Center, const FString& Text, float WorldSize, const FLinearColor& Color, bool bNorthFacing = false, bool bAlongX = false);
	UMaterialInstanceDynamic* GetColorMID(const FLinearColor& Color);

	/** Half-local (u along the long axis from the end wall, v across) to table-local XY. bNorth mirrors across the table center. */
	static FVector2D HalfToTable(float U, float V, bool bNorth);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedLayoutComponents;

	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<UMaterialInstanceDynamic>> ColorMIDCache;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CachedCubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CachedCylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CachedScoreboardMesh;
};
