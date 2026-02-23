#include "OpeningKnightBattleComponent.h"
#include "OpeningKnightPawn.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UOpeningKnightBattleComponent::UOpeningKnightBattleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Dice.SetNum(6);
}

void UOpeningKnightBattleComponent::BeginPlay()
{
	Super::BeginPlay();
	StartNewRun();
}

void UOpeningKnightBattleComponent::StartNewRun()
{
	// Ensure any pending auto-confirm doesn't fire into a new run.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoConfirmPlayerHandle);
		World->GetTimerManager().ClearTimer(AutoConfirmEnemyHandle);
		World->GetTimerManager().ClearTimer(EnemyReturnHandle);
		World->GetTimerManager().ClearTimer(EnemyTurnHandle);
		World->GetTimerManager().ClearTimer(AutoAdvanceVictoryHandle);
	}

	PlayerHP = PlayerMaxHP;
	PlayerShield = BaseShield;

	Level = 1;
	SpawnEnemyForLevel();
	ResetHand();
	BroadcastStats();
}

void UOpeningKnightBattleComponent::SetPhase(EOKTurnPhase NewPhase)
{
	if (Phase == NewPhase) return;
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] SetPhase: %d -> %d (2=AwaitPlayerHit 3=AwaitEnemyHit 4=EnemyTurn 5=Victory 6=Defeat)"),
		(int32)Phase, (int32)NewPhase);
	Phase = NewPhase;
	OnPhaseChanged.Broadcast(Phase);
}

void UOpeningKnightBattleComponent::ResetHand()
{
	// Cancel any pending auto-confirm for the previous hand.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoConfirmPlayerHandle);
		World->GetTimerManager().ClearTimer(AutoConfirmEnemyHandle);
		World->GetTimerManager().ClearTimer(EnemyReturnHandle);
		World->GetTimerManager().ClearTimer(EnemyTurnHandle);
		World->GetTimerManager().ClearTimer(AutoAdvanceVictoryHandle);
	}

	RerollsLeft = MaxRerollsPerHand;
	bHasRolledThisHand = false;

	for (FDie& D : Dice)
	{
		D.Value = 1;
		D.bWild = false;
		D.bSelected = false;
	}

	SetPhase(EOKTurnPhase::WaitingForRoll);
	OnRerollsChanged.Broadcast(RerollsLeft);
	BroadcastDice();
	BroadcastStats();
}

void UOpeningKnightBattleComponent::SpawnEnemyForLevel()
{
	const float Scale = FMath::Pow(EnemyHPScalePerLevel, float(Level - 1));
	EnemyMaxHP = FMath::FloorToInt(float(BaseEnemyHP) * Scale);
	EnemyHP = EnemyMaxHP;
	EnemyShield = BaseShield;
}

void UOpeningKnightBattleComponent::BroadcastStats()
{
	FOKStatsUI Stats;
	Stats.PlayerHP = PlayerHP;
	Stats.PlayerMaxHP = PlayerMaxHP;
	Stats.PlayerShield = PlayerShield;
	Stats.EnemyHP = EnemyHP;
	Stats.EnemyMaxHP = EnemyMaxHP;
	Stats.EnemyShield = EnemyShield;
	OnStatsChanged.Broadcast(Stats);
}

void UOpeningKnightBattleComponent::BroadcastDice()
{
	TArray<FOKDieUI> Ui;
	Ui.Reserve(Dice.Num());

	for (const FDie& D : Dice)
	{
		FOKDieUI Out;
		if (!bHasRolledThisHand)
		{
			Out.Value = 0;
			Out.bIsWild = false;
			Out.bIsSelected = false;
		}
		else
		{
			Out.Value = D.bWild ? 1 : D.Value; // UI can ignore Value when bIsWild=true; keep non-zero
			Out.bIsWild = D.bWild;
			Out.bIsSelected = D.bSelected;
		}
		Ui.Add(Out);
	}

	OnDiceChanged.Broadcast(Ui);
}

