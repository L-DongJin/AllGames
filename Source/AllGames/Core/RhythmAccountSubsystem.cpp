// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmAccountSubsystem.h"

#include "PlayFab.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabError.h"
#include "PlayFabRuntimeSettings.h"
#include "EOSShared.h"
#include "IEOSSDKManager.h"
#include "Online/Auth.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEOSGS.h"
#include "eos_connect.h"
#include "eos_sdk.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

void URhythmAccountSubsystem::Login(const FString& InUsername, const FString& Password)
{
	if (bRequestInProgress || !ValidateCredentials(InUsername, Password))
	{
		return;
	}

	PlayFab::ClientModels::FLoginWithPlayFabRequest Request;
	Request.TitleId = GetDefault<UPlayFabRuntimeSettings>()->TitleId;
	Request.Username = InUsername.TrimStartAndEnd();
	Request.Password = Password;
	PendingUsername = Request.Username;
	bRequestInProgress = true;

	IPlayFabModuleInterface::Get().GetClientAPI()->LoginWithPlayFab(
		Request,
		PlayFab::UPlayFabClientAPI::FLoginWithPlayFabDelegate::CreateUObject(
			this, &ThisClass::HandleLoginSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(
			this, &ThisClass::HandleRequestError));
}

void URhythmAccountSubsystem::Register(const FString& InUsername, const FString& Password)
{
	if (bRequestInProgress || !ValidateCredentials(InUsername, Password))
	{
		return;
	}

	const FString CleanUsername = InUsername.TrimStartAndEnd();
	PlayFab::ClientModels::FRegisterPlayFabUserRequest Request;
	Request.TitleId = GetDefault<UPlayFabRuntimeSettings>()->TitleId;
	Request.Username = CleanUsername;
	Request.DisplayName = CleanUsername.Left(25);
	Request.Password = Password;
	Request.RequireBothUsernameAndEmail = false;
	PendingUsername = CleanUsername;
	bRequestInProgress = true;

	IPlayFabModuleInterface::Get().GetClientAPI()->RegisterPlayFabUser(
		Request,
		PlayFab::UPlayFabClientAPI::FRegisterPlayFabUserDelegate::CreateUObject(
			this, &ThisClass::HandleRegisterSuccess),
		PlayFab::FPlayFabErrorDelegate::CreateUObject(
			this, &ThisClass::HandleRequestError));
}

void URhythmAccountSubsystem::Logout()
{
	bLoggedIn = false;
	bEOSLoggedIn = false;
	bRequestInProgress = false;
	PendingUsername.Reset();
	PlayerId.Reset();
	Username.Reset();
}

bool URhythmAccountSubsystem::ValidateCredentials(const FString& InUsername, const FString& Password)
{
	const FString CleanUsername = InUsername.TrimStartAndEnd();
	if (CleanUsername.Len() < 3 || CleanUsername.Len() > 20)
	{
		CompleteAuthentication(false, TEXT("아이디는 3자 이상 20자 이하로 입력해 주세요."));
		return false;
	}
	if (Password.Len() < 6 || Password.Len() > 100)
	{
		CompleteAuthentication(false, TEXT("비밀번호는 6자 이상 100자 이하로 입력해 주세요."));
		return false;
	}
	if (GetDefault<UPlayFabRuntimeSettings>()->TitleId.IsEmpty())
	{
		CompleteAuthentication(false, TEXT("PlayFab Title ID가 설정되지 않았습니다."));
		return false;
	}
	return true;
}

void URhythmAccountSubsystem::HandleLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result)
{
	PlayerId = Result.PlayFabId;
	Username = PendingUsername;
	bLoggedIn = true;
	bEOSDeviceCreateAttempted = false;
	BeginEOSDeviceLogin();
}

