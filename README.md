# DELTARUNE: Chapter 1 - Nintendo DS Port
*(Powered by 🥧 Snail Pie DS 🥧 - Real-time GameMaker / Deltarune DS Runner & Compiler)*

A complete homebrew recreation of **Toby Fox's DELTARUNE Chapter 1** built specifically for the **Nintendo DS** and output as standalone `.nds` ROMs: **`Deltarune.nds`** and **`snail_pie_ds/compiled/SnailPieDS.nds`**.

Playable on real Nintendo DS, DS Lite, DSi, and 3DS hardware (via R4, flashcarts, or TwilightMenu++) as well as all Nintendo DS emulators (melonDS, DeSmuME, Delta, etc.).

---

## 🥧 Snail Pie DS (Live GameMaker Compiler for Nintendo DS)

**Snail Pie DS** is the Nintendo DS companion to Butterscotch!
Drop any GameMaker or Deltarune game file (`data.win`, sprite PNGs, audio, rooms) into the `snail_pie_ds/drop/` folder and **BIM!** It compiles the Nintendo DS ROM in real time!

### Commands

```bash
# 1. Compile immediately from the drop folder
./snail_pie_ds_run
# or
python3 snail_pie_ds/snail_pie_ds.py

# 2. Real-time Live Watcher mode
# Watches snail_pie_ds/drop/ continuously: drop a file and BAM! Recompiles instantly!
python3 snail_pie_ds/snail_pie_ds.py --watch

# 3. Extract all Chapter 1 sprites directly to snail_pie_ds/extracted_sprites/
python3 snail_pie_ds/snail_pie_ds.py --extract
```

### Folder Layout
- `snail_pie_ds/drop/`: Drop your files here (`data.win`, `.ogg`, sprites).
- `snail_pie_ds/extracted_sprites/`: Extracted PNG sprites from GameMaker texture pages.
- `snail_pie_ds/compiled/`: Pre-compiled `SnailPieDS.nds`.

---

## 🎮 Features & Full Chapter 1 Coverage

### 1. Complete Cast of Sprites & Animations
All authentic sprites and animation cycles from Chapter 1 reproduced:
- **Kris**:
  - Light World school clothes walk (4 directions x 4 frames).
  - Dark World knight armor walk (4 directions x 4 frames).
  - Battle idle (2 frames), attack sword slash (3 frames), defend pose, hurt reaction, bow, pirouette, victory.
- **Susie**:
  - Light World walk, Dark World walk (4 directions x 4 frames).
  - Battle idle (2 frames), heavy axe attack (3 frames), Rude Buster spell casting, defend pose, hurt, laugh.
- **Ralsei**:
  - Dark World walk (4 directions x 4 frames), blush walk.
  - Battle idle (2 frames), spell casting (Heal Prayer & Pacify), defend pose, hurt.
- **Lancer**:
  - Overworld walk (4 directions), battle idle, hurt, bike, laugh, mustache pose.
- **Rudinn**:
  - Overworld patrolling enemy, battle idle bobbing, hurt / spared.
- **K. Round (Checker King)**:
  - Giant 43x77 checker piece with stomping idle animation (4 frames), crown adjustments, deep bow reaction.
- **King (Chaos King)**:
  - Giant 75x75 Chaos King with idle breathing animation (5 frames), cape slash, spade chain attacks, hurt.
- **Jevil (Secret Boss)**:
  - 46x48 jumping and dancing animation (8 frames), teleport attack, tired pose, spinning Devilsknife scythe blades.
- **Face Portraits**:
  - High-res portraits for Susie (neutral/angry/smile/bangs), Ralsei (hat/smile/blush), Lancer, Toriel, and King!

### 2. Dual-Screen 16-bit Bitmap Engine (Mode 5 2D)
- **Top Screen**: Perspective battle theater with tiled battle grid (`bg_battle_tile`), party animations, enemy animations, damage popups, and speech bubbles.
- **Bottom Screen**: TP Gauge (0-100%), Party health cards, 5 Action Buttons (FIGHT, ACT, ITEM, SPARE, DEFEND) supporting both **D-Pad** and **Touch Screen**, moving attack timing meter, and real-time Bullet Hell arena.
- **Zero-Tearing Double Buffering**: Offscreen EWRAM framebuffers transferred via ARM9 DMA during VBlank.

