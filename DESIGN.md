# VISUAL & AUDIO BIBLE: HYPER-SATURATED CRT SYNTH-NOIR

## 1. COLOR PALETTE & LIGHTING (LUMEN)
- **Environment Void:** Pitch-black surroundings (`#050508`). The casino beyond the table is swallowed in darkness; only the table felt, player hands, and Pit Boss silhouette are illuminated.
- **Table Felt:** Matte-black crushed velvet (`#0A0B10`, Roughness `0.92`) etched with emissive vector grid lines.
- **Semantic Emissive Colors (Lumen Bounce Sources):**
  - **Base Chips / Table Grid:** Phosphor Cyan (`#00F5D4`, Emissive Multiplier `15.0`)
  - **Throw Mult / Lasers / Hazards:** Hot Synth Magenta (`#FF007F`, Emissive Multiplier `20.0`)
  - **Bet Mult / Gold Bullion / Jackpots:** High-Voltage Amber/Gold (`#FFB703`, Emissive Multiplier `18.0`)
  - **Danger / Seven-Out Warning:** CRT Blood Crimson (`#FF1E42`, Emissive Multiplier `25.0`)

## 2. DYNAMIC CRT "OVERDRIVE" POST-PROCESS SHADER (`M_PP_CRTOverdrive`)
All visual intensity is driven by a single normalized scalar parameter `MultiplierIntensity` (`0.0` to `1.0`) updated in real time by `UScoringQueueSubsystem`:
- **Tier 1 (`0.0 - 0.2` | 1x–10x Mult):** Subtle curved barrel distortion, crisp 480p phosphor scanlines, gentle bloom on felt vector lines.
- **Tier 2 (`0.2 - 0.6` | 50x–500x Mult):** Radial RGB chromatic aberration at screen edges, Niagara ribbon light-trails (*Tron*-style) behind tumbling dice, mild screen shake on bounce impacts.
- **Tier 3 (`0.6 - 1.0` | 1,000x+ "Breaking the Table"):** Heavy VHS tracking distortion bars, wireframe geometry shockwaves erupting from the felt on collision, 3D score widgets physically shattering into emissive voxel particles.

## 3. PROCEDURAL AUDIO (METASOUNDS)
- **Sequential Pitch Escalation:** Every step popped from `UScoringQueueSubsystem` increments the `PitchSemitoneShift` parameter (`+1.0` semitone per trigger, clamping or octave-wrapping at `+24`), syncing audio dopamine directly with *Balatro*-style visual popups.
- **Tactile Foley:** Heavy acrylic clacks for on-felt dice tipping, velvet slide friction loops for Table-Skimmer throws, and low-end sub-bass drops when a 7 settles or is negated.