int32 UOpeningKnightBattleComponent::RollDieValue(bool& bOutWild) const
{
	if (FMath::FRand() < WildChance)
	{
		bOutWild = true;
		return 0;
	}

	bOutWild = false;
	return FMath::RandRange(1, 6);
}

void UOpeningKnightBattleComponent::RollAllDice()
{
	for (FDie& D : Dice)
	{
		bool bWild = false;
		const int32 V = RollDieValue(bWild);
		D.bWild = bWild;
		D.Value = bWild ? 0 : V;
		D.bSelected = false;
	}

	bHasRolledThisHand = true;
	SetPhase(EOKTurnPhase::PlayerSelecting);
	BroadcastDice();
}

void UOpeningKnightBattleComponent::RerollSelectedDice()
{
	if (!bHasRolledThisHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reroll rejected (not rolled yet)"));
		return;
	}
	if (RerollsLeft <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reroll rejected (no rerolls left)"));
		return;
	}

	bool bAny = false;

	for (int32 i = 0; i < Dice.Num(); i++)
	{
		FDie& D = Dice[i];

		// New semantics (matches your desired UX):
		// bSelected=true means "this die is selected FOR reroll".
		if (!D.bSelected)
		{
			continue;
		}

		bool bWild = false;
		const int32 V = RollDieValue(bWild);
		D.bWild = bWild;
		D.Value = bWild ? 0 : V;
		bAny = true;
	}

	if (!bAny)
	{
		return;
	}

	// After every reroll, clear selection so the next reroll requires a fresh click selection.
	for (FDie& D : Dice)
	{
		D.bSelected = false;
	}

	RerollsLeft--;
	OnRerollsChanged.Broadcast(RerollsLeft);
	SetPhase(EOKTurnPhase::PlayerSelecting);
	BroadcastDice();
}

void UOpeningKnightBattleComponent::RollOrReroll()
{
	// During impact/enemy/ending phases, ignore roll input.
	if (Phase == EOKTurnPhase::AwaitingPlayerHitConfirm ||
		Phase == EOKTurnPhase::AwaitingEnemyHitConfirm ||
		Phase == EOKTurnPhase::EnemyTurn ||
		Phase == EOKTurnPhase::Victory)
	{
		return;
	}

	// QoL: once defeated, let the player click Roll to immediately restart a new run.
	if (Phase == EOKTurnPhase::Defeat)
	{
		StartNewRun();
		return;
	}

	if (!bHasRolledThisHand)
	{
		RollAllDice();
		return;
	}

	RerollSelectedDice();
}

