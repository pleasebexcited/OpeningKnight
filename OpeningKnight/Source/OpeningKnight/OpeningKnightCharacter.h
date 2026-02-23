// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OpeningKnightCharacter.generated.h"

/**
 * Minimal Character so the Game Mode can use it (or a Blueprint child) as Default Pawn Class.
 * Create a Blueprint that has this as parent, add your knight mesh/logic there, then set that Blueprint as Default Pawn Class.
 */
UCLASS()
class OPENINGKNIGHT_API AOpeningKnightCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOpeningKnightCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
