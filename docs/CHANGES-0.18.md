# Burner Edition 0.18

- Keep scope artwork visible when No HUD is enabled.
- Restore blood pools for dead enemies, including clipping at ledges. Knocked-out enemies do not create pools.
- Advance the TV electrocution sound by 300 ms in the affected scene. Other dialogue and sound timing are unchanged.
- Reuse blood-pool clipping buffers to reduce temporary allocation work without changing clipped geometry.
- Update the launcher cover with the approved orange brushstroke and gold/red lettering.
- Keep Manage > DLC / Prototypes visible as an unavailable preview. Prototype integration is paused; no import or play action is enabled.

Missions start normally. The private hotel spawn, prototype scenes, test saves and original game files are excluded.

Windows and Linux/Proton use the same runtime and shared resources. The existing 0.09 HD texture download is unchanged and does not need reinstalling.

The user accepted the three fixes on Windows, plus the cover and normal mission spawn. Automated build, installer and update checks are recorded separately. Live Proton/Steam Deck and a full campaign pass remain unverified. Co-op remains experimental. HD HUD remains a work in progress.

Source downloads are inspection/contribution snapshots; clean bootstrapping still requires the private title inputs described in docs/BUILDING.md. No publication has been performed.