void UOpeningKnightBattleComponent::ToggleDieSelected(int32 DieIndex)
{
	if (!bHasRolledThisHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleDieSelected rejected: not rolled yet (idx=%d)"), DieIndex);
		return;
	}
	if (Phase != EOKTurnPhase::PlayerSelecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleDieSelected rejected: wrong phase=%d (idx=%d)"), int32(Phase), DieIndex);
		return;
	}
	if (!Dice.IsValidIndex(DieIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleDieSelected rejected: invalid index=%d (dice num=%d)"), DieIndex, Dice.Num());
		return;
	}

	Dice[DieIndex].bSelected = !Dice[DieIndex].bSelected;

	BroadcastDice();
}

void UOpeningKnightBattleComponent::PlayHand()
{
	if (!bHasRolledThisHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitCloud] PlayHand IGNORED: not rolled yet"));
		return;
	}
	if (Phase != EOKTurnPhase::PlayerSelecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitCloud] PlayHand IGNORED: wrong phase=%d (need 1=PlayerSelecting)"), (int32)Phase);
		return;
	}

	TArray<int32> Values;
	Values.Reserve(6);

	TArray<int32> WildIndices;

	for (int32 i = 0; i < Dice.Num(); i++)
	{
		Values.Add(Dice[i].bWild ? 0 : Dice[i].Value);
		if (Dice[i].bWild) WildIndices.Add(i);
	}

	int32 Score = 0;
	FString Hand, Calc;
	CalculateBestScore(Values, WildIndices, Score, Hand, Calc);

	PendingPlayerDamage = Score;
	PendingPlayerDebugText = FString::Printf(TEXT("%s: %s = %d"), *Hand, *Calc, Score);

	// UX: once the player commits the hand, clear any dice selection tint immediately.
	for (FDie& D : Dice)
	{
		D.bSelected = false;
	}
	BroadcastDice();

	SetPhase(EOKTurnPhase::AwaitingPlayerHitConfirm);
	OnPlayerAttackStarted.Broadcast(PendingPlayerDamage, PendingPlayerDebugText);
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] PlayHand -> AwaitingPlayerHitConfirm, damage=%d, AutoConfirmPlayerHitDelay=%.2fs"),
		PendingPlayerDamage, AutoConfirmPlayerHitDelaySeconds);

	// Fallback so the game loop keeps moving even before BP wires ConfirmPlayerHit to an animation impact.
	if (AutoConfirmPlayerHitDelaySeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoConfirmPlayerHandle);
			World->GetTimerManager().SetTimer(
				AutoConfirmPlayerHandle,
				this,
				&UOpeningKnightBattleComponent::ConfirmPlayerHit,
				AutoConfirmPlayerHitDelaySeconds,
				false
			);
			UE_LOG(LogTemp, Log, TEXT("[HitCloud] Auto ConfirmPlayerHit timer set for %.2fs (will fire if BP does not call ConfirmPlayerHit at impact)"), AutoConfirmPlayerHitDelaySeconds);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitCloud] AutoConfirmPlayerHitDelaySeconds=0: you MUST call ConfirmPlayerHit from Blueprint at the cloud/impact moment or the game will stall."));
	}
}

void UOpeningKnightBattleComponent::ConfirmPlayerHit()
{
	// Log whether this is likely from Blueprint (PlayerController) or from the auto timer (no controller in call stack).
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmPlayerHit called, Phase=%d (expect 2=AwaitPlayerHit). If you see this twice, first=BP at impact, second=auto timer; set AutoConfirmPlayerHitDelaySeconds=0 to use only BP."), (int32)Phase);
	if (Phase != EOKTurnPhase::AwaitingPlayerHitConfirm)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitCloud] ConfirmPlayerHit IGNORED: wrong phase=%d (need 2). Call only at the player attack impact/cloud moment."), (int32)Phase);
		return;
	}

	ApplyDamageToEnemy(PendingPlayerDamage);
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmPlayerHit applied damage %d to enemy, EnemyHP now=%d"), PendingPlayerDamage, EnemyHP);

	if (EnemyHP <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmPlayerHit -> Victory (enemy defeated)"));
		SetPhase(EOKTurnPhase::Victory);

		// Victory is now a stable phase so UI can run stage transitions (curtains, swaps, etc).
		// Call AdvanceAfterVictory() when ready (e.g., when curtains finish closing).
		// Optional safety: auto-advance after a delay.
		if (AutoAdvanceAfterVictorySeconds > 0.0f)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(AutoAdvanceVictoryHandle);
				World->GetTimerManager().SetTimer(
					AutoAdvanceVictoryHandle,
					this,
					&UOpeningKnightBattleComponent::AdvanceAfterVictory,
					AutoAdvanceAfterVictorySeconds,
					false
				);
			}
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmPlayerHit -> StartEnemyTurn"));
	StartEnemyTurn();
}

