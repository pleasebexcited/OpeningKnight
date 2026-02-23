#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OpeningKnightBattleComponent.generated.h"

UENUM(BlueprintType)
enum class EOKTurnPhase : uint8
{
	WaitingForRoll UMETA(DisplayName = "Waiting For Roll"),
	PlayerSelecting UMETA(DisplayName = "Player Selecting"),
	AwaitingPlayerHitConfirm UMETA(DisplayName = "Awaiting Player Hit Confirm"),
	AwaitingEnemyHitConfirm UMETA(DisplayName = "Awaiting Enemy Hit Confirm"),
	EnemyTurn UMETA(DisplayName = "Enemy Turn"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat"),
};

USTRUCT(BlueprintType)
struct FOKDieUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Value = 0;      // 0 = empty/unrolled, 1..6 = face

	UPROPERTY(BlueprintReadOnly)
	bool bIsWild = false; // star

	UPROPERTY(BlueprintReadOnly)
	bool bIsSelected = false;
};

USTRUCT(BlueprintType)
struct FOKStatsUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerHP = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerMaxHP = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerShield = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 EnemyHP = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 EnemyMaxHP = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 EnemyShield = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOKDiceChanged, const TArray<FOKDieUI>&, Dice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOKPhaseChanged, EOKTurnPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOKRerollsChanged, int32, RerollsLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOKAttackStarted, int32, PendingDamage, FString, DebugText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOKEnemyAttackStartedWithKnight, int32, PendingDamage, FString, DebugText, AActor*, KnightActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOKStatsChanged, FOKStatsUI, Stats);

