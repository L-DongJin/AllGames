#include "DrawingQuizGameState.h"
#include "DrawingQuizPlayerState.h"
#include "Net/UnrealNetwork.h"

ADrawingQuizGameState::ADrawingQuizGameState(){bReplicates=true;}
void ADrawingQuizGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,CurrentRound);DOREPLIFETIME(ThisClass,TotalRounds);DOREPLIFETIME(ThisClass,RemainingTime);DOREPLIFETIME(ThisClass,DrawerPlayer);DOREPLIFETIME(ThisClass,Hint);DOREPLIFETIME(ThisClass,Feedback);DOREPLIFETIME(ThisClass,bFinished);DOREPLIFETIME(ThisClass,CanvasRevision);}
void ADrawingQuizGameState::OnRep_State(){OnStateChanged.Broadcast();}
void ADrawingQuizGameState::OnRep_CanvasRevision(){OnCanvasCleared.Broadcast();}
void ADrawingQuizGameState::SetRoundState(int32 Round,int32 Total,ADrawingQuizPlayerState* Drawer,int32 Seconds,const FString& InHint){if(!HasAuthority())return;CurrentRound=Round;TotalRounds=Total;DrawerPlayer=Drawer;RemainingTime=Seconds;Hint=InHint;Feedback.Empty();bFinished=false;OnStateChanged.Broadcast();ForceNetUpdate();}
void ADrawingQuizGameState::SetRemainingTime(const int32 Seconds){if(HasAuthority()){RemainingTime=Seconds;OnStateChanged.Broadcast();ForceNetUpdate();}}
void ADrawingQuizGameState::SetFeedback(const FString& Value){if(HasAuthority()){Feedback=Value;OnStateChanged.Broadcast();ForceNetUpdate();}}
void ADrawingQuizGameState::SetFinished(const bool bValue){if(HasAuthority()){bFinished=bValue;OnStateChanged.Broadcast();ForceNetUpdate();}}
void ADrawingQuizGameState::ClearCanvasState(){if(!HasAuthority())return;++CanvasRevision;OnCanvasCleared.Broadcast();ForceNetUpdate();}
void ADrawingQuizGameState::MulticastStrokes_Implementation(const TArray<FDrawingQuizStrokeSegment>& Segments){for(const FDrawingQuizStrokeSegment& Segment:Segments)OnStroke.Broadcast(Segment);}
void ADrawingQuizGameState::MulticastChat_Implementation(const FString& PlayerName,const FString& Message,const int32 PlayerColorIndex){OnChat.Broadcast(PlayerName,Message,PlayerColorIndex);}
