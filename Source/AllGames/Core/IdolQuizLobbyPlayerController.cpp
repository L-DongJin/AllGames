#include "IdolQuizLobbyPlayerController.h"
#include "IdolQuizLobbyGameModeBase.h"
#include "IdolQuizPlayerState.h"
#include "RhythmAccountSubsystem.h"
#include "../UI/IdolQuizLobbyWidget.h"
void AIdolQuizLobbyPlayerController::BeginPlay(){Super::BeginPlay();if(IsLocalController()){UClass* LobbyClass=LoadClass<UIdolQuizLobbyWidget>(nullptr,TEXT("/Game/UI/WBP_SharedLobby.WBP_SharedLobby_C"));LobbyWidget=CreateWidget<UIdolQuizLobbyWidget>(this,LobbyClass?LobbyClass:UIdolQuizLobbyWidget::StaticClass());if(LobbyWidget){LobbyWidget->AddToViewport();LobbyWidget->SetKeyboardFocus();bShowMouseCursor=true;FInputModeUIOnly Mode;Mode.SetWidgetToFocus(LobbyWidget->TakeWidget());SetInputMode(Mode);}FString Name=TEXT("Player");if(UGameInstance* GI=GetGameInstance())if(URhythmAccountSubsystem* Account=GI->GetSubsystem<URhythmAccountSubsystem>())if(!Account->GetUsername().IsEmpty())Name=Account->GetUsername();ServerSetQuizPlayerName(Name);}}
void AIdolQuizLobbyPlayerController::ServerSetQuizPlayerName_Implementation(const FString& Name){if(AIdolQuizPlayerState* PS=GetPlayerState<AIdolQuizPlayerState>())PS->SetQuizPlayerName(Name);}
void AIdolQuizLobbyPlayerController::ServerStartSelectedGame_Implementation(){if(!HasAuthority())return;if(AIdolQuizLobbyGameModeBase* GM=GetWorld()->GetAuthGameMode<AIdolQuizLobbyGameModeBase>())GM->StartGameFromLobby();}
