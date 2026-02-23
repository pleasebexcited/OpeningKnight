# BP_Enemy Attack Implementation Guide

This guide adds an enemy attack to **BP_Enemy** that mirrors the knight’s dash (right-to-left), keeps existing knockback, and applies health damage only when the attack hits the knight.

**If you’ve already done Part 2 (bind delegate + get enemy in mainMenu)** and want the next steps in one place, use **[BP_Enemy_Attack_Continue.md](BP_Enemy_Attack_Continue.md)** for Parts 3–5 plus the mainMenu **StartAttack** wire.

---

## Part 1: C++ Changes (Already Done)

- **EnemyAttackStartDelaySeconds** (default 0.8s) was added so the enemy attack starts after the knight has time to return. Tune this in the Battle component (e.g. on **BP_OpeningKnightPlayerController** → **Battle** → Details → **OpeningKnight | Tuning**).
- **OnEnemyAttackStartedWithKnight** was added: it fires when the enemy turn attack starts and passes **PendingDamage**, **DebugText**, and **KnightActor**. Use this in Blueprint to tell the enemy to run its attack.
- **AutoConfirmEnemyHitDelaySeconds** default is now **0** so damage is only applied when you call **ConfirmEnemyHit** from Blueprint at the real impact moment.

---

## Part 2: Get the Enemy to Start Its Attack (Level or Game Mode)

You need something to listen to **OnEnemyAttackStartedWithKnight** and call **StartAttack(Knight)** on your enemy.

### Option A: Level Blueprint (mainMenu or your battle level)

1. Open your **level** (e.g. **mainMenu**).
2. Open the **Level Blueprint**: **Blueprints → Open Level Blueprint** (or **Toolbar → Blueprints → Open Level Blueprint**).
3. In the Level Blueprint **Event Graph**:
   - **Get Player Controller**: Right-click → search **Get Player Controller** → leave **Player Index** as 0.
   - **Cast to BP_OpeningKnightPlayerController**: Drag from the return value → **Cast to BP_OpeningKnightPlayerController** (search **Cast to BP_OpeningKnightPlayerController**). Connect the **exec** (white) pin of **Get Player Controller** into the **Cast** node.
   - **Get Battle**: From the **As BP Opening Knight Player Controller** pin, drag and search **Get Battle** (or in the Details of the cast result, find **Battle** and drag it out). You should get the **Battle** component reference.
   - **Bind Event to OnEnemyAttackStartedWithKnight**: Drag from the **Battle** pin → search **Assign On Enemy Attack Started With Knight** (or **Add Custom Event** and choose to create an event that listens to the delegate). If your version uses **Bind Event** / **Add Custom Event for OnEnemyAttackStartedWithKnight**, use that so you get an event node with **Pending Damage**, **Debug Text**, and **Knight Actor**.
   - **Get Enemy Reference**: You need a reference to the **BP_Enemy** in the level. Either:
     - **Get All Actors of Class**: Right-click → search **Get All Actors of Class** → set **Actor Class** to **BP_Enemy** (click the dropdown, search **BP_Enemy**). From the **Out Actors** array, use **Get** (index 0) to get the first enemy, or use **Array Get** if you have multiple and want a specific one.
     - Or place the enemy in the level and **Get a reference** to it (e.g. by dragging the enemy instance from the World Outliner into the Level Blueprint to create a reference).
   - **Call StartAttack on the enemy**: From the **Knight Actor** pin of the bound event, and your **Enemy** reference, call the enemy’s **StartAttack** custom event (you will add this in Part 3). So: from the **exec** output of the **OnEnemyAttackStartedWithKnight** event, call **StartAttack** on the enemy, and pass **Knight Actor** as the **Target Actor** argument.

**Wiring summary:**  
Player Controller → Cast to BP_OpeningKnightPlayerController → Get **Battle** → **Assign On Enemy Attack Started With Knight** (or equivalent). In the bound event: get your **BP_Enemy** reference, then call **Enemy → StartAttack(Target Actor = Knight Actor)**.

