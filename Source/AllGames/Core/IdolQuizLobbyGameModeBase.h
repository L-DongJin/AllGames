#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IdolQuizLobbyGameModeBase.generated.h"
UCLASS()
class ALLGAMES_API AIdolQuizLobbyGameModeBase:public AGameModeBase
{
	GENERATED_BODY()
public:AIdolQuizLobbyGameModeBase();void StartGameFromLobby();
};
