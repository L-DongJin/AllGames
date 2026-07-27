#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DrawingQuizWidget.generated.h"

class ADrawingQuizGameState; class UButton; class UDrawingCanvasWidget; class UEditableTextBox; class UTextBlock; class UVerticalBox; class UWidget;
UCLASS()
class ALLGAMES_API UDrawingQuizWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UDrawingQuizWidget(const FObjectInitializer& ObjectInitializer); void SetSecretWord(const FString& Word);
protected:
	virtual TSharedRef<SWidget> RebuildWidget()override; virtual void NativeConstruct()override; virtual void NativeDestruct()override; virtual void NativeTick(const FGeometry& Geometry,float DeltaTime)override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)override;
private:
	struct FVisibleChatMessage{TObjectPtr<UWidget> Row;double ExpiresAtSeconds=0.0;};
	void BuildLayout(); void BindState(); void RefreshState(); void ReceiveChat(const FString& Name,const FString& Message,int32 PlayerColorIndex); void RefreshPlayers(); void AddChatMessage(const FString& Name,const FString& Message,int32 PlayerColorIndex);
	UFUNCTION() void SubmitChat(); UFUNCTION() void CommitChat(const FText& Text,ETextCommit::Type Method);
	UFUNCTION() void UseBlack(); UFUNCTION() void UseRed(); UFUNCTION() void UseBlue();
	UFUNCTION() void UseOrange(); UFUNCTION() void UseYellow(); UFUNCTION() void UseGreen(); UFUNCTION() void UsePurple();
	UFUNCTION() void UseEraser(); UFUNCTION() void ClearCanvas(); UFUNCTION() void ReturnHub();
	UPROPERTY(Transient) TObjectPtr<ADrawingQuizGameState> QuizState; UPROPERTY(Transient) TObjectPtr<UDrawingCanvasWidget> Canvas; UPROPERTY(Transient) TObjectPtr<UEditableTextBox> ChatInput;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RoundText; UPROPERTY(Transient) TObjectPtr<UTextBlock> TimerText; UPROPERTY(Transient) TObjectPtr<UTextBlock> DrawerText; UPROPERTY(Transient) TObjectPtr<UTextBlock> WordText; UPROPERTY(Transient) TObjectPtr<UTextBlock> FeedbackText; UPROPERTY(Transient) TObjectPtr<UVerticalBox> ChatContainer; UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerText;
	TArray<FVisibleChatMessage> ChatMessages; FString SecretWord;
};
