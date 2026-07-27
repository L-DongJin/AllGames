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

UENUM(BlueprintType)
enum class EMiniGameRoomType : uint8
{
	PersonQuiz,
	DrawingQuiz
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

	UPROPERTY(BlueprintReadOnly)
	int32 QuestionCount = 50;

	UPROPERTY(BlueprintReadOnly)
	EMiniGameRoomType GameType = EMiniGameRoomType::PersonQuiz;

	UPROPERTY(BlueprintReadOnly)
	int32 DrawingRoundsPerPlayer = 2;

	UPROPERTY(BlueprintReadOnly)
	int32 DrawingRoundTime = 60;
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

	void CreateRoom(const FString& RoomName, EMiniGameRoomType GameType,
		EIdolQuizRoomCategory Category, int32 QuestionCount,
		int32 DrawingRoundsPerPlayer, int32 DrawingRoundTime);
	void FindRooms();
	void JoinRoom(int32 VisibleRoomIndex);
	void LeaveRoom(bool bReturnToBrowser = true);

	bool HasActiveSession() const;
	FString GetActiveRoomName() const;
	EIdolQuizRoomCategory GetActiveRoomCategory() const;
	int32 GetActiveRoomQuestionCount() const;
	EMiniGameRoomType GetActiveGameType() const;
	int32 GetActiveDrawingRoundsPerPlayer() const;
	int32 GetActiveDrawingRoundTime() const;
	static FString GetCategoryLabel(EIdolQuizRoomCategory Category);
	static FString GetGameTypeLabel(EMiniGameRoomType GameType);

private:
	bool TryGetEOSAccount(UE::Online::FAccountId& OutAccountId) const;
	void CreateEOSRoom();
	void CleanupJoinedLobbiesAndCreate();
	void CreateEOSRoomInternal();
	void StartEOSFindAttempt();
	void ScheduleFindRetry();
	void LeaveEOSLobbyNow();
	void ReturnToRoomBrowser();
	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);

	FString PendingRoomName;
	EIdolQuizRoomCategory PendingCategory = EIdolQuizRoomCategory::Idol;
	int32 PendingQuestionCount = 50;
	EMiniGameRoomType PendingGameType = EMiniGameRoomType::PersonQuiz;
	int32 PendingDrawingRoundsPerPlayer = 2;
	int32 PendingDrawingRoundTime = 60;
	bool bFindInProgress = false;
	bool bRoomOperationInProgress = false;
	bool bReturnAfterLeave = false;
	int32 FindAttempt = 0;
	FTimerHandle FindRetryTimer;
	FDelegateHandle NetworkFailureHandle;
	TArray<TSharedRef<const UE::Online::FLobby>> EOSSearchResults;
	TSharedPtr<const UE::Online::FLobby> ActiveEOSLobby;
};
