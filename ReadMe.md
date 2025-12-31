use cmake with win32 config to generate vs project

## Message from author

Hey, you finally found this project.

This sloppy project can be used to enable isaac's localization feature, which was disabled in the rep+ beta test.

This is developed for the (un)official Chinese translation patch. But other languages still can be configured with a `config.ini` file.

Feel free to contact me via issue or some other ways if you are intrested with this.

see OpenSourceLicense.txt for the licence advice.

## Developer notes / How it works

### About the DLL mod

DLL mods are intresting. It's intresting that you can do everythings inside the game. But it's not intresting because mod developers can do everything including something bad. And most mod developers can't do anything good because they don't know how to code the game.

**This is a DAMOCLES.** I would say that many games give mod developers a great deal of system privileges. You can develop dll mod in any unity game or java game, including slay the spire, minecraft, celeste, beat saber... But why not the isaac game when needed?

If this mod is flagged as a virus by your antivirus software, you can contact me. I will contact the antivirus software vendor manually, and provide the sample to prove its innocence. Please understand that I have paid my time to this project, but I will not spend money to purchase certificates for this project to prove the innocence.

### Mod Loader via Exe patch

We have a DLL mod loader with auto-update ability, but the mod loader can't update itself, with the guide of Keep-It-Simple-Stupid.

Player is required to run `patcher.exe` at the first time. see `exe_patcher` directory for source code.

The patcher replace the string `userenv.dll` to `bootstp.dll` inside the `isaac-ng.exe`, and extract a `bootstp.dll`. The game will load `bootstp.dll` when it trying to obtain the save data path.

The `bootstp.dll`, which is compiled from directory `bootstrap`, will try to sync a file called `inject.bin` from the mod directory to the game install folder `inject.dll` at every game start. Then `LoadLibrary("inject.dll")` and call `Load(modPath)` inside the `inject.dll`.

The mod loader is not a version-related design, it will succees at every game version, including the future game update.

### Mod Loader via repentogon

The only changes is that, the repentogon's mod-loader loads our mod-loader, it is cascaded.

For the rgon version of chinese patch, the isaac-ng.exe is not patched. The `bootstp.dll` is renamed to `zhlLangHack.dll` and a function called `ModInit` is added so this dll is loaded by [the repentogon Mod Loader](https://github.com/TeamREPENTOGON/REPENTOGON/blob/0db6d0ca7ec5286318f774ca388195cf06109fd2/loader/loader.cpp#L143).

### Inject.dll

The inject.dll can be updated by the mod loader, so we had no pressure to fully test it. That's right, I'm confident it will work well. Even if it doesn't work well, I can update it without the player having to do anything.

> All things in this section maybe changed in the future. Because the inject.dll can be auto updated and totally changed.

The `inject.dll` reads `config.ini` inside the game folder. And reads `data/xxx` as an input from mod config menu. Player don't need to change config.ini unless they want do some hack.


