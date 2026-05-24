# Sparkle Launcher Design Strengthening Review

## Purpose

This document critiques the current Sparkle Launcher GUI from a top-tier product design perspective and defines the next design bar. The goal is not to add decoration. The goal is to make the launcher feel like a calm, modern, professional desktop tool that can be defended in a serious UX/UI design interview.

The launcher should help a developer who just synced the repository understand what to do next, run workflows without using a console, and monitor long-running work without feeling buried in terminal noise.

Related deeper critique: `docs/plans/launcher-gui-top-tier-product-hardening.md`.

## Product Thesis

Sparkle Launcher is a local workflow control surface for engine development. It is not a marketing launcher, not an editor UI, and not a console wrapper. Its job is to turn complex build, cook, launch, and maintenance operations into a small set of trustworthy actions.

The product should feel closer to GitHub Desktop, Visual Studio Installer, JetBrains Toolbox, and the operational parts of Unity Hub than to a game launcher storefront.

## Design Bar

The launcher should satisfy these principles:

- **Calm by default:** The first screen should not shout, imply urgency, or show too much implementation detail.
- **Workflow-first:** The order should match what a user actually does after syncing the repo.
- **Contextual controls:** Parameters appear only when they affect the selected workflow.
- **Low text burden:** Copy should explain uncertainty, risk, or state. It should not narrate obvious UI behavior.
- **Strong hierarchy:** Navigation, selected workflow, parameters, run action, activity, and output should each have a clear visual rank.
- **Operational trust:** Long-running commands should feel trackable, cancelable when supported, and recoverable after errors.
- **Desktop-native restraint:** Use compact spacing, clear borders, crisp controls, and predictable Windows desktop behavior.

## Primary User Scenarios

### Fresh Sync

A developer clones or syncs the repository and wants to get to a working state.

Expected path:

1. Setup workspace.
2. Check toolchain if setup fails or looks stale.
3. Generate solution if needed.
4. Build editor/runtime.
5. Build cook tools if cooking is needed.
6. Cook project assets.
7. Run editor/runtime.
8. Use smoke tests or maintenance only after the main path is understood.

### Daily Development

A developer already has a configured workspace and wants to repeat common actions.

Expected path:

1. Select project.
2. Build editor/runtime.
3. Cook changed content.
4. Launch.
5. Watch output only when something is running or failed.

### Troubleshooting

A developer sees a failed process and needs to understand what happened.

Expected path:

1. Identify which run failed.
2. See concise status and exit result.
3. Read or copy relevant output.
4. Retry the same operation with adjusted parameters.

### Maintenance

A developer wants to format or clean the workspace.

Expected path:

1. Open maintenance area.
2. Choose a narrow scope first.
3. Confirm destructive actions only when the selected scope is destructive.

## Current UI Strengths

- The app now has a single primary surface rather than several disconnected pages.
- The operation order is closer to a real first-run workflow: setup, build, cook, run, maintain.
- Project selection is contextual instead of a heavy global rail.
- Preview was removed, reducing mode confusion.
- Multiple running operations can be tracked through the activity list.
- The visual style is quieter than earlier iterations: dark shell, restrained blue accent, compact controls.
- Parameters are contextual and explicit; there are no free-form text fields for bounded options.

## Current UI Problems

### 1. The Screen Still Feels Like Three Separate Apps

The current window is visually divided into workflow navigation, parameter configuration, and activity/output. Each section has its own box and density. The result is functional, but it does not yet feel like one designed product surface.

Design risk:

- A reviewer may say the app is assembled from panels rather than composed as a single experience.
- The selected workflow and the activity stream do not feel related enough.

Improvement:

- Treat the selected workflow as the main content area and the activity area as a lower utility drawer.
- Reduce heavy panel borders and align the left nav, content, and activity grid to a shared spacing system.

### 2. The Left Navigation Uses Tabs and Lists Together

The workflow area uses tabs for categories and a vertical list for operations. This works, but it creates two levels of navigation inside a narrow sidebar. The tab row also competes visually with the `Workflows` title.

Design risk:

- A reviewer may ask why categories are tabs instead of a vertical step list or grouped sidebar.
- Tabs imply content panes of equal importance, while the product is actually a workflow sequence.

Improvement:

- Consider replacing horizontal tabs with a vertical grouped sequence:
  - Setup
  - Build
  - Cook
  - Run
  - Maintain
