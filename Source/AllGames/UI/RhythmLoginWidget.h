// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmLoginWidget.generated.h"

class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FOnRhythmLoginAccepted);

/** Minimal username/password entry screen displayed before the song lobby. */
UCLASS()
class ALLGAMES_API URhythmLoginWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmLoginWidget(const FObjectInitializer& ObjectInitializer);

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
	void HandleOpenRegistrationClicked();

	UFUNCTION()
	void HandleCreateAccountClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> LoginPanel;

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

	bool bRegistrationMode = false;
};
