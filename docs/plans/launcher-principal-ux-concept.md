# Sparkle Launcher Principal UX Concept

Date: 2026-06-03

Status: concept and product-direction document. Do not implement until the design direction is accepted.

## Product Intent

Sparkle Launcher should feel like an engine command center, not a CMake front-end. Its first job is to help any first-time user launch something meaningful quickly. Its second job is to help that same user grow naturally into deeper production workflows: prepare, build, cook, validate, package, diagnose, and maintain. Its third job is to keep advanced rebuild, recook, package, diagnostic, and maintenance workflows available without letting them dominate the first impression.

The launcher should answer these questions in order:

1. What can I run right now?
2. If I cannot run it, what is the smallest next action?
3. What capability will that action unlock?
4. What did the action change?
5. Where do I go if I need deeper logs, paths, or diagnostics?

The first screen must serve two usage depths without splitting the product in two:

1. A first-contact path for someone who needs immediate proof that the package runs and that the engineering underneath is serious.
2. A production path for someone who needs reliable access to prepare, build, cook, validate, package, and maintain workflows.

The solution is not a separate "reviewer mode" or "demo mode" that hides the real system. The solution is a real Quick Start home surface that uses the same readiness model as production workflows, but presents it in first-contact order: launch first, evidence second, rebuild/recook third.

## Current-State Read From Screenshots

Strengths:

- Workflow groups already separate Start, Setup, Build, Cook, Run, Package, and Maintenance.
- Actions are generally outcome-oriented rather than raw function names.
- Dependency tiers are visible and mostly separated from host prerequisites.
- The app has useful readiness data and direct recovery actions.
- Bottom project/build/IDE selectors support daily iteration.

Primary UX problems:

- The app gives equal visual weight to everything. Host tools, source tiers, generated files, paths, optional tools, and recovery hints all appear as nearly identical rows.
- The main content area reads like a status audit table instead of a guided workflow surface.
- The bottom log panel is always visually loud, even when it contains an old failed clean unrelated to the selected workflow.
- The screenshots show a repeated old failure across every page, which makes the entire launcher feel broken even when the selected workflow is ready.
- Pages often explain what the workflow changes, then immediately show long prerequisite lists. The user has to infer the next best action.
- Many pages expose local machine paths inline. Paths are useful, but they should live in details, tooltips, copyable diagnostics, or an expandable technical drawer.
- The second-level navigation duplicates internal operation structure. It helps power users, but it makes first-run users scan too many choices before they know what matters.
- Start workflows are visually weaker than setup/build/cook workflows, even though launch-first should be the first-contact priority.
- Build and Cook pages repeat the same Host Rebuild Dependencies block, creating clutter and attention fatigue.
- Package Release currently looks underpowered and abstract compared to other workflows, despite being a major product milestone.

## Design Principles

- Launch-first: a packaged user should see a clear primary launch path before any rebuild or recook language.
- Capability-first: dependencies are shown as capability unlocks, not as a wall of missing prerequisites.
- One recommended next action: each screen should elevate one primary action and one recovery action at most.
- Progressive disclosure: raw paths, full dependency lists, command lines, and logs are available, but secondary.
- Stable mental model: actions should be grouped by user intent, not implementation category.
- Signal over inventory: show "ready, blocked, stale, optional, disabled" at summary level first; details are expandable.
- Old failures should not poison unrelated pages. A stale failed maintenance run belongs in Activity, not as global emotional noise.
- Labels should speak user outcomes: Open, Prepare, Build, Cook, Validate, Package, Clean.
- Product and source states must be distinct: "Ready from package" is not the same as "Ready from local build."
- Progressive depth: first-contact and daily production use the same data and workflows; the default screen simply prioritizes quick launch and engineering evidence before operational inventory.
- No artificial roles: the app should not expose `Reviewer Mode`, `Developer Mode`, or portfolio-specific copy. It should feel like a real launcher that happens to be easy to evaluate quickly.

## Target User Funnel

These are usage journeys, not product roles. The launcher should not ask the user to choose a persona. A first-time evaluator and a daily developer should use the same app, with the same workflows, and the deeper production path should feel like a continuation of Quick Start.

### 0. First Contact And Quick Start

Goal:

- understand what Sparkle is within seconds
- launch the strongest available demo without reading setup docs
- trust that the project is engineered, not only visually presented
- know where to inspect architecture, source rebuild requirements, package manifests, and logs if time allows

Assumption:

- the launcher may be the first and only product surface someone opens
- the user may have 5 to 15 minutes during first contact
- the user may not want to rebuild or recook anything immediately
- an external evaluator may be judging technical clarity as much as raw feature count
- the launcher is the first product surface and must behave like a confident tour guide

Ideal path:

1. Launcher opens on Home / Quick Start.
2. Header states what the package is: `Sparkle Engine Showcase`.
3. The first card says whether the user can run immediately: `Ready to explore`, `Package incomplete`, or `Source checkout requires preparation`.
4. Primary CTA is `Open Showcase Editor`.
5. Secondary CTA is `Open Runtime`.
6. A small `Engineering evidence` area links to architecture docs, manifests, source rebuild requirements, and latest validation report.
7. Rebuild, recook, sync, and clean actions are available but not pushed into the first decision.

First-contact questions:

1. Can I run something immediately?
2. What am I looking at and why should I care?
3. Is this a real engineered system or a loose demo?
4. Where is the evidence: source layout, dependency model, manifests, validation, logs, architecture?
5. If I choose to rebuild, are the requirements honest and well structured?

Time-boxed journey:

| Time | User Outcome | Launcher Responsibility |
| --- | --- | --- |
| 0 to 30 seconds | Understand project identity and launch status | Show title, mode, selected project, strongest available CTA, and any launch blocker in one sentence |
| 2 minutes | Run editor or runtime, or know the exact blocker | Prefer packaged components; if blocked, show the smallest repair action |
| 5 minutes | Inspect engineering depth | Provide one-click evidence links for architecture, renderer/backend notes, dependency tiers, manifests, validation report, and source layout |
| 15 minutes | Evaluate rebuild discipline | Guide to Prepare Source Workspace, Build Missing, Cook Missing, and package manifests without forcing full dependency sync |

Quick-start screen:

```text
Sparkle Engine Showcase                                      Source checkout
Realtime renderer, editor/runtime sample, cooked content pipeline, release packaging.

Ready to explore
Open the Showcase editor or runtime now. Rebuild and recook are optional.

[Open Showcase Editor] [Open Runtime]

If you have more time
[View Architecture] [View Package Manifest] [Prepare Source Workspace]

Engineering snapshot
Renderer: D3D12 + Vulkan | Project: Showcase | Build: Development | Package: source checkout
```

What this first-contact experience must not do:

- start with a wall of dependencies
- imply the user must sync all source dependencies
- show stale unrelated failures as the dominant visual state
- expose local machine paths before explaining the product
- make package/rebuild/cook vocabulary compete with the first launch action
- expose a role switch or copy that says the app was made specifically for an interview/reviewer

Evidence surface:

```text
Engineering evidence

Architecture        Renderer architecture, product boundaries, artifact layout
Runtime proof       Build/package manifest, commit, toolchain, Qt kit, checksums
Dependency model    Host prerequisites, source tiers, optional capability unlocks
Validation          Latest package/build validation report and smoke-test logs
Source map          Where launcher, editor, runtime, cook tools, and projects live
```

Evidence rules:

- evidence links should be visible from Home, but visually secondary to launch
- each link should open a real file, manifest, report, folder, or diagnostics view
- do not use marketing-only claims; use concrete artifacts and short explanations
- if an evidence artifact is missing, say exactly which workflow produces it

### 1. First-Run User

Goal:

- open launcher
- understand Sparkle is a real engine package
- launch editor or runtime without rebuilding
- optionally inspect source/dependencies later

Ideal path:

1. Launcher opens on Command Center.
2. Hero card says `Showcase ready from package` or `Showcase needs 2 generated outputs`.
3. Primary CTA is `Open Editor` or `Open Runtime`.
4. If blocked, CTA becomes `Build Missing Runtime` or `Cook Missing Assets`, not a generic setup instruction.
5. Technical requirements are collapsed under `Why is this blocked?`.

### 2. Source Developer

Goal:

- verify host tools
- sync only required source tiers
- generate project files
- build editor/runtime
- run and iterate

Ideal path:

1. Command Center says `Source workspace needs refresh`.
2. Recommended action is `Prepare Source Workspace`.
3. Prepare screen shows a compact sequence: Host Tools, Source Tiers, Project Files.
4. Developer fixes the first blocker only.
5. Build/Run become available.

### 3. Content Or Shader Developer

Goal:

- build cook tools
- recook only affected assets
- understand optional dependency tiers

Ideal path:

1. Cook screen shows asset domains as cards: Shaders, Textures, Scene Assets.
2. Each card shows Ready, Missing, Stale, or Disabled.
3. Missing domain links to exact cook action.
4. Advanced shader options stay collapsed unless `Advanced shader cook` is opened.

### 4. Release Builder

Goal:

- assemble a reviewable package
- verify what is included
- produce manifests and checksums
- keep publish sign-off separate

Ideal path:

1. Package screen shows release readiness summary.
2. It lists missing package inputs, not a vague planned status.
3. Primary CTA is `Assemble Review Package`.
4. Secondary CTA is `Open Dist Folder`.
5. Final validation remains a separate checklist/report.

## Proposed Information Architecture

Current groups:

- Start
- Setup
- Build
- Cook
- Run
- Package
- Maintenance

Proposed groups:

- Home / Quick Start
- Launch
- Prepare
- Build
- Cook
- Validate
- Package
- Maintain

Rationale:

- `Home / Quick Start` becomes the command center and first-run funnel.
- `Launch` replaces `Start`; "Start" is too generic and visually weak for the most important first-contact path.
- `Prepare` replaces `Setup`; "Prepare" implies outcome and readiness, while "Setup" sounds like installation chores.
- `Validate` replaces `Run` for smoke tests and diagnostic launch. Normal running belongs under Launch.
- `Maintain` is shorter and less bureaucratic than Maintenance.

Proposed primary navigation:

| Group | Primary User Question | Actions |
| --- | --- | --- |
| Home / Quick Start | What should I do next? | Command Center, Run Showcase, View Evidence |
| Launch | What can I open now? | Open Editor, Open Runtime |
| Prepare | What does this machine/workspace need? | Prepare Source Workspace, Verify Host Tools, Sync Source Tiers, Generate Project Files, Open IDE |
| Build | What do I want to rebuild locally? | Build Missing, Build Editor, Build Runtime, Build Launcher, Build Cook Tools, Build All |
| Cook | What generated content do I need? | Cook Missing, Cook All, Cook Shaders, Cook Textures, Cook Scene Assets |
| Validate | How do I test or diagnose? | Run Smoke Test, Run Custom |
| Package | How do I assemble a review package? | Assemble Review Package, Open Dist Folder |
| Maintain | What generated state should I clean? | Clean Workspace, Format Code |

## Proposed Screen Model

Every workflow screen should use the same hierarchy:

1. Header: action name, short purpose, current state badge.
2. Recommended next action card: one sentence, primary CTA, secondary recovery if needed.
3. Readiness summary: 3 to 5 high-level checks only.
4. Options: only user-adjustable inputs needed for this workflow.
5. Details drawer: paths, full dependency rows, command preview, raw logs.
6. Activity panel: recent runs, scoped to relevance by default.

This reverses the current hierarchy. Today, details come first and the user has to synthesize the action.

## Command Center Concept

Purpose:

- replace blank first impression with a high-confidence operating picture
- make the launch-first path obvious
- separate package readiness from source rebuild readiness
- support a first-time user or external evaluator who needs a fast product read before deep inspection
- keep production workflows nearby without asking users to understand the whole pipeline first

Layout sketch:

```text
Sparkle Engine Showcase

[Showcase] [Development] [Visual Studio]              [Architecture] [Diagnostics] [Settings]

+------------------------------------------------------------------------------+
| Ready to explore / Needs local outputs / Source workspace stale               |
| Open the strongest available demo first. Rebuild and recook are optional.     |
|                                                                              |
| [Open Showcase Editor] [Open Runtime]   Secondary: Build Missing | Cook Missing|
+------------------------------------------------------------------------------+

+---------------+ +---------------+ +---------------+ +---------------+
| Launch        | | Source        | | Content       | | Package       |
| Editor ready  | | Tools ready   | | 3 missing     | | Review ready? |
| Runtime ready | | Files stale   | | Cook missing  | | Assemble      |
+---------------+ +---------------+ +---------------+ +---------------+

Next best action
1. Generate Project Files
   CMake cache differs from selected generator/platform/toolset/Qt kit.
   [Generate]

Engineering evidence
[Architecture] [Dependency Tiers] [Manifests] [Latest Validation]

Recent activity
Clean Workspace failed because launcher artifacts were locked.
[Resolve] [Dismiss] [View log]
```

Quick Start home layout:

```text
Sparkle Engine Showcase                                  Package Mode / Source Mode
Launch first. Rebuild and recook are optional unless you want to inspect the source pipeline.

+------------------------------------------------------------------------------+
| Ready to explore                                                             |
| The packaged Showcase editor and runtime are available.                      |
| [Open Showcase Editor] [Open Runtime]                                        |
+------------------------------------------------------------------------------+

+-------------------------+ +-------------------------+ +----------------------+
| Review engineering      | | Continue as developer   | | Current evidence     |
| Architecture            | | Prepare Source Workspace| | Manifest: available  |
| Dependency tiers        | | Build Missing           | | Validation: latest   |
| Validation report       | | Cook Missing            | | Source: detected     |
+-------------------------+ +-------------------------+ +----------------------+
```

Production home behavior:

- if a prior production workflow was active, Home may show that workflow as the recommended next action, but launch status still remains visible
- daily production shortcuts should be persistent: Prepare, Build Missing, Cook Missing, Validate, Package, Maintain
- quick-start copy should never block or obscure expert operations; it should simply set the default attention order

## Phase 0 Handoff: First-Contact Quick Start Contract

