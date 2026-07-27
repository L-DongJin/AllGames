#include "IdolQuizPlayerState.h"
#include "Net/UnrealNetwork.h"
AIdolQuizPlayerState::AIdolQuizPlayerState(){bReplicates=true;}
void AIdolQuizPlayerState::SetQuizPlayerName(const FString& InName){if(HasAuthority()){AssignAvailablePlayerColor();QuizPlayerName=InName.TrimStartAndEnd().Left(20);if(QuizPlayerName.IsEmpty())QuizPlayerName=TEXT("Player");SetPlayerName(QuizPlayerName);}}
void AIdolQuizPlayerState::AddCorrectAnswer(){if(HasAuthority())++CorrectAnswers;}
void AIdolQuizPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,QuizPlayerName);DOREPLIFETIME(ThisClass,CorrectAnswers);}