void UOpeningKnightBattleComponent::AdvanceAfterVictory()
{
	if (Phase != EOKTurnPhase::Victory)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceVictoryHandle);
		World->GetTimerManager().ClearTimer(AutoConfirmPlayerHandle);
		World->GetTimerManager().ClearTimer(AutoConfirmEnemyHandle);
		World->GetTimerManager().ClearTimer(EnemyReturnHandle);
		World->GetTimerManager().ClearTimer(EnemyTurnHandle);
	}

	// basic loop for now: heal + restore shield + next enemy
	const int32 Heal = FMath::FloorToInt(float(PlayerMaxHP) * PostVictoryHealPercent);
	PlayerHP = FMath::Clamp(PlayerHP + Heal, 0, PlayerMaxHP);
	PlayerShield = BaseShield;

	Level++;
	SpawnEnemyForLevel();
	ResetHand();
}

void UOpeningKnightBattleComponent::StartEnemyTurn()
{
	SetPhase(EOKTurnPhase::EnemyTurn);
	const float TotalDelay = EnemyTurnDelaySeconds + EnemyAttackStartDelaySeconds;
	UE_LOG(LogTemp, Log, TEXT("StartEnemyTurn: phase=EnemyTurn, ResolveEnemyTurn in %.2fs"), TotalDelay);
	GetWorld()->GetTimerManager().SetTimer(
		EnemyTurnHandle,
		this,
		&UOpeningKnightBattleComponent::ResolveEnemyTurn,
		TotalDelay,
		false
	);
}

void UOpeningKnightBattleComponent::ResolveEnemyTurn()
{
	// Enemy roll
	TArray<int32> Values;
	Values.SetNum(6);

	TArray<int32> WildIndices;

	for (int32 i = 0; i < 6; i++)
	{
		bool bWild = false;
		const int32 V = RollDieValue(bWild);
		Values[i] = bWild ? 0 : V;
		if (bWild) WildIndices.Add(i);
	}

	int32 Score = 0;
	FString Hand, Calc;
	CalculateBestScore(Values, WildIndices, Score, Hand, Calc);

	PendingEnemyDamage = Score;
	PendingEnemyDebugText = FString::Printf(TEXT("%s: %s = %d"), *Hand, *Calc, Score);

	// Get Knight actor for StartAttack and for broadcast.
	AActor* KnightActor = nullptr;
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		APawn* Pawn = PC->GetPawn();
		if (AOpeningKnightPawn* Wrapper = Cast<AOpeningKnightPawn>(Pawn))
		{
			KnightActor = Wrapper->GetKnightActor();
		}
		else
		{
			KnightActor = Pawn;
		}
	}

	// CRITICAL: Call StartAttack on the enemy BEFORE setting phase to AwaitingEnemyHitConfirm.
	// StartAttack disables the Attack HitBox, so any overlap (or stale overlap) cannot trigger
	// ConfirmEnemyHit until the animation notify re-enables the hitbox at impact.
	if (KnightActor && GetWorld())
	{
		TArray<AActor*> Enemies;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Enemy")), Enemies);
		if (Enemies.Num() == 0)
		{
			UClass* EnemyClass = LoadObject<UClass>(nullptr, TEXT("/Game/Images/Enemies/BP_Enemy.BP_Enemy_C"));
			if (EnemyClass)
			{
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), EnemyClass, Enemies);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[HitCloud] ResolveEnemyTurn: calling StartAttack on enemy BEFORE phase=3 (hitbox disabled first)"));
		for (AActor* Enemy : Enemies)
		{
			if (!Enemy) continue;
			UFunction* StartAttackFunc = Enemy->FindFunction(FName(TEXT("StartAttack")));
			if (StartAttackFunc)
			{
				struct FStartAttackParms { AActor* TargetActor; };
				FStartAttackParms Parms;
				Parms.TargetActor = KnightActor;
				Enemy->ProcessEvent(StartAttackFunc, &Parms);
				UE_LOG(LogTemp, Log, TEXT("ResolveEnemyTurn: called StartAttack on %s (hitbox now disabled until impact notify)"), *Enemy->GetName());
				break;
			}
		}
	}

	// Now safe to enter AwaitingEnemyHitConfirm; overlap cannot fire until impact notify enables hitbox.
	SetPhase(EOKTurnPhase::AwaitingEnemyHitConfirm);
	OnEnemyAttackStarted.Broadcast(PendingEnemyDamage, PendingEnemyDebugText);
	OnEnemyAttackStartedWithKnight.Broadcast(PendingEnemyDamage, PendingEnemyDebugText, KnightActor);
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ResolveEnemyTurn -> AwaitingEnemyHitConfirm, damage=%d, Knight=%s, AutoConfirmEnemyHitDelay=%.2fs"),
		PendingEnemyDamage, KnightActor ? *KnightActor->GetName() : TEXT("(null)"), AutoConfirmEnemyHitDelaySeconds);

	// Fallback so the game loop keeps moving. If delay > 0, use it; if 0, use a safety fallback so the game never stalls until BP calls ConfirmEnemyHit at impact.
	const float SafetyFallbackSeconds = 2.0f;
	const float DelayToUse = (AutoConfirmEnemyHitDelaySeconds > 0.0f) ? AutoConfirmEnemyHitDelaySeconds : SafetyFallbackSeconds;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoConfirmEnemyHandle);
		World->GetTimerManager().SetTimer(
			AutoConfirmEnemyHandle,
			this,
			&UOpeningKnightBattleComponent::ConfirmEnemyHit,
			DelayToUse,
			false
		);
		if (AutoConfirmEnemyHitDelaySeconds > 0.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("[HitCloud] Auto ConfirmEnemyHit timer set for %.2fs (will fire if BP does not call at impact)"), AutoConfirmEnemyHitDelaySeconds);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HitCloud] AutoConfirmEnemyHitDelaySeconds=0: using %.1fs safety fallback. Call ConfirmEnemyHit from Blueprint at the enemy impact (e.g. animation notify) to sync damage; the fallback will be cleared when you do."), SafetyFallbackSeconds);
		}
	}
}

