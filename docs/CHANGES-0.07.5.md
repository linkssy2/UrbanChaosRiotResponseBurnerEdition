**Linux-only compatibility test/patch for 0.07. This is a regular release, but Linux gameplay remains experimental. Windows users should keep using [v0.07](https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition/releases/tag/v0.07).**

- Corrected shader syntax rejected by Wine, addressing a verified cause of a black screen with audio and an FPS counter. All 185 captured shader variants now compile in the Wine test; 89 failed before the fix.
- Refreshed the bundled shader cache while preserving existing learned caches, saves and settings. Windows compiler comparisons produced identical shader bytecode.
- Rewrote the Linux instructions around Steam and Proton, with clearer import, play, language, HD texture and troubleshooting steps.
- Includes the integrated Spanish translation and all 0.07 features. The existing HD game texture pack is unchanged and remains optional.

**Download:** `UrbanChaos-BurnerEdition-0.07.5-Linux-Compatibility-Test.tar.gz` is the complete Linux package, not a patch to copy over the game executable. Extract it and follow `README-linux.txt`, using the recommended Steam setup. Requires Linux x86-64, Steam/Proton, suitable Vulkan drivers and your supported original Xbox ISO or previously imported `GameData`.

**Updating:** back up and copy your existing `GameData` into the new extracted folder. Update your existing Steam shortcut to the new folder, then launch through the documented worker. It updates the runtime and bundled shader seed automatically. No ISO reimport or HD texture reinstall is needed for an existing working import.

The optional [HD game texture download](https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition/releases/download/v0.07/UrbanChaos-BurnerEdition-0.06-HD-Game-Textures.zip) is the same pack used by 0.07. Spanish text is already included; no separate language download is needed. Voices remain in English when using Spanish.

**Testing limits:** release/build and package checks passed, along with shader compilation using Wine 11.20. Live Proton rendering, gameplay, FMVs, audio and controller behavior still need confirmation on Linux hardware; this is not a native Linux port or a guarantee that every black-screen cause is resolved. HD HUD retains its known prompt/display issues.

If it still fails, use the optional Proton logging steps in `README-linux.txt` and report your Linux distribution, GPU/driver, Proton version and what appears on screen.
