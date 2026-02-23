# Knight (Player) Attack – HitCloud Fix

Apply the **same pattern** you used for the enemy in **BP_Enemy** to the **knight (player)** in **BP_Knight** so the player attack hits at impact, one cloud, correct position.

**Debugging first vs subsequent attacks:** See **[HitCloud-Debug-and-First-vs-Subsequent.md](HitCloud-Debug-and-First-vs-Subsequent.md)** for how to add logs (C++ **Log Hit Cloud Impact** or Print String) in BP_Knight and BP_Enemy, why the first attack works and 2nd/3rd don’t, and a checklist (timeline **PlayFromStart**, DoOnce reset, etc.).

---

## Fix: “Accessed None” / HitCloudRef is None in PIE

If you see **"Accessed None trying to read property K2Node_DynamicCast_AsBP_Hit_Cloud_FX"** at **Set Actor Location** in **BP_Knight** when playing in editor (PIE), **HitCloudRef** is **None** at runtime. The Blueprint default was pointing at a level instance (`mainMenu:PersistentLevel.BP_HitCloudFX_C_1`), which is an illegal cross-package reference in PIE, so it doesn’t survive. Fix it by **setting HitCloudRef at runtime** and (optionally) clearing the bad default.

### Step 1: Enable Event Begin Play in BP_Knight

- Open **BP_Knight** → **Event Graph**.
- Find **Event Begin Play**. If it is **disabled** (greyed out, “This node is disabled and will not be called”):
  - **Right‑click** the node → **Enable** (or enable it in the Details panel).
- You will use this event only to set **HitCloudRef**; the rest of your logic can stay as is.

### Step 2: Set HitCloudRef in Begin Play (recommended: C++ helper)

Use the Player Controller’s **Get Hit Cloud In World** so the Knight always gets the HitCloud actor from the current world (works in PIE and in packaged game):

1. From **Event Begin Play** → **Get Player Controller** → **Cast to OpeningKnightPlayerController** (or **BP_OpeningKnightPlayerController**).
2. Call **Get Hit Cloud In World**.
   - **World Context Object**: pass **self** (the Knight actor). This uses the correct world (e.g. PIE world).
3. **Set** your **HitCloudRef** variable to the **Return Value** of **Get Hit Cloud In World**.

Optional: add a **Branch** on **Is Valid** (Return Value) and only **Set HitCloudRef** when valid; you can log a warning on the **False** branch.

### Alternative: Pure Blueprint (no C++)

1. From **Event Begin Play**: **Get All Actors of Class** → Class = **BP_HitCloudFX** (select the Blueprint class).
2. **Branch**: condition = **Length** of the returned array **≥ 1** (or **Is Valid** on **Get (0)**).
3. On **True**: **Get** index **0** from the array → **Set HitCloudRef** to that actor.

### Step 3: Clear the bad default (recommended)

- In **BP_Knight** → **Variables** → select **HitCloudRef**.
- In **Details**, clear the **Default Value** (remove the reference to the level instance).  
  This removes the “Illegal TEXT reference to a private object in external package” warning and avoids confusion; **HitCloudRef** will be set only in **Begin Play**.

### Step 4: Rebuild and test

- **Compile** and **Save** BP_Knight.
- **Play in Editor** (PIE). **HitCloudRef** should be set in Begin Play; the impact chain (Set Actor Location → Flash) should no longer hit “Accessed None,” and the HitCloud should appear at the knight’s impact on every attack.

---

## Simple solution (same as BP_Enemy: add the location)

We know **the first hit works**. So the 2nd and 3rd hits go wrong because something that runs on the first impact **isn’t run again** (or isn’t reset) for later attacks. What fixed the enemy was **setting the HitCloud’s location from the hitbox at impact**. Do the same for the knight, and make sure it runs **every** impact, not only the first.

**In BP_Knight, at the knight’s impact moment** (the timeline event or animation notify where the hit visually lands):

1. **Get Component Location** → target = the knight’s **attack hitbox** (the component that overlaps the enemy). Take the **Return Value** (vector).
2. **Set Actor Location** → target = the knight’s **HitCloudRef**, **New Location** = that vector.
3. **Flash** → target = **HitCloudRef**.
4. **Confirm Player Hit** → Get Player Controller → **Confirm Player Hit** (if you don’t call it from overlap).

So: **Impact event** → **Set Actor Location (HitCloudRef, hitbox location)** → **Flash** → **Confirm Player Hit**.

Do this **every** time the impact event fires — **no DoOnce** around this chain. If you have a DoOnce (or “has confirmed this attack” flag) that only lets this run once, the 2nd/3rd impact won’t get location + Flash + Confirm, which matches “works on first hit, not later.” So: no DoOnce on impact logic, or reset that DoOnce when the knight’s attack **ends** (timeline Finished). The rest of this doc (hitbox off at start/end) is for overlap/confirm firing at the wrong time; if damage and confirm already work every time, the fix is **run location → Flash → Confirm every impact**, like the first hit.

---

