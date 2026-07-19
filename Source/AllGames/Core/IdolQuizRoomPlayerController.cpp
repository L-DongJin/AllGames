#include "IdolQuizRoomPlayerController.h"
#include "../UI/IdolQuizRoomWidget.h"
void AIdolQuizRoomPlayerController::BeginPlay(){Super::BeginPlay();if(IsLocalController()){RoomWidget=CreateWidget<UIdolQuizRoomWidget>(this,UIdolQuizRoomWidget::StaticClass());if(RoomWidget){RoomWidget->AddToViewport();RoomWidget->SetKeyboardFocus();bShowMouseCursor=true;FInputModeUIOnly Mode;Mode.SetWidgetToFocus(RoomWidget->TakeWidget());SetInputMode(Mode);}}}
