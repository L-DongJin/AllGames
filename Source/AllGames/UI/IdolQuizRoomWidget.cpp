#include "IdolQuizRoomWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

namespace
{
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

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BG"));
	Background->SetColorAndOpacity(FLinearColor(.005f, .012f, .035f, 1));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BackgroundSlot->SetOffsets(FMargin(0));
	Add(MakeText(WidgetTree, TEXT("Title"), TEXT("온라인 방 목록"), 44), .5f, .07f, {700, 70});

	UButton* Create = MakeButton(WidgetTree, TEXT("OpenCreate"), TEXT("방 만들기"), 22);
	Create->OnClicked.AddDynamic(this, &ThisClass::OpenCreateDialog);
	Add(Create, .38f, .17f, {230, 58});
	UButton* Refresh = MakeButton(WidgetTree, TEXT("Refresh"), TEXT("새로고침"), 22);
	Refresh->OnClicked.AddDynamic(this, &ThisClass::HandleRefresh);
	Add(Refresh, .62f, .17f, {230, 58});

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UButton* Button = MakeButton(WidgetTree, *FString::Printf(TEXT("Room%d"), Index), TEXT(""), 19);
		UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0));
		Button->SetVisibility(ESlateVisibility::Collapsed);
		Add(Button, .5f, .28f + Index * .085f, {980, 62});
		RoomButtons.Add(Button);
		RoomLabels.Add(Label);
	}
	RoomButtons[0]->OnClicked.AddDynamic(this, &ThisClass::Join0);
	RoomButtons[1]->OnClicked.AddDynamic(this, &ThisClass::Join1);
	RoomButtons[2]->OnClicked.AddDynamic(this, &ThisClass::Join2);
	RoomButtons[3]->OnClicked.AddDynamic(this, &ThisClass::Join3);
	RoomButtons[4]->OnClicked.AddDynamic(this, &ThisClass::Join4);
	RoomButtons[5]->OnClicked.AddDynamic(this, &ThisClass::Join5);

	StatusText = MakeText(WidgetTree, TEXT("Status"), TEXT("방을 찾는 중..."), 18);
	Add(StatusText, .5f, .84f, {900, 45});
	UButton* Back = MakeButton(WidgetTree, TEXT("Back"), TEXT("ALL GAMES"), 20);
	Back->OnClicked.AddDynamic(this, &ThisClass::HandleMainHub);
	Add(Back, .1f, .94f, {190, 52});

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
	AddDialog(MakeText(WidgetTree, TEXT("CategoryLabel"), TEXT("출제 범위"), 19), 45, 175, {120, 45});
	CategoryDropdown = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CategoryDropdown"));
	CategoryDropdown->AddOption(TEXT("아이돌"));
	CategoryDropdown->AddOption(TEXT("배우"));
	CategoryDropdown->AddOption(TEXT("아이돌 + 배우"));
	CategoryDropdown->SetSelectedIndex(0);
	CategoryDropdown->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleCategoryChanged);
	AddDialog(CategoryDropdown, 175, 175, {420, 48});
	AddDialog(MakeText(WidgetTree, TEXT("QuestionCountLabel"), TEXT("문제 수"), 19), 45, 245, {120, 45});
	QuestionCountDropdown = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("QuestionCountDropdown"));
	for (int32 Count = 50; Count <= 1000; Count += 50)
	{
		QuestionCountDropdown->AddOption(FString::Printf(TEXT("%d문제"), Count));
	}
	QuestionCountDropdown->SetSelectedIndex(0);
	AddDialog(QuestionCountDropdown, 175, 245, {420, 48});
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
}

void UIdolQuizRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UIdolQuizSessionSubsystem>() : nullptr;
	if (Sessions)
	{
		Sessions->OnRoomsFound.AddUObject(this, &ThisClass::HandleRoomsFound);
		Sessions->OnSessionAction.AddUObject(this, &ThisClass::HandleSessionAction);
		Sessions->FindRooms();
	}
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
	SelectedCategory = EIdolQuizRoomCategory::Idol;
	DialogErrorText->SetText(FText::GetEmpty());
	CreateDialog->SetVisibility(ESlateVisibility::Visible);
	RoomNameInput->SetKeyboardFocus();
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
	if (Sessions)
	{
		CloseCreateDialog();
		StatusText->SetText(FText::FromString(TEXT("방을 만드는 중...")));
		Sessions->CreateRoom(Name, SelectedCategory, QuestionCount);
	}
}

void UIdolQuizRoomWidget::HandleRefresh()
{
	if (Sessions)
	{
		StatusText->SetText(FText::FromString(TEXT("방을 빠르게 검색 중...")));
		Sessions->FindRooms();
	}
}

void UIdolQuizRoomWidget::HandleRoomsFound(const bool bSuccess, const TArray<FIdolQuizRoomInfo>& Rooms)
{
	for (UButton* Button : RoomButtons)
	{
		Button->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 Index = 0; Index < FMath::Min(Rooms.Num(), RoomButtons.Num()); ++Index)
	{
		RoomLabels[Index]->SetText(FText::FromString(FString::Printf(
			TEXT("%s    [%s / %d문제]    %d / %d    %d ms"), *Rooms[Index].RoomName,
			*UIdolQuizSessionSubsystem::GetCategoryLabel(Rooms[Index].Category), Rooms[Index].QuestionCount,
			Rooms[Index].CurrentPlayers, Rooms[Index].MaxPlayers, Rooms[Index].PingMs)));
		RoomButtons[Index]->SetVisibility(ESlateVisibility::Visible);
	}
	StatusText->SetText(FText::FromString(!bSuccess ? TEXT("방 검색에 실패했습니다.")
		: Rooms.IsEmpty() ? TEXT("참가 가능한 방이 없습니다.")
		: FString::Printf(TEXT("%d개의 방을 찾았습니다."), Rooms.Num())));
}

void UIdolQuizRoomWidget::HandleSessionAction(const bool bSuccess, const FString& Message)
{
	StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(.3f, 1, .5f) : FLinearColor(1, .3f, .3f)));
	StatusText->SetText(FText::FromString(Message));
}

void UIdolQuizRoomWidget::JoinIndex(const int32 Index) { if (Sessions) Sessions->JoinRoom(Index); }
void UIdolQuizRoomWidget::Join0() { JoinIndex(0); }
void UIdolQuizRoomWidget::Join1() { JoinIndex(1); }
void UIdolQuizRoomWidget::Join2() { JoinIndex(2); }
void UIdolQuizRoomWidget::Join3() { JoinIndex(3); }
void UIdolQuizRoomWidget::Join4() { JoinIndex(4); }
void UIdolQuizRoomWidget::Join5() { JoinIndex(5); }
void UIdolQuizRoomWidget::HandleMainHub() { UGameplayStatics::OpenLevel(this, TEXT("MainHubMap")); }
