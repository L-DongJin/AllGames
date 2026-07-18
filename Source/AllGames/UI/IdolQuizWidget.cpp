// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "../Core/IdolQuizGameModeBase.h"
namespace{UTextBlock* QuizText(UWidgetTree* Tree,const TCHAR* Name,const FString& Value,int32 Size){UTextBlock* Text=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Text->SetText(FText::FromString(Value));Text->SetJustification(ETextJustify::Center);Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));FSlateFontInfo Font=Text->GetFont();Font.Size=Size;Text->SetFont(Font);return Text;}}
UIdolQuizWidget::UIdolQuizWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){SetIsFocusable(true);}
TSharedRef<SWidget> UIdolQuizWidget::RebuildWidget(){if(WidgetTree&&!WidgetTree->RootWidget)BuildLayout();return Super::RebuildWidget();}
void UIdolQuizWidget::BuildLayout()
{
	UCanvasPanel* Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("IdolQuizRoot"));WidgetTree->RootWidget=Root;
	auto Add=[Root](UWidget* Widget,float X,float Y,FVector2D Size){UCanvasPanelSlot* CanvasSlot=Root->AddChildToCanvas(Widget);CanvasSlot->SetAnchors(FAnchors(X,Y));CanvasSlot->SetAlignment(FVector2D(.5f,.5f));CanvasSlot->SetSize(Size);};
	UImage* Background=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Background"));Background->SetColorAndOpacity(FLinearColor(.006f,.012f,.035f,1));UCanvasPanelSlot* BackgroundSlot=Root->AddChildToCanvas(Background);BackgroundSlot->SetAnchors(FAnchors(0,0,1,1));BackgroundSlot->SetOffsets(FMargin(0));
	Add(QuizText(WidgetTree,TEXT("Title"),TEXT("아이돌 얼굴 맞히기"),42),.5f,.06f,FVector2D(800,70));
	RoundText=QuizText(WidgetTree,TEXT("RoundText"),TEXT("1 / 10"),26);Add(RoundText,.16f,.10f,FVector2D(300,50));
	ScoreText=QuizText(WidgetTree,TEXT("ScoreText"),TEXT("SCORE 0"),26);Add(ScoreText,.84f,.10f,FVector2D(300,50));
	FaceImage=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("FaceImage"));Add(FaceImage,.38f,.46f,FVector2D(620,620));
	ChatText=QuizText(WidgetTree,TEXT("ChatText"),TEXT("정답을 입력해보세요."),22);ChatText->SetJustification(ETextJustify::Left);Add(ChatText,.78f,.42f,FVector2D(440,500));
	FeedbackText=QuizText(WidgetTree,TEXT("FeedbackText"),TEXT(""),30);Add(FeedbackText,.5f,.79f,FVector2D(700,55));
	AnswerInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("AnswerInput"));AnswerInput->SetHintText(FText::FromString(TEXT("채팅으로 정답 입력")));AnswerInput->OnTextCommitted.AddDynamic(this,&ThisClass::HandleTextCommitted);Add(AnswerInput,.42f,.88f,FVector2D(620,58));
	SubmitButton=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("SubmitButton"));SubmitButton->AddChild(QuizText(WidgetTree,TEXT("SubmitText"),TEXT("입력"),24));SubmitButton->OnClicked.AddDynamic(this,&ThisClass::HandleSubmit);Add(SubmitButton,.69f,.88f,FVector2D(150,58));
	RestartButton=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("RestartButton"));RestartButton->AddChild(QuizText(WidgetTree,TEXT("RestartText"),TEXT("다시 하기"),24));RestartButton->OnClicked.AddDynamic(this,&ThisClass::HandleRestart);Add(RestartButton,.42f,.88f,FVector2D(230,62));RestartButton->SetVisibility(ESlateVisibility::Collapsed);
	UButton* HubButton=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("HubButton"));HubButton->AddChild(QuizText(WidgetTree,TEXT("HubText"),TEXT("ALL GAMES"),20));HubButton->OnClicked.AddDynamic(this,&ThisClass::HandleMainHub);Add(HubButton,.08f,.95f,FVector2D(170,48));
}
void UIdolQuizWidget::NativeConstruct(){Super::NativeConstruct();BindGameMode();if(AnswerInput)AnswerInput->SetKeyboardFocus();}
void UIdolQuizWidget::BindGameMode(){QuizGameMode=Cast<AIdolQuizGameModeBase>(UGameplayStatics::GetGameMode(this));if(!QuizGameMode)return;QuizGameMode->OnQuestionChanged.AddUObject(this,&ThisClass::HandleQuestionChanged);QuizGameMode->OnAnswerResolved.AddUObject(this,&ThisClass::HandleAnswerResolved);QuizGameMode->OnQuizFinished.AddUObject(this,&ThisClass::HandleQuizFinished);if(const FIdolQuizQuestion* Question=QuizGameMode->GetCurrentQuestion())HandleQuestionChanged(*Question,1,10);}
void UIdolQuizWidget::NativeDestruct(){if(QuizGameMode){QuizGameMode->OnQuestionChanged.RemoveAll(this);QuizGameMode->OnAnswerResolved.RemoveAll(this);QuizGameMode->OnQuizFinished.RemoveAll(this);}Super::NativeDestruct();}
void UIdolQuizWidget::HandleQuestionChanged(const FIdolQuizQuestion& Question,int32 Round,int32 Total){FaceImage->SetBrushFromTexture(Question.Image,true);FaceImage->SetVisibility(ESlateVisibility::Visible);RoundText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"),Round,Total)));FeedbackText->SetText(FText::GetEmpty());AnswerInput->SetText(FText::GetEmpty());AnswerInput->SetKeyboardFocus();}
void UIdolQuizWidget::HandleAnswerResolved(bool bCorrect,const FString& Answer,int32 Score){if(bCorrect){FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(.3f,1,.5f)));FeedbackText->SetText(FText::FromString(FString::Printf(TEXT("정답! %s"),*Answer)));ScoreText->SetText(FText::FromString(FString::Printf(TEXT("SCORE %d"),Score)));}else{ChatLines.Add(FString::Printf(TEXT("나: %s"),*Answer));if(ChatLines.Num()>12)ChatLines.RemoveAt(0);ChatText->SetText(FText::FromString(FString::Join(ChatLines,TEXT("\n"))));}AnswerInput->SetText(FText::GetEmpty());AnswerInput->SetKeyboardFocus();}
void UIdolQuizWidget::HandleQuizFinished(int32 Score,int32 Total){FaceImage->SetVisibility(ESlateVisibility::Collapsed);FeedbackText->SetText(FText::FromString(FString::Printf(TEXT("게임 종료! %d점 / %d문제"),Score,Total)));AnswerInput->SetVisibility(ESlateVisibility::Collapsed);SubmitButton->SetVisibility(ESlateVisibility::Collapsed);RestartButton->SetVisibility(ESlateVisibility::Visible);}
void UIdolQuizWidget::HandleSubmit(){if(QuizGameMode&&AnswerInput)QuizGameMode->SubmitAnswer(AnswerInput->GetText().ToString());}
void UIdolQuizWidget::HandleTextCommitted(const FText&,ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)HandleSubmit();}
void UIdolQuizWidget::HandleRestart(){UGameplayStatics::OpenLevel(this,TEXT("IdolQuizMap"));}
void UIdolQuizWidget::HandleMainHub(){UGameplayStatics::OpenLevel(this,TEXT("MainHubMap"));}
