#include "IdolQuizRoomWidget.h"
#include "../Audio/UiSoundStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "RoomComboBoxString.h"

namespace
{
	void ApplyRoomButtonArtwork(UButton* Button,UTexture2D* Normal,UTexture2D* Hovered,UTexture2D* Pressed,const FVector2D Size)
	{
		if(!Button)return;if(Normal||Hovered||Pressed){UTexture2D* Base=Normal?Normal:(Hovered?Hovered:Pressed);auto Make=[Size](UTexture2D* T){FSlateBrush B;B.SetResourceObject(T);B.DrawAs=ESlateBrushDrawType::Box;B.Margin=FMargin(.08f);B.ImageSize=Size;B.TintColor=FSlateColor(FLinearColor::White);return B;};const FSlateBrush N=Make(Base);FButtonStyle S=Button->GetStyle();S.SetNormal(N).SetHovered(Make(Hovered?Hovered:Base)).SetPressed(Make(Pressed?Pressed:Base)).SetDisabled(N);Button->SetStyle(S);Button->SetBackgroundColor(FLinearColor::White);}if(UCanvasPanelSlot* Slot=Cast<UCanvasPanelSlot>(Button->Slot))if(Size.X>0&&Size.Y>0)Slot->SetSize(Size);
	}
	void ApplyRoomEntryColors(UButton* Button,const FLinearColor Normal,const FLinearColor Hovered,const FLinearColor Pressed)
	{
		if(!Button)return;FButtonStyle S=Button->GetStyle();FSlateBrush N=S.Normal;N.TintColor=FSlateColor(Normal);FSlateBrush H=S.Hovered;H.TintColor=FSlateColor(Hovered);FSlateBrush P=S.Pressed;P.TintColor=FSlateColor(Pressed);FSlateBrush D=N;D.TintColor=FSlateColor(Normal*.55f);S.SetNormal(N).SetHovered(H).SetPressed(P).SetDisabled(D);Button->SetStyle(S);
	}
	UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, const int32 Size)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value));
		Text->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		return Text;
	}

	UButton* MakeButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, const int32 Size)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->AddChild(MakeText(Tree, *FString(Name).Append(TEXT("Text")), Value, Size));
		return Button;
	}

	void SetBlackInputText(UEditableTextBox* Input)
	{
		const FSlateColor Black(FLinearColor::Black);
		FEditableTextBoxStyle Style = Input->GetWidgetStyle();
		Style.SetForegroundColor(Black).SetFocusedForegroundColor(Black).SetReadOnlyForegroundColor(Black);
		FTextBlockStyle TextStyle = Style.TextStyle;
		TextStyle.SetColorAndOpacity(Black);
		Style.SetTextStyle(TextStyle);
		Input->SetWidgetStyle(Style);
	}
}

