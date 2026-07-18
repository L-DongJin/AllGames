// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLeaderboardSubsystem.h"

#include "PlayFab.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabError.h"
#include "../Core/RhythmAccountSubsystem.h"
#include "../Data/RhythmSongDataAsset.h"
#include "../Scoring/RhythmScoreManager.h"

namespace
{
	FString SanitizeStatisticToken(const FString& Source)
	{
		FString Result;
		Result.Reserve(Source.Len());
		for (const TCHAR Character : Source.ToUpper())
		{
			if (FChar::IsAlnum(Character))
			{
				Result.AppendChar(Character);
			}
			else if (!Result.EndsWith(TEXT("_")))
			{
				Result.AppendChar(TEXT('_'));
			}
		}
		return Result.TrimChar(TEXT('_'));
	}

	FRhythmLeaderboardResult ConvertLeaderboard(
		const FString& StatisticName,
		const TArray<PlayFab::ClientModels::FPlayerLeaderboardEntry>& Source)
	{
		FRhythmLeaderboardResult Converted;
		Converted.StatisticName = StatisticName;
		Converted.Entries.Reserve(Source.Num());
		for (const PlayFab::ClientModels::FPlayerLeaderboardEntry& SourceEntry : Source)
		{
			FRhythmLeaderboardEntry& Entry = Converted.Entries.AddDefaulted_GetRef();
			Entry.Rank = SourceEntry.Position + 1;
			Entry.PlayerId = SourceEntry.PlayFabId;
			Entry.DisplayName = SourceEntry.DisplayName.IsEmpty() ? TEXT("Unknown Player") : SourceEntry.DisplayName;
			Entry.Score = SourceEntry.StatValue;
		}
		return Converted;
	}
}

FString URhythmLeaderboardSubsystem::BuildStatisticName(const URhythmSongDataAsset* Chart)
{
	if (!Chart)
	{
		return FString();
	}

	const FString RawSongId = Chart->SongId.IsNone() ? Chart->SongTitle.ToString() : Chart->SongId.ToString();
	const FString SongToken = SanitizeStatisticToken(RawSongId);
	const TCHAR* KeyModeToken = Chart->KeyMode == ERhythmChartKeyMode::FiveKey ? TEXT("5K") : TEXT("9K");
	const TCHAR* DifficultyToken = TEXT("NORMAL");
	switch (Chart->Difficulty)
	{
	case ERhythmDifficulty::Easy: DifficultyToken = TEXT("EASY"); break;
	case ERhythmDifficulty::Normal: break;
	case ERhythmDifficulty::Hard: DifficultyToken = TEXT("HARD"); break;
	case ERhythmDifficulty::Expert: DifficultyToken = TEXT("EXPERT"); break;
	}
	return FString::Printf(TEXT("%s_%s_%s_V%d"), *SongToken, KeyModeToken, DifficultyToken,
		FMath::Max(1, Chart->ChartVersion));
}

bool URhythmLeaderboardSubsystem::ValidateRequest(
	const URhythmSongDataAsset* Chart, FString& OutStatisticName, FString& OutError) const
{
	const URhythmAccountSubsystem* Accounts = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>() : nullptr;
	if (!Accounts || !Accounts->IsLoggedIn())
	{
		OutError = TEXT("PlayFab 로그인이 필요합니다.");
		return false;
	}
	OutStatisticName = BuildStatisticName(Chart);
	if (OutStatisticName.IsEmpty())
	{
		OutError = TEXT("유효한 곡 또는 랭킹 식별자가 없습니다.");
		return false;
	}
	return true;
}