### 3. Story Campaign & Cutscenes
- **Scene 1 (School Wakeup)**: Toriel's morning greeting, driving to school, classroom with Miss Alphys, and Susie heading to the supply closet.
- **Scene 2 (The Dark Fall)**: The closet floor drops into the void! Dual-screen falling physics with tumbling sprites and rushing speed lines.
- **Scene 3 (The Legend of Delta Rune)**: Glowing crest and the ancient prophecy recited by Ralsei.
- **Overworld Exploration**: Hometown School, Dark Abyss, Castle Town (Great Door & Ralsei), and Field of Hopes & Dreams (Save Stars, wandering Rudinn, Lancer roadblock).

### 4. Turn-Based & Bullet Hell Combat System
- **Party Actions**:
  - *Kris*: Attack, Check, Compliment, Deep Bow (against K. Round to knock off the crown!), Courage (King), Pirouette (Jevil).
  - *Susie*: Axe Attack, Rude Buster (40% TP, massive damage), Warn.
  - *Ralsei*: Heal Prayer (32% TP, +45 HP party), Pacify (spares tired enemies), Defend.
- **Bullet Hell Soul Dodging**:
  - Smooth 8-directional Red Soul dodging inside the arena box.
  - **Graze System**: Grazing close to bullets triggers white aura rings, plays `snd_graze`, and awards +2% TP per graze.
  - **Boss Attack Patterns**:
    - *Rudinn*: Targeted diamond projectile spreads.
    - *Lancer*: Bouncing spade waves.
    - *K. Round*: Checker shrapnel and diamond stomps.
    - *Chaos King*: Spade chains and sweeping arcs.
    - *Jevil*: Rotating Devilsknife scythes, chaos bursts, and voice lines ("CHAOS, CHAOS!").

### 5. Sound & Music
- Hardware 8-bit signed PCM audio.
- Seamlessly looping tracks: `AUDIO_DARKNESS` (Dark World & Field), `Rude Buster` (Battle theme), `The Circus` (Jevil).
- Sound effects: text blips, bell chimes, door creaks, attack slashes, damage hits, soul hurt, graze hums, spell audio, and Jevil's voice clips.

---

## 🕹️ Controls

| Button | Overworld | Battle Menu | Bullet Hell Arena |
|---|---|---|---|
| **D-Pad** | 4-Direction Walk | Navigate buttons & options | 8-Direction Soul movement |
| **B Button** | Dash / Run | Cancel / Back | - |
| **A Button** | Interact / Advance text | Confirm / Strike | - |
| **X Button** | Fast-forward text | - | - |
| **START** | Select / Begin | - | - |
| **SELECT** | Quick return to Title | Return to Title | Return to Title |
| **Touch Screen** | - | Tap to select buttons/options | - |

---

## 📂 Repository Structure

```
├── Deltarune.nds                 # Standalone Nintendo DS ROM
├── Makefile                      # BlocksDS build configuration
├── icon.gif                      # 32x32 NDS banner icon (Red Soul)
├── snail_pie_ds_run              # Snail Pie DS CLI runner
├── snail_pie_ds/
│   ├── snail_pie_ds.py           # Real-time GameMaker asset compiler & watcher
│   ├── qoi_fast.c / libqoi_fast  # Ultra-fast GameMaker QOI texture decoder
│   ├── drop/                     # Drop data.win or assets here for instant compile!
│   ├── extracted_sprites/        # All extracted Deltarune sprites PNGs
│   └── compiled/SnailPieDS.nds   # Compiled Snail Pie DS ROM
├── source/
│   ├── main.c                    # Entry point & game loop
│   ├── renderer.h/.c             # Dual-screen 16-bit graphics engine
│   ├── sound_manager.h/.c        # Hardware PCM sound manager
│   ├── font_data.h/.c            # Determination font tables
│   ├── dialogue.h/.c             # Typewriter dialogue & portraits
│   ├── title_screen.h/.c         # Mode select & Title screen
│   ├── overworld.h/.c            # Caterpillar party exploration
│   ├── battle.h/.c               # Turn-based + Bullet Hell combat engine
│   ├── cutscene.h/.c             # Story sequences
│   ├── assets_gfx.h/.c           # Sprite & texture arrays
│   └── assets_audio.h/.c         # Audio & music samples
└── tools/
    ├── convert_assets.py         # Asset conversion script
    └── convert_font.py           # Font extraction script
```

---

## 🔨 Building from Source

```bash
# Standard build:
make clean && make

# Or with Snail Pie DS live pipeline:
./snail_pie_ds_run
```

