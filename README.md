# Urban Chaos: Riot Response - Burner Edition 0.20

Windows and experimental Linux/Proton share a statically recompiled Xbox game
runtime built on XboxXBERecomp, with maintained Urban Chaos adapters. Linux uses
Proton and is not native. Single-player and local split-screen co-op use one
launcher and main menu.

This source snapshot contains the current build/setup dependency closure,
regression tests and shared asset recipes. It excludes the retired runtime,
original disc data, generated game code, private captures and player state.

A fresh checkout does not yet reproduce the complete executable. See
[build status](docs/BUILDING.md), [source map](docs/CODE_MAP.md),
[changes](docs/CHANGES-0.20.md) and
[runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md) for the supported scope.
Dependency identities and backports are recorded in `dependencies.lock.json`.

Version 0.20 fixes the entity-bucket unlink defect associated with issue #32
and improves local co-op hostage targeting, cinematic ownership, shared native
slow motion, conversation weapon restrictions and Hope Street progression.
The user accepted the corrected Hope Street encounter on Windows; other
encounters and live Proton remain unverified. Co-op remains experimental.
All v0.19 features remain, including lighting presets, both weapon wheels,
the ember pointer, scope, blood pools and TV-audio correction.

Supply your own supported Xbox disc. Optional Spanish voices are prepared from
your European multilingual PS2 disc. No original game audio is distributed.

The launcher can now download and install the pinned official 0.09 HD texture
pack from Manage > Add-ons. Existing local-ZIP installation remains available.
The pack is downloaded separately; it is not added to either game package.
