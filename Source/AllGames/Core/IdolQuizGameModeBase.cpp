// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizGameModeBase.h"
#include "IdolQuizPlayerController.h"
#include "TimerManager.h"
AIdolQuizGameModeBase::AIdolQuizGameModeBase()
{
	DefaultPawnClass=nullptr; PlayerControllerClass=AIdolQuizPlayerController::StaticClass();
	QuestionCatalog=TSoftObjectPtr<UIdolQuizCatalogDataAsset>(FSoftObjectPath(TEXT("/Game/IdolQuiz/Data/DA_IdolQuiz_3rdGeneration.DA_IdolQuiz_3rdGeneration")));
}
void AIdolQuizGameModeBase::BeginPlay(){Super::BeginPlay();StartQuiz();}
void AIdolQuizGameModeBase::StartQuiz()
{
	LoadedCatalog=QuestionCatalog.LoadSynchronous();QuestionOrder.Reset();CurrentOrderIndex=INDEX_NONE;Score=0;bRoundResolved=false;
	if(!LoadedCatalog||LoadedCatalog->Questions.IsEmpty()){UE_LOG(LogTemp,Error,TEXT("Idol Quiz catalog is missing or empty."));return;}
	for(int32 Index=0;Index<LoadedCatalog->Questions.Num();++Index)QuestionOrder.Add(Index);
	for(int32 Index=QuestionOrder.Num()-1;Index>0;--Index)QuestionOrder.Swap(Index,FMath::RandRange(0,Index));
	QuestionOrder.SetNum(FMath::Min(QuestionsPerGame,QuestionOrder.Num()));AdvanceQuestion();
}
const FIdolQuizQuestion* AIdolQuizGameModeBase::GetCurrentQuestion()const
{
	if(!LoadedCatalog||!QuestionOrder.IsValidIndex(CurrentOrderIndex))return nullptr;
	const int32 Index=QuestionOrder[CurrentOrderIndex];return LoadedCatalog->Questions.IsValidIndex(Index)?&LoadedCatalog->Questions[Index]:nullptr;
}
FString AIdolQuizGameModeBase::NormalizeAnswer(const FString& Value)
{
	FString Result;for(const TCHAR Character:Value.TrimStartAndEnd().ToLower())if(FChar::IsAlnum(Character))Result.AppendChar(Character);return Result;
}
void AIdolQuizGameModeBase::SubmitAnswer(const FString& Answer)
{
	const FIdolQuizQuestion* Question=GetCurrentQuestion();if(!Question||bRoundResolved||Answer.TrimStartAndEnd().IsEmpty())return;
	const FString Normalized=NormalizeAnswer(Answer);bool bCorrect=Normalized==NormalizeAnswer(Question->Answer);
	for(const FString& Accepted:Question->AcceptedAnswers)bCorrect|=Normalized==NormalizeAnswer(Accepted);
	if(!bCorrect){OnAnswerResolved.Broadcast(false,Answer.TrimStartAndEnd(),Score);return;}
	bRoundResolved=true;Score+=100;OnAnswerResolved.Broadcast(true,Question->Answer,Score);GetWorldTimerManager().SetTimer(AdvanceTimer,this,&ThisClass::AdvanceQuestion,1.2f,false);
}
void AIdolQuizGameModeBase::AdvanceQuestion()
{
	++CurrentOrderIndex;bRoundResolved=false;if(!QuestionOrder.IsValidIndex(CurrentOrderIndex)){OnQuizFinished.Broadcast(Score,QuestionOrder.Num());return;}
	OnQuestionChanged.Broadcast(*GetCurrentQuestion(),CurrentOrderIndex+1,QuestionOrder.Num());
}