- Show operations under the active group or use collapsible group sections.
- Keep one selection model: either grouped list or tabs, not both if it feels crowded.

### 3. Workflow Labels Are Technically Accurate But Not Always User-Centered

Labels such as `Run Editor RHI Smoke Test` are precise, but they read like backend operation names. Some are long enough to make the UI feel heavier. Avoid slash-combined labels such as `Build Meshes / Scene Assets`; the button should say `Build Meshes` and let the description carry secondary detail.

Design risk:

- A reviewer may ask whether the UI is optimized for backend parity or user comprehension.

Improvement:

- Use short primary labels and move specificity into secondary metadata only when needed.
- Examples:
  - `Run Editor Smoke` instead of `Run Editor RHI Smoke Test`.
  - `Build Meshes` instead of `Build Meshes / Scene Assets`.
  - `Cook Project` instead of `Cook All Assets` if the backend operation is project-scoped.

### 4. The Selected Workflow Area Has Weak State Modeling

The selected workflow shows title, description, parameters, and Run. It does not yet communicate readiness, dependency expectation, last run, or whether the action is likely to be first-run safe.

Design risk:

- A reviewer may ask how the user knows whether a workflow is available, recommended, stale, risky, or already complete.

Improvement:

- Add restrained status metadata near the workflow title:
  - `Ready`
  - `Requires project`
  - `Destructive confirmation required`
  - `Last run failed`
  - `Running`
- Do not add verbose explanation by default. Use small status chips or muted inline metadata.

### 5. Parameters Are Better, But Still Feel Like Raw Form Controls

The parameter panel now uses compact rows, but it still visually resembles a settings form. For a workflow launcher, parameters should feel like scoped decisions for the selected operation.

Design risk:

- A reviewer may ask what is essential versus optional.
- The user may not know which controls are safe defaults and which are secondary/risky.

Improvement:

- Divide parameters by user importance when needed:
  - Primary: Project, Profile, Scope, Frame limit.
  - Secondary/risky: Force configure, Force recook, trace, skip level switching.
- Avoid visible `Advanced` section language. If uncommon options need hiding, use a quiet `More options` disclosure or reveal controls only when a selected scope makes them relevant.
- Show destructive confirmations only after a destructive option is enabled or selected.

### 6. Activity and Output Compete With Configuration

The activity section is always visible and consumes significant height even before anything runs. When output is active, it becomes the most information-dense area on the screen, but it has limited structure.

Design risk:

- A reviewer may ask whether activity should be a dock, a drawer, or a job monitor instead of permanent content.
- Output currently looks like terminal text pasted into a UI rather than an interpreted job log.

Improvement:

- Use a collapsed or compact activity state when no jobs have run.
- When jobs are running, expand the activity area automatically.
- Give each run a clear state: Queued, Running, Done, Failed.
- Add small actions later if supported: copy output, open log, clear completed.

### 7. Progress Is Global But Not Explanatory

The progress bar shows `0/2 complete` or similar. This is useful, but it does not explain whether operations are independent, sequential, failed, or blocked.

Design risk:

- A reviewer may ask what the progress model represents.

Improvement:

- Keep global progress, but pair it with a concise label such as `2 running` or `1 failed, 1 running`.
- Consider per-run progress only if the backend can provide real step progress.
- Avoid fake progress. If only completion count is known, present it honestly as job count.

### 8. Visual Hierarchy Needs More Confidence

The app is now calm, but some hierarchy is still accidental:

- `Workflows` is large relative to the selected operation title.
- The selected operation card and bottom activity panel both use strong containers.
- The Run button is visually clear but isolated in a large empty area.

Design risk:

- A reviewer may say the eye does not flow naturally from choose to configure to run to monitor.

Improvement:

- Make selected operation title the strongest text on the screen.
- Reduce `Workflows` title size and treat the left rail as navigation, not the page title.
- Move Run closer to the parameter content or align it with a footer that clearly belongs to the selected workflow.

### 9. Empty States Need To Be Useful Without Being Noisy

The no-selection and no-parameter states are restrained, which is good. They still need careful wording so the app does not feel blank or broken.

Design risk:

- A reviewer may ask what guidance exists for a new user without turning the UI into tutorial copy.

Improvement:

- No workflow selected: `Choose a workflow from the left.`
- No parameters: `No parameters`.
- No activity: `No runs yet`.
- Avoid long explanatory copy unless the user hits an error or a destructive action.

### 10. The App Needs Stronger Error and Recovery Design

The screenshots show running output, but not a designed failed state. Professional workflow tools are judged heavily on failure handling.

Design risk:

- A reviewer may ask: what happens when build fails, toolchain is missing, project discovery fails, or cook output is stale?

Improvement:

- Define explicit failed run styling.
- Show failure summary above raw output.
- Provide next actions where possible:
  - `Open log`
  - `Copy output`
  - `Run toolchain check`
  - `Retry`
- Keep raw output available but do not make it the only explanation.

## Information Architecture Recommendation

Use the fresh-sync sequence as the primary architecture:

```text
Setup -> Build -> Cook -> Run -> Maintain
```

### Setup

Purpose: make the local workspace ready.

Operations:

- Setup Workspace
- Check Toolchain
- Generate Solution

Design notes:

- This should be the first tab/group.
- These workflows usually have no parameters.
- Empty parameter state should be minimal.

### Build

Purpose: compile executable targets and tools.

Operations:

- Compile Editor
- Compile Runtime
- Build Cook Tools

Design notes:

- Project and profile are primary parameters.
- Force configure is secondary and should be tucked into a quiet `More options` reveal.
- Build Cook Tools belongs here because it prepares later cook operations.

### Cook

Purpose: prepare runtime content.

Operations:

- Cook Project
- Cook Shaders
- Build Textures
- Build Meshes

Design notes:

- Project and profile are primary.
- Shader package is primary only for Cook Shaders.
- Force recook and confirmation are secondary/risky and should not read as a generic settings section.

### Run

Purpose: launch and verify built output.

Operations:

- Run Editor
- Run Runtime
- Run Editor Smoke
- Run Runtime Smoke

Design notes:

- Smoke tests are verification workflows, so placing them after launch is logical.
- Frame limit is primary for smoke tests.
- Backend override and tracing are secondary diagnostic controls.

### Maintain

Purpose: clean and format after normal work.

Operations:

- Format Source
- Clean Workspace

Design notes:

- Maintenance should not sit in the first-run path unless the workspace is broken.
- Clean should lead with scope, then project if the scope requires it.

## Layout Direction

### Recommended Structure

```text
+---------------------------------------------------------------+
| Left workflow rail | Selected workflow                         |
|                    | Title / status                            |
| Setup              | Description                               |
| Build              | Parameters                                |
| Cook               | More options only when needed             |
| Run                |                              [Run]        |
| Maintain           |                                           |
+---------------------------------------------------------------+
| Activity / output drawer                                      |
+---------------------------------------------------------------+
```

### Left Rail

- Treat it as navigation, not content.
- Keep it narrow and stable.
- Prefer vertical groups over horizontal tabs if the tab row keeps feeling cramped.
- Show selected group and selected operation with one consistent accent treatment.

### Selected Workflow Panel

- This is the primary content area.
- Title should be the strongest text.
- Description should be one line when possible.
- Parameters should be compact and aligned.
- Secondary/risky options should be quiet and hidden unless needed.
- Run action should be anchored and visually connected to the selected workflow.

### Activity Drawer

- Empty state should be compact.
- Running state should expand enough to show useful output.
- Completed state should preserve the last run and allow inspection.
- Failed state should prioritize summary before raw output.

## Visual System Recommendations

### Color

Keep the GitHub Desktop-inspired dark direction, but reduce single-blue dominance.

Recommended roles:

- Background: deep neutral.
- Panel: slightly raised neutral.
- Border: subtle cool gray.
- Primary action/selection: blue.
- Success: muted green, only for completed states.
- Warning/destructive: amber/red, only when risk is real.
- Text primary: near-white.
- Text secondary: cool muted gray.

Avoid:

- Bright blue on too many elements at once.
- Green primary buttons if blue is the product accent.
- Large filled selected states competing with Run.

### Typography

- Use fewer sizes.
- Left navigation labels should be quieter than selected workflow title.
- Descriptions should be shorter and lower contrast.
- Avoid all-caps labels unless the app commits to a full enterprise form style.

Suggested hierarchy:

- Window content title: 16-18 px, semibold.
- Section label: 12-13 px, semibold.
- Body/controls: 12-13 px.
- Metadata: 11-12 px, muted.

