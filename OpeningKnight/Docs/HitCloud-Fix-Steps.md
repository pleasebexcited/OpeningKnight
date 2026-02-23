# HitCloud / Confirm Hit – Fix Steps

Use these steps so damage and health bars sync with your attack **impact** (cloud) moment and you don’t get duplicate or early confirms.

---

## 0. C++ order fix (already done)

**ResolveEnemyTurn** now calls **StartAttack** on the enemy **before** setting phase to AwaitingEnemyHitConfirm and before broadcasting. So the enemy’s Attack HitBox is disabled at the start of the attack (in StartAttack) before the battle is “waiting” for confirm. That stops the early ConfirmEnemyHit from overlap at attack start. When you add the **animation notify at impact** that re-enables the hitbox (or calls ConfirmEnemyHit directly), damage will sync with the hit. The auto ConfirmEnemyHit timer is also cleared when ConfirmEnemyHit runs, so you won’t see a second IGNORED log.

---

## 1. Rely on Blueprint for player impact (no duplicate ConfirmPlayerHit)

- **In Editor:** Select the actor that has **OpeningKnightBattleComponent** (e.g. your PlayerController or GameMode).
- In Details, under **OpeningKnight | Tuning** set:
  - **Auto Confirm Player Hit Delay Seconds** = **0**
- **In Blueprint:** Keep calling **Confirm Player Hit** at the **player attack impact** moment (e.g. from the knight’s attack animation notify when the hit lands).  
- Result: Only one call to `ConfirmPlayerHit` (from BP at impact). No second call from the timer.

*(If you prefer the timer as fallback, leave this value > 0; the second call will be ignored by C++ but you’ll still see a log.)*

---

## 2. Call ConfirmEnemyHit only at enemy impact (not at attack start)

Right now `ConfirmEnemyHit` is being triggered when the enemy attack **starts** (e.g. from `OnEnemyAttackStartedWithKnight` or from the enemy’s `StartAttack`). It must run only when the hit **lands**.

### 2a. Remove the early call

- Open the Blueprint that currently calls **Confirm Enemy Hit** (e.g. **BP_OpeningKnightGameMode**, **BP_OpeningKnightPlayerController**, or **BP_Enemy**).
- Find any of these and **remove** the call to **Confirm Enemy Hit** from:
  - Event bound to **On Enemy Attack Started With Knight**
  - Event bound to **On Enemy Attack Started**
  - Inside the enemy’s **Start Attack** implementation
- Save the Blueprint.

### 2b. Call ConfirmEnemyHit at the impact moment

- Open the **enemy attack animation** (the one that plays when the enemy hits the knight).
- Add an **Animation Notify** (or Notify State) at the frame where the hit **visually lands** (impact/cloud moment).
- In that notify’s Blueprint (or in a Blueprint that listens for it):
  - Get the **Player Controller** (or the actor that has the Battle component).
  - Call **Confirm Enemy Hit** on it (e.g. `Get Player Controller` → `Confirm Enemy Hit` on your player controller that exposes it).
- Save.

If your **Player Controller** already has a **Confirm Enemy Hit** BlueprintCallable, the notify just needs to call that (e.g. from the GameMode or from the enemy by getting the player controller and calling the function).

### 2c. Optional: auto timer as fallback

- On the **OpeningKnightBattleComponent**, under **OpeningKnight | Tuning**:
  - **Auto Confirm Enemy Hit Delay Seconds** = **0** if you always call from the animation notify (recommended). With 0, C++ uses a **2 second safety fallback** so the game never stalls; once you call ConfirmEnemyHit from Blueprint at impact, that timer is cleared.
  - Set to something like **0.35** if you want a shorter fallback; damage will then apply after that delay if the notify doesn’t fire.

### 2d. Disable hitbox again when the attack ends

- In **BP_Enemy**, when the attack finishes (e.g. when returning to idle, or when `bIsAttacking` is set false), call **Set Collision Enabled** on **Attack Hit Box** with **No Collision**. That way the hitbox is not left enabled between attacks and won’t overlap the knight when the next turn starts.

---

## 3. Quick checklist

| Step | Where | Action |
|------|--------|--------|
| 1 | Battle component (Details) | Set **Auto Confirm Player Hit Delay Seconds** = **0** (if you use BP at impact). |
| 2a | GameMode / Controller / Enemy BP | **Remove** **Confirm Enemy Hit** from **On Enemy Attack Started** / **Start Attack**. |
| 2b | Enemy attack animation | Add **Notify** at impact frame; in BP call **Confirm Enemy Hit** (e.g. via Player Controller). |
| 2c | Battle component (Details) | Leave **Auto Confirm Enemy Hit Delay Seconds** = **0** (or set > 0 only as fallback). |

After this, you should see in the logs only one **ConfirmPlayerHit** (from BP) and one **ConfirmEnemyHit** (from your impact notify), with no “IGNORED” duplicate messages.