void URhythmAccountSubsystem::BeginEOSDeviceLogin()
{
	if (!LoadPrivateEOSConfig())
	{
		HandleEOSLoginComplete(false, TEXT("EOS local credentials are missing."));
		return;
	}

	// Existing installations already have a cached EOS Device ID. Try it first so EOS does not
	// emit an error for an unnecessary CreateDeviceId call. A missing ID falls back to creation.
	if (!bEOSDeviceCreateAttempted)
	{
		LoginEOSIdentity();
		return;
	}

	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services =
		StaticCastSharedPtr<UE::Online::FOnlineServicesEOSGS>(
			UE::Online::GetServices(UE::Online::EOnlineServices::Epic));
	if (!Services.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("EOSGS login failed: Online Services EOSGS is unavailable."));
		CompleteAuthentication(true, FString::Printf(TEXT("%s님, 환영합니다. (인터넷 멀티플레이 연결 대기)"), *Username));
		return;
	}

	const IEOSPlatformHandlePtr PlatformHandle = Services->GetEOSPlatformHandle();
	if (!PlatformHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("EOS login failed: platform handle is unavailable."));
		CompleteAuthentication(true, FString::Printf(TEXT("%s님, 환영합니다. (인터넷 멀티플레이 연결 대기)"), *Username));
		return;
	}

	EOS_Connect_CreateDeviceIdOptions Options{};
	Options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
	Options.DeviceModel = "WindowsPC";
	EOS_Connect_CreateDeviceId(
		EOS_Platform_GetConnectInterface(static_cast<EOS_HPlatform>(*PlatformHandle)),
		&Options,
		this,
		&ThisClass::HandleCreateDeviceIdComplete);
}

bool URhythmAccountSubsystem::LoadPrivateEOSConfig()
{
	const FString SecretsPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("AllGamesSecrets.ini"));
	if (!IFileManager::Get().FileExists(*SecretsPath))
	{
		UE_LOG(LogTemp, Error, TEXT("EOS credentials file is missing: %s"), *SecretsPath);
		return false;
	}

	FConfigFile SecretsConfig;
	SecretsConfig.Read(SecretsPath);

	static const TCHAR* Section = TEXT("EOSSDK.Platform.AllGames");
	FString ClientSecret;
	FString ClientEncryptionKey;
	if (!SecretsConfig.GetString(Section, TEXT("ClientSecret"), ClientSecret)
		|| !SecretsConfig.GetString(Section, TEXT("ClientEncryptionKey"), ClientEncryptionKey)
		|| ClientSecret.IsEmpty()
		|| ClientEncryptionKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("EOS credentials file does not contain the required values."));
		return false;
	}

	GConfig->SetString(Section, TEXT("ClientSecret"), *ClientSecret, GEngineIni);
	GConfig->SetString(Section, TEXT("ClientEncryptionKey"), *ClientEncryptionKey, GEngineIni);
	return true;
}