void UOpeningKnightBattleComponent::ConfirmEnemyHit()
{
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmEnemyHit called, Phase=%d (expect 3=AwaitEnemyHit). Only call from BP at the IMPACT moment (e.g. animation notify), not from OnEnemyAttackStarted/StartAttack."), (int32)Phase);
	if (Phase != EOKTurnPhase::AwaitingEnemyHitConfirm)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitCloud] ConfirmEnemyHit IGNORED: wrong phase=%d (need 3). Call only at the enemy attack impact moment."), (int32)Phase);
		return;
	}

	// Cancel the auto timer so it doesn't fire later and log IGNORED.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoConfirmEnemyHandle);
	}

	ApplyDamageToPlayer(PendingEnemyDamage);
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmEnemyHit applied damage %d to player, PlayerHP now=%d"), PendingEnemyDamage, PlayerHP);

	if (PlayerHP <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmEnemyHit -> Defeat"));
		SetPhase(EOKTurnPhase::Defeat);
		return;
	}

	// Keep input blocked briefly so the enemy can "return" after impact.
	UE_LOG(LogTemp, Log, TEXT("[HitCloud] ConfirmEnemyHit -> EnemyTurn (return pending), EnemyReturnDelay=%.2fs"), EnemyReturnDelaySeconds);
	SetPhase(EOKTurnPhase::EnemyTurn);
	bEnemyReturnPending = true;

	// Fallback: if BP doesn't call FinishEnemyTurn(), we'll finish automatically.
	if (EnemyReturnDelaySeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("[HitCloud] FinishEnemyTurn called immediately (no delay)"));
		FinishEnemyTurn();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnemyReturnHandle);
		World->GetTimerManager().SetTimer(
			EnemyReturnHandle,
			this,
			&UOpeningKnightBattleComponent::FinishEnemyTurn,
			EnemyReturnDelaySeconds,
			false
		);
	}
}

