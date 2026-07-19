#include "IdolQuizSessionSubsystem.h"

#include "../Core/RhythmAccountSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Online/Auth.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEOSGS.h"
#include "TimerManager.h"

namespace
{
	const FName RoomLocalName(TEXT("IdolQuizGameLobby"));
	const UE::Online::FSchemaId RoomSchemaId(TEXT("IdolQuizLobby"));
	const UE::Online::FSchemaAttributeId RoomNameAttribute(TEXT("RoomName"));
	const UE::Online::FSchemaAttributeId RoomCategoryAttribute(TEXT("RoomCategory"));

	TSharedPtr<UE::Online::FOnlineServicesEOSGS> GetEOSServices()
	{
		return StaticCastSharedPtr<UE::Online::FOnlineServicesEOSGS>(
			UE::Online::GetServices(UE::Online::EOnlineServices::Epic));
	}

	FString ReadRoomName(const UE::Online::FLobby& Lobby)
	{
		if (const UE::Online::FSchemaVariant* Value = Lobby.Attributes.Find(RoomNameAttribute);
			Value && Value->GetType() == UE::Online::ESchemaAttributeType::String)
		{
			return Value->GetString();
		}
		return TEXT("이름 없는 방");
	}

	EIdolQuizRoomCategory ReadRoomCategory(const UE::Online::FLobby& Lobby)
	{
		if (const UE::Online::FSchemaVariant* Value = Lobby.Attributes.Find(RoomCategoryAttribute);
			Value && Value->GetType() == UE::Online::ESchemaAttributeType::Int64)
		{
			return static_cast<EIdolQuizRoomCategory>(FMath::Clamp<int64>(Value->GetInt64(), 0, 2));
		}
		return EIdolQuizRoomCategory::Idol;
	}
}

void UIdolQuizSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &ThisClass::HandleNetworkFailure);
	}
}

void UIdolQuizSessionSubsystem::Deinitialize()
{
	if (GEngine && NetworkFailureHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FindRetryTimer);
	}
	EOSSearchResults.Reset();
	ActiveEOSLobby.Reset();
	Super::Deinitialize();
}

bool UIdolQuizSessionSubsystem::TryGetEOSAccount(UE::Online::FAccountId& OutAccountId) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const URhythmAccountSubsystem* Accounts = GameInstance
		? GameInstance->GetSubsystem<URhythmAccountSubsystem>()
		: nullptr;
	if (!Accounts || !Accounts->IsEOSLoggedIn())
	{
		return false;
	}

	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::IAuthPtr Auth = Services.IsValid() ? Services->GetAuthInterface() : nullptr;
	if (!Auth.IsValid())
	{
		return false;
	}

	UE::Online::FAuthGetLocalOnlineUserByPlatformUserId::Params Params;
	Params.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);
	const UE::Online::TOnlineResult<UE::Online::FAuthGetLocalOnlineUserByPlatformUserId> Result =
		Auth->GetLocalOnlineUserByPlatformUserId(MoveTemp(Params));
	if (Result.IsError())
	{
		return false;
	}

	OutAccountId = Result.GetOkValue().AccountInfo->AccountId;
	return OutAccountId.IsValid();
}

void UIdolQuizSessionSubsystem::CreateRoom(
	const FString& RoomName,
	const EIdolQuizRoomCategory Category)
{
	PendingRoomName = RoomName.TrimStartAndEnd().Left(24);
	if (PendingRoomName.IsEmpty())
	{
		OnSessionAction.Broadcast(false, TEXT("방 제목을 입력해 주세요."));
		return;
	}
	if (bRoomOperationInProgress)
	{
		OnSessionAction.Broadcast(false, TEXT("이전 방 요청을 처리하고 있습니다."));
		return;
	}
	if (ActiveEOSLobby.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("현재 방에서 나간 뒤 새 방을 만들어 주세요."));
		return;
	}

	PendingCategory = Category;
	CreateEOSRoom();
}

