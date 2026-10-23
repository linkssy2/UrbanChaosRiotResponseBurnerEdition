# XBE menus and input

Edit maintained files in `scripts/`; `build/xbe-full-integration-20261014` is generated staging.

| Responsibility | Maintained source |
| --- | --- |
| Menu drawing and navigation | `xbe_trial_graphics_ui.cpp` |
| Scrolling bindings, grouped display, profile options | `xbe_controls_ui.inc` |
| Guest menu identities, original callbacks and draw suppression | `xbe_frontend_guest.inc`, `prepare_xbe_frontend.py` |
| Physical input, focus and release guards | `xbe_trial_menu_input.cpp` |
| Separate reload/interaction consumers | `xbe_conversation_skip.*`, `prepare_xbe_conversation_skip.py` |
| Directional firearm cycling | `xbe_firearm_cycle.h`, `prepare_xbe_firearm_cycle.py` |
| Movie hold progress and original movie hooks | `xbe_intro_skip.*`, `prepare_xbe_intro_skip.py` |
| Portable import, launcher and package allowlist | `launcher/burner-xbe/` |

Keep persistent binding indices stable: `ControlsAction` changes display order only.
Profile settings are exchanged with the guest thread and applied only to the same
screen and unchanged profile snapshot. Save/load/delete retain original callbacks.
Original profile row drawing is suppressed only during its scoped renderer in
mouse mode; the preview and medal drawing stay in the original path.

Keyboard/mouse action 2 remains the saved Firearms binding and is displayed as
Next Firearm. Previous Firearm has two separate `Firearms/Previous0` and
`Previous1` bindings. New profiles use Wheel Down; upgrades leave it unbound if
that wheel direction already belongs to an existing action. The new direction
uses the retail firearm selection loop, scoped to the player's explicit input.
Controller selection, automatic switching, ammo checks and weapon ownership
remain in the original path. The HUD's firearm shortcut continues to show Next.

Pause confirmations share the native two-row mouse menu for checkpoint restart
(2672A0), mission restart (267330), and Quit (267400). The shared callback/title
allowlist is in `xbe_frontend.h`; guest qualification additionally checks the
manager, message, choices and Back callback. Original modal callbacks and sounds
remain active. Controller mode returns the original modal. Regression fixtures
execute all three confirmation paths and test stale actions and device switching.

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
Keep the optional HD game texture ZIP separate. Do not copy GameData, learned
caches, logs, symbols, diagnostics or original game assets into the base package.
The verified portable shader seed is the only cache exception.
