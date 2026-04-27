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

Full port of OpenMW to PS Vita via [vitaGL](https://github.com/Rinnegatamante/vitaGL). Runs Morrowind, Tribunal, and Bloodmoon at 15–30 FPS at 640x368 render resolution (upscaled to native 960x544) with controller input, front touchscreen cursor, and a dynamic fog system that auto-scales draw distance to hold target framerate.

AI Usage: AI Assisted (Dependency/Build system detangling, other odds ends and analysis.)

### Installation

- Install `openmw.vpk` on your Vita using VitaShell
- Copy your Morrowind game data to `ux0:data/openmw/Data Files/`:
  - `Morrowind.esm`, `Morrowind.bsa`
  - `Tribunal.esm`, `Tribunal.bsa` (if GOTY)
  - `Bloodmoon.esm`, `Bloodmoon.bsa` (if GOTY)
  - `meshes/`, `textures/`, `sound/`, `music/`, `fonts/`, `splash/`, `video/`, etc.

- Copy your Morrowind.ini `ux0:data/openmw/`:
- Launch from the home screen (first boot is slow while shaders compile)

- Note to Mac OS users. 
  - Double check your file transfers dont auto add an '_' to the beginning of a file name.
  - If they do please delete that file or you will have errors on boot

### Controls

| Input                              | Action |
|------------------------------------| --- |
| Left stick                         | Move |
| Right stick                        | Look |
| Cross                              | Activate / Talk / Confirm |
| Square                             | Toggle weapon (ready / sheathe) |
| Triangle                           | Toggle spell (ready / unready) |
| Circle                             | Inventory / Back |
| L trigger                          | Jump |
| R trigger                          | Attack / Cast (use equipped) |
| D-pad Up                           | Rest / Wait |
| D-pad Down                         | Sneak |
| D-pad Left                         | Cycle weapon |
| D-pad Right                        | Cycle spell |
| L3 (Top Left Corner Touch Screen)  | Toggle 1st / 3rd person |
| R3 (Top Right Corner Touch Screen) | Quick save |
| Start                              | Game menu |
| Select                             | Journal |
| Hold Select + Start                | Console |
| Front touchscreen                  | Cursor (in menus) |

### Vita Settings

A dedicated Vita tab is available under Options in the game menu:

- Dynamic Fog — auto-shrinks draw distance to hold target FPS
- Dynamic Fog Target FPS — 15 (max distance), 18 (long), or 20 (balanced)
- Dynamic Fog Aggression — Normal, Aggressive, or Very Aggressive (how hard fog reacts to FPS dips)
- View Distance — manual draw distance when dynamic fog is off
- Field of View
- Font Size
- Preload Cell Cache — 1 (default) or 2 (smoother cell transitions, more RAM)

### Mods

Drop full mod folders into `ux0:data/openmw/mods/<name>/`. Plugin files (`.esm`, `.esp`, `.omwaddon`, `.omwscripts`) and `.bsa` archives inside are auto-detected and added to the load order on next boot. Loose meshes/textures can also go directly under `Data Files/`. Saves live in `ux0:data/openmw/saves/` and are interchangeable with PC OpenMW saves.

Recommended starter: [Morrowind Optimization Patch](https://www.nexusmods.com/morrowind/mods/45384) — meaningfully better FPS thanks to simpler meshes.

### Notes

- Video plays without audio
- Shadows, post-processing, water shaders, distant terrain, and groundcover are disabled for performance
- Water and Video tabs are removed from Settings

### Building for Vita

Requires the [VitaSDK](https://vitasdk.org/) toolchain. Docker is easiest:

```bash
$ sudo docker build -f Dockerfile.vita -t openmw-vita .
$ sudo docker create --name openmw-vita-build openmw-vita
$ sudo docker cp openmw-vita-build:/src/build-vita/apps/openmw/openmw.vpk .
```

Or manually:

```bash
$ ./scripts/vita-deps/build-all.sh
$ mkdir build-vita && cd build-vita
$ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/VitaToolchain.cmake ..
$ make -j$(nproc) openmw.vpk-vpk
```

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