void EOS_CALL URhythmAccountSubsystem::HandleCreateDeviceIdComplete(
	const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
{
	URhythmAccountSubsystem* This = static_cast<URhythmAccountSubsystem*>(Data ? Data->ClientData : nullptr);
	if (!IsValid(This))
	{
		return;
	}

	if (Data->ResultCode == EOS_EResult::EOS_Success ||
		Data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed)
	{
		This->LoginEOSIdentity();
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("EOS CreateDeviceId failed: %s"),
		UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
	This->CompleteAuthentication(true, FString::Printf(
		TEXT("%s님, 환영합니다. (인터넷 멀티플레이 연결 대기)"), *This->Username));
}

void URhythmAccountSubsystem::LoginEOSIdentity()
{
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services =
		StaticCastSharedPtr<UE::Online::FOnlineServicesEOSGS>(
			UE::Online::GetServices(UE::Online::EOnlineServices::Epic));
	const UE::Online::IAuthPtr Auth = Services.IsValid() ? Services->GetAuthInterface() : nullptr;
	if (!Auth.IsValid())
	{
		CompleteAuthentication(true, FString::Printf(TEXT("%s님, 환영합니다. (인터넷 멀티플레이 연결 대기)"), *Username));
		return;
	}

	UE::Online::FAuthGetLocalOnlineUserByPlatformUserId::Params ExistingUserParams;
	ExistingUserParams.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);
	const UE::Online::TOnlineResult<UE::Online::FAuthGetLocalOnlineUserByPlatformUserId> ExistingUser =
		Auth->GetLocalOnlineUserByPlatformUserId(MoveTemp(ExistingUserParams));
	if (ExistingUser.IsOk() && UE::Online::IsOnlineStatus(ExistingUser.GetOkValue().AccountInfo->LoginStatus))
	{
		HandleEOSLoginComplete(true, FString());
		return;
	}

	UE::Online::FAuthLogin::Params Params;
	Params.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);
	Params.CredentialsType = UE::Online::LoginCredentialsType::ExternalAuth;
	Params.CredentialsId = Username;
	Params.CredentialsToken.Set<UE::Online::FExternalAuthToken>(
		UE::Online::FExternalAuthToken{UE::Online::ExternalLoginType::DeviceIdAccessToken, FString()});

	Auth->Login(MoveTemp(Params)).OnComplete(
		this,
		[this](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
		{
			HandleEOSLoginComplete(
				Result.IsOk(),
				Result.IsError() ? Result.GetErrorValue().GetLogString() : FString());
		});
}

void URhythmAccountSubsystem::HandleEOSLoginComplete(
	const bool bWasSuccessful,
	const FString& Error)
{
	bEOSLoggedIn = bWasSuccessful;
	if (!bWasSuccessful)
	{
		if (!bEOSDeviceCreateAttempted)
		{
			UE_LOG(LogTemp, Log, TEXT("EOS Device ID was not found; creating it before retrying login."));
			bEOSDeviceCreateAttempted = true;
			BeginEOSDeviceLogin();
			return;
		}
		UE_LOG(LogTemp, Error, TEXT("EOS identity login failed after Device ID creation: %s"), *Error);
		CompleteAuthentication(true, FString::Printf(
			TEXT("%s님, 환영합니다. (인터넷 멀티플레이 연결 대기)"), *Username));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EOS Connect device login succeeded for %s."), *Username);
	CompleteAuthentication(true, FString::Printf(TEXT("%s님, 환영합니다."), *Username));
}

void URhythmAccountSubsystem::HandleRegisterSuccess(
	const PlayFab::ClientModels::FRegisterPlayFabUserResult& Result)
{
	// Registration returns a session, but the initial flow intentionally requires an explicit
	// login so the player learns which credentials they will use on another PC.
	PlayerId.Reset();
	Username.Reset();
	bLoggedIn = false;
	CompleteAuthentication(true, TEXT("회원가입이 완료되었습니다."));
}

void URhythmAccountSubsystem::HandleRequestError(const PlayFab::FPlayFabCppError& Error)
{
	FString Message;
	switch (Error.ErrorCode)
	{
	case PlayFab::PlayFabErrorInvalidUsernameOrPassword:
	case PlayFab::PlayFabErrorAccountNotFound:
		Message = TEXT("아이디 또는 비밀번호가 올바르지 않습니다.");
		break;
	case PlayFab::PlayFabErrorUsernameNotAvailable:
		Message = TEXT("이미 사용 중인 아이디입니다.");
		break;
	case PlayFab::PlayFabErrorInvalidUsername:
		Message = TEXT("아이디에 사용할 수 없는 문자가 포함되어 있습니다.");
		break;
	case PlayFab::PlayFabErrorInvalidPassword:
		Message = TEXT("비밀번호가 계정 생성 조건에 맞지 않습니다.");
		break;
	default:
		Message = Error.ErrorMessage.IsEmpty()
			? TEXT("서버에 연결할 수 없습니다. 인터넷 연결을 확인해 주세요.")
			: TEXT("요청 처리 중 오류가 발생했습니다. 잠시 후 다시 시도해 주세요.");
		break;
	}
	UE_LOG(LogTemp, Error, TEXT("PlayFab authentication failed: %s"), *Error.GenerateErrorReport());
	CompleteAuthentication(false, Message);
}

void URhythmAccountSubsystem::CompleteAuthentication(const bool bSuccess, const FString& Message)
{
	bRequestInProgress = false;
	OnAuthenticationCompleted.Broadcast(bSuccess, Message);
}
