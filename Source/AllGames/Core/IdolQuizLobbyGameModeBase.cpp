#include "IdolQuizLobbyGameModeBase.h"
#include "IdolQuizLobbyPlayerController.h"
#include "IdolQuizPlayerState.h"
#include "../Online/IdolQuizSessionSubsystem.h"
AIdolQuizLobbyGameModeBase::AIdolQuizLobbyGameModeBase(){DefaultPawnClass=nullptr;PlayerControllerClass=AIdolQuizLobbyPlayerController::StaticClass();PlayerStateClass=AIdolQuizPlayerState::StaticClass();bUseSeamlessTravel=true;}
void AIdolQuizLobbyGameModeBase::StartGameFromLobby()
{
	if (!HasAuthority() || !GetWorld()) return;
	EMiniGameRoomType GameType = EMiniGameRoomType::PersonQuiz;
	if (UGameInstance* GI = GetGameInstance())
		if (UIdolQuizSessionSubsystem* Sessions = GI->GetSubsystem<UIdolQuizSessionSubsystem>())
			GameType = Sessions->GetActiveGameType();
	const TCHAR* Map = GameType == EMiniGameRoomType::DrawingQuiz
		? TEXT("/Game/Maps/DrawingQuizMap?listen")
		: TEXT("/Game/Maps/IdolQuizMap?listen");
	GetWorld()->ServerTravel(Map, true);
}
