Urban Chaos: Riot Response — Burner Edition **0.07** is ready, with these updates since **0.06**:

- **Rifle scope:** corrected the remaining widescreen border gaps and incomplete red tint.
- **FOV and first-person arms:** climbing keeps your selected FOV. Improved arm coverage during climbing and shield bashes, corrected stretched sleeve mapping, and aligned the taser's electrical arc at changed FOV settings.
- **Menus:** fixed invisible restart confirmations and Difficulty-menu flicker. Mission/checkpoint restart dialogs now use mouse-friendly controls when playing with keyboard and mouse; controller layouts remain available.
- **Lighting:** fixed the reported flickering wall lighting and restored the original depth-clamp handling.
- **Wall textures:** added a targeted stone-ledge mapping repair for both original and HD textures. The reporter's exact wall still needs visual confirmation.
- **Blue pipe flames:** fixed another cause of invisible flames, where active effects could run out of renderer registrations. This fix does not require reinstalling the HD texture pack.
- **Shader warm-up:** included known shader variants to reduce first-use stutters while preserving your existing learned cache. New shaders and other first-use work can still cause hitches.
- **Performance:** reduced repeated audio decoding using a selected upstream improvement and reduced unnecessary keyboard polling.
- **Controls and ragdolls:** added separate **Next Firearm** and **Previous Firearm** bindings, and restored character collision with ragdoll bodies.
- **Spanish:** added Spanish PC menus and control descriptions, plus language selection in Setup. Additional original-game translations use the updated optional Spanish pack; voices remain in English.
- **Experimental Linux support:** added a separate Proton package with launch/setup scripts for running the same Windows game on Linux. It is **not a native Linux port**, and Linux gameplay is not yet verified.

**Downloads and updating**

Choose the Windows ZIP or the experimental Linux/Proton archive. Linux users need their own Steam/Proton installation; the package includes setup instructions but does not bundle Proton. Source and checksums are also included with this release.

The **HD game texture pack remains a separate download**. The unchanged **0.06 HD pack is compatible with 0.07**; existing users do not need to reinstall it. Spanish game archives are also optional and are not bundled in the base game download. Credit for the HD textures goes to **xXthe.RockoXx**, and for the community Spanish translation to **ZARF**.

For a new Windows installation, extract the complete package and import your supported original Xbox ISO through Setup. Linux users should follow `README-linux.txt` and use `burner.sh`. For an update, copy your existing **GameData** folder beside the new launchers and keep a backup. No original disc files or personal saves are bundled.

On first ISO import, Setup can automatically copy matching saves and settings from the older `%LOCALAPPDATA%\UrbanChaosBurnerEdition` installation. **A freshly extracted folder therefore does not necessarily start with a fresh profile.** Existing portable saves take priority, and the older installation is left untouched.

**Remaining issues**

- HD HUD still has prompt/display issues; the original HUD remains available.
- The intermittent missing injured officer in Angel Heights remains unresolved.
- Spanish additions need community review, and the ledge repair needs confirmation at the original reported location.
- Linux gameplay, full-campaign coverage, and the full range of GPU configurations remain unverified.

Powered by **XboxXBERecomp by TGP482**, with earlier **xboxrecomp work by sp00nz**. Thanks to **xXthe.RockoXx** and **ZARF** for their community texture and translation work.
