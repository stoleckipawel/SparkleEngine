# Sparkle Launcher GUI Phase 1 Architecture

## Decisions
- **Qt Version:** Qt 6.8 LTS or newer, using the Widgets module.
- **Target:** Windows-only `SparkleLauncher` executable with a native GUI subsystem.
- **Project Structure:** Keep the GUI source inside `Tools/Launcher/SparkleLauncher/Private/Gui` so the launcher owns its frontend without coupling to the engine editor UI.
- **Backend Boundary:** GUI talks to `SparkleLauncherCore` through typed C++ adapter classes. Phase 1 may read operation descriptors and project discovery data, but it must not execute workflows or shell out to the console app.

## Module Responsibilities
- **`LauncherGuiApp`:** Qt application bootstrap, repository root lookup, and top-level object composition.
- **`LauncherMainWindow`:** Qt Widgets shell, sidebar navigation, placeholder pages, and visual styling inspired by Unity Hub and Epic Launcher.
- **`LauncherProjectModel`:** Project discovery state, selected project state, and change notifications for the UI.
- **`LauncherSettings`:** Launcher profile preferences that are currently in-memory and ready for persistence in a later phase.
- **`LauncherBackend`:** Qt-facing backend adapter that exposes operation descriptors and preview signals without executing commands.

## Positive Guiderails
- Keep frontend classes small and directly tied to one responsibility.
- Use Qt ownership, widgets, and signal/slot flow for UI state changes.
- Keep Sparkle editor UI as visual inspiration only; do not include or link editor UI code.
- Keep backend access typed through `SparkleLauncherCore` APIs.

## Negative Guiderails
- Do not use `QProcess`, `system`, batch files, or the console executable as a backend bridge.
- Do not execute build, cook, launch, format, clean, or validation workflows in Phase 1.
- Do not add new user-facing launcher features beyond the shell and placeholders needed to prove navigation and module boundaries.
- Do not run a full build between implementation stages; reserve the full build for the final checkpoint before legacy removal.

## Source Layout
```text
Tools/Launcher/SparkleLauncher/
  Private/Gui/
    LauncherGuiApp.*
    LauncherMainWindow.*
    LauncherProjectModel.*
    LauncherSettings.*
    LauncherBackend.*
```

## Acceptance Criteria
- Qt 6.8+ Widgets is documented and wired in CMake for `SparkleLauncher`.
- `LauncherMainWindow` provides navigable placeholder pages for Projects, Operations, Settings, and About.
- `LauncherProjectModel`, `LauncherSettings`, and `LauncherBackend` exist as explicit modules with clear ownership.
- `LauncherBackend` exposes existing launcher operation descriptors but only returns Phase 1 preview text.
- No console app execution path is introduced.
- No full build is run for this phase.