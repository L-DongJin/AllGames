#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Online/IdolQuizSessionSubsystem.h"
#include "IdolQuizRoomWidget.generated.h"
class UButton;class UCanvasPanel;class UComboBoxString;class UEditableTextBox;class UTextBlock;
UCLASS()
class ALLGAMES_API UIdolQuizRoomWidget:public UUserWidget
{
	GENERATED_BODY()
public: UIdolQuizRoomWidget(const FObjectInitializer& ObjectInitializer);
protected:virtual TSharedRef<SWidget>RebuildWidget()override;virtual void NativeConstruct()override;virtual void NativeDestruct()override;
private:
	void BuildLayout();void HandleRoomsFound(bool bSuccess,const TArray<FIdolQuizRoomInfo>& Rooms);void HandleSessionAction(bool bSuccess,const FString& Message);void JoinIndex(int32 Index);
	UFUNCTION()void OpenCreateDialog();UFUNCTION()void CloseCreateDialog();UFUNCTION()void ConfirmCreateRoom();UFUNCTION()void HandleCategoryChanged(FString SelectedItem,ESelectInfo::Type SelectionType);UFUNCTION()void HandleRefresh();UFUNCTION()void HandleMainHub();UFUNCTION()void Join0();UFUNCTION()void Join1();UFUNCTION()void Join2();UFUNCTION()void Join3();UFUNCTION()void Join4();UFUNCTION()void Join5();
	UPROPERTY(Transient)TObjectPtr<UCanvasPanel>CreateDialog;UPROPERTY(Transient)TObjectPtr<UEditableTextBox>RoomNameInput;UPROPERTY(Transient)TObjectPtr<UComboBoxString>CategoryDropdown;UPROPERTY(Transient)TObjectPtr<UComboBoxString>QuestionCountDropdown;UPROPERTY(Transient)TObjectPtr<UTextBlock>DialogErrorText;UPROPERTY(Transient)TObjectPtr<UTextBlock>StatusText;UPROPERTY(Transient)TArray<TObjectPtr<UButton>>RoomButtons;UPROPERTY(Transient)TArray<TObjectPtr<UTextBlock>>RoomLabels;UPROPERTY(Transient)TObjectPtr<UIdolQuizSessionSubsystem>Sessions;EIdolQuizRoomCategory SelectedCategory=EIdolQuizRoomCategory::Idol;
};
