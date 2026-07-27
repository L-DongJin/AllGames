// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLoginWidget.h"
#include "../Audio/UiSoundStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "../Core/RhythmAccountSubsystem.h"

namespace
{
	void ApplyButtonArtwork(UButton* Button,UTexture2D* Normal,UTexture2D* Hovered,UTexture2D* Pressed,const FVector2D Size)
	{
		if(!Button)return;
		if(Normal||Hovered||Pressed){UTexture2D* Base=Normal?Normal:(Hovered?Hovered:Pressed);FButtonStyle Style=Button->GetStyle();auto Brush=[Size](UTexture2D* Texture){FSlateBrush Result;Result.SetResourceObject(Texture);Result.DrawAs=ESlateBrushDrawType::Box;Result.Margin=FMargin(.08f);Result.ImageSize=Size;Result.TintColor=FSlateColor(FLinearColor::White);return Result;};const FSlateBrush N=Brush(Base);Style.SetNormal(N).SetHovered(Brush(Hovered?Hovered:Base)).SetPressed(Brush(Pressed?Pressed:Base)).SetDisabled(N);Button->SetStyle(Style);Button->SetBackgroundColor(FLinearColor::White);}
		if(UCanvasPanelSlot* Slot=Cast<UCanvasPanelSlot>(Button->Slot))if(Size.X>0&&Size.Y>0)Slot->SetSize(Size);
	}
	void ApplyBlackInputText(UEditableTextBox* Input)
	{
		if (!Input) return;
		const FSlateColor Black(FLinearColor::Black);
		FEditableTextBoxStyle Style = Input->GetWidgetStyle();
		Style.SetForegroundColor(Black).SetFocusedForegroundColor(Black).SetReadOnlyForegroundColor(Black);
		FTextBlockStyle TextStyle = Style.TextStyle;
		TextStyle.SetColorAndOpacity(Black);
		Style.SetTextStyle(TextStyle);
		Input->SetWidgetStyle(Style);
	}
	UTextBlock* MakeLoginText(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, const int32 Size)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value));
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		return Text;
	}

	void AddCentered(UCanvasPanel* Panel, UWidget* Widget, const float Y, const FVector2D Size)
	{
		UCanvasPanelSlot* WidgetSlot = Panel->AddChildToCanvas(Widget);
		WidgetSlot->SetAnchors(FAnchors(0.5f, Y));
		WidgetSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		WidgetSlot->SetSize(Size);
	}
}

URhythmLoginWidget::URhythmLoginWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> URhythmLoginWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void URhythmLoginWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!AccountBackground && WidgetTree)
	{
		AccountBackground = Cast<UImage>(WidgetTree->FindWidget(TEXT("AccountBackground")));
	}
	if (AccountBackground && LoginBackgroundImage)
	{
		AccountBackground->SetBrushFromTexture(LoginBackgroundImage, true);
		AccountBackground->SetColorAndOpacity(LoginBackgroundTint);
	}
	if(WidgetTree){if(!LoginButton)LoginButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("LoginButton")));if(!RegisterButton)RegisterButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("OpenRegistrationButton")));if(!CreateAccountButton)CreateAccountButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("CreateAccountButton")));if(!RegistrationBackButton)RegistrationBackButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("RegistrationBackButton")));}
	ApplyButtonArtwork(LoginButton,LoginButtonNormalImage,LoginButtonHoveredImage,LoginButtonPressedImage,LoginButtonSize);
	ApplyButtonArtwork(RegisterButton,RegisterButtonNormalImage,RegisterButtonHoveredImage,RegisterButtonPressedImage,RegisterButtonSize);
	ApplyButtonArtwork(CreateAccountButton,CreateAccountButtonNormalImage,CreateAccountButtonHoveredImage,CreateAccountButtonPressedImage,CreateAccountButtonSize);
	ApplyButtonArtwork(RegistrationBackButton,RegistrationBackButtonNormalImage,RegistrationBackButtonHoveredImage,RegistrationBackButtonPressedImage,RegistrationBackButtonSize);
	if(RegistrationBackButton)RegistrationBackButton->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleRegistrationBackClicked);
	if (PasswordInput)
	{
		PasswordInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::HandleLoginPasswordCommitted);
		PasswordInput->OnTextCommitted.AddDynamic(this, &ThisClass::HandleLoginPasswordCommitted);
	}
	if (URhythmAccountSubsystem* Accounts = GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>())
	{
		Accounts->OnAuthenticationCompleted.AddUObject(
			this, &ThisClass::HandleAuthenticationCompleted);
	}
	ShowRegistrationPanel(false);
	AllGamesUiSound::ApplyButtonClickSound(WidgetTree);
}