void UIdolQuizSessionSubsystem::CreateEOSRoom()
{
	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("EOS 인터넷 연결이 준비되지 않았습니다. 다시 로그인해 주세요."));
		return;
	}

	UE::Online::FCreateLobby::Params Params;
	Params.LocalAccountId = AccountId;
	Params.LocalName = RoomLocalName;
	Params.SchemaId = RoomSchemaId;
	Params.MaxMembers = 6;
	Params.JoinPolicy = UE::Online::ELobbyJoinPolicy::PublicAdvertised;
	Params.bPresenceEnabled = false;
	Params.Attributes.Emplace(RoomNameAttribute, UE::Online::FSchemaVariant(PendingRoomName));
	Params.Attributes.Emplace(
		RoomCategoryAttribute,
		UE::Online::FSchemaVariant(static_cast<int64>(PendingCategory)));

	bRoomOperationInProgress = true;
	Lobbies->CreateLobby(MoveTemp(Params)).OnComplete(
		this,
		[this](const UE::Online::TOnlineResult<UE::Online::FCreateLobby>& Result)
		{
			bRoomOperationInProgress = false;
			if (Result.IsError() || !Result.GetOkValue().Lobby.IsValid())
			{
				const FString Error = Result.IsError() ? Result.GetErrorValue().GetLogString() : TEXT("No lobby");
				UE_LOG(LogTemp, Error, TEXT("EOS lobby creation failed: %s"), *Error);
				OnSessionAction.Broadcast(false, TEXT("인터넷 방 생성에 실패했습니다."));
				return;
			}

			ActiveEOSLobby = Result.GetOkValue().Lobby;
			UE_LOG(LogTemp, Log, TEXT("Idol Quiz EOS room created: %s"), *PendingRoomName);
			OnSessionAction.Broadcast(true, TEXT("인터넷 방을 만들었습니다."));
			UGameplayStatics::OpenLevel(this, TEXT("IdolQuizLobbyMap"), true, TEXT("listen"));
		});
}

void UIdolQuizSessionSubsystem::FindRooms()
{
	if (bFindInProgress)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FindRetryTimer);
	}
	FindAttempt = 0;
	StartEOSFindAttempt();
}

void UIdolQuizSessionSubsystem::StartEOSFindAttempt()
{
	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnRoomsFound.Broadcast(false, {});
		return;
	}

	bFindInProgress = true;
	++FindAttempt;
	UE::Online::FFindLobbies::Params Params;
	Params.LocalAccountId = AccountId;
	Params.MaxResults = 50;
	Lobbies->FindLobbies(MoveTemp(Params)).OnComplete(
		this,
		[this](const UE::Online::TOnlineResult<UE::Online::FFindLobbies>& Result)
		{
			bFindInProgress = false;
			EOSSearchResults.Reset();
			TArray<FIdolQuizRoomInfo> Rooms;
			if (Result.IsOk())
			{
				EOSSearchResults = Result.GetOkValue().Lobbies;
				for (const TSharedRef<const UE::Online::FLobby>& Lobby : EOSSearchResults)
				{
					FIdolQuizRoomInfo& Room = Rooms.AddDefaulted_GetRef();
					Room.RoomName = ReadRoomName(*Lobby);
					Room.Category = ReadRoomCategory(*Lobby);
					Room.CurrentPlayers = Lobby->Members.Num();
					Room.MaxPlayers = Lobby->MaxMembers;
					Room.PingMs = 0;
				}
			}

			if (Rooms.IsEmpty() && FindAttempt < 3)
			{
				ScheduleFindRetry();
				return;
			}
			OnRoomsFound.Broadcast(Result.IsOk(), Rooms);
		});
}

void UIdolQuizSessionSubsystem::ScheduleFindRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FindRetryTimer,
			this,
			&ThisClass::StartEOSFindAttempt,
			0.35f,
			false);
	}
}