Status:

- Direction A: Command Center is the accepted target for this contract.
- This phase defines product copy, state names, CTA priority, evidence ownership, and stale-failure behavior only.
- No launcher code, build, launch, or package validation is part of this phase.

### Home Identity And Hero Copy

Home title:

- `Sparkle Engine`

Project subtitle:

- `Showcase project`
- Use the selected project name when the launcher supports multiple projects.

Mode labels:

| Label | Meaning | Source |
| --- | --- | --- |
| `Package Mode` | Launcher is running from an assembled package root with package manifests available | package-root discovery, `dist/releases/<version>/.../manifests/` |
| `Source Checkout` | Launcher is running from a repository checkout with source/project markers available | repository root discovery |
| `Source Checkout + Local Artifacts` | Source checkout has local runnable artifacts under `artifacts/dev` | artifact-root discovery |
| `Mixed State` | Package or source markers exist, but expected manifests/artifacts are incomplete | root discovery plus manifest/artifact checks |

Hero state labels:

| State | Copy | Primary Meaning |
| --- | --- | --- |
| `Ready to explore` | `Open the strongest available Showcase target. Rebuild and recook are optional.` | At least one editor/runtime launch target is available from package or local artifacts |
| `Ready from package` | `This launch uses bundled package components. Local rebuilds are optional.` | Package editor/runtime and required cooked content are present |
| `Ready from local build` | `This launch uses local development artifacts. Package components are not required.` | Local editor/runtime and required content are present |
| `Needs one action` | `One preparation step unlocks the selected launch path.` | One clear missing/stale blocker maps to one workflow |
| `Source checkout requires preparation` | `Prepare the workspace before local rebuilds. Package launch may still be available.` | Source exists but workspace files/dependencies/artifacts are not ready |
| `Package incomplete` | `Expected package files or manifests are missing. Use a complete package or switch to source workflows.` | Package markers exist but package launch cannot be trusted |

Hero CTA order:

1. `Open Editor` when editor launch is ready.
2. `Open Runtime` when runtime launch is ready and editor is unavailable.
3. `Build Missing` when a local executable is the smallest blocker.
4. `Cook Missing` when cooked content is the smallest blocker.
5. `Generate Workspace Files` when stale/missing project files block build/cook workflows.
6. `Prepare Source Workspace` when multiple source-preparation blockers exist.
7. `View Details` when no safe single action can be inferred.

Secondary CTA rules:

- show at most two secondary actions in the hero
- prefer `Open Runtime`, `Open Editor`, `View Evidence`, `Prepare Source Workspace`, `Build Missing`, or `Cook Missing`
- never place `Sync Source Tiers`, `Clean Workspace`, or full dependency repair as a hero primary action unless it is the only honest next step

### Direction A Home Card Contract

Home cards:

| Card | Purpose | Primary Sources | Default Action |
| --- | --- | --- | --- |
| `Launch` | Shows editor/runtime availability and launch provenance | package manifests, bundled component manifest, artifact discovery | `Open Editor` or `Open Runtime` |
| `Prepare` | Shows host/workspace readiness without dumping full tool lists | host prerequisite checks, source dependency tier state, workspace file state | `Prepare Source Workspace` or `Generate Workspace Files` |
| `Content` | Shows cooked domain readiness | cooked root discovery, shader/texture/scene readiness | `Cook Missing` |
| `Package` | Shows whether review package assembly/inspection is available | `dist/releases/<version>`, release assembly manifests | `Assemble Review Package` or `Open Dist Folder` |
| `Evidence` | Shows concrete engineering proof links | docs, generated manifests, validation reports, source map | `View Evidence` |
| `Activity` | Shows only relevant current or recent issues | action history, latest logs, dismissed-run state | `Resolve`, `Dismiss`, or `View Log` |

Card density rules:

- each card should show one status chip, one sentence, and one action by default
- details move to card expansion, workflow page, diagnostics, or logs
- raw paths never appear on Home unless a path problem is the blocker
- cards must not become a target/output matrix; Direction C density belongs in Details or Advanced

### Engineering Evidence Link Contract

Every Home evidence link must resolve to a concrete source. If the source does not exist yet, the link should explain the producing workflow rather than pretending it is available.

| Evidence Link | User-Facing Purpose | Owner | Expected Source |
| --- | --- | --- | --- |
| `Architecture` | Explain product boundaries, artifact layout, launcher workflows, and engine structure | docs owner | architecture or roadmap docs under `docs/` |
| `Dependency Tiers` | Explain host prerequisites versus syncable capability tiers | dependency/system owner | `docs/dependency-capability-tiers.md` and launcher dependency tier metadata |
| `Release Manifests` | Show package contents, build metadata, dependencies, checksums, and bundled components | release assembly owner | `dist/releases/<version>/.../manifests/` generated by `CMake/SparkleReleaseAssembly.cmake` |
| `Validation Report` | Show final build/package/smoke results when available | validation owner | final validation report under `docs/plans/` or generated package report |
| `Source Map` | Show where launcher, editor, runtime, cook tools, projects, artifacts, and packages live | docs/build owner | repository navigation docs plus artifact contract |
| `Diagnostics` | Provide copyable logs, tool versions, selected options, package/source mode, and current blockers | launcher owner | launcher state, action history, latest operation logs, manifests |

Evidence status labels:

| Label | Meaning |
| --- | --- |
| `Available` | Link opens an existing file, folder, manifest, report, or launcher view |
| `Generated` | Link opens generated output from the current workspace or package |
| `Pending` | Expected output is missing and the card names the workflow that creates it |
| `Not configured` | Feature is intentionally disabled by the selected dependency/capability configuration |

### Stale Failure And Activity Contract

Home must not make the whole launcher feel broken because an unrelated old action failed.

Activity display rules:

- active current workflow failure: show inline recovery on that workflow and a compact Home Activity card
- old unrelated failure: show a quiet Activity badge/card only
- dismissed old failure: hide from primary Home/workflow attention, keep raw logs and history
- clean failure caused by live launcher artifacts: show `Close launcher and retry clean` as a recovery insight
- package/build/cook failure: show the failed workflow, one likely next action, and `View Log`

Activity card copy examples:

```text
Activity
Clean Workspace failed because launcher artifacts were locked.
[Close Launcher And Retry] [Dismiss] [View Log]
```

```text
Activity
No blocking activity for the selected workflow.
[View History]
```

### First-Contact Timing Review

| Journey Point | Contract Check |
| --- | --- |
| 30 seconds | Home title, project, package/source mode, hero state, and primary CTA are visible without scrolling |
| 2 minutes | User can launch editor/runtime if available, or sees the exact smallest blocker |
| 5 minutes | User can open Architecture, Dependency Tiers, Release Manifests, Validation Report, Source Map, or Diagnostics |
| 15 minutes | User can continue from Quick Start into Prepare, Build Missing, Cook Missing, Package, or Validate without switching roles |

### Phase 0 Validation Result

