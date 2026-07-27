#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DrawingQuiz/DrawingQuizTypes.h"
#include "DrawingCanvasWidget.generated.h"

class ADrawingQuizGameState;
UCLASS()
class ALLGAMES_API UDrawingCanvasWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetBrushColor(const FColor& Value){BrushColor=Value;bEraser=false;}
	void SetEraser(bool bValue){bEraser=bValue;}
	void RequestClear();
protected:
	virtual void NativeConstruct()override;
	virtual void NativeDestruct()override;
	virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
	virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& AllottedGeometry,const FSlateRect& MyCullingRect,FSlateWindowElementList& OutDrawElements,int32 LayerId,const FWidgetStyle& InWidgetStyle,bool bParentEnabled)const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event)override;
private:
	void BindState(); void AddNetworkStroke(const FDrawingQuizStrokeSegment& Segment); void ClearNetworkCanvas(); bool CanDraw()const; FVector2D ToNormalized(const FGeometry& Geometry,const FVector2D& ScreenPosition)const;
	UPROPERTY(Transient) TObjectPtr<ADrawingQuizGameState> QuizState;
	TArray<FDrawingQuizStrokeSegment> Segments;
	TArray<FDrawingQuizStrokeSegment> PendingPredictedSegments;
	TArray<FDrawingQuizStrokeSegment> OutboundSegments;
	FVector2D PreviousPoint;
	FColor BrushColor=FColor::Black;
	float BrushThickness=6.f;
	bool bEraser=false;
	bool bDragging=false;
};
