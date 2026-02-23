# Knight (Player) Attack Fix – Step-by-Step

This guide walks you through fixing the knight attack so it works correctly on **every** attack (not just the first): hitbox off at start, on only at impact, off again at end, and any one-shot logic reset so the 2nd and 3rd attacks behave the same as the 1st.

---

## What’s in C++ (already done)

Three Blueprint-callable functions are available on **OpeningKnightPlayerController** (and thus on **BP_OpeningKnightPlayerController**):

| Function | When to call from BP_Knight | Effect |
|----------|----------------------------|--------|
| **Get Hit Cloud In World** | In **Event Begin Play** (once) | Returns the first BP_HitCloudFX in the level; assign to **HitCloudRef** so impact chain works in PIE. |
| **Notify Player Attack Hitbox Start** | At the **start** of every knight attack | Sets the given hitbox’s collision to **No Collision** (so it can’t overlap the enemy until impact). |
| **Notify Player Attack Hitbox End** | When the knight attack **ends** (e.g. timeline **Finished**) | Sets the given hitbox’s collision to **No Collision** again (clean state for the next attack). |

You pass the knight’s **attack hitbox component** (the primitive that overlaps the enemy). If you call these from Blueprint at the right times, the “hitbox off at start/end” behavior is guaranteed in code and works on every attack.

The **impact** moment (hitbox on, cloud position, Flash, ConfirmPlayerHit) stays in Blueprint because it involves your HitCloud actor and timing with the timeline/notify.

---

## Step-by-step in the editor (BP_Knight and event bindings)

### Step 1: Identify the knight’s attack hitbox

1. Open **BP_Knight** (your knight Actor in the level, tagged `"Knight"`).
2. Find the **component** that is used to detect overlap with the enemy (e.g. a **Box Collision** or **Sphere** on the weapon or hand). This is the “attack hitbox.”
3. Note its name (e.g. `AttackHitbox`, `WeaponOverlap`). You will pass this component into the C++ functions and use it for **Set Collision Enabled** at impact.

If you don’t have a dedicated attack hitbox yet, add a **Box Collision** (or similar) as a child of the mesh bone that should represent the hit, and use that.

---

### Step 2: Where the knight attack “starts”

The knight’s attack is started when the player clicks **Play** and the hand is valid. In C++, that triggers **On Player Attack Started** on the Battle component (with damage and debug text). Something in your project (Level Blueprint, or BP_Knight itself) listens to that and starts the knight’s attack (timeline or animation).

Find that logic:

- If you use **Level Blueprint**: find the node that binds to **Battle → On Player Attack Started** (or similar) and then starts the knight’s attack (e.g. **Play** or **Play from Start** on a timeline, or **Play Animation**).
- If you use **BP_Knight**: you might have an **Event Begin Play** that gets the Battle component (e.g. from the Player Controller) and binds to **On Player Attack Started**, then plays the attack timeline.

Wherever the attack is **started** (the first node that runs when “player pressed Play” and the knight begins his attack), you will add the “hitbox off” call **as the very first action**.

---

### Step 3: At attack start – disable the hitbox (every time)

Right at the start of the knight attack (before the timeline plays or the anim starts), do **one** of the following.

**Option A – Use C++ (recommended)**  
So that hitbox-off is guaranteed and consistent on every attack:

1. In that same graph where you start the attack, **before** you **Play** the timeline (or play the anim):
   - **Get Player Controller** → cast to **BP_OpeningKnightPlayerController** (or **OpeningKnightPlayerController**).
   - Call **Notify Player Attack Hitbox Start**.
   - In the **Hitbox** pin, pass the knight’s **attack hitbox component** (e.g. drag the component from the Components panel, or use **Get Component by Class** / **Get Component by Name** if you have a reference).
2. Then continue with your existing “Play timeline” or “Play animation” logic.

So the order is: **Attack start event** → **Notify Player Attack Hitbox Start (Hitbox)** → **Play timeline / Play animation**.

**Option B – Pure Blueprint**  
If you prefer not to use the C++ helpers:

1. In that same “attack start” path, **before** playing the timeline/anim:
   - **Set Collision Enabled**.
   - **Target** = the knight’s attack hitbox component.
   - **New Type** = **No Collision**.
2. Then **Play** the timeline or animation.

Do this in the **same** event path that runs for **every** Play (1st, 2nd, 3rd attack). Do not gate it with a “first attack only” branch.

---

