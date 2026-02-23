// Fill out your copyright notice in the Description page of Project Settings.

#include "OpeningKnightPawn.h"
#include "Kismet/GameplayStatics.h"

AOpeningKnightPawn::AOpeningKnightPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AOpeningKnightPawn::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), KnightActorTag, Found);
	if (Found.Num() > 0)
	{
		AActor* Knight = Found[0];
		KnightActor = Knight;
		// Attach so this pawn (and camera) follows the knight
		AttachToActor(Knight, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}
