// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "OpeningKnightPawn.generated.h"

/**
 * Wrapper pawn used as Default Pawn Class. It finds your existing BP_Knight (Actor) in the level
 * by tag and exposes it so the battle system passes the real knight to the enemy. No need to
 * reparent or remake BP_Knight.
 *
 * Setup: 1) Place BP_Knight in the level. 2) Select it, in Details set Actor tag to "Knight".
 * 3) Set Game Mode Default Pawn Class to OpeningKnightPawn (this class).
 */
UCLASS()
class OPENINGKNIGHT_API AOpeningKnightPawn : public APawn
{
	GENERATED_BODY()

public:
	AOpeningKnightPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	/** Returns the knight Actor in the level (BP_Knight). Used so enemy attack targets the real knight, not this pawn. */
	UFUNCTION(BlueprintPure, Category = "OpeningKnight")
	AActor* GetKnightActor() const { return KnightActor.Get(); }

	/** Tag used to find the knight in the level. Set on your BP_Knight instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight")
	FName KnightActorTag = FName(TEXT("Knight"));

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> KnightActor;
};