UCLASS(ClassGroup = (OpeningKnight), meta = (BlueprintSpawnableComponent))
class OPENINGKNIGHT_API UOpeningKnightBattleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOpeningKnightBattleComponent();

	// ===== UI / Blueprint calls =====
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void StartNewRun();

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void RollOrReroll();

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void ToggleDieSelected(int32 DieIndex);

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void PlayHand(); // computes score + fires OnPlayerAttackStarted; waits for ConfirmPlayerHit()

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void ConfirmPlayerHit(); // call at the IMPACT moment (cloud/knockback moment)

	// Enemy attack: call at the enemy IMPACT moment (to time player HP reduction with animation).
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void ConfirmEnemyHit();

	// Call after the enemy has finished returning to idle (lets you sync ResetHand with animation end).
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void FinishEnemyTurn();

	/**
	 * Called after the curtains close (or any stage transition effect finishes) to advance from Victory
	 * into the next encounter (heal, increment level, spawn next enemy, reset hand).
	 *
	 * This exists so Victory can be a stable phase for UI/animations instead of instantly resetting.
	 */
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	void AdvanceAfterVictory();

	// ===== Getters =====
	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	EOKTurnPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	bool HasRolledThisHand() const { return bHasRolledThisHand; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetRerollsLeft() const { return RerollsLeft; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetPlayerHP() const { return PlayerHP; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetPlayerMaxHP() const { return PlayerMaxHP; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetPlayerShield() const { return PlayerShield; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetEnemyHP() const { return EnemyHP; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetEnemyShield() const { return EnemyShield; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetEnemyMaxHP() const { return EnemyMaxHP; }

	UFUNCTION(BlueprintCallable, Category = "OpeningKnight|Battle")
	int32 GetLevel() const { return Level; }

	// ===== Events (bind in BP) =====
	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKDiceChanged OnDiceChanged;

	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKRerollsChanged OnRerollsChanged;

	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKAttackStarted OnPlayerAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKAttackStarted OnEnemyAttackStarted;

	/** Fired when the enemy turn attack starts; includes Knight actor so BP can tell the enemy to attack him. Bind in Level or GameMode to call StartAttack on BP_Enemy. */
	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKEnemyAttackStartedWithKnight OnEnemyAttackStartedWithKnight;

	// Player/Enemy HP + Shield updates for UI
	UPROPERTY(BlueprintAssignable, Category = "OpeningKnight|Events")
	FOKStatsChanged OnStatsChanged;

	// ===== Tunables (match Python prototype defaults) =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float WildChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	int32 MaxRerollsPerHand = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float EnemyTurnDelaySeconds = 0.65f;

	/** Extra delay (seconds) after player hit is confirmed before the enemy attack starts. Use this so the knight can finish his return animation first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float EnemyAttackStartDelaySeconds = 0.8f;

	/**
	 * Optional safety auto-advance for Victory if nothing calls AdvanceAfterVictory().
	 * Set to 0 to fully control victory transitions from UI (recommended for curtains).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float AutoAdvanceAfterVictorySeconds = 0.75f;

	// If > 0, PlayHand() will auto-call ConfirmPlayerHit() after this delay. Set to 0 to use only Blueprint at the impact moment (recommended; avoids duplicate call).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float AutoConfirmPlayerHitDelaySeconds = 0.0f;

	// If > 0, enemy turn will auto-call ConfirmEnemyHit() after this delay. Set to 0 and call ConfirmEnemyHit from BP at impact (e.g. animation notify) so damage syncs with the hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float AutoConfirmEnemyHitDelaySeconds = 0.0f;

	// Optional delay after enemy impact before ResetHand() (lets enemy "return" before player can roll).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float EnemyReturnDelaySeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	int32 PlayerMaxHP = 25000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	int32 BaseEnemyHP = 15000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float EnemyHPScalePerLevel = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	int32 BaseShield = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpeningKnight|Tuning")
	float PostVictoryHealPercent = 0.15f;

protected:
	virtual void BeginPlay() override;

private:
	struct FDie
	{
		int32 Value = 1;   // 1..6, or 0 if wild/unassigned
		bool bWild = false;
		bool bSelected = false;
	};

	EOKTurnPhase Phase = EOKTurnPhase::WaitingForRoll;

	int32 Level = 1;

	int32 RerollsLeft = 3;
	bool bHasRolledThisHand = false;

	TArray<FDie> Dice;

	int32 PlayerHP = 0;
	int32 PlayerShield = 0;
	int32 EnemyHP = 0;
	int32 EnemyMaxHP = 0;
	int32 EnemyShield = 0;

	int32 PendingPlayerDamage = 0;
	FString PendingPlayerDebugText;

	int32 PendingEnemyDamage = 0;
	FString PendingEnemyDebugText;

	bool bEnemyReturnPending = false;

	// Flow helpers
	void SetPhase(EOKTurnPhase NewPhase);
	void ResetHand();
	void SpawnEnemyForLevel();
	void BroadcastDice();
	void BroadcastStats();

	FTimerHandle AutoConfirmPlayerHandle;
	FTimerHandle AutoConfirmEnemyHandle;
	FTimerHandle EnemyReturnHandle;
	FTimerHandle EnemyTurnHandle;
	FTimerHandle AutoAdvanceVictoryHandle;

	void RollAllDice();
	void RerollSelectedDice();

	void ApplyDamageToEnemy(int32 Damage);
	void ApplyDamageToPlayer(int32 Damage);

	void StartEnemyTurn();
	void ResolveEnemyTurn();

	// Dice helpers
	int32 RollDieValue(bool& bOutWild) const;

	// Scoring helpers (ported from your Python approach)
	void CalculateBestScore(const TArray<int32>& Values, const TArray<int32>& WildIndices, int32& OutScore, FString& OutHand, FString& OutCalc) const;
	void CalculateSingleScore(const TArray<int32>& Values, int32& OutScore, FString& OutHand, FString& OutCalc) const;
	bool IsSequential(const TArray<int32>& Seq) const;
	void FindBestStraight(const TArray<int32>& Sorted, int32& OutLen, TArray<int32>& OutSeq) const;
};