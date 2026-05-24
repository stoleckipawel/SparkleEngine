# Sparkle Launcher Top-Tier Product Hardening Review

## Purpose

This document is a product-design critique and hardening plan for the current Sparkle Launcher GUI. It is written as if the app will be discussed in a high-bar UX/UI design interview at a company like Adobe, Meta, Amazon, Google, Microsoft, Unity, or Epic.

The goal is not to make the launcher decorative. The goal is to make the launcher feel like a mature local workflow product: calm, clear, resilient, keyboard-friendly, and visually intentional.

This document should guide the next design and implementation passes after the existing staged roadmap. It is more critical than the implementation roadmap on purpose. A strong interview critique must expose weak decisions before reviewers do.

## Product Thesis

Sparkle Launcher is a workflow control surface for engine development. It replaces command-line launcher workflows with a compact desktop app that helps a developer prepare, build, cook, run, test, and maintain a local Sparkle workspace.

It should feel closer to GitHub Desktop, JetBrains Toolbox, Visual Studio Installer, Unity Hub's project/workflow surfaces, and Epic's operational tooling than to a game launcher or marketing shell.

The product promise:

> A developer can sync Sparkle, open one native app, understand the next useful action, run it safely, and recover from failures without reading terminal noise first.

## Current-State Diagnosis

The current app is directionally right. The vertical workflow rail, selected workflow detail panel, contextual parameters, and activity drawer are a strong foundation. The app no longer feels like a raw console wrapper, and the structure is much more defensible than the older tabbed design.

The current app is not yet top-tier. It still has several places where a strong reviewer would see implementation artifacts instead of product decisions.

### What Is Working

- The main flow is understandable: Setup, Build, Cook, Run, Maintain.
- The left rail is now a workflow sequence instead of a generic tab set.
- Parameters are contextual and mostly bounded.
- The UI has a calm dark desktop-tool direction.
- The Run action is clear and does not imply that selection runs anything.
- Activity output is available and copyable.
- Concurrent runs are represented in the activity list.
- The app has begun separating primary controls from secondary options.

### What Still Feels Weak

- The workflow rail, detail panel, and activity drawer still feel like three zones placed together more than one fully composed product.
- The selected workflow panel often has a large empty area, making the Run button feel detached from the decision the user just made.
- The activity drawer can dominate the screen after a completed run, even when the user's next task is configuration.
- The progress bar is visually large for the amount of information it carries.
- The UI has almost no icon language except the Run icon, so the workflow categories are less scannable than they could be.
- Focus and selected states are improving, but the system must prove that keyboard navigation is first-class.
- The app still relies on text labels where subtle metadata, icons, or status chips could reduce reading load.
- Failure recovery is still mostly textual. It needs a stronger summary layer and clearer next action hierarchy.

## Interview-Level Questions The App Must Answer

These are questions a senior UX/UI interviewer could ask. The app should have a clear answer for each one.

### Product Strategy

- Who is the primary user: new contributor, engine developer, technical artist, build engineer, or release engineer?
- What is the first useful thing a fresh-sync user should do?
- Which workflows are daily-use and which are rare or dangerous?
- What user error is most expensive?
- Why is this a native launcher instead of a script list or editor panel?
- Why does the launcher stop at workflow orchestration instead of becoming an editor?

### Information Architecture

- Why are the groups Setup, Build, Cook, Run, Maintain?
- Why is Maintain last?
- Why are smoke tests under Run instead of Build or Maintain?
- Why does Build Cook Tools belong in Build instead of Cook?
- What happens if the user does not know which operation they need?
- Does the selected workflow always stay consistent with the selected group?

### Interaction Design

- Why does selecting not run?
- How does the user know a workflow is available?
- How does the user know a workflow needs a project?
- How does the user discover secondary options without feeling buried in settings?
- What happens if two workflows run at once?
- What happens if two workflows fail at once?
- What is the retry path after failure?
- What is the cancellation story, and why is Stop not present yet?

### Visual Design

- What is the visual hierarchy from left to right and top to bottom?
- Why is the UI dark?
- Why is blue used where it is used?
- Why are some controls filled and others outlined?
- Why does the activity drawer deserve its current height?
- How does the app avoid looking like terminal output in a dark box?
- What role do icons play, and why are they not decorative?

