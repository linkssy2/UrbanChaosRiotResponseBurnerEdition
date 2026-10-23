Urban Chaos 0.07 Linux/Proton black-screen investigation — 2026-10-23

Report: game starts with audio and an FPS counter, but the scene is black.
Reporter hardware, distribution and Proton version were unavailable.

Upstream leads (primary sources checked):
https://github.com/TGP482/SCDA-XBERecomp/issues/19
https://github.com/TGP482/BLACK-XBERecomp/issues/69
https://github.com/TGP482/XboxXBERecomp
https://github.com/ValveSoftware/Proton/wiki/Changelog

The related projects report shader-compiler compatibility problems under Proton.
The active Urban Chaos renderer calls D3DCompile for guest graphics and skips
draws after compiler failure. The independent FPS overlay can still display.
The upstream shader generator includes Microsoft-specific 1.#INF and -1.#INF
tokens in common vertex helpers. Wine rejects those tokens when parsing HLSL.

Reproduction:
- Used the exact 185 source/entry/profile/flags records from the released seed.
- Used an isolated official WineHQ 11.20~jammy-1 extraction in WSL Ubuntu 22.04.
- Explicitly selected Wine's builtin d3dcompiler_47. No game or GPU was launched.
- Original source: 96 compiled, 89 failed with syntax errors.
- Revised source: all 185 compiled. The invalid-source control still failed.
- On Microsoft SDK and Windows system compilers, all 185 original/revised pairs
  produced identical bytecode. Four additional zero/negative-zero/finite helper
  tests also produced identical native bytecode.

Correction:
- The maintained private shader preparation replaces the two affected helper
  lines with asfloat(0x7f800000u) and asfloat(0xff800000u), retaining exact IEEE
  positive/negative infinity. The pinned upstream checkout remains untouched.
- Regenerated all 185 seed records with the same system compiler identity as
  the published seed. 89 source identities change; cache validation is intact.
- Existing installed learned caches, saves and settings are preserved.
- No Microsoft compiler DLL is redistributed and no DLL override is imposed.
- Linux instructions now recommend Steam first, with direct Proton advanced.

Verification:
- Release build and its automated runtime checks passed.
- Launcher/package, portable install/update, shader cache, Spanish and Linux
  wrapper checks passed. TAR contents were reread and hashes matched exactly.
- Previous runtime EXE/PDB and shader seed are preserved in this evidence area.
- Candidate identity and package hashes are in verification.json.
- No existing GitHub release or public download was replaced.

Limits:
This proves a real shader-compiler defect and its compilation correction. It is
not a live Linux/Proton GPU test or proof of the reporter's exact cause. Actual
Proton presentation, GPU rendering, FMVs, audio and controls require visual
acceptance on Linux. The seed only covers the observed 185 shader variants.
