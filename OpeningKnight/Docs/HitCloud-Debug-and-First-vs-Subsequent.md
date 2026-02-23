# HitCloud Debug Logging and First vs Subsequent Attack

This doc explains how to add logs to see why the knight’s HitCloud is correct on the **first** attack but spawns at the knight’s **starting point** on 2nd/3rd attacks, and how that compares to the enemy (which was fixed with the same chain). It also covers timeline (TL) and state causes.

---

## 1. C++ debug log (use from both Blueprints)

A Blueprint-callable function was added on **OpeningKnightPlayerController** (and thus **BP_OpeningKnightPlayerController**):

- **Log Hit Cloud Impact**  
  - **Source Name** (String): e.g. `"Knight"` or `"Enemy"`  
  - **Hitbox World Location** (Vector): from **Get Component Location** of the attack hitbox  
  - **Hit Cloud Actor Location** (Vector): from **Get Actor Location** of the HitCloudRef  

It writes one line to the Output Log, for example:

```text
[HitCloud] Knight impact: AttackHitBox world=(x, y, z)  HitCloudRef location=(x, y, z)
```

Rebuild the project so this function is available in Blueprint.

---

## 2. Where to call the log in Blueprint

### BP_Enemy (reference – already correct)

- In the **Event Graph**, find the **TL_AttackDash** timeline and the **HitCloudEvent** exec output.
- The chain is: **HitCloudEvent** → Set Collision Enabled (AttackHitBox, Query Only) → Set Actor Location (HitCloudRef, Get Component Location(AttackHitBox)) → Flash (HitCloudRef).
- Add the log at the **start** of this chain (right after HitCloudEvent):
  1. **Get Player Controller** (0) → **Cast to BP_OpeningKnightPlayerController**.
  2. **Get Component Location** (Target = **AttackHitBox**) → **Return Value**.
  3. **Get Actor Location** (Target = **HitCloudRef**).
  4. Call **Log Hit Cloud Impact** on the cast result: **Source Name** = `Enemy`, **Hitbox World Location** = step 2, **Hit Cloud Actor Location** = step 3.

So you log **before** Set Actor Location. That shows where the enemy’s hitbox is and where the cloud currently is each time HitCloudEvent fires.

### BP_Knight (where the bug is)

- In the **Event Graph**, find the **TL_AttackDash** timeline and the **KnightHitBc** exec output (the impact event).
- Add the **same** log at the **start** of the impact chain (first thing when **KnightHitBc** fires):
  1. **Get Player Controller** (0) → **Cast to BP_OpeningKnightPlayerController**.
  2. **Get Component Location** (Target = the knight’s **attack hitbox**) → **Return Value**.
  3. **Get Actor Location** (Target = the knight’s **HitCloudRef**).
  4. Call **Log Hit Cloud Impact**: **Source Name** = `Knight`, **Hitbox World Location** = step 2, **Hit Cloud Actor Location** = step 3.

Then run PIE and do **at least two knight attacks**. In the log:

- **First attack:** You should see one `[HitCloud] Knight impact: ...` line. Note **AttackHitBox world** (should be near the impact point) and **HitCloudRef location**.
- **Second attack:** See if you get another `[HitCloud] Knight impact: ...` at all.
  - If you **do not** get a second line → **KnightHitBc is not firing on the 2nd attack** (timeline or DoOnce).
  - If you **do** get a second line but **AttackHitBox world** is at the knight’s starting position → the hitbox component’s world position is wrong when the event fires on 2nd attack.
  - If you get a second line and **AttackHitBox world** is correct but the cloud still appears at start → something after the log (e.g. Set Actor Location) isn’t running or is overwritten.

---

## 3. First attack vs subsequent attacks – what’s different

We know:

- **First knight attack:** HitCloud appears at the correct (impact) position.
- **Second and later:** HitCloud appears at the **knight’s starting point** (same symptom the enemy had before the fix).
- The issue started **after** adding the enemy’s “location from hitbox at impact” chain (Set Actor Location from AttackHitBox, then Flash).

So the difference is not “first vs later” in C++ (ConfirmPlayerHit runs every time); it’s in **when** and **whether** the knight’s impact chain runs, and **what location** is used.

### Why the first attack can look correct

- On the **first** play, the knight’s **TL_AttackDash** runs from start to finish and **KnightHitBc** fires.
- The impact chain runs: hitbox location is read at impact, HitCloudRef is moved there, Flash runs.
- So the cloud is at the right place once.

### Why the second/third go wrong (likely causes)

