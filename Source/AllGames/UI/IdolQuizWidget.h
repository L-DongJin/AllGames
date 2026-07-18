// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Data/IdolQuizCatalogDataAsset.h"
#include "IdolQuizWidget.generated.h"
class AIdolQuizGameModeBase; class UButton; class UEditableTextBox; class UImage; class UTextBlock;
UCLASS()
class ALLGAMES_API UIdolQuizWidget:public UUserWidget
{
	GENERATED_BODY()
public: UIdolQuizWidget(const FObjectInitializer& ObjectInitializer);
protected: virtual TSharedRef<SWidget> RebuildWidget()override; virtual void NativeConstruct()override; virtual void NativeDestruct()override;
private:
	void BuildLayout(); void BindGameMode(); void HandleQuestionChanged(const FIdolQuizQuestion& Question,int32 Round,int32 Total);
	void HandleAnswerResolved(bool bCorrect,const FString& Answer,int32 Score); void HandleQuizFinished(int32 Score,int32 Total);
	UFUNCTION()void HandleSubmit(); UFUNCTION()void HandleTextCommitted(const FText& Text,ETextCommit::Type Method);
	UFUNCTION()void HandleRestart(); UFUNCTION()void HandleMainHub();
	UPROPERTY(Transient)TObjectPtr<UImage>FaceImage; UPROPERTY(Transient)TObjectPtr<UTextBlock>RoundText;
	UPROPERTY(Transient)TObjectPtr<UTextBlock>ScoreText; UPROPERTY(Transient)TObjectPtr<UTextBlock>ChatText;
	UPROPERTY(Transient)TObjectPtr<UTextBlock>FeedbackText; UPROPERTY(Transient)TObjectPtr<UEditableTextBox>AnswerInput;
	UPROPERTY(Transient)TObjectPtr<UButton>SubmitButton; UPROPERTY(Transient)TObjectPtr<UButton>RestartButton;
	UPROPERTY(Transient)TObjectPtr<AIdolQuizGameModeBase>QuizGameMode; TArray<FString>ChatLines;
};