void URhythmLeaderboardSubsystem::SubmitScore(
	const URhythmSongDataAsset* Chart, const FRhythmScoreState& ScoreState)
{
	FString Error;
	if (bSubmittingScore || !ValidateRequest(Chart, PendingSubmitStatisticName, Error))
	{
		OnScoreSubmissionCompleted.Broadcast(false, bSubmittingScore ? TEXT("점수 제출이 이미 진행 중입니다.") : Error);
		return;
	}
	if (ScoreState.Score < 0 || ScoreState.Score > MAX_int32)
	{
		OnScoreSubmissionCompleted.Broadcast(false, TEXT("점수가 PlayFab 통계 범위를 벗어났습니다."));
		return;
	}

	PendingSubmitScore = static_cast<int32>(ScoreState.Score);
	bSubmittingScore = true;
	PlayFab::ClientModels::FGetPlayerStatisticsRequest Request;
	Request.StatisticNames.Add(PendingSubmitStatisticName);
	IPlayFabModuleInterface::Get().GetClientAPI()->GetPlayerStatistics(
		Request,
		PlayFab::UPlayFabClientAPI::FGetPlayerStatisticsDelegate::CreateUObject(
			this, &ThisClass::HandleCurrentStatisticsSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &ThisClass::HandleCurrentStatisticsError));
}

void URhythmLeaderboardSubsystem::HandleCurrentStatisticsSuccess(
	const PlayFab::ClientModels::FGetPlayerStatisticsResult& Result)
{
	int32 CurrentBest = INDEX_NONE;
	for (const PlayFab::ClientModels::FStatisticValue& Statistic : Result.Statistics)
	{
		if (Statistic.StatisticName == PendingSubmitStatisticName)
		{
			CurrentBest = Statistic.Value;
			break;
		}
	}

	if (CurrentBest != INDEX_NONE && PendingSubmitScore <= CurrentBest)
	{
		bSubmittingScore = false;
		UE_LOG(LogTemp, Log, TEXT("PlayFab score kept existing best: %s, best %d, run %d"),
			*PendingSubmitStatisticName, CurrentBest, PendingSubmitScore);
		OnScoreSubmissionCompleted.Broadcast(true, TEXT("기존 최고 기록을 유지했습니다."));
		return;
	}

	UpdatePendingScore();
}

void URhythmLeaderboardSubsystem::HandleCurrentStatisticsError(const PlayFab::FPlayFabCppError& Error)
{
	bSubmittingScore = false;
	UE_LOG(LogTemp, Error, TEXT("PlayFab best-score lookup failed; score was not submitted: %s"),
		*Error.GenerateErrorReport());
	OnScoreSubmissionCompleted.Broadcast(false, TEXT("최고 기록을 확인하지 못해 점수를 등록하지 않았습니다."));
}

void URhythmLeaderboardSubsystem::UpdatePendingScore()
{
	PlayFab::ClientModels::FStatisticUpdate Statistic;
	Statistic.StatisticName = PendingSubmitStatisticName;
	Statistic.Value = PendingSubmitScore;
	PlayFab::ClientModels::FUpdatePlayerStatisticsRequest Request;
	Request.Statistics.Add(Statistic);
	IPlayFabModuleInterface::Get().GetClientAPI()->UpdatePlayerStatistics(
		Request,
		PlayFab::UPlayFabClientAPI::FUpdatePlayerStatisticsDelegate::CreateUObject(
			this, &ThisClass::HandleSubmitSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &ThisClass::HandleSubmitError));
}

void URhythmLeaderboardSubsystem::RequestTopLeaderboard(const URhythmSongDataAsset* Chart, const int32 MaxResults)
{
	if (bRequestingTop)
	{
		UE_LOG(LogTemp, Verbose, TEXT("PlayFab top leaderboard request deferred while another request is active."));
		return;
	}
	FString Error;
	if (!ValidateRequest(Chart, PendingTopStatisticName, Error))
	{
		FRhythmLeaderboardResult FailedResult;
		FailedResult.StatisticName = BuildStatisticName(Chart);
		OnTopLeaderboardCompleted.Broadcast(false, FailedResult);
		return;
	}
	PlayFab::ClientModels::FGetLeaderboardRequest Request;
	Request.StatisticName = PendingTopStatisticName;
	Request.StartPosition = 0;
	Request.MaxResultsCount = FMath::Clamp(MaxResults, 1, 100);
	bRequestingTop = true;
	IPlayFabModuleInterface::Get().GetClientAPI()->GetLeaderboard(
		Request,
		PlayFab::UPlayFabClientAPI::FGetLeaderboardDelegate::CreateUObject(this, &ThisClass::HandleTopSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &ThisClass::HandleTopError));
}

