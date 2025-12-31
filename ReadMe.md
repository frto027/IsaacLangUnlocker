use cmake with win32 config to generate vs project

## Message from author

This sloppy project can be used to enable isaac's localization feature, which was disabled in the rep+ beta test.

This is developed for the (un)official Chinese translation patch. But other languages still can be configured with a `config.ini` file.

Feel free to contact me via issue or some other ways if you are intrested with this.

see OpenSourceLicense.txt for the licence advice.

## Technical notes / How it works

Please understand we did many code designs to make you feel comfortable. We were never designed to be offensive.

### About the DLL mod

DLL mods are intresting. It's intresting that you can do everythings inside the game. But it's not intresting because mod developers can do everything including something bad. And most mod developers can't do anything good because they don't know how to code the game. C/CPP language is hard. It's a complete mess for beginners.

**This is a DAMOCLES.** I would say that many games give mod developers a great deal of system privileges. You can develop dll/jar mod in any unity game or java game, including slay the spire, minecraft, celeste, beat saber... But why not the isaac game when needed?

> [!TIP]
> If this mod is flagged as a virus by your antivirus software, you can contact me. I will contact the antivirus software vendor manually, and provide the sample to prove its innocence. Please understand that I have paid my time to this project, but I will not spend money to purchase certificates for this project to prove the innocence.

### Mod Loader via exe Patch

We have a DLL mod loader with auto-update ability, but the mod loader can't update itself, with the guide of Keep-It-Simple-Stupid.

Player is required to run `patcher.exe` at the first time. see `exe_patcher` directory for source code.

The patcher replace the string `userenv.dll` to `bootstp.dll` inside the `isaac-ng.exe`, and extract a `bootstp.dll`. The game will load `bootstp.dll` when it trying to obtain the save data path.

> For the earlier version, I use `userenv.dll` directly, without the exe-patch. But some player can't load it, and some player loaded it but it breaks some third-party programs such as IME that also depends on the `userenv.dll` inside the `isaac-ng.exe`. And some player is confused about how to uninstall the mod because the original game file is not changed. It's good design because player don't need re-patch after the game update, but some player confuse about this so I removed this design.

The `bootstp.dll`, which is compiled from directory `bootstrap`, will try to sync a file called `inject.bin` from the mod directory to the game install folder `inject.dll` at every game start. Then `LoadLibrary("inject.dll")` and call `Load(modPath)` inside the `inject.dll`.

The mod loader is not a version-related design, it will succees at every game version, including the future game update.

You'd notice that the mod loader actually update a dll file from network and run it at every game start. This will make some player unconfortable, so I made a prompt box before every update. You can click 'No' to stop the code update, so it's not offensive. 

### Mod Loader via repentogon
> [!NOTE]
> The old mod actually works with rgon natually, without any modification, if you put the userenv.dll to the rgon's game folder and patch the game manually. I just re-designed the mod to make it work in steam workshop.

The only changes is that, the repentogon's mod-loader loads our mod-loader, it is cascaded.

For the rgon version of chinese patch, the isaac-ng.exe is not patched. The `bootstp.dll` is renamed to `zhlLangHack.dll` and a function called `ModInit` is added so this dll is loaded by [the repentogon's Mod Loader](https://github.com/TeamREPENTOGON/REPENTOGON/blob/0db6d0ca7ec5286318f774ca388195cf06109fd2/loader/loader.cpp#L143).

### Inject.dll

The inject.dll can be updated by the mod loader, so we had no pressure to fully test it. That's right, I'm confident it will work well. Even if it doesn't work well, I can update it, the player don't need do anything.

> [!TIP]
> All things in this section maybe changed in the future. Because the inject.dll can be auto updated and totally changed.

The `inject.dll` reads `config.ini` inside the game folder. And reads `data/xxx` as an input from mod config menu. Player don't need to change config.ini unless they want do some hack.

The resource files are replaced by `inject.dll`. So we can control the replace progress in future update.

We do have several patchers inside the inject.dll. They are carefully designed to not break the game. I'd say the online game works well with this design.

### The magic of `.a` file

Distributing scattered game resources is complex. Fortunately, with `.a` files, we can overwrite every resources of the original game, including the IID things and string table things, which is something scattered resources cannot do. I made a `.a` file pack tool. It's not open source just because I'm lazy and nobody else need it.

