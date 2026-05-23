# Sparkle Launcher Simplified Redesign

## Problem Statement
The current launcher proves the backend integration works, but the UI feels too busy. Projects, operations, settings, and status are split across multiple pages, so the user has to understand the launcher structure before doing simple work.

This redesign makes the launcher feel like a quiet production tool: one main surface, one selected project, a small number of obvious actions, and advanced controls hidden until they matter.

## Design Goals
- One primary screen instead of separate Projects, Operations, Settings, and About pages.
- Make the active project and most common workflows visible immediately.
- Keep full backend feature parity without showing every backend operation as first-class UI chrome.
- Move rare flags, filters, and destructive options into an Advanced drawer.
- Keep preview and run behavior, but make Preview feel like a normal safety step instead of a separate mental mode.
- Preserve the dark editor-inspired visual language, but reduce borders, section noise, and page switching.

## Non-Goals
- Do not add new backend workflows.
- Do not bring back the console app.
- Do not remove advanced options; collapse them behind progressive disclosure.
- Do not make this a wizard. Users should still be able to jump directly to a workflow.

## Main Window: Default State

```text
+--------------------------------------------------------------------------------+
| Sparkle Launcher                                      [Refresh] [Advanced v] [x] |
| Project: [ Showcase                                      v ]  Profile: [Dev v]  |
+------------------------------+-------------------------------------------------+
| Projects                     | What do you want to do?                         |
|                              |                                                 |
| > Showcase                   | +----------------+ +----------------+           |
|   MyGame                     | | Build          | | Cook           |           |
|   Sandbox                    | | Editor/Runtime | | Assets/Shaders |           |
|                              | +----------------+ +----------------+           |
|                              |                                                 |
|                              | +----------------+ +----------------+           |
|                              | | Launch         | | Validate       |           |
|                              | | Editor/Runtime | | Format/Checks  |           |
|                              | +----------------+ +----------------+           |
|                              |                                                 |
|                              | +----------------+                              |
|                              | | Workspace      |                              |
|                              | | Setup/Clean    |                              |
|                              | +----------------+                              |
+------------------------------+-------------------------------------------------+
| Preview: Build Editor                                                        |
| Ready. Will build Showcase editor target with DevelopmentEditor profile.       |
|                                                                              |
|                                             [Preview Again] [Run Build Editor] |
+--------------------------------------------------------------------------------+
| Ready                                                                          |
+--------------------------------------------------------------------------------+
```

### Default State Rules
- The project list is always visible, but narrow and quiet.
- The center is a workflow chooser, not a raw operation list.
- Selecting a workflow updates the preview panel immediately.
- The preview panel stays attached to the bottom so users always know what will happen before running.
- The About page is removed from primary navigation; version/backend notes can live in a small About dialog from the window menu later.

## Workflow Card Expansion
Cards expand in-place instead of navigating to a different page.

```text
+--------------------------------------------------------------------------------+
| Project: [ Showcase                                      v ]  Profile: [Dev v]  |
+------------------------------+-------------------------------------------------+
| Projects                     | Build                                           |
|                              |                                                 |
| > Showcase                   |   ( ) Build Editor                              |
|   MyGame                     |   ( ) Build Runtime                             |
|   Sandbox                    |   ( ) Generate Solution                         |
|                              |   ( ) Check Toolchain                           |
|                              |                                                 |
|                              |   Targets: [ optional target filter             ]|
|                              |   [ ] Force configure before build              |
|                              |                                                 |
|                              |                         [Back] [Preview] [Run]  |
+------------------------------+-------------------------------------------------+
| Preview: Build Editor                                                        |
| Ready. Output will stream here when the operation starts.                      |
+--------------------------------------------------------------------------------+
```

### Expansion Rules
- Only one workflow card can be expanded at a time.
- Expanded card content shows only options relevant to that workflow.
- Global settings disappear as a page and become contextual controls.
- Rare fields remain available, but only next to the operation that uses them.

## Advanced Drawer
Advanced is a right-side drawer, not a full page. It is closed by default.

```text
+--------------------------------------------------------------------------------+
| Sparkle Launcher                                      [Refresh] [Advanced ^] [x] |
+------------------------------+----------------------------------+-------------+
| Projects                     | What do you want to do?          | Advanced    |
|                              |                                  |             |
| > Showcase                   | +----------------+ +------------+ | Profiles    |
|   MyGame                     | | Build          | | Cook       | | Editor [v]  |
|   Sandbox                    | +----------------+ +------------+ | Runtime[v]  |
|                              |                                  |             |
|                              | +----------------+ +------------+ | Validation  |
|                              | | Launch         | | Validate   | | Groups  []  |
|                              | +----------------+ +------------+ | Targets []  |
|                              |                                  |             |
|                              | +----------------+                | Smoke       |
|                              | | Workspace      |                | Backend []  |
|                              | +----------------+                | Frames  []  |
+------------------------------+----------------------------------+-------------+
| Preview: No operation selected                                                 |
+--------------------------------------------------------------------------------+
```

### Advanced Drawer Rules
- Advanced options never block the main path visually.
- The drawer stores cross-workflow preferences and filters.
- Destructive confirmations remain explicit in the workflow that triggers them.
- The drawer can be implemented as a `QFrame` shown/hidden inside the same main layout.