1. **KnightHitBc doesn’t run again (most likely)**  
   - Something prevents the **KnightHitBc** event from executing on the 2nd attack:
     - **DoOnce** (or similar) around the impact chain: it runs once, then blocks on later attacks.
     - **Timeline not restarted from 0:** If the knight uses **Play** instead of **PlayFromStart** when starting the next attack, the timeline may not run from 0 again, so the event track might not fire or might fire at the wrong time.
   - If the impact chain doesn’t run, the cloud never gets moved to the hitbox at impact. If something else (e.g. at attack start) sets the cloud to the knight’s position, you’ll see the cloud at the knight’s **starting** position on 2nd/3rd attack.

2. **Timeline (TL) – Play vs PlayFromStart**  
   - **BP_Enemy** uses **PlayFromStart** for **TL_AttackDash** when the enemy attack begins (from the export data: `PlayFromStart` is wired). So every attack the timeline goes 0 → 1 and **HitCloudEvent** fires at 1.0.
   - **BP_Knight** must do the same: when the player attack starts (e.g. from **On Player Attack Started**), the node that starts the knight’s attack timeline must be **PlayFromStart**, not **Play**. Otherwise the second time the timeline might not run from 0 and **KnightHitBc** may not fire again.

3. **HitCloud position set at attack start**  
   - If at the **start** of each knight attack you set HitCloudRef’s location to the knight (e.g. knight root or start position), and the **impact** chain (Set Actor Location from hitbox, Flash) does **not** run on 2nd/3rd attack, the cloud will stay at that start position. That matches “cloud at knight’s starting point on subsequent attacks.”

4. **DoOnce / “has applied damage” not reset**  
   - If a DoOnce (or a boolean) gates the whole impact chain (Set Collision → Set Actor Location → Flash → ConfirmPlayerHit), and it is **never reset** when the knight’s attack **ends** (e.g. **TL_AttackDash** **Finished**), then only the first attack runs the chain. Fix: remove the DoOnce from the impact chain, or reset it from the timeline **Finished** pin.

---

## 4. Checklist – what to verify in BP_Knight

| # | What to check | Where | Why it matters |
|---|----------------|--------|-----------------|
| 1 | Timeline start | Where you start the knight attack (e.g. On Player Attack Started) | Use **PlayFromStart** for **TL_AttackDash**, not **Play**, so every attack runs 0→1 and **KnightHitBc** fires. |
| 2 | Impact chain | **KnightHitBc** exec output | First nodes: Set Collision Enabled (hitbox, Query Only) → Get Component Location (hitbox) → Set Actor Location (HitCloudRef, that location) → Flash → Confirm Player Hit. No **DoOnce** around this. |
| 3 | DoOnce / flags | Any DoOnce or “has confirmed this attack” used for impact | If present, they must be **reset** when the knight attack **ends** (e.g. **TL_AttackDash** **Finished**). Otherwise 2nd/3rd impact chain won’t run. |
| 4 | Attack start | First thing when knight attack starts | Don’t set HitCloudRef location to the knight at attack start (or if you do, the impact chain must still run and overwrite it every time). Prefer: only set cloud position at **KnightHitBc** from hitbox location. |
| 5 | Attack end | **TL_AttackDash** **Finished** | Set attack hitbox to **No Collision**; reset any DoOnce used for impact. |

---

## 5. Optional: Print String in Blueprint (instead of or in addition to C++)

If you prefer not to use the C++ log:

- In **BP_Knight**, at the very start of the **KnightHitBc** chain, add **Print String**.
  - In the string, use **Append** to include e.g. **Get Component Location** of the attack hitbox (convert vector to string) so you see the position in the on-screen log.
- In **BP_Enemy**, at the start of the **HitCloudEvent** chain, add a **Print String** (e.g. “Enemy HitCloudEvent”).
- Run two knight attacks. If you see the knight’s Print String only on the first attack, **KnightHitBc** is not firing on the second (timeline or DoOnce).

---

## 6. Summary

- **Logging:** Use **Log Hit Cloud Impact** from both **BP_Knight** (KnightHitBc) and **BP_Enemy** (HitCloudEvent) at the start of the impact chain to compare hitbox and HitCloud positions on 1st vs 2nd/3rd attack.
- **First attack works** because the impact chain runs once; **subsequent fail** because either the impact chain doesn’t run again (DoOnce or timeline not restarted with **PlayFromStart**) or the location used is wrong.
- **Timeline:** Use **PlayFromStart** for the knight’s **TL_AttackDash** on every attack so **KnightHitBc** fires every time.
- **DoOnce:** Don’t gate the impact chain with a DoOnce, or reset it when the knight’s attack ends (timeline **Finished**).

After adding the logs and checking the checklist, the log output will tell you exactly whether the problem is “KnightHitBc not firing on 2nd attack” or “firing but with wrong location.”
