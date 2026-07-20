#include "IdolQuizGameStateBase.h"
#include "Net/UnrealNetwork.h"
AIdolQuizGameStateBase::AIdolQuizGameStateBase(){bReplicates=true;}
void AIdolQuizGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,QuestionImagePath);DOREPLIFETIME(ThisClass,CurrentRound);DOREPLIFETIME(ThisClass,TotalRounds);DOREPLIFETIME(ThisClass,RemainingTime);DOREPLIFETIME(ThisClass,Hint);DOREPLIFETIME(ThisClass,bFinished);DOREPLIFETIME(ThisClass,SkipVotes);DOREPLIFETIME(ThisClass,RequiredSkipVotes);}
void AIdolQuizGameStateBase::OnRep_State(){OnQuizStateChanged.Broadcast();}
void AIdolQuizGameStateBase::SetRoundState(const FSoftObjectPath& Path,int32 Round,int32 Total){if(HasAuthority()){QuestionImagePath=Path;CurrentRound=Round;TotalRounds=Total;bFinished=false;OnQuizStateChanged.Broadcast();ForceNetUpdate();}}
void AIdolQuizGameStateBase::SetRemainingTime(int32 Seconds){if(HasAuthority()){RemainingTime=Seconds;OnQuizStateChanged.Broadcast();ForceNetUpdate();}}
void AIdolQuizGameStateBase::SetHint(const FString& InHint){if(HasAuthority()){Hint=InHint;OnQuizStateChanged.Broadcast();ForceNetUpdate();}}
void AIdolQuizGameStateBase::SetFinished(bool bInFinished){if(HasAuthority()){bFinished=bInFinished;OnQuizStateChanged.Broadcast();ForceNetUpdate();}}
void AIdolQuizGameStateBase::SetSkipProgress(int32 Votes,int32 RequiredVotesValue){if(HasAuthority()){SkipVotes=FMath::Max(0,Votes);RequiredSkipVotes=FMath::Max(1,RequiredVotesValue);OnQuizStateChanged.Broadcast();ForceNetUpdate();}}
void AIdolQuizGameStateBase::MulticastChat_Implementation(const FString& Name,const FString& Message){OnChatReceived.Broadcast(Name,Message);}
void AIdolQuizGameStateBase::MulticastFeedback_Implementation(bool bCorrect,const FString& Name,const FString& Message){OnFeedbackReceived.Broadcast(bCorrect,Name,Message);}
void AIdolQuizGameStateBase::MulticastSkipFeedback_Implementation(const FString& CorrectAnswer){OnSkipReceived.Broadcast(CorrectAnswer);}