- Home / Quick Start contract maps to Direction A's hero, Launch/Prepare/Content/Package/Evidence/Activity cards, and one next action.
- Every proposed evidence link has an owner and expected source.
- The contract does not introduce `Reviewer Mode`, `Developer Mode`, or interview-specific app copy.
- Rebuild, recook, dependency sync, and clean remain secondary unless they are the smallest honest next action.
- Final build, launch, package, and visual validation were not run in this phase.

What earns attention:

- project/package state
- primary launch CTA
- one recommended next action
- critical blocker from the current funnel
- quick links that prove engineering depth without interrupting first launch

What does not earn primary attention:

- full tool list
- every cached dependency
- raw paths
- stale unrelated logs

## Workflow Page Concept

Example: Build Editor

```text
Build Editor                                             Needs project files refresh
Rebuild the Showcase editor locally. This replaces package/editor artifacts only.

Recommended next action
Generated project files are stale.
[Generate Project Files]    Secondary: Clean Build Files

Readiness
OK Host tools ready
! Project files stale
OK Core source tier cached
OK Editor target discovered

Build options
Configuration: DevelopmentEditor
Project: Showcase

Details >
```

Important changes from current UI:

- Host tool details collapse by default after the dependency set is ready.
- Dependency tiers show as chips or compact cards, not full rows unless expanded.
- `Build files` stale state becomes the main recommendation instead of one row among many.
- Full paths are hidden behind Details unless a path is the actual problem.

## Launch Page Concept

Example: Open Runtime

```text
Open Runtime                                             Blocked: missing runtime
Launch the packaged or locally built runtime for Showcase.

Recommended next action
Runtime executable is missing.
[Build Runtime]    Secondary: Open package docs

Readiness
OK Project directory ready
! Runtime executable missing
! Cooked scene assets missing
! Cooked textures missing
! Cooked shaders missing

Runtime options >
Graphics backend: D3D12
VSync: On
GPU preference: High performance

Details >
```

If package components exist, the status should say:

```text
Ready from package
This launch uses packaged runtime components. Local rebuild is optional.
```

If source artifacts exist:

```text
Ready from local build
This launch uses artifacts/dev/projects/Showcase/runtime/DevelopmentGame.
```

## Prepare Page Concept

Current Setup has too many sibling actions: Verify Host Environment, Sync Source Dependencies, Generate Project Files, Open Workspace.

Proposed Prepare Source Workspace combines them into a guided sequence:

```text
Prepare Source Workspace                                  Needs project files refresh
Make this checkout ready for local rebuilds.

1. Host tools                      Ready
2. Source tiers                    Core ready, Content cached, Shader cached, KTX disabled
3. Project files                   Needs refresh
4. IDE workspace                   Available after generation

[Generate Project Files]

Advanced actions
[Verify Host Tools] [Sync Source Tiers] [Open IDE]
```

This keeps expert escape hatches while making the common path obvious.

## Cook Page Concept

Cook should be asset-domain-first.

```text
Cook Content                                              Missing outputs
Prepare generated runtime content for Showcase.

Asset domains
* Shaders       Missing   [Cook Shaders]       Uses Shader Compiler Source Tier
* Textures      Missing   [Cook Textures]      Uses Content Pipeline Source Tier
* Scene Assets  Missing   [Cook Scene Assets]  Uses Content Pipeline Source Tier

Primary action
[Cook Missing]   Secondary: Cook All

Advanced shader options >
```

Current issue:

- Cook Shaders exposes many advanced controls immediately.

Recommended:

- Default view should show package, backend, and target preset only.
- Cache/debug/stats controls should be under `Advanced shader cook`.

## Maintain Page Concept

Maintenance should feel safe. The current Clean Workspace page shows many checkboxes and a failed log, which makes it feel dangerous and broken.

Proposed:

```text
Clean Workspace                                           Safe scopes selected
Remove generated files. Source files and installed tools are never deleted.

Recommended clean
OK Project cooked outputs
OK Build outputs
OK Logs

Danger zone >
Source dependency cache
Generated workspace reset

Blocked cleanup from last run
Launcher is currently using artifacts/dev/launcher-state/Live.
[Close Launcher And Retry] [Open log] [Dismiss]
```

Important:

- A failed clean caused by the launcher locking its live artifact should become an actionable maintenance insight, not a global failure shown on every page.
- Dangerous scopes need progressive disclosure and stronger confirmation copy.

## Activity And Logs

Current bottom log panel problems:

- It consumes a large permanent portion of the viewport.
- It repeats stale errors on unrelated pages.
- It uses raw terminal text as primary UI.
- Runs list and log panel compete with the main workflow.

Proposed:

- Activity dock is collapsed by default unless a run is active or the selected workflow has a relevant recent failure.
- A global toast/insight should summarize failures in human language.
- Raw logs live in an expandable drawer.
- Users can dismiss old failures from the main view without deleting logs.

Activity states:

| State | UI Treatment |
| --- | --- |
| Running | bottom drawer expands with progress and live log |
| Failed current workflow | inline recovery card plus log drawer |
| Failed unrelated workflow | compact Activity badge only |
| Completed | short success toast, log available in Activity |

## Naming Refinement

Recommended renames:

| Current | Proposed | Reason |
| --- | --- | --- |
| Start | Launch | clearer primary outcome |
| Setup | Prepare | less installer-like, more workflow-oriented |
| Sync Source Dependencies | Sync Source Tiers | matches capability-tier language |
| Generate Project Files | Generate Workspace Files | covers CMake/IDE state more clearly |
| Open Workspace | Open IDE | direct user language |
| Build All | Build Missing or Build All | `Build All` should not be default if only one target is missing |
| Build Cook Tools | Build Cooking Tools | more natural noun phrase |
| Cook Scene Assets | Cook Scenes And Meshes | clearer domain |
| Run | Validate | separates normal launch from tests |
| Run Project | Run Custom | clearer that this is configurable launch |
| Package Release | Assemble Review Package | avoids claiming publish-ready release |
| Maintenance | Maintain | shorter and active |
| Clean Workspace | Clean Generated Files | emphasizes safety |

Names to avoid:

- internal helper names
- phase numbers in user-facing app copy
- CMake target names in primary labels
- raw build root names unless in details
- "required" for optional rebuild dependencies when the user is using package artifacts

## Visual Direction

The current dark industrial direction is appropriate, but it needs stronger hierarchy and more atmosphere.

Recommended art direction:

- NVIDIA-adjacent but not NVIDIA-branded: graphite, tungsten, warm black, electric blue accent, restrained green for ready states, amber for stale/missing.
- Use cards and summary strips instead of full-width row tables everywhere.
- Keep the technical precision, but make it scannable.
- Larger type for primary state and CTAs.
- Use status chips consistently: Ready, Ready from package, Ready from local build, Missing, Stale, Disabled, Optional.
- Reserve bright blue for primary action only.
- Use green for success, amber for attention, red only for destructive/failure states.
- Paths should be muted monospace in detail drawers, not paragraph text in primary rows.

## What To Hide By Default

Hide unless expanded:

- full host prerequisite list when all required tools are ready
- full source dependency cache paths
- individual third-party dependency rows when a tier is fully cached
- command lines
- raw logs
- debug shader options
- destructive clean scopes
- package manifest file lists

Always show:

