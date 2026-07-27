#include "DrawingCanvasWidget.h"
#include "../DrawingQuiz/DrawingQuizGameState.h"
#include "../DrawingQuiz/DrawingQuizPlayerController.h"
#include "../DrawingQuiz/DrawingQuizPlayerState.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"

namespace
{
	bool SameSegment(const FDrawingQuizStrokeSegment& A,const FDrawingQuizStrokeSegment& B)
	{
		return A.Start.Equals(B.Start,.0001f)&&A.End.Equals(B.End,.0001f)
			&&A.Color==B.Color&&FMath::IsNearlyEqual(A.Thickness,B.Thickness,.01f)
			&&A.bEraser==B.bEraser;
	}
}

void UDrawingCanvasWidget::NativeConstruct(){Super::NativeConstruct();SetVisibility(ESlateVisibility::Visible);BindState();}
void UDrawingCanvasWidget::NativeDestruct(){if(QuizState){QuizState->OnStroke.RemoveAll(this);QuizState->OnCanvasCleared.RemoveAll(this);}Super::NativeDestruct();}
void UDrawingCanvasWidget::NativeTick(const FGeometry& MyGeometry,const float InDeltaTime)
{
	Super::NativeTick(MyGeometry,InDeltaTime);
	if(!QuizState)BindState();
	if(!OutboundSegments.IsEmpty())
	{
		if(ADrawingQuizPlayerController* PC=GetOwningPlayer<ADrawingQuizPlayerController>())
		{
			PC->ServerDrawSegments(OutboundSegments);
		}
		OutboundSegments.Reset();
	}
}
void UDrawingCanvasWidget::BindState(){ADrawingQuizGameState* State=GetWorld()?GetWorld()->GetGameState<ADrawingQuizGameState>():nullptr;if(!State||State==QuizState)return;if(QuizState){QuizState->OnStroke.RemoveAll(this);QuizState->OnCanvasCleared.RemoveAll(this);}QuizState=State;QuizState->OnStroke.AddUObject(this,&ThisClass::AddNetworkStroke);QuizState->OnCanvasCleared.AddUObject(this,&ThisClass::ClearNetworkCanvas);}
bool UDrawingCanvasWidget::CanDraw()const{return QuizState&&GetOwningPlayer()&&GetOwningPlayer()->PlayerState==QuizState->GetDrawer()&&!QuizState->IsFinished();}
FVector2D UDrawingCanvasWidget::ToNormalized(const FGeometry& Geometry,const FVector2D& ScreenPosition)const{const FVector2D Local=Geometry.AbsoluteToLocal(ScreenPosition);const FVector2D Size=Geometry.GetLocalSize();return FVector2D(Size.X>0?FMath::Clamp(Local.X/Size.X,0.f,1.f):0,Size.Y>0?FMath::Clamp(Local.Y/Size.Y,0.f,1.f):0);}
int32 UDrawingCanvasWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool ParentEnabled)const
{
	const FVector2D Size=Geometry.GetLocalSize();
	FSlateDrawElement::MakeBox(Out,Layer,Geometry.ToPaintGeometry(),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor::White);
	for(int32 Index=0;Index<Segments.Num();)
	{
		const FDrawingQuizStrokeSegment& First=Segments[Index];
		TArray<FVector2D> Points={First.Start*Size,First.End*Size};
		int32 Next=Index+1;
		while(Next<Segments.Num())
		{
			const FDrawingQuizStrokeSegment& Previous=Segments[Next-1];
			const FDrawingQuizStrokeSegment& Candidate=Segments[Next];
			if(Previous.bEraser||Candidate.bEraser||Previous.Color!=Candidate.Color
				||!FMath::IsNearlyEqual(Previous.Thickness,Candidate.Thickness,.01f)
				||!Previous.End.Equals(Candidate.Start,.0001f))
			{
				break;
			}
			Points.Add(Candidate.End*Size);
			++Next;
		}
		FSlateDrawElement::MakeLines(Out,Layer+1,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(First.Color),true,First.Thickness);
		Index=Next;
	}
	return Layer+1;
}
FReply UDrawingCanvasWidget::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event){if(Event.GetEffectingButton()!=EKeys::LeftMouseButton||!CanDraw())return FReply::Unhandled();bDragging=true;PreviousPoint=ToNormalized(Geometry,Event.GetScreenSpacePosition());return FReply::Handled().CaptureMouse(TakeWidget());}
FReply UDrawingCanvasWidget::NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)
{
	if(!bDragging||!CanDraw())return FReply::Unhandled();
	const FVector2D Point=ToNormalized(Geometry,Event.GetScreenSpacePosition());
	if(FVector2D::Distance(Point,PreviousPoint)<.0025f)return FReply::Handled();
	FDrawingQuizStrokeSegment Segment;
	Segment.Start=PreviousPoint;
	Segment.End=Point;
	Segment.Color=BrushColor;
	Segment.Thickness=bEraser?24.f:BrushThickness;
	Segment.bEraser=bEraser;
	AddNetworkStroke(Segment);
	PendingPredictedSegments.Add(Segment);
	OutboundSegments.Add(Segment);
	PreviousPoint=Point;
	return FReply::Handled();
}
FReply UDrawingCanvasWidget::NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent& Event){if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();bDragging=false;return FReply::Handled().ReleaseMouseCapture();}
void UDrawingCanvasWidget::AddNetworkStroke(const FDrawingQuizStrokeSegment& Segment)
{
	const int32 PredictedIndex=PendingPredictedSegments.IndexOfByPredicate(
		[&](const FDrawingQuizStrokeSegment& Pending){return SameSegment(Pending,Segment);});
	if(PredictedIndex!=INDEX_NONE)
	{
		PendingPredictedSegments.RemoveAt(PredictedIndex);
		return;
	}
	if(Segment.bEraser)
	{
		const FVector2D EraserMid=(Segment.Start+Segment.End)*.5f;
		Segments.RemoveAll([&](const FDrawingQuizStrokeSegment& Existing)
		{
			const FVector2D ExistingMid=(Existing.Start+Existing.End)*.5f;
			return FVector2D::Distance(ExistingMid,EraserMid)<.045f
				||FVector2D::Distance(Existing.Start,EraserMid)<.04f
				||FVector2D::Distance(Existing.End,EraserMid)<.04f;
		});
	}
	else
	{
		Segments.Add(Segment);
	}
	InvalidateLayoutAndVolatility();
}
void UDrawingCanvasWidget::ClearNetworkCanvas(){Segments.Reset();PendingPredictedSegments.Reset();OutboundSegments.Reset();InvalidateLayoutAndVolatility();}
void UDrawingCanvasWidget::RequestClear()
{
	if(!CanDraw()){UE_LOG(LogTemp,Warning,TEXT("Drawing Quiz local clear ignored: local player is not the drawer."));return;}
	ClearNetworkCanvas();
	if(ADrawingQuizPlayerController* PC=GetOwningPlayer<ADrawingQuizPlayerController>())
	{
		PC->ServerClearCanvas();
	}
}