### Spacing

- Use a consistent 4 px or 8 px spacing system.
- Parameter rows should have consistent height.
- Do not let available vertical space distribute between navigation rows.
- Avoid large empty voids inside cards unless the empty space is intentionally reserved for dynamic content.

### Borders and Panels

- Use fewer nested boxes.
- Prefer one primary panel plus subtle separators.
- If a section is visually inside another section, use spacing and alignment before adding another filled background.

### Icons

Icons can help, but only if used systematically.

Potential useful icons:

- Setup: tool/wrench.
- Build: hammer or package.
- Cook: layers/archive.
- Run: play.
- Maintain: broom/sliders.
- Success/failure/running states in activity list.

Avoid adding icons only to make the UI look busy.

## Interaction Design Requirements

### Selection

- Selecting a workflow should never run it.
- Selection should update parameters and status immediately.
- The selected operation should remain visible when a run starts.

### Running

- Run button should be disabled until a workflow is selected.
- Run button text should remain simple: `Run`.
- Concurrent runs are allowed, so the activity list must make parallel jobs obvious.
- If cancellation is not implemented, do not show Stop.

### Secondary And Risky Options

- Secondary options should be contextual, not global.
- Potentially destructive options should reveal confirmation requirements.
- Defaults should be obvious without long explanatory copy.

### Output

- Raw output must remain available.
- The app should eventually add a summary layer above raw output.
- Output should use a readable monospace font, adequate line height, and clear selection/copy behavior.

### Error Handling

- Failed runs should have a distinct visual state.
- Missing toolchain should direct the user to Setup or Check Toolchain.
- Missing project should show a concise empty state, not a broken combo box experience.
- Destructive operations should require confirmation at the point of action.

## Content Design Rules

Use text only when it changes user behavior.

Good copy:

- `No parameters`
- `No runs yet`
- `Requires confirmation`
- `Failed: CMake configure returned exit code 1`
- `Project`
- `Profile`
- `Scope`

Avoid copy:

- `Choose one action to configure` if the layout already makes that clear.
- Long descriptions under every operation.
- Repeating the operation name inside the parameter form.
- Explaining what a dropdown is.

## Accessibility and Usability Checklist

- Keyboard navigation reaches workflow groups, operation rows, fields, Run, activity list, and output.
- Focus states are visible and not identical to hover states.
- Text contrast remains readable on the dark background.
- Selected state is not communicated by color alone.
- Long operation names truncate or wrap predictably.
- Window works at minimum supported size without overlapping controls.
- Output is selectable and copyable.
- Destructive actions use both color and text.

## Interview-Style Questions This Design Should Answer

### Product and User Questions

- Who is the primary user: engine developer, technical artist, build engineer, or new contributor?
- What is the first action after syncing the repo?
- Which workflows are daily-use versus rare-use?
- What should the user understand in the first 10 seconds?
- What user mistake is most expensive?

### Information Architecture Questions

- Why are operations grouped this way?
- Why is maintenance last?
- Why are smoke tests under Run instead of Maintain?
- Why is Build Cook Tools under Build instead of Cook?
- What happens when a user does not know which operation they need?

### Interaction Questions

- Why does selecting not run?
- How does the user know a workflow is ready?
- How are concurrent runs represented?
- What happens if two runs fail at once?
- How does the user recover after failure?

### Visual Design Questions

- What is the primary visual hierarchy?
- Why this color system?
- Why dark UI for a workflow launcher?
- How does the UI avoid looking like a console skin?
- Which elements are intentionally quiet?

### Systems Questions

- What is the spacing system?
- What are the typography roles?
- What reusable components exist?
- What states does each component support?
- How will the design scale when more operations are added?

### Accessibility Questions

- Can the launcher be used by keyboard?
- Are focus states clear?
- Is selected state visible without relying only on color?
- Does output remain readable at smaller sizes?
- How are errors announced visually?

### Engineering Questions

- Which design changes require backend support?
- Which changes are UI-only?
- How does the GUI preserve operation parity?
- How do we avoid duplicating backend workflow logic in the UI?
- How does the UI stay maintainable in Qt Widgets?

## Proposed Component Model

### WorkflowRail

Responsibilities:

- Shows workflow groups in expected order.
- Shows operation rows within the selected group.
- Owns selection visuals only.

