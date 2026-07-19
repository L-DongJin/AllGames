#include "IdolQuizLobbyGameModeBase.h"
#include "IdolQuizLobbyPlayerController.h"
#include "IdolQuizPlayerState.h"
AIdolQuizLobbyGameModeBase::AIdolQuizLobbyGameModeBase(){DefaultPawnClass=nullptr;PlayerControllerClass=AIdolQuizLobbyPlayerController::StaticClass();PlayerStateClass=AIdolQuizPlayerState::StaticClass();bUseSeamlessTravel=true;}
void AIdolQuizLobbyGameModeBase::StartGameFromLobby(){if(HasAuthority()&&GetWorld())GetWorld()->ServerTravel(TEXT("/Game/Maps/IdolQuizMap?listen"),true);}
