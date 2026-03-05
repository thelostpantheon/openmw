OpenMW
======

OpenMW is an open-source open-world RPG game engine that supports playing Morrowind by Bethesda Softworks. You need to own the game for OpenMW to play Morrowind.

OpenMW also comes with OpenMW-CS, a replacement for Bethesda's Construction Set.

* Version: 0.51.0
* License: GPLv3 (see [LICENSE](https://gitlab.com/OpenMW/openmw/-/raw/master/LICENSE) for more information)
* Website: https://www.openmw.org
* IRC: #openmw on irc.libera.chat
* Discord: https://discord.gg/bWuqq2e


Font Licenses:
* DejaVuLGCSansMono.ttf: custom (see [files/data/fonts/DejaVuFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DejaVuFontLicense.txt) for more information)
* DemonicLetters.ttf: SIL Open Font License (see [files/data/fonts/DemonicLettersFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DemonicLettersFontLicense.txt) for more information)
* MysticCards.ttf: SIL Open Font License (see [files/data/fonts/MysticCardsFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/MysticCardsFontLicense.txt) for more information)

PS Vita Port
------------

A native ARM port of OpenMW for the PS Vita, using [vitaGL](https://github.com/Rinnegatamante/vitaGL) for rendering. Runs the full Morrowind game (including Tribunal and Bloodmoon) on handheld hardware.

### Requirements

- A PS Vita or PS TV with [HENkaku](https://henkaku.xyz/) (firmware 3.60 or 3.65 recommended)
- [VitaShell](https://github.com/TheOfficialFloW/VitaShell) for file management and VPK installation
- A legitimate copy of The Elder Scrolls III: Morrowind (Game of the Year Edition recommended)
- A storage card with at least 2GB free (more if using mods)

### Installing on Vita

**1. Install the VPK:**

Transfer `openmw.vpk` to your Vita via USB or FTP, then install it with VitaShell.

**2. Copy Morrowind game data:**

From your Morrowind installation on PC, copy the `Data Files` folder to:

```
ux0:data/openmw/Data Files/
```

The folder structure should look like this:

```
ux0:data/openmw/
  Data Files/
    Morrowind.esm
    Tribunal.esm         (if GOTY)
    Bloodmoon.esm        (if GOTY)
    Morrowind.bsa
    Tribunal.bsa          (if GOTY)
    Bloodmoon.bsa         (if GOTY)
    meshes/
    textures/
    ...
```

**3. Launch** OpenMW from the LiveArea.

First boot will take longer than usual as shaders are compiled and cached.

### Controls

```
Left Analog Stick     Move
Right Analog Stick    Look / Camera

Cross  (X)            Activate / Talk / Confirm
Square                Attack / Use equipped item
Triangle              Jump
Circle                Inventory / Back

L Shoulder            Toggle Weapon (ready/sheathe)
R Shoulder            Toggle Spell (ready/unready)

D-pad Up              Rest / Wait
D-pad Down            Sneak (toggle)
D-pad Left            Cycle Weapon
D-pad Right           Cycle Spell

START                 Game Menu (save, load, settings, quit)
SELECT                Journal

START + SELECT        Console Commands

Front Touchscreen:
  Top-left corner     Toggle 1st/3rd person
  Top-right corner    Quick Save
```

In menus, the right analog stick scrolls lists and text. The on-screen button bar shows context-sensitive actions.

### In-Game Settings

Open the Game Menu (START) and go to the Settings tab. The Vita tab provides hardware-specific options:

- **Viewing Distance** - Draw distance (lower = better FPS)
- **Actors Processing Range** - AI/physics range
- **Camera Sensitivity** - Right stick sensitivity
- **Gamepad Cursor Speed** - Menu cursor speed
- **GUI Scaling Factor** - UI element size
- **Field of View** - Camera FOV
- **Enable VBOs** - Vertex buffer objects (keep enabled)

### Saves

Save games are stored at `ux0:data/openmw/saves/`. They are compatible with the PC version of OpenMW -- you can transfer saves between Vita and PC.
Note: Saves are not directly transferable from original Morrowind save files only other OpenMW saves.

### Adding Mods

Place `.esm`, `.esp`, `.omwaddon`, and `.bsa` files directly in `ux0:data/openmw/Data Files/`. New content files are automatically detected and added to the load order on each boot (ESMs first, then ESPs, alphabetical within each group). To disable a mod, remove the file from `Data Files/`.

For mods that include loose files (meshes, textures), copy them into the corresponding subdirectories under `Data Files/`. Loose files automatically override BSA-packed files.

### Recommended Mod: Morrowind Optimization Patch

[Morrowind Optimization Patch](https://www.nexusmods.com/morrowind/mods/45384) (MOP) by Remiros & Greatness7 replaces vanilla meshes with optimized versions -- fewer polygons, merged geometry. This significantly improves performance on Vita, especially in dense areas like Balmora and Vivec.

**Installation:**

1. Download MOP from Nexus Mods
2. Extract the archive on your PC
3. Copy the contents of `00 Core/meshes/` into `ux0:data/openmw/Data Files/meshes/` on your Vita
4. No config changes needed -- loose mesh files automatically override the BSA-packed vanilla meshes

The optional modules (Lake Fjalding Anti-Suck, Chuzei Fix, etc.) can be installed the same way -- copy their `meshes/` contents into `Data Files/meshes/`, and for modules with an ESP, copy the `.esp` into `Data Files/`.

### Known Limitations

- **Video playback disabled** -- intro/cutscene videos are skipped
- **No distant terrain** -- distant land rendering is disabled for performance
- **No shadows** -- shadow rendering is disabled
- **No post-processing** -- shader post-processing effects are disabled
- **Reduced draw distance** -- default 2250 units (adjustable in settings)
- **Texture quality reduced** -- large textures are automatically downscaled to save memory
- **No groundcover** -- grass/groundcover mods are not supported
- **Cell preloading disabled** -- cells load on demand rather than being preloaded

### Troubleshooting

**Game won't start / crashes immediately:**
- Verify your `Data Files` folder contains `Morrowind.esm` and `Morrowind.bsa`
- Check that `ux0:data/openmw/config/openmw.cfg` exists and lists the correct content files
- Check `ux0:data/openmw/boot.log` for crash information

**Low FPS:**
- Install the Morrowind Optimization Patch (see above)
- Reduce Viewing Distance in settings
- Interiors generally run better than exteriors

**Out of memory crashes:**
- The Vita has limited RAM. Large exterior areas with many objects may cause issues
- Reducing viewing distance helps

Building from Source
--------------------

### Docker Build

The easiest way to build. No toolchain setup required.

```bash
sudo docker build -f Dockerfile.vita -t openmw-vita .
sudo docker create --name vita-build openmw-vita
sudo docker cp vita-build:/src/build-vita/apps/openmw/openmw.vpk .
sudo docker rm vita-build
```

### Manual Build

**1. Install VitaSDK:**

```bash
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH
git clone https://github.com/vitasdk/vdpm.git && cd vdpm
./bootstrap-vitasdk.sh
./install-all.sh
```

**2. Build vitaGL:**

```bash
git clone https://github.com/Rinnegatamante/vitaGL.git
cd vitaGL
make -j$(nproc) \
    HAVE_GLSL_SUPPORT=1 \
    HAVE_UNFLIPPED_FBOS=1 \
    DRAW_SPEEDHACK=1 MATH_SPEEDHACK=1 \
    TEXTURES_SPEEDHACK=1 BUFFERS_SPEEDHACK=1 \
    CIRCULAR_VERTEX_POOL=2 HAVE_WVP_ON_GPU=1 \
    SAMPLERS_SPEEDHACK=1 HAVE_SHADER_CACHE=1
```

**3. Build dependencies (Boost, LuaJIT, FFmpeg, ICU, PVR_PSP2):**

```bash
./scripts/vita-deps/build-all.sh
```

This will take a while.

**4. Build OpenMW:**

```bash
mkdir build-vita && cd build-vita
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/VitaToolchain.cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DVITAGL_DIR=/path/to/vitaGL \
      ..
make -j$(nproc) openmw.vpk-vpk
```

The VPK will be at `build-vita/apps/openmw/openmw.vpk`.

Upstream OpenMW
---------------

Current Status
--------------

The main quests in Morrowind, Tribunal and Bloodmoon are all completable. Some issues with side quests are to be expected (but rare). Check the [bug tracker](https://gitlab.com/OpenMW/openmw/-/issues/?milestone_title=openmw-1.0) for a list of issues we need to resolve before the "1.0" release. Even before the "1.0" release, however, OpenMW boasts some new [features](https://wiki.openmw.org/index.php?title=Features), such as improved graphics and user interfaces.

Pre-existing modifications created for the original Morrowind engine can be hit-and-miss. The OpenMW script compiler performs more thorough error-checking than Morrowind does, meaning that a mod created for Morrowind may not necessarily run in OpenMW. Some mods also rely on quirky behaviour or engine bugs in order to work. We are considering such compatibility issues on a case-by-case basis - in some cases adding a workaround to OpenMW may be feasible, in other cases fixing the mod will be the only option. If you know of any mods that work or don't work, feel free to add them to the [Mod status](https://wiki.openmw.org/index.php?title=Mod_status) wiki page.

Getting Started
---------------

* [Official forums](https://forum.openmw.org/)
* [Installation instructions](https://openmw.readthedocs.io/en/latest/manuals/installation/index.html)
* [Build from source](https://wiki.openmw.org/index.php?title=Development_Environment_Setup)
* [Testing the game](https://wiki.openmw.org/index.php?title=Testing)
* [How to contribute](https://wiki.openmw.org/index.php?title=Contribution_Wanted)
* [Report a bug](https://gitlab.com/OpenMW/openmw/issues) - read the [guidelines](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines) before submitting your first bug!
* [Known issues](https://gitlab.com/OpenMW/openmw/issues?label_name%5B%5D=Bug)

The data path
-------------

The data path tells OpenMW where to find your Morrowind files. If you run the launcher, OpenMW should be able to pick up the location of these files on its own, if both Morrowind and OpenMW are installed properly (installing Morrowind under WINE is considered a proper install).

Command line options
--------------------

    Syntax: openmw <options>
    Allowed options:
      --config arg                          additional config directories
      --replace arg                         settings where the values from the
                                            current source should replace those
                                            from lower-priority sources instead of
                                            being appended
      --user-data arg                       set user data directory (used for
                                            saves, screenshots, etc)
      --resources arg (=resources)          set resources directory
      --help                                print help message
      --version                             print version information and quit
      --data arg (=data)                    set data directories (later directories
                                            have higher priority)
      --data-local arg                      set local data directory (highest
                                            priority)
      --fallback-archive arg (=fallback-archive)
                                            set fallback BSA archives (later
                                            archives have higher priority)
      --start arg                           set initial cell
      --content arg                         content file(s): esm/esp, or
                                            omwgame/omwaddon/omwscripts
      --groundcover arg                     groundcover content file(s): esm/esp,
                                            or omwgame/omwaddon
      --no-sound [=arg(=1)] (=0)            disable all sounds
      --script-all [=arg(=1)] (=0)          compile all scripts (excluding dialogue
                                            scripts) at startup
      --script-all-dialogue [=arg(=1)] (=0) compile all dialogue scripts at startup
      --script-console [=arg(=1)] (=0)      enable console-only script
                                            functionality
      --script-run arg                      select a file containing a list of
                                            console commands that is executed on
                                            startup
      --script-warn [=arg(=1)] (=1)         handling of warnings when compiling
                                            scripts
                                            0 - ignore warnings
                                            1 - show warnings but consider script as
                                            correctly compiled anyway
                                            2 - treat warnings as errors
      --load-savegame arg                   load a save game file on game startup
                                            (specify an absolute filename or a
                                            filename relative to the current
                                            working directory)
      --skip-menu [=arg(=1)] (=0)           skip main menu on game startup
      --new-game [=arg(=1)] (=0)            run new game sequence (ignored if
                                            skip-menu=0)
      --encoding arg (=win1252)             Character encoding used in OpenMW game
                                            messages:

                                            win1250 - Central and Eastern European
                                            such as Polish, Czech, Slovak,
                                            Hungarian, Slovene, Bosnian, Croatian,
                                            Serbian (Latin script), Romanian and
                                            Albanian languages

                                            win1251 - Cyrillic alphabet such as
                                            Russian, Bulgarian, Serbian Cyrillic
                                            and other languages

                                            win1252 - Western European (Latin)
                                            alphabet, used by default
      --fallback arg                        fallback values
      --no-grab [=arg(=1)] (=0)             Don't grab mouse cursor
      --export-fonts [=arg(=1)] (=0)        Export Morrowind .fnt fonts to PNG
                                            image and XML file in current directory
      --activate-dist arg (=-1)             activation distance override
      --random-seed arg (=<impl defined>)   seed value for random number generator
