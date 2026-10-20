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
};
