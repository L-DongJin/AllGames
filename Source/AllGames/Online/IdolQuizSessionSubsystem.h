#pragma once

#include "CoreMinimal.h"
#include "Online/Lobbies.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IdolQuizSessionSubsystem.generated.h"

UENUM(BlueprintType)
enum class EIdolQuizRoomCategory : uint8
{
	Idol,
	Actor,
	IdolAndActor
};

USTRUCT(BlueprintType)
struct FIdolQuizRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString RoomName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 6;

	UPROPERTY(BlueprintReadOnly)
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly)
	EIdolQuizRoomCategory Category = EIdolQuizRoomCategory::Idol;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIdolRoomsFound, bool, const TArray<FIdolQuizRoomInfo>&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIdolSessionAction, bool, const FString&);

UCLASS()
class ALLGAMES_API UIdolQuizSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FOnIdolRoomsFound OnRoomsFound;
	FOnIdolSessionAction OnSessionAction;

	void CreateRoom(const FString& RoomName, EIdolQuizRoomCategory Category);
	void FindRooms();
	void JoinRoom(int32 VisibleRoomIndex);
	void LeaveRoom(bool bReturnToBrowser = true);

	bool HasActiveSession() const;
	EIdolQuizRoomCategory GetActiveRoomCategory() const;
	static FString GetCategoryLabel(EIdolQuizRoomCategory Category);

private:
	bool TryGetEOSAccount(UE::Online::FAccountId& OutAccountId) const;
	void CreateEOSRoom();
	void StartEOSFindAttempt();
	void ScheduleFindRetry();
	void ReturnToRoomBrowser();
	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);

	FString PendingRoomName;
	EIdolQuizRoomCategory PendingCategory = EIdolQuizRoomCategory::Idol;
	bool bFindInProgress = false;
	bool bRoomOperationInProgress = false;
	bool bReturnAfterLeave = false;
	int32 FindAttempt = 0;
	FTimerHandle FindRetryTimer;
	FDelegateHandle NetworkFailureHandle;
	TArray<TSharedRef<const UE::Online::FLobby>> EOSSearchResults;
	TSharedPtr<const UE::Online::FLobby> ActiveEOSLobby;
};