- current project
- whether launch is possible
- one recommended next action
- why the primary action is blocked
- what action changes: host machine, workspace, build outputs, cooked outputs, package outputs, or diagnostics

## Required Technical Capabilities

This UX should not fake confidence. If the launcher cannot answer a question needed by the intended experience, the correct response is to add the missing launcher or engine capability, not to hide the gap behind vague copy.

Principle:

- every primary UI claim must be backed by executable state, a manifest, a discovered artifact, or a real workflow
- if a state cannot be detected yet, the implementation plan must add detection before the UI presents that state as truth
- first-contact polish and daily production reliability must share the same underlying data model

Required capabilities:

| Capability | Why UX Needs It | Implementation Surface |
| --- | --- | --- |
| Package/source mode detection | Home must know whether it is running from a ready package or a source checkout | launcher root discovery, package manifests, repository markers |
| Bundled runtime component discovery | Launch-first flow depends on packaged editor/runtime/cooked assets before local rebuild | release manifests, bundled component manifests, artifact lookup |
| Local artifact discovery | Daily development must distinguish package outputs from local rebuild outputs | artifact contract, project target metadata |
| Project readiness model | Home and Launch pages need precise `ready`, `missing`, `stale`, and `disabled` states | launcher backend readiness evaluators |
| Exact next-action mapping | Missing runtime, stale project files, or missing cooked assets must point to one real action | workflow metadata and dependency graph |
| Engineering evidence index | First-time users need one-click access to architecture, manifests, validation, and source layout | docs manifest, package manifest, launcher evidence model |
| Dependency capability tiers | Setup must show capability unlocks without duplicating host prerequisites | shared dependency tier definitions |
| Build Missing workflow | UI needs a minimal rebuild path instead of pushing `Build All` | target ownership metadata, selected project targets |
| Cook Missing workflow | UI needs asset-domain recovery instead of generic cooking | cooked domain readiness, cook target mapping |
| Activity relevance and dismissal | Old unrelated failures must not dominate every page | scoped run history, dismissed-run state, current workflow matching |
| Diagnostics bundle | Users need copyable proof/debug context | collected logs, manifests, tool versions, paths, selected options |
| Safe clean awareness | Clean should detect launcher-held locks and explain recovery | process/lock-aware clean operations |
| User profile and defaults | First-run and daily production can have different default attention order without becoming separate product roles | launcher settings, recent workflow/project state |
| Command preview and provenance | Advanced users need trust in what each workflow executes | command construction model, manifest/log recording |
| Non-machine-specific path discovery | External evaluators must not depend on one developer's local paths | Qt/toolchain discovery, CMake presets, environment overrides, relative roots |

UX blocking rule:

- if a proposed screen needs one of these capabilities and the capability does not exist, implement the capability first or mark the screen as intentionally unavailable
- do not add cosmetic labels such as `Ready`, `Supported`, or `Package Mode` unless the launcher can prove them
- do not rely on hardcoded user paths, stale build folders, or developer-machine side effects to make the UX look ready

## Potential Missing Product Features

- Command Center landing screen.
- Quick Start as the default Home state for first launch and package-root usage.
- Time-boxed first-contact path: launch in 2 minutes, evidence in 5 minutes, rebuild path in 15 minutes.
- Engineering evidence links from Home: architecture, manifests, validation, dependency tiers, source map.
- Dismissible activity failures.
- `Build Missing` action that builds only missing editor/runtime/tool outputs.
- `Cook Missing` action that cooks only missing asset domains.
- `Prepare Source Workspace` guided sequence.
- Package/root mode indicator: `Package Mode` vs `Source Checkout Mode`.
- Source/package artifact provenance: `Ready from package` vs `Ready from local build`.
- Compact capability matrix for Source Tiers.
- Details drawer with copyable diagnostics bundle.
- "Close launcher and retry clean" affordance when launcher locks live artifacts.
- `Open artifacts folder`, `Open dist folder`, and `Open latest log` secondary actions.
- A global settings surface for profile, IDE, toolchain, Qt kit, and advanced defaults.

## Proposed Implementation Phases

Chosen direction:

- Direction A: Command Center is the target UX architecture.
- Direction B may contribute only guided copy patterns: "what this unlocks", simple next-step language, and after-action suggestions.
- Direction C may contribute only advanced/details patterns: compact target matrices, command previews, diagnostics, and provenance.
- Phases should not drift toward a pure wizard, a raw production console, or a portfolio-specific demo shell.

Shared rules for all implementation phases:

- keep each phase reviewable on its own, but do not build or launch between phases
- use static validation during phases: source inspection, naming grep, metadata checks, and document review
- do not claim a UX state is working until the final validation pass proves it in the running launcher
- do not add UI copy that promises readiness unless the backend can prove that state
- do not hide missing backend capability behind cosmetic labels; add the capability or mark the UX as blocked
- preserve daily production workflows while improving the launch-first hierarchy
- converge every phase toward Direction A: Command Center

Final build, launch, and visual validation happen only after Phase 5 is complete.

### Phase 0: First-Contact Quick Start Contract

Goal:

- make the launcher explicitly support first-contact usage before changing widget layout

Work:

- define Home copy, primary CTA rules, and engineering evidence links
- decide which docs/manifests/logs should be one click from Home
- define package/source mode labels and launch-readiness language
- define how stale unrelated failures are hidden, summarized, or dismissed during first contact

Positive guardrails:

- define the first-contact path using real launcher states: package/source mode, launch readiness, evidence availability, and next action
- keep production workflows visible as secondary paths
- decide which evidence links must resolve to concrete files, folders, manifests, reports, or launcher views
- define this contract as the top hero and evidence area of Direction A, not a separate wizard or role mode

Negative guardrails:

- do not add code in this phase
- do not invent marketing claims that are not backed by real artifacts
- do not make rebuild, recook, or dependency sync the first decision for a packaged user
- do not introduce role-specific UI such as `Reviewer Mode` or `Developer Mode`
- do not turn Quick Start into Direction B's full step-by-step runway

Validation:

- review the Home / Quick Start contract against the 30-second, 2-minute, 5-minute, and 15-minute first-contact journey
- confirm every proposed evidence link has an owner and expected source
- confirm the contract maps to Direction A's hero, cards, evidence, and Activity areas
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 0 of docs/plans/launcher-principal-ux-concept.md. Add a first-contact Quick Start contract to the launcher UX design without changing code yet. Define the Home screen copy, package/source mode states, primary CTA rules, engineering evidence links, and stale-failure behavior for a first-time user or external evaluator with limited time.

Positive guardrails: use real package/source, launch-readiness, evidence, and next-action states; keep daily production workflows visible as a natural continuation of Quick Start; assign every evidence link to a concrete file, folder, manifest, report, or launcher view; define the contract as Direction A's hero and evidence area, not a separate wizard or role mode.

Negative guardrails: do not implement code; do not add marketing claims that are not backed by artifacts; do not make rebuild, recook, or dependency sync the first decision for a packaged user; do not introduce role-specific UI such as `Reviewer Mode` or `Developer Mode`; do not turn Quick Start into Direction B's full step-by-step runway.