### Step 4: At impact – enable hitbox, move cloud, flash, confirm

Use a **Timeline Event** (or **Animation Notify**) at the frame where the knight’s hit **visually lands** on the enemy.

In that event/notify, do **in this order**:

1. **Set Collision Enabled**  
   - Target = knight’s attack hitbox.  
   - New Type = **Query Only** (so overlap events can fire).

2. **Get Component Location**  
   - Target = same attack hitbox.  
   - You’ll use the **Return Value** (vector) as the cloud position.

3. **Set Actor Location**  
   - Target = the **HitCloudRef** actor (your hit cloud).  
   - **New Location** = the vector from step 2.

4. **Flash**  
   - Call **Flash** on the HitCloudRef (or whatever makes the cloud appear).

5. **Confirm Player Hit**  
   - **Get Player Controller** → **Confirm Player Hit** (the existing Blueprint-callable on the player controller).  
   - Or: if you use **On Component Begin Overlap** on the knight’s hitbox to call **Confirm Player Hit**, you can skip calling it here; the overlap will fire when the enemy is inside the hitbox. In that case the impact event only needs steps 1–4.

So: **Impact event** → **Set Collision Enabled (Query Only)** → **Set Actor Location (HitCloudRef)** → **Flash** → (optionally) **Confirm Player Hit**.

This must run **every** time the timeline reaches the impact event. If you use a **DoOnce** here, you **must** reset it when the attack ends (see Step 5).

---

### Step 5: When the knight attack ends – hitbox off and reset one-shots

When the knight’s attack **ends** (e.g. the attack timeline’s **Finished** pin, or an “attack end” anim notify), do the following **every time**:

1. **Disable the hitbox again**  
   - **Option A (C++)**: **Get Player Controller** → **Notify Player Attack Hitbox End** and pass the knight’s attack hitbox.  
   - **Option B (Blueprint)**: **Set Collision Enabled** on the attack hitbox → **No Collision**.

2. **Reset any one-shot logic**  
   - If you use a **DoOnce** (or similar) for the impact/Confirm Player Hit/cloud logic, call its **Reset** pin from this “attack end” path.  
   - If you have a boolean like “has applied damage this attack” or “has confirmed this attack,” set it back to **False** here.

This way the **next** attack (2nd, 3rd, …) will again run impact and confirm logic.

---

### Step 6: Don’t confirm at attack start

Make sure **Confirm Player Hit** is **never** called when the attack **starts** (e.g. from **On Player Attack Started** or the first frame of the timeline). It should only be called at **impact** (from the timeline event or from overlap when the hitbox is enabled at impact).

---

## Quick checklist

| Step | Where | What to do |
|------|--------|------------|
| 1 | BP_Knight | Identify the attack hitbox component. |
| 2 | Level BP or BP_Knight | Find where the knight attack **starts** (bound to On Player Attack Started → Play timeline/anim). |
| 3 | That “attack start” path | **First** action: **Notify Player Attack Hitbox Start(Hitbox)** (C++) or **Set Collision Enabled → No Collision** (BP). Then start timeline/anim. |
| 4 | Timeline/anim **impact** event | Set hitbox **Query Only** → Set Actor Location (HitCloudRef) → Flash → Confirm Player Hit (or let overlap do it). |
| 5 | Timeline **Finished** (attack end) | **Notify Player Attack Hitbox End(Hitbox)** (C++) or **Set Collision Enabled → No Collision**; **Reset** DoOnce / clear “has confirmed” flags. |
| 6 | Everywhere | Never call Confirm Player Hit at attack start; only at impact. |

---

## Summary: what’s in C++ vs Blueprint

- **C++**  
  - **Notify Player Attack Hitbox Start** and **Notify Player Attack Hitbox End** on the Player Controller: they take the knight’s attack hitbox and set its collision to **No Collision**. Call them from Blueprint at attack start and attack end so every attack starts and ends with the hitbox disabled.

- **Blueprint**  
  - **When** the attack starts/ends (binding to On Player Attack Started, timeline start/Finished).  
  - **At impact**: enable hitbox (Query Only), set HitCloud position, Flash, and call Confirm Player Hit (or rely on overlap).  
  - **Reset** any DoOnce or “has confirmed this attack” when the attack ends.

If you follow the steps above and use the C++ helpers at start and end, the “first works, 2nd/3rd don’t” issue from the hitbox staying on or one-shots not resetting should be resolved.
