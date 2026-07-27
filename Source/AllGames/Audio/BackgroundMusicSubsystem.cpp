#include "BackgroundMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Containers/Ticker.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectGlobals.h"

void UBackgroundMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ThisClass::HandlePostLoadMap);
	BackgroundMusic = LoadObject<USoundWave>(
		nullptr, TEXT("/Game/Audio/BGM/AllGames-BGM.AllGames-BGM"));
	if (!BackgroundMusic)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shared BGM asset could not be loaded."));
	}
	InitialStartTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			if (UWorld* World = GetWorld())
			{
				UpdateForWorld(World);
				return false;
			}
			return true;
		}));
}

void UBackgroundMusicSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	}
	if (InitialStartTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(InitialStartTickerHandle);
		InitialStartTickerHandle.Reset();
	}
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}
	BackgroundMusic = nullptr;
	Super::Deinitialize();
}

void UBackgroundMusicSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UpdateForWorld(LoadedWorld);
}

bool UBackgroundMusicSubsystem::IsRhythmMap(const FString& MapName) const
{
	return MapName.Equals(TEXT("LobbyMap"), ESearchCase::IgnoreCase)
		|| MapName.Equals(TEXT("FiveKeyMap"), ESearchCase::IgnoreCase)
		|| MapName.Equals(TEXT("TestMap"), ESearchCase::IgnoreCase);
}

void UBackgroundMusicSubsystem::UpdateForWorld(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || !BackgroundMusic)
	{
		return;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(World, true);
	if (IsRhythmMap(MapName))
	{
		if (MusicComponent && MusicComponent->IsPlaying())
		{
			MusicComponent->FadeOut(.35f, 0.f);
		}
		return;
	}

	if (!MusicComponent || !IsValid(MusicComponent))
	{
		BackgroundMusic->bLooping = true;
		MusicComponent = UGameplayStatics::SpawnSound2D(
			World, BackgroundMusic, .525f, 1.f, 0.f, nullptr, true, false);
		if (MusicComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("Shared BGM spawned for map: %s (playing: %s)"),
				*MapName, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));
		}
		return;
	}
	if (MusicComponent && !MusicComponent->IsPlaying())
	{
		MusicComponent->FadeIn(.5f, .525f);
		UE_LOG(LogTemp, Log, TEXT("Shared BGM started for map: %s"), *MapName);
	}
}
