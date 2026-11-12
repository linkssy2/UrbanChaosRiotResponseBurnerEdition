# Urban Chaos: Riot Response — Burner Edition 0.18

Windows and experimental Linux/Proton share a statically recompiled Xbox game
runtime built on XboxXBERecomp, with maintained Urban Chaos adapters. Linux uses
Proton and is not native. Single-player and local split-screen co-op use one
launcher and main menu.

This source snapshot contains the current build/setup dependency closure,
regression tests and shared asset recipes. It excludes the retired runtime,
original disc data, generated game code, private captures and player state.

A fresh checkout does not yet reproduce the complete executable. See
[build status](docs/BUILDING.md), [source map](docs/CODE_MAP.md),
[changes](docs/CHANGES-0.18.md) and
[runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md) for the supported scope.
Dependency identities and backports are recorded in `dependencies.lock.json`.

Supply your own supported Xbox disc. Optional Spanish voices are prepared from
your European multilingual PS2 disc. No original game audio is distributed.