### Option B: Game Mode (BP_OpeningKnightGameMode)

If you prefer to do this in the Game Mode:

1. Open **BP_OpeningKnightGameMode** (Content Browser → **Blueprints** or your folder → double‑click).
2. In **Event BeginPlay** (or a suitable event):
   - Get **Player Controller** (e.g. **Get Player Controller** with index 0).
   - **Cast to BP_OpeningKnightPlayerController**.
   - Get **Battle** from the cast result.
   - **Assign On Enemy Attack Started With Knight** (or bind the delegate) so you get an event with **Pending Damage**, **Debug Text**, **Knight Actor**.
3. You still need a reference to **BP_Enemy** (e.g. store it when you spawn the enemy, or **Get All Actors of Class** BP_Enemy and **Get** index 0).
4. In the bound event, call **StartAttack** on that enemy, passing **Knight Actor** as **Target Actor**.

---

## Part 3: BP_Enemy – Variables and Timeline

1. Open **BP_Enemy** (Content Browser → **Images/Enemies** or where **BP_Enemy** lives → double‑click).
2. In the **My Blueprint** panel, under **Variables**:
   - Add (click **+** next to Variables):
     - **AttackHomeLocation** (Type: **Vector**)
     - **AttackTargetLocation** (Type: **Vector**)
     - **AttackPlayRate** (Type: **Float**)
     - **bIsAttacking** (Type: **Boolean**)
     - **bAttackReturning** (Type: **Boolean**)
     - **StopShortDistance** (Type: **Float**; default e.g. **80** – enemy stops this far from the knight, mirroring the knight’s **Stop Short Distance**)
     - **AttackSpeed** (Type: **Float**; default e.g. **800** – similar to knight’s **Dash Speed**)
   - Optionally **HitCloudRefAttack** if you want a separate HitCloud for the attack (or reuse **HitCloudRef** at impact).
3. **Add Timeline for the attack:**
   - In the **Event Graph**, right‑click → search **Add Timeline**.
   - Select **Timeline** (not **Timeline component** unless you want a component). Name it **TL_AttackDash** (or **TL_EnemyAttack**).
   - Double‑click the timeline to open it. Set **Length** to **1.0**. Add a **Float track** named **Alpha**. Add two keys: time **0** value **0**, and time **1** value **1**, with linear interpolation. Save/close.

---

## Part 4: BP_Enemy – StartAttack Function (Mirror of Knight)

1. In **BP_Enemy**, in **My Blueprint** under **Functions**, click **+** to add a new **Function**. Name it **StartAttack**.
2. Open the **StartAttack** function graph. Add an **Input**: right‑click the **Target Actor** pin on the **Entry** node (or **Add pin** on the function entry) → add **Target Actor** (Type: **Actor** (Object Reference)).
3. Build the logic (mirror of **BP_Knight → StartAttachDash**):
   - **Branch** on **bIsAttacking**: if **True**, return (do nothing). If **False**:
   - **Set bIsAttacking** = **True**.
   - **Set bAttackReturning** = **False**.
   - **Get Actor Location** (self) → **Set AttackHomeLocation**.
   - **Get Actor Location** of **Target Actor** (the knight) → **Break Vector** (X, Y, Z).
   - **Get AttackHomeLocation** → **Break Vector**.
   - For the **attack target** (enemy moves **left** toward knight):  
     **Target X** = **Knight X + StopShortDistance** (so the enemy stops short on the knight’s side).  
     Use **Make Vector**: X = **Knight X + StopShortDistance**, Y = **AttackHomeLocation Y**, Z = **AttackHomeLocation Z** → **Set AttackTargetLocation**.
   - **Vector Distance** between **AttackHomeLocation** and **AttackTargetLocation** → divide by **AttackSpeed** → **Max** with **0.05** → then **1.0 / that** → **Set AttackPlayRate**.
   - From the **exec** output, call the custom event **Attack_Play** (you will add it next).

---

## Part 5: BP_Enemy – Attack_Play and Timeline Movement

