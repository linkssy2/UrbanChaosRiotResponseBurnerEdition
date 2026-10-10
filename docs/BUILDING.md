# Source and build status

This snapshot includes handwritten integration, native components, launcher code,
release metadata, and the local xboxrecomp changes. It excludes the ISO, extracted
resources, generated game C, disassembly, machine-specific captures and private saves.

The launcher can be rebuilt on Windows with the .NET Framework C# compiler using
`launcher/burner/build.ps1`. Its output directory is set in `release.json`; keep the
release ZIP's supporting folders beside the rebuilt EXEs. The launcher source does
not need game assets to compile.

The complete game executable cannot currently be reproduced from this snapshot alone.
The development build selects frozen generated units and prior proof artifacts from
local build directories. Those inputs require regeneration from the supported disc;
the self-contained release build pipeline is unfinished. Do not treat the historical
root build scripts as a verified reproduction of the 0.01 executable.

The packaged game executable is byte-identical to the tested freeze-fix runtime:
`844233e2a2733b09ec7e7fcf041efd38a736827330995deb2fbf13d4a26852e1`.
Routine logging and instrumentation are disabled through PLAY_ONLY, LEAN_RUNTIME and
RELEASE_NO_LOG on the seven targets (game plus six subsystems). Crash-error reporting remains available after failure.

Names such as CAPTURE_DIRECT and LIGHT_WRITE_WATCH select rendering/cache correctness
paths, not recording sessions; removing them indiscriminately changes behavior.

## Review scope

Release review covers import validation/cancellation, settings preservation, package
contents, runtime identity, logging configuration, fixed-pool ownership and the GPU
wait repair. Offline checks passed; the tester reported the freeze fix working.
This is not an exhaustive audit of every engine function or a full-game completion test.

The generated authored-program comparison header `hybrid_base_pgraph_contract.h`
and two mechanically lifted `.inc` bodies are also excluded from public source.
