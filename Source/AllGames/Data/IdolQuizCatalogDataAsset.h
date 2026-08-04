// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "IdolQuizCatalogDataAsset.generated.h"
class UTexture2D;
USTRUCT(BlueprintType)
struct FIdolQuizQuestion : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") FName QuestionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") TSoftObjectPtr<UTexture2D> Image;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") FString StageName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") FString RealName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz", meta=(ToolTip="Separate additional accepted answers with |")) FString Aliases;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") FString GroupName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") int32 Generation = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") FString Category = TEXT("Idol");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz", meta=(ToolTip="Stable pool IDs separated with |")) FString PoolTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") bool bEnabled = true;
};
UCLASS(BlueprintType)
class ALLGAMES_API UIdolQuizCatalogDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Idol Quiz") TArray<FIdolQuizQuestion> Questions;
};
