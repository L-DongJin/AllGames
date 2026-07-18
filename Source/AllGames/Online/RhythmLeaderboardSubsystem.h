// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RhythmLeaderboardTypes.h"
#include "RhythmLeaderboardSubsystem.generated.h"

class URhythmSongDataAsset;
struct FRhythmScoreState;

namespace PlayFab
{
	struct FPlayFabCppError;
	namespace ClientModels
	{
		struct FGetPlayerStatisticsResult;
		struct FGetLeaderboardResult;
		struct FGetLeaderboardAroundPlayerResult;
		struct FUpdatePlayerStatisticsResult;
	}
}

/** Owns PlayFab score submission and leaderboard queries across map travel. */
UCLASS()
class ALLGAMES_API URhythmLeaderboardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FOnRhythmScoreSubmissionCompleted OnScoreSubmissionCompleted;
	FOnRhythmLeaderboardRequestCompleted OnTopLeaderboardCompleted;
	FOnRhythmLeaderboardRequestCompleted OnAroundPlayerLeaderboardCompleted;

	void SubmitScore(const URhythmSongDataAsset* Chart, const FRhythmScoreState& ScoreState);
	void RequestTopLeaderboard(const URhythmSongDataAsset* Chart, int32 MaxResults = 20);
	void RequestLeaderboardAroundPlayer(const URhythmSongDataAsset* Chart, int32 MaxResults = 10);

	static FString BuildStatisticName(const URhythmSongDataAsset* Chart);
	bool IsSubmittingScore() const { return bSubmittingScore; }
	bool IsRequestingTopLeaderboard() const { return bRequestingTop; }
	bool IsRequestingAroundPlayerLeaderboard() const { return bRequestingAroundPlayer; }

private:
	void HandleCurrentStatisticsSuccess(const PlayFab::ClientModels::FGetPlayerStatisticsResult& Result);
	void HandleCurrentStatisticsError(const PlayFab::FPlayFabCppError& Error);
	void UpdatePendingScore();
	void HandleSubmitSuccess(const PlayFab::ClientModels::FUpdatePlayerStatisticsResult& Result);
	void HandleSubmitError(const PlayFab::FPlayFabCppError& Error);
	void HandleTopSuccess(const PlayFab::ClientModels::FGetLeaderboardResult& Result);
	void HandleTopError(const PlayFab::FPlayFabCppError& Error);
	void HandleAroundPlayerSuccess(const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result);
	void HandleAroundPlayerError(const PlayFab::FPlayFabCppError& Error);
	bool ValidateRequest(const URhythmSongDataAsset* Chart, FString& OutStatisticName, FString& OutError) const;

	bool bSubmittingScore = false;
	bool bRequestingTop = false;
	bool bRequestingAroundPlayer = false;
	FString PendingSubmitStatisticName;
	int32 PendingSubmitScore = 0;
	FString PendingTopStatisticName;
	FString PendingAroundPlayerStatisticName;
};
