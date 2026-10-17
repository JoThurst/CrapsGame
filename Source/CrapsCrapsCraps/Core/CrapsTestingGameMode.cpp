// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/CrapsTestingGameMode.h"
#include "Core/CrapsTableGrayboxActor.h"
#include "Pawn/CrapsPawn.h"
#include "UI/CrapsDebugHUD.h"
#include "Table/CrapsGameState.h"
#include "Table/BetZone.h"
#include "Progression/RunProgressionSubsystem.h"
#include "Hazards/TableHazard.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ACrapsTestingGameMode::ACrapsTestingGameMode()
{
	DefaultPawnClass = ACrapsPawn::StaticClass();
	HUDClass = ACrapsDebugHUD::StaticClass();
	GameStateClass = ACrapsGameState::StaticClass();
}

APawn* ACrapsTestingGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	const FVector DefaultPitOrigin(0.0f, 0.0f, 5000.0f);
	FTransform TableHeadTransform(FRotator::ZeroRotator, DefaultPitOrigin);

	if (World)
	{
		AActor* ExistingTable = UGameplayStatics::GetActorOfClass(World, ACrapsTableGrayboxActor::StaticClass());
		if (!ExistingTable)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ExistingTable = World->SpawnActor<ACrapsTableGrayboxActor>(ACrapsTableGrayboxActor::StaticClass(), DefaultPitOrigin, FRotator::ZeroRotator, SpawnParams);
		}
		if (ExistingTable)
		{
			TableHeadTransform = FTransform(ExistingTable->GetActorRotation(), ExistingTable->GetActorLocation());
		}
	}

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, TableHeadTransform);
}

void ACrapsTestingGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Table = UGameplayStatics::GetActorOfClass(World, ACrapsTableGrayboxActor::StaticClass());
	const FVector PitOrigin = Table ? Table->GetActorLocation() : FVector(0.0f, 0.0f, 5000.0f);

	ACrapsGameState* CrapsGS = GetGameState<ACrapsGameState>();

	if (ACrapsTableGrayboxActor* GrayboxTable = Cast<ACrapsTableGrayboxActor>(Table))
	{
		GrayboxTable->EnsureLayoutResolved();
		if (CrapsGS)
		{
			GrayboxTable->BindToGameState(CrapsGS);
		}
	}

	if (CrapsGS)
	{
		if (URunProgressionSubsystem* Progression = World->GetSubsystem<URunProgressionSubsystem>())
		{
			Progression->BindToGameState(CrapsGS);
			Progression->StartNewRun();
		}
	}

	// 1. Spawn the Felt Betting Layout (Task 2.2) if not already placed in the map
	TArray<AActor*> ExistingZones;
	UGameplayStatics::GetAllActorsOfClass(World, ABetZone::StaticClass(), ExistingZones);
	if (ExistingZones.Num() == 0)
	{
		struct FZoneLayoutDef
		{
			ECrapsBetType Type;
			const TCHAR* Name;
			FVector RelativePos;
			FVector2D HalfExtent;
			float BaseMult;
			int64 InitialWager;
		};

		// Positions match the felt markings built in ACrapsTableGrayboxActor::BuildFeltLayout (table-local cm).
		const FZoneLayoutDef LayoutDefs[] = {
			{ ECrapsBetType::PassLine, TEXT("PASS LINE"),      FVector( 215.0f, -335.0f, 1.0f), FVector2D(235.0f, 30.0f), 2.0f, 50 },
			{ ECrapsBetType::DontPass, TEXT("DON'T PASS BAR"), FVector( 262.5f, -277.5f, 1.0f), FVector2D(187.5f, 27.5f), 2.0f, 0 },
			{ ECrapsBetType::Field,    TEXT("FIELD"),          FVector( 262.5f, -172.5f, 1.0f), FVector2D(187.5f, 77.5f), 1.5f, 50 },
			{ ECrapsBetType::Place4,   TEXT("PLACE 4"),        FVector( 125.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.8f, 25 },
			{ ECrapsBetType::Place5,   TEXT("PLACE 5"),        FVector( 185.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.5f, 25 },
			{ ECrapsBetType::Place6,   TEXT("PLACE 6"),        FVector( 245.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.4f, 50 },
			{ ECrapsBetType::Place8,   TEXT("PLACE 8"),        FVector( 305.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.4f, 50 },
			{ ECrapsBetType::Place9,   TEXT("PLACE 9"),        FVector( 365.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.5f, 25 },
			{ ECrapsBetType::Place10,  TEXT("PLACE 10"),       FVector( 425.0f,  180.0f, 1.0f), FVector2D( 30.0f, 100.0f), 1.8f, 25 },
			{ ECrapsBetType::Hard4,    TEXT("HARD 4"),         FVector( 655.0f,  -45.0f, 1.0f), FVector2D( 60.0f, 45.0f), 3.0f, 25 },
			{ ECrapsBetType::Hard10,   TEXT("HARD 10"),        FVector( 655.0f, -145.0f, 1.0f), FVector2D( 60.0f, 45.0f), 3.0f, 25 },
			{ ECrapsBetType::Hard6,    TEXT("HARD 6"),         FVector( 785.0f,  -45.0f, 1.0f), FVector2D( 60.0f, 45.0f), 3.0f, 25 },
			{ ECrapsBetType::Hard8,    TEXT("HARD 8"),         FVector( 785.0f, -145.0f, 1.0f), FVector2D( 60.0f, 45.0f), 3.0f, 25 },
			{ ECrapsBetType::Horn,     TEXT("HORN BET"),       FVector( 720.0f, -250.0f, 1.0f), FVector2D(125.0f, 50.0f), 3.5f, 25 },
		};

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (const FZoneLayoutDef& Def : LayoutDefs)
		{
			if (ABetZone* NewZone = World->SpawnActor<ABetZone>(ABetZone::StaticClass(), PitOrigin + Def.RelativePos, FRotator::ZeroRotator, SpawnParams))
			{
				NewZone->SetZoneHalfExtent(Def.HalfExtent.X, Def.HalfExtent.Y);
				NewZone->InitializeZone(Def.Type, Def.Name, Def.BaseMult, Def.InitialWager);
				if (CrapsGS)
				{
					CrapsGS->RegisterBetZone(NewZone);
				}
			}
		}

		if (CrapsGS)
		{
			CrapsGS->PlaceDefaultSpreadBets();
		}
	}
	// 2. Spawn Physical Table Hazards & Targets (Task 2.3) if not already in the map
	TArray<AActor*> ExistingHazards;
	UGameplayStatics::GetAllActorsOfClass(World, ATableHazard::StaticClass(), ExistingHazards);
	if (ExistingHazards.Num() == 0)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Two Synth-Noir Pinball Bumpers flanking the North landing zone (X = 1120..1220, Y = ±140)
		World->SpawnActor<APinballBumperHazard>(APinballBumperHazard::StaticClass(), PitOrigin + FVector(1140.0f, -140.0f, 14.0f), FRotator::ZeroRotator, SpawnParams);
		World->SpawnActor<APinballBumperHazard>(APinballBumperHazard::StaticClass(), PitOrigin + FVector(1140.0f,  140.0f, 14.0f), FRotator::ZeroRotator, SpawnParams);

		// One Laser Tripwire Gate spanning across the mid-table approach
		World->SpawnActor<ALaserGateHazard>(ALaserGateHazard::StaticClass(), PitOrigin + FVector(890.0f, 0.0f, 20.0f), FRotator::ZeroRotator, SpawnParams);

		// Two Scatterable 3D Physics Chip Stacks (bUseCCD = true) in the North landing lane
		World->SpawnActor<AChipStackHazard>(AChipStackHazard::StaticClass(), PitOrigin + FVector(1210.0f, -140.0f, 2.0f), FRotator::ZeroRotator, SpawnParams);
		World->SpawnActor<AChipStackHazard>(AChipStackHazard::StaticClass(), PitOrigin + FVector(1210.0f,  140.0f, 2.0f), FRotator::ZeroRotator, SpawnParams);
	}
}
