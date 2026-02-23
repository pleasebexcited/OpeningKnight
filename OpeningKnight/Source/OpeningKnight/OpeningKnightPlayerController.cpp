#include "OpeningKnightPlayerController.h"
#include "OpeningKnightBattleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

AOpeningKnightPlayerController::AOpeningKnightPlayerController()
{
	Battle = CreateDefaultSubobject<UOpeningKnightBattleComponent>(TEXT("Battle"));
	bShowMouseCursor = true;
}

void AOpeningKnightPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Bind phase changes so we can keep the Roll button art in sync (Roll vs Reroll) without BP wiring.
	if (Battle)
	{
		Battle->OnPhaseChanged.AddDynamic(this, &AOpeningKnightPlayerController::HandleBattlePhaseChanged);

		// Apply immediately for initial state.
		UpdateRollButtonModeForPhase(Battle->GetPhase());
	}

	// Curtains UI (optional)
	if (CurtainsWidgetClass)
	{
		CurtainsWidget = CreateWidget<UUserWidget>(this, CurtainsWidgetClass);
		if (CurtainsWidget)
		{
			CurtainsWidget->AddToViewport(CurtainsZOrder);
		}
	}
}

void AOpeningKnightPlayerController::HandleBattlePhaseChanged(EOKTurnPhase NewPhase)
{
	UpdateRollButtonModeForPhase(NewPhase);

	// When the enemy is defeated, the battle enters Victory and stays there until UI advances.
	// This is the right moment to close curtains and do a stage transition.
	if (NewPhase == EOKTurnPhase::Victory && CurtainsWidget)
	{
		bCurtainsTransitionInProgress = true;
		TryCallCurtainsFunction(FName(TEXT("CloseCurtains")));
	}
}

void AOpeningKnightPlayerController::UpdateRollButtonModeForPhase(EOKTurnPhase PhaseToApply)
{
	// Desired behavior:
	// - WaitingForRoll => show ROLL
	// - PlayerSelecting => show REROLL
	// - anything else (attack/enemy/etc) => show ROLL
	const bool bIsReroll = (PhaseToApply == EOKTurnPhase::PlayerSelecting);

	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), false);

	for (UUserWidget* W : Widgets)
	{
		if (!IsValid(W)) continue;

		// Call BP function if present: SetRollButtonMode(bool bIsReroll)
		static const FName FuncName(TEXT("SetRollButtonMode"));
		UFunction* Func = W->FindFunction(FuncName);
		if (!Func) continue;

		struct FSetRollButtonModeParams
		{
			bool bIsReroll;
		};

		FSetRollButtonModeParams Params{ bIsReroll };
		W->ProcessEvent(Func, &Params);
	}
}

void AOpeningKnightPlayerController::RollOrReroll()
{
	if (UWorld* World = GetWorld())
	{
		LastRollClickTimeSeconds = World->GetTimeSeconds();
	}

	if (Battle) Battle->RollOrReroll();
}

void AOpeningKnightPlayerController::PlayHand()
{
	if (!Battle) return;

	// Prevent spam-clicks on the Play button from looking like the game is "doing nothing":
	// only forward when the battle will actually accept the PlayHand() call.
	if (Battle->GetPhase() != EOKTurnPhase::PlayerSelecting)
	{
		return;
	}

	Battle->PlayHand();
}

void AOpeningKnightPlayerController::OnDieClicked(int32 DieIndex)
{
	if (UWorld* World = GetWorld())
	{
		const float Now = World->GetTimeSeconds();
		if ((Now - LastRollClickTimeSeconds) < DieClickSuppressionAfterRollSeconds)
		{
			return;
		}
	}

	if (Battle) Battle->ToggleDieSelected(DieIndex);
}

void AOpeningKnightPlayerController::ConfirmPlayerHit()
{
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] PlayerController::ConfirmPlayerHit (Blueprint/code entry), Battle=%s"), Battle ? TEXT("valid") : TEXT("null"));
	if (Battle) Battle->ConfirmPlayerHit();
}

void AOpeningKnightPlayerController::ConfirmEnemyHit()
{
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] PlayerController::ConfirmEnemyHit (Blueprint/code entry), Battle=%s"), Battle ? TEXT("valid") : TEXT("null"));
	if (Battle) Battle->ConfirmEnemyHit();
}

void AOpeningKnightPlayerController::FinishEnemyTurn()
{
	if (Battle) Battle->FinishEnemyTurn();
}

void AOpeningKnightPlayerController::TryCallCurtainsFunction(FName FuncName)
{
	if (!CurtainsWidget) return;
	UFunction* Func = CurtainsWidget->FindFunction(FuncName);
	if (!Func) return;
	CurtainsWidget->ProcessEvent(Func, nullptr);
}

void AOpeningKnightPlayerController::CurtainsNotifyFullyClosed()
{
	if (!bCurtainsTransitionInProgress) return;
	if (!Battle) return;

	// Hook for later sprints: swap assets while curtains are closed.
	OnCurtainsClosedForTransition();

	// Advance to next encounter.
	Battle->AdvanceAfterVictory();

	// Open curtains again.
	TryCallCurtainsFunction(FName(TEXT("OpenCurtains")));
}

void AOpeningKnightPlayerController::CurtainsNotifyFullyOpened()
{
	bCurtainsTransitionInProgress = false;
}

AActor* AOpeningKnightPlayerController::GetHitCloudInWorld(UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return nullptr;

	UClass* HitCloudClass = LoadClass<AActor>(nullptr, TEXT("/Game/Images/BP_HitCloudFX.BP_HitCloudFX_C"));
	if (!HitCloudClass) return nullptr;

	TArray<AActor*> Out;
	UGameplayStatics::GetAllActorsOfClass(World, HitCloudClass, Out);
	return Out.Num() > 0 ? Out[0] : nullptr;
}

void AOpeningKnightPlayerController::NotifyPlayerAttackHitboxStart(UPrimitiveComponent* Hitbox)
{
	if (Hitbox)
	{
		Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AOpeningKnightPlayerController::NotifyPlayerAttackHitboxEnd(UPrimitiveComponent* Hitbox)
{
	if (Hitbox)
	{
		Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AOpeningKnightPlayerController::LogHitCloudImpact(const FString& SourceName, FVector HitboxWorldLocation, FVector HitCloudActorLocation)
{
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] %s impact: AttackHitBox world=(%.1f, %.1f, %.1f)  HitCloudRef location=(%.1f, %.1f, %.1f)"),
		*SourceName,
		HitboxWorldLocation.X, HitboxWorldLocation.Y, HitboxWorldLocation.Z,
		HitCloudActorLocation.X, HitCloudActorLocation.Y, HitCloudActorLocation.Z);
}

UTexture2D* AOpeningKnightPlayerController::GetDiceTextureForValue(int32 Value, bool bIsWild) const
{
	if (bIsWild) return DiceStar;

	switch (Value)
	{
	case 1: return Dice1;
	case 2: return Dice2;
	case 3: return Dice3;
	case 4: return Dice4;
	case 5: return Dice5;
	case 6: return Dice6;
	default: return nullptr;
	}
}
