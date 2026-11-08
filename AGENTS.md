# Project instructions

- Maintain Windows and Linux/Proton builds in tandem, as requested by the user.
  Future build and release preparation covers both platforms unless the user
  explicitly limits the task to one platform.
- Use the same release version, maintained source revision, game runtime and
  shared asset manifests for both packages. Include applicable fixes and features
  in both; keep platform-specific launch and setup changes in their own paths.
- Verify each platform separately and record the evidence and any untested areas.
  Windows success alone does not establish Proton compatibility, or vice versa.
- Follow the paired-release workflow in `docs/XBE-RUNTIME-MAINTENANCE.md`.
  Linux currently uses the Windows runtime through Proton; do not label it native.
- Use Markdown (`.md`) for new README files and future release instructions,
  as requested by the user. Preserve third-party documentation filenames and
  published archives. When migrating packaged README filenames, verify updates
  from the currently published launcher on both platforms before shipping.