## 1. At attack start: hitbox OFF

- When the player attack **starts** (e.g. from **On Player Attack Started** or when your knight attack timeline/anim begins):
  - Call **Set Collision Enabled** on the knight’s **attack hitbox** (the component that overlaps the enemy) and set it to **No Collision**.
- So at the very start of the attack, the hitbox is disabled and cannot overlap the enemy yet.

---

## 2. At impact only: hitbox ON + cloud position + Flash + ConfirmPlayerHit

- Use a **Timeline Event** or **Animation Notify** at the frame where the knight’s hit **visually lands** (impact).
- From that event/notify, in this order:

| Order | Node | What to do |
|-------|------|------------|
| 1 | **Set Collision Enabled** | Target = knight’s attack hitbox, New Type = **Query Only** |
| 2 | **Get Component Location** | Target = same attack hitbox → get **Return Value** (Vector) |
| 3 | **Set Actor Location** | Target = knight’s **HitCloudRef**, **New Location** = Get Component Location’s Return Value |
| 4 | **Flash** | Target = **HitCloudRef** (cloud appears at impact position) |
| 5 | **Confirm Player Hit** | Get Player Controller → call **Confirm Player Hit** (or let **OnComponentBeginOverlap** on the hitbox call it when enemy is overlapped) |

So the **exec chain** is:  
**Impact Event** → **Set Collision Enabled (Query Only)** → **Set Actor Location (HitCloudRef, location from hitbox)** → **Flash (HitCloudRef)** → (optionally) **Confirm Player Hit** if you don’t use overlap.

If you use **OnComponentBeginOverlap** on the knight’s hitbox to call **Confirm Player Hit**, then the impact event only needs: Set Collision Enabled → Set Actor Location → Flash. Overlap will fire when the enemy is inside the hitbox and your existing overlap logic will call Confirm Player Hit.

---

## 3. After attack: hitbox OFF again

- When the knight’s attack **ends** (timeline finished or anim done), call **Set Collision Enabled** on the knight’s attack hitbox with **No Collision** so it isn’t left on for the next turn.

---

## 4. Remove any early Confirm Player Hit

- **Do not** call **Confirm Player Hit** when the attack **starts** (e.g. from **On Player Attack Started** or at the beginning of the knight’s attack timeline).
- Call it only at **impact** (from the timeline event or anim notify above), or from **OnComponentBeginOverlap** when the hitbox is enabled at impact.

---

## 5. Optional: timer fallback

- If **Auto Confirm Player Hit Delay Seconds** = **0** on the Battle component, you **must** call **Confirm Player Hit** from Blueprint at impact or the game will wait forever.
- Set it to a small value (e.g. **0.5**) if you want a fallback until the knight BP is fully wired.

---

---

## 6. Fix for subsequent attacks (first works, 2nd/3rd don’t)

If the **first** knight attack works but **later** attacks misbehave (e.g. hit/cloud too early, or no hit/cloud), do the following in **BP_Knight**:

### 6.1 Hitbox OFF at the start of **every** attack

- In the logic that runs when the player attack **starts** (e.g. **On Player Attack Started**, or the **Play** / **PlayFromStart** of the knight’s attack timeline), the **first** thing must be:
  - **Set Collision Enabled** on the knight’s attack hitbox → **No Collision**.
- This must run on **every** attack (1st, 2nd, 3rd), not only the first. Otherwise the hitbox can stay enabled from the previous attack and trigger overlap/ConfirmPlayerHit immediately when the next attack begins.

### 6.2 Reset one-shot logic when the attack ends

- If you use a **DoOnce** (or similar) for impact / Confirm Player Hit / cloud, it will only run **once** unless you reset it.
- When the knight’s attack **ends** (e.g. attack timeline **Finished**), do **both**:
  1. **Set Collision Enabled** on the knight’s attack hitbox → **No Collision** (same as §3).
  2. **Reset** any **DoOnce** nodes used for this attack (use the **Reset** exec pin), and set any “has applied damage this attack”–style boolean back to **False**.
- That way the next attack can run the same impact/confirm/cloud logic again.

### Checklist (subsequent attacks)

| Step | Where in BP_Knight | Action |
|------|--------------------|--------|
| A | Attack **start** (On Player Attack Started / timeline start) | Set attack hitbox → **No Collision** (first node in the chain). |
| B | Attack **end** (timeline **Finished**) | Set attack hitbox → **No Collision**; **DoOnce Reset**; clear any “has confirmed this attack” flag. |

---

## Summary (mirror of BP_Enemy)

| When | In BP_Knight |
|------|----------------|
| **Attack start** | Set knight attack hitbox → **No Collision** (every attack) |
| **Impact (Timeline Event or Anim Notify)** | Set hitbox → **Query Only**; Set Actor Location (HitCloudRef, hitbox location); Flash (HitCloudRef); Confirm Player Hit (from overlap or direct call) |
| **Attack end** | Set knight attack hitbox → **No Collision**; reset DoOnce / one-shot flags |