1. **Add Custom Event**: In the **Event Graph**, right‑click → **Add Custom Event** → name it **Attack_Play**.
2. **Set Play Rate and Play timeline:**
   - **Get** your **TL_AttackDash** timeline reference (if it’s a timeline node in the graph, use that; if you added a **Timeline component**, get the component and use **Set Play Rate** on it).
   - **Set Play Rate** (search **Set Play Rate** on **Timeline**): **Target** = timeline, **New Rate** = **AttackPlayRate**.
   - **Play from Start** (or **Play**) on the same timeline.
3. **Timeline Update** (movement):
   - From the timeline node, use the **Update** exec pin.
   - **VLerp**: **A** = **AttackHomeLocation**, **B** = **AttackTargetLocation**, **Alpha** = timeline’s **Alpha** output.
   - **Set Actor Location** (self) with **New Location** = **VLerp** result (Sweep/Teleport unchecked).
4. **Timeline Finished** (return home or reverse):
   - From the timeline’s **Finished** exec pin, use a **Branch** on **bAttackReturning**:
     - **True**: **Set Actor Location** (self) to **AttackHomeLocation** → **Set bIsAttacking** = **False** → **Set bAttackReturning** = **False** → then call **FinishEnemyTurn** on the player controller (see below).
     - **False**: **Set bAttackReturning** = **True** → **Reverse** (or **Reverse from End**) on the timeline so the enemy moves back; when the timeline finishes again, **bAttackReturning** will be true and you’ll run the True branch.

This mirrors the knight’s **Dash_Play** + **TL_AttackDash** (Update = move, Finished = return or snap home and clear flags).

---

## Part 6: BP_Enemy – HitCloud and Health on Contact

### HitCloud at impact

- When the enemy reaches the knight (e.g. when **Alpha** is around **0.5** or at the peak of the lunge), trigger your HitCloud.
- **Option 1 – Timeline event track:** In **TL_AttackDash**, add an **Event** track and place an event at the time that matches “impact” (e.g. 0.5). From that event’s exec output, call **Flash** on **HitCloudRef** (or **HitCloudRefAttack**).
- **Option 2 – In Update:** From the timeline **Update** branch, use a **Branch** on **Alpha >= 0.48 and Alpha < 0.52** (or use a **Boolean** “has played hit cloud this attack” and set it after the first Flash) and call **Flash** on **HitCloudRef** once.

### Health bar only when attack hits (ConfirmEnemyHit on contact)

- **Do not** call **ConfirmEnemyHit** from **HitTrigger** overlap (that’s for the knight hitting the enemy). Call it only when the **enemy’s attack** hits the knight.
- Add a separate **attack hitbox** (e.g. a **Box Collision** or **Sphere** component) on **BP_Enemy** that is **disabled by default** and **enabled only during the attack** (e.g. **Set Collision Enabled** when you start the attack, disable when you finish).
- On **Begin Overlap** of this attack hitbox:
  - **Cast Other Actor to BP_Knight**. If the cast succeeds and you are in the “attacking” phase (e.g. **bIsAttacking** is true) and you haven’t applied damage yet for this attack, then:
    - **Get Player Controller** (0) → **Cast to BP_OpeningKnightPlayerController** → **Get Battle** → call **ConfirmEnemyHit**.
    - Set a local **Boolean** (e.g. **bHasAppliedDamageThisAttack**) to **True** so you only apply damage once per attack. Reset this to **False** when you finish the attack (when you set **bIsAttacking** = false in the timeline Finished branch).
- So: **ConfirmEnemyHit** is called only when the attack hitbox overlaps the knight during the enemy attack, and only once per attack.

**Steps for the attack hitbox:**

