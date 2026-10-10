## Burner Edition 0.02 — compatibility hotfix

- Added support for the verified USA/Europe retail ISO and its XISO conversion, alongside the previously supported copy.
- Fixed the missing mouse hold-and-drag visual cue when accepting healing from a medic.
- Removed embedded original HUD artwork; it is now generated from your imported disc with unchanged pixels and custom PC prompts.
- Existing imports, settings and saves remain compatible. Disc compatibility passed user gameplay testing. The final HUD extraction change passed byte-for-byte pixel, renderer/input and import checks; it has not had another live playthrough.

**Install:** Extract the Win64 ZIP, run **Burner Edition Setup.exe**, and select your supported Xbox ISO/XISO. After import, use **Play Burner Edition.exe**. Existing users can extract the update to a new folder and reuse their installed data.

An unofficial Windows x64 hybrid recompilation using [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp) and native PC engine components, retaining original gameplay and mission logic.

**Performance:** Dynamic scenery lighting and character lighting can cause slowdowns. Lower their settings in Graphics and press Apply. Routine runtime logging and diagnostic recorders remain disabled.

No ISO, standalone Xbox executable, bulk game data, videos or saves are bundled. Original HUD artwork is also sourced from your disc rather than bundled.

Source/build limitations remain documented in docs/BUILDING.md. This hotfix does not complete the standalone rebuild pipeline.