Validation: review the contract against the 30-second, 2-minute, 5-minute, and 15-minute first-contact journey; confirm every evidence link has an owner and source; confirm the contract maps to Direction A's hero, cards, evidence, and Activity areas; confirm no build, launch, or package validation was run.
```

### Phase 1: UX Contract And Copy

Goal:

- make the current UI less cluttered without replacing layout yet

Work:

- rename groups/actions according to the naming table
- remove phase-number language from primary UI copy
- convert old failed runs into dismissible Activity entries
- collapse full host tool/dependency details when ready
- move raw paths to detail text or copyable diagnostics

Positive guardrails:

- make labels describe user outcomes rather than implementation functions
- keep operation behavior unchanged while improving copy, grouping, and hierarchy
- use existing readiness and dependency metadata wherever possible
- align group/action naming with Direction A: Home / Quick Start, Launch, Prepare, Build, Cook, Validate, Package, Maintain

Negative guardrails:

- do not remove expert workflows
- do not duplicate host prerequisites inside source dependency sync copy
- do not expose raw local paths in primary UI unless the path itself is the blocker
- do not build or launch in this phase
- do not over-script the UI with Direction B step numbers

Validation:

- inspect user-facing strings for naming consistency with Launch, Prepare, Build, Cook, Validate, Package, and Maintain
- grep for legacy labels that should no longer appear in primary UI copy
- confirm labels match Direction A's command-center tone rather than wizard or raw-console tone
- confirm operation commands and workflow behavior were not intentionally changed
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 1 of docs/plans/launcher-principal-ux-concept.md. Keep the current Qt widget architecture, but improve naming, copy, and information hierarchy. Rename workflow groups/actions to Launch, Prepare, Build, Cook, Validate, Package, and Maintain where appropriate. Remove phase-number language from primary UI copy. Collapse or demote ready host/dependency details so each workflow elevates one recommended next action and one blocker. Keep raw paths available in details/logs, not primary rows.

Positive guardrails: make labels describe user outcomes; keep operation behavior unchanged; use existing readiness and dependency metadata where possible; align group/action naming with Direction A.

Negative guardrails: do not remove expert workflows; do not duplicate host prerequisites inside source dependency sync copy; do not expose raw local paths in primary UI unless the path itself is the blocker; do not over-script the UI with Direction B step numbers; do not build or launch in this phase.

Validation: inspect user-facing strings for naming consistency; grep for legacy primary labels; confirm labels match Direction A's command-center tone rather than wizard or raw-console tone; confirm operation commands and workflow behavior were not intentionally changed; confirm no build, launch, or package validation was run.
```

### Phase 2: Command Center

Goal:

- add a Home screen that owns the first-run funnel

Work:

- add Home workflow group
- create project readiness hero
- show Launch, Source, Content, and Package capability cards
- show one Next Best Action
- move global stale failures into Activity insight cards

Positive guardrails:

- make Home / Quick Start the default first-contact surface
- show launch status, source status, content status, package status, and evidence links as summarized cards
- drive the primary CTA from real readiness and next-action metadata
- implement the Direction A card grid: Launch, Prepare/Source, Content, Package, Evidence, Activity

Negative guardrails:

- do not make Home a dashboard of every raw prerequisite
- do not hide blockers behind vague "not ready" copy
- do not make Home depend on machine-specific paths
- do not build or launch in this phase
- do not drift into Direction C density on the default Home view

Validation:

- inspect the Home data model and verify each displayed status has a real source
- confirm the primary CTA rules cover package-ready, source-ready, missing-runtime, missing-cooked-content, and stale-workspace cases
- confirm Home contains Direction A's hero plus compact cards and no raw target/output matrix by default
- confirm stale unrelated failures are represented as Activity insights, not page-dominating errors
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 2 of docs/plans/launcher-principal-ux-concept.md. Add a Home / Quick Start command center workflow screen that summarizes selected project readiness, package/source mode, launch availability, source workspace state, content readiness, package assembly readiness, and engineering evidence links. Show one primary next action and one secondary recovery action. Keep existing workflow pages intact.

Positive guardrails: make Home the default first-contact surface; summarize launch, source, content, package, evidence, and Activity state as Direction A cards; drive CTAs from real readiness and next-action metadata.

Negative guardrails: do not turn Home into a raw prerequisite dashboard; do not hide blockers behind vague "not ready" copy; do not depend on machine-specific paths; do not drift into Direction C density on the default Home view; do not build or launch in this phase.

Validation: inspect the Home data model and verify each displayed status has a real source; confirm primary CTA rules cover package-ready, source-ready, missing-runtime, missing-cooked-content, and stale-workspace cases; confirm Home contains Direction A's hero plus compact cards and no raw target/output matrix by default; confirm stale unrelated failures are Activity insights only; confirm no build, launch, or package validation was run.
```

### Phase 3: Workflow Page Redesign

Goal:

- replace audit-table pages with guided workflow pages

Work:

- introduce common page sections: header, recommended action, readiness summary, options, details drawer
- update Build, Cook, Launch, Prepare, Package, and Maintain pages
- collapse advanced shader options
- make dependency tiers compact cards

Positive guardrails:

- use one page hierarchy everywhere: header, recommended action, readiness summary, options, details
- make details available without making them the default attention field
- preserve current operations, settings, and advanced controls behind better hierarchy
- use Direction A workflow pages as the default page model, with Direction B's "what this unlocks" copy only where it clarifies an action

Negative guardrails:

- do not delete diagnostics or command visibility
- do not bury the actual blocker behind decorative cards
- do not introduce separate readiness logic per page when a shared model is possible
- do not build or launch in this phase
- do not expose Direction C matrices in primary pages unless placed in Details or Advanced

Validation:

- inspect each redesigned page for exactly one dominant recommended action
- confirm raw paths, command lines, full dependency rows, and logs are in details or diagnostics surfaces
- confirm Build, Cook, Launch, Prepare, Package, and Maintain pages use the same section model
- confirm Direction A hierarchy is visible on each primary workflow page
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 3 of docs/plans/launcher-principal-ux-concept.md. Refactor workflow pages around header, recommended next action, compact readiness summary, workflow options, and expandable details. Build, Cook, Launch, Prepare, Package, and Maintain pages should show only high-signal blockers by default. Full host tools, dependency rows, raw paths, command lines, and logs should move into details. Preserve all current operations and settings.

Positive guardrails: use one Direction A page hierarchy everywhere; keep details available without making them primary; preserve advanced controls behind better hierarchy; use Direction B's "what this unlocks" copy only where it clarifies an action.

Negative guardrails: do not delete diagnostics or command visibility; do not bury the actual blocker behind decorative cards; do not introduce separate readiness logic per page when a shared model is possible; do not expose Direction C matrices in primary pages unless placed in Details or Advanced; do not build or launch in this phase.

Validation: inspect each redesigned page for exactly one dominant recommended action; confirm raw paths, command lines, dependency rows, and logs are secondary; confirm all major workflow pages use the same Direction A section model; confirm no build, launch, or package validation was run.
```

### Phase 4: Activity And Recovery Model

Goal:

- make logs useful without polluting every workflow

