#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OpeningKnightBattleComponent.h"
#include "OpeningKnightPlayerController.generated.h"

class AActor;
class UOpeningKnightBattleComponent;
class UTexture2D;
class UPrimitiveComponent;
class UUserWidget;

UCLASS()
class OPENINGKNIGHT_API AOpeningKnightPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOpeningKnightPlayerController();

protected:
	virtual void BeginPlay() override;

	// Called by UI widgets
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void RollOrReroll();

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void PlayHand();

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void OnDieClicked(int32 DieIndex);

	// Called by BP_Knight at impact moment
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void ConfirmPlayerHit();

	// Called by BP_Enemy (or similar) at impact moment
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void ConfirmEnemyHit();

	// Called by BP_Enemy (or similar) after the enemy returns to idle
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void FinishEnemyTurn();

	/**
	 * Call this from your curtain widget when the "close" animation finishes (curtains meet in middle).
	 * We'll advance the battle out of Victory and then request the curtains open again.
	 */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void CurtainsNotifyFullyClosed();

	/** Optional: call from the curtain widget when fully opened (off-screen). */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|UI")
	void CurtainsNotifyFullyOpened();

	/** Hook for later sprints: called right after curtains close, before advancing to next encounter. */
	UFUNCTION(BlueprintImplementableEvent, Category = "OpeningKnight|UI")
	void OnCurtainsClosedForTransition();

	/** Returns the first BP_HitCloudFX actor in the level. Call from BP_Knight Event Begin Play and assign to HitCloudRef so the knight attack impact can move and flash the cloud. Default reference to a level instance is invalid in PIE. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetHitCloudInWorld(UObject* WorldContextObject);

	/** Call from BP_Knight at the start of the knight attack (e.g. when On Player Attack Started fires). Disables collision on the given hitbox so it cannot trigger overlap until impact. Call every attack. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void NotifyPlayerAttackHitboxStart(UPrimitiveComponent* Hitbox);

	/** Call from BP_Knight when the knight attack ends (e.g. timeline Finished). Disables collision on the hitbox so the next attack starts clean. Call every attack. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void NotifyPlayerAttackHitboxEnd(UPrimitiveComponent* Hitbox);

	/** Debug: call from BP_Knight (KnightHitBc) and BP_Enemy (HitCloudEvent) at impact. Logs SourceName + hitbox world location + HitCloud actor location so you can compare 1st vs 2nd/3rd attack. */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Debug")
	void LogHitCloudImpact(const FString& SourceName, FVector HitboxWorldLocation, FVector HitCloudActorLocation);

	// Dice face lookup for UMG
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Dice")
	UTexture2D* GetDiceTextureForValue(int32 Value, bool bIsWild) const;

	// Dice tint helpers for UMG (selected dice should be more noticeable).
	UFUNCTION(BlueprintPure, Category = "OpeningKnight|Dice")
	FLinearColor GetSelectedDiceTint() const { return SelectedDiceTint; }

	UFUNCTION(BlueprintPure, Category = "OpeningKnight|Dice")
	FLinearColor GetUnselectedDiceTint() const { return UnselectedDiceTint; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OpeningKnight")
	UOpeningKnightBattleComponent* Battle = nullptr;

	/** Assign `WBP_Curtains` (or similar) in BP_OpeningKnightPlayerController defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|UI")
	TSubclassOf<UUserWidget> CurtainsWidgetClass;

	/** Higher draws on top of other UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|UI")
	int32 CurtainsZOrder = 500;

	// Assign these in the BP defaults of BP_OpeningKnightPlayerController after re-parenting
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice1 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice2 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice3 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice4 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice5 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* Dice6 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Faces")
	UTexture2D* DiceStar = nullptr;

	// Tint colors applied in the dice UMG widget.
	// Note: UMG tinting multiplies the sprite color, so "gold" is achieved by reducing blue.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Tint")
	FLinearColor UnselectedDiceTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OpeningKnight|Dice Tint")
	FLinearColor SelectedDiceTint = FLinearColor(1.0f, 0.85f, 0.15f, 1.0f);

private:
	UPROPERTY(Transient)
	UUserWidget* CurtainsWidget = nullptr;

	bool bCurtainsTransitionInProgress = false;

	void TryCallCurtainsFunction(FName FuncName);

	// Keep the Roll button art in sync with the battle phase by calling the widget BP function
	// `SetRollButtonMode(bool bIsReroll)` if present.
	UFUNCTION()
	void HandleBattlePhaseChanged(EOKTurnPhase NewPhase);

	void UpdateRollButtonModeForPhase(EOKTurnPhase PhaseToApply);

	// Prevent UMG click-through: a Roll click can also land on a die in the same frame.
	// We ignore die clicks briefly after RollOrReroll() is pressed.
	float LastRollClickTimeSeconds = -1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "OpeningKnight|UI")
	float DieClickSuppressionAfterRollSeconds = 0.15f;
};