void UOpeningKnightBattleComponent::FinishEnemyTurn()
{
	if (!bEnemyReturnPending) return;
	if (Phase != EOKTurnPhase::EnemyTurn) return;

	bEnemyReturnPending = false;
	ResetHand();
}

void UOpeningKnightBattleComponent::ApplyDamageToEnemy(int32 Damage)
{
	int32 Remaining = Damage;

	if (EnemyShield > 0)
	{
		const int32 Absorb = FMath::Min(EnemyShield, Remaining);
		EnemyShield -= Absorb;
		Remaining -= Absorb;
	}

	if (Remaining > 0)
	{
		EnemyHP = FMath::Max(0, EnemyHP - Remaining);
	}

	BroadcastStats();
}

void UOpeningKnightBattleComponent::ApplyDamageToPlayer(int32 Damage)
{
	int32 Remaining = Damage;

	if (PlayerShield > 0)
	{
		const int32 Absorb = FMath::Min(PlayerShield, Remaining);
		PlayerShield -= Absorb;
		Remaining -= Absorb;
	}

	if (Remaining > 0)
	{
		PlayerHP = FMath::Max(0, PlayerHP - Remaining);
	}

	BroadcastStats();
}

// ===================== Scoring =====================

static int32 BaseStraightScore(int32 Len)
{
	switch (Len)
	{
	case 6: return 1500;
	case 5: return 800;
	case 4: return 600;
	case 3: return 400;
	case 2: return 250;
	default: return 0;
	}
}

bool UOpeningKnightBattleComponent::IsSequential(const TArray<int32>& Seq) const
{
	for (int32 i = 0; i + 1 < Seq.Num(); i++)
	{
		if ((Seq[i + 1] - Seq[i]) != 1) return false;
	}
	return true;
}

void UOpeningKnightBattleComponent::FindBestStraight(const TArray<int32>& Sorted, int32& OutLen, TArray<int32>& OutSeq) const
{
	OutLen = 0;
	OutSeq.Reset();

	for (int32 Len = 2; Len <= 6; Len++)
	{
		for (int32 i = 0; i + Len <= Sorted.Num(); i++)
		{
			TArray<int32> Seq;
			Seq.Reserve(Len);

			for (int32 j = 0; j < Len; j++)
			{
				Seq.Add(Sorted[i + j]);
			}

			if (IsSequential(Seq) && Len > OutLen)
			{
				OutLen = Len;
				OutSeq = Seq;
			}
		}
	}
}

void UOpeningKnightBattleComponent::CalculateBestScore(const TArray<int32>& Values, const TArray<int32>& WildIndices, int32& OutScore, FString& OutHand, FString& OutCalc) const
{
	OutScore = 0;
	OutHand = TEXT("No scoring combination");
	OutCalc = TEXT("0");

	if (WildIndices.Num() == 0)
	{
		CalculateSingleScore(Values, OutScore, OutHand, OutCalc);
		return;
	}

	const int32 WildCount = WildIndices.Num();
	TArray<int32> Test = Values;

	int32 Total = 1;
	for (int32 i = 0; i < WildCount; i++) Total *= 6;

	for (int32 Combo = 0; Combo < Total; Combo++)
	{
		int32 X = Combo;
		for (int32 wi = 0; wi < WildCount; wi++)
		{
			const int32 Digit = (X % 6); // 0..5
			X /= 6;
			Test[WildIndices[wi]] = Digit + 1;
		}

		int32 Score = 0;
		FString Hand, Calc;
		CalculateSingleScore(Test, Score, Hand, Calc);

		if (Score > OutScore)
		{
			OutScore = Score;
			OutHand = Hand;
			OutCalc = Calc;
		}
	}
}

