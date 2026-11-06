# Optional diagnostic logging

In **Burner Edition Setup → Manage → Support**, turn **Diagnostic Logging** on before
reproducing a problem. It applies to subsequent setup actions and the next game
session. Use **Open Logs** afterward and share the relevant files manually.

Logging is off by default. Files stay in the launcher's `GameData/logs`, including
when an existing installation is selected elsewhere. Nothing is uploaded.

On Linux/Proton, temporarily use this Steam shortcut's Launch Options:

```text
-B "tools/import_game.py" diagnostics --logging on --interactive
```

Press Play, acknowledge the message, then restore the usual game launch options.
Use `off` in the same command to disable logging. The advanced terminal wrapper
also accepts `./burner.sh diagnostics on` and `./burner.sh diagnostics off`.

Logs include setup progress/errors, runtime version/hash, saved settings,
startup system/device details, existing runtime reports/warnings/errors, periodic
presentation FPS samples and the game's exit code. The terminal wrapper also
retains bounded Proton startup output once it reaches the worker-launch step.
The Steam shortcut cannot capture failures in Proton before the worker starts.

This is support logging, not a full performance trace: it does not enable the
developer GPU/frame captures, heap probes, input recordings, audio recordings or
memory dumps. FPS samples do not identify individual CPU/GPU bottlenecks. Local
paths, usernames, device names and control settings may appear; review before
sharing. A read-only/full disk may prevent logging, but must not prevent play.

Runtime sessions are capped at 64 MiB, launcher actions and terminal Proton output
at 8 MiB each, retaining up to 30 files per category. Limits include truncated
output; missing entries after saturation do not prove a path was unused. Turn
logging off after reproducing the problem, as it can affect timing. Disabling it
keeps old files; delete them manually when no longer needed. The existing small
failure reports still work with logging off. Logs are never included in builds.
