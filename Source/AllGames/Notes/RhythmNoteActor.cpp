// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmNoteActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARhythmNoteActor::ARhythmNoteActor()
{
	PrimaryActorTick.bCanEverTick = false;

	NoteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NoteMesh"));
	SetRootComponent(NoteMesh);
	NoteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NoteMesh->SetRelativeScale3D(FVector(0.8, 0.8, 0.2));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		NoteMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

void ARhythmNoteActor::InitializeNote(const FRhythmNoteData& InNoteData)
{
	NoteData = InNoteData;
#if WITH_EDITOR
	SetActorLabel(FString::Printf(TEXT("Note_Lane%d_Time%.2f"), NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds));
#endif
}
