// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dice/PDA_DiceFace.h"

UPDA_DiceFace::UPDA_DiceFace()
{
	FaceName = FText::FromString(TEXT("Standard Pip Face"));
	FaceValue = 1;
	SecondaryFaceValue = 0;
	BaseChipBonus = 10;
	ThrowMultBonus = 0.0f;
	DirectCashBonus = 0;
}

FPrimaryAssetId UPDA_DiceFace::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("DiceFace")), GetFName());
}
