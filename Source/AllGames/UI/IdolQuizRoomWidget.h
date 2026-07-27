#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Online/IdolQuizSessionSubsystem.h"
#include "IdolQuizRoomWidget.generated.h"
class UButton;class UCanvasPanel;class UComboBoxString;class UEditableTextBox;class UImage;class UTextBlock;class UTexture2D;
UCLASS()
class ALLGAMES_API UIdolQuizRoomWidget:public UUserWidget
{
	GENERATED_BODY()
public:
	UIdolQuizRoomWidget(const FObjectInitializer& ObjectInitializer);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Appearance")TObjectPtr<UTexture2D>RoomBrowserBackgroundImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Appearance")FLinearColor RoomBrowserBackgroundTint=FLinearColor::White;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Create Button")TObjectPtr<UTexture2D>CreateButtonNormalImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Create Button")TObjectPtr<UTexture2D>CreateButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Create Button")TObjectPtr<UTexture2D>CreateButtonPressedImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Create Button",meta=(ClampMin="0.0"))FVector2D CreateButtonSize=FVector2D(230,58);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Refresh Button")TObjectPtr<UTexture2D>RefreshButtonNormalImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Refresh Button")TObjectPtr<UTexture2D>RefreshButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Refresh Button")TObjectPtr<UTexture2D>RefreshButtonPressedImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Refresh Button",meta=(ClampMin="0.0"))FVector2D RefreshButtonSize=FVector2D(230,58);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Back Button")TObjectPtr<UTexture2D>BackButtonNormalImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Back Button")TObjectPtr<UTexture2D>BackButtonHoveredImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Back Button")TObjectPtr<UTexture2D>BackButtonPressedImage;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Back Button",meta=(ClampMin="0.0"))FVector2D BackButtonSize=FVector2D(190,52);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Room Entries")FLinearColor RoomEntryNormalColor=FLinearColor(.22f,.22f,.22f,1);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Room Entries")FLinearColor RoomEntryHoveredColor=FLinearColor(.32f,.36f,.48f,1);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Room Browser|Room Entries")FLinearColor RoomEntryPressedColor=FLinearColor(.14f,.18f,.3f,1);
protected:virtual TSharedRef<SWidget>RebuildWidget()override;virtual void NativeConstruct()override;virtual void NativeDestruct()override;
private:
	void BuildLayout();void BuildCreateDialog(UCanvasPanel* Root);void HandleRoomsFound(bool bSuccess,const TArray<FIdolQuizRoomInfo>& Rooms);void HandleSessionAction(bool bSuccess,const FString& Message);void JoinIndex(int32 Index);
	void RefreshGameSettingsVisibility();
	UFUNCTION()void OpenCreateDialog();UFUNCTION()void CloseCreateDialog();UFUNCTION()void ConfirmCreateRoom();UFUNCTION()void HandleGameChanged(FString SelectedItem,ESelectInfo::Type SelectionType);UFUNCTION()void HandleCategoryChanged(FString SelectedItem,ESelectInfo::Type SelectionType);UFUNCTION()void HandleRefresh();UFUNCTION()void HandleMainHub();UFUNCTION()void Join0();UFUNCTION()void Join1();UFUNCTION()void Join2();UFUNCTION()void Join3();UFUNCTION()void Join4();UFUNCTION()void Join5();
	UPROPERTY(Transient)TObjectPtr<UImage>RoomBrowserBackground;
	UPROPERTY(Transient)TObjectPtr<UButton>CreateRoomButton;
	UPROPERTY(Transient)TObjectPtr<UButton>RefreshRoomsButton;
	UPROPERTY(Transient)TObjectPtr<UButton>BackToHubButton;
	UPROPERTY(Transient)TObjectPtr<UCanvasPanel>CreateDialog;UPROPERTY(Transient)TObjectPtr<UEditableTextBox>RoomNameInput;UPROPERTY(Transient)TObjectPtr<UComboBoxString>GameDropdown;UPROPERTY(Transient)TObjectPtr<UComboBoxString>CategoryDropdown;UPROPERTY(Transient)TObjectPtr<UComboBoxString>QuestionCountDropdown;UPROPERTY(Transient)TObjectPtr<UComboBoxString>DrawingRoundsDropdown;UPROPERTY(Transient)TObjectPtr<UComboBoxString>DrawingTimeDropdown;UPROPERTY(Transient)TObjectPtr<UTextBlock>CategoryLabel;UPROPERTY(Transient)TObjectPtr<UTextBlock>QuestionCountLabel;UPROPERTY(Transient)TObjectPtr<UTextBlock>DrawingRoundsLabel;UPROPERTY(Transient)TObjectPtr<UTextBlock>DrawingTimeLabel;UPROPERTY(Transient)TObjectPtr<UTextBlock>DialogErrorText;UPROPERTY(Transient)TObjectPtr<UTextBlock>StatusText;UPROPERTY(Transient)TArray<TObjectPtr<UButton>>RoomButtons;UPROPERTY(Transient)TArray<TObjectPtr<UTextBlock>>RoomLabels;UPROPERTY(Transient)TObjectPtr<UIdolQuizSessionSubsystem>Sessions;EIdolQuizRoomCategory SelectedCategory=EIdolQuizRoomCategory::Idol;EMiniGameRoomType SelectedGameType=EMiniGameRoomType::PersonQuiz;
	int32 VisibleRoomCount=0;
};
