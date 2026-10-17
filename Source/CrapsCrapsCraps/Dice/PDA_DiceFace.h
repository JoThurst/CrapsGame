// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PDA_DiceFace.generated.h"

class UTexture2D;
class UMaterialInterface;

/**
 * Data-driven definition for a customizable Die Face (Task 1.1).
 * Never hardcode special face effects in ADieBase; use UPDA_DiceFace + GameplayTags.
 */
UCLASS(BlueprintType)
class CRAPSCRAPSCRAPS_API UPDA_DiceFace : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPDA_DiceFace();

	/** Display name of the die face (e.g., "Standard 6", "Neon Wild (?)", "Splitter Face"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	FText FaceName;

	/** Description shown in the Bone Surgeon shop tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace", meta = (MultiLine = "true"))
	FText FaceDescription;

	/** Primary integer pip value (1-6 for standard, 0 for Zero/Gold Bullion face). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	int32 FaceValue = 1;

	/** Optional secondary pip value (e.g. 12 for Double-Pip 6/6 face; 0 if unused). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	int32 SecondaryFaceValue = 0;

	/** Cyan Base Chips added when this face settles face-up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	int32 BaseChipBonus = 10;

	/** Magenta Throw Multiplier bonus added when this face triggers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	float ThrowMultBonus = 0.0f;

	/** Direct bankroll payout ($ value, e.g., +$15 for Gold Bullion Face). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	int32 DirectCashBonus = 0;

	/** GameplayTags driving special behaviors (e.g., Craps.Face.Wild, Craps.Face.Splitter, Craps.Face.Repulsor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace")
	FGameplayTagContainer FaceAbilityTags;

	/** Emissive color tint for this face's pips on the 20cm die. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace|Visuals")
	FLinearColor PipEmissiveColor = FLinearColor(0.0f, 0.96f, 0.83f, 1.0f);

	/** Optional icon texture for HUD and shop inspection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace|Visuals")
	TObjectPtr<UTexture2D> FaceIcon = nullptr;

	/** Optional custom decal/material instance for the die socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craps|DiceFace|Visuals")
	TObjectPtr<UMaterialInterface> FaceMaterialOverride = nullptr;

	//~ Begin UPrimaryDataAsset Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset Interface
};
