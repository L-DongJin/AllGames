// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmGameInstance.h"
#include "../Data/RhythmSongCatalogDataAsset.h"
#include "UObject/ConstructorHelpers.h"

URhythmGameInstance::URhythmGameInstance()
{
	static ConstructorHelpers::FObjectFinder<URhythmSongCatalogDataAsset> CatalogFinder(
		TEXT("/Game/Data/DA_RhythmSongCatalog.DA_RhythmSongCatalog"));
	SongCatalog = CatalogFinder.Object;
}

float URhythmGameInstance::GetNoteDensity() const
{
	// Difficulty now selects a separately authored Data Asset; runtime thinning is disabled.
	return 1.0f;
}

float URhythmGameInstance::GetJudgementWindowScale() const
{
	switch (SelectedDifficulty)
	{
	case ERhythmDifficulty::Easy: return 1.15f;
	case ERhythmDifficulty::Normal: return 1.0f;
	case ERhythmDifficulty::Hard: return 1.0f;
	case ERhythmDifficulty::Expert: return 1.0f;
	default: return 1.0f;
	}
}

float URhythmGameInstance::GetTravelTimeSeconds(const float ChartTravelTimeSeconds) const
{
	return ChartTravelTimeSeconds / FMath::Max(ScrollSpeed, 0.1f);
}

FText URhythmGameInstance::GetDifficultyDisplayName() const
{
	switch (SelectedDifficulty)
	{
	case ERhythmDifficulty::Easy: return FText::FromString(TEXT("EASY"));
	case ERhythmDifficulty::Normal: return FText::FromString(TEXT("NORMAL"));
	case ERhythmDifficulty::Hard: return FText::FromString(TEXT("HARD"));
	case ERhythmDifficulty::Expert: return FText::FromString(TEXT("EXPERT"));
	default: return FText::GetEmpty();
	}
}

URhythmSongDataAsset* URhythmGameInstance::GetSelectedSong() const
{
	if (!SongCatalog) return nullptr;
	TArray<TObjectPtr<USoundBase>> UniqueMusic;
	for (const URhythmSongDataAsset* Chart : SongCatalog->Songs)
	{
		if (Chart && Chart->Music) UniqueMusic.AddUnique(Chart->Music);
	}
	if (!UniqueMusic.IsValidIndex(SelectedSongIndex)) return nullptr;

	URhythmSongDataAsset* Fallback = nullptr;
	for (URhythmSongDataAsset* Chart : SongCatalog->Songs)
	{
		if (!Chart || Chart->Music != UniqueMusic[SelectedSongIndex]) continue;
		if (!Fallback || Chart->Difficulty == ERhythmDifficulty::Normal) Fallback = Chart;
		if (Chart->Difficulty == SelectedDifficulty) return Chart;
	}
	return Fallback;
}

FText URhythmGameInstance::GetSelectedSongDisplayName() const
{
	if (const URhythmSongDataAsset* Song = GetSelectedSong())
	{
		return Song->SongTitle.IsEmpty() ? FText::FromString(Song->GetName()) : Song->SongTitle;
	}
	return FText::FromString(TEXT("NO SONG"));
}

void URhythmGameInstance::ChangeSong(const int32 Direction)
{
	TArray<TObjectPtr<USoundBase>> UniqueMusic;
	if (SongCatalog)
	{
		for (const URhythmSongDataAsset* Chart : SongCatalog->Songs)
		{
			if (Chart && Chart->Music) UniqueMusic.AddUnique(Chart->Music);
		}
	}
	const int32 SongCount = UniqueMusic.Num();
	if (SongCount > 0)
	{
		SelectedSongIndex = (SelectedSongIndex + Direction + SongCount) % SongCount;
	}
}

void URhythmGameInstance::ChangeDifficulty(const int32 Direction)
{
	constexpr int32 DifficultyCount = 4;
	const int32 Current = static_cast<int32>(SelectedDifficulty);
	SelectedDifficulty = static_cast<ERhythmDifficulty>((Current + Direction + DifficultyCount) % DifficultyCount);
}

void URhythmGameInstance::ChangeScrollSpeed(const int32 Direction)
{
	const int32 CurrentSpeed = FMath::Clamp(FMath::RoundToInt(ScrollSpeed), 1, 4);
	ScrollSpeed = static_cast<float>((CurrentSpeed - 1 + Direction + 4) % 4 + 1);
}
