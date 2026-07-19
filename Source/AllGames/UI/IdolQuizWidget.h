// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Data/IdolQuizCatalogDataAsset.h"
#include "IdolQuizWidget.generated.h"
class AIdolQuizGameStateBase; class UButton; class UEditableTextBox; class UImage; class UTextBlock;
UCLASS()
class ALLGAMES_API UIdolQuizWidget:public UUserWidget
{
	GENERATED_BODY()
public: UIdolQuizWidget(const FObjectInitializer& ObjectInitializer);
protected: virtual TSharedRef<SWidget> RebuildWidget()override; virtual void NativeConstruct()override; virtual void NativeDestruct()override; virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override; virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	struct FVisibleChatMessage{FString Text;double ExpiresAtSeconds=0.0;};
	void BuildLayout(); void BindGameState(); void RefreshReplicatedState(); void HandleNetworkChat(const FString& PlayerName,const FString& Message); void HandleNetworkFeedback(bool bCorrect,const FString& PlayerName,const FString& Message);
	void HandleQuestionChanged(const FIdolQuizQuestion& Question,int32 Round,int32 Total);
	void HandleAnswerResolved(bool bCorrect,const FString& Answer,int32 Score); void HandleQuizFinished(int32 Score,int32 Total);
	void HandleTimeChanged(int32 RemainingSeconds); void HandleHintChanged(const FString& Hint); void HandleRoundTimedOut(const FString& CorrectAnswer);
	void AddChatMessage(const FString& PlayerName,const FString& Message); void RefreshChatText(); void RefreshPlayerSlots(int32 LocalCorrectCount=0); void OpenChatInput(); void CloseChatInput();
	UFUNCTION()void HandleSubmit(); UFUNCTION()void HandleTextCommitted(const FText& Text,ETextCommit::Type Method);
	UFUNCTION()void HandleRestart(); UFUNCTION()void HandleMainHub();
	UPROPERTY(Transient)TObjectPtr<UImage>FaceImage; UPROPERTY(Transient)TObjectPtr<UTextBlock>RoundText;
	UPROPERTY(Transient)TArray<TObjectPtr<UTextBlock>>PlayerNameTexts; UPROPERTY(Transient)TArray<TObjectPtr<UTextBlock>>PlayerCountTexts; UPROPERTY(Transient)TObjectPtr<UTextBlock>ChatText;
	UPROPERTY(Transient)TObjectPtr<UTextBlock>TimerText; UPROPERTY(Transient)TObjectPtr<UTextBlock>HintText;
	UPROPERTY(Transient)TObjectPtr<UTextBlock>FeedbackText; UPROPERTY(Transient)TObjectPtr<UEditableTextBox>AnswerInput;
	UPROPERTY(Transient)TObjectPtr<UButton>SubmitButton; UPROPERTY(Transient)TObjectPtr<UButton>RestartButton;
	UPROPERTY(Transient)TObjectPtr<AIdolQuizGameStateBase>QuizGameState; TArray<FVisibleChatMessage>ChatMessages; FString LocalPlayerName=TEXT("Player"); bool bChatInputOpen=false;
};
