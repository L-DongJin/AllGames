#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BackgroundMusicSubsystem.generated.h"

class UAudioComponent;
class USoundWave;

/** Plays the shared menu/quiz BGM and automatically silences it in rhythm-game maps. */
UCLASS()
class ALLGAMES_API UBackgroundMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void UpdateForWorld(UWorld* World);
	bool IsRhythmMap(const FString& MapName) const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundWave> BackgroundMusic;

	FDelegateHandle PostLoadMapHandle;
	FTSTicker::FDelegateHandle InitialStartTickerHandle;
};