States:

- Default
- Hover
- Selected
- Running child operation
- Failed child operation

### WorkflowDetail

Responsibilities:

- Shows selected operation title, short description, status, and parameters.
- Hosts primary Run action.

States:

- No selection
- No parameters
- Has parameters
- Running selected operation
- Last run failed
- Destructive confirmation required

### ParameterForm

Responsibilities:

- Renders operation-specific controls.
- Separates primary controls from secondary or risky controls without advertising a generic `Advanced` mode.
- Avoids global settings clutter.

States:

- Empty
- Primary only
- Primary plus more options collapsed
- Primary plus more options expanded

### ActivityMonitor

Responsibilities:

- Shows running, completed, and failed jobs.
- Shows output for selected job.
- Summarizes progress honestly.

States:

- No runs
- Running
- Completed
- Failed
- Mixed concurrent runs

## Improvement Roadmap

The stages below are written so each can be copied directly into an implementation prompt. Each stage should be completed as a focused UI pass. Keep backend behavior unchanged unless the stage explicitly calls out a backend-supported state or action.

## Current Implementation Check

The current built launcher is moving in the right direction: the workflow rail, selected workflow panel, contextual parameters, and activity drawer now read as one focused desktop tool rather than a console wrapper. Setup, Build, Cook, Run, and Maintain are visible in the expected order, and the visual system is compact enough for daily use.

The remaining corrections are product-language and polish issues, not architecture reversals:

- Operation labels should be short and product-facing. Avoid slash-combined button text; `Build Meshes` is enough.
- The parameter area should not expose an `Advanced` section as a visible concept. Use `More options`, contextual reveal, or direct inline controls depending on the risk and frequency of the option.
- Blue should stay mostly reserved for the selected operation and primary `Run` action. Progress and completed states should stay neutral or state-colored.
- The activity drawer is useful, but screenshots should keep checking that it does not visually overpower configuration after a completed run.
- Remaining stages should focus on robustness, focus states, minimum-size behavior, and failure/no-project recovery rather than adding new features.

### Stage 1: Workflow Structure and Hierarchy

Implementation prompt:

```text
Refine the Sparkle Launcher main window hierarchy so the app reads as a workflow-first desktop tool. Keep the existing Qt Widgets architecture and backend operations. Focus on the left workflow chooser, selected workflow detail area, and overall visual hierarchy. Do not add new launcher workflows or backend features.
```

Goal:

- Make the existing layout feel intentionally composed instead of assembled from independent panels.
- Make the selected workflow the primary focus of the screen.
- Preserve the fresh-sync order: Setup, Build, Cook, Run, Maintain.

Likely files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`

Implementation tasks:

- Reduce the visual weight of the `Workflows` heading and remove redundant instruction copy if the layout already communicates selection.
- Evaluate replacing the horizontal category tabs with a vertical grouped workflow rail. If keeping tabs, reduce their visual competition with the operation list.
- Make the selected operation title the strongest text on the screen.
- Align the workflow rail, detail panel, and activity panel to a shared spacing grid.
- Keep the no-selection state minimal: one short instruction, no large explanatory block.
- Keep the no-parameter state minimal: `No parameters`.

Positive guardrails:

- Prefer fewer nested panels and stronger alignment over new decoration.
- Preserve compact GitHub Desktop-style restraint.
- Keep selection separate from execution; selecting a workflow must not run it.

Negative guardrails:

- Do not add landing-page, hero, or marketing-style UI.
- Do not add global project rails or free-form parameter inputs.
- Do not make maintenance actions prominent in the first-run path.

Validation:

- Build `SparkleLauncher` at the end of the stage.
- Refresh and launch the packaged executable.
- Run `git diff --check`.
- Capture or inspect screenshots for Setup, Build, Cook, Run, and Maintain selections.

Acceptance criteria:

- A fresh launch feels calm without needing explanation.
- A new user can infer the expected workflow order within 10 seconds.
- The selected workflow title is visually stronger than left navigation labels.
- The UI still exposes every existing operation.

### Stage 2: Contextual Parameter Design

Implementation prompt:

```text
Redesign the selected workflow parameter area so settings feel contextual, elegant, and low-noise. Keep all existing operation options reachable, but separate primary controls from secondary or risky controls without presenting a generic Advanced section. Do not introduce free-form text fields for bounded options.
```

Goal:

- Make operation settings feel like scoped workflow decisions, not a raw settings form.
- Reduce visible controls for common paths.
- Make risky options obvious only when relevant.

Likely files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherSettings.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherSettings.cpp`