UIdolQuizRoomWidget::UIdolQuizRoomWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UIdolQuizRoomWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UIdolQuizRoomWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RoomRoot"));
	WidgetTree->RootWidget = Root;
	auto Add = [Root](UWidget* Widget, float X, float Y, FVector2D Size)
	{
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(X, Y));
		Slot->SetAlignment(FVector2D(.5f, .5f));
		Slot->SetSize(Size);
	};

	RoomBrowserBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BG"));
	if(RoomBrowserBackgroundImage){RoomBrowserBackground->SetBrushFromTexture(RoomBrowserBackgroundImage,true);RoomBrowserBackground->SetColorAndOpacity(RoomBrowserBackgroundTint);}else RoomBrowserBackground->SetColorAndOpacity(FLinearColor(.005f, .012f, .035f, 1));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(RoomBrowserBackground);
	BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BackgroundSlot->SetOffsets(FMargin(0));
	Add(MakeText(WidgetTree, TEXT("Title"), TEXT("온라인 방 목록"), 44), .5f, .07f, {700, 70});

	CreateRoomButton = MakeButton(WidgetTree, TEXT("OpenCreate"), TEXT("방 만들기"), 22);
	CreateRoomButton->OnClicked.AddDynamic(this, &ThisClass::OpenCreateDialog);
	Add(CreateRoomButton, .38f, .17f, CreateButtonSize);
	RefreshRoomsButton = MakeButton(WidgetTree, TEXT("Refresh"), TEXT("새로고침"), 22);
	RefreshRoomsButton->OnClicked.AddDynamic(this, &ThisClass::HandleRefresh);
	Add(RefreshRoomsButton, .62f, .17f, RefreshButtonSize);

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UButton* Button = MakeButton(WidgetTree, *FString::Printf(TEXT("Room%d"), Index), TEXT(""), 19);
		UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0));
		Button->SetVisibility(ESlateVisibility::Collapsed);
		Add(Button, .5f, .28f + Index * .085f, {980, 62});
		RoomButtons.Add(Button);
		RoomLabels.Add(Label);
		ApplyRoomEntryColors(Button,RoomEntryNormalColor,RoomEntryHoveredColor,RoomEntryPressedColor);
	}
	RoomButtons[0]->OnClicked.AddDynamic(this, &ThisClass::Join0);
	RoomButtons[1]->OnClicked.AddDynamic(this, &ThisClass::Join1);
	RoomButtons[2]->OnClicked.AddDynamic(this, &ThisClass::Join2);
	RoomButtons[3]->OnClicked.AddDynamic(this, &ThisClass::Join3);
	RoomButtons[4]->OnClicked.AddDynamic(this, &ThisClass::Join4);
	RoomButtons[5]->OnClicked.AddDynamic(this, &ThisClass::Join5);

	StatusText = MakeText(WidgetTree, TEXT("Status"), TEXT("방을 찾는 중..."), 18);
	Add(StatusText, .5f, .84f, {900, 45});
	BackToHubButton = MakeButton(WidgetTree, TEXT("Back"), TEXT("ALL GAMES"), 20);
	BackToHubButton->OnClicked.AddDynamic(this, &ThisClass::HandleMainHub);
	Add(BackToHubButton, .1f, .94f, BackButtonSize);

	CreateDialog = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CreateRoomDialog"));
	Add(CreateDialog, .5f, .5f, {650, 510});
	auto AddDialog = [this](UWidget* Widget, float X, float Y, FVector2D Size)
	{
		UCanvasPanelSlot* Slot = CreateDialog->AddChildToCanvas(Widget);
		Slot->SetPosition({X, Y});
		Slot->SetSize(Size);
	};
	UImage* Panel = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DialogBackground"));
	Panel->SetColorAndOpacity(FLinearColor(.025f, .045f, .1f, .98f));
	AddDialog(Panel, 0, 0, {650, 510});
	AddDialog(MakeText(WidgetTree, TEXT("DialogTitle"), TEXT("방 만들기"), 32), 75, 28, {500, 55});
	AddDialog(MakeText(WidgetTree, TEXT("NameLabel"), TEXT("방 제목"), 19), 45, 105, {120, 45});
	RoomNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RoomNameInput"));
	RoomNameInput->SetHintText(FText::FromString(TEXT("방 제목을 입력하세요")));
	SetBlackInputText(RoomNameInput);
	AddDialog(RoomNameInput, 175, 105, {420, 48});
	AddDialog(MakeText(WidgetTree, TEXT("GameLabel"), TEXT("게임"), 19), 45, 165, {120, 45});
	GameDropdown = WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(), TEXT("GameDropdown"));
	GameDropdown->AddOption(TEXT("인물 퀴즈"));
	GameDropdown->AddOption(TEXT("그림 퀴즈"));
	GameDropdown->SetSelectedIndex(0);
	GameDropdown->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleGameChanged);
	AddDialog(GameDropdown, 175, 165, {420, 48});
	CategoryLabel = MakeText(WidgetTree, TEXT("CategoryLabel"), TEXT("출제 범위"), 19);
	AddDialog(CategoryLabel, 45, 225, {120, 45});
	CategoryDropdown = WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(), TEXT("CategoryDropdown"));
	CategoryDropdown->AddOption(TEXT("아이돌"));
	CategoryDropdown->AddOption(TEXT("배우"));
	CategoryDropdown->AddOption(TEXT("아이돌 + 배우"));
	CategoryDropdown->SetSelectedIndex(0);
	CategoryDropdown->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleCategoryChanged);
	AddDialog(CategoryDropdown, 175, 225, {420, 48});
	QuestionCountLabel = MakeText(WidgetTree, TEXT("QuestionCountLabel"), TEXT("문제 수"), 19);
	AddDialog(QuestionCountLabel, 45, 285, {120, 45});
	QuestionCountDropdown = WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(), TEXT("QuestionCountDropdown"));
	for (int32 Count = 50; Count <= 300; Count += 50)
	{
		QuestionCountDropdown->AddOption(FString::Printf(TEXT("%d문제"), Count));
	}
	QuestionCountDropdown->SetSelectedIndex(0);
	AddDialog(QuestionCountDropdown, 175, 285, {420, 48});
	DrawingRoundsLabel = MakeText(WidgetTree, TEXT("DrawingRoundsLabel"), TEXT("인당 라운드"), 19);
	AddDialog(DrawingRoundsLabel, 45, 225, {120, 45});
	DrawingRoundsDropdown = WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(), TEXT("DrawingRoundsDropdown"));
	for (int32 Count = 1; Count <= 5; ++Count) DrawingRoundsDropdown->AddOption(FString::Printf(TEXT("%d회"), Count));
	DrawingRoundsDropdown->SetSelectedIndex(1);
	AddDialog(DrawingRoundsDropdown, 175, 225, {420, 48});
	DrawingTimeLabel = MakeText(WidgetTree, TEXT("DrawingTimeLabel"), TEXT("제한시간"), 19);
	AddDialog(DrawingTimeLabel, 45, 285, {120, 45});
	DrawingTimeDropdown = WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(), TEXT("DrawingTimeDropdown"));
	for (int32 Seconds = 30; Seconds <= 120; Seconds += 15) DrawingTimeDropdown->AddOption(FString::Printf(TEXT("%d초"), Seconds));
	DrawingTimeDropdown->SetSelectedIndex(2);
	AddDialog(DrawingTimeDropdown, 175, 285, {420, 48});
	DialogErrorText = MakeText(WidgetTree, TEXT("DialogError"), TEXT(""), 17);
	DialogErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1, .3f, .3f)));
	AddDialog(DialogErrorText, 75, 305, {500, 38});
	UButton* Confirm = MakeButton(WidgetTree, TEXT("ConfirmCreate"), TEXT("만들기"), 21);
	Confirm->OnClicked.AddDynamic(this, &ThisClass::ConfirmCreateRoom);
	AddDialog(Confirm, 335, 390, {220, 58});
	UButton* Cancel = MakeButton(WidgetTree, TEXT("CancelCreate"), TEXT("취소"), 21);
	Cancel->OnClicked.AddDynamic(this, &ThisClass::CloseCreateDialog);
	AddDialog(Cancel, 95, 390, {200, 58});
	CreateDialog->SetVisibility(ESlateVisibility::Collapsed);
	RefreshGameSettingsVisibility();
}

void UIdolQuizRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if(WidgetTree){if(!RoomBrowserBackground)RoomBrowserBackground=Cast<UImage>(WidgetTree->FindWidget(TEXT("BG")));if(!CreateRoomButton)CreateRoomButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("OpenCreate")));if(!RefreshRoomsButton)RefreshRoomsButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("Refresh")));if(!BackToHubButton)BackToHubButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("Back")));if(!StatusText)StatusText=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Status")));if(!StatusText)StatusText=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StatusText")));if(RoomButtons.IsEmpty())for(int32 I=0;I<6;++I)if(UButton* B=Cast<UButton>(WidgetTree->FindWidget(*FString::Printf(TEXT("Room%d"),I)))){RoomButtons.Add(B);RoomLabels.Add(Cast<UTextBlock>(WidgetTree->FindWidget(*FString::Printf(TEXT("Room%dText"),I))));}if(!CreateDialog)CreateDialog=Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CreateRoomDialog")));if(!CreateDialog){UCanvasPanel* Root=Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RoomRoot")));if(!Root)Root=Cast<UCanvasPanel>(WidgetTree->RootWidget);BuildCreateDialog(Root);}}
	if(CreateRoomButton)CreateRoomButton->OnClicked.AddUniqueDynamic(this,&ThisClass::OpenCreateDialog);
	if(RefreshRoomsButton)RefreshRoomsButton->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleRefresh);
	if(BackToHubButton)BackToHubButton->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleMainHub);
	if(RoomButtons.IsValidIndex(0))RoomButtons[0]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join0);if(RoomButtons.IsValidIndex(1))RoomButtons[1]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join1);if(RoomButtons.IsValidIndex(2))RoomButtons[2]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join2);if(RoomButtons.IsValidIndex(3))RoomButtons[3]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join3);if(RoomButtons.IsValidIndex(4))RoomButtons[4]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join4);if(RoomButtons.IsValidIndex(5))RoomButtons[5]->OnClicked.AddUniqueDynamic(this,&ThisClass::Join5);
	VisibleRoomCount=0;for(UButton* Button:RoomButtons)if(Button){Button->SetVisibility(ESlateVisibility::Visible);Button->SetIsEnabled(false);}for(UTextBlock* Label:RoomLabels)if(Label)Label->SetText(FText::GetEmpty());
	if(RoomBrowserBackground&&RoomBrowserBackgroundImage){RoomBrowserBackground->SetBrushFromTexture(RoomBrowserBackgroundImage,true);RoomBrowserBackground->SetColorAndOpacity(RoomBrowserBackgroundTint);}
	ApplyRoomButtonArtwork(CreateRoomButton,CreateButtonNormalImage,CreateButtonHoveredImage,CreateButtonPressedImage,CreateButtonSize);
	ApplyRoomButtonArtwork(RefreshRoomsButton,RefreshButtonNormalImage,RefreshButtonHoveredImage,RefreshButtonPressedImage,RefreshButtonSize);
	ApplyRoomButtonArtwork(BackToHubButton,BackButtonNormalImage,BackButtonHoveredImage,BackButtonPressedImage,BackButtonSize);
	for(UButton* Button:RoomButtons)ApplyRoomEntryColors(Button,RoomEntryNormalColor,RoomEntryHoveredColor,RoomEntryPressedColor);
	Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UIdolQuizSessionSubsystem>() : nullptr;
	if (Sessions)
	{
		Sessions->OnRoomsFound.AddUObject(this, &ThisClass::HandleRoomsFound);
		Sessions->OnSessionAction.AddUObject(this, &ThisClass::HandleSessionAction);
		Sessions->FindRooms();
	}
	AllGamesUiSound::ApplyButtonClickSound(WidgetTree);
}

