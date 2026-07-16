// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmAccountSubsystem.h"

#include "PlayFab.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabError.h"
#include "PlayFabRuntimeSettings.h"

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