### Accessibility

- Can the app be fully operated by keyboard?
- Is focus visible on every interactive element?
- Is selected state visible without color alone?
- Can a screen reader distinguish workflow group, operation, parameter, run action, activity list, and output?
- Does the minimum window size preserve all critical actions?
- Can the user copy output without mouse-only behavior?

### Engineering Product Fit

- Which improvements are UI-only?
- Which improvements require backend state?
- How does the GUI avoid duplicating backend operation logic?
- How will adding a new backend operation affect the UI?
- What is the design system boundary inside Qt Widgets?

## Current Workflow Breakdown

### Fresh Sync

Expected user goal:

- Get from cloned repository to a usable workspace.

Current strengths:

- Setup is first.
- Check Toolchain is visible.
- Generate Solution is grouped correctly.

Current risks:

- The app does not yet present readiness state near the selected workflow.
- If project discovery fails, the user needs a concise state that explains what is missing and which Setup action helps.
- The activity drawer can show old completed output, which may distract from the fresh-sync path.

Hardening target:

- Fresh launch should answer, within 10 seconds: "Start with Setup Workspace or Check Toolchain. Nothing dangerous is happening yet."

### Daily Development

Expected user goal:

- Build, cook, and run the selected project repeatedly.

Current strengths:

- Build, Cook, and Run are separate groups.
- Project/profile controls are contextual.
- Common options are visible without free-form input.

Current risks:

- The Run button is visually far from the parameters, so the decision/action relationship is weak.
- The activity drawer may take attention away from the selected workflow after a run completes.
- Secondary controls such as More options still read as a small settings button rather than a workflow-specific reveal.

Hardening target:

- Common daily workflows should feel like a tight loop: choose operation, confirm project/profile, run, inspect only if needed.

### Troubleshooting

Expected user goal:

- Understand what failed and what to try next.

Current strengths:

- Activity rows include state text.
- Raw output remains available.
- Copy output exists.

Current risks:

- Failure summary is still not visually separated enough from raw output.
- Recovery suggestions need to be specific to missing toolchain, missing project, missing binaries, failed cook tools, and destructive confirmations.
- Multiple failures need a count and clear list state.

Hardening target:

- A failed run should show: what failed, why it likely failed, next best action, and raw output.

### Maintenance

Expected user goal:

- Clean or format without accidentally deleting too much.

Current strengths:

- Maintain is last.
- Clean scope is explicit.
- Confirmation exists as a separate control.

Current risks:

- Confirmation may be hidden behind More options; for destructive scopes this should become visible at the moment it matters.
- The app should disable or explain project-scoped clean when no project exists.

Hardening target:

- Destructive operations should require clear, contextual confirmation without turning the whole app into warning UI.

## Visual Critique

### Composition

The current layout is logical but still boxy. Left rail, detail panel, and activity drawer have strong boundaries. This makes the app readable, but it can feel assembled.

Recommended direction:

- Keep the three functional regions, but make spacing and hierarchy imply one product surface.
- Let the selected workflow panel be the visual anchor.
- Make the activity drawer feel like an operational utility region, not a second primary panel.

### Density

The app is compact enough, but it has uneven density. The selected workflow panel can be mostly empty, while the activity drawer can become dense with output.

Recommended direction:

- Keep parameter rows compact.
- Bring the Run action closer to the parameter block or place it in a detail-panel footer that belongs to the selected workflow.
- Reduce the visual prominence of progress when it only represents job count.

### Color

The dark palette is appropriate for developer tooling. Blue is useful as an accent, but the app must avoid treating every important thing as blue.

Recommended roles:

- Blue: selected operation, primary Run action.
- Neutral gray: panels, secondary buttons, progress tracks.
- Green: completed state only.
- Amber: risky but not destructive.
- Red: failed or destructive confirmation only.
- Near-white: primary text.
- Muted gray-blue: descriptions, field labels, metadata.

### Typography

The typography is improving, but a top-tier review would ask whether every text size has a job.

Recommended roles:

- Workflow detail title: strongest text in the app.
- Rail title: smaller than detail title.
- Operation labels: compact, semibold, no backend phrasing.
- Parameter labels: muted, consistent width.
- Output: monospace, readable line height, not over-bright.

### Motion

Qt Widgets does not need decorative animation. However, state changes should feel intentional.

Recommended direction:

- Avoid animation for now unless it solves a state transition problem.
- If animation is added later, use only subtle drawer expand/collapse or status change transitions.

## Icon Strategy

The repo already fetches Font Awesome Free Solid 6.7.1 for the editor icon font asset. The editor has a small semantic glyph layer in `Engine/Editor/Private/Util/EditorIconGlyphs.h`.

The launcher should not directly become dependent on editor UI internals. It should reuse the same icon source concept, not necessarily the editor header.

### Recommended Approach

Create a small launcher-owned semantic icon layer if icons are implemented:

```text
LauncherIconGlyphs
  Setup
  Build
  Cook
  Run
  Maintain
  Success
  Warning
  Failed
  Copy
  MoreOptions
```

Use the existing Font Awesome font asset as the source if packaging is already available to the launcher. If packaging the font into the launcher is not yet wired, defer icons rather than mixing random image assets.

### Where Icons Should Help

- Workflow group rail: one small icon per group.
- Activity row state: running, done, failed.
- Copy output: copy icon plus text or icon-only with tooltip if compact.
- More options: small chevron or sliders/settings icon.
- Run: keep play icon.

### Where Icons Should Not Be Used

- Do not put icons beside every parameter label.
- Do not add icons to decorative panel headers.
- Do not use icons as the only state signal.
- Do not mix Qt standard icons, Font Awesome glyphs, and arbitrary PNGs in the same control family.

### Suggested Mapping

- Setup: wrench or tools.
- Build: hammer or compile request.
- Cook: boxes/layers/package.
- Run: play.
- Maintain: broom/sliders/settings.
- Done: check.
- Failed: circle-x or warning triangle.
- Running: spinner is not necessary unless there is real animation; a small dot plus `Running` text is enough.

## Content Design Rules

The launcher should use text only when it changes behavior, explains risk, or summarizes state.

### Good Labels

- Setup Workspace
- Check Toolchain
- Compile Editor
- Build Cook Tools
- Cook Shaders
- Build Meshes
- Run Editor Smoke
- More options
- No runs yet
- No projects found
- Failed: CMake configure returned exit code 1

### Labels To Avoid

- Build Meshes / Scene Assets
- Run Editor RHI Smoke Test, if `RHI` is not needed for the user's decision.
- Advanced, as a visible section name.
- No processes running, if the accepted empty state is `No runs yet`.
- Long explanatory helper copy under every operation.

## Top-Tier UX Issues To Fix

### P0: Selection And Detail Must Never Disagree

Problem:

- If the workflow rail shows one group but the detail panel shows an operation from another group, the app looks broken.

Why interviewers care:

- This is a core mental-model failure. It suggests the UI is a wrapper over widgets rather than a coherent product.

Fix:

- Selecting a group should select the first operation in that group or preserve the last selected operation within that group.
- The selected group and selected operation must always agree.

Acceptance:

- Click Setup, Build, Cook, Run, Maintain in sequence. The detail panel always belongs to the visible group.

### P0: Keyboard Navigation Must Be Predictable

Problem:

- A desktop workflow app must not assume mouse-only use.

Why interviewers care:

- Keyboard behavior is a baseline production quality signal.

Fix:

- Explicit tab order: workflow groups, operation rows, parameters, More options, Run, activity list, output, Copy output.
- Neutral visible focus ring distinct from hover and selected state.
- Enter/Space should activate focused buttons.

Acceptance:

- A user can select a workflow, change a parameter, run it, inspect activity, and copy output with keyboard only.

### P0: Failure Summary Must Be Designed, Not Just Logged

Problem:

- Raw output is available, but the app still depends too heavily on it.

Why interviewers care:

- Recovery is where product quality shows. Good tools do not leave users alone with logs.

Fix:

- Failed run header: state, workflow name, exit code, short summary.
- Recovery hint below summary.
- Raw output remains below.
- Copy output remains available.

Acceptance:

