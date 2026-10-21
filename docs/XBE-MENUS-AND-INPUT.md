# XBE menus and input

Edit maintained files in `scripts/`; `build/xbe-full-integration-20261014` is generated staging.

| Responsibility | Maintained source |
| --- | --- |
| Menu drawing and navigation | `xbe_trial_graphics_ui.cpp` |
| Scrolling bindings, grouped display, profile options | `xbe_controls_ui.inc` |
| Guest menu identities, original callbacks and draw suppression | `xbe_frontend_guest.inc`, `prepare_xbe_frontend.py` |
| Physical input, focus and release guards | `xbe_trial_menu_input.cpp` |
| Separate reload/interaction consumers | `xbe_conversation_skip.*`, `prepare_xbe_conversation_skip.py` |
| Movie hold progress and original movie hooks | `xbe_intro_skip.*`, `prepare_xbe_intro_skip.py` |
| Portable import, launcher and package allowlist | `launcher/burner-xbe/` |

Keep persistent binding indices stable: `ControlsAction` changes display order only.
Profile settings are exchanged with the guest thread and applied only to the same
screen and unchanged profile snapshot. Save/load/delete retain original callbacks.
Original profile row drawing is suppressed only during its scoped renderer in
mouse mode; the preview and medal drawing stay in the original path.

Movie progress observes the original update delta. A PC hold lasts 0.65 seconds;
release or focus loss cancels it, and a held input cannot skip the next movie.
Only exact supported movie names qualify; backgrounds and the title screen do not.

Tests: `test_xbe_menu_style.cpp`, `test_xbe_frontend.cpp`,
`test_xbe_graphics_input.cpp`, `test_xbe_conversation_skip.cpp`,
`test_xbe_intro_skip.cpp` and `test_xbe_pc_prompts.cpp`. Run the guarded full build;
never bypass the running-game guard. Preparation verifies original executable
bytes before patching. Offline tests do not replace live input/visual acceptance.

Packaging: assemble a fresh directory with `launcher/burner-xbe/package.py`, compile
its two launchers with `build.ps1`, then run the package audit and launcher tests.
Keep the optional HD game texture ZIP separate. Do not copy GameData, caches,
logs, symbols, diagnostics or original game assets into the base package.