void URhythmLoginWidget::NativeDestruct()
{
	if (URhythmAccountSubsystem* Accounts = GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>())
	{
		Accounts->OnAuthenticationCompleted.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void URhythmLoginWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AccountRoot"));
	WidgetTree->RootWidget = Root;

	AccountBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AccountBackground"));
	if (LoginBackgroundImage)
	{
		AccountBackground->SetBrushFromTexture(LoginBackgroundImage, true);
		AccountBackground->SetColorAndOpacity(LoginBackgroundTint);
	}
	else AccountBackground->SetColorAndOpacity(FLinearColor(0.005f, 0.01f, 0.035f, 1.0f));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(AccountBackground);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	LoginPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LoginPanel"));
	UCanvasPanelSlot* LoginPanelSlot = Root->AddChildToCanvas(LoginPanel);
	LoginPanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	LoginPanelSlot->SetOffsets(FMargin(0.0f));

	AddCentered(LoginPanel, MakeLoginText(WidgetTree, TEXT("LoginTitle"), TEXT("로그인"), 64),
		0.22f, FVector2D(900.0f, 100.0f));
	AddCentered(LoginPanel, MakeLoginText(WidgetTree, TEXT("LoginSubtitle"),
		TEXT("계정에 로그인하면 다른 PC에서도 기록을 이어서 플레이할 수 있습니다."), 24),
		0.30f, FVector2D(1200.0f, 55.0f));

	UsernameInput = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(), TEXT("LoginUsernameInput"));
	UsernameInput->SetHintText(FText::FromString(TEXT("아이디를 입력하세요")));
	ApplyBlackInputText(UsernameInput);
	AddCentered(LoginPanel, UsernameInput, 0.42f, FVector2D(520.0f, 64.0f));

	PasswordInput = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(), TEXT("LoginPasswordInput"));
	PasswordInput->SetHintText(FText::FromString(TEXT("비밀번호를 입력하세요")));
	PasswordInput->SetIsPassword(true);
	ApplyBlackInputText(PasswordInput);
	AddCentered(LoginPanel, PasswordInput, 0.51f, FVector2D(520.0f, 64.0f));

	auto MakeButton = [this](UCanvasPanel* Panel, const TCHAR* Name, const TCHAR* Label,
		const float X, const float Y, const FVector2D Size)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->AddChild(MakeLoginText(
			WidgetTree, *FString::Printf(TEXT("%sText"), Name), Label, 27));
		UCanvasPanelSlot* ButtonSlot = Panel->AddChildToCanvas(Button);
		ButtonSlot->SetAnchors(FAnchors(X, Y));
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ButtonSlot->SetSize(Size);
		return Button;
	};

	LoginButton = MakeButton(LoginPanel, TEXT("LoginButton"), TEXT("로그인"),
		0.42f, 0.63f, FVector2D(250.0f, 72.0f));
	LoginButton->OnClicked.AddDynamic(this, &ThisClass::HandleLoginClicked);
	RegisterButton = MakeButton(LoginPanel, TEXT("OpenRegistrationButton"), TEXT("회원가입"),
		0.58f, 0.63f, FVector2D(250.0f, 72.0f));
	RegisterButton->OnClicked.AddDynamic(this, &ThisClass::HandleOpenRegistrationClicked);

	StatusText = MakeLoginText(WidgetTree, TEXT("LoginStatus"),
		TEXT("비밀번호는 이 PC에 저장하지 않습니다."), 21);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.7f, 0.85f)));
	AddCentered(LoginPanel, StatusText, 0.74f, FVector2D(1000.0f, 70.0f));

	RegistrationPanel = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RegistrationPanel"));
	UCanvasPanelSlot* RegistrationPanelSlot = Root->AddChildToCanvas(RegistrationPanel);
	RegistrationPanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	RegistrationPanelSlot->SetOffsets(FMargin(0.0f));

	AddCentered(RegistrationPanel, MakeLoginText(
		WidgetTree, TEXT("RegistrationTitle"), TEXT("회원가입"), 64),
		0.16f, FVector2D(900.0f, 100.0f));
	AddCentered(RegistrationPanel, MakeLoginText(
		WidgetTree, TEXT("RegistrationSubtitle"),
		TEXT("한 번 만든 계정은 다른 PC에서도 같은 아이디와 비밀번호로 사용할 수 있습니다."), 24),
		0.24f, FVector2D(1250.0f, 55.0f));

	RegistrationUsernameInput = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(), TEXT("RegistrationUsernameInput"));
	RegistrationUsernameInput->SetHintText(FText::FromString(TEXT("사용할 아이디 (3~20자)")));
	ApplyBlackInputText(RegistrationUsernameInput);
	AddCentered(RegistrationPanel, RegistrationUsernameInput, 0.36f, FVector2D(520.0f, 64.0f));

	RegistrationPasswordInput = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(), TEXT("RegistrationPasswordInput"));
	RegistrationPasswordInput->SetHintText(FText::FromString(TEXT("사용할 비밀번호 (6자 이상)")));
	RegistrationPasswordInput->SetIsPassword(true);
	ApplyBlackInputText(RegistrationPasswordInput);
	AddCentered(RegistrationPanel, RegistrationPasswordInput, 0.45f, FVector2D(520.0f, 64.0f));

	RegistrationPasswordConfirmInput = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(), TEXT("RegistrationPasswordConfirmInput"));
	RegistrationPasswordConfirmInput->SetHintText(FText::FromString(TEXT("비밀번호를 한 번 더 입력하세요")));
	RegistrationPasswordConfirmInput->SetIsPassword(true);
	ApplyBlackInputText(RegistrationPasswordConfirmInput);
	AddCentered(RegistrationPanel, RegistrationPasswordConfirmInput, 0.54f, FVector2D(520.0f, 64.0f));

	CreateAccountButton = MakeButton(RegistrationPanel, TEXT("CreateAccountButton"), TEXT("가입"),
		0.5f, 0.66f, FVector2D(300.0f, 72.0f));
	CreateAccountButton->OnClicked.AddDynamic(this, &ThisClass::HandleCreateAccountClicked);
	RegistrationBackButton = MakeButton(RegistrationPanel, TEXT("RegistrationBackButton"), TEXT("로그인으로 돌아가기"),
		0.5f, 0.88f, RegistrationBackButtonSize);
	RegistrationBackButton->OnClicked.AddDynamic(this, &ThisClass::HandleRegistrationBackClicked);

	RegistrationStatusText = MakeLoginText(WidgetTree, TEXT("RegistrationStatus"),
		TEXT("이메일은 현재 필요하지 않지만, 비밀번호 분실 시 계정 복구가 어렵습니다."), 20);
	RegistrationStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.3f)));
	AddCentered(RegistrationPanel, RegistrationStatusText, 0.78f, FVector2D(1200.0f, 80.0f));
}