- A failed run can be understood at a glance before reading the log.

### P0: No-Project State Must Be Explicit

Problem:

- Project-dependent operations can look available even if no project is discovered.

Why interviewers care:

- Empty state handling reveals whether the app was designed for real environments.

Fix:

- Project combo shows `No projects found`.
- Project-dependent Run is disabled with a clear tooltip/status.
- Setup and toolchain operations remain runnable.
- Clean scopes that do not require a project remain usable.

Acceptance:

- Launching outside a valid repo or with no projects produces a concise, useful state rather than a broken form.

### P1: Activity Drawer Should Not Dominate After Completion

Problem:

- After one completed job, the activity drawer can feel more visually important than the selected workflow.

Why interviewers care:

- A tool must guide the user's next action, not trap attention in past output.

Fix:

- Keep completed output inspectable, but visually lower the drawer after completion.
- Consider a compact completed state with activity list and output visible only when selected or expanded.
- Keep failed output more prominent than successful output.

Acceptance:

- A completed successful run does not overpower the next workflow selection.

### P1: Progress Should Explain Job State, Not Pretend Step Progress

Problem:

- The progress bar is honest, but its visual weight implies more precision than it has.

Why interviewers care:

- Progress UI must match the real data model.

Fix:

- Pair progress with job state counts: `2 running`, `1 failed, 1 running`, `3 done`.
- Consider replacing the large bar with compact status chips if only job count is known.

Acceptance:

- The user never mistakes job-count progress for real per-step progress.

### P1: More Options Should Feel Contextual

Problem:

- `More options` is better than `Advanced`, but it can still feel like a settings drawer.

Why interviewers care:

- Operation parameters should feel like decisions inside a workflow, not application settings.

Fix:

- Keep More options quiet.
- Reveal destructive confirmations only after a risky option or scope is selected.
- Consider inline secondary controls for smoke tests if they are regularly used.

Acceptance:

- Common workflows do not require opening More options.

### P1: Icons Should Improve Scanning

Problem:

- The workflow rail is text-only except for the Run button.

Why interviewers care:

- Icons, when systematic, help structure and recall. Missing icons are not fatal, but inconsistent icons are.

Fix:

- Add one semantic icon per workflow group using the shared Font Awesome source if launcher packaging supports it.
- Add state icons to activity rows only if they reinforce the text state.

Acceptance:

- Icons can be removed and the UI still works; icons can stay and the UI scans faster.

### P2: Status Metadata Near Workflow Title

Problem:

- The selected workflow title does not communicate readiness, last run, missing project, or running state.

Why interviewers care:

- Metadata reduces uncertainty without adding paragraphs.

Fix:

- Add restrained chips near the title:
  - Ready
  - Requires project
  - Running
  - Last failed
  - Confirmation required

Acceptance:

- The user can understand workflow state without opening output.

### P2: Output Should Become A Log Viewer, Not A Text Dump

Problem:

- Output is readable, but still looks like terminal text pasted into a box.

Why interviewers care:

- Developer tools are judged by how they handle noise.

Fix:

- Add a summary region above raw output for failed runs.
- Keep raw output monospace and selectable.
- Consider log level highlighting only if backend emits structured severity later.

Acceptance:

- Raw output remains available, but summary drives recovery.

## Proposed Future Layout

```text
+----------------------------------------------------------------+
| Rail          | Workflow detail                                 |
|               | Title                  [Ready] [Last: failed]     |
| Setup         | Short description                                |
| Build         | Project     [Showcase v]                         |
| Cook          | Profile     [DevelopmentGame v]                  |
| Run           | More options                                     |
| Maintain      |                                      [Run]        |
+----------------------------------------------------------------+
| Activity: 1 running, 1 failed                         [Copy]    |
| [Running] Cook Shaders      [Failed] Compile Runtime             |
| Output summary / raw output                                      |
+----------------------------------------------------------------+
```

Rules:

- The workflow detail is the primary surface.
- Activity is a drawer, not a second page.
- Run belongs visually to the selected workflow.
- Failure summary appears before raw output.
- Icons reinforce group and state only.

## Roadmap From Current App To Interview-Defensible App

