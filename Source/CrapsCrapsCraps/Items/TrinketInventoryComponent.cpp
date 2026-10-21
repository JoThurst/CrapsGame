// Copyright CrapsCrapsCraps. All Rights Reserved.

#include "Items/TrinketInventoryComponent.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "GameFramework/Actor.h"

UTrinketInventoryComponent::UTrinketInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	EquippedFingerRings.Init(nullptr, MaxFingerRings);
	EquippedRailProps.Init(nullptr, MaxRailProps);
}

void UTrinketInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure slot arrays are cleanly sized
	if (EquippedFingerRings.Num() != MaxFingerRings)
	{
		EquippedFingerRings.SetNum(MaxFingerRings);
	}
	if (EquippedRailProps.Num() != MaxRailProps)
	{
		EquippedRailProps.SetNum(MaxRailProps);
	}
}

bool UTrinketInventoryComponent::EquipTrinket(UPDA_Trinket* Trinket, int32 SlotIndex)
{
	if (!Trinket)
	{
		return false;
	}

	// Prevent duplicate instances of the same trinket
	if (HasTrinket(Trinket))
	{
		return false;
	}

	switch (Trinket->SlotType)
	{
	case ETrinketSlotType::FingerRing:
	{
		int32 TargetIndex = SlotIndex;
		if (TargetIndex < 0 || TargetIndex >= MaxFingerRings)
		{
			// Find first available slot
			TargetIndex = -1;
			for (int32 i = 0; i < MaxFingerRings; ++i)
			{
				if (!EquippedFingerRings[i])
				{
					TargetIndex = i;
					break;
				}
			}
		}

		if (TargetIndex < 0 || TargetIndex >= MaxFingerRings)
		{
			return false; // All ring slots occupied
		}

		if (EquippedFingerRings[TargetIndex])
		{
			UnequipSlot(ETrinketSlotType::FingerRing, TargetIndex);
		}

		EquippedFingerRings[TargetIndex] = Trinket;
		OnTrinketEquipped.Broadcast(Trinket, ETrinketSlotType::FingerRing, TargetIndex);
		return true;
	}

	case ETrinketSlotType::WristwearActive:
	{
		if (EquippedWristwear)
		{
			UnequipSlot(ETrinketSlotType::WristwearActive, 0);
		}

		EquippedWristwear = Trinket;
		OnTrinketEquipped.Broadcast(Trinket, ETrinketSlotType::WristwearActive, 0);
		return true;
	}

	case ETrinketSlotType::RailProp:
	{
		int32 TargetIndex = SlotIndex;
		if (TargetIndex < 0 || TargetIndex >= MaxRailProps)
		{
			TargetIndex = -1;
			for (int32 i = 0; i < MaxRailProps; ++i)
			{
				if (!EquippedRailProps[i])
				{
					TargetIndex = i;
					break;
				}
			}
		}

		if (TargetIndex < 0 || TargetIndex >= MaxRailProps)
		{
			return false; // All prop slots occupied
		}

		if (EquippedRailProps[TargetIndex])
		{
			UnequipSlot(ETrinketSlotType::RailProp, TargetIndex);
		}

		EquippedRailProps[TargetIndex] = Trinket;
		OnTrinketEquipped.Broadcast(Trinket, ETrinketSlotType::RailProp, TargetIndex);
		return true;
	}

	default:
		break;
	}

	return false;
}

bool UTrinketInventoryComponent::UnequipTrinket(UPDA_Trinket* Trinket)
{
	if (!Trinket)
	{
		return false;
	}

	for (int32 i = 0; i < EquippedFingerRings.Num(); ++i)
	{
		if (EquippedFingerRings[i] == Trinket)
		{
			return UnequipSlot(ETrinketSlotType::FingerRing, i);
		}
	}

	if (EquippedWristwear == Trinket)
	{
		return UnequipSlot(ETrinketSlotType::WristwearActive, 0);
	}

	for (int32 i = 0; i < EquippedRailProps.Num(); ++i)
	{
		if (EquippedRailProps[i] == Trinket)
		{
			return UnequipSlot(ETrinketSlotType::RailProp, i);
		}
	}

	return false;
}

bool UTrinketInventoryComponent::UnequipSlot(ETrinketSlotType SlotType, int32 SlotIndex)
{
	UPDA_Trinket* RemovedTrinket = nullptr;

	switch (SlotType)
	{
	case ETrinketSlotType::FingerRing:
		if (EquippedFingerRings.IsValidIndex(SlotIndex) && EquippedFingerRings[SlotIndex])
		{
			RemovedTrinket = EquippedFingerRings[SlotIndex];
			EquippedFingerRings[SlotIndex] = nullptr;
		}
		break;

	case ETrinketSlotType::WristwearActive:
		if (EquippedWristwear)
		{
			RemovedTrinket = EquippedWristwear;
			EquippedWristwear = nullptr;
		}
		break;

	case ETrinketSlotType::RailProp:
		if (EquippedRailProps.IsValidIndex(SlotIndex) && EquippedRailProps[SlotIndex])
		{
			RemovedTrinket = EquippedRailProps[SlotIndex];
			EquippedRailProps[SlotIndex] = nullptr;
		}
		break;

	default:
		break;
	}

	if (RemovedTrinket)
	{
		OnTrinketUnequipped.Broadcast(RemovedTrinket, SlotType, SlotIndex);
		return true;
	}

	return false;
}