void UIdolQuizRoomWidget::BuildCreateDialog(UCanvasPanel* Root)
{
	if(!Root||CreateDialog)return;CreateDialog=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("CreateRoomDialog"));UCanvasPanelSlot* DialogSlot=Root->AddChildToCanvas(CreateDialog);DialogSlot->SetAnchors(FAnchors(.5f,.5f));DialogSlot->SetAlignment(FVector2D(.5f,.5f));DialogSlot->SetSize(FVector2D(650,510));auto Add=[this](UWidget* W,float X,float Y,FVector2D S){UCanvasPanelSlot* P=CreateDialog->AddChildToCanvas(W);P->SetPosition(FVector2D(X,Y));P->SetSize(S);};UImage* Panel=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("DialogBackground"));Panel->SetColorAndOpacity(FLinearColor(.025f,.045f,.1f,.98f));Add(Panel,0,0,{650,510});Add(MakeText(WidgetTree,TEXT("DialogTitle"),TEXT("방 만들기"),32),75,24,{500,55});Add(MakeText(WidgetTree,TEXT("NameLabel"),TEXT("방 제목"),19),45,90,{120,45});RoomNameInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("RoomNameInput"));RoomNameInput->SetHintText(FText::FromString(TEXT("방 제목을 입력하세요")));SetBlackInputText(RoomNameInput);Add(RoomNameInput,175,90,{420,48});Add(MakeText(WidgetTree,TEXT("GameLabel"),TEXT("게임"),19),45,150,{120,45});GameDropdown=WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(),TEXT("GameDropdown"));GameDropdown->AddOption(TEXT("인물 퀴즈"));GameDropdown->AddOption(TEXT("그림 퀴즈"));GameDropdown->SetSelectedIndex(0);GameDropdown->OnSelectionChanged.AddDynamic(this,&ThisClass::HandleGameChanged);Add(GameDropdown,175,150,{420,48});CategoryLabel=MakeText(WidgetTree,TEXT("CategoryLabel"),TEXT("출제 범위"),19);Add(CategoryLabel,45,210,{120,45});CategoryDropdown=WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(),TEXT("CategoryDropdown"));CategoryDropdown->AddOption(TEXT("아이돌"));CategoryDropdown->AddOption(TEXT("배우"));CategoryDropdown->AddOption(TEXT("아이돌 + 배우"));CategoryDropdown->SetSelectedIndex(0);CategoryDropdown->OnSelectionChanged.AddDynamic(this,&ThisClass::HandleCategoryChanged);Add(CategoryDropdown,175,210,{420,48});QuestionCountLabel=MakeText(WidgetTree,TEXT("QuestionCountLabel"),TEXT("문제 수"),19);Add(QuestionCountLabel,45,270,{120,45});QuestionCountDropdown=WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(),TEXT("QuestionCountDropdown"));for(int32 Count=50;Count<=300;Count+=50)QuestionCountDropdown->AddOption(FString::Printf(TEXT("%d문제"),Count));QuestionCountDropdown->SetSelectedIndex(0);Add(QuestionCountDropdown,175,270,{420,48});DrawingRoundsLabel=MakeText(WidgetTree,TEXT("DrawingRoundsLabel"),TEXT("인당 라운드"),19);Add(DrawingRoundsLabel,45,210,{120,45});DrawingRoundsDropdown=WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(),TEXT("DrawingRoundsDropdown"));for(int32 Count=1;Count<=5;++Count)DrawingRoundsDropdown->AddOption(FString::Printf(TEXT("%d회"),Count));DrawingRoundsDropdown->SetSelectedIndex(1);Add(DrawingRoundsDropdown,175,210,{420,48});DrawingTimeLabel=MakeText(WidgetTree,TEXT("DrawingTimeLabel"),TEXT("제한시간"),19);Add(DrawingTimeLabel,45,270,{120,45});DrawingTimeDropdown=WidgetTree->ConstructWidget<URoomComboBoxString>(URoomComboBoxString::StaticClass(),TEXT("DrawingTimeDropdown"));for(int32 Seconds=30;Seconds<=120;Seconds+=15)DrawingTimeDropdown->AddOption(FString::Printf(TEXT("%d초"),Seconds));DrawingTimeDropdown->SetSelectedIndex(2);Add(DrawingTimeDropdown,175,270,{420,48});DialogErrorText=MakeText(WidgetTree,TEXT("DialogError"),TEXT(""),17);DialogErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1,.3f,.3f)));Add(DialogErrorText,75,325,{500,38});UButton* Confirm=MakeButton(WidgetTree,TEXT("ConfirmCreate"),TEXT("만들기"),21);Confirm->OnClicked.AddDynamic(this,&ThisClass::ConfirmCreateRoom);Add(Confirm,335,400,{220,58});UButton* Cancel=MakeButton(WidgetTree,TEXT("CancelCreate"),TEXT("취소"),21);Cancel->OnClicked.AddDynamic(this,&ThisClass::CloseCreateDialog);Add(Cancel,95,400,{200,58});CreateDialog->SetVisibility(ESlateVisibility::Collapsed);RefreshGameSettingsVisibility();
}