Implementation tasks:

- Split operation parameters into primary, secondary, and risky controls.
- Keep primary controls visible:
  - Project
  - Profile
  - Scope
  - Shader package
  - Frame limit
- Move secondary or riskier controls behind a quiet `More options` disclosure, or reveal them only when the selected scope makes them relevant:
  - Force configure
  - Force recook
  - Confirm recook cleanup
  - Backend override
  - Enable trace
  - Skip level switching
  - Confirm clean
- Reveal destructive confirmation controls only when the destructive option or scope makes them relevant.
- Standardize parameter row widths, control heights, row gaps, and label alignment.
- Keep no-parameter workflows visually calm and short.

Positive guardrails:

- Use explicit dropdowns and checkboxes for bounded choices.
- Keep common workflows runnable without opening secondary controls.
- Prefer local helper methods in `LauncherMainWindow` over new tiny classes unless repeated logic becomes hard to maintain.

Negative guardrails:

- Do not remove any existing backend-supported option.
- Do not use long helper copy beside every control.
- Do not make secondary options global if they only affect one operation family.

Validation:

- Build `SparkleLauncher` at the end of the stage.
- Refresh and launch the packaged executable.
- Run `git diff --check`.
- Inspect Build, Cook Shaders, Smoke Test, Clean Workspace, and no-parameter workflows.

Acceptance criteria:

- Build, Cook, Run, and Maintain parameter pages feel like variations of one component.
- Common paths show only primary controls by default.
- Secondary/risky controls are still reachable and understandable.
- No parameter page contains redundant section titles or unnecessary explanatory text.

### Stage 3: Activity and Output Experience

Implementation prompt:

```text
Redesign the activity and output area so long-running workflows are easy to monitor and failures are easier to recover from. Preserve concurrent run support and existing backend signals. Do not fake progress that the backend does not provide.
```

Goal:

- Make running, completed, and failed work inspectable at a glance.
- Reduce idle-state visual weight.
- Keep raw output available without making it the only source of meaning.

Likely files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherBackend.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherBackend.cpp`

Implementation tasks:

- Compact the activity panel when no run has started.
- Expand or visually emphasize the activity panel when one or more jobs are running.
- Add clear per-run states in the activity list:
  - Queued
  - Running
  - Done
  - Failed
- Keep global progress honest as job count progress, not step progress.
- Show a concise failure summary before raw output when a run fails.
- If backend log paths are already available, consider `Open log`; otherwise defer it.
- Consider `Copy output` as a UI-only action because `QTextEdit` already holds the output text.

Positive guardrails:

- Preserve the ability to inspect output from older runs.
- Make concurrent runs visually distinct.
- Keep raw output selectable and readable.

Negative guardrails:

- Do not add Stop or Cancel unless backend cancellation exists.
- Do not simulate progress percentages.
- Do not hide raw output after failures.

Validation:

- Build `SparkleLauncher` at the end of the stage.
- Refresh and launch the packaged executable.
- Run `git diff --check`.
- Start two concurrent workflows and verify the activity list remains understandable.
- Inspect idle, running, completed, and failed or failure-like states where available.

Acceptance criteria:

- Idle activity state is compact and not visually dominant.
- Two concurrent runs are understandable at a glance.
- A failed run can be identified without reading the entire log.
- Output remains available for every tracked run.

### Stage 4: Visual System Hardening

Implementation prompt:

```text
Harden the Sparkle Launcher visual system so the UI feels consistent, modern, and intentionally designed. Keep the current dark desktop-tool direction, but consolidate color, spacing, typography, and state styling into a coherent local design system.
```

Goal:

- Make Setup, Build, Cook, Run, and Maintain screens look like one product.
- Reduce reliance on incidental Qt default spacing and styling.
- Keep the visual language modest and elegant.

Likely files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`
- Optional only if structure demands it: `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h`

Implementation tasks:

- Define local style roles in the stylesheet for:
  - Background
  - Panel
  - Border
  - Primary action
  - Selection
  - Success
  - Warning/destructive
  - Primary text
  - Secondary text
