# Enemy Attack – Continue From Here

You’ve already done:
- mainMenu: Bind **OnEnemyAttackStartedWithKnight**, get first enemy via **Get All Actors Of Class** → **GET [0]**.
- Event flow: only the custom event’s **then** drives Get All Actors Of Class.

Do the following **in order**. When done, the enemy will dash at the knight and return; then we’ll add hit/damage (Part 6).

---

## Step 1: mainMenu – Call StartAttack on the enemy

1. Open **mainMenu** (Level Blueprint or whatever blueprint has your Event Graph).
2. In the **OnEnemyAttackStartedWithKnight_Event** chain, after **GET [0]**:
   - Drag from **GET [0]**’s **Output** (the first BP_Enemy) → search **StartAttack**. If it doesn’t exist yet, search **Call function on self** or type the name; you’ll get a “missing function” node that will link once we add it in BP_Enemy.
   - On the **StartAttack** node:
     - **Target** = the enemy (wire from **GET [0] Output**).
     - **Target Actor** = **Knight Actor** from **OnEnemyAttackStartedWithKnight_Event**.
   - **Execution**: connect **Get All Actors Of Class**’s **then** (or the last exec before StartAttack) to **StartAttack**’s **execute** input. So the flow is: **OnEnemyAttackStartedWithKnight_Event (then)** → **Get All Actors Of Class** → **GET [0]** → **StartAttack** (enemy as target, Knight Actor as Target Actor).

If **StartAttack** doesn’t appear, add a **Call Function** node and set the function to **StartAttack** on the enemy class; the pin will exist after Step 3.

---

## Step 2: BP_Enemy – Variables (Part 3)

1. Open **BP_Enemy** (Content Browser → e.g. **Images/Enemies/BP_Enemy**).
2. **My Blueprint** → **Variables** → add these (click **+**):

| Variable               | Type    | Default / note        |
|------------------------|---------|------------------------|
| AttackHomeLocation     | Vector  | (none)                |
| AttackTargetLocation   | Vector  | (none)                |
| AttackPlayRate         | Float   | (none)                |
| bIsAttacking           | Boolean | false                 |
| bAttackReturning       | Boolean | false                 |
| StopShortDistance      | Float   | **80**                |
| AttackSpeed            | Float   | **800**               |

3. Optional for later: **bHasAppliedDamageThisAttack** (Boolean, false).

---

## Step 3: BP_Enemy – Timeline (Part 3)

1. In **BP_Enemy** **Event Graph**, right‑click → **Add Timeline**.
2. Choose **Timeline** (not Timeline component). Name it **TL_AttackDash**.
3. Double‑click the timeline:
   - **Length** = **1.0**.
   - Add **Float** track, name **Alpha**.
   - Key at time **0**: value **0**.
   - Key at time **1**: value **1**.
   - Interpolation: Linear. Save and close.

---

## Step 4: BP_Enemy – StartAttack function (Part 4)

1. **My Blueprint** → **Functions** → **+** → name **StartAttack**.
2. Open **StartAttack**. On the **Entry** node, add input **Target Actor** (type **Actor** reference).
3. Build this flow (all in **StartAttack**):
   - **Entry (exec)** → **Branch**.
   - **Branch (Condition)** = **bIsAttacking** (get variable).
   - **Branch (False)** → **Set bIsAttacking** = **True** → **Set bAttackReturning** = **False**.
   - **Get Actor Location** (self) → **Set AttackHomeLocation**.
   - **Get Actor Location** (Target Actor) → **Break Vector** → take **X**, **Y**, **Z**.
   - **Get AttackHomeLocation** → **Break Vector** (we only need Y and Z from here).
   - **Make Vector**:  
     **X** = **Knight X + StopShortDistance** (so enemy stops short of the knight; use **float + float** with **StopShortDistance** variable),  
     **Y** = **AttackHomeLocation Y**,  
     **Z** = **AttackHomeLocation Z**  
     → **Set AttackTargetLocation**.
   - **Vector Distance** (AttackHomeLocation, AttackTargetLocation) → **/ AttackSpeed** → **Max (0.05)** (so no divide by zero) → **1.0 / that** → **Set AttackPlayRate**.
   - From the same chain (after Set AttackPlayRate), **Call** custom event **Attack_Play** (you add it in Step 5). So: exec wire into **Attack_Play**.
4. **Branch (True)**: leave unconnected (early return when already attacking).

---

## Step 5: BP_Enemy – Attack_Play and timeline movement (Part 5)

### 5a. Custom event and play

1. In **Event Graph**, right‑click → **Add Custom Event** → name **Attack_Play**.
2. From **Attack_Play (then)**:
   - **Set Play Rate** (on **TL_AttackDash**): **New Rate** = **AttackPlayRate**.
   - **Play from Start** (or **Play**) on **TL_AttackDash**.

### 5b. Timeline Update (move enemy)

1. Drag **TL_AttackDash** into the graph (or use the timeline node you already have).
2. **TL_AttackDash** → **Update** (exec) → **VLerp**:
   - **A** = **AttackHomeLocation**,
   - **B** = **AttackTargetLocation**,
   - **Alpha** = timeline’s **Alpha** (float output from the same timeline node).
3. **VLerp (Return Value)** → **Set Actor Location** (self), **New Location** = VLerp result. Leave **Sweep** unchecked.

### 5c. Timeline Finished (return or reverse)

1. **TL_AttackDash** → **Finished** (exec) → **Branch**; **Condition** = **bAttackReturning**.
2. **Branch (True)**:
   - **Set Actor Location** (self) = **AttackHomeLocation**.
   - **Set bIsAttacking** = **False**, **Set bAttackReturning** = **False**.
   - **Get Player Controller** (0) → **Cast to BP_OpeningKnightPlayerController** → **FinishEnemyTurn** (call on the cast result).
3. **Branch (False)**:
   - **Set bAttackReturning** = **True**.
   - **TL_AttackDash** → **Reverse** (or **Reverse from End**). When the timeline finishes again, **bAttackReturning** will be true and the True branch will run.

---

## Step 6: Test

1. **Compile & Save** BP_Enemy.
2. In mainMenu, confirm **StartAttack** is called with enemy from **GET [0]** and **Knight Actor** from the event.
3. Play the level: when the enemy turn starts, the enemy should dash toward the knight then return; after return, the turn should finish (dice/hand reset).

---

## Next (Part 6 – hit and damage)

After this works, add:
- **HitCloud** at impact (timeline event at ~0.5 or Alpha check in Update).
- **AttackHitBox** component: enable during attack, **On Begin Overlap** with **BP_Knight** → **ConfirmEnemyHit** once per attack, disable hitbox when attack finishes.

See **BP_Enemy_Attack_Implementation.md** Part 6 for details.