void URhythmLoginWidget::HandleLoginClicked()
{
	if (URhythmAccountSubsystem* Accounts = GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>())
	{
		SetRequestState(true, TEXT("로그인 중입니다..."));
		Accounts->Login(UsernameInput->GetText().ToString(), PasswordInput->GetText().ToString());
	}
}

void URhythmLoginWidget::HandleLoginPasswordCommitted(const FText&, const ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter && !bRegistrationMode && LoginButton && LoginButton->GetIsEnabled())
	{
		HandleLoginClicked();
	}
}

void URhythmLoginWidget::HandleOpenRegistrationClicked()
{
	ShowRegistrationPanel(true);
}

void URhythmLoginWidget::HandleCreateAccountClicked()
{
	const FString Password = RegistrationPasswordInput->GetText().ToString();
	if (Password != RegistrationPasswordConfirmInput->GetText().ToString())
	{
		RegistrationStatusText->SetText(FText::FromString(TEXT("입력한 비밀번호가 서로 다릅니다.")));
		RegistrationStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.25f, 0.25f)));
		return;
	}

	if (URhythmAccountSubsystem* Accounts = GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>())
	{
		SetRequestState(true, TEXT("계정을 생성하고 있습니다..."));
		Accounts->Register(RegistrationUsernameInput->GetText().ToString(), Password);
	}
}