- Standardize margins, row heights, panel gaps, control heights, and border radii.
- Reduce single-blue dominance by reserving filled blue for active selection and primary action.
- Add status colors only where state needs them.
- Add icons only where they clarify workflow group or run status.
- Verify operation labels are short and product-facing; avoid slash-combined button labels.
- Avoid visible `Advanced` language in the parameter area; uncommon options should feel like secondary workflow options, not a settings form.
- Ensure text hierarchy stays restrained and readable at the minimum window size.

Positive guardrails:

- Prefer a small number of reusable object names and style rules.
- Keep the design close to a native desktop productivity app.
- Use spacing and typography before adding new containers.

Negative guardrails:

- Do not introduce decorative gradients, hero sections, or marketing visuals.
- Do not add icons inconsistently.
- Do not make every selected or focused element bright blue.
- Do not reintroduce `Build Meshes / Scene Assets` or similar backend-combined labels.

Validation:

- Build `SparkleLauncher` at the end of the stage.
- Refresh and launch the packaged executable.
- Run `git diff --check`.
- Inspect screenshots of all workflow groups and at least one running state.

Acceptance criteria:

- Screenshots of each workflow group clearly belong to the same design system.
- Visual hierarchy is consistent across selected operations.
- No section appears as an accidental nested Qt default widget.
- The app remains compact and calm.

### Stage 5: Accessibility, Edge Cases, and Production UX

Implementation prompt:

```text
Audit and improve Sparkle Launcher accessibility, edge cases, and production UX states from the current built UI. Focus on keyboard usability, focus visibility, minimum window behavior, empty states, failure states, and screenshot-based implementation checks. Keep feature scope limited to UX robustness.
```

Goal:

- Make the launcher robust under real desktop usage.
- Ensure empty and failed states feel designed.
- Make the UI easier to defend in a professional product review.

Likely files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherProjectModel.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherProjectModel.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherBackend.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherBackend.cpp`

Implementation tasks:

- Verify and adjust keyboard tab order through workflow groups, operation rows, parameters, Run, activity list, and output.
- Ensure focus states are visible and distinct from hover and selected states.
- Test the minimum window size and fix overlapping or clipped text.
- Improve no-projects-discovered state.
- Improve project discovery failure messaging.
- Improve missing-toolchain path by directing the user toward Setup or Check Toolchain.
- Improve multiple concurrent failure presentation.
- Ensure selected state is not communicated by color alone.
- Re-check all workflow screenshots after fixes so labels, parameter controls, activity output, and visual hierarchy still feel like one product.

Positive guardrails:

- Prefer clear state and recovery over more explanatory copy.
- Keep raw technical detail available after concise failure summary.
- Preserve existing backend operation parity.

Negative guardrails:

- Do not add a tutorial system.
- Do not add nonfunctional buttons.
- Do not hide failures behind generic messages.

Validation:

- Build `SparkleLauncher` at the end of the stage.
- Refresh and launch the packaged executable.
- Run `git diff --check`.
- Manually test keyboard navigation.
- Manually test minimum window size.
- Test or simulate no-project and failed-run states where practical.

Acceptance criteria:

- The app can be navigated without mouse-only assumptions.
- Focus is visible and predictable.
- Empty states are short and useful.
- Failure states communicate what happened and preserve raw output.
- The launcher feels robust enough for daily engine workflow use.

## Near-Term Design Direction

The next implementation pass should treat the current structure as directionally correct and focus on product polish plus robustness:

1. Keep operation labels short and user-facing.
2. Replace `Advanced` framing with quiet `More options` or contextual reveals.
3. Keep blue reserved for active selection and the primary `Run` action.
4. Continue screenshot inspection across Setup, Build, Cook, Run, Maintain, and active output states.
5. Use the final stage for keyboard, focus, minimum-size, empty-state, and failure-state hardening.

This preserves the current Qt architecture and backend integration while moving the product toward a more mature, interview-defensible design.

## Success Criteria

The launcher is stronger when:

- A fresh-sync user can infer the correct operation order without reading documentation.
- The screen has one obvious primary focus at any time.
- Common workflows require no secondary controls.
- Risky options are visible only when they matter.
- Activity output helps users recover, not just observe text.
- The UI feels intentionally designed rather than assembled from Qt widgets.
- Every visual decision can be explained in terms of user workflow, clarity, state, or risk.