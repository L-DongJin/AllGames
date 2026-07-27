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
	const UE::Online::FSchemaAttributeId QuestionCountAttribute(TEXT("QuestionCount"));
	const UE::Online::FSchemaAttributeId GameTypeAttribute(TEXT("GameType"));
	const UE::Online::FSchemaAttributeId DrawingRoundsAttribute(TEXT("DrawingRounds"));
	const UE::Online::FSchemaAttributeId DrawingRoundTimeAttribute(TEXT("DrawingRoundTime"));

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

	int32 ReadQuestionCount(const UE::Online::FLobby& Lobby)
	{
		if (const UE::Online::FSchemaVariant* Value = Lobby.Attributes.Find(QuestionCountAttribute);
			Value && Value->GetType() == UE::Online::ESchemaAttributeType::Int64)
		{
			return FMath::Clamp(static_cast<int32>(Value->GetInt64()), 50, 300);
		}
		return 50;
	}

	int32 ReadIntAttribute(const UE::Online::FLobby& Lobby, const UE::Online::FSchemaAttributeId Id, const int32 DefaultValue)
	{
		if (const UE::Online::FSchemaVariant* Value = Lobby.Attributes.Find(Id);
			Value && Value->GetType() == UE::Online::ESchemaAttributeType::Int64)
		{
			return static_cast<int32>(Value->GetInt64());
		}
		return DefaultValue;
	}

	EMiniGameRoomType ReadGameType(const UE::Online::FLobby& Lobby)
	{
		return static_cast<EMiniGameRoomType>(FMath::Clamp(
			ReadIntAttribute(Lobby, GameTypeAttribute, 0), 0, 1));
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
	const EMiniGameRoomType GameType,
	const EIdolQuizRoomCategory Category,
	const int32 QuestionCount,
	const int32 DrawingRoundsPerPlayer,
	const int32 DrawingRoundTime)
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
	// Reaching room creation from the browser means any previously tracked lobby
	// is stale for this UI flow. Keep the backend membership for Restore/cleanup,
	// but release the local guard so creation can proceed to that cleanup stage.
	if (ActiveEOSLobby.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Replacing the currently tracked EOS lobby before room creation."));
		ActiveEOSLobby.Reset();
	}
	if (ActiveEOSLobby.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("현재 방에서 나간 뒤 새 방을 만들어 주세요."));
		return;
	}

	PendingCategory = Category;
	PendingQuestionCount = FMath::Clamp(FMath::RoundToInt(static_cast<float>(QuestionCount) / 50.0f) * 50, 50, 300);
	PendingGameType = GameType;
	PendingDrawingRoundsPerPlayer = FMath::Clamp(DrawingRoundsPerPlayer, 1, 5);
	PendingDrawingRoundTime = FMath::Clamp(DrawingRoundTime, 30, 120);
	CreateEOSRoom();
}

void UIdolQuizSessionSubsystem::CreateEOSRoom()
{
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	UE::Online::FAccountId AccountId;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("EOS 연결이 준비되지 않았습니다. 다시 로그인해 주세요."));
		return;
	}

	// Rebuild EOSGS's local joined-lobby registry first. A forced PIE/app shutdown can
	// leave backend membership that GetJoinedLobbies cannot see until it is restored.
	bRoomOperationInProgress = true;
	UE::Online::FRestoreLobbies::Params RestoreParams;
	Lobbies->RestoreLobbies(MoveTemp(RestoreParams)).OnComplete(
		this,
		[this](const UE::Online::TOnlineResult<UE::Online::FRestoreLobbies>& RestoreResult)
		{
			bRoomOperationInProgress = false;
			if (RestoreResult.IsError())
			{
				UE_LOG(LogTemp, Verbose, TEXT("EOS lobby restore before create returned: %s"),
					*RestoreResult.GetErrorValue().GetLogString());
			}
			CleanupJoinedLobbiesAndCreate();
		});
}

