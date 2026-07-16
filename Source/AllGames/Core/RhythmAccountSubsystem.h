// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RhythmAccountSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRhythmAuthenticationCompleted, bool, const FString&);

namespace PlayFab
{
	struct FPlayFabCppError;
	namespace ClientModels
	{
		struct FLoginResult;
		struct FRegisterPlayFabUserResult;
	}
}

/** Owns the persistent PlayFab login session across lobby and gameplay map travel. */
UCLASS()
class ALLGAMES_API URhythmAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FOnRhythmAuthenticationCompleted OnAuthenticationCompleted;

	void Login(const FString& Username, const FString& Password);
	void Register(const FString& Username, const FString& Password);
	void Logout();

	bool IsLoggedIn() const { return bLoggedIn; }
	bool IsRequestInProgress() const { return bRequestInProgress; }
	const FString& GetPlayerId() const { return PlayerId; }
	const FString& GetUsername() const { return Username; }

private:
	bool ValidateCredentials(const FString& InUsername, const FString& Password);
	void HandleLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result);
	void HandleRegisterSuccess(const PlayFab::ClientModels::FRegisterPlayFabUserResult& Result);
	void HandleRequestError(const PlayFab::FPlayFabCppError& Error);
	void CompleteAuthentication(bool bSuccess, const FString& Message);

	bool bLoggedIn = false;
	bool bRequestInProgress = false;
	FString PendingUsername;
	FString PlayerId;
	FString Username;
};
