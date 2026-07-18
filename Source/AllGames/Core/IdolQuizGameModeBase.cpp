// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizGameModeBase.h"
#include "IdolQuizPlayerController.h"
#include "TimerManager.h"
AIdolQuizGameModeBase::AIdolQuizGameModeBase()
{
	DefaultPawnClass=nullptr; PlayerControllerClass=AIdolQuizPlayerController::StaticClass();
	QuestionTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IdolQuiz/Data/DT_IdolQuizQuestions.DT_IdolQuizQuestions")));
}
void AIdolQuizGameModeBase::BeginPlay(){Super::BeginPlay();StartQuiz();}
void AIdolQuizGameModeBase::StartQuiz()
{
	GetWorldTimerManager().ClearTimer(AdvanceTimer);GetWorldTimerManager().ClearTimer(RoundTimer);
	LoadedQuestionTable=QuestionTable.LoadSynchronous();EligibleQuestions.Reset();QuestionOrder.Reset();CurrentOrderIndex=INDEX_NONE;Score=0;bRoundResolved=false;
	if(!LoadedQuestionTable){UE_LOG(LogTemp,Error,TEXT("Idol Quiz question DataTable is missing."));return;}
	TArray<FIdolQuizQuestion*> Rows;LoadedQuestionTable->GetAllRows(TEXT("IdolQuiz"),Rows);
	for(const FIdolQuizQuestion* Row:Rows)if(Row&&Row->bEnabled&&!Row->StageName.IsEmpty()&&!Row->Image.IsNull())EligibleQuestions.Add(Row);
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
	const FIdolQuizQuestion* Question=GetCurrentQuestion();if(!Question||bRoundResolved||Answer.TrimStartAndEnd().IsEmpty())return;
	const FString Normalized=NormalizeAnswer(Answer);TArray<FString> AcceptedAnswers={Question->StageName,Question->RealName};AppendAliases(Question->Aliases,AcceptedAnswers);
	bool bCorrect=false;for(const FString& Accepted:AcceptedAnswers)if(!Accepted.IsEmpty()&&(Answer.TrimStartAndEnd().Equals(Accepted.TrimStartAndEnd(),ESearchCase::IgnoreCase)||Normalized==NormalizeAnswer(Accepted))){bCorrect=true;break;}
	if(!bCorrect){UE_LOG(LogTemp,Log,TEXT("Idol Quiz wrong answer: input='%s', stage='%s', real='%s'"),*Answer,*Question->StageName,*Question->RealName);OnAnswerResolved.Broadcast(false,Answer.TrimStartAndEnd(),Score);return;}
	bRoundResolved=true;GetWorldTimerManager().ClearTimer(RoundTimer);Score+=100;UE_LOG(LogTemp,Log,TEXT("Idol Quiz correct answer: %s (%s)"),*Question->StageName,*Answer);OnAnswerResolved.Broadcast(true,Question->StageName,Score);GetWorldTimerManager().SetTimer(AdvanceTimer,this,&ThisClass::AdvanceQuestion,1.2f,false);
}
void AIdolQuizGameModeBase::AdvanceQuestion()
{
	GetWorldTimerManager().ClearTimer(RoundTimer);++CurrentOrderIndex;bRoundResolved=false;bHintRevealed=false;if(!QuestionOrder.IsValidIndex(CurrentOrderIndex)){RemainingTimeSeconds=0;OnTimeChanged.Broadcast(0);OnHintChanged.Broadcast(FString());OnQuizFinished.Broadcast(Score,QuestionOrder.Num());return;}
	OnQuestionChanged.Broadcast(*GetCurrentQuestion(),CurrentOrderIndex+1,QuestionOrder.Num());StartRoundTimer();
}
void AIdolQuizGameModeBase::StartRoundTimer()
{
	RemainingTimeSeconds=RoundDurationSeconds;bHintRevealed=false;OnTimeChanged.Broadcast(RemainingTimeSeconds);OnHintChanged.Broadcast(FString());GetWorldTimerManager().SetTimer(RoundTimer,this,&ThisClass::TickRoundTimer,1.f,true);
}
void AIdolQuizGameModeBase::TickRoundTimer()
{
	if(bRoundResolved)return;RemainingTimeSeconds=FMath::Max(0,RemainingTimeSeconds-1);OnTimeChanged.Broadcast(RemainingTimeSeconds);
	if(!bHintRevealed&&RoundDurationSeconds-RemainingTimeSeconds>=HintAfterSeconds){bHintRevealed=true;const FIdolQuizQuestion* Question=GetCurrentQuestion();OnHintChanged.Broadcast(Question?BuildInitialHint(Question->StageName):FString());}
	if(RemainingTimeSeconds<=0){bRoundResolved=true;GetWorldTimerManager().ClearTimer(RoundTimer);const FIdolQuizQuestion* Question=GetCurrentQuestion();const FString CorrectAnswer=Question?Question->StageName:FString();OnRoundTimedOut.Broadcast(CorrectAnswer);GetWorldTimerManager().SetTimer(AdvanceTimer,this,&ThisClass::AdvanceQuestion,2.f,false);}
}