Use the following passes as implementation prompts. Each pass is written so it can be copied directly into a future coding task. Keep the passes sequential unless a later pass is explicitly requested; the order matters because visual polish depends on correct state and interaction foundations.

General implementation constraints for every pass:

- Preserve backend operation parity with the existing console/launcher core.
- Keep the launcher native Qt Widgets and Windows-focused.
- Start by reading the listed primary anchors and the current implementation state; do not assume a pass has no partial work already present.
- Do not add nonfunctional buttons, fake progress, unsupported cancellation, marketing visuals, or decorative UI.
- Prefer small, verifiable changes in `LauncherMainWindow` and adjacent launcher GUI helpers before introducing new files.
- Keep labels product-facing and compact. Avoid slash-combined labels and visible `Advanced` section framing.
- Validate with source-only checks and a focused launcher build/package smoke when the pass changes code.

Expected handoff after any pass:

- Files changed.
- UX behavior changed.
- Validation run and result.
- Screenshots captured or reason screenshots were not practical.
- Any deferred work that should remain in a later pass.

### Pass 1: State Coherence

Implementation prompt:

```text
Harden Sparkle Launcher Pass 1: State Coherence.

Goal:
Make the current launcher feel internally consistent and keyboard-operable before adding visual polish. The workflow rail, selected operation detail, parameter controls, Run availability, activity output, and failure recovery state must agree with each other at all times.

Primary anchors:
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherProjectModel.h/.cpp only if no-project state cannot be solved from the window layer

Scope:
- Ensure selecting a workflow group always selects an operation from that group, preferably preserving the last selected operation inside that group when reasonable.
- Ensure the visible detail panel never belongs to a different group than the selected rail item.
- Add or finish explicit tab-order registration for workflow groups, operation rows, contextual parameters, More options, Run, activity list, output, and Copy output.
- Add neutral, visible focus styling that is distinct from hover and selected styling.
- Disable Run for project-dependent operations when no project exists, while keeping Setup/toolchain operations usable.
- Show a concise no-project state such as `No projects found` instead of leaving project controls ambiguous.
- Add or finish concise failure recovery hints above raw output for common failure categories: project missing, setup/toolchain missing, build failure, cook failure, launch failure, and destructive confirmation missing.

Do not:
- Add icons in this pass unless they are already present in the current implementation.
- Add fake Stop/Cancel behavior.
- Add new backend operation logic.
- Rework the whole layout beyond what is needed for state coherence and keyboard focus.

Acceptance checks:
- Clicking Setup, Build, Cook, Run, and Maintain always updates the detail panel to an operation in that group.
- Keyboard-only flow can select a group, select an operation, change parameters, open More options, run an operation, inspect activity, and copy output.
- No-project state disables only project-dependent operations and explains why.
- A failed run shows a readable summary and next recovery hint before raw output.
- The UI still builds and launches from the packaged launcher path.

Validation:
- Run `C:/Progra~1/CMake/bin/cmake.exe --build build --target SparkleLauncher --config DevelopmentEditor`.
- Refresh the packaged exe if needed and launch `build/Package/SparkleLauncher/DevelopmentEditor/SparkleLauncher.exe`.
- Capture screenshots for each workflow group, one no-project/blocked state if practical, and one failed or simulated failure state if practical.
- Run `git diff --check` on touched files.
```

### Pass 2: Activity Drawer Refinement

Implementation prompt:

```text
Harden Sparkle Launcher Pass 2: Activity Drawer Refinement.

Goal:
Make activity and output useful without letting completed logs dominate the selected workflow. Successful work should recede; failed work should remain easy to understand and recover from.

Primary anchors:
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherBackend.cpp only if the GUI needs an existing completion summary that is already exposed there

Scope:
- Rebalance the activity drawer so the selected workflow detail remains the primary surface after successful completion.
- Keep failed output more prominent than successful output.
- Replace, reduce, or restyle the progress bar if it only communicates job count rather than real step progress.
- Make aggregate activity text clear for queued/running/done/failed states, including multiple failures.
- Make selecting a run in the activity list update output and summary predictably.
- Keep Copy output available for the currently selected run.

Do not:
- Add fake per-step progress unless the backend exposes real step data.
- Hide raw output completely.
- Add log filtering unless structured severity exists or can be derived safely.
- Change backend operation execution semantics.

Acceptance checks:
- A completed successful run does not visually overpower the selected workflow panel.
- A failed run remains visually discoverable and shows summary plus raw output.
- Two concurrent or recent runs are distinguishable in the activity list.
- The progress/status presentation never implies more precision than the app knows.
- Copy output copies the selected run output, not stale output from another run.

Validation:
- Run `C:/Progra~1/CMake/bin/cmake.exe --build build --target SparkleLauncher --config DevelopmentEditor`.
- Launch the packaged app and capture screenshots for one successful run, one failed or blocked run, and two listed runs if practical.
- Run `git diff --check` on touched files.
```