void UIdolQuizRoomWidget::NativeDestruct()
{
	if (Sessions)
	{
		Sessions->OnRoomsFound.RemoveAll(this);
		Sessions->OnSessionAction.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UIdolQuizRoomWidget::OpenCreateDialog()
{
	RoomNameInput->SetText(FText::GetEmpty());
	CategoryDropdown->SetSelectedIndex(0);
	QuestionCountDropdown->SetSelectedIndex(0);
	GameDropdown->SetSelectedIndex(0);
	DrawingRoundsDropdown->SetSelectedIndex(1);
	DrawingTimeDropdown->SetSelectedIndex(2);
	SelectedGameType = EMiniGameRoomType::PersonQuiz;
	SelectedCategory = EIdolQuizRoomCategory::Idol;
	RefreshGameSettingsVisibility();
	DialogErrorText->SetText(FText::GetEmpty());
	CreateDialog->SetVisibility(ESlateVisibility::Visible);
	RoomNameInput->SetKeyboardFocus();
}

void UIdolQuizRoomWidget::HandleGameChanged(FString Item, ESelectInfo::Type)
{
	SelectedGameType = Item == TEXT("그림 퀴즈") ? EMiniGameRoomType::DrawingQuiz : EMiniGameRoomType::PersonQuiz;
	RefreshGameSettingsVisibility();
}

void UIdolQuizRoomWidget::RefreshGameSettingsVisibility()
{
	const ESlateVisibility PersonVisibility = SelectedGameType == EMiniGameRoomType::PersonQuiz ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	const ESlateVisibility DrawingVisibility = SelectedGameType == EMiniGameRoomType::DrawingQuiz ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (CategoryLabel) CategoryLabel->SetVisibility(PersonVisibility);
	if (CategoryDropdown) CategoryDropdown->SetVisibility(PersonVisibility);
	if (QuestionCountLabel) QuestionCountLabel->SetVisibility(PersonVisibility);
	if (QuestionCountDropdown) QuestionCountDropdown->SetVisibility(PersonVisibility);
	if (DrawingRoundsLabel) DrawingRoundsLabel->SetVisibility(DrawingVisibility);
	if (DrawingRoundsDropdown) DrawingRoundsDropdown->SetVisibility(DrawingVisibility);
	if (DrawingTimeLabel) DrawingTimeLabel->SetVisibility(DrawingVisibility);
	if (DrawingTimeDropdown) DrawingTimeDropdown->SetVisibility(DrawingVisibility);
}

void UIdolQuizRoomWidget::CloseCreateDialog()
{
	CreateDialog->SetVisibility(ESlateVisibility::Collapsed);
}

void UIdolQuizRoomWidget::HandleCategoryChanged(FString Item, ESelectInfo::Type)
{
	SelectedCategory = Item == TEXT("배우") ? EIdolQuizRoomCategory::Actor
		: Item == TEXT("아이돌 + 배우") ? EIdolQuizRoomCategory::IdolAndActor
		: EIdolQuizRoomCategory::Idol;
}

void UIdolQuizRoomWidget::ConfirmCreateRoom()
{
	const FString Name = RoomNameInput->GetText().ToString().TrimStartAndEnd();
	if (Name.IsEmpty())
	{
		DialogErrorText->SetText(FText::FromString(TEXT("방 제목을 입력해 주세요.")));
		return;
	}
	const int32 QuestionCount = (QuestionCountDropdown->GetSelectedIndex() + 1) * 50;
	const int32 DrawingRounds = DrawingRoundsDropdown->GetSelectedIndex() + 1;
	const int32 DrawingTime = 30 + DrawingTimeDropdown->GetSelectedIndex() * 15;
	if (Sessions)
	{
		CloseCreateDialog();
		if(StatusText)StatusText->SetText(FText::FromString(TEXT("방을 만드는 중...")));
		Sessions->CreateRoom(Name, SelectedGameType, SelectedCategory, QuestionCount, DrawingRounds, DrawingTime);
	}
}

void UIdolQuizRoomWidget::HandleRefresh()
{
	if (Sessions)
	{
		if(StatusText)StatusText->SetText(FText::FromString(TEXT("방을 빠르게 검색 중...")));
		Sessions->FindRooms();
	}
}

void UIdolQuizRoomWidget::HandleRoomsFound(const bool bSuccess, const TArray<FIdolQuizRoomInfo>& Rooms)
{
	VisibleRoomCount=0;
	for (UButton* Button : RoomButtons)
	{
		Button->SetVisibility(ESlateVisibility::Visible);
		Button->SetIsEnabled(false);
	}
	for(UTextBlock* Label:RoomLabels)if(Label)Label->SetText(FText::GetEmpty());
	for (int32 Index = 0; Index < FMath::Min(Rooms.Num(), RoomButtons.Num()); ++Index)
	{
		if(RoomLabels.IsValidIndex(Index)&&RoomLabels[Index])RoomLabels[Index]->SetText(FText::FromString(FString::Printf(
			TEXT("%s    [%s / %s]    %d / %d    %d ms"), *Rooms[Index].RoomName,
			*UIdolQuizSessionSubsystem::GetGameTypeLabel(Rooms[Index].GameType),
			Rooms[Index].GameType == EMiniGameRoomType::DrawingQuiz
				? *FString::Printf(TEXT("인당 %d회 · %d초"), Rooms[Index].DrawingRoundsPerPlayer, Rooms[Index].DrawingRoundTime)
				: *FString::Printf(TEXT("%s · %d문제"), *UIdolQuizSessionSubsystem::GetCategoryLabel(Rooms[Index].Category), Rooms[Index].QuestionCount),
			Rooms[Index].CurrentPlayers, Rooms[Index].MaxPlayers, Rooms[Index].PingMs)));
		RoomButtons[Index]->SetIsEnabled(true);
		++VisibleRoomCount;
	}
	if(StatusText)StatusText->SetText(FText::FromString(!bSuccess ? TEXT("방 검색에 실패했습니다.")
		: Rooms.IsEmpty() ? TEXT("참가 가능한 방이 없습니다.")
		: FString::Printf(TEXT("%d개의 방을 찾았습니다."), Rooms.Num())));
}

void UIdolQuizRoomWidget::HandleSessionAction(const bool bSuccess, const FString& Message)
{
	if(StatusText){StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(.3f, 1, .5f) : FLinearColor(1, .3f, .3f)));StatusText->SetText(FText::FromString(Message));}
}

void UIdolQuizRoomWidget::JoinIndex(const int32 Index) { if(Index<0||Index>=VisibleRoomCount)return;if(Sessions&&RoomButtons.IsValidIndex(Index)&&RoomButtons[Index]&&RoomButtons[Index]->GetIsEnabled())Sessions->JoinRoom(Index); }
void UIdolQuizRoomWidget::Join0() { JoinIndex(0); }
void UIdolQuizRoomWidget::Join1() { JoinIndex(1); }
void UIdolQuizRoomWidget::Join2() { JoinIndex(2); }
void UIdolQuizRoomWidget::Join3() { JoinIndex(3); }
void UIdolQuizRoomWidget::Join4() { JoinIndex(4); }
void UIdolQuizRoomWidget::Join5() { JoinIndex(5); }
void UIdolQuizRoomWidget::HandleMainHub() { UGameplayStatics::OpenLevel(this, TEXT("MainHubMap")); }