void UIdolQuizSessionSubsystem::CleanupJoinedLobbiesAndCreate()
{
	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("EOS 인터넷 연결이 준비되지 않았습니다. 다시 로그인해 주세요."));
		return;
	}

	UE::Online::FGetJoinedLobbies::Params JoinedParams;
	JoinedParams.LocalAccountId = AccountId;
	const UE::Online::TOnlineResult<UE::Online::FGetJoinedLobbies> JoinedResult =
		Lobbies->GetJoinedLobbies(MoveTemp(JoinedParams));
	if (JoinedResult.IsOk() && !JoinedResult.GetOkValue().Lobbies.IsEmpty())
	{
		const TArray<TSharedRef<const UE::Online::FLobby>> JoinedLobbies = JoinedResult.GetOkValue().Lobbies;
		const TSharedRef<int32> Remaining = MakeShared<int32>(JoinedLobbies.Num());
		const TSharedRef<bool> bCleanupFailed = MakeShared<bool>(false);
		bRoomOperationInProgress = true;
		UE_LOG(LogTemp, Log, TEXT("Cleaning %d joined EOS lobby/lobbies before room creation."),
			JoinedLobbies.Num());

		for (const TSharedRef<const UE::Online::FLobby>& Lobby : JoinedLobbies)
		{
			UE::Online::FLeaveLobby::Params LeaveParams;
			LeaveParams.LocalAccountId = AccountId;
			LeaveParams.LobbyId = Lobby->LobbyId;
			Lobbies->LeaveLobby(MoveTemp(LeaveParams)).OnComplete(
				this,
				[this, Remaining, bCleanupFailed](
					const UE::Online::TOnlineResult<UE::Online::FLeaveLobby>& LeaveResult)
				{
					if (LeaveResult.IsError())
					{
						*bCleanupFailed = true;
						UE_LOG(LogTemp, Warning, TEXT("EOS pre-create lobby cleanup failed: %s"),
							*LeaveResult.GetErrorValue().GetLogString());
					}

					--(*Remaining);
					if (*Remaining == 0)
					{
						bRoomOperationInProgress = false;
						ActiveEOSLobby.Reset();
						if (*bCleanupFailed)
						{
							OnSessionAction.Broadcast(false,
								TEXT("이전 방 정리에 실패했습니다. 잠시 후 다시 시도해 주세요."));
							return;
						}
						CreateEOSRoomInternal();
					}
				});
		}
		return;
	}

	CreateEOSRoomInternal();
}