### Pass 3: Icon System

Implementation prompt:

```text
Harden Sparkle Launcher Pass 3: Icon System.

Goal:
Add a restrained, semantic icon language that improves scanning without turning the launcher into decorative UI. Icons should clarify workflow groups and activity states while text remains the source of truth.

Primary anchors:
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp
- CMake/Dependencies/FetchDependencies.cmake for the existing Font Awesome Free Solid asset path context
- Engine/Editor/Private/Util/EditorIconGlyphs.h as a reference for semantic glyph mapping, not as a dependency to blindly reuse

Scope:
- Add a launcher-owned semantic icon mapping if the launcher can cleanly load the existing Font Awesome Free Solid font asset.
- If loading the font asset requires too much packaging work, document the blocker and keep this pass source-ready without mixing icon sources.
- Add one small icon per workflow group: Setup, Build, Cook, Run, Maintain.
- Add activity state icons only when paired with state text: queued, running, done, failed.
- Keep Run using a play symbol, but make icon style consistent with the new system if possible.
- Ensure icon failure does not break labels, layout, or workflow usability.

Do not:
- Depend on editor UI internals from the launcher unless there is already an approved shared utility boundary.
- Mix Qt standard icons, Font Awesome glyphs, arbitrary PNGs, and custom drawings in the same control family.
- Use icons beside every parameter label.
- Use icon-only buttons without tooltip and accessible name.
- Add decorative panel-header icons.

Acceptance checks:
- Workflow groups scan faster with icons but still work if icons are unavailable.
- Activity state remains readable without color or icon alone.
- Icon sizes, alignment, and spacing hold at normal and minimum window sizes.
- The launcher has one consistent icon source or explicitly defers the pass.

Validation:
- Run `C:/Progra~1/CMake/bin/cmake.exe --build build --target SparkleLauncher --config DevelopmentEditor`.
- Launch the packaged app and capture normal-size and minimum-size screenshots.
- Temporarily test or reason through missing-font behavior so text labels still display.
- Run `git diff --check` on touched files.
```

### Pass 4: Readiness Metadata

Implementation prompt:

```text
Harden Sparkle Launcher Pass 4: Readiness Metadata.

Goal:
Add compact, high-signal readiness metadata near the selected workflow title so users know whether the current operation is ready, blocked, running, failed recently, or needs confirmation before reading output.

Primary anchors:
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherProjectModel.h/.cpp only if project state needs a cleaner read API

Scope:
- Add restrained status chips near the selected workflow title.
- Supported chip meanings should include: Ready, Requires project, Running, Last failed, and Confirmation required.
- Keep chip text short and product-facing.
- Ensure chips update when operation, project, clean scope, running state, and completion state change.
- Use color roles consistently: neutral for metadata, blue for selected/primary, green for done, amber for caution, red for failure/destructive risk.
- Keep chips secondary to the workflow title and Run action.

Do not:
- Add long helper text under the title.
- Turn every backend detail into a chip.
- Add readiness states the app cannot actually know.
- Overuse red or amber for non-risk states.

Acceptance checks:
- Project-ready operations show Ready or no unnecessary warning.
- Project-dependent operations with no project show Requires project and disabled Run.
- Running operations show Running without hiding parameters.
- A previously failed selected operation shows Last failed until superseded by a newer successful or running state.
- Destructive clean scopes show Confirmation required until confirmed.

Validation:
- Run `C:/Progra~1/CMake/bin/cmake.exe --build build --target SparkleLauncher --config DevelopmentEditor`.
- Launch the packaged app and capture screenshots for project present, no project if practical, failed previous run, running state, and destructive clean scope.
- Run `git diff --check` on touched files.
```