Work:

- collapse bottom log panel by default
- expand when active run starts
- show stale unrelated failures as Activity badges only
- add recovery cards for current workflow failures
- add dismiss action for old run summaries

Positive guardrails:

- make active and current-workflow failures highly visible
- make unrelated old failures available but quiet
- keep raw logs preserved and accessible
- provide recovery actions that map to real workflow fixes
- present Activity as Direction A's compact Home card plus scoped workflow recovery, not as a permanent terminal panel

Negative guardrails:

- do not delete logs when dismissing a failure from the main view
- do not show stale unrelated failure text on every workflow page
- do not convert every failure into a generic toast without recovery context
- do not build or launch in this phase
- do not let Activity recreate the current bottom-panel clutter

Validation:

- inspect run-history matching so failures are scoped to relevant workflows
- confirm dismissing a run changes UI attention, not stored log files
- confirm current failed workflows show recovery cards and raw log access
- confirm Home Activity follows Direction A's compact card pattern
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 4 of docs/plans/launcher-principal-ux-concept.md. Redesign Activity and logs so old unrelated failures do not dominate every workflow. Collapse raw logs by default, expand for running/current failed operations, add current-workflow recovery cards, and make stale failures dismissible from the main view while preserving log files.

Positive guardrails: make active and current-workflow failures visible; keep unrelated old failures quiet but accessible; preserve raw logs; map recovery actions to real workflow fixes; present Activity as Direction A's compact Home card plus scoped workflow recovery.

Negative guardrails: do not delete logs when dismissing UI failures; do not show stale unrelated failure text on every page; do not replace recovery context with generic toasts; do not let Activity recreate the current bottom-panel clutter; do not build or launch in this phase.

Validation: inspect workflow-to-run matching; confirm dismissal affects UI attention but not log storage; confirm current failures show recovery cards and raw log access; confirm Home Activity follows Direction A's compact card pattern; confirm no build, launch, or package validation was run.
```

### Phase 5: Product Polish

Goal:

- make the launcher feel like a serious engine product

Work:

- refine spacing, typography, status chips, cards, and CTA hierarchy
- add package/source mode indicator
- add copyable diagnostics bundle action
- add folder shortcuts for artifacts, dist, logs
- prepare final desktop and smaller-window validation scenarios

Positive guardrails:

- strengthen hierarchy without changing the accepted workflow model
- keep visual styling precise, restrained, and consistent with a serious engine tool
- make primary CTAs unmistakable and secondary actions calm
- keep folder shortcuts and diagnostics grounded in real paths from the artifact/package contract
- polish Direction A's command-center cards, hero, status chips, and details surfaces as the final target

Negative guardrails:

- do not introduce visual novelty that reduces clarity
- do not add new buttons just because helper functions exist
- do not use color alone to communicate state
- do not build or launch in this phase
- do not make the app look like a portfolio slideshow or a raw enterprise dashboard

Validation:

- inspect style and layout changes against the accepted hierarchy
- confirm status chips use consistent labels and severity
- confirm diagnostics/folder shortcuts point to declared roots only
- confirm the final static layout matches Direction A with selected Direction B/C support patterns only
- prepare the final validation checklist for the build-and-launch pass
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform Phase 5 of docs/plans/launcher-principal-ux-concept.md. Apply visual polish to the accepted UX model: stronger typography hierarchy, command-center cards, consistent status chips, restrained accent color usage, package/source mode indicator, copyable diagnostics bundle, and useful folder shortcuts. Prepare desktop and smaller-window validation scenarios for the final pass.

Positive guardrails: strengthen hierarchy without changing the accepted workflow model; polish Direction A's command-center cards, hero, status chips, and details surfaces; keep styling precise and restrained; make primary CTAs unmistakable and secondary actions calm; keep shortcuts and diagnostics grounded in declared artifact/package roots.

Negative guardrails: do not introduce visual novelty that reduces clarity; do not add new buttons just because helper functions exist; do not use color alone to communicate state; do not make the app look like a portfolio slideshow or a raw enterprise dashboard; do not build or launch in this phase.

Validation: inspect style/layout changes against the accepted hierarchy; confirm status chips use consistent labels and severity; confirm diagnostics/folder shortcuts point to declared roots only; confirm the final static layout matches Direction A with selected Direction B/C support patterns only; prepare the final validation checklist; confirm no build, launch, or package validation was run.
```

### Final UX Validation Pass: Build, Launch, And Review

Goal:

- validate the completed UX implementation only after all implementation phases are done
- confirm the implemented launcher matches Direction A: Command Center

Work:

- build SparkleLauncher once from a clean or intentionally selected build tree
- launch SparkleLauncher
- capture first-contact Home / Quick Start behavior
- review Launch, Prepare, Build, Cook, Validate, Package, and Maintain workflows
- verify desktop and smaller-window layouts
- verify launch-first first-contact path and daily production path both remain usable

Positive guardrails:

- record exact commands, build paths, artifact paths, package/source mode, and launcher version/state
- validate against real package/source/artifact/evidence states instead of screenshots alone
- capture failures as implementation fixes, not as excuses to weaken the UX contract

Negative guardrails:

- do not start this pass until Phases 0-5 are complete
- do not quietly skip blocked acceptance criteria
- do not treat a successful build as proof of UX acceptance without visual and workflow review

Validation:

- a first-time user can understand identity, mode, launch status, primary CTA, and evidence links from Home
- daily production users can still prepare, build, cook, validate, package, and maintain without fighting the Quick Start hierarchy
- Home matches Direction A: hero, launch/source/content/package/evidence/Activity cards, and one next action
- Direction B/C elements appear only in approved places: guidance copy, after-action suggestions, details, diagnostics, command previews, and advanced matrices
- stale unrelated failures do not dominate unrelated pages
- raw logs, commands, paths, and dependency inventories remain available in secondary surfaces
- final report lists pass/fail for every acceptance criterion

Prompt:

```text
Please perform the Final UX Validation Pass from docs/plans/launcher-principal-ux-concept.md only after Phases 0-5 are complete. Build SparkleLauncher once, launch it, and validate that the implemented launcher matches Direction A: Command Center. Validate the launch-first Home / Quick Start path plus daily production workflows. Review Launch, Prepare, Build, Cook, Validate, Package, and Maintain at desktop and smaller-window sizes. Validate package/source mode, launch readiness, engineering evidence links, next-action mapping, Activity behavior, diagnostics/details access, and status-chip consistency.

Positive guardrails: record exact commands, build paths, artifact paths, package/source mode, and launcher state; validate against real package/source/artifact/evidence states; convert failures into concrete implementation fixes.

Negative guardrails: do not run before Phases 0-5 are complete; do not skip blocked acceptance criteria; do not treat build success alone as UX acceptance.

Validation: produce a final pass/fail report for every acceptance criterion and list remaining fixes.
```

## Acceptance Criteria