void URhythmLeaderboardSubsystem::RequestLeaderboardAroundPlayer(
	const URhythmSongDataAsset* Chart, const int32 MaxResults)
{
	FString Error;
	if (bRequestingAroundPlayer || !ValidateRequest(Chart, PendingAroundPlayerStatisticName, Error))
	{
		OnAroundPlayerLeaderboardCompleted.Broadcast(false, FRhythmLeaderboardResult());
		return;
	}
	PlayFab::ClientModels::FGetLeaderboardAroundPlayerRequest Request;
	Request.StatisticName = PendingAroundPlayerStatisticName;
	Request.MaxResultsCount = FMath::Clamp(MaxResults, 1, 100);
	bRequestingAroundPlayer = true;
	IPlayFabModuleInterface::Get().GetClientAPI()->GetLeaderboardAroundPlayer(
		Request,
		PlayFab::UPlayFabClientAPI::FGetLeaderboardAroundPlayerDelegate::CreateUObject(
			this, &ThisClass::HandleAroundPlayerSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &ThisClass::HandleAroundPlayerError));
}

void URhythmLeaderboardSubsystem::HandleSubmitSuccess(
	const PlayFab::ClientModels::FUpdatePlayerStatisticsResult& Result)
{
	bSubmittingScore = false;
	UE_LOG(LogTemp, Log, TEXT("PlayFab score submitted: %s"), *PendingSubmitStatisticName);
	OnScoreSubmissionCompleted.Broadcast(true, TEXT("점수가 등록되었습니다."));
}

void URhythmLeaderboardSubsystem::HandleSubmitError(const PlayFab::FPlayFabCppError& Error)
{
	bSubmittingScore = false;
	UE_LOG(LogTemp, Error, TEXT("PlayFab score submission failed: %s"), *Error.GenerateErrorReport());
	OnScoreSubmissionCompleted.Broadcast(false, TEXT("점수 등록에 실패했습니다."));
}

void URhythmLeaderboardSubsystem::HandleTopSuccess(const PlayFab::ClientModels::FGetLeaderboardResult& Result)
{
	bRequestingTop = false;
	const FRhythmLeaderboardResult Converted = ConvertLeaderboard(PendingTopStatisticName, Result.Leaderboard);
	UE_LOG(LogTemp, Log, TEXT("PlayFab top leaderboard received: %s, entries %d"),
		*PendingTopStatisticName, Converted.Entries.Num());
	OnTopLeaderboardCompleted.Broadcast(true, Converted);
}

void URhythmLeaderboardSubsystem::HandleTopError(const PlayFab::FPlayFabCppError& Error)
{
	bRequestingTop = false;
	UE_LOG(LogTemp, Error, TEXT("PlayFab top leaderboard failed: %s"), *Error.GenerateErrorReport());
	FRhythmLeaderboardResult FailedResult;
	FailedResult.StatisticName = PendingTopStatisticName;
	OnTopLeaderboardCompleted.Broadcast(false, FailedResult);
}

void URhythmLeaderboardSubsystem::HandleAroundPlayerSuccess(
	const PlayFab::ClientModels::FGetLeaderboardAroundPlayerResult& Result)
{
	bRequestingAroundPlayer = false;
	const FRhythmLeaderboardResult Converted = ConvertLeaderboard(
		PendingAroundPlayerStatisticName, Result.Leaderboard);
	UE_LOG(LogTemp, Log, TEXT("PlayFab around-player leaderboard received: %s, entries %d"),
		*PendingAroundPlayerStatisticName, Converted.Entries.Num());
	OnAroundPlayerLeaderboardCompleted.Broadcast(true, Converted);
}

void URhythmLeaderboardSubsystem::HandleAroundPlayerError(const PlayFab::FPlayFabCppError& Error)
{
	bRequestingAroundPlayer = false;
	UE_LOG(LogTemp, Error, TEXT("PlayFab around-player leaderboard failed: %s"), *Error.GenerateErrorReport());
	FRhythmLeaderboardResult FailedResult;
	FailedResult.StatisticName = PendingAroundPlayerStatisticName;
	OnAroundPlayerLeaderboardCompleted.Broadcast(false, FailedResult);
}
