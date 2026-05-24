# Sparkle Launcher Design Strengthening Review

## Purpose

This document critiques the current Sparkle Launcher GUI from a top-tier product design perspective and defines the next design bar. The goal is not to add decoration. The goal is to make the launcher feel like a calm, modern, professional desktop tool that can be defended in a serious UX/UI design interview.

The launcher should help a developer who just synced the repository understand what to do next, run workflows without using a console, and monitor long-running work without feeling buried in terminal noise.

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

Labels such as `Build Meshes / Scene Assets` and `Run Editor RHI Smoke Test` are precise, but they read like backend operation names. Some are long enough to make the UI feel heavier.

Design risk:

- A reviewer may ask whether the UI is optimized for backend parity or user comprehension.

Improvement:

- Use short primary labels and move specificity into secondary metadata only when needed.
- Examples:
  - `Run Editor Smoke` instead of `Run Editor RHI Smoke Test`.
  - `Build Scene Assets` instead of `Build Meshes / Scene Assets`.
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
- The user may not know which controls are safe defaults and which are advanced/risky.

Improvement:

- Divide parameters into two groups when needed:
  - Primary: Project, Profile, Scope, Frame limit.
  - Advanced: Force configure, Force recook, trace, skip level switching.
- Hide advanced checkboxes behind an `Advanced` disclosure within the selected workflow.
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
- Force configure is advanced.
- Build Cook Tools belongs here because it prepares later cook operations.

### Cook

Purpose: prepare runtime content.

Operations:

- Cook Project
- Cook Shaders
- Build Textures
- Build Scene Assets

Design notes:

- Project and profile are primary.
- Shader package is primary only for Cook Shaders.
- Force recook and confirmation are advanced and potentially risky.

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
- Backend override and tracing are advanced.

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
| Cook               | Advanced disclosure                       |
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
- Advanced options should be hidden unless needed.
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

### Advanced Options

- Advanced options should be contextual, not global.
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
- Separates primary and advanced controls.
- Avoids global settings clutter.

States:

- Empty
- Primary only
- Primary plus advanced collapsed
- Primary plus advanced expanded

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

### Stage 1: Structure and Hierarchy

Goal: make the existing layout feel intentional without adding backend work.

Tasks:

- Reduce `Workflows` title weight and remove redundant instruction text.
- Evaluate replacing category tabs with vertical grouped navigation.
- Make selected workflow title the primary text on screen.
- Align left rail, detail panel, and activity panel to the same margin grid.
- Keep no-selection and no-parameter states minimal.

Validation:

- Fresh launch screenshot feels calm without requiring explanation.
- A new user can identify the first expected action in under 10 seconds.

### Stage 2: Parameter Design

Goal: make operation settings feel contextual and professional.

Tasks:

- Split primary settings from advanced settings.
- Hide force/trace/destructive checkboxes behind contextual Advanced disclosure.
- Show destructive confirmations only when relevant.
- Standardize parameter row widths and control heights.

Validation:

- Build, Cook, Run, and Maintain panels all look like variations of one component.
- No operation shows unnecessary text.

### Stage 3: Activity and Output

Goal: make long-running work feel inspectable instead of noisy.

Tasks:

- Compact activity area when no run exists.
- Add run status styling for queued, running, done, and failed.
- Add failure summary above output when a run fails.
- Consider `Copy output` and `Open log` actions if backend paths are available.

Validation:

- Two concurrent runs are understandable at a glance.
- Failed run recovery is obvious without reading the entire log.

### Stage 4: Visual System Hardening

Goal: make the launcher feel like a production design system.

Tasks:

- Define color tokens in one local style section.
- Define spacing constants for margins, rows, gaps, and panels.
- Define typography roles and apply them consistently.
- Add iconography only where it clarifies workflow or status.

Validation:

- Screenshots of Setup, Build, Cook, Run, and Maintain look like one product.
- No section relies on accidental Qt default spacing.

### Stage 5: Accessibility and Edge Cases

Goal: make the launcher robust under real desktop usage.

Tasks:

- Verify keyboard tab order.
- Verify focus rings and contrast.
- Test minimum window size.
- Test no projects discovered.
- Test failed toolchain discovery.
- Test multiple concurrent failures.

Validation:

- The app remains usable without mouse-only assumptions.
- Empty and failed states are designed, not incidental.

## Near-Term Design Direction

The next implementation pass should focus on structure before visual decoration:

1. Convert the left workflow chooser into a stronger workflow sequence.
2. Reduce title/instruction noise in the left rail.
3. Add contextual Advanced sections inside parameter pages.
4. Compact the activity area when idle.
5. Add clearer job status states.

This preserves the current Qt architecture and backend integration while moving the product toward a more mature, interview-defensible design.

## Success Criteria

The launcher is stronger when:

- A fresh-sync user can infer the correct operation order without reading documentation.
- The screen has one obvious primary focus at any time.
- Common workflows require no advanced controls.
- Risky options are visible only when they matter.
- Activity output helps users recover, not just observe text.
- The UI feels intentionally designed rather than assembled from Qt widgets.
- Every visual decision can be explained in terms of user workflow, clarity, state, or risk.