- A first-time user or external evaluator can understand the project identity, package/source mode, and launch status within 30 seconds.
- A first-time user can launch the strongest available editor/runtime path within 2 minutes when packaged components exist.
- A first-time user can reach concrete engineering evidence within 2 clicks from Home.
- A first-time user can find the optional source rebuild path without being forced through it first.
- The app never exposes artificial role choices such as `Reviewer Mode` or `Developer Mode`.
- Every primary page has exactly one dominant recommended action.
- Ready workflows do not show full prerequisite inventories by default.
- Missing workflows explain the smallest next action, not a generic dependency checklist.
- Old unrelated failures do not occupy the main attention field.
- Source dependencies are described as capability tiers.
- Package artifacts and local source artifacts are clearly distinguished.
- Advanced controls remain available but do not dominate first-run workflows.
- The app feels like an engine launcher and command center, not a raw CMake dashboard.
- The implemented UX matches Direction A: Command Center as the primary architecture.

## Example App View Directions

These are selectable product directions, not implementation promises yet. Each direction uses the same real workflows and backend readiness model. The difference is the default hierarchy, density, and emotional feel.

Selection criteria:

- first contact: can a new user understand what to do in 30 seconds?
- production depth: can a daily developer keep working without friction?
- engineering confidence: does the launcher show proof without feeling like a portfolio slideshow?
- scalability: can this layout grow as the engine adds projects, tools, packages, and diagnostics?

### Direction A: Command Center

Summary:

- recommended default direction
- strongest balance between first-contact clarity and daily production utility
- feels like an engine operations surface, not a setup wizard

Home:

```text
Sparkle Engine                                      Source Checkout
Project: Showcase        Config: Development        IDE: Visual Studio

+------------------------------------------------------------------------------+
| Ready to explore                                                             |
| Open the strongest available Showcase target. Rebuild and recook are optional.|
|                                                                              |
| [Open Editor] [Open Runtime]                         Details: source artifacts|
+------------------------------------------------------------------------------+

+----------------------+ +----------------------+ +----------------------+
| Launch               | | Prepare              | | Content              |
| Editor: Ready        | | Host tools: Ready    | | Shaders: Missing    |
| Runtime: Missing     | | Source tiers: Ready  | | Textures: Missing   |
| Next: Build Runtime  | | Files: Stale         | | Next: Cook Missing  |
+----------------------+ +----------------------+ +----------------------+

+----------------------+ +----------------------+ +----------------------+
| Package              | | Evidence             | | Activity             |
| Review package: N/A  | | Manifests: Available | | Last clean: Failed   |
| Next: Assemble       | | Validation: Latest   | | [Resolve] [Dismiss] |
+----------------------+ +----------------------+ +----------------------+
```

Launch:

```text
Open Editor                                                 Ready from local build
Launch the Showcase editor. Package components are used first when available.

Recommended action
[Open Editor]                              Secondary: Open Runtime

Readiness
Ready   Editor executable
Ready   Project directory
Missing Cooked scene assets       [Cook Scene Assets]
Missing Cooked textures           [Cook Textures]
Missing Cooked shaders            [Cook Shaders]

Options
Graphics backend: D3D12        VSync: On        GPU: High performance

Details >
```

Prepare:

```text
Prepare Source Workspace                                  Project files stale
Make this checkout ready for local rebuilds and IDE work.

Recommended action
[Generate Workspace Files]

Readiness
Ready   Host tools
Ready   Core source tier
Cached  Content pipeline tier
Cached  Shader compiler tier
Stale   Workspace files

Advanced actions
[Verify Host Tools] [Sync Source Tiers] [Open IDE]

Details >
```

Why choose this:

- gives first-time users a clear launch path
- gives developers compact production status without a wizard feeling
- scales well to more projects and package states
- keeps evidence visible but secondary

Risk:

- if cards get too numerous, Home could become a dashboard again
- needs disciplined rules for what earns a Home card

### Direction B: Guided Runway

Summary:

- more guided and beginner-friendly
- best if the launcher should actively coach users through preparation
- slightly less "pro operations console" than Direction A

Home:

```text
Sparkle Engine
Start with the Showcase, then go deeper when you are ready.

1. Explore
+------------------------------------------------------------------------------+
| Open the Showcase editor or runtime using available package/local artifacts.  |
| [Open Editor] [Open Runtime]                                                  |
+------------------------------------------------------------------------------+

2. Prepare source workspace
+------------------------------------------------------------------------------+
| Project files are stale. Generate workspace files before local rebuilds.      |
| [Generate Workspace Files]                                                    |
+------------------------------------------------------------------------------+

3. Build and cook only what is missing
+---------------------------+ +---------------------------+
| Build Missing             | | Cook Missing              |
| Runtime executable missing| | 3 cooked domains missing  |
+---------------------------+ +---------------------------+

4. Review engineering evidence
[Architecture] [Manifests] [Validation] [Dependency Tiers]
```

Workflow page:

```text
Build Missing                                            Step 3 of production path
Build only the missing local artifacts for Showcase.

What this unlocks
Runtime launch from local build.

Blockers
Stale workspace files
[Generate Workspace Files]

After this
[Open Runtime] [Cook Missing]

Details >
```

Why choose this:

- very easy for first-time users
- makes the funnel almost impossible to misunderstand
- excellent for onboarding and package-first exploration

Risk:

- can feel less like a power tool for daily production
- step numbers may become awkward when users jump around
- needs careful language so it does not feel like an installer

### Direction C: Production Console

Summary:

- densest and most technical direction
- best for daily developers who want maximum state visibility
- weakest first-contact experience unless carefully softened

Home:

```text
Sparkle Engine Console                         Showcase / Development / VS

+-----------------+------------------+------------------+------------------+
| Launch          | Source           | Content          | Package          |
| Editor Ready    | Host Ready       | Shaders Missing  | Dist Missing     |
| Runtime Missing | Tiers Cached     | Textures Missing | Manifests N/A    |
| [Open Editor]   | Files Stale      | Scenes Missing   | [Assemble]       |
+-----------------+------------------+------------------+------------------+

Next action: Generate Workspace Files
[Run] [Details] [Copy Diagnostics]

Recent runs
Clean Workspace failed: launcher artifacts are locked. [Resolve] [Dismiss]
```

Build:

```text
Build Matrix

Target                  State       Output Root                         Action
Launcher                Ready       artifacts/dev/launcher              [Build]
ShowcaseEditor          Missing     artifacts/dev/projects/Showcase     [Build]
ShowcaseRuntime         Missing     artifacts/dev/projects/Showcase     [Build]
CookTools               Ready       artifacts/dev/tools                 [Build]

Details >
```

Why choose this:

- strong daily production control
- easy to compare targets and outputs
- feels technical and serious

Risk:

- most likely to become cluttered
- first-contact users may see a wall of state instead of a launch path
- raw paths and target names can dominate if not aggressively managed

### Recommended Direction

Choose Direction A: Command Center as the base.

Use selected ideas from Direction B:

- first-contact copy that explains the next step plainly
- "what this unlocks" text on workflow pages
- small after-action suggestions

Use selected ideas from Direction C:

- compact target/output matrix only inside details or advanced production views
- copyable diagnostics and command preview
- strong artifact/package provenance

Do not choose:

- a separate reviewer/demo mode
- a pure wizard that hides the real engine workflows
- a raw production console as the default first impression
