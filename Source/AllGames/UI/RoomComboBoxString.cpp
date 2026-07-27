#include "RoomComboBoxString.h"

#include "Widgets/Text/STextBlock.h"

URoomComboBoxString::URoomComboBoxString(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UE 5.7 exposes these style values as deprecated fields but does not provide
	// equivalent setters. Setting them before RebuildWidget is safe and avoids
	// constructing UWidgets recursively from the row-generation callback.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	ForegroundColor = FSlateColor(FLinearColor::White);
	Font.Size = 18;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

TSharedRef<SWidget> URoomComboBoxString::HandleGenerateWidget(
	TSharedPtr<FString> Item) const
{
	const FString Label = Item.IsValid() ? *Item : FString();
	return SNew(STextBlock)
		.Text(FText::FromString(Label))
		.Font(GetFont())
		.ColorAndOpacity(FSlateColor(FLinearColor::White));
}
