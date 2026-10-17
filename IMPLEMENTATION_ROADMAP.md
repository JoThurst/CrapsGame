# IMPLEMENTATION ROADMAP & AGENT MILESTONES

## MILESTONE 1: THE 10x GRAYBOX "TESTING PIT" (`L_TestingPit`)
**Objective:** Build the playable POV physics sandbox to evaluate setting and throwing feel before adding roguelike progression.
- [x] **Task 1.1 (`ADieBase` & `UPDA_DiceFace`):** Create the 20cm modular die C++ actor with 6 directional face sockets (`+X, -X, +Y, -Y, +Z, -Z`) and Dot-Product top-face detection against `FVector::UpVector`.
- [x] **Task 1.2 (`UDiceRiggingComponent`):** Implement the hybrid "Torque Magnet" (`AddTorqueInRadians` when velocity drops below `SettlingVelocityThreshold`) driven by a `DiceControlStat` (`0.0` to `1.0` slider).
- [x] **Task 1.3 (`ACrapsPawn` & On-Felt Setting):** Implement the Stickman push-back state and the 3 On-Felt Dice Setting methods (Finger-Tip 90° click rotation, V-Set locked-axis scroll, and `V/B/N` Preset Snap).
- [x] **Task 1.4 (`IThrowStyleInterface`):** Implement the 4 hot-swappable Throw Archetypes (`1` Table-Skimmer, `2` Parabolic Lob, `3` Back-Wall Whip, `4` Mechanical Plunger) + Debug UI overlay to toggle `[Mouse Flick]`, `[Arcade Meter]`, and `[Slingshot Pull]` inputs.

## MILESTONE 2: THE BALATRO SCORING ENGINE & FELT ZONES
**Objective:** Wire the 3-layer math engine and sequential dopamine queue.
- [x] **Task 2.1 (`UScoringQueueSubsystem`):** Build the sequential event queue that processes `Base Chips (Cyan) * Throw Mult (Magenta) * Bet Mult (Gold)` on a dynamic timer and drives the `MultiplierIntensity` shader parameter.
- [x] **Task 2.2 (`ABetZone` & Craps State Machine):** Build `ACrapsGameState` (Come-Out Roll -> Point Established -> Point Hit / Seven-Out) and 3D collision boxes for Pass Line, Don't Pass, Field, Hardways, Place Bets, and Horn Bets.
- [x] **Task 2.3 (Table Hazards):** Create physical pinball bumpers, laser tripwire gates, and destructible/scatterable 3D chip stacks that inject `FScoringStep` entries upon mid-roll collision.

## MILESTONE 3: LEFT-HAND ANCHOR, DICE FACES & SHOP LOOP
**Objective:** Populate the roguelike build-crafting items and Backroom vendor loop.
- [ ] **Task 3.1 (Left-Hand Socket Architecture):** Add the Left Arm mesh to `ACrapsPawn` with 4 Ring sockets, 1 Wristwear socket (clickable active raycast), and 2 Rail Prop sockets.
- [ ] **Task 3.2 (Batch Content Generation via MCP):** Generate the 8 `UPDA_DiceFace` assets and 15 `UPDA_Trinket` assets in `/Game/Data/` with GameplayTag triggers hooked into `UScoringQueueSubsystem`.
- [ ] **Task 3.3 (The Backroom Shop & Pit Bosses):** Implement the between-Ante briefcase shop UI and the first 3 Pit Boss table modifiers (*The Cooler*, *The Mathematician*, *The Hawk*).

## MILESTONE 4: CRT SYNTH-NOIR SHADER & METASOUNDS JUICE
**Objective:** Apply the visual and audio overdrive layer from `DESIGN.md`.
- [ ] **Task 4.1 (`M_PP_CRTOverdrive`):** Build the Post-Process CRT barrel distortion, scanline, chromatic aberration, and VHS glitch material linked to `UScoringQueueSubsystem::GetNormalizedIntensity()`.
- [ ] **Task 4.2 (Niagara & MetaSounds):** Hook up *Tron*-style dice ribbon trails, voxel score bursts, and the `+1 semitone` rising MetaSound pitch trigger on every `FScoringStep`.