void UIdolQuizSessionSubsystem::CreateEOSRoomInternal()
{
	UE::Online::FAccountId AccountId;
	const TSharedPtr<UE::Online::FOnlineServicesEOSGS> Services = GetEOSServices();
	const UE::Online::ILobbiesPtr Lobbies = Services.IsValid() ? Services->GetLobbiesInterface() : nullptr;
	if (!TryGetEOSAccount(AccountId) || !Lobbies.IsValid())
	{
		OnSessionAction.Broadcast(false, TEXT("EOS 연결이 준비되지 않았습니다. 다시 로그인해 주세요."));
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
	Params.Attributes.Emplace(
		QuestionCountAttribute,
		UE::Online::FSchemaVariant(static_cast<int64>(PendingQuestionCount)));
	Params.Attributes.Emplace(GameTypeAttribute, UE::Online::FSchemaVariant(static_cast<int64>(PendingGameType)));
	Params.Attributes.Emplace(DrawingRoundsAttribute, UE::Online::FSchemaVariant(static_cast<int64>(PendingDrawingRoundsPerPlayer)));
	Params.Attributes.Emplace(DrawingRoundTimeAttribute, UE::Online::FSchemaVariant(static_cast<int64>(PendingDrawingRoundTime)));

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
		[this, Lobbies, AccountId](const UE::Online::TOnlineResult<UE::Online::FFindLobbies>& Result)
		{
			bFindInProgress = false;
			EOSSearchResults.Reset();
			TArray<FIdolQuizRoomInfo> Rooms;
			if (Result.IsOk())
			{
				for (const TSharedRef<const UE::Online::FLobby>& Lobby : Result.GetOkValue().Lobbies)
				{
					// A PIE crash or forced shutdown can leave this local EOS account registered
					// in a lobby even though the GameInstance no longer tracks it. Such a row
					// cannot be joined again (EOS_Lobby_LobbyAlreadyExists), so clean it up.
					if (!ActiveEOSLobby.IsValid() && Lobby->Members.Contains(AccountId))
					{
						UE::Online::FLeaveLobby::Params CleanupParams;
						CleanupParams.LocalAccountId = AccountId;
						CleanupParams.LobbyId = Lobby->LobbyId;
						Lobbies->LeaveLobby(MoveTemp(CleanupParams)).OnComplete(
							this,
							[RoomName = ReadRoomName(*Lobby)](
								const UE::Online::TOnlineResult<UE::Online::FLeaveLobby>& CleanupResult)
							{
								if (CleanupResult.IsError())
								{
									UE_LOG(LogTemp, Warning, TEXT("EOS orphan lobby cleanup failed for %s: %s"),
										*RoomName, *CleanupResult.GetErrorValue().GetLogString());
								}
								else
								{
									UE_LOG(LogTemp, Log, TEXT("EOS orphan lobby cleaned up: %s"), *RoomName);
								}
							});
						continue;
					}
					// EOS discovery can briefly return a cached lobby after its last member has left.
					// It has no listen server and cannot be joined, so never expose it as a room row.
					if (Lobby->Members.IsEmpty())
					{
						UE_LOG(LogTemp, Verbose, TEXT("Ignoring empty cached EOS lobby: %s"),
							*ReadRoomName(*Lobby));
						continue;
					}
					EOSSearchResults.Add(Lobby);
					FIdolQuizRoomInfo& Room = Rooms.AddDefaulted_GetRef();
					Room.RoomName = ReadRoomName(*Lobby);
					Room.Category = ReadRoomCategory(*Lobby);
					Room.QuestionCount = ReadQuestionCount(*Lobby);
					Room.GameType = ReadGameType(*Lobby);
					Room.DrawingRoundsPerPlayer = FMath::Clamp(ReadIntAttribute(*Lobby, DrawingRoundsAttribute, 2), 1, 5);
					Room.DrawingRoundTime = FMath::Clamp(ReadIntAttribute(*Lobby, DrawingRoundTimeAttribute, 60), 30, 120);
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

			const FString ResolvedConnectString = ConnectResult.GetOkValue().ResolvedConnectString;
			// UE 5.7 EOSGS returns "EOS:PUID". FURL treats a single-colon value as a
			// protocol plus map name, so bracket it to preserve Host="EOS:PUID" for NetDriverEOS.
			const FString TravelURL = FString::Printf(TEXT("[%s]"), *ResolvedConnectString);
			UE_LOG(LogTemp, Log, TEXT("Idol Quiz EOS room joined; starting P2P travel to %s"),
				*TravelURL);
			OnSessionAction.Broadcast(true, TEXT("인터넷 방에 입장합니다."));
			if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
			{
				PlayerController->ClientTravel(
					TravelURL,
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

	// The final owner first stops advertising the lobby. EOS may retain an empty lobby
	// snapshot briefly after LeaveLobby, but it will no longer be returned as joinable.
	if (ActiveEOSLobby->OwnerAccountId == AccountId && ActiveEOSLobby->Members.Num() <= 1 &&
		ActiveEOSLobby->JoinPolicy == UE::Online::ELobbyJoinPolicy::PublicAdvertised)
	{
		UE::Online::FModifyLobbyJoinPolicy::Params CloseParams;
		CloseParams.LocalAccountId = AccountId;
		CloseParams.LobbyId = ActiveEOSLobby->LobbyId;
		CloseParams.JoinPolicy = UE::Online::ELobbyJoinPolicy::InvitationOnly;
		bRoomOperationInProgress = true;
		Lobbies->ModifyLobbyJoinPolicy(MoveTemp(CloseParams)).OnComplete(
			this,
			[this](const UE::Online::TOnlineResult<UE::Online::FModifyLobbyJoinPolicy>& Result)
			{
				bRoomOperationInProgress = false;
				if (Result.IsError())
				{
					UE_LOG(LogTemp, Warning, TEXT("EOS empty lobby close failed before leave: %s"),
						*Result.GetErrorValue().GetLogString());
				}
				LeaveEOSLobbyNow();
			});
		return;
	}

	LeaveEOSLobbyNow();
}

void UIdolQuizSessionSubsystem::LeaveEOSLobbyNow()
{
	if (!ActiveEOSLobby.IsValid())
	{
		if (bReturnAfterLeave)
		{
			bReturnAfterLeave = false;
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
			bReturnAfterLeave = false;
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

FString UIdolQuizSessionSubsystem::GetActiveRoomName() const
{
	return ActiveEOSLobby.IsValid() ? ReadRoomName(*ActiveEOSLobby) : PendingRoomName;
}

EIdolQuizRoomCategory UIdolQuizSessionSubsystem::GetActiveRoomCategory() const
{
	return ActiveEOSLobby.IsValid() ? ReadRoomCategory(*ActiveEOSLobby) : PendingCategory;
}

int32 UIdolQuizSessionSubsystem::GetActiveRoomQuestionCount() const
{
	return ActiveEOSLobby.IsValid() ? ReadQuestionCount(*ActiveEOSLobby) : PendingQuestionCount;
}

EMiniGameRoomType UIdolQuizSessionSubsystem::GetActiveGameType() const
{
	return ActiveEOSLobby.IsValid() ? ReadGameType(*ActiveEOSLobby) : PendingGameType;
}

int32 UIdolQuizSessionSubsystem::GetActiveDrawingRoundsPerPlayer() const
{
	return ActiveEOSLobby.IsValid()
		? FMath::Clamp(ReadIntAttribute(*ActiveEOSLobby, DrawingRoundsAttribute, 2), 1, 5)
		: PendingDrawingRoundsPerPlayer;
}

int32 UIdolQuizSessionSubsystem::GetActiveDrawingRoundTime() const
{
	return ActiveEOSLobby.IsValid()
		? FMath::Clamp(ReadIntAttribute(*ActiveEOSLobby, DrawingRoundTimeAttribute, 60), 30, 120)
		: PendingDrawingRoundTime;
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

FString UIdolQuizSessionSubsystem::GetGameTypeLabel(const EMiniGameRoomType GameType)
{
	return GameType == EMiniGameRoomType::DrawingQuiz ? TEXT("그림 퀴즈") : TEXT("인물 퀴즈");
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
