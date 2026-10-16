## Burner Edition 0.03 — PC port

- Moved to **[XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp)** and its Direct3D 12 renderer, with improved performance in testing. Thanks also to **[sp00nz's xboxrecomp](https://github.com/sp00nznet/xboxrecomp)** for the earlier foundation and retained helpers.
- Added game-style mouse menus, keyboard/controller rebinding, profile management and PC prompts. Fixed dialogue Skip prompts, HUD pulse behavior, audio and fog issues.
- Expanded Graphics settings: widescreen, resolution/internal scaling, MSAA, filtering, lighting, fog and ragdoll options.
- Added English, French, Italian and German selection in **Options → Language**; restart to apply. Custom PC settings text remains English.
- Updated ISO setup/direct Play launchers, bundled required C++ runtime DLLs, and disabled routine diagnostics/logging.

**Install:** Extract the Windows x64 ZIP, run **Burner Edition Setup.exe**, and select your supported Xbox ISO/XISO. Then use **Play Burner Edition.exe**. Requires Windows 10/11 x64 and a Direct3D 12-capable GPU. **No original game assets are included.**

**Upgrading:** Verified disc imports are reused. New saves/settings use the separate `xbe-runtime` folder; older saves/settings are preserved but are not automatically migrated.

**Performance:** Dynamic scenery lighting, character lighting, effects and retained bodies can still cause slowdowns, especially during explosions. Adjust Graphics settings as needed; 60 FPS is not guaranteed.
