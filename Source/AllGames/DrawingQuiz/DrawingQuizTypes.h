#pragma once

#include "CoreMinimal.h"
#include "DrawingQuizTypes.generated.h"

USTRUCT(BlueprintType)
struct FDrawingQuizStrokeSegment
{
	GENERATED_BODY()

	UPROPERTY() FVector2D Start = FVector2D::ZeroVector;
	UPROPERTY() FVector2D End = FVector2D::ZeroVector;
	UPROPERTY() FColor Color = FColor::Black;
	UPROPERTY() float Thickness = 6.0f;
	UPROPERTY() bool bEraser = false;
};

