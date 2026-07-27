#include "DrawingQuizPlayerController.h"
#include "DrawingQuizGameMode.h"
#include "DrawingQuizPlayerState.h"
#include "../Core/RhythmAccountSubsystem.h"
#include "../UI/DrawingQuizWidget.h"

void ADrawingQuizPlayerController::BeginPlay(){Super::BeginPlay();if(!IsLocalController())return;QuizWidget=CreateWidget<UDrawingQuizWidget>(this,UDrawingQuizWidget::StaticClass());if(QuizWidget){QuizWidget->AddToViewport();QuizWidget->SetKeyboardFocus();FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(QuizWidget->TakeWidget());Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);bShowMouseCursor=true;}FString Name=TEXT("Player");if(UGameInstance* GI=GetGameInstance())if(URhythmAccountSubsystem* Account=GI->GetSubsystem<URhythmAccountSubsystem>())if(!Account->GetUsername().IsEmpty())Name=Account->GetUsername();ServerSetPlayerName(Name);}
void ADrawingQuizPlayerController::ServerSetPlayerName_Implementation(const FString& Name){if(ADrawingQuizPlayerState* PS=GetPlayerState<ADrawingQuizPlayerState>())PS->SetQuizName(Name);}
void ADrawingQuizPlayerController::ServerSubmitChat_Implementation(const FString& Message){if(ADrawingQuizGameMode* GM=GetWorld()->GetAuthGameMode<ADrawingQuizGameMode>())GM->SubmitChat(this,Message);}
void ADrawingQuizPlayerController::ServerDrawSegments_Implementation(const TArray<FDrawingQuizStrokeSegment>& Segments){if(ADrawingQuizGameMode* GM=GetWorld()->GetAuthGameMode<ADrawingQuizGameMode>())GM->SubmitStrokes(this,Segments);}
void ADrawingQuizPlayerController::ServerClearCanvas_Implementation(){if(ADrawingQuizGameMode* GM=GetWorld()->GetAuthGameMode<ADrawingQuizGameMode>())GM->ClearCanvas(this);}
void ADrawingQuizPlayerController::ClientSetSecretWord_Implementation(const FString& Word){if(QuizWidget)QuizWidget->SetSecretWord(Word);}
