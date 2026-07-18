// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IdolQuizCatalogDataAsset.generated.h"
class UTexture2D;
USTRUCT(BlueprintType)
struct FIdolQuizQuestion
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName QuestionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> Image;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Answer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> AcceptedAnswers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString GroupName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Generation = 0;
};
UCLASS(BlueprintType)
class ALLGAMES_API UIdolQuizCatalogDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") TArray<FIdolQuizQuestion> Questions;
};
