// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CrapsTestingGameMode.generated.h"

/**
 * GameMode for Milestone 1 (/Game/Maps/L_TestingPit).
 * Defaults to ACrapsPawn and ACrapsDebugHUD, and ensures an ACrapsTableGrayboxActor
 * exists in the level on BeginPlay.
 */
UCLASS(Blueprintable)
class CRAPSCRAPSCRAPS_API ACrapsTestingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACrapsTestingGameMode();

	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

protected:
	virtual void BeginPlay() override;
};
