#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IdolQuizLobbyWidget.generated.h"
class UButton;class UTextBlock;
UCLASS()
class ALLGAMES_API UIdolQuizLobbyWidget:public UUserWidget
{
	GENERATED_BODY()
public: UIdolQuizLobbyWidget(const FObjectInitializer& ObjectInitializer);
protected:virtual TSharedRef<SWidget>RebuildWidget()override;virtual void NativeConstruct()override;virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
private:void BuildLayout();void RefreshPlayers();UFUNCTION()void HandleStart();UFUNCTION()void HandleLeave();UPROPERTY(Transient)TArray<TObjectPtr<UTextBlock>>PlayerNames;UPROPERTY(Transient)TObjectPtr<UButton>StartButton;UPROPERTY(Transient)TObjectPtr<UTextBlock>StatusText;
};