void UIdolQuizSessionSubsystem::JoinRoom(const int32 VisibleRoomIndex)
{
	if (bRoomOperationInProgress)
	{
		OnSessionAction.Broadcast(false, TEXT("이전 방 요청을 처리하고 있습니다."));
		return;
	}
	if (!EOSSearchResults.IsValidIndex(VisibleRoomIndex))
	{
		OnSessionAction.Broadcast(false, TEXT("선택한 방을 찾을 수 없습니다."));
		return;
	}

	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("EOS 인터넷 연결이 준비되지 않았습니다."));
		return;
	}

	UE::Online::FJoinLobby::Params Params;
	Params.LocalAccountId = AccountId;
	Params.LocalName = RoomLocalName;
	Params.LobbyId = EOSSearchResults[VisibleRoomIndex]->LobbyId;
	Params.bPresenceEnabled = false;
	bRoomOperationInProgress = true;
	Lobbies->JoinLobby(MoveTemp(Params)).OnComplete(
		this,
		[this, Services, AccountId](const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& Result)
		{
			bRoomOperationInProgress = false;
			if (Result.IsError() || !Result.GetOkValue().Lobby.IsValid())
			{
				const FString Error = Result.IsError() ? Result.GetErrorValue().GetLogString() : TEXT("No lobby");
				UE_LOG(LogTemp, Error, TEXT("EOS lobby join failed: %s"), *Error);
				OnSessionAction.Broadcast(false, TEXT("인터넷 방 입장에 실패했습니다."));
				return;
			}

			ActiveEOSLobby = Result.GetOkValue().Lobby;
			UE::Online::FGetResolvedConnectString::Params ConnectParams;
			ConnectParams.LocalAccountId = AccountId;
			ConnectParams.LobbyId = ActiveEOSLobby->LobbyId;
			const UE::Online::TOnlineResult<UE::Online::FGetResolvedConnectString> ConnectResult =
				Services->GetResolvedConnectString(MoveTemp(ConnectParams));
			if (ConnectResult.IsError())
			{
				UE_LOG(LogTemp, Error, TEXT("EOS connect string resolution failed: %s"),
					*ConnectResult.GetErrorValue().GetLogString());
				OnSessionAction.Broadcast(false, TEXT("방 연결 주소를 확인하지 못했습니다."));
				LeaveRoom(false);
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("Idol Quiz EOS room joined; starting P2P travel to %s"),
				*ConnectResult.GetOkValue().ResolvedConnectString);
			OnSessionAction.Broadcast(true, TEXT("인터넷 방에 입장합니다."));
			if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
			{
				PlayerController->ClientTravel(
					ConnectResult.GetOkValue().ResolvedConnectString,
					TRAVEL_Absolute);
			}
		});
}

void UIdolQuizSessionSubsystem::LeaveRoom(const bool bReturnToBrowser)
{
	bReturnAfterLeave = bReturnToBrowser;
	if (!ActiveEOSLobby.IsValid())
	{
		if (bReturnAfterLeave)
		{
			ReturnToRoomBrowser();
		}
		return;
	}

	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		ActiveEOSLobby.Reset();
		if (bReturnAfterLeave)
		{
			ReturnToRoomBrowser();
		}
		return;
	}

	UE::Online::FLeaveLobby::Params Params;
	Params.LocalAccountId = AccountId;
	Params.LobbyId = ActiveEOSLobby->LobbyId;
	bRoomOperationInProgress = true;
	Lobbies->LeaveLobby(MoveTemp(Params)).OnComplete(
		this,
		[this](const UE::Online::TOnlineResult<UE::Online::FLeaveLobby>& Result)
		{
			bRoomOperationInProgress = false;
			if (Result.IsError())
			{
				UE_LOG(LogTemp, Warning, TEXT("EOS lobby leave failed: %s"),
					*Result.GetErrorValue().GetLogString());
			}
			ActiveEOSLobby.Reset();
			if (bReturnAfterLeave)
			{
				bReturnAfterLeave = false;
				ReturnToRoomBrowser();
			}
		});
}

bool UIdolQuizSessionSubsystem::HasActiveSession() const
{
	return ActiveEOSLobby.IsValid();
}

EIdolQuizRoomCategory UIdolQuizSessionSubsystem::GetActiveRoomCategory() const
{
	return ActiveEOSLobby.IsValid() ? ReadRoomCategory(*ActiveEOSLobby) : PendingCategory;
}

FString UIdolQuizSessionSubsystem::GetCategoryLabel(const EIdolQuizRoomCategory Category)
{
	switch (Category)
	{
	case EIdolQuizRoomCategory::Actor:
		return TEXT("배우");
	case EIdolQuizRoomCategory::IdolAndActor:
		return TEXT("아이돌 + 배우");
	default:
		return TEXT("아이돌");
	}
}

void UIdolQuizSessionSubsystem::ReturnToRoomBrowser()
{
	UGameplayStatics::OpenLevel(this, TEXT("IdolQuizRoomMap"));
}

void UIdolQuizSessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver*,
	ENetworkFailure::Type,
	const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || World->GetNetMode() != NM_Client)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Idol Quiz EOS host connection lost: %s"), *ErrorString);
	LeaveRoom(true);
}
