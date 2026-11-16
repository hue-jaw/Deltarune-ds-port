# DELTARUNE: Chapter 1 - Nintendo DS Port

A complete homebrew recreation of **Toby Fox's DELTARUNE Chapter 1** built specifically for the **Nintendo DS** and output as a standalone `.nds` ROM playable on real hardware (DS, DS Lite, DSi, 3DS via flashcarts/TwilightMenu++) and all Nintendo DS emulators (melonDS, DeSmuME, Delta, etc.).

---

## 🎮 Features

### 1. Dual-Screen Architecture (Mode 5 2D 16-bit Bitmap)
- **Top Screen (Battle Theater & Overworld View)**:
  - Overworld: 256x192 rendering of Hometown School, Dark Abyss Cliffs, Castle Town, and Field of Hopes and Dreams.
  - Battle: Dynamic perspective battle arena with tiled grid pattern (`bg_battle_tile`), animated party members (Kris, Susie, Ralsei), enemy animations, floating damage numbers, and speech bubbles.
- **Bottom Screen (HUD, Touch Commands & Bullet Hell Arena)**:
  - TP (Tension Points) gauge (0% - 100%) with orange fluid fill.
  - Party stats HUD cards with real-time HP bars.
  - Command interface supporting both D-Pad and Touch Screen input.
  - Dedicated bullet hell arena with smooth 60 FPS Soul dodging.
- **Zero-Tearing Double Buffering**: Offscreen framebuffers in EWRAM blitted to VRAM via DMA on hardware VBlank.

### 2. Overworld Exploration & Party Caterpillar System
- **Party Movement**: Kris leads the party with 4-directional movement and B-dash running.
- **Caterpillar Followers**: Susie and Ralsei trail behind Kris smoothly following his exact movement history.
- **Multiple Rooms**:
  - **Hometown School**: Light World classroom with Toriel, chalkboard, desks, and the ominous supply closet door.
  - **The Dark Abyss / Cliffs**: Kris transformed into blue Dark Knight armor with floating ambient dust particles.
  - **Castle Town**: Castle spire silhouette, starry twilight sky, golden save star, and meeting Prince Ralsei.
  - **Field of Hopes and Dreams**: Indigo turf, trees, patrolling Rudinn encounters, Lancer roadblock, and the hidden Jevil door.
- **Interactive Objects**: Sparkling golden Save Stars (HP recovery & sound fanfare), doors, NPCs, and enemy triggers.

### 3. Story Cutscenes & Typewriter Dialogue System
- **Authentic Determination Font**: Pixel-perfect 8x16 font extracted directly from Deltarune.
- **Character Voice Blips**: Custom sound blips for Kris/narrator, Susie, Ralsei, and Toriel.
- **Portraits**: High-resolution face portraits for Susie (neutral/angry/smile), Ralsei (hat/smile/blush), Lancer, and Toriel.
- **Cinematic Cutscenes**:
  - **Prologue & Awakening**: Toriel waking Kris for school and classroom arrival.
  - **The Dark Fall**: The closet floor collapses into an endless void with dual-screen tumbling physics and rushing speed lines.
  - **The Legend of Delta Rune**: The glowing Delta Rune crest and the ancient prophecy of Light and Dark.

### 4. Turn-Based & Bullet Hell Combat System
- **Party Members**:
  - **Kris**: Sword slashes, ACT commands (Check, Compliment, Courage).
  - **Susie**: Heavy axe attacks, Rude Buster spell (40% TP).
  - **Ralsei**: Defend, Heal Prayer (+45 HP, 32% TP), Pacify.
- **Action Selection**: 5 Icon Buttons (FIGHT, ACT, ITEM, SPARE, DEFEND) controllable with D-Pad or Touch Screen.
- **Attack Timing Gauge**: Moving slider timing mini-game where hitting near the center scores critical damage.
- **Soul Dodging Phase**:
  - Real-time bullet hell arena box opens on the bottom screen.
  - Red Soul heart with 8-directional analog/digital control.
  - **Graze Mechanic**: Grazing close to bullets triggers white aura rings, plays `snd_graze`, and recharges TP (+2% per graze).
  - **Enemy Attacks**:
    - **Rudinn**: Aimed diamond projectile spreads.
    - **Lancer**: Bouncing spade waves.
    - **Jevil**: Spinning Devilsknife scythe blades and chaotic burst patterns with authentic voice clips ("CHAOS, CHAOS!").
  - Invulnerability flashing, screen reactions, victory fanfare, and game-over retry loop.

### 5. Sound & Music Engine
- 8-bit signed PCM hardware audio playback.
- Seamlessly looping music tracks:
  - `AUDIO_DARKNESS` (Dark World & Field ambient theme)
  - `Rude Buster` (Standard battle theme)
  - `The Circus / Dialtone` (Jevil boss theme)
- Full library of Deltarune sound effects: text blips, menu clicks, bell chimes, door creaks, slash impacts, damage hits, soul hurt, graze hums, and spells.

---

## 🕹️ Controls

| Button | Overworld | Battle Menu | Bullet Hell Arena |
|---|---|---|---|
| **D-Pad** | Walk (4 directions) | Navigate options | Move Red Soul (8 directions) |
| **B Button** | Run / Dash | Cancel / Back | - |
| **A Button** | Interact / Advance text | Confirm / Strike | - |
| **X Button** | Fast-forward dialogue | - | - |
| **START** | Title / Confirm | - | - |
| **SELECT** | Return to Title Menu | Return to Title | Return to Title |
| **Touch Screen** | - | Tap to select buttons/options | - |

---

## 📂 Repository Structure

```
├── Deltarune.nds          # Ready-to-play Nintendo DS ROM
├── Makefile               # BlocksDS build configuration
├── icon.gif               # 32x32 NDS banner icon (Red Soul)
├── source/
│   ├── main.c             # Entry point & game state machine
│   ├── renderer.h/.c      # Dual-screen 16-bit double buffered graphics engine
│   ├── sound_manager.h/.c # Hardware PCM sound & BGM streaming manager
│   ├── font_data.h/.c     # Determination 8x16 font bitmap tables
│   ├── dialogue.h/.c      # Typewriter dialogue box & portrait manager
│   ├── title_screen.h/.c  # Mode select & Title screen
│   ├── overworld.h/.c     # Caterpillar party exploration & map triggers
│   ├── battle.h/.c        # Turn-based + Bullet Hell combat engine
│   ├── cutscene.h/.c      # Cinematic story sequences
│   ├── assets_gfx.h/.c    # Converted spritesheets, portraits, tiles
│   └── assets_audio.h/.c  # Converted sound effects & music samples
└── tools/
    ├── convert_assets.py  # Image & audio processing pipeline
    └── convert_font.py    # Font extraction pipeline
```

---

## 🔨 Building from Source

To compile the ROM from source, ensure you have the [BlocksDS](https://blocksds.skylyrac.net/) toolchain installed:

```bash
make clean
make
```

This generates `Deltarune.nds` in the repository root.
