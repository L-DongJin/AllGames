// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmLoginWidget.generated.h"

class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE(FOnRhythmLoginAccepted);

/** Minimal username/password entry screen displayed before the song lobby. */
UCLASS()
class ALLGAMES_API URhythmLoginWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmLoginWidget(const FObjectInitializer& ObjectInitializer);

	/** Full-screen image used behind both login and registration panels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Account|Appearance")
	TObjectPtr<UTexture2D> LoginBackgroundImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Account|Appearance")
	FLinearColor LoginBackgroundTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Login") TObjectPtr<UTexture2D> LoginButtonNormalImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Login") TObjectPtr<UTexture2D> LoginButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Login") TObjectPtr<UTexture2D> LoginButtonPressedImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Login",meta=(ClampMin="0.0")) FVector2D LoginButtonSize=FVector2D(250,72);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Open Registration") TObjectPtr<UTexture2D> RegisterButtonNormalImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Open Registration") TObjectPtr<UTexture2D> RegisterButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Open Registration") TObjectPtr<UTexture2D> RegisterButtonPressedImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Open Registration",meta=(ClampMin="0.0")) FVector2D RegisterButtonSize=FVector2D(250,72);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Create Account") TObjectPtr<UTexture2D> CreateAccountButtonNormalImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Create Account") TObjectPtr<UTexture2D> CreateAccountButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Create Account") TObjectPtr<UTexture2D> CreateAccountButtonPressedImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Create Account",meta=(ClampMin="0.0")) FVector2D CreateAccountButtonSize=FVector2D(300,72);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Registration Back") TObjectPtr<UTexture2D> RegistrationBackButtonNormalImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Registration Back") TObjectPtr<UTexture2D> RegistrationBackButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Registration Back") TObjectPtr<UTexture2D> RegistrationBackButtonPressedImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Account|Buttons|Registration Back",meta=(ClampMin="0.0")) FVector2D RegistrationBackButtonSize=FVector2D(260,64);

	FOnRhythmLoginAccepted OnLoginAccepted;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void HandleAuthenticationCompleted(bool bSuccess, const FString& Message);
	void SetRequestState(bool bRequesting, const FString& Message);
	void ShowRegistrationPanel(bool bShowRegistration);

	UFUNCTION()
	void HandleLoginClicked();

	UFUNCTION()
	void HandleLoginPasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleOpenRegistrationClicked();

	UFUNCTION()
	void HandleCreateAccountClicked();

	UFUNCTION()
	void HandleRegistrationBackClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> LoginPanel;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UImage> AccountBackground;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RegistrationPanel;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> UsernameInput;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> PasswordInput;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RegisterButton;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegistrationUsernameInput;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegistrationPasswordInput;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegistrationPasswordConfirmInput;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RegistrationStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CreateAccountButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RegistrationBackButton;

	bool bRegistrationMode = false;
};
