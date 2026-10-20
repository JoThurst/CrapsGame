// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CrapsDebugHUD.h"
#include "Pawn/CrapsPawn.h"
#include "Dice/DieBase.h"
#include "Dice/DiceRiggingComponent.h"
#include "Scoring/ScoringQueueSubsystem.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Progression/RunProgressionSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

ACrapsDebugHUD::ACrapsDebugHUD()
{
}

void ACrapsDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	UWorld* World = GetWorld();
	ACrapsPawn* CrapsPawn = Cast<ACrapsPawn>(GetOwningPawn());
	if (!CrapsPawn)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	// Semantic CRT Synth-Noir Palette (from DESIGN.md)
	const FLinearColor BgColor(0.02f, 0.02f, 0.04f, 0.86f);
	const FLinearColor CyanColor(0.0f, 0.96f, 0.83f, 1.0f);     // #00F5D4 Layer 1: Base Chips
	const FLinearColor MagentaColor(1.0f, 0.16f, 0.52f, 1.0f);  // #FF2A85 Layer 2: Throw Mult / Hazards
	const FLinearColor GoldColor(1.0f, 0.72f, 0.01f, 1.0f);     // #FFB703 Layer 3: Bet Mult / Bankroll
	const FLinearColor CrimsonColor(1.0f, 0.12f, 0.26f, 1.0f);  // #FF1E42 Seven-Out Danger
	const FLinearColor WhiteColor(0.92f, 0.94f, 0.98f, 1.0f);

	const float MinScale = 0.38f;
	const float AspectRatioScale = FMath::Min(Canvas->SizeX / 1920.0f, Canvas->SizeY / 1080.0f);
	const float UiScale = FMath::Clamp(AspectRatioScale, MinScale, 1.0f);

	// ========================================================================
	// 1. LEFT PANEL: Optional Developer Controls (Toggled via [H])
	// ========================================================================
	if (bShowDebugControls)
	{
		const float PanelX = 16.0f;
		const float PanelY = 16.0f;
		const float PanelW = 420.0f * UiScale;
		const float PanelH = 340.0f * UiScale;

		DrawRect(FLinearColor(0.015f, 0.015f, 0.035f, 0.85f), PanelX, PanelY, PanelW, PanelH);
		DrawLine(PanelX, PanelY, PanelX + PanelW, PanelY, CyanColor, 1.5f);
		DrawLine(PanelX, PanelY + PanelH, PanelX + PanelW, PanelY + PanelH, MagentaColor, 1.5f);
		DrawLine(PanelX, PanelY, PanelX, PanelY + PanelH, CyanColor, 1.5f);
		DrawLine(PanelX + PanelW, PanelY, PanelX + PanelW, PanelY + PanelH, MagentaColor, 1.5f);

		float CurY = PanelY + 10.0f * UiScale;
		const float LeftX = PanelX + 12.0f * UiScale;
		const float LineStep = 18.0f * UiScale;

		auto DrawRow = [&](const FString& Text, const FLinearColor& Color, float Scale = 0.95f)
		{
			DrawText(Text, Color, LeftX, CurY, Font, Scale * UiScale, false);
			CurY += LineStep * Scale;
		};

		DrawRow(TEXT("DEBUG CONTROLS // [H] to Hide"), CyanColor, 1.05f);
		CurY += 2.0f;

		int32 LiveD1 = 0;
		int32 LiveD2 = 0;
		int32 Bounces = 0;
		bool bAnyRigging = false;

		if (CrapsPawn->ActiveDice.Num() >= 2 && CrapsPawn->ActiveDice[0] && CrapsPawn->ActiveDice[1])
		{
			LiveD1 = CrapsPawn->ActiveDice[0]->GetTopFaceValue();
			LiveD2 = CrapsPawn->ActiveDice[1]->GetTopFaceValue();
			Bounces = CrapsPawn->ActiveDice[0]->GetCurrentThrowBounceCount() + CrapsPawn->ActiveDice[1]->GetCurrentThrowBounceCount();
			bAnyRigging = (CrapsPawn->ActiveDice[0]->DiceRiggingComponent && CrapsPawn->ActiveDice[0]->DiceRiggingComponent->IsActivelyRigging())
				|| (CrapsPawn->ActiveDice[1]->DiceRiggingComponent && CrapsPawn->ActiveDice[1]->DiceRiggingComponent->IsActivelyRigging());
		}

		const int32 LiveSum = LiveD1 + LiveD2;
		const FLinearColor SumColor = (LiveSum == 7) ? CrimsonColor : GoldColor;
		DrawRow(FString::Printf(TEXT("DICE: [%d]+[%d]=%d (Bounces: %d)"), LiveD1, LiveD2, LiveSum, Bounces), SumColor, 1.0f);

		FString ArchetypeStr;
		switch (CrapsPawn->ActiveThrowArchetype)
		{
		case EThrowArchetype::TableSkimmer:      ArchetypeStr = TEXT("[1] Skimmer"); break;
		case EThrowArchetype::ParabolicLob:      ArchetypeStr = TEXT("[2] Lob"); break;
		case EThrowArchetype::BackWallWhip:      ArchetypeStr = TEXT("[3] Whip"); break;
		case EThrowArchetype::MechanicalPlunger: ArchetypeStr = TEXT("[4] Plunger"); break;
		}
		DrawRow(FString::Printf(TEXT("STYLE [1-4]: %s"), *ArchetypeStr), MagentaColor, 0.92f);

		FString InputModeStr;
		switch (CrapsPawn->ActiveInputMode)
		{
		case EThrowInputMode::MouseFlick:    InputModeStr = TEXT("Mouse Flick"); break;
		case EThrowInputMode::ArcadeMeter:   InputModeStr = TEXT("Arcade Meter"); break;
		case EThrowInputMode::SlingshotPull: InputModeStr = TEXT("Slingshot"); break;
		}
		DrawRow(FString::Printf(TEXT("INPUT [Tab]: %s"), *InputModeStr), CyanColor, 0.92f);

		const int32 RiggingPct = FMath::RoundToInt(CrapsPawn->GlobalDiceControlStat * 100.0f);
		DrawRow(FString::Printf(TEXT("TORQUE MAGNET [ / ]: %d%% %s"), RiggingPct, bAnyRigging ? TEXT("<<ENGAGED>>") : TEXT("")), bAnyRigging ? GoldColor : CyanColor, 0.92f);

		const int32 FrictionPct = FMath::RoundToInt(CrapsPawn->TableSurfaceFriction * 100.0f);
		DrawRow(FString::Printf(TEXT("FRICTION [- / =]: %d%%"), FrictionPct), WhiteColor, 0.92f);
	}
	else
	{
		// Sleek minimal floating pill at bottom-left
		const float PillW = (bStreamlinedHUD ? 640.0f : 740.0f) * UiScale;
		const float PillH = 26.0f * UiScale;
		const float PillX = 16.0f;
		const float PillY = Canvas->SizeY - PillH - 12.0f;

		DrawRect(FLinearColor(0.015f, 0.015f, 0.035f, 0.78f), PillX, PillY, PillW, PillH);
		DrawLine(PillX, PillY, PillX + PillW, PillY, CyanColor, 1.2f);

		FString PillText;
		if (CrapsPawn->GetHoveredBetZone())
		{
			PillText = FString::Printf(TEXT("ZONE: %s  |  CHIP: [$%lld] (T/Wheel)  |  [LMB] Bet  |  [RMB] Refund  |  [Space] Throw"),
				*CrapsPawn->GetHoveredBetZone()->GetZoneName(),
				CrapsPawn->SelectedChipDenomination);
		}
		else
		{
			PillText = FString::Printf(TEXT("CHIP: [$%lld] (T/Wheel)  |  [LMB/Space] Throw  |  [F] Bank  |  [X] Press All  |  [H] Debug"),
				CrapsPawn->SelectedChipDenomination);
		}

		DrawText(PillText, CyanColor, PillX + 8.0f * UiScale, PillY + 4.0f * UiScale, Font, 0.82f * UiScale, false);
	}

	// ========================================================================
	// 2. CENTER-BOTTOM: Throw Charging Power Bar (Only while charging)
	// ========================================================================
	if (CrapsPawn->IsChargingThrow() || CrapsPawn->CurrentPhase == ECrapsThrowPhase::ThrowCharging)
	{
		const float MeterW = 280.0f * UiScale;
		const float MeterH = 14.0f * UiScale;
		const float MeterX = (Canvas->SizeX - MeterW) * 0.5f;
		const float MeterY = Canvas->SizeY - 70.0f * UiScale;
		const float PowerPct = FMath::Clamp(CrapsPawn->CurrentLaunchParams.NormalizedPower, 0.0f, 1.0f);

		DrawRect(FLinearColor(0.02f, 0.02f, 0.05f, 0.85f), MeterX - 4.0f, MeterY - 18.0f * UiScale, MeterW + 8.0f, MeterH + 24.0f * UiScale);
		DrawText(FString::Printf(TEXT("THROW POWER: %d%% %s"), FMath::RoundToInt(PowerPct * 100.0f),
			CrapsPawn->CurrentLaunchParams.bSweetSpotRelease ? TEXT("[SWEET SPOT!]") : TEXT("")),
			CrapsPawn->CurrentLaunchParams.bSweetSpotRelease ? GoldColor : MagentaColor,
			MeterX, MeterY - 16.0f * UiScale, Font, 0.88f * UiScale, false);

		DrawRect(FLinearColor(0.1f, 0.1f, 0.15f, 1.0f), MeterX, MeterY, MeterW, MeterH);
		DrawRect(FLinearColor(1.0f, 0.72f, 0.01f, 0.45f), MeterX + MeterW * 0.78f, MeterY, MeterW * 0.12f, MeterH);
		DrawRect(CrapsPawn->CurrentLaunchParams.bSweetSpotRelease ? GoldColor : MagentaColor, MeterX, MeterY, MeterW * PowerPct, MeterH);
	}

	// ========================================================================
	// 3. TOP-RIGHT: Compact & Sleek Progression & Table Status Card
	// ========================================================================
	UScoringQueueSubsystem* ScoringQueue = World ? World->GetSubsystem<UScoringQueueSubsystem>() : nullptr;
	ACrapsGameState* CrapsGS = World ? World->GetGameState<ACrapsGameState>() : nullptr;
	URunProgressionSubsystem* Progression = World ? World->GetSubsystem<URunProgressionSubsystem>() : nullptr;

	const float ScoreW = (bStreamlinedHUD ? 320.0f : 390.0f) * UiScale;
	const float ScoreH = (bStreamlinedHUD ? 116.0f : 135.0f) * UiScale;
	const float ScoreX = Canvas->SizeX - ScoreW - 16.0f;
	const float ScoreY = 16.0f;

	DrawRect(FLinearColor(0.015f, 0.015f, 0.032f, 0.70f), ScoreX, ScoreY, ScoreW, ScoreH);
	DrawLine(ScoreX, ScoreY, ScoreX + ScoreW, ScoreY, GoldColor, 1.6f);
	DrawLine(ScoreX, ScoreY + ScoreH, ScoreX + ScoreW, ScoreY + ScoreH, CyanColor, 1.6f);
	DrawLine(ScoreX, ScoreY, ScoreX, ScoreY + ScoreH, GoldColor, 1.6f);
	DrawLine(ScoreX + ScoreW, ScoreY, ScoreX + ScoreW, ScoreY + ScoreH, CyanColor, 1.6f);

	float SY = ScoreY + (bStreamlinedHUD ? 6.0f : 8.0f) * UiScale;
	const float SX = ScoreX + 10.0f * UiScale;
	const float RowStep = (bStreamlinedHUD ? 18.0f : 21.0f) * UiScale;

	auto DrawScoreRow = [&](const FString& Text, const FLinearColor& Color, float Scale = 0.94f)
	{
		DrawText(Text, Color, SX, SY, Font, Scale * UiScale, false);
		SY += RowStep * Scale;
	};

	if (Progression)
	{
		FString StateStr;
		FLinearColor StateCol = CyanColor;
		switch (Progression->GetRunState())
		{
		case ERunProgressionState::ActiveAnte:
			StateStr = TEXT("ACTIVE");
			StateCol = GoldColor;
			break;
		case ERunProgressionState::AnteCleared:
			StateStr = TEXT("CLEARED!");
			StateCol = CyanColor;
			break;
		case ERunProgressionState::BetweenAntes:
			StateStr = TEXT("BETWEEN");
			StateCol = WhiteColor;
			break;
		case ERunProgressionState::RunVictory:
			StateStr = TEXT("VICTORY!");
			StateCol = GoldColor;
			break;
		case ERunProgressionState::RunDefeat:
			StateStr = TEXT("DEFEAT");
			StateCol = CrimsonColor;
			break;
		default:
			StateStr = TEXT("START");
			break;
		}

		DrawScoreRow(
			FString::Printf(TEXT("ANTE %d / %d [%s]  |  ROLLS: %d / %d"),
				Progression->GetCurrentAnteIndex(),
				Progression->GetTotalAntesCount(),
				*StateStr,
				Progression->GetRollsRemaining(),
				Progression->GetRollAllowance()),
			StateCol,
			1.0f);

		const int64 TargetQ = Progression->GetCurrentAnteTargetQuota();
		const int64 CurProg = Progression->GetCurrentAnteProgress();
		const float ProgressPct = TargetQ > 0 ? FMath::Clamp(static_cast<float>(CurProg) / static_cast<float>(TargetQ), 0.0f, 1.0f) : 0.0f;

		DrawScoreRow(
			FString::Printf(TEXT("PROGRESS: $%lld / $%lld (%d%%)"),
				CurProg,
				TargetQ,
				FMath::RoundToInt(ProgressPct * 100.0f)),
			CurProg >= TargetQ ? CyanColor : GoldColor,
			0.95f);

		const float ProgBarW = ScoreW - 24.0f * UiScale;
		const float ProgBarH = 4.0f * UiScale;
		DrawRect(FLinearColor(0.08f, 0.08f, 0.12f, 1.0f), SX, SY, ProgBarW, ProgBarH);
		DrawRect(CurProg >= TargetQ ? CyanColor : GoldColor, SX, SY, ProgBarW * ProgressPct, ProgBarH);
		SY += ProgBarH + 5.0f * UiScale;
	}

	if (CrapsGS)
	{
		DrawScoreRow(
			FString::Printf(TEXT("BANKROLL: $%lld   |   POOL: +$%lld [F]"),
				CrapsGS->GetPlayerBankroll(),
				CrapsGS->GetUnbankedPool()),
			CrapsGS->GetUnbankedPool() > 0 ? GoldColor : WhiteColor,
			0.95f);

		const bool bPointOn = (CrapsGS->GetTablePhase() == ECrapsTablePhase::PointEstablished);
		const FString PuckText = bPointOn
			? FString::Printf(TEXT("PUCK [ON %d]  |  %s"), CrapsGS->GetEstablishedPoint(), *CrapsGS->GetLastRollOutcomeBanner())
			: FString::Printf(TEXT("PUCK [OFF] COME-OUT  |  %s"), *CrapsGS->GetLastRollOutcomeBanner());
		DrawScoreRow(PuckText, bPointOn ? GoldColor : CyanColor, 0.88f);
	}

	// Compact Inline 3-Layer Scoring Pill (shown below card during scoring)
	if (ScoringQueue && (ScoringQueue->RunningBaseChips > 0 || ScoringQueue->GetCurrentThrowTotalPayout() > 0))
	{
		const float PillW = ScoreW;
		const float PillH = 26.0f * UiScale;
		const float PillY = ScoreY + ScoreH + 6.0f;

		DrawRect(FLinearColor(0.01f, 0.01f, 0.03f, 0.82f), ScoreX, PillY, PillW, PillH);
		DrawText(
			FString::Printf(TEXT("%d Chips x %.1fx Throw x %.1fx Bet = +%lld"),
				ScoringQueue->RunningBaseChips,
				ScoringQueue->RunningThrowMult,
				ScoringQueue->RunningBetMult,
				ScoringQueue->GetCurrentThrowTotalPayout()),
			GoldColor, ScoreX + 8.0f * UiScale, PillY + 5.0f * UiScale, Font, 0.90f * UiScale, false);
	}

	// Project floating 3D pop-up text in world space above the dice
	if (ScoringQueue)
	{
		for (const FActiveScoringPopup& Pop : ScoringQueue->ActivePopups)
		{
			if (!Pop.WorldLocation.IsZero())
			{
				FLinearColor PopColor = CyanColor;
				if (Pop.Layer == EScoringLayer::ThrowMult) PopColor = MagentaColor;
				else if (Pop.Layer == EScoringLayer::BetMult || Pop.Layer == EScoringLayer::PointBonus) PopColor = GoldColor;
				else if (Pop.Layer == EScoringLayer::SevenOut) PopColor = CrimsonColor;

				const FVector Projected = Project(Pop.WorldLocation + FVector(0.0f, 0.0f, (1.35f - Pop.RemainingTime) * 45.0f));
				if (Projected.Z > 0.0f)
				{
					DrawText(Pop.Text, PopColor, Projected.X - 50.0f, Projected.Y, Font, 1.05f * UiScale, false);
				}
			}
		}
	}

	// ========================================================================
	// 3. PROJECT 3D FELT BET ZONE LABELS ONTO THE CANVAS (SMART FILTER)
	// ========================================================================
	if (CrapsGS)
	{
		const ABetZone* HoveredZone = CrapsPawn->GetHoveredBetZone();
		for (ABetZone* Zone : CrapsGS->GetActiveBetZones())
		{
			if (!Zone)
			{
				continue;
			}

			const bool bIsHovered = (Zone == HoveredZone);
			const bool bHasWager = (Zone->GetPlacedWager() > 0);

			// In streamlined mode, only draw 2D overlays for hovered zones or zones with active wagers
			if (bStreamlinedHUD && !bIsHovered && !bHasWager)
			{
				continue;
			}

			const FVector ScreenPos = Project(Zone->GetActorLocation() + FVector(0.0f, 0.0f, 14.0f));
			if (ScreenPos.Z > 0.0f)
			{
				if (bIsHovered)
				{
					// Expanded interactive tooltip card
					const FString Label = FString::Printf(
						TEXT("%s  [$%lld | %.1fx]\n[LMB] +$%lld  |  [RMB] -$%lld"),
						*Zone->GetZoneName(),
						Zone->GetPlacedWager(),
						Zone->GetEffectiveBetMultiplier(),
						CrapsPawn->SelectedChipDenomination,
						CrapsPawn->SelectedChipDenomination);
					const FLinearColor ZoneCol = bHasWager ? GoldColor : CyanColor;
					DrawText(Label, ZoneCol, ScreenPos.X - 60.0f * UiScale, ScreenPos.Y - 18.0f * UiScale, Font, 0.84f * UiScale, false);
				}
				else
				{
					// Compact chip stack badge for active bets
					const FString Label = FString::Printf(
						TEXT("%s: $%lld"),
						*Zone->GetZoneName(),
						Zone->GetPlacedWager());
					DrawText(Label, GoldColor, ScreenPos.X - 35.0f * UiScale, ScreenPos.Y - 10.0f * UiScale, Font, 0.78f * UiScale, false);
				}
			}
		}
	}

	// ========================================================================
	// 4. RETRO CRT SYNTH-NOIR TRANSITION MODAL (Milestone B1)
	// ========================================================================
	if (Progression)
	{
		const ERunProgressionState RState = Progression->GetRunState();
		if (RState == ERunProgressionState::AnteCleared || RState == ERunProgressionState::BetweenAntes
			|| RState == ERunProgressionState::RunDefeat || RState == ERunProgressionState::RunVictory)
		{
			const float ModalW = 720.0f;
			const float ModalH = 220.0f;
			const float ModalX = (Canvas->SizeX - ModalW) * 0.5f;
			const float ModalY = (Canvas->SizeY - ModalH) * 0.5f;

			FLinearColor BorderCol = CyanColor;
			FString TitleText;
			FString SubtitleText;
			FString ActionPrompt;

			if (RState == ERunProgressionState::AnteCleared || RState == ERunProgressionState::BetweenAntes)
			{
				BorderCol = GoldColor;
				TitleText = FString::Printf(TEXT("*** ANTE %d CLEARED! ***"), Progression->GetCurrentAnteIndex());
				SubtitleText = FString::Printf(TEXT("Target Quota: $%lld   |   Progress: $%lld   |   Bankroll: $%lld"),
					Progression->GetCurrentAnteTargetQuota(),
					Progression->GetCurrentAnteProgress(),
					CrapsGS ? CrapsGS->GetPlayerBankroll() : 0LL);

				const int64 Unbanked = CrapsGS ? CrapsGS->GetUnbankedPool() : 0LL;
				if (Unbanked > 0)
				{
					ActionPrompt = FString::Printf(TEXT("[F] BANK UNBANKED POOL ($%lld)   |   [ENTER] ADVANCE TO NEXT ANTE"), Unbanked);
				}
				else
				{
					ActionPrompt = TEXT("PRESS [ENTER] TO ADVANCE TO THE NEXT ANTE");
				}
			}
			else if (RState == ERunProgressionState::RunDefeat)
			{
				BorderCol = CrimsonColor;
				TitleText = TEXT("*** RUN DEFEAT — ROLLS EXHAUSTED ***");
				SubtitleText = FString::Printf(TEXT("Reached Ante %d / %d   |   Progress: $%lld / $%lld Quota"),
					Progression->GetCurrentAnteIndex(),
					Progression->GetTotalAntesCount(),
					Progression->GetCurrentAnteProgress(),
					Progression->GetCurrentAnteTargetQuota());
				ActionPrompt = TEXT("PRESS [R] TO RESTART A NEW RUN");
			}
			else if (RState == ERunProgressionState::RunVictory)
			{
				BorderCol = GoldColor;
				TitleText = TEXT("*** RUN VICTORY — THE PIT CONQUERED! ***");
				SubtitleText = FString::Printf(TEXT("All %d Antes Cleared!   |   Final Bankroll: $%lld"),
					Progression->GetTotalAntesCount(),
					CrapsGS ? CrapsGS->GetPlayerBankroll() : 0LL);
				ActionPrompt = TEXT("PRESS [R] TO START A NEW RUN");
			}

			// Modal Background & Neon Border
			DrawRect(FLinearColor(0.01f, 0.01f, 0.03f, 0.94f), ModalX, ModalY, ModalW, ModalH);
			DrawLine(ModalX, ModalY, ModalX + ModalW, ModalY, BorderCol, 3.0f);
			DrawLine(ModalX, ModalY + ModalH, ModalX + ModalW, ModalY + ModalH, BorderCol, 3.0f);
			DrawLine(ModalX, ModalY, ModalX, ModalY + ModalH, BorderCol, 3.0f);
			DrawLine(ModalX + ModalW, ModalY, ModalX + ModalW, ModalY + ModalH, BorderCol, 3.0f);

			float TextY = ModalY + 28.0f;
			DrawText(TitleText, BorderCol, ModalX + 30.0f, TextY, Font, 1.45f, false);
			TextY += 46.0f;
			DrawText(SubtitleText, WhiteColor, ModalX + 30.0f, TextY, Font, 1.12f, false);
			TextY += 48.0f;
			DrawText(ActionPrompt, GoldColor, ModalX + 30.0f, TextY, Font, 1.20f, false);
		}
	}
}