1. In **BP_Enemy**, in the **Components** panel, **Add Component** → **Box Collision** (or **Sphere**). Name it e.g. **AttackHitBox**.
2. Place and scale it so it’s in front of the enemy (where the “sword” or hit would be). Set **Collision** so it overlaps **BP_Knight** (e.g. **Collision Preset** that overlaps Pawn or your knight’s channel).
3. In **Details**, set **Collision Enabled** to **No Collision** or **Query Only** and **Generate Overlap Events** = true; you will enable it in Blueprint when the attack starts and disable when it ends.
4. In the **Event Graph**, **Event BeginPlay**: **Set Collision Enabled** on **AttackHitBox** to **Disabled** (or keep it disabled in the Details).
5. When you **StartAttack** (or when **Attack_Play** runs), **Set Collision Enabled** on **AttackHitBox** to **Query and Physics** (or whatever allows overlap with the knight).
6. When the timeline **Finished** (True branch, when you reset **bIsAttacking** and call **FinishEnemyTurn**), **Set Collision Enabled** on **AttackHitBox** back to **Disabled**, and set **bHasAppliedDamageThisAttack** = **False**.
7. **On Component Begin Overlap** (AttackHitBox): **Other Actor** → **Cast to BP_Knight**. If cast succeeds and **bIsAttacking** and not **bHasAppliedDamageThisAttack**, then **Get Player Controller (0)** → **Cast to BP_OpeningKnightPlayerController** → **Get Battle** → **ConfirmEnemyHit** → **Set bHasAppliedDamageThisAttack** = **True**.

---

## Part 7: Calling FinishEnemyTurn When the Enemy Returns

When the enemy has finished returning to its home position (timeline Finished, **bAttackReturning** True branch):

1. **Get Player Controller** (0).
2. **Cast to BP_OpeningKnightPlayerController**.
3. From the cast result, call **FinishEnemyTurn** (this is on the Player Controller; it forwards to **Battle → FinishEnemyTurn()**).

This lets the battle component reset the hand and allow the player to roll again.

---

## Part 8: Tuning in the Editor

- On **BP_OpeningKnightPlayerController** → select **Battle** → **Details**:
  - **Enemy Attack Start Delay Seconds**: time after player hit is confirmed before the enemy attack starts (knight return + small delay). Try **0.8** and adjust.
  - **Auto Confirm Enemy Hit Delay Seconds**: leave at **0** so damage is only applied when your attack hitbox overlap calls **ConfirmEnemyHit**.
  - **Enemy Return Delay Seconds**: still used if you don’t call **FinishEnemyTurn** from BP; if you always call **FinishEnemyTurn** when the enemy’s timeline finishes, this fallback may not run.

- On **BP_Enemy**:
  - **StopShortDistance**, **AttackSpeed**: tune so the enemy’s lunge distance and speed feel right (mirror of the knight’s feel).

---

## Quick Checklist

- [ ] Level Blueprint or Game Mode: bind **OnEnemyAttackStartedWithKnight** and call **StartAttack(Knight Actor)** on the enemy reference.
- [ ] BP_Enemy: variables **AttackHomeLocation**, **AttackTargetLocation**, **AttackPlayRate**, **bIsAttacking**, **bAttackReturning**, **StopShortDistance**, **AttackSpeed** (and optional **bHasAppliedDamageThisAttack**).
- [ ] BP_Enemy: **TL_AttackDash** timeline (Alpha 0→1), **StartAttack(Target Actor)** function (mirror of StartAttachDash, right-to-left), **Attack_Play** event (Set Play Rate → Play timeline).
- [ ] BP_Enemy: Timeline **Update** → VLerp(AttackHomeLocation, AttackTargetLocation, Alpha) → Set Actor Location.
- [ ] BP_Enemy: Timeline **Finished** → Branch **bAttackReturning** → True: Set Location to AttackHomeLocation, clear flags, call **FinishEnemyTurn**; False: set **bAttackReturning**, Reverse timeline.
- [ ] BP_Enemy: HitCloud **Flash** at impact (event track or Alpha check in Update).
- [ ] BP_Enemy: **AttackHitBox** component, enabled only during attack; **On Begin Overlap** with **BP_Knight** → **ConfirmEnemyHit** once per attack, then **FinishEnemyTurn** when attack fully finished.

If you tell me whether you’re using Level Blueprint or Game Mode and how you get the enemy reference (single enemy in level vs spawned), I can adapt the steps to your exact setup.
