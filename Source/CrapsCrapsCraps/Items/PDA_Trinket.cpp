// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Items/PDA_Trinket.h"

UPDA_Trinket::UPDA_Trinket()
{
}

bool UPDA_Trinket::EvaluateTrinket(
	const FRollEvaluationContext& Context,
	FScoringStep& OutScoringStep,
	int32& OutBonusBaseChips,
	float& OutBonusThrowMult,
	float& OutBetMultFactor) const
{
	OutBonusBaseChips = 0;
	OutBonusThrowMult = 0.0f;
	OutBetMultFactor = 1.0f;

	// 1. Miracle Seven check is handled separately during Seven-Out resolution
	if (bIsMiracleSevenTrinket)
	{
		return false;
	}

	// 2. Odd / Even sum splitting (e.g. Neon Signet)
	if (bSplitOddEven)
	{
		const bool bIsOdd = (Context.RollSum % 2 != 0);
		if (bIsOdd && OddSumThrowMult > 0.0f)
		{
			OutBonusThrowMult = OddSumThrowMult;
			OutScoringStep.StepLabel = FString::Printf(TEXT("[%s] ODD ROLL (+%.1fx THROW MULT)"), *TrinketName.ToString().ToUpper(), OddSumThrowMult);
			OutScoringStep.Layer = EScoringLayer::ThrowMult;
			OutScoringStep.ThrowMultDelta = OddSumThrowMult;
			return true;
		}
		else if (!bIsOdd && EvenSumBaseChips > 0)
		{
			OutBonusBaseChips = EvenSumBaseChips;
			OutScoringStep.StepLabel = FString::Printf(TEXT("[%s] EVEN ROLL (+%d CHIPS)"), *TrinketName.ToString().ToUpper(), EvenSumBaseChips);
			OutScoringStep.Layer = EScoringLayer::BaseChips;
			OutScoringStep.BaseChipDelta = EvenSumBaseChips;
			return true;
		}
		return false;
	}

	// 3. Bounce scaling (e.g. Brass Knuckles)
	if (bScaleWithBounces)
	{
		if (Context.TotalBounces > 0)
		{
			OutBonusBaseChips = ChipsPerBounce * Context.TotalBounces;
			OutBonusThrowMult = ThrowMultPerBounce * Context.TotalBounces;
			OutScoringStep.StepLabel = FString::Printf(TEXT("[%s] %d BOUNCES (+%d CHIPS, +%.2fx MULT)"),
				*TrinketName.ToString().ToUpper(), Context.TotalBounces, OutBonusBaseChips, OutBonusThrowMult);
			OutScoringStep.Layer = EScoringLayer::BaseChips;
			OutScoringStep.BaseChipDelta = OutBonusBaseChips;
			OutScoringStep.ThrowMultDelta = OutBonusThrowMult;
			return true;
		}
		return false;
	}

	// 4. Standard GameplayTag matching
	bool bTriggered = false;
	if (TriggerConditionTag.IsValid())
	{
		bTriggered = Context.TriggerTags.HasTag(TriggerConditionTag);
	}
	else
	{
		bTriggered = true; // Untagged trinkets apply passively on all rolls
	}

	if (!bTriggered)
	{
		return false;
	}

	OutBonusBaseChips = BonusBaseChips;
	OutBonusThrowMult = BonusThrowMult;
	OutBetMultFactor = BetMultFactor;

	OutScoringStep.StepLabel = FString::Printf(TEXT("[%s] SYNERGY!"), *TrinketName.ToString().ToUpper());
	OutScoringStep.Layer = TargetScoringLayer;
	OutScoringStep.BaseChipDelta = BonusBaseChips;
	OutScoringStep.ThrowMultDelta = BonusThrowMult;
	OutScoringStep.MultiplierFactor = BetMultFactor;

	return true;
}

bool UPDA_Trinket::CheckMiracleSevenAverted(
	const FRollEvaluationContext& Context,
	FScoringStep& OutScoringStep,
	int32& OutBonusBaseChips) const
{
	OutBonusBaseChips = 0;
	if (!bIsMiracleSevenTrinket)
	{
		return false;
	}

	// Must be rolling 7 during PointEstablished
	if (Context.RollSum != 7 || Context.TablePhase != ECrapsTablePhase::PointEstablished)
	{
		return false;
	}

	// Requires minimum hazard impacts (bumpers/scatterers) during the throw
	if (Context.MidRollHazardsHit < MinHazardsForMiracleSeven)
	{
		return false;
	}

	OutBonusBaseChips = MiracleSevenBonusChips;
	OutScoringStep.StepLabel = FString::Printf(TEXT("[%s] MIRACLE SEVEN! (+%d CHIPS) SEVEN-OUT AVERTED!"),
		*TrinketName.ToString().ToUpper(), MiracleSevenBonusChips);
	OutScoringStep.Layer = EScoringLayer::PointBonus;
	OutScoringStep.BaseChipDelta = MiracleSevenBonusChips;
	return true;
}
