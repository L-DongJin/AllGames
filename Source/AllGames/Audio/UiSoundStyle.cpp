// Copyright Epic Games, Inc. All Rights Reserved.

#include "UiSoundStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	USoundBase* GetSharedButtonClickSound()
	{
		static TSoftObjectPtr<USoundBase> ClickSound(
			FSoftObjectPath(TEXT("/Game/Audio/UI/S_UI_Click.S_UI_Click")));
		return ClickSound.LoadSynchronous();
	}
}

void AllGamesUiSound::ApplyButtonClickSound(UWidgetTree* WidgetTree)
{
	USoundBase* ClickSound = GetSharedButtonClickSound();
	if (!WidgetTree || !ClickSound)
	{
		return;
	}

	FSlateSound PressedSound;
	PressedSound.SetResourceObject(ClickSound);
	WidgetTree->ForEachWidget([&PressedSound](UWidget* Widget)
	{
		if (UButton* Button = Cast<UButton>(Widget))
		{
			FButtonStyle Style = Button->GetStyle();
			Style.SetPressedSound(PressedSound);
			Button->SetStyle(Style);
		}
	});
}
