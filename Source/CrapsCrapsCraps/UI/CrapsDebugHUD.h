// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Core/CrapsTypes.h"
#include "CrapsDebugHUD.generated.h"

/**
 * CRT Synth-Noir Testing Pit Overlay HUD (Milestones 1 & 2).
 * Renders the 3-Layer Balatro Scoring Bar, Come-Out / Point Puck Banner, Bankroll & Unbanked Combo Pool,
 * 3D World-to-Screen Bet Zone / Pop-Up Labels, and live Throw / Rigging controls.
 */
UCLASS()
class CRAPSCRAPSCRAPS_API ACrapsDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACrapsDebugHUD();

	virtual void DrawHUD() override;

	/** Toggle the detailed developer rigging & physics controls panel (Key H). Defaults to collapsed/minimal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|UI")
	bool bShowDebugControls = false;

	UFUNCTION(BlueprintCallable, Category = "Craps|UI")
	void ToggleDebugControls() { bShowDebugControls = !bShowDebugControls; }

	/** Toggle streamlined minimal HUD mode. When true, uses compact cards and smart label filtering. Defaults to true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|UI")
	bool bStreamlinedHUD = true;

	UFUNCTION(BlueprintCallable, Category = "Craps|UI")
	void ToggleStreamlinedHUD() { bStreamlinedHUD = !bStreamlinedHUD; }

	/** Toggle the interactive synth-noir betting board panel (Key B / Tab). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|UI")
	bool bBettingBoardOpen = false;

	UFUNCTION(BlueprintCallable, Category = "Craps|UI")
	void ToggleBettingBoard() { bBettingBoardOpen = !bBettingBoardOpen; }

	UFUNCTION(BlueprintCallable, Category = "Craps|UI")
	void SetBettingBoardOpen(bool bOpen) { bBettingBoardOpen = bOpen; }

	UFUNCTION(BlueprintPure, Category = "Craps|UI")
	bool IsBettingBoardOpen() const { return bBettingBoardOpen; }

	/** Handles mouse click on the betting board overlay. Returns true if consumed. */
	UFUNCTION(BlueprintCallable, Category = "Craps|UI")
	bool HandleBettingBoardClick(const FVector2D& ScreenPos, bool bIsRightClick);

protected:
	enum class EBetBoardActionType : uint8
	{
		BetSlot,
		PressAll,
		ClearBets,
		BankPool,
		Close
	};

	struct FCrapsBetSlotClickArea
	{
		EBetBoardActionType ActionType = EBetBoardActionType::BetSlot;
		ECrapsBetType BetType = ECrapsBetType::PassLine;
		FString DisplayName;
		float X = 0.0f;
		float Y = 0.0f;
		float W = 0.0f;
		float H = 0.0f;
	};

	TArray<FCrapsBetSlotClickArea> CachedBetSlotClickAreas;
	FVector2D CachedPanelTopLeft = FVector2D::ZeroVector;
	FVector2D CachedPanelSize = FVector2D::ZeroVector;

	void DrawBettingBoard(
		UWorld* World,
		class ACrapsPawn* CrapsPawn,
		class ACrapsGameState* CrapsGS,
		class URunProgressionSubsystem* Progression,
		UFont* Font,
		float UiScale);
};