void URhythmLoginWidget::HandleRegistrationBackClicked()
{
	ShowRegistrationPanel(false);
}

void URhythmLoginWidget::HandleAuthenticationCompleted(const bool bSuccess, const FString& Message)
{
	SetRequestState(false, Message);
	if (bSuccess)
	{
		if (bRegistrationMode)
		{
			UsernameInput->SetText(RegistrationUsernameInput->GetText());
			PasswordInput->SetText(FText::GetEmpty());
			RegistrationPasswordInput->SetText(FText::GetEmpty());
			RegistrationPasswordConfirmInput->SetText(FText::GetEmpty());
			ShowRegistrationPanel(false);
			StatusText->SetText(FText::FromString(
				TEXT("회원가입이 완료되었습니다. 방금 만든 계정으로 로그인해 주세요.")));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 1.0f, 0.55f)));
		}
		else
		{
			PasswordInput->SetText(FText::GetEmpty());
			OnLoginAccepted.Broadcast();
		}
	}
}

void URhythmLoginWidget::SetRequestState(const bool bRequesting, const FString& Message)
{
	for (UButton* Button : { LoginButton.Get(), RegisterButton.Get(), CreateAccountButton.Get() })
	{
		if (Button) Button->SetIsEnabled(!bRequesting);
	}
	for (UEditableTextBox* Input : {
		UsernameInput.Get(), PasswordInput.Get(), RegistrationUsernameInput.Get(),
		RegistrationPasswordInput.Get(), RegistrationPasswordConfirmInput.Get() })
	{
		if (Input) Input->SetIsReadOnly(bRequesting);
	}

	UTextBlock* ActiveStatus = bRegistrationMode ? RegistrationStatusText.Get() : StatusText.Get();
	if (ActiveStatus)
	{
		ActiveStatus->SetText(FText::FromString(Message));
		ActiveStatus->SetColorAndOpacity(FSlateColor(
			bRequesting ? FLinearColor(0.3f, 0.85f, 1.0f) : FLinearColor::White));
	}
}

void URhythmLoginWidget::ShowRegistrationPanel(const bool bShowRegistration)
{
	bRegistrationMode = bShowRegistration;
	if (LoginPanel)
	{
		LoginPanel->SetVisibility(bShowRegistration ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (RegistrationPanel)
	{
		RegistrationPanel->SetVisibility(bShowRegistration ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (bShowRegistration && RegistrationUsernameInput)
	{
		RegistrationUsernameInput->SetKeyboardFocus();
	}
	else if (!bShowRegistration && UsernameInput)
	{
		UsernameInput->SetKeyboardFocus();
	}
}