### Pass 5: Production Polish

Implementation prompt:

```text
Harden Sparkle Launcher Pass 5: Production Polish.

Goal:
Finish a full quality pass so the launcher behaves like a polished native desktop tool across minimum size, keyboard use, disabled states, labels, tooltips, accessibility names, and packaging smoke launch.

Primary anchors:
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherSettings.* only if persisted UI behavior needs adjustment
- Tools/Launcher/SparkleLauncher/Private/Gui/LauncherProjectModel.* only if empty/project state polish requires it

Scope:
- Audit minimum window size for all workflow groups and parameter states.
- Fix clipped labels, unstable widths, awkward empty regions, and overlapping controls.
- Add concise tooltips where labels are intentionally compact or controls can be disabled.
- Add accessible names/descriptions for icon buttons, More options, activity rows, output, and Copy output.
- Ensure disabled states are visually consistent and explainable.
- Confirm Copy output has a keyboard path and copies the selected run.
- Keep the visual system calm: no new decorative graphics, no marketing layout, no fake status elements.

Do not:
- Redesign the entire app layout unless a concrete minimum-size or accessibility problem requires it.
- Add unsupported features such as cancellation, pause, cloud sync, notifications, account state, or update management.
- Change operation names unless a label is clearly user-hostile or inconsistent with the existing product language.

Acceptance checks:
- Every workflow group is usable at the minimum supported window size.
- No critical action or parameter clips, overlaps, or disappears.
- Keyboard users can reach and operate all controls.
- Screen-reader names exist for non-text or compact controls.
- Disabled Run states communicate why they are disabled.
- The packaged launcher starts successfully after refresh.

Validation:
- Run `C:/Progra~1/CMake/bin/cmake.exe --build build --target SparkleLauncher --config DevelopmentEditor`.
- Refresh the packaged launcher exe and launch `build/Package/SparkleLauncher/DevelopmentEditor/SparkleLauncher.exe`.
- Capture minimum-size screenshots for Setup, Build, Cook, Run, and Maintain.
- Run a keyboard-only checklist manually.
- Run `git diff --check` on touched files.
```

## Design Scorecard

Use this checklist before calling the launcher interview-ready.

### Product Clarity

- The first useful action is obvious.
- The workflow order maps to real developer behavior.
- Maintenance is not visually promoted above daily workflows.
- The app is clearly a workflow tool, not a storefront or editor clone.

### Interaction Quality

- Selection never runs.
- Group and detail selection never disagree.
- Run availability matches selected workflow requirements.
- More options are discoverable but not dominant.
- Destructive actions require contextual confirmation.

### Visual Quality

- One clear primary focus exists at all times.
- The palette has defined roles.
- Blue is not overused.
- Text hierarchy is consistent.
- Empty space feels intentional, not accidental.

### Accessibility

- Keyboard navigation reaches all interactive controls.
- Focus is visible and not confused with hover.
- Selected state includes structure/text, not color alone.
- Output is readable and copyable.
- Minimum window size has no overlap or clipped critical actions.

### Failure And Recovery

- Failed jobs are identifiable in the activity list.
- Failure summary appears before raw output.
- Recovery hint is specific enough to act on.
- Raw output remains available.
- Multiple failures remain understandable.

## Recommended Next Implementation Prompt

```text
Use `Pass 1: State Coherence` from docs/plans/launcher-gui-top-tier-product-hardening.md as the implementation prompt. Treat later passes as out of scope unless Pass 1 exposes a small dependency needed to finish its acceptance checks. Keep backend operation parity unchanged, preserve the native Qt Widgets launcher direction, validate with the focused SparkleLauncher build, and report files changed, UX behavior changed, validation results, screenshots captured, and deferred work.
```

## Final Bar

The launcher is ready for a top-tier design review when a reviewer can ask "why?" about any visible element and the answer is about user workflow, state, risk, or recovery rather than "because Qt gave us this widget." 
