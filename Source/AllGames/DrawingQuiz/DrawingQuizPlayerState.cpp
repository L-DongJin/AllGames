#include "DrawingQuizPlayerState.h"
#include "Net/UnrealNetwork.h"

ADrawingQuizPlayerState::ADrawingQuizPlayerState(){bReplicates=true;}
void ADrawingQuizPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,QuizName);DOREPLIFETIME(ThisClass,QuizScore);DOREPLIFETIME(ThisClass,bSolvedCurrentRound);}
void ADrawingQuizPlayerState::SetQuizName(const FString& Value){if(HasAuthority()){AssignAvailablePlayerColor();QuizName=Value.TrimStartAndEnd().Left(20);}}
void ADrawingQuizPlayerState::AddScore(const int32 Value){if(HasAuthority())QuizScore=FMath::Max(0,QuizScore+Value);}
void ADrawingQuizPlayerState::SetSolvedCurrentRound(const bool bValue){if(HasAuthority())bSolvedCurrentRound=bValue;}