void UOpeningKnightBattleComponent::CalculateSingleScore(const TArray<int32>& Values, int32& OutScore, FString& OutHand, FString& OutCalc) const
{
	OutScore = 0;
	OutHand = TEXT("No scoring combination");
	OutCalc = TEXT("0");

	TMap<int32, int32> C;
	for (int32 V : Values)
	{
		C.FindOrAdd(V)++;
	}

	auto Bonus = [](int32 Pip, int32 Count) -> int32
		{
			return Pip * 200 * Count;
		};

	struct FCand { int32 Score; FString Hand; FString Calc; };
	TArray<FCand> Cand;

	// Straight
	{
		TArray<int32> Sorted = Values;
		Sorted.Sort();

		int32 BestLen = 0;
		TArray<int32> BestSeq;
		FindBestStraight(Sorted, BestLen, BestSeq);

		if (BestLen >= 2)
		{
			const int32 Base = BaseStraightScore(BestLen);
			int32 BonusScore = 0;
			for (int32 V : BestSeq) BonusScore += V * 200;

			const int32 Total = Base + BonusScore;
			Cand.Add({ Total, FString::Printf(TEXT("%d-straight"), BestLen), FString::Printf(TEXT("%d + %d bonus = %d"), Base, BonusScore, Total) });
		}
	}

	// Kinds (best pip for each)
	auto AddKind = [&](int32 Count, int32 Base, const TCHAR* Name)
		{
			int32 BestPip = -1;
			for (const auto& KVP : C)
			{
				if (KVP.Value >= Count)
				{
					BestPip = FMath::Max(BestPip, KVP.Key);
				}
			}

			if (BestPip >= 1)
			{
				const int32 BonusScore = Bonus(BestPip, Count);
				const int32 Total = Base + BonusScore;
				Cand.Add({ Total, Name, FString::Printf(TEXT("%d + %d bonus = %d"), Base, BonusScore, Total) });
			}
		};

	AddKind(6, 1200, TEXT("Six of a Kind"));
	AddKind(5, 1000, TEXT("Five of a Kind"));
	AddKind(4, 800, TEXT("Four of a Kind"));
	AddKind(3, 300, TEXT("Three of a Kind"));
	AddKind(2, 150, TEXT("Pair"));

	// Two Pair
	{
		TArray<int32> Pairs;
		for (const auto& KVP : C)
		{
			if (KVP.Value >= 2) Pairs.Add(KVP.Key);
		}
		Pairs.Sort(TGreater<int32>());

		if (Pairs.Num() >= 2)
		{
			const int32 Base = 700;
			const int32 BonusScore = Bonus(Pairs[0], 2) + Bonus(Pairs[1], 2);
			const int32 Total = Base + BonusScore;
			Cand.Add({ Total, TEXT("Two Pair"), FString::Printf(TEXT("%d + %d bonus = %d"), Base, BonusScore, Total) });
		}
	}

	// Full House
	{
		TArray<int32> Trips;
		TArray<int32> Pairs;

		for (const auto& KVP : C)
		{
			if (KVP.Value >= 3) Trips.Add(KVP.Key);
			if (KVP.Value >= 2) Pairs.Add(KVP.Key);
		}

		Trips.Sort(TGreater<int32>());
		Pairs.Sort(TGreater<int32>());

		if (Trips.Num() >= 1)
		{
			const int32 Three = Trips[0];

			int32 Pair = -1;
			for (int32 P : Pairs)
			{
				if (P != Three) { Pair = P; break; }
			}

			if (Pair >= 1)
			{
				const int32 Base = 1000;
				const int32 BonusScore = Bonus(Three, 3) + Bonus(Pair, 2);
				const int32 Total = Base + BonusScore;
				Cand.Add({ Total, TEXT("Full House"), FString::Printf(TEXT("%d + %d bonus = %d"), Base, BonusScore, Total) });
			}
		}
	}

	// choose best
	for (const FCand& It : Cand)
	{
		if (It.Score > OutScore)
		{
			OutScore = It.Score;
			OutHand = It.Hand;
			OutCalc = It.Calc;
		}
	}
}


