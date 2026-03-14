#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OpeningKnightBattleComponent.h"
#include "BlockMinigameWidget.generated.h"

class UImage;
class UCanvasPanel;
class UTexture2D;

/**
 * Block/Counter minigame: 3 dice in triangle, player clicks in ascending order within time limit.
 * Bind to Battle's OnBlockMinigameBlockRoundStarted / OnBlockMinigameCounterRoundStarted.
 * Call NotifyBlockMinigameRoundResult when done. For testing, create BP child and add to viewport from PlayerController.
 */
UCLASS()
class OPENINGKNIGHT_API UBlockMinigameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Call when player clicks a die. DieIndex = 0, 1, or 2. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|BlockMinigame")
	void OnBlockDiceClickedByIndex(int32 DieIndex);

	/** Alternative: call with the die face value (1-6) when a die is clicked. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|BlockMinigame")
	void OnBlockDiceClicked(int32 Value);

protected:
	UFUNCTION()
	void HandleBlockRoundStarted(FOKBlockMinigameDice Dice);

	UFUNCTION()
	void HandleCounterRoundStarted(FOKBlockMinigameDice Dice);

	UFUNCTION()
	void HandleBlockFailed();

	UFUNCTION()
	void HandleBlockOnly();

	UFUNCTION()
	void HandleFullSuccess();

	void ShowDice(const TArray<int32>& Values);
	void HideAndReset();
	void OnMinigameTimerExpired();
	void ShowResultAndHideAfterDelay(UImage* ResultImage, float DelaySeconds);
	void OnBlockSuccessThenCounterDice();  // After BLOCKED displays briefly, show counter dice

	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* RootCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BlockDice0;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BlockDice1;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BlockDice2;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ImgBlocked;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ImgCountered;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ImgFailed;

	/** Size (in slate units) at which dice textures are displayed in the block minigame. Default 64. Increase for bigger dice (e.g. 96, 128). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block Minigame", meta = (ClampMin = "32", ClampMax = "256"))
	float DiceDisplaySize = 64.0f;

private:
	TArray<int32> CurrentDiceValues;
	TArray<int32> SortedValues;     // ascending order to click
	int32 NextExpectedIndex = 0;
	FTimerHandle MinigameTimerHandle;
	FTimerHandle ResultDisplayTimerHandle;
	FTimerHandle BlockSuccessDelayHandle;
	TArray<int32> PendingCounterDice;
};
