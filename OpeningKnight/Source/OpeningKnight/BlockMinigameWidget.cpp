#include "BlockMinigameWidget.h"
#include "OpeningKnightPlayerController.h"
#include "OpeningKnightBattleComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

void UBlockMinigameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
	if (!PC || !PC->GetBattle()) return;

	UOpeningKnightBattleComponent* Battle = PC->GetBattle();
	Battle->OnBlockMinigameBlockRoundStarted.AddDynamic(this, &UBlockMinigameWidget::HandleBlockRoundStarted);
	Battle->OnBlockMinigameCounterRoundStarted.AddDynamic(this, &UBlockMinigameWidget::HandleCounterRoundStarted);
	Battle->OnBlockMinigameFailed.AddDynamic(this, &UBlockMinigameWidget::HandleBlockFailed);
	Battle->OnBlockMinigameBlockOnly.AddDynamic(this, &UBlockMinigameWidget::HandleBlockOnly);
	Battle->OnBlockMinigameFullSuccess.AddDynamic(this, &UBlockMinigameWidget::HandleFullSuccess);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UBlockMinigameWidget::NativeDestruct()
{
	AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
	if (PC && PC->GetBattle())
	{
		UOpeningKnightBattleComponent* B = PC->GetBattle();
		if (B)
		{
			B->OnBlockMinigameBlockRoundStarted.RemoveDynamic(this, &UBlockMinigameWidget::HandleBlockRoundStarted);
			B->OnBlockMinigameCounterRoundStarted.RemoveDynamic(this, &UBlockMinigameWidget::HandleCounterRoundStarted);
			B->OnBlockMinigameFailed.RemoveDynamic(this, &UBlockMinigameWidget::HandleBlockFailed);
			B->OnBlockMinigameBlockOnly.RemoveDynamic(this, &UBlockMinigameWidget::HandleBlockOnly);
			B->OnBlockMinigameFullSuccess.RemoveDynamic(this, &UBlockMinigameWidget::HandleFullSuccess);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MinigameTimerHandle);
		World->GetTimerManager().ClearTimer(ResultDisplayTimerHandle);
		World->GetTimerManager().ClearTimer(BlockSuccessDelayHandle);
	}

	Super::NativeDestruct();
}

void UBlockMinigameWidget::HandleBlockRoundStarted(FOKBlockMinigameDice Dice)
{
	UE_LOG(LogTemp, Log, TEXT("[Block] BlockMinigameWidget received OnBlockMinigameBlockRoundStarted, dice=%d %d %d"), Dice.Values[0], Dice.Values[1], Dice.Values[2]);
	ShowDice(Dice.Values);
}

void UBlockMinigameWidget::HandleCounterRoundStarted(FOKBlockMinigameDice Dice)
{
	// Block succeeded: show BLOCKED for ~1s, then transition to counter dice (no BLOCKED during counter)
	PendingCounterDice = Dice.Values;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	TArray<UImage*> DiceImages = { BlockDice0, BlockDice1, BlockDice2 };
	for (UImage* Img : DiceImages)
	{
		if (Img) Img->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ImgBlocked) ImgBlocked->SetVisibility(ESlateVisibility::Visible);
	if (ImgCountered) ImgCountered->SetVisibility(ESlateVisibility::Collapsed);
	if (ImgFailed) ImgFailed->SetVisibility(ESlateVisibility::Collapsed);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockSuccessDelayHandle);
		World->GetTimerManager().SetTimer(BlockSuccessDelayHandle, this, &UBlockMinigameWidget::OnBlockSuccessThenCounterDice, 1.0f, false);
	}
	else
	{
		OnBlockSuccessThenCounterDice();
	}
}

void UBlockMinigameWidget::OnBlockSuccessThenCounterDice()
{
	BlockSuccessDelayHandle.Invalidate();
	if (ImgBlocked) ImgBlocked->SetVisibility(ESlateVisibility::Collapsed);
	ShowDice(PendingCounterDice);
	PendingCounterDice.Reset();
}

void UBlockMinigameWidget::HandleBlockFailed()
{
	ShowResultAndHideAfterDelay(ImgFailed, 1.5f);
}

void UBlockMinigameWidget::HandleBlockOnly()
{
	// Block succeeded but counter failed: show only FAILED, not BLOCKED
	if (ImgBlocked) ImgBlocked->SetVisibility(ESlateVisibility::Collapsed);
	if (ImgFailed) ImgFailed->SetVisibility(ESlateVisibility::Visible);
	ShowResultAndHideAfterDelay(nullptr, 2.0f);
}

