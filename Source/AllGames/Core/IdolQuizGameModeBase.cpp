// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizGameModeBase.h"
#include "IdolQuizPlayerController.h"
#include "IdolQuizGameStateBase.h"
#include "IdolQuizPlayerState.h"
#include "../Online/IdolQuizSessionSubsystem.h"
#include "TimerManager.h"
AIdolQuizGameModeBase::AIdolQuizGameModeBase()
{
	DefaultPawnClass=nullptr; PlayerControllerClass=AIdolQuizPlayerController::StaticClass(); GameStateClass=AIdolQuizGameStateBase::StaticClass(); PlayerStateClass=AIdolQuizPlayerState::StaticClass(); bUseSeamlessTravel=true;
	QuestionTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IdolQuiz/Data/DT_IdolQuizQuestions.DT_IdolQuizQuestions")));
}
void AIdolQuizGameModeBase::BeginPlay(){Super::BeginPlay();StartQuiz();}
void AIdolQuizGameModeBase::StartQuiz()
{
	GetWorldTimerManager().ClearTimer(AdvanceTimer);GetWorldTimerManager().ClearTimer(RoundTimer);
	LoadedQuestionTable=QuestionTable.LoadSynchronous();EligibleQuestions.Reset();QuestionOrder.Reset();CurrentOrderIndex=INDEX_NONE;Score=0;bRoundResolved=false;
	if(!LoadedQuestionTable){UE_LOG(LogTemp,Error,TEXT("Idol Quiz question DataTable is missing."));return;}
	EIdolQuizRoomCategory RoomCategory=EIdolQuizRoomCategory::Idol;if(UGameInstance* GI=GetGameInstance())if(UIdolQuizSessionSubsystem* Sessions=GI->GetSubsystem<UIdolQuizSessionSubsystem>())RoomCategory=Sessions->GetActiveRoomCategory();
	TArray<FIdolQuizQuestion*> Rows;LoadedQuestionTable->GetAllRows(TEXT("IdolQuiz"),Rows);
	for(const FIdolQuizQuestion* Row:Rows)if(Row&&Row->bEnabled&&!Row->StageName.IsEmpty()&&!Row->Image.IsNull()){const bool bActor=Row->Category.Equals(TEXT("Actor"),ESearchCase::IgnoreCase);const bool bAllowed=RoomCategory==EIdolQuizRoomCategory::IdolAndActor||(RoomCategory==EIdolQuizRoomCategory::Actor?bActor:!bActor);if(bAllowed)EligibleQuestions.Add(Row);}
	if(EligibleQuestions.IsEmpty()){UE_LOG(LogTemp,Error,TEXT("Idol Quiz question DataTable has no enabled rows."));return;}
	for(int32 Index=0;Index<EligibleQuestions.Num();++Index)QuestionOrder.Add(Index);
	for(int32 Index=QuestionOrder.Num()-1;Index>0;--Index)QuestionOrder.Swap(Index,FMath::RandRange(0,Index));
	QuestionOrder.SetNum(FMath::Min(QuestionsPerGame,QuestionOrder.Num()));AdvanceQuestion();
}
const FIdolQuizQuestion* AIdolQuizGameModeBase::GetCurrentQuestion()const
{
	if(!LoadedQuestionTable||!QuestionOrder.IsValidIndex(CurrentOrderIndex))return nullptr;
	const int32 Index=QuestionOrder[CurrentOrderIndex];return EligibleQuestions.IsValidIndex(Index)?EligibleQuestions[Index]:nullptr;
}
FString AIdolQuizGameModeBase::NormalizeAnswer(const FString& Value)
{
	FString Result=Value.TrimStartAndEnd().ToLower();
	for(const TCHAR* Separator:{TEXT(" "),TEXT("\t"),TEXT("\r"),TEXT("\n"),TEXT("-"),TEXT("_"),TEXT("."),TEXT("·"),TEXT("("),TEXT(")")})Result.ReplaceInline(Separator,TEXT(""));
	return Result;
}
FString AIdolQuizGameModeBase::BuildInitialHint(const FString& Value)
{
	static const TCHAR Initials[]=TEXT("ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ");FString Result;for(const TCHAR Character:Value){if(Character>=0xAC00&&Character<=0xD7A3)Result.AppendChar(Initials[(Character-0xAC00)/588]);else if(FChar::IsAlnum(Character))Result.AppendChar(Character);}return Result;
}
void AIdolQuizGameModeBase::AppendAliases(const FString& Aliases,TArray<FString>& OutAnswers)
{
	TArray<FString> ParsedAliases;Aliases.ParseIntoArray(ParsedAliases,TEXT("|"),true);OutAnswers.Append(ParsedAliases);
}
void AIdolQuizGameModeBase::SubmitAnswer(const FString& Answer)
{
	SubmitMessage(nullptr,Answer);
}
void AIdolQuizGameModeBase::SubmitMessage(APlayerController* Sender,const FString& Answer)
{
	const FIdolQuizQuestion* Question=GetCurrentQuestion();if(!Question||bRoundResolved||Answer.TrimStartAndEnd().IsEmpty())return;
	AIdolQuizPlayerState* PlayerState=Sender?Sender->GetPlayerState<AIdolQuizPlayerState>():nullptr;const FString PlayerName=PlayerState?PlayerState->GetQuizPlayerName():TEXT("Player");
	if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->MulticastChat(PlayerName,Answer.TrimStartAndEnd().Left(80));
	const FString Normalized=NormalizeAnswer(Answer);TArray<FString> AcceptedAnswers={Question->StageName,Question->RealName};AppendAliases(Question->Aliases,AcceptedAnswers);
	bool bCorrect=false;for(const FString& Accepted:AcceptedAnswers)if(!Accepted.IsEmpty()&&(Answer.TrimStartAndEnd().Equals(Accepted.TrimStartAndEnd(),ESearchCase::IgnoreCase)||Normalized==NormalizeAnswer(Accepted))){bCorrect=true;break;}
	if(!bCorrect){UE_LOG(LogTemp,Log,TEXT("Idol Quiz wrong answer: input='%s', stage='%s', real='%s'"),*Answer,*Question->StageName,*Question->RealName);OnAnswerResolved.Broadcast(false,Answer.TrimStartAndEnd(),Score);return;}
	bRoundResolved=true;GetWorldTimerManager().ClearTimer(RoundTimer);Score+=100;if(PlayerState)PlayerState->AddCorrectAnswer();UE_LOG(LogTemp,Log,TEXT("Idol Quiz correct answer: %s (%s)"),*Question->StageName,*Answer);OnAnswerResolved.Broadcast(true,Question->StageName,Score);if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->MulticastFeedback(true,PlayerName,Question->StageName);GetWorldTimerManager().SetTimer(AdvanceTimer,this,&ThisClass::AdvanceQuestion,1.2f,false);
}
void AIdolQuizGameModeBase::AdvanceQuestion()
{
	GetWorldTimerManager().ClearTimer(RoundTimer);++CurrentOrderIndex;bRoundResolved=false;bHintRevealed=false;if(!QuestionOrder.IsValidIndex(CurrentOrderIndex)){RemainingTimeSeconds=0;OnTimeChanged.Broadcast(0);OnHintChanged.Broadcast(FString());OnQuizFinished.Broadcast(Score,QuestionOrder.Num());if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->SetFinished(true);return;}
	const FIdolQuizQuestion* Question=GetCurrentQuestion();OnQuestionChanged.Broadcast(*Question,CurrentOrderIndex+1,QuestionOrder.Num());if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->SetRoundState(Question->Image.ToSoftObjectPath(),CurrentOrderIndex+1,QuestionOrder.Num());StartRoundTimer();
}
void AIdolQuizGameModeBase::StartRoundTimer()
{
	RemainingTimeSeconds=RoundDurationSeconds;bHintRevealed=false;OnTimeChanged.Broadcast(RemainingTimeSeconds);OnHintChanged.Broadcast(FString());if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>()){GS->SetRemainingTime(RemainingTimeSeconds);GS->SetHint(FString());}GetWorldTimerManager().SetTimer(RoundTimer,this,&ThisClass::TickRoundTimer,1.f,true);
}
void AIdolQuizGameModeBase::TickRoundTimer()
{
	if(bRoundResolved)return;RemainingTimeSeconds=FMath::Max(0,RemainingTimeSeconds-1);OnTimeChanged.Broadcast(RemainingTimeSeconds);if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->SetRemainingTime(RemainingTimeSeconds);
	if(!bHintRevealed&&RoundDurationSeconds-RemainingTimeSeconds>=HintAfterSeconds){bHintRevealed=true;const FIdolQuizQuestion* Question=GetCurrentQuestion();const FString NewHint=Question?BuildInitialHint(Question->StageName):FString();OnHintChanged.Broadcast(NewHint);if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->SetHint(NewHint);}
	if(RemainingTimeSeconds<=0){bRoundResolved=true;GetWorldTimerManager().ClearTimer(RoundTimer);const FIdolQuizQuestion* Question=GetCurrentQuestion();const FString CorrectAnswer=Question?Question->StageName:FString();OnRoundTimedOut.Broadcast(CorrectAnswer);if(AIdolQuizGameStateBase* GS=GetGameState<AIdolQuizGameStateBase>())GS->MulticastFeedback(false,FString(),CorrectAnswer);GetWorldTimerManager().SetTimer(AdvanceTimer,this,&ThisClass::AdvanceQuestion,2.f,false);}
}