void UTrinketInventoryComponent::ClearAllTrinkets()
{
	for (int32 i = 0; i < EquippedFingerRings.Num(); ++i)
	{
		if (EquippedFingerRings[i])
		{
			UnequipSlot(ETrinketSlotType::FingerRing, i);
		}
	}

	if (EquippedWristwear)
	{
		UnequipSlot(ETrinketSlotType::WristwearActive, 0);
	}

	for (int32 i = 0; i < EquippedRailProps.Num(); ++i)
	{
		if (EquippedRailProps[i])
		{
			UnequipSlot(ETrinketSlotType::RailProp, i);
		}
	}
}

bool UTrinketInventoryComponent::HasTrinket(const UPDA_Trinket* Trinket) const
{
	if (!Trinket)
	{
		return false;
	}

	for (const UPDA_Trinket* Ring : EquippedFingerRings)
	{
		if (Ring == Trinket)
		{
			return true;
		}
	}

	if (EquippedWristwear == Trinket)
	{
		return true;
	}

	for (const UPDA_Trinket* Prop : EquippedRailProps)
	{
		if (Prop == Trinket)
		{
			return true;
		}
	}

	return false;
}

TArray<UPDA_Trinket*> UTrinketInventoryComponent::GetAllEquippedTrinkets() const
{
	TArray<UPDA_Trinket*> Result;

	for (UPDA_Trinket* Ring : EquippedFingerRings)
	{
		if (Ring)
		{
			Result.Add(Ring);
		}
	}

	if (EquippedWristwear)
	{
		Result.Add(EquippedWristwear);
	}

	for (UPDA_Trinket* Prop : EquippedRailProps)
	{
		if (Prop)
		{
			Result.Add(Prop);
		}
	}

	return Result;
}

UPDA_Trinket* UTrinketInventoryComponent::GetEquippedTrinket(ETrinketSlotType SlotType, int32 SlotIndex) const
{
	switch (SlotType)
	{
	case ETrinketSlotType::FingerRing:
		if (EquippedFingerRings.IsValidIndex(SlotIndex))
		{
			return EquippedFingerRings[SlotIndex];
		}
		break;

	case ETrinketSlotType::WristwearActive:
		return EquippedWristwear;

	case ETrinketSlotType::RailProp:
		if (EquippedRailProps.IsValidIndex(SlotIndex))
		{
			return EquippedRailProps[SlotIndex];
		}
		break;

	default:
		break;
	}

	return nullptr;
}

int32 UTrinketInventoryComponent::GetEquippedCount() const
{
	int32 Count = 0;
	for (const UPDA_Trinket* Ring : EquippedFingerRings)
	{
		if (Ring) ++Count;
	}
	if (EquippedWristwear) ++Count;
	for (const UPDA_Trinket* Prop : EquippedRailProps)
	{
		if (Prop) ++Count;
	}
	return Count;
}

void UTrinketInventoryComponent::EvaluateTrinketsForRoll(
	const FRollEvaluationContext& Context,
	UScoringQueueSubsystem* ScoringQueue,
	int32& OutTotalBonusChips,
	float& OutTotalBonusThrowMult,
	float& OutTotalBetMultFactor,
	TArray<FRollBreakdownItem>* OutBreakdownItems)
{
	OutTotalBonusChips = 0;
	OutTotalBonusThrowMult = 0.0f;
	OutTotalBetMultFactor = 1.0f;

	const TArray<UPDA_Trinket*> Trinkets = GetAllEquippedTrinkets();
	for (UPDA_Trinket* Trinket : Trinkets)
	{
		if (!Trinket)
		{
			continue;
		}

		FScoringStep Step;
		int32 StepChips = 0;
		float StepThrowMult = 0.0f;
		float StepBetMult = 1.0f;

		if (Trinket->EvaluateTrinket(Context, Step, StepChips, StepThrowMult, StepBetMult))
		{
			OutTotalBonusChips += StepChips;
			OutTotalBonusThrowMult += StepThrowMult;
			OutTotalBetMultFactor *= StepBetMult;

			if (OutBreakdownItems)
			{
				FRollBreakdownItem Item;
				Item.SourceName = Trinket->TrinketName.ToString();
				Item.Detail = Step.StepLabel;
				Item.MultiplierDelta = StepThrowMult;
				Item.MultiplierFactor = StepBetMult;
				Item.ChipDelta = StepChips;

				switch (Trinket->SlotType)
				{
				case ETrinketSlotType::FingerRing:
					Item.AccentColor = FLinearColor(0.0f, 0.85f, 1.0f);
					break;
				case ETrinketSlotType::WristwearActive:
					Item.AccentColor = FLinearColor(0.95f, 0.55f, 0.10f);
					break;
				case ETrinketSlotType::RailProp:
					Item.AccentColor = FLinearColor(0.85f, 0.40f, 0.05f);
					break;
				default:
					Item.AccentColor = FLinearColor::White;
					break;
				}
				OutBreakdownItems->Add(Item);
			}

			if (ScoringQueue)
			{
				Step.SourceActor = GetOwner();
				if (AActor* OwnerActor = GetOwner())
				{
					Step.PopupWorldLocation = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
				}
				ScoringQueue->EnqueueScoringStep(Step);
			}
		}
	}
}

bool UTrinketInventoryComponent::CheckMiracleSevenAverted(
	const FRollEvaluationContext& Context,
	FScoringStep& OutScoringStep,
	int32& OutBonusBaseChips) const
{
	OutBonusBaseChips = 0;
	const TArray<UPDA_Trinket*> Trinkets = GetAllEquippedTrinkets();
	for (const UPDA_Trinket* Trinket : Trinkets)
	{
		if (Trinket && Trinket->CheckMiracleSevenAverted(Context, OutScoringStep, OutBonusBaseChips))
		{
			return true;
		}
	}
	return false;
}
