# Project Context: CRT Synth-Noir Craps Roguelike (UE5)

## 1. Core Vision & Aesthetic
- **Concept:** A first-person 3D roguelike craps deckbuilder ("Balatro meets Craps").
- **Visual Style:** Hyper-Saturated CRT Synth-Noir (matte-black velvet felt, glowing phosphor-green/magenta vector lines, Lumen emissive lighting, dynamic CRT Post-Process shader driven by `MultiplierIntensity`).
- **Two-Handed POV Architecture:**
  - **Right Hand (Active Shooter):** Handles on-felt dice setting and physical throw archetypes (Table-Skimmer, Parabolic Lob, Back-Wall Whip, Mechanical Plunger).
  - **Left Hand (Passive/Active Anchor):** Rests on the left leather rail. Holds sockets for Rings (4), Wristwear (1), and Rail Props (2) that act as passive "Jokers" and clickable active abilities.

## 2. Non-Negotiable Unreal Engine 5 Architecture Rules
- **10x World & Physics Scale:** Real-world craps dice (1.9cm) cause Chaos Physics tunneling. The entire game is built at **10x scale**. Standard dice are `20cm x 20cm x 20cm` in Unreal units. Scale table, chips, hands, and camera distances by 10x accordingly. Always enable `bUseCCD = true` (Continuous Collision Detection) on dice and chip physics bodies.
- **C++ Core / Thin Blueprints Split:**
  - Write all math, state machines, scoring queues, data structures, and physics rigging in **C++**.
  - Expose properties with `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Craps|...")` and functions with `UFUNCTION(BlueprintCallable, Category="Craps|...")`.
  - Use **Blueprints** strictly for visual/audio hooks (`BlueprintImplementableEvent`), Niagara particle triggers, MetaSounds pitch escalation, and UMG widget animations.
- **Data-Driven Modifiers:** Never hardcode individual Trinkets, Dice Faces, or Bet Upgrades into C++ classes. Use `UPrimaryDataAsset` subclasses (`UPDA_DiceFace`, `UPDA_Trinket`, `UPDA_BetUpgrade`) and `FGameplayTag` containers, or JSON-backed `UDataTable` rows so content can be batch-generated.

## 3. Core Gameplay Systems & Math Formula
- **The Scoring Formula:**
  `Total Payout = (Base Chips) * (Throw Mult) * (Bet Mult)`
  - `Base Chips` (Cyan): Earned from settled Dice Faces + physical hazard/bumper impacts.
  - `Throw Mult` (Magenta): Accumulated mid-air and during felt travel (alligator wall hits, target ring accuracy, laser gates).
  - `Bet Mult` (Gold): Triggered sequentially when dice settle across active table bets (Pass Line, Hardways, Field, Horn, Place Bets) and Left-Hand Trinkets.
- **Sequential Scoring Queue (`UScoringQueueSubsystem`):**
  - NEVER resolve end-of-roll scoring in an instant synchronous `for` loop.
  - Push every triggered `FScoringStep` (Source Actor, Chip Delta, Mult Delta, Multiplier Factor, Audio Pitch Index, Visual Socket) into `UScoringQueueSubsystem`.
  - Pop and execute steps via a timer (`0.25s` base interval, accelerating per step) so each die face, chip stack, and left-hand ring visibly animates and tallies in sequence.

## 4. Dice Physics & "Torque Magnet" Rigging (`UDiceRiggingComponent`)
- **Face Detection:** `ADieBase` uses 6 directional sockets (`+X, -X, +Y, -Y, +Z, -Z`). Determine the settled top face via maximum Dot Product against `FVector::UpVector` `(0, 0, 1)`.
- **Deterministic Influence ("Dice Control"):** Chaos physics handles mid-air collisions, but as linear velocity drops below `SettlingVelocityThreshold`, `UDiceRiggingComponent` applies calculated angular torque (`AddTorqueInRadians`) based on the player's `DiceControlStat` (`0.0` pure RNG to `1.0` locked axis) to nudge the die toward the pre-set top face or away from a fatal 7.

## 5. MCP & Editor Automation Guidelines
- When using Unreal MCP tools or Editor Python scripts:
  - Place generated Data Assets in `/Game/Data/Trinkets/`, `/Game/Data/DiceFaces/`, and `/Game/Data/Bets/`.
  - Keep the developer graybox level at `/Game/Maps/L_TestingPit` with live debug toggles for Dice Set Method, Throw Archetype, Physics Rigging Slider (`0-100%`), and Table Surface friction.
  - Prefer triggering **Live Coding** after C++ modifications rather than restarting the editor.