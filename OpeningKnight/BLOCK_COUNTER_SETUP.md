# Block/Counter System — Blueprint Setup

The C++ implementation is complete.

## Fix: "Battle" missing / Accessed None (mainMenu, hot reload)

If you see *"Attempted to access missing property 'Battle'"* or *"Accessed None trying to read property Battle"* when binding to `OnEnemyAttackStartedWithKnight`:

1. Open **mainMenu** level Blueprint.
2. Find the **Bind Event to On Enemy Attack Started with Knight** node.
3. Change the **Target** input: instead of pulling from the **Battle** variable, use **Get Battle** (call the function on the Player Controller).
4. Add a **Branch** before the bind: call **Get Battle**, check **IsValid**; only bind if valid.
5. Restart the editor (avoid hot reload) if the error persists. Follow these steps to finish wiring in the editor.

## 1. Btn_Block in WBP_Play_BillHUD

- Open **WBP_Play_BillHUD**
- Select **Btn_Block**
- In the Details panel, under Events: **On Clicked**
- Add: **Get Owning Player** → **Cast to OpeningKnight Player Controller** → **Block Or Counter**

## 2. WBP_BlockMinigame (create Blueprint)

1. **Create widget**: Right‑click in Content Browser → User Interface → Widget Blueprint.
2. **Parent class**: Choose **Block Minigame Widget** (our C++ class).
3. **Name**: `WBP_BlockMinigame`.

### Fix: RootCanvas has no Slot/Anchors

The Blueprint root (Block Minigame Widget) doesn't provide Canvas Panel slots, so `RootCanvas` gets no Anchors. Fix:

1. In Hierarchy, add a **Canvas Panel** as a child of the root.
2. Name it `LayoutCanvas` (or leave default).
3. **Drag `RootCanvas`** so it becomes a child of this new Canvas Panel (not direct child of root).
4. Select `RootCanvas` — the Details panel will now show **Slot (Canvas Panel Slot)** and **Anchors**.
5. Set Anchors to stretch (e.g. fill) and position as needed.

C++ still binds `RootCanvas` by name — no code changes.

### Layout (triangle: 2 top, 1 bottom)

- **Root** → **LayoutCanvas** (Canvas Panel, full screen) → **RootCanvas** (your main canvas).
- Add a **Vertical Box** (or Overlay) centered on the canvas.
- Inside, add:
  - **Horizontal Box** (top row): 2 **Button** widgets, each with a child **Image** for the dice texture.
  - **Horizontal Box** (bottom row, centered): 1 **Button** with child **Image**.

### Required widget names (for C++ BindWidgetOptional)

| Widget Name | Type |
|-------------|------|
| BlockDice0 | Image (child of first button, top-left) |
| BlockDice1 | Image (child of second button, top-right) |
| BlockDice2 | Image (child of third button, bottom) |
| ImgBlocked | Image (BLOCKED.png) |
| ImgCountered | Image (COUNTERED.png) |
| ImgFailed | Image (FAILED.png) |

- Make **ImgBlocked**, **ImgCountered**, **ImgFailed** initially Collapsed.
- Assign your PNG textures (BLOCKED, COUNTERED, FAILED) to their brushes.

### Button click bindings

- For each of the 3 dice **Buttons**, On Clicked → call **On Block Dice Clicked By Index** with:
  - Top-left button: **0**
  - Top-right button: **1**
  - Bottom button: **2**

### Alternative: bind by value

If you prefer to bind by die value instead of index, use **On Block Dice Clicked** and pass the current value of that die. The widget will use `CurrentDiceValues`; by-index is simpler.

## 3. BP_OpeningKnightPlayerController

- Open **BP_OpeningKnightPlayerController**
- **Block Minigame Widget Class**: Set to `WBP_BlockMinigame`
- **Block Minigame Z Order**: 400 (below curtains if used)

## 4. Enemy freeze / unfreeze / return to position

The battle component broadcasts:

- **On Block Minigame Freeze Enemy** — when Block is activated
- **On Block Minigame Unfreeze Enemy** — when the block round **fails** (enemy resumes its attack)
- **On Block Minigame Enemy Return To Position** — when the block round **succeeds** (enemy must return to start before knight counter-attacks)

C++ also calls **ReturnToPosition** directly on the enemy when block succeeds. Add this function in **BP_Enemy** for the enemy to snap back.

Bind in **Game Mode** or **Level Blueprint**:

1. **On Block Minigame Freeze Enemy** → Get Enemy → call **Freeze For Block** (pause timeline).
2. **On Block Minigame Unfreeze Enemy** → Get Enemy → call **Unfreeze From Block** (resume timeline). Only when block fails.
3. **On Block Minigame Enemy Return To Position** — optional; C++ already calls **ReturnToPosition** on the enemy. Bind only if you need extra logic.

**BP_Enemy functions to add:**

- **Freeze For Block**: Pause `TL_AttackDash` (or set play rate to 0).
- **Unfreeze From Block**: Resume the timeline (set play rate back).
- **Return To Position** (Blueprint Callable): Called when block succeeds. Stop the attack, snap enemy back to start, reset attack state. See below.

### Return To Position implementation (BP_Enemy)

When the knight successfully blocks, the enemy must return to its starting position **immediately** and wait for the knight’s counter — it must not finish its attack.

1. **My Blueprint** → **Functions** → **+** → name **ReturnToPosition**.
2. In the **ReturnToPosition** graph:
   - **Stop** the `TL_AttackDash` timeline (Stop, or set Play Rate to 0 and stop).
   - **Set Actor Location** (self) = **AttackHomeLocation** (snap back to where the attack started).
   - **Set bIsAttacking** = **False**, **Set bAttackReturning** = **False**.

   Do **not** call `FinishEnemyTurn` — the battle system handles the block+counter flow; the knight will counter-attack after `BlockCounterAttackDelaySeconds`.

## 5. Result PNG assets

Add these textures to the project and assign them in WBP_BlockMinigame:

- **BLOCKED** — shown when block succeeds
- **COUNTERED** — shown when counter succeeds
- **FAILED** — shown when block or counter fails

## 6. Knight flows (optional BP tweaks)

- **Block + Counter success**: The C++ already triggers a counter-attack using `PendingPlayerDamage`. Your knight animation can bind to `On Player Attack Started` (same event as the initial attack).
- **Block only / Counter fail**: Knight returns without attacking. You may want a quick “return” or “block” animation instead of the full attack.