## Running State

```text
+--------------------------------------------------------------------------------+
| Sparkle Launcher                                      [Refresh disabled] [Stop?] |
| Project: Showcase                                      Build Editor running...  |
+------------------------------+-------------------------------------------------+
| Projects                     | Build                                           |
|                              |                                                 |
| > Showcase                   |   Build Editor                                  |
|   MyGame                     |   DevelopmentEditor                             |
|   Sandbox                    |                                                 |
|                              |   [ running... controls disabled ]              |
+------------------------------+-------------------------------------------------+
| Output                                                                       |
| > cmake --build ...                                                          |
| > compiling ...                                                              |
| > linking SparkleLauncher.exe                                                |
|                                                                              |
+--------------------------------------------------------------------------------+
| Running Build Editor                                                          |
+--------------------------------------------------------------------------------+
```

### Running Rules
- Project switching and operation controls are disabled while a workflow runs.
- Output replaces preview, but stays in the same bottom panel.
- The existing concurrent-run guard remains.
- A Stop button is optional only if backend cancellation is implemented; otherwise do not show it.

## Workflow Mapping

| Workflow Card | Operations |
| --- | --- |
| Build | `project.build.editor`, `project.build.runtime`, `workspace.generate-solution`, `toolchain.check` |
| Cook | `cook.tools.prepare`, `cook.project`, `cook.shaders`, `cook.textures`, `cook.assets` |
| Launch | `project.launch.editor`, `project.launch.runtime` |
| Validate | `quality.validate`, `quality.format`, `smoke.rhi.editor`, `smoke.rhi.runtime` |
| Workspace | `workspace.setup`, `workspace.clean` |

## Option Placement

| Option | New Placement |
| --- | --- |
| Project | Always-visible project selector/list |
| Editor profile | Header quick selector plus Advanced drawer |
| Runtime profile | Header quick selector plus Advanced drawer |
| Build targets | Build expansion |
| Force configure | Build expansion |
| Force recook | Cook expansion |
| Confirm force recook | Cook expansion when force recook is enabled |
| Shader packages | Cook expansion |
| Format mode | Validate expansion |
| Validation groups | Advanced drawer and Validate expansion |
| Validation targets | Advanced drawer and Validate expansion |
| Clean scope | Workspace expansion |
| Confirm clean | Workspace expansion when clean is selected |
| Smoke backend | Advanced drawer and Validate expansion |
| Smoke frame limit | Advanced drawer and Validate expansion |
| Smoke trace | Validate expansion |
| Skip smoke level switching | Validate expansion |

## Compact Window Variant

```text
+--------------------------------------------------------------+
| Sparkle Launcher                         [Advanced v]        |
| Project [Showcase v] Profile [Dev v]                         |
+--------------------------------------------------------------+
| Build     Cook     Launch     Validate     Workspace         |
+--------------------------------------------------------------+
| Build Editor                                                  |
| Targets [ optional ]                                          |
| [ ] Force configure                                           |
|                                      [Preview] [Run]          |
+--------------------------------------------------------------+
| Ready. Will build Showcase editor target.                     |
+--------------------------------------------------------------+
```

## Implementation Notes
- Replace the sidebar/page stack with one root layout: project rail, workflow area, optional advanced drawer, bottom preview/output panel.
- Keep `LauncherBackend`, `LauncherProjectModel`, and `LauncherSettings`; the redesign is UI structure, not backend architecture.
- Replace the flat operation list with grouped workflow definitions in the UI layer.
- Keep operation ids unchanged so backend parity and Phase 2 documentation remain valid.
- Build the simplified UI first with the existing dark style, then tune spacing and colors after the structure feels calm.

## Implementation Status
- Implemented in the Qt `SparkleLauncher` main window.
- The old primary sidebar/page-stack navigation has been replaced by a one-screen project rail, workflow surface, advanced drawer, and fixed preview/output panel.
- `LauncherBackend`, `LauncherProjectModel`, and `LauncherSettings` remain the reused backend/model/settings path.
- Validated with a `DevelopmentEditor` launcher build and packaged launch smoke.

## Ultra-Simple Revision
Follow-up user review found the workflow-card surface still too busy. The launcher now favors the old batch-file mental model:

- The default screen is a small set of direct processes: Generate Solution, Build Editor, Cook Project, Launch Editor, and Validate.
- Selecting a process does not run it. The user reviews process-specific options first, then clicks Run.
- Secondary operations remain available from a compact Other selector instead of being shown as radio-button groups.
- Profile and rare operation flags are placed with the process that uses them instead of in a detached drawer.
- The bottom panel shows process activity, a completion progress bar, and output for the selected run.
- Multiple processes can be started while earlier runs are still active; each run has its own activity entry and output buffer.

## Acceptance Criteria For Redesign Implementation
- No primary page navigation is required for normal use.
- A new user can pick a project and run Build, Cook, Launch, Validate, or Workspace actions from the first screen.
- Advanced settings are available but closed by default.
- Preview/output is always visible in the same place.
- Every Phase 2 operation remains reachable.
- Every Phase 2 option remains reachable either contextually or through Advanced.