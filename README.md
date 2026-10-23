# Pokémon Platinum PC Port — Single-Screen Mod

This is a mod for the native PC port of Pokémon Platinum, built on top of **[pret's Pokémon Platinum decompilation](https://github.com/pret/pokeplatinum)** and **[cybervisi0n's Pokémon Platinum PC port](https://github.com/cybervisi0n/pokeplatinum)**.

The focus here is a widescreen, single-screen experience, with adapted interfaces, expanded overworld rendering, and quality-of-life improvements.

**You must provide your own dumped US Pokémon Platinum ROM (CPUE01).** Game assets are extracted from that ROM on first launch.

## Features

Configurable gameplay modifiers below are opt-in through `gameplay_config.ini`; built-in gameplay defaults follow original Platinum behavior.

- **Single-screen presentation:** all screens reworked to make the game playable with just the top screen
- **Widescreen rendering:** broader overworld views, adjusted world
- **Three save slots:** select a slot from the main menu
- **Pokétch in the in-game menu:** open it after acquiring it, switch apps with L/R, and return to the game with B or X.
- **Quit from the in-game menu:** a confirmation prompt lets you cancel before closing the game.
- **QoL Repels:** when a Repel wears off, you can use another without reopening the Bag. If that type has run out, an available alternative is offered.
- **Forgettable HMs:** HM moves can be replaced when learning moves, including after leveling up in battle.
- **Free move relearning:** the Move Maniac in Pastoria City no longer requires Heart Scales.
- **Free shard tutors and main-story access:** shard tutors teach for free, and the Route 212 tutor offers all three tutors' move lists, including Survival Area moves.
- **Unlimited Great Marsh steps:** explore until you retire or run out of Safari Balls; the entry fee and Ball limit still apply.
- **Faster egg hatching:** optionally remove egg cycles and reduce steps between hatch checks.
- **Instant Honey Trees:** slather Honey, then interact again immediately for the encounter. Original encounter tables, failure chances, and Honey costs remain.
- **Reusable TMs:** teaching a move no longer consumes its TM. Selling or discarding a TM still removes it; obtain another to use it again.
- **Easier evolutions:** plain trade evolutions also evolve from level 40; held-item evolutions trigger on the next level-up with their required item at any time of day. Feebas evolves with a Water Stone.
- **Instant Spiritomb:** inserting an Odd Keystone starts the encounter without Underground conversations.
- **No overworld poison damage:** poisoned party Pokémon retain their HP while walking or running.
- **Stability and visual fixes:**

## Visual upgrades

Richer day/night lighting, regional haze, and smooth 3D edges are enabled by default.
Toggle them under Tab → Application → Config → Graphics.
See [graphics notes](docs/visual_upgrades.md).

## How to run

Use a build from this fork's [Releases](https://github.com/elyro1/pokeplatinum/releases) or [GitHub Actions artifacts](https://github.com/elyro1/pokeplatinum/actions). Place your dumped US Platinum `.nds` ROM beside the executable and launch the game: missing assets are extracted automatically. Filenames do not matter; ROMs are identified by their internal game codes. If no compatible ROM is found, the game opens the usual file picker.

Optionally place your dumped **US Black 2 or White 2** `.nds` ROM beside the executable too. On startup, the game extracts the required battle assets into `bw2/` and automatically enables **BW2 battle visuals**, saving the setting. Existing assets are reused. While a compatible BW2 ROM remains present, startup enables the setting again; move the ROM away if you want to keep BW2 visuals disabled. Platinum is still required.

Discovery checks the executable's directory first, then the working directory. Extraction uses the working directory, alongside settings and saves; normally this is the game folder. You can also select a BW2 ROM manually under **Tab → libntr → Application → Config**.

## Default keybindings

These are the defaults for a fresh configuration. Existing settings retain your previous bindings. Open **Tab → Input** to remap keyboard and controller buttons.

| Key | DS button / action |
| --- | --- |
| Arrow keys | D-pad: move / navigate |
| Z | A: confirm / interact |
| X | B: cancel / run where available |
| C | X: open / close the in-game menu |
| V | Y: use a registered item |
| Q | L; previous Pokétch app while the Pokétch is open |
| E | R; next Pokétch app while the Pokétch is open |
| Enter | Start |
| Right Shift | Select |
| Tab | Toggle PC settings / debug tools |
| Left Shift (hold) | Fast-forward (5× by default) |
| F11 | Toggle fullscreen |
| Left mouse button | Touch input where supported by the displayed interface |

Inside the Pokétch, **X** or **C** on the keyboard closes it (DS B or X).

In **Tab → Input → Gamepad**, each direction has Primary and Secondary bindings. The D-pad is Primary and the left stick is Secondary by default (axis 0 for left/right, axis 1 for up/down), so either control works for movement and menus. Click **Clear** to remove a secondary binding; press Escape to cancel controller binding capture. Saved mappings, including cleared secondary bindings, are preserved; configurations without secondary bindings inherit the left-stick defaults.

The **Axis deadzone** slider applies to all mapped controller axes and saves automatically. Its default is 25%; set it to 0% to disable filtering, or increase it to prevent stick drift. **Reset** restores the default. Movement remains digital, with the original game’s directional behavior and speed.

### Settings

The default layout shows one screen, with 60 FPS and Smooth battle pacing enabled, and Normal Battle UI & Timing disabled by default. Speed correction is automatic at 60 FPS. Existing configuration files may override these defaults.

Press **Tab** to access:

- **libntr → Config:** screen layout, fullscreen, internal resolution, volume, frame pacing, and fast-forward speed (1×–10×).
- **libntr → Input:** keyboard and controller mappings.
- **libntr → Application → Config:** frame rate (30/60 FPS), battle pacing (Original/Smooth/Quick), and the independent Normal Battle UI & Timing checkbox. Smooth keeps original animation timing with sprite interpolation; Quick accelerates battle effects while keeping Pokémon idle animations and idle camera movement at Smooth speed. The checkbox keeps HP/EXP bars, messages and UI delays at normal speed when enabled. See [battle pacing settings](docs/battle_speed_settings.md) for details.
- **libntr → Application tools:** cheats, Pokémon and item generation, and map jumping while a game is loaded.

Settings are stored locally in `sim_config.ini` and `sim_config_prj.ini`. If performance is poor, lower the internal resolution scale in the PC settings.

Gameplay options load built-in defaults from `src/port/gameplay_config.c`, then apply optional overrides from `gameplay_config.ini` in the game folder (the same working directory as `sim_config_prj.ini` and saves). In **libntr → Application → Config → Gameplay**, click **Generate gameplay config** to create a commented file containing every option and its default value. The button preserves any existing file. Edit the file and restart the game to apply changes; delete it and restart to restore built-in defaults. The game does not create this file automatically.

The built-in defaults follow original Platinum gameplay. Existing override files retain their configured values; remove the file and restart to use the new defaults. You can also create a partial file yourself, for example to disable battle items while keeping all other defaults:

```ini
[Gameplay]
prohibitBattleItems=true
```

Boolean values accept `true`/`false`, `TRUE`/`FALSE`, or `1`/`0`. Numeric ranges are listed in the generated file. Missing or invalid values keep defaults, and unknown keys are ignored. A malformed INI file falls back to all built-in defaults.

All boolean gameplay modifiers default to `false`. Enable individual options with `true`:

- `prohibitBattleItems` blocks player medicine and battle items, while allowing Poké Balls, held items, and opponents' items. `forceSetBattleStyle` suppresses the Shift prompt regardless of the saved Options selection.
- `unlimitedVitaminEVs` allows vitamins beyond 100 EVs while retaining the normal per-stat and total limits. EV-reducing berries retain their normal behavior.
- `forgettableHMs` allows HM replacement when learning moves. `infiniteTMs` prevents TM consumption after teaching; selling or discarding TMs still removes them.
- `freeMoveRelearning` removes the Move Maniac's Heart Scale requirement. `freeMoveTutors` removes shard requirements and payments. `mainStoryMoveTutors` makes all three shard tutors' move lists available from the Route 212 tutor; species compatibility and known-move checks remain.
- `unlimitedGreatMarshSteps` removes the step limit while retaining the entry fee and Safari Ball limit.
- `zeroEggCycles` makes new and existing eggs ready to hatch at their next hatch check. Disabling it does not restore cycles already removed from existing eggs. With it disabled, Flame Body/Magma Armor bonuses apply normally.
- `instantHoneyTrees` skips the six-hour wait, including for already-slathered trees. Encounter chances, Honey costs, and the 24-hour expiration remain.
- `levelUpHeldItemEvolutions` triggers held-item evolutions on the next level-up at any time of day, consuming the required item. Normal trade evolution remains available.
- `feebasWaterStoneEvolution` replaces Feebas's Beauty requirement with a Water Stone. `instantSpiritomb` skips Underground conversations, while retaining the Odd Keystone requirement and normal encounter outcome.
- `disableOverworldPoisonDamage` skips walking poison damage and its effects while preserving poison status and battle damage.

`tradeEvolutionLevel` defaults to 0, retaining trade-only evolutions. Set it to 1–100 to also trigger plain trade evolutions on level-up at or above that level. Held-item trades use `levelUpHeldItemEvolutions` instead.

`shinyRolls` defaults to 1, keeping original shiny odds. Higher values add rolls for new eligible Pokémon; existing Pokémon, fixed-personality gifts, and shiny locks are preserved. A value of 0 also keeps original odds.

`friendshipGainMultiplier`, `eggHatchSpeedMultiplier`, and `catchRateMultiplier` all default to 1, preserving original behavior (0 also preserves it). Values from 2–255 multiply positive friendship gains, divide steps per hatch check, or multiply final catch rates, respectively. Friendship still caps at 255; losses and initial friendship are unaffected. Hatch checks take at least one step. Master Balls and guaranteed catches still work, and trainer Pokémon cannot be caught.

BP prices default to their original values: `bpPricePercent=100` and `rareCandyBPPrice=48`. The percentage scales other BP prices, rounded up (50 = half, 0 = free). Rare Candy uses its independent exact price (0 = free). These settings affect displayed prices and payment together, without changing shop inventories or cash prices.

## Saves

The three slots use `save1.bin`, `save2.bin`, and `save3.bin` in the game folder. Save through the normal in-game Save action before quitting.

An older upstream `save.bin` is copied into slot 1 when `save1.bin` does not already exist. The original file is retained. Back up your save files before updating builds or deleting a slot.

## Building from source

Clone **this fork**:

```bash
git clone https://github.com/elyro1/pokeplatinum.git
cd pokeplatinum
```

The Makefile fetches Meson and the compiler tools as needed, and Meson fetches the SDK subprojects. The initial build requires network access. You still need your own ROM to run the PC port.

### Linux with Docker

Install Docker and ensure your user can run it, then run:

```bash
./drun.sh make linux
```

The script builds the container image on first use. Output is written to `build_linux/pokeplatinum`, including `main` and `pokeplatinum.AppImage`.

### Linux without Docker

Use a 64-bit Linux environment. The build requires GCC/G++, Make, Python 3, Git, Ninja, NASM, Flex, Bison, pkg-config, the ARM GNU toolchain, libpng, SDL2, ENet, and SoundTouch development packages. Some build tools also require 32-bit compatibility libraries; AppImage packaging requires FUSE.

For Debian / Ubuntu, the core dependencies are:

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install build-essential python3 git ninja-build nasm flex bison \
    pkg-config gcc-arm-none-eabi libpng-dev libsdl2-dev libenet-dev \
    libsoundtouch-dev libc6:i386 wget ca-certificates fuse file patchelf
make linux
```

Refer to [the build container](tools/docker/Dockerfile) for the full container environment and [INSTALL.md](INSTALL.md) for additional setup information inherited from pret. Its ROM-build instructions are separate from the PC targets here.

### Windows

From a fresh clone, run the included setup script in PowerShell:

```powershell
.\Install_MSys2.ps1
```

This creates a portable MSYS2 environment and installs dependencies. Then launch its shell:

```powershell
.\Launch_MSys2.ps1
```

In that shell, build with:

```bash
make win64
```

The game and its required DLLs are placed in `build_win64/pokeplatinum`.

### Other build targets

The inherited Nintendo Switch target requires DevKitA64 (included in the Docker environment):

```bash
./drun.sh make nx
```

This target is experimental and has poor performance; the changes in this fork primarily target PC play.

The source tree also retains the Nintendo DS ROM target:

```bash
make
```

That output is `build/pokeplatinum.us.nds`. PC-specific features do not necessarily apply to the DS build. See [INSTALL.md](INSTALL.md) for ROM revisions and debugger setup.

### Development notes

- To update SDK subprojects, run `make update`.
- To enable Tracy profiling for a Linux build, run:

  ```bash
  ./drun.sh ./subprojects/meson-1.12.0/meson.py configure -Dtracy_enable=true build_linux
  ```

  Profiling is also available for the Win64 and NX build targets.
- `firmware.bin` is a placeholder for Wi-Fi configuration storage, not a dump of real DS firmware.

## Credits and upstream projects

- **[pret/pokeplatinum](https://github.com/pret/pokeplatinum):** Pokémon Platinum decompilation, documentation, and the original source tree.
- **[cybervisi0n/pokeplatinum](https://github.com/cybervisi0n/pokeplatinum):** the native PC port on which this fork is built.
- **[cybervisi0n/libntr](https://github.com/cybervisi0n/libntr):** the NitroSDK replacement libraries powering the port.
- **[elyro1/libntr](https://github.com/elyro1/libntr/tree/lib_single_screen_mod):** the modified library branch used by this fork.

Please report issues specific to this fork in [this repository's issue tracker](https://github.com/elyro1/pokeplatinum/issues). The original PC port's community is available on its [Discord](https://discord.gg/ZgtPszuBeN).
