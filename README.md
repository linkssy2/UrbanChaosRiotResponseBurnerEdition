# Urban Chaos: Riot Response - Burner Edition 0.21

Windows and experimental Linux/Proton share a statically recompiled Xbox game
runtime built on XboxXBERecomp, with maintained Urban Chaos adapters. Linux runs
the Windows runtime through Proton; it is not native. Single-player and local
split-screen co-op use one launcher and main menu.

This source snapshot contains the active build/setup dependency closure,
regression tests and shared asset recipes. It excludes original disc data,
generated game code, private captures, player state and the retired runtime.
A fresh checkout does not yet reproduce the complete executable. See
[build status](docs/BUILDING.md), [source map](docs/CODE_MAP.md),
[changes](docs/CHANGES-0.21.md) and
[runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md).
Dependency identities and backports are recorded in `dependencies.lock.json`.

Version 0.21 addresses additional Angel Heights minigun-sequence crash paths
by extending entity registration and the matching update, render and cleanup
lists beyond their original limits. It also guards optional muzzle flashes
against missing or stale owners. Forever ragdolls remain available without
changing the selected retention setting or forcibly removing retained bodies.
The tester reported that turning Forever off prevented the crash; a replay of
this candidate with Forever enabled remains unconfirmed.

The runtime passed 193 automated test steps, including registration and cleanup
of 3,075 entities. New capacity fixtures also passed on native Linux. These
checks do not establish a complete gameplay replay, long-session/save-load
acceptance or live Proton/Steam Deck compatibility. Co-op remains experimental.
Previous fixes and features remain included.

Supply your own supported Xbox disc. Optional Spanish voices are prepared from
your European multilingual PS2 disc. No original game audio is distributed.
The launcher can download and install the pinned official 0.09 HD texture pack
from Manage > Add-ons, or install an existing local ZIP. The pack remains a
separate download and is not included in either game package.
