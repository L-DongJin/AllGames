#include "DrawingQuizWidget.h"
#include "DrawingCanvasWidget.h"
#include "../Audio/UiSoundStyle.h"
#include "../DrawingQuiz/DrawingQuizGameState.h"
#include "../DrawingQuiz/DrawingQuizPlayerController.h"
#include "../DrawingQuiz/DrawingQuizPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "QuizPlayerColors.h"

namespace
{
	UTextBlock* DText(UWidgetTree* T,const TCHAR* N,const FString& V,int32 S)
	{
		UTextBlock* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),N);
		W->SetText(FText::FromString(V));
		W->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo F=W->GetFont();
		F.Size=S;
		W->SetFont(F);
		return W;
	}

	UButton* DButton(UWidgetTree* T,const TCHAR* N,const FString& V)
	{
		UButton* B=T->ConstructWidget<UButton>(UButton::StaticClass(),N);
		UTextBlock* Label=DText(T,*FString(N).Append(TEXT("Text")),V,18);
		Label->SetJustification(ETextJustify::Center);
		B->AddChild(Label);
		return B;
	}

	UButton* ColorButton(UWidgetTree* T,const TCHAR* N,const FLinearColor& Color)
	{
		UButton* Button=T->ConstructWidget<UButton>(UButton::StaticClass(),N);
		Button->SetBackgroundColor(FLinearColor::White);
		UImage* Swatch=T->ConstructWidget<UImage>(UImage::StaticClass(),*FString(N).Append(TEXT("Swatch")));
		Swatch->SetColorAndOpacity(Color);
		Swatch->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(Swatch);
		return Button;
	}
}
UDrawingQuizWidget::UDrawingQuizWidget(const FObjectInitializer& O):Super(O){SetIsFocusable(true);}
TSharedRef<SWidget> UDrawingQuizWidget::RebuildWidget(){if(WidgetTree&&!WidgetTree->RootWidget)BuildLayout();return Super::RebuildWidget();}
void UDrawingQuizWidget::BuildLayout()
{
	UCanvasPanel* Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("DrawingQuizRoot"));
	WidgetTree->RootWidget=Root;
	auto Add=[Root](UWidget* W,float X,float Y,FVector2D S,int32 Z=0)
	{
		UCanvasPanelSlot* P=Root->AddChildToCanvas(W);
		P->SetAnchors(FAnchors(X,Y));
		P->SetAlignment(FVector2D(.5f,.5f));
		P->SetSize(S);
		P->SetZOrder(Z);
	};

	UImage* BG=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Background"));
	BG->SetColorAndOpacity(FLinearColor(.008f,.015f,.04f,1));
	BG->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BGS=Root->AddChildToCanvas(BG);
	BGS->SetAnchors(FAnchors(0,0,1,1));
	BGS->SetOffsets(FMargin(0));

	RoundText=DText(WidgetTree,TEXT("Round"),TEXT("ROUND 0 / 0"),24);
	Add(RoundText,.12f,.05f,{300,45});
	TimerText=DText(WidgetTree,TEXT("Timer"),TEXT("TIME 60"),32);
	TimerText->SetJustification(ETextJustify::Center);
	Add(TimerText,.5f,.05f,{260,55});
	DrawerText=DText(WidgetTree,TEXT("Drawer"),TEXT("출제자: -"),23);
	DrawerText->SetJustification(ETextJustify::Right);
	Add(DrawerText,.84f,.05f,{430,45});
	WordText=DText(WidgetTree,TEXT("Word"),TEXT("제시어: ○○"),28);
	WordText->SetJustification(ETextJustify::Center);
	WordText->SetColorAndOpacity(FSlateColor(FLinearColor(1,.85f,.25f)));
	Add(WordText,.5f,.105f,{700,50});

	Canvas=WidgetTree->ConstructWidget<UDrawingCanvasWidget>(UDrawingCanvasWidget::StaticClass(),TEXT("DrawingCanvas"));
	Add(Canvas,.38f,.49f,{1120,690},1);

	PlayerText=DText(WidgetTree,TEXT("Players"),TEXT(""),19);
	Add(PlayerText,.84f,.28f,{430,300});
	ChatContainer=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("ChatContainer"));
	Add(ChatContainer,.84f,.58f,{430,300});
	FeedbackText=DText(WidgetTree,TEXT("Feedback"),TEXT(""),24);
	FeedbackText->SetJustification(ETextJustify::Center);
	FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(.3f,1,.55f)));
	FeedbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	Add(FeedbackText,.5f,.87f,{900,55},2);

	ChatInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("ChatInput"));
	ChatInput->SetHintText(FText::FromString(TEXT("채팅 또는 정답 입력")));
	FEditableTextBoxStyle IS=ChatInput->GetWidgetStyle();
	IS.SetForegroundColor(FSlateColor(FLinearColor::Black)).SetFocusedForegroundColor(FSlateColor(FLinearColor::Black));
	ChatInput->SetWidgetStyle(IS);
	ChatInput->OnTextCommitted.AddDynamic(this,&ThisClass::CommitChat);
	Add(ChatInput,.82f,.82f,{360,52},3);
	UButton* Send=DButton(WidgetTree,TEXT("Send"),TEXT("전송"));
	Send->OnClicked.AddDynamic(this,&ThisClass::SubmitChat);
	Add(Send,.95f,.82f,{110,52},3);

	UButton* Black=ColorButton(WidgetTree,TEXT("Black"),FLinearColor::Black);
	Black->OnClicked.AddDynamic(this,&ThisClass::UseBlack);
	Add(Black,.055f,.25f,{52,52},3);
	UButton* Red=ColorButton(WidgetTree,TEXT("Red"),FLinearColor::Red);
	Red->OnClicked.AddDynamic(this,&ThisClass::UseRed);
	Add(Red,.055f,.32f,{52,52},3);
	UButton* Blue=ColorButton(WidgetTree,TEXT("Blue"),FLinearColor::Blue);
	Blue->OnClicked.AddDynamic(this,&ThisClass::UseBlue);
	Add(Blue,.055f,.39f,{52,52},3);
	UButton* Orange=ColorButton(WidgetTree,TEXT("Orange"),FLinearColor(1.f,.32f,.02f,1.f));
	Orange->OnClicked.AddDynamic(this,&ThisClass::UseOrange);
	Add(Orange,.055f,.46f,{52,52},3);
	UButton* Yellow=ColorButton(WidgetTree,TEXT("Yellow"),FLinearColor::Yellow);
	Yellow->OnClicked.AddDynamic(this,&ThisClass::UseYellow);
	Add(Yellow,.055f,.53f,{52,52},3);
	UButton* Green=ColorButton(WidgetTree,TEXT("Green"),FLinearColor::Green);
	Green->OnClicked.AddDynamic(this,&ThisClass::UseGreen);
	Add(Green,.055f,.60f,{52,52},3);
	UButton* Purple=ColorButton(WidgetTree,TEXT("Purple"),FLinearColor(.55f,.12f,.85f,1.f));
	Purple->OnClicked.AddDynamic(this,&ThisClass::UsePurple);
	Add(Purple,.055f,.67f,{52,52},3);

	UButton* Eraser=DButton(WidgetTree,TEXT("Eraser"),TEXT("지우개"));
	Eraser->OnClicked.AddDynamic(this,&ThisClass::UseEraser);
	Add(Eraser,.31f,.85f,{150,56},3);
	UButton* Clear=DButton(WidgetTree,TEXT("Clear"),TEXT("전체 삭제"));
	Clear->OnClicked.AddDynamic(this,&ThisClass::ClearCanvas);
	Add(Clear,.43f,.85f,{170,56},3);
	UButton* Hub=DButton(WidgetTree,TEXT("Hub"),TEXT("ALL GAMES"));
	Hub->OnClicked.AddDynamic(this,&ThisClass::ReturnHub);
	Add(Hub,.92f,.94f,{160,48},3);
}
void UDrawingQuizWidget::NativeConstruct(){Super::NativeConstruct();BindState();AllGamesUiSound::ApplyButtonClickSound(WidgetTree);}
void UDrawingQuizWidget::NativeDestruct(){if(QuizState){QuizState->OnStateChanged.RemoveAll(this);QuizState->OnChat.RemoveAll(this);}Super::NativeDestruct();}
void UDrawingQuizWidget::NativeTick(const FGeometry& G,float D){Super::NativeTick(G,D);if(!QuizState)BindState();RefreshPlayers();const double Now=FPlatformTime::Seconds();for(int32 Index=ChatMessages.Num()-1;Index>=0;--Index)if(ChatMessages[Index].ExpiresAtSeconds<=Now){if(ChatMessages[Index].Row)ChatMessages[Index].Row->RemoveFromParent();ChatMessages.RemoveAt(Index);}}
FReply UDrawingQuizWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
	if(KeyEvent.GetKey()==EKeys::Enter&&ChatInput&&!ChatInput->HasKeyboardFocus())
	{
		ChatInput->SetKeyboardFocus();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry,KeyEvent);
}
void UDrawingQuizWidget::BindState(){ADrawingQuizGameState* State=GetWorld()?GetWorld()->GetGameState<ADrawingQuizGameState>():nullptr;if(!State||State==QuizState)return;QuizState=State;QuizState->OnStateChanged.AddUObject(this,&ThisClass::RefreshState);QuizState->OnChat.AddUObject(this,&ThisClass::ReceiveChat);RefreshState();}
void UDrawingQuizWidget::RefreshState(){if(!QuizState)return;RoundText->SetText(FText::FromString(FString::Printf(TEXT("ROUND %d / %d"),QuizState->GetRound(),QuizState->GetTotalRounds())));TimerText->SetText(FText::FromString(FString::Printf(TEXT("TIME %02d"),QuizState->GetRemainingTime())));TimerText->SetColorAndOpacity(FSlateColor(QuizState->GetRemainingTime()<=10?FLinearColor(1,.2f,.2f):FLinearColor(.2f,.9f,1)));ADrawingQuizPlayerState* Drawer=QuizState->GetDrawer();DrawerText->SetText(FText::FromString(FString::Printf(TEXT("출제자: %s"),Drawer?*Drawer->GetQuizName():TEXT("-"))));const bool bAmDrawer=GetOwningPlayer()&&GetOwningPlayer()->PlayerState==Drawer;WordText->SetText(FText::FromString(FString::Printf(TEXT("제시어: %s"),bAmDrawer&&!SecretWord.IsEmpty()?*SecretWord:*QuizState->GetHint())));FeedbackText->SetText(FText::FromString(QuizState->GetFeedback()));}
void UDrawingQuizWidget::SetSecretWord(const FString& Word){SecretWord=Word;RefreshState();}
void UDrawingQuizWidget::ReceiveChat(const FString& Name,const FString& Message,const int32 PlayerColorIndex){AddChatMessage(Name,Message,PlayerColorIndex);}
void UDrawingQuizWidget::AddChatMessage(const FString& Name,const FString& Message,const int32 PlayerColorIndex){if(Message.IsEmpty()||!ChatContainer)return;const int32 MessageId=ChatMessages.Num();UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),*FString::Printf(TEXT("DrawingChatRow%d"),MessageId));UTextBlock* NameText=DText(WidgetTree,*FString::Printf(TEXT("DrawingChatName%d"),MessageId),FString::Printf(TEXT("%s: "),*Name),18);NameText->SetColorAndOpacity(FSlateColor(QuizPlayerColors::Get(PlayerColorIndex)));if(UHorizontalBoxSlot* NameSlot=Row->AddChildToHorizontalBox(NameText))NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));UTextBlock* MessageText=DText(WidgetTree,*FString::Printf(TEXT("DrawingChatMessage%d"),MessageId),Message,18);MessageText->SetAutoWrapText(true);if(UHorizontalBoxSlot* MessageSlot=Row->AddChildToHorizontalBox(MessageText))MessageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));if(UVerticalBoxSlot* RowSlot=ChatContainer->AddChildToVerticalBox(Row))RowSlot->SetPadding(FMargin(0,2));ChatMessages.Add({Row,FPlatformTime::Seconds()+6.0});constexpr int32 MaxVisibleMessages=10;while(ChatMessages.Num()>MaxVisibleMessages){if(ChatMessages[0].Row)ChatMessages[0].Row->RemoveFromParent();ChatMessages.RemoveAt(0);}}
void UDrawingQuizWidget::RefreshPlayers(){if(!QuizState)return;TArray<FString> Lines;for(APlayerState* PS:QuizState->PlayerArray)if(const ADrawingQuizPlayerState* D=Cast<ADrawingQuizPlayerState>(PS))Lines.Add(FString::Printf(TEXT("%s    %d점"),*D->GetQuizName(),D->GetQuizScore()));PlayerText->SetText(FText::FromString(FString::Join(Lines,TEXT("\n"))));}
void UDrawingQuizWidget::SubmitChat(){const FString M=ChatInput?ChatInput->GetText().ToString().TrimStartAndEnd():FString();if(!M.IsEmpty())if(ADrawingQuizPlayerController* PC=GetOwningPlayer<ADrawingQuizPlayerController>())PC->ServerSubmitChat(M);if(ChatInput){ChatInput->SetText(FText::GetEmpty());ChatInput->SetKeyboardFocus();}}
void UDrawingQuizWidget::CommitChat(const FText&,const ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)SubmitChat();}
void UDrawingQuizWidget::UseBlack(){if(Canvas)Canvas->SetBrushColor(FColor::Black);}
void UDrawingQuizWidget::UseRed(){if(Canvas)Canvas->SetBrushColor(FColor::Red);}
void UDrawingQuizWidget::UseBlue(){if(Canvas)Canvas->SetBrushColor(FColor::Blue);}
void UDrawingQuizWidget::UseOrange(){if(Canvas)Canvas->SetBrushColor(FColor(255,82,5));}
void UDrawingQuizWidget::UseYellow(){if(Canvas)Canvas->SetBrushColor(FColor::Yellow);}
void UDrawingQuizWidget::UseGreen(){if(Canvas)Canvas->SetBrushColor(FColor::Green);}
void UDrawingQuizWidget::UsePurple(){if(Canvas)Canvas->SetBrushColor(FColor(140,31,217));}
void UDrawingQuizWidget::UseEraser(){if(Canvas)Canvas->SetEraser(true);}
void UDrawingQuizWidget::ClearCanvas(){if(Canvas)Canvas->RequestClear();}
void UDrawingQuizWidget::ReturnHub(){UGameplayStatics::OpenLevel(this,TEXT("MainHubMap"));}