void UBlockMinigameWidget::HandleFullSuccess()
{
	// Block+counter success: show only COUNTERED, not BLOCKED
	if (ImgBlocked) ImgBlocked->SetVisibility(ESlateVisibility::Collapsed);
	if (ImgCountered) ImgCountered->SetVisibility(ESlateVisibility::Visible);
	ShowResultAndHideAfterDelay(nullptr, 2.0f);
}

void UBlockMinigameWidget::ShowResultAndHideAfterDelay(UImage* ResultImage, float DelaySeconds)
{
	if (ResultImage)
	{
		ResultImage->SetVisibility(ESlateVisibility::Visible);
	}
	// Hide dice
	TArray<UImage*> DiceImages = { BlockDice0, BlockDice1, BlockDice2 };
	for (UImage* Img : DiceImages)
	{
		if (Img) Img->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWorld* World = GetWorld())
	{
		FTimerDelegate Del;
		Del.BindLambda([this]() { HideAndReset(); });
		World->GetTimerManager().SetTimer(ResultDisplayTimerHandle, Del, DelaySeconds, false);
	}
	else
	{
		HideAndReset();
	}
}

void UBlockMinigameWidget::ShowDice(const TArray<int32>& Values)
{
	CurrentDiceValues = Values;
	SortedValues = Values;
	SortedValues.Sort();
	NextExpectedIndex = 0;

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
	if (!PC) return;

	TArray<UImage*> DiceImages = { BlockDice0, BlockDice1, BlockDice2 };
	for (int32 i = 0; i < 3 && i < Values.Num(); i++)
	{
		if (UImage* Img = DiceImages[i])
		{
			UTexture2D* Tex = PC->GetDiceTextureForValue(Values[i], false);
			if (Tex)
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(Tex);
				Brush.SetImageSize(FVector2D(DiceDisplaySize, DiceDisplaySize));
				Img->SetBrush(Brush);
				Img->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

	if (ImgBlocked) ImgBlocked->SetVisibility(ESlateVisibility::Collapsed);
	if (ImgCountered) ImgCountered->SetVisibility(ESlateVisibility::Collapsed);
	if (ImgFailed) ImgFailed->SetVisibility(ESlateVisibility::Collapsed);

	float TimeLimit = 5.0f;
	if (PC->Battle)
	{
		TimeLimit = PC->GetBattle()->BlockMinigameTimeLimitSeconds;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MinigameTimerHandle);
		World->GetTimerManager().SetTimer(MinigameTimerHandle, this, &UBlockMinigameWidget::OnMinigameTimerExpired, TimeLimit, false);
	}
}

void UBlockMinigameWidget::HideAndReset()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MinigameTimerHandle);
	}
	SetVisibility(ESlateVisibility::Collapsed);
	CurrentDiceValues.Reset();
	SortedValues.Reset();
	NextExpectedIndex = 0;
}

void UBlockMinigameWidget::OnMinigameTimerExpired()
{
	MinigameTimerHandle.Invalidate();
	AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
	if (PC && PC->GetBattle())
	{
		PC->GetBattle()->NotifyBlockMinigameRoundResult(false);
	}
	// Battle will fire OnBlockMinigameFailed or OnBlockMinigameBlockOnly; handler shows image + HideAndReset
}

void UBlockMinigameWidget::OnBlockDiceClickedByIndex(int32 DieIndex)
{
	if (CurrentDiceValues.IsValidIndex(DieIndex))
	{
		OnBlockDiceClicked(CurrentDiceValues[DieIndex]);
	}
}

void UBlockMinigameWidget::OnBlockDiceClicked(int32 Value)
{
	if (!SortedValues.IsValidIndex(NextExpectedIndex)) return;
	if (Value != SortedValues[NextExpectedIndex])
	{
		// Wrong order - fail (Battle will fire OnBlockMinigameFailed or OnBlockMinigameBlockOnly, handler will show image + hide)
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MinigameTimerHandle);
		}
		AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
		if (PC && PC->GetBattle())
		{
			PC->GetBattle()->NotifyBlockMinigameRoundResult(false);
		}
		return;
	}

	NextExpectedIndex++;
	if (NextExpectedIndex >= 3)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MinigameTimerHandle);
		}
		AOpeningKnightPlayerController* PC = Cast<AOpeningKnightPlayerController>(GetOwningPlayer());
		if (PC && PC->GetBattle())
		{
			PC->GetBattle()->NotifyBlockMinigameRoundResult(true);
		}
		// Do NOT HideAndReset - Battle will fire CounterRoundStarted (we ShowDice again) or a finish event (we HideAndReset via HandleMinigameFinished)
	}
}
