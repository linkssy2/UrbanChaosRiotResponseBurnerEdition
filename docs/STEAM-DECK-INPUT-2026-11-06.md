# Steam Deck report and menu input handoff

## Reported performance

The supplied v0.14 log identifies the published runtime
`a2d7380ffde0fc0baa80a99e87821b59c675f5239595b3a21b6cd48c26202782`,
Wine 11.0 and AMD VANGOGH. Its desktop/window is 1920 x 1080, automatic
aspect, 100% internal scale, 60 FPS cap and VSync enabled. HD HUD, HD game
textures and MSAA are off; scenery/character lighting are on. The renderer
reports a 640 x 480 guest surface and a 2.25 render scale at startup. Those
startup dimensions do not independently establish every later render target.

The user confirmed this was the Deck's built-in screen, whose native resolution
is [1280 x 800](https://www.steamdeck.com/en/tech/deck). The borderless window
also measures 1920 x 1080, so the report cannot establish that replacing the
saved-mode query with a current-mode query would recover the physical panel size.
The runtime's Desktop choice follows the display exposed by Windows/Proton;
there is no proof of a specific Steam shortcut or virtual-desktop configuration.
Do not force a handheld resolution for every Deck: docked users and explicit
resolution choices need to remain supported. For this reporter, select 1280 x 800
and 16:10 in Graphics, retain 100% internal scale, and restart the game before
comparing performance. No default or saved user setting was changed here.

The 57 sparse presentation samples during mission 12-01 range from 33.4 to
58.0 FPS, with a median of 49.3. Frontend samples before and after the mission
return to approximately 60. The 4.9 FPS sample overlaps loading and is not a
steady gameplay result. These samples are not a whole-run average, frame-time
percentiles, CPU timings or GPU timings.

The maintained Proton wrapper does not force a DXVK/VKD3D configuration or
sync override. No Linux-only GPU wait was identified. Existing Windows
performance captures cannot establish the limiting subsystem on this Deck.
Two controller enumeration names do not prove duplicate physical controllers,
co-op use or a Steam Input fault.

A useful next comparison is the same mission, viewpoint and effects at
1920 x 1080 versus 1280 x 720 with the same 16:9 aspect, restarting the game
between settings so retained render targets cannot invalidate the comparison.
Check 1280 x 800 and 16:10 separately: changing aspect also changes visible
scene coverage. Retain lighting, textures and other settings. Record actual target dimensions and CPU/GPU frame timings
before selecting engine changes. This is a diagnostic comparison, not a
promised performance fix or a change to default visual quality.

## Menu correction

The active prompt preparer previously marked any held controller input as current
input on every poll. Menu rows also highlighted mouse hover independently of
navigation focus. These two paths explain the observed competing selections.

Menu ownership now changes on fresh controller button/direction activity or
actual keyboard/mouse input. Hysteresis prevents a stationary stick near the
menu threshold from repeatedly claiming control. Keyboard autorepeat and an
unchanged mouse position do not count as a new device. The losing device's
queued navigation, stale hover and native pointer path are suppressed. The
character-selection lobby still accepts both players simultaneously, and the
player who paused keeps control of that pause screen.

## Verification boundaries

Private log analysis, source backups and candidate receipts belong in
`build/deck-input-20261106`. No Steam Deck is available locally. Windows
fixtures/build success does not establish Proton input or gameplay performance.
Do not label the menu ownership correction as a Steam Deck FPS improvement.
Published v0.14 remains unchanged.
