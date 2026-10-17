// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Progression/PDA_RunProgressionConfig.h"

UPDA_RunProgressionConfig::UPDA_RunProgressionConfig()
{
	StartingSpendableBankroll = 500;

	// Default 8-Ante Progression Ladder (Provisional 12-16 roll allowance)
	Antes.Empty();

	FAnteConfig Ante1;
	Ante1.AnteIndex = 1;
	Ante1.AnteTitle = TEXT("Ante 1");
	Ante1.AnteDescription = TEXT("Low-Stakes Warmup");
	Ante1.TargetWinningsQuota = 1000;
	Ante1.RollAllowance = 15;
	Antes.Add(Ante1);

	FAnteConfig Ante2;
	Ante2.AnteIndex = 2;
	Ante2.AnteTitle = TEXT("Ante 2");
	Ante2.AnteDescription = TEXT("Mid-Stakes Lounge");
	Ante2.TargetWinningsQuota = 2500;
	Ante2.RollAllowance = 15;
	Antes.Add(Ante2);

	FAnteConfig Ante3;
	Ante3.AnteIndex = 3;
	Ante3.AnteTitle = TEXT("Ante 3");
	Ante3.AnteDescription = TEXT("Underground Pit");
	Ante3.TargetWinningsQuota = 5000;
	Ante3.RollAllowance = 14;
	Antes.Add(Ante3);

	FAnteConfig Ante4;
	Ante4.AnteIndex = 4;
	Ante4.AnteTitle = TEXT("Ante 4");
	Ante4.AnteDescription = TEXT("High-Limit Velvet Room");
	Ante4.TargetWinningsQuota = 8500;
	Ante4.RollAllowance = 14;
	Antes.Add(Ante4);

	FAnteConfig Ante5;
	Ante5.AnteIndex = 5;
	Ante5.AnteTitle = TEXT("Ante 5");
	Ante5.AnteDescription = TEXT("Neon Pinball Foundry");
	Ante5.TargetWinningsQuota = 14000;
	Ante5.RollAllowance = 14;
	Antes.Add(Ante5);

	FAnteConfig Ante6;
	Ante6.AnteIndex = 6;
	Ante6.AnteTitle = TEXT("Ante 6");
	Ante6.AnteDescription = TEXT("Cybernetic VIP Annex");
	Ante6.TargetWinningsQuota = 22000;
	Ante6.RollAllowance = 13;
	Antes.Add(Ante6);

	FAnteConfig Ante7;
	Ante7.AnteIndex = 7;
	Ante7.AnteTitle = TEXT("Ante 7");
	Ante7.AnteDescription = TEXT("The Penultimate Floor");
	Ante7.TargetWinningsQuota = 35000;
	Ante7.RollAllowance = 13;
	Antes.Add(Ante7);

	FAnteConfig Ante8;
	Ante8.AnteIndex = 8;
	Ante8.AnteTitle = TEXT("Ante 8");
	Ante8.AnteDescription = TEXT("Grand Pit Boss Finale");
	Ante8.TargetWinningsQuota = 50000;
	Ante8.RollAllowance = 12;
	Antes.Add(Ante8);
}

FAnteConfig UPDA_RunProgressionConfig::GetAnteConfig(int32 AnteIndex) const
{
	const int32 ArrayIdx = AnteIndex - 1;
	if (Antes.IsValidIndex(ArrayIdx))
	{
		return Antes[ArrayIdx];
	}

	if (Antes.Num() > 0)
	{
		// Fallback: scale exponentially beyond configured array
		FAnteConfig Fallback = Antes.Last();
		Fallback.AnteIndex = AnteIndex;
		Fallback.AnteTitle = FString::Printf(TEXT("Ante %d (Endless)"), AnteIndex);
		Fallback.TargetWinningsQuota = static_cast<int64>(Fallback.TargetWinningsQuota * 1.5);
		Fallback.RollAllowance = FMath::Max(10, Fallback.RollAllowance - 1);
		return Fallback;
	}

	FAnteConfig DefaultFallback;
	DefaultFallback.AnteIndex = AnteIndex;
	DefaultFallback.TargetWinningsQuota = 1000 * FMath::Max(1, AnteIndex);
	DefaultFallback.RollAllowance = 15;
	return DefaultFallback;
}
