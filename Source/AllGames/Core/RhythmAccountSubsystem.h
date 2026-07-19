// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "eos_connect_types.h"
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
	bool IsEOSLoggedIn() const { return bEOSLoggedIn; }
	bool IsRequestInProgress() const { return bRequestInProgress; }
	const FString& GetPlayerId() const { return PlayerId; }
	const FString& GetUsername() const { return Username; }

private:
	bool ValidateCredentials(const FString& InUsername, const FString& Password);
	void HandleLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result);
	void HandleRegisterSuccess(const PlayFab::ClientModels::FRegisterPlayFabUserResult& Result);
	void HandleRequestError(const PlayFab::FPlayFabCppError& Error);
	void BeginEOSDeviceLogin();
	bool LoadPrivateEOSConfig();
	void LoginEOSIdentity();
	static void EOS_CALL HandleCreateDeviceIdComplete(const EOS_Connect_CreateDeviceIdCallbackInfo* Data);
	void HandleEOSLoginComplete(bool bWasSuccessful, const FString& Error);
	void CompleteAuthentication(bool bSuccess, const FString& Message);

	bool bLoggedIn = false;
	bool bEOSLoggedIn = false;
	bool bEOSDeviceCreateAttempted = false;
	bool bRequestInProgress = false;
	FString PendingUsername;
	FString PlayerId;
	FString Username;
};
