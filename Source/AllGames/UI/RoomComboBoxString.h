#pragma once

#include "CoreMinimal.h"
#include "Components/ComboBoxString.h"
#include "RoomComboBoxString.generated.h"

/** Combo box used by the room dialog with a readable, Slate-safe default text style. */
UCLASS()
class ALLGAMES_API URoomComboBoxString : public UComboBoxString
{
	GENERATED_BODY()

public:
	URoomComboBoxString(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> HandleGenerateWidget(TSharedPtr<FString> Item) const override;
};
