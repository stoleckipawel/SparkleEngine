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

- assemble a release package
- verify what is included
- produce manifests and checksums
- keep publish sign-off separate

Ideal path:

1. Package screen shows release readiness summary.
2. It lists missing package inputs, not a vague planned status.
3. Primary CTA is `Assemble Release Package`.
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
| Prepare | What does this machine/workspace need? | Prepare Source Workspace, Verify Host Tools, Sync Source Tiers, Generate Workspace Files, Open IDE |
| Build | What do I want to rebuild locally? | Build Missing, Build Editor, Build Runtime, Build Launcher, Build Cooking Tools, Build All |
| Cook | What generated content do I need? | Cook Missing, Cook All, Cook Shaders, Cook Textures, Cook Scenes And Meshes |
| Validate | How do I test or diagnose? | Run Smoke Test, Run Custom |
| Package | How do I assemble a release package? | Assemble Release Package, Open Dist Folder |
| Maintain | What generated state should I clean? | Clean Generated Files, Format Code |

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
1. Generate Workspace Files
   CMake cache differs from selected generator/platform/toolset/Qt kit.
   [Generate]

Engineering evidence
[Architecture] [Dependency Tiers] [Manifests] [Latest Validation]

Recent activity
Clean Generated Files failed because launcher artifacts were locked.
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

## Archived Early Prototype Concepts

The earlier Direction A card-grid handoff, old workflow-page examples, and old activity/log concept were moved to:

- docs/plans/launcher-principal-ux-implemented-prototype-archive.md

Keep the main concept focused on the current reference stack:

- NVIDIA App for product shell and visual language
- Visual Studio Installer for source-tier/component/commit-flow complexity
- Rider for dense expert settings, Activity, logs, diagnostics, and compact action menus
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
| Package Release | Assemble Release Package | avoids claiming publish-ready publication |
| Maintenance | Maintain | shorter and active |
| Clean Workspace | Clean Generated Files | emphasizes safety |

Names to avoid:

- internal helper names
- phase numbers in user-facing app copy
- CMake target names in primary labels
- raw build root names unless in details
- "required" for optional rebuild dependencies when the user is using package artifacts

## Visual Direction

The previous Sparkle Launcher execution is not visually accepted. It moved the information architecture toward a command center, but it did not capture the calm, high-confidence product language we want. The new target is an NVIDIA App-inspired engine launcher: restrained, sparse, dark, precise, and operational, with strong hierarchy and very little decorative chrome.

The goal is not to clone NVIDIA branding or make Sparkle pretend to be an NVIDIA product. The goal is to learn from the NVIDIA App's product discipline: simple shell, bright active accent, generous negative space, crisp tabs, clear page titles, compact cards, and settings/detail pages that feel quiet until the user asks for depth.

Recommended art direction:

- NVIDIA App-inspired, not NVIDIA-branded: near-black canvas, charcoal header/content bands, graphite cards, thin dividers, white text, muted gray secondary text, and a single electric lime accent.
- Replace the old blue CTA language with lime-green primary actions and active states.
- Use cards only when they have strong product purpose: hero media, launch target, package/evidence tile, stat tile, or capability tile.
- Use list rows and section dividers for settings, prerequisites, dependency tiers, and diagnostics instead of boxing every line as a card.
- Keep technical precision, but make it scannable through page sections, tabs, concise summaries, and expandable detail.
- Use larger type for page titles, hero headline, and primary CTA copy; keep dense operational metadata smaller and quieter.
- Use status labels consistently: Ready, Ready from package, Ready from local build, Missing, Stale, Disabled, Optional, Pending.
- Use lime green for active navigation, ready/available states, primary action buttons, and selected tabs.
- Use amber for stale/missing action-needed states, red only for destructive/failure states, and gray for optional/disabled.
- Paths should be muted monospace in detail drawers, diagnostics, or copyable output, not paragraph text in primary rows.
- The launcher should feel like a serious control center with product-grade restraint, not a raw CMake dashboard and not a portfolio slideshow.

## NVIDIA App Reference Analysis And Sparkle Translation

This section analyzes the supplied NVIDIA App screenshots and translates them into Sparkle Launcher rules. The reference is the visual and interaction language, not the brand identity.

### What NVIDIA App Does Well

Shell:

- very narrow icon rail on the far left
- active navigation uses a bright lime vertical bar and a quiet selected background
- page title lives in a broad top header band
- top-right utility actions are small icons, not large labeled buttons
- the shell does not have a persistent bottom terminal or footer fighting for attention

Page structure:

- each page has one clear title: `Home`, `Drivers`, `System`, `Settings`
- many pages use horizontal tabs directly under the title: `Monitors`, `Video`, `Performance`, `My Rig`, `Advanced`
- page content starts after a thin divider; the divider gives structure without boxing everything
- content width is intentionally bounded; it does not stretch every row across the entire monitor just because space exists
- large empty space is allowed when the page does not need more information

Home / discovery:

- Home uses a large visual hero with strong editorial hierarchy
- the hero has one headline, one short body, one lime primary CTA, and carousel indicators
- secondary product tiles use image-heavy cards, not status-table cards
- cards are large enough to feel touchable and deliberate
- empty library state is one calm card, not a warning wall

Drivers:

- driver state is summarized first, then a large visual details panel
- release notes are split into readable cards: `What's New`, `What's Fixed`
- status/action text like reinstall or refresh is small and positioned consistently
- the page feels product/editorial, not a build log

System:

- `Performance` uses compact stat cards for high-signal values only
- `My Rig` uses plain information sections instead of cards for every item
- `Advanced` uses settings rows with toggles and dropdowns, not giant forms
- the monitoring page has one large diagram area and one expandable properties panel

Settings:

- settings are grouped by section with thin dividers
- toggles sit on the right edge of the row
- dropdowns are modest, dark, and rectangular
- links use a small external-link icon plus text
- legal/privacy/info pages remain calm and text-first

Visual language:

- background: almost black, not blue-black
- surface: dark charcoal, slightly lighter than background
- cards: modest elevation through shade and spacing, not heavy borders
- active accent: lime green, used sparingly but confidently
- typography: white headings, pale gray body, dim gray metadata
- layout density: sparse by default; detail appears only inside focused pages, tabs, drawers, or expandable sections

### Sparkle Shell Translation

Sparkle should adopt the NVIDIA-style shell structure:

```text
Top chrome
NVIDIA-style product strip: Sparkle mark + Sparkle Launcher
Window controls remain native.

Left rail
Home
Launch
Prepare
Build
Cook
Validate
Package
System
Settings

Main header band
Page title on left
Small utilities on right: Share/Copy diagnostics, Activity, Account/Root mode or Settings

Content
Tabs when a page has sub-domains
Focused page body with bounded content width
No persistent bottom terminal on default pages
```

Sparkle navigation mapping:

| NVIDIA App Pattern | Sparkle Equivalent | Rule |
| --- | --- | --- |
| Home | Home | Product hero, strongest launch path, library/project tiles, evidence/discovery cards |
| Drivers | Package / Updates | Package assembly, bundled runtime status, release notes, manifests, checksums |
| Graphics | Launch / Runtime Settings | graphics backend, VSync, GPU preference, project launch settings |
| System | System | host tools, dependency tiers, artifact roots, hardware/toolchain summary |
| Settings | Settings | launcher preferences, toolchain defaults, Qt kit, privacy/logging, diagnostics |
| Redeem / Discover cards | Evidence / Tools | engineering evidence, docs, optional tools, package reports |

Recommended Sparkle left rail:

- `Home`
- `Launch`
- `Prepare`
- `Build`
- `Cook`
- `Validate`
- `Package`
- `System`
- `Settings`

Notes:

- `Maintain` should move under `Settings` or `System > Maintenance` unless it is actively needed. Cleaning generated files is a utility, not a primary product pillar.
- `Activity` should be a top-right icon or drawer, not a permanent rail item and not a bottom panel.
- `Diagnostics` should be a top-right utility or a `System` tab, not a footer button competing with project context.

### Sparkle Home Translation

Home should be closer to NVIDIA App Home than to the current Sparkle card grid.

Target Home:

```text
Sparkle Engine

+--------------------------------------------------------------------------------+
| HERO                                                                           |
| Realtime renderer showcase                                                     |
| Open the Showcase editor from package or local artifacts.                      |
| [OPEN EDITOR]                                                                  |
| Secondary links: Open Runtime | View Architecture | Package Manifest           |
|                                                        visual/preview area      |
+--------------------------------------------------------------------------------+

Library
+------------------------------+ +------------------------------+
| Showcase Editor              | | Showcase Runtime             |
| Ready from package/local      | | Missing local output          |
| [OPEN] or [BUILD]             | | [BUILD RUNTIME]               |
+------------------------------+ +------------------------------+

Discover / Evidence
+------------------+ +------------------+ +------------------+ +------------------+
| Architecture     | | Dependency Tiers | | Validation       | | Release Package  |
| Open docs        | | Source capability| | Latest report    | | Assemble / Open  |
+------------------+ +------------------+ +------------------+ +------------------+
```

Home rules:

- one hero, not six equally weighted cards at the top
- hero primary CTA is launch-first whenever possible
- if launch is blocked, hero CTA is the smallest real repair action
- project/editor/runtime targets become `Library` cards, inspired by NVIDIA App's game/app library
- architecture, validation, packages, and optional tools become `Discover` cards, inspired by NVIDIA App's discover/product tiles
- source preparation status should be present but secondary, not the emotional center of first contact
- stale workspace should not turn the whole hero amber if a package launch is available
- when source checkout has no runnable package/local output, the hero can say `Prepare source workspace` but should remain calm and product-like

### Sparkle Workflow Page Translation

Use NVIDIA-style tabs and sections rather than old audit tables.

Prepare page:

```text
Prepare

[Overview] [Host Tools] [Source Tiers] [Workspace Files] [Advanced]

Overview
Source checkout needs one refresh before local builds.
[GENERATE WORKSPACE FILES]

Host tools
Visual Studio        Ready
CMake                Ready
Qt kit               Ready

Source tiers
Core                 Cached
Content Pipeline     Optional / Disabled / Cached
Shader Compiler      Optional / Disabled / Cached
```

System page:

```text
System

[Overview] [Toolchain] [Artifacts] [Dependencies] [Diagnostics]

Statistics
Build profile        Development
IDE                  Visual Studio
Qt kit               6.11.1 MSVC
Source tiers         3 cached
Package mode         Source checkout

My workspace
Repository root      ...
Artifacts root       ...
Dist root            ...
```

Settings page:

```text
Settings

[Launcher] [Toolchain] [Privacy / Logs] [About]

Launcher
Default project                                     [Showcase v]
Default launch target                               [Editor v]
Auto-open Activity when a run fails                 [toggle]

Toolchain
Preferred IDE                                       [Visual Studio v]
Qt kit                                              [Auto detect v]
Use clang-cl when available                         [toggle]
```

Package page:

```text
Package

[Current] [Manifests] [Release Notes] [Symbols] [Advanced]

Current package
Sparkle Showcase Runtime Package
Version, commit, toolchain, Qt kit
[ASSEMBLE PACKAGE] [OPEN DIST]

What's included
Launcher
Showcase Editor
Showcase Runtime
Cooked Showcase Content

What's missing
Only show blockers that prevent a package review.
```

Launch page:

```text
Launch

[Editor] [Runtime] [Graphics] [Arguments] [Advanced]

Showcase Editor
Ready from package/local artifact.
[OPEN EDITOR]

Runtime settings
Graphics backend      D3D12
VSync                 On
GPU preference        High performance
```

### Visual Token Contract

Use these as implementation targets:

| Token | Target |
| --- | --- |
| App background | `#121212` to `#151515` |
| Header band | `#252525` to `#2a2a2a` |
| Rail background | `#181818` |
| Card/surface | `#202020` to `#252525` |
| Divider | `#303030` |
| Primary text | `#f2f2f2` |
| Secondary text | `#c9c9c9` |
| Muted text | `#8f8f8f` |
| NVIDIA-like accent | `#76b900` or nearby Sparkle lime |
| Accent hover | brighter lime, not blue |
| Warning | amber, used sparingly |
| Error/destructive | red only for failure/destructive confirmation |

Typography:

- page title: 18 to 22 pt, bold, white
- tab labels: 10 to 11 pt, semibold
- section titles: 11 to 13 pt, bold
- body: 9 to 10 pt
- metadata: 8 to 9 pt, muted
- primary CTA: uppercase or semibold label, lime background, black text if contrast is better

Spacing:

- left rail width: narrow icon rail or icon+short-label rail; avoid a wide workflow rail
- page left inset: roughly 24 px from content rail
- content max width: do not stretch text rows across ultrawide screens
- section spacing: generous vertical gaps with thin dividers
- card grid: large cards for product/evidence tiles, not tiny status boxes

Interaction:

- active nav: lime vertical bar plus selected background
- active tab: lime underline
- buttons: primary lime, secondary transparent/text or dark gray
- toggles: gray off, lime on
- activity/logs: drawer or icon-triggered panel; never always-on terminal
- details: expandable sections or tabs, not full visible inventories

### What To Avoid From The Current Sparkle Execution

- equal-weight status card grid as the first visual impression
- blue primary action color
- footer controls competing with page content
- permanent bottom log panel
- heavy borders on every row
- page-wide rows that stretch to ultrawide monitors
- showing old failed operations on unrelated pages
- turning every workflow into a card dashboard
- making Home look like a build status monitor instead of a product launcher

### NVIDIA-Inspired Acceptance Criteria

- Home visually reads as a product launcher within 5 seconds: title, hero, primary CTA, project/library cards, evidence/discovery tiles.
- The app uses lime accent for navigation, tabs, primary CTAs, and ready states; blue is removed from the primary visual language.
- The left rail is narrow and icon-led, with selected state matching the NVIDIA App pattern.
- Page tabs are used for sub-domains instead of secondary vertical operation menus where possible.
- The default surface does not show a bottom terminal/log panel.
- Settings, System, Prepare, and Package pages use section rows, dividers, toggles, dropdowns, and tabs in the NVIDIA App style.
- Cards are reserved for hero/product/evidence/stat surfaces; technical inventories use rows, sections, or details.
- Content width is intentionally bounded so wide screens do not create sparse, awkward status strips.
- The visual design feels product-grade and restrained, not like a restyled CMake dashboard.

### Visual Studio Installer-Inspired Acceptance Criteria

- Source dependency tiers are shown as capability/workload selections with summaries, unlocks, selected state, and optional/disabled state.
- Individual dependencies/components are reachable through a detailed searchable tab, not shown by default on first-contact or overview pages.
- Consequential workflows show what will change before running: selected tiers, build outputs, cooked domains, package contents, clean scopes, target roots, and relevant paths.
- Package assembly has a contents/details model that separates included, optional, missing, and output-location information.
- Location/path configuration lives in dedicated `Locations` or detail surfaces, not scattered across primary cards.
- Primary actions are consistently placed and clearly named, with secondary `More`/details paths for advanced choices.
- Simple launch/evidence flows remain simple and are not burdened with installer-style commit bars.

### Rider-Inspired Acceptance Criteria

- Dense expert surfaces remain compact and readable without becoming cramped.
- Settings and advanced pages support search, left category navigation, breadcrumbs, and clear section grouping.
- Tool/activity panels can be docked, collapsed, or opened contextually rather than permanently occupying primary attention.
- Context menus and `More` actions are compact, keyboard-friendly, and predictable.
- Status/progress information lives in a calm bottom/status area only when useful; it does not become a persistent log terminal.
- Dark theme colors use subtle contrast, not heavy borders, and selected states are clear without visual noise.
- Expert workflows preserve speed: power users can reach Build, Cook, Validate, logs, and settings quickly without stepping through a beginner wizard.

## Visual Studio Installer Reference Analysis And Sparkle Translation

The Visual Studio Installer is not the primary visual reference. Its light color palette and Microsoft installer branding are not a fit for Sparkle. The useful reference is its flow model: complexity is available exactly where it is needed, while the default path remains obvious. It is especially relevant for Sparkle's source tiers, host prerequisites, optional components, package/dependency selection, and setup/build/cook consequences.

### What Visual Studio Installer Does Well

High-level install management:

- the top-level `Installed` / `Available` tabs make the user's current mode obvious
- each installed product is represented by one large product row/card with version, update state, and clear actions
- primary actions such as `Modify`, `Launch`, `Install`, and `More` are predictable and consistently placed on the right
- developer news/help is separated in a side column; it does not interrupt the install action flow
- update state is summarized in a calm banner instead of a noisy warning wall

Workload selection:

- complexity is grouped into visible categories such as Web/Cloud, Desktop/Mobile, Gaming, etc.
- workload cards are large enough to read quickly and include icon, title, summary, and checkbox
- selected workload cards get a strong left accent and checked state
- cards represent capability bundles, not individual packages
- the user can understand "what to install" before seeing the deep component list

Individual components:

- individual packages are available in a separate tab with search
- the list is dense because the user intentionally entered a detailed mode
- checkboxes are aligned and predictable
- categories break a long list into navigable chunks
- search is prominent and keyboard-friendly

Installation details:

- a persistent right panel shows exactly what the current selection includes
- included versus optional components are separated
- hierarchy is expandable/collapsible
- the selected work is summarized before the user commits
- the user can see consequences without leaving the selection page

Install locations and commitment:

- install/cache/shared paths live in a dedicated tab, not scattered across the primary workflow
- the bottom commitment bar shows location, license/notice text, required space, install behavior, and final action
- destructive or consequential choices are explicit and located near the final commit button
- users can review before applying changes

### Sparkle Translation From Visual Studio Installer

Sparkle should borrow the Installer's complexity model for any workflow that has selectable components, dependency groups, package contents, or generated-output consequences.

Sparkle equivalents:

| Visual Studio Installer Pattern | Sparkle Equivalent | Rule |
| --- | --- | --- |
| Installed / Available | Package / Source or Ready / Available Products | Separate what exists now from what can be prepared or built |
| Product row with Modify/Launch/More | Project/product row with Open/Prepare/More | Put predictable actions on the right side of each product row/card |
| Workloads tab | Source Tiers / Capability Bundles | Show optional dependency groups as capability cards |
| Individual Components tab | Advanced Dependencies / Package Files | Dense lists belong behind search/detail tabs |
| Installation Details side panel | Selection Details / Workflow Impact panel | Keep a persistent summary of what the chosen action will change |
| Installation Locations tab | Artifact Roots / Package Roots / Cache Locations | Paths belong in a dedicated tab or details surface |
| Bottom install bar | Commit bar for build/cook/package/clean | Show required outputs, target root, consequence, and final action before running |

### Source Tier And Dependency UX

Source dependency management should feel more like Visual Studio workloads than a package manager log.

Target model:

```text
Prepare

[Overview] [Source Tiers] [Individual Dependencies] [Locations] [Advanced]

Source Tiers
Select capability bundles to make more workflows available.

+--------------------------------------+ +--------------------------------------+
| Core Workspace Source Tier        [x]| | Content Pipeline Source Tier      [ ]|
| Required for launcher, editor, build | | Unlocks mesh import and texture cook |
| 3 dependencies cached                | | 6 dependencies available             |
+--------------------------------------+ +--------------------------------------+

+--------------------------------------+ +--------------------------------------+
| Shader Compiler Source Tier       [ ]| | KTX Container Source Tier         [ ]|
| Unlocks offline shader cooking       | | Extends texture container workflows  |
| 1 dependency available               | | Disabled by configuration            |
+--------------------------------------+ +--------------------------------------+

Selection Details
Included
  Core Workspace Source Tier
Optional
  Content Pipeline Source Tier
  Shader Compiler Source Tier

[SYNC SELECTED TIERS]
```

Rules:

- source tiers are capability bundles first, dependency lists second
- each tier card says what workflows it unlocks
- dense individual dependency lists require an explicit `Individual Dependencies` tab and search
- host prerequisites never appear as syncable source tiers
- optional disabled tiers say which setting disables them
- selection details show what will change before running sync

### Build, Cook, Package, And Clean Commit Bar

For consequential workflows, Sparkle should borrow the Installer's commitment model. The user should always know what will be changed before pressing the primary action.

Commit bar model:

```text
Target
artifacts/dev/projects/Showcase/editor/DevelopmentEditor

Impact
Builds missing editor output. Does not sync dependencies, recook assets, or modify package releases.

Estimated/generated size
Known size or "computed after build"

[BUILD EDITOR] [More options v]
```

Use the commit bar for:

- Build Editor / Build Runtime / Build Missing / Build All
- Cook Missing / Cook Shaders / Cook Textures / Cook Scenes And Meshes
- Assemble Release Package
- Clean Generated Files
- Sync Source Tiers

Do not use the commit bar for:

- Home hero launch
- simple Open Editor/Open Runtime when already ready
- passive diagnostics or evidence links

### Package Assembly UX

Package assembly should combine NVIDIA's release/details feel with Visual Studio Installer's details/commit clarity.

Package page target:

```text
Package

[Current] [Contents] [Manifests] [Locations] [Advanced]

Current
Sparkle Showcase Runtime Package
Version: pending / selected
Status: ready to assemble / missing cooked content / missing launcher artifact

Contents
+ Launcher
+ Showcase Editor
+ Showcase Runtime
+ Cooked Showcase Content
+ Manifests and checksums
+ Licenses

Selection Details
Included outputs
  artifacts/dev/launcher/...
  artifacts/dev/projects/Showcase/...
  artifacts/dev/projects/Showcase/cooked/...
Written to
  dist/releases/<version>/...

[ASSEMBLE PACKAGE]
```

Rules:

- package contents should be previewable before assembly
- included versus missing inputs must be separated
- package roots and artifact roots are visible in `Locations`, not in every row
- final package action says `Assemble`, not `Publish`, unless publishing/sign-off exists

### Workflow Simplicity Rules From Visual Studio Installer

- simple pages stay simple; complex pages expose tabs
- one product/workflow row should have predictable right-side actions
- optional depth should be discoverable through tabs, search, details, and "More"
- a selected set of changes should always have a visible summary
- location/path decisions belong in dedicated locations tabs
- advanced individual dependency/component lists must be searchable
- a workflow should show what will happen before a destructive or long-running action starts
- secondary news/help/evidence areas should not interrupt the main action path

### Combined Reference Strategy

NVIDIA App remains the primary reference for:

- visual language
- shell
- left rail
- dark surfaces
- lime accent
- Home hero
- product/evidence cards
- tabbed system/settings pages

Visual Studio Installer becomes the secondary reference for:

- workload/source-tier selection
- optional component complexity
- install/build/cook/package impact summaries
- right-side or drawer-based selection details
- locations/path management
- final commit bar for consequential actions
- predictable primary/secondary action placement

The combined Sparkle goal:

- NVIDIA App outside: confident product shell and visual language
- Visual Studio Installer inside complex workflows: clear selection, detail, consequence, and commit flow

## Rider Reference Analysis And Sparkle Translation

Rider is a valuable third reference for expert-tool clarity. It should not replace the NVIDIA App as the main product-shell reference and should not turn Sparkle Launcher into an IDE. The useful lessons are compact dark styling, discoverable expert controls, contextual tool windows, searchable settings, and high information density without panic.

### What Rider Does Well

Shell and expert navigation:

- narrow tool-window rails can expose many expert areas without making all of them visually dominant
- panels can be docked, collapsed, or opened only when relevant
- the main canvas stays calm even while side and bottom tools carry dense technical information
- menus are compact and action-oriented, with keyboard shortcuts visible for power users
- project trees and issue/navigation panels support deep structure without replacing the primary workspace

Settings:

- settings use a left category tree, search field, breadcrumbs, and a clear detail pane
- dense options are grouped into named sections with horizontal dividers
- checkboxes, dropdowns, disabled states, and explanatory text are compact but legible
- Save/Cancel remain obvious and stable at the bottom
- settings density is acceptable because the user intentionally entered the settings surface

Activity and diagnostics:

- bottom tool windows are powerful but contextual; they are not the product's first impression
- logs and build output can be detailed and technical while remaining visually contained
- errors use color and icons sparingly, with enough text to diagnose the problem
- status/progress information is available in a calm status bar rather than through modal interruption

Dark styling:

- surfaces use subtle contrast rather than thick borders
- selected rows use a quiet colored background
- disabled and secondary text are readable but clearly lower priority
- dense UI controls remain small, aligned, and predictable
- the app trusts spacing, hierarchy, and typography more than decorative cards

### Sparkle Translation From Rider

Use Rider patterns for Sparkle's expert and daily-production depth:

| Rider Pattern | Sparkle Equivalent | Rule |
| --- | --- | --- |
| Tool window rails | Activity, Logs, Diagnostics, Project Structure | Make these collapsible/contextual, not permanent Home clutter |
| Settings category tree | Sparkle Settings | Use searchable categories for Launcher, Toolchain, Projects, Logs, Advanced |
| Breadcrumb path | Settings/System location | Show where the user is inside dense settings or system pages |
| Compact action menus | `More`, workflow actions, dependency actions | Keep expert actions discoverable without making them primary buttons |
| Bottom tool window | Build/cook/log output | Open when running or failed; collapse otherwise |
| Status bar | Progress/root/project/build context | Keep lightweight context available without stealing page attention |
| Project tree | Source Map / Project Structure | Use for source layout/evidence, not as the default Home |

### Sparkle Settings Model Inspired By Rider

Settings should lean more Rider than NVIDIA App because settings are inherently dense.

```text
Settings

[search settings]

Launcher
  Appearance
  Activity
  Diagnostics
Projects
  Default Project
  Launch Profiles
Toolchain
  Visual Studio
  Qt Kit
  CMake
  Clang
Artifacts
  Build Roots
  Package Roots
  Logs
Advanced
  Experimental
  Environment

Toolchain > Qt Kit
Qt kit discovery
  Auto-detect Qt kits                         [x]
  Preferred Qt root                           [C:/Qt/6.11.1/msvc2022_64]
  Require MSVC kit for default Windows path   [x]

Details
  Discovered kits...

[SAVE] [CANCEL]
```

Rules:

- settings can be dense because the user intentionally opened them
- use search and category tree to keep density manageable
- use breadcrumbs for deep pages
- keep Save/Cancel stable and visible
- keep advanced and experimental settings separated from everyday settings

### Sparkle Activity And Diagnostics Inspired By Rider

Activity should behave like a tool window, not a permanent bottom console.

```text
Activity drawer

Build Editor
Failed
Compiler error C2065 ...
[Open Log] [Copy Diagnostics] [Retry] [Dismiss]

Raw output
collapsed by default unless current operation is running or failed
```

Rules:

- open Activity automatically for active runs and current failures
- collapse Activity when the user returns to first-contact Home
- keep raw logs available but not always visible
- use compact rows with icons, state, timestamp, and one action
- show keyboard-friendly copy/open actions for expert users

### Sparkle Expert Menu Model Inspired By Rider

Use compact menus for secondary operations:

- `More` on product/library cards: Open Folder, Copy Path, View Manifest, Rebuild, Clean Output
- `More` on source tier cards: Clean Cache, Open Cache, View Dependencies, Configure Tier
- `More` on package page: Open Dist, View Checksums, Copy Package Path, Create Symbols Archive
- `More` on Activity rows: Copy Output, Open Log, Dismiss, Retry, Open Diagnostics

Rules:

- primary action remains visible
- expert actions live in predictable `More` menus
- menu labels should be verbs and should not expose internal helper names
- keyboard shortcuts can be shown where they exist, but do not invent shortcuts only for decoration

### Rider Styling Lessons For Sparkle

Apply these especially to System, Settings, Activity, Advanced, and Diagnostics:

- compact row height is acceptable when sections are clear
- subtle blue-gray selection can be used inside expert/settings surfaces, while lime remains the product accent
- avoid thick card borders in dense settings; use dividers and indentation
- use muted explanatory text under settings labels
- use clear disabled states
- use icons sparingly and consistently
- prefer alignment and grouping over visual ornament

### Combined Reference Strategy With Rider

NVIDIA App remains primary for:

- product shell
- Home visual language
- lime accent and dark surfaces
- hero/library/discover structure
- top-level page rhythm

Visual Studio Installer remains secondary for:

- source tier/workload selection
- optional component complexity
- selection details
- locations and commit bars
- package/build/cook consequence summaries

Rider becomes tertiary for:

- compact expert surfaces
- settings search/category/breadcrumb model
- dockable Activity/log/diagnostic tool windows
- action menus and keyboard-friendly expert workflows
- dense dark UI clarity

The combined Sparkle goal becomes:

- NVIDIA App for first impression and product confidence
- Visual Studio Installer for workflow consequence clarity
- Rider for expert depth, settings, diagnostics, and daily production compactness

### Combined Prepare Flow Sketch

```text
Prepare

[Overview] [Source Tiers] [Individual Dependencies] [Locations] [Advanced]

Overview
Source checkout is almost ready for local builds.
[GENERATE WORKSPACE FILES]

Source Tiers
+-------------------------------------+ +-------------------------------------+
| Core Workspace Source Tier       [x]| | Content Pipeline Source Tier     [ ]|
| Required for launcher/editor builds | | Unlocks import, texture, mesh cook   |
| Ready: 3 of 3                       | | Cached: 6 of 6                       |
+-------------------------------------+ +-------------------------------------+

+-------------------------------------+ +-------------------------------------+
| Shader Compiler Source Tier      [ ]| | KTX Container Source Tier        [ ]|
| Unlocks offline shader cook         | | Optional texture container support   |
| Cached: 1 of 1                      | | Disabled by current configuration    |
+-------------------------------------+ +-------------------------------------+

Selection Details
Included
  Core Workspace Source Tier
Optional
  Content Pipeline Source Tier
  Shader Compiler Source Tier
Disabled
  KTX Container Source Tier

Impact
Syncs selected source tiers into the workspace dependency cache.
Does not install Visual Studio, Qt, CMake, Git, or Windows SDK.

[SYNC SELECTED TIERS] [MORE OPTIONS]
```

### Combined Package Flow Sketch

```text
Package

[Current] [Contents] [Manifests] [Locations] [Advanced]

Current
Sparkle Showcase Runtime Package
Status: Missing cooked shaders
[COOK SHADERS] [ASSEMBLE WHEN READY]

Contents
+ Launcher                         Ready
+ Showcase Editor                  Ready from local artifact
+ Showcase Runtime                 Missing
+ Cooked Scenes And Meshes         Ready
+ Cooked Textures                  Ready
! Cooked Shaders                   Missing
+ Manifests / Checksums            Generated during assembly

Selection Details
Will read from
  artifacts/dev/launcher/...
  artifacts/dev/projects/Showcase/...
  artifacts/dev/projects/Showcase/cooked/...

Will write to
  dist/releases/<version>/SparkleShowcaseRuntime

Impact
Creates a runtime package. Does not publish or sign off a release.

[ASSEMBLE PACKAGE] [OPEN DIST]
```

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

- NVIDIA App is the primary reference for product shell, visual hierarchy, Home, tabs, cards, settings rows, dark palette, lime accent, and restraint.
- Visual Studio Installer is the secondary reference for capability tier selection, component details, location/configuration pages, and commit bars for consequential actions.
- Rider is the tertiary reference for expert-density surfaces: settings search, category trees, breadcrumbs, compact action menus, diagnostics, logs, and command visibility.
- Phases should not drift toward a pure wizard, a raw production console, or a portfolio-specific demo shell.
- The previous Sparkle visual implementation is not accepted as final; it is a functional prototype that must be redesigned against the NVIDIA-inspired visual contract above.

Shared rules for all implementation phases:

- keep each phase reviewable on its own, but do not build or launch between phases
- use static validation during phases: source inspection, naming grep, metadata checks, and document review
- do not claim a UX state is working until the final validation pass proves it in the running launcher
- do not add UI copy that promises readiness unless the backend can prove that state
- do not hide missing backend capability behind cosmetic labels; add the capability or mark the UX as blocked
- preserve daily production workflows while improving the launch-first hierarchy
- converge every phase toward Direction A information architecture with NVIDIA App-inspired visual execution

Final build, launch, and visual validation happen only after the Visual Redesign Reset and Visual Redesign Phases 1-3 are complete.

### Visual Redesign Reset: NVIDIA-Inspired Launcher Shell

Goal:

- replace the visually rejected Sparkle shell with an NVIDIA App-inspired product shell before another final validation
- keep the real workflow/backend model, but rebuild the presentation layer around the reference language

Work:

- remove the wide workflow rail as the dominant navigation object
- introduce a narrow icon-led rail with lime active indicator
- move root mode, diagnostics, activity, and settings into top/header utilities or dedicated pages
- remove footer controls from the default visual hierarchy
- convert Activity/log output into a drawer or page-level detail surface
- define shared page scaffolding: title band, tab row, bounded content body, section rows, cards only where appropriate

Positive guardrails:

- treat the screenshots as a layout/product-language reference, not as branding to copy
- keep Sparkle identity, icons, project names, and engineering evidence honest and original
- preserve existing workflows and operation IDs while changing how users navigate them
- use lime accent consistently for active nav, selected tabs, ready states, and primary CTAs
- verify every old blue primary-action or selected-state style is replaced or intentionally kept only in non-primary technical contexts

Negative guardrails:

- do not make a fake NVIDIA clone
- do not retain the current equal-weight six-card Home grid as the primary first impression
- do not keep a persistent bottom log/terminal panel
- do not keep footer project/config/IDE selectors as permanent competing chrome
- do not stretch rows/cards across ultrawide screens without a content-width rule
- do not build or launch in this reset phase

Validation:

- inspect launcher shell code and style rules for old blue active/primary states
- confirm the default shell can be described as narrow rail + title band + content tabs + bounded body
- confirm Activity/logs are not default persistent bottom chrome
- confirm no build, launch, or package validation was run

Prompt:

```text
Please perform the Visual Redesign Reset from docs/plans/launcher-principal-ux-concept.md. Replace the visually rejected launcher shell with an NVIDIA App-inspired product shell while preserving workflow behavior. Implement a narrow icon-led rail, lime active indicator, title/header band, top utility actions, tab-capable page scaffold, bounded content width, and remove footer/log chrome from default attention. Preserve operation IDs and backend workflows.

Positive guardrails: use the NVIDIA App screenshots as visual/product-language reference only; keep Sparkle identity original; use lime accent for active nav, selected tabs, ready states, and primary CTAs; keep Activity/logs accessible but not persistent bottom chrome.

Negative guardrails: do not clone NVIDIA branding; do not keep the current equal-weight Home card grid; do not keep footer controls as permanent primary chrome; do not stretch rows/cards across ultrawide screens; do not build or launch in this phase.

Validation: inspect shell/style implementation for the new structure and old blue-primary remnants; confirm Activity/logs moved out of default bottom chrome; confirm no build, launch, or package validation was run.
```

Reset implementation handoff:

- Replaced the dominant wide workflow/action rail with a narrow icon-led rail using text-under-icon navigation and a lime active indicator.
- Moved workflow operation choices into a tab row under a persistent title/header band.
- Moved root mode, folder shortcuts, Activity, diagnostics, project, build configuration, and IDE context into header utilities instead of a bottom footer.
- Converted the old persistent bottom output/log area into a right-side Activity drawer that opens for active or failed runs and remains hidden by default.
- Added an explicit bounded content body for workflow pages so rows and cards do not stretch across ultrawide displays.
- Replaced old blue primary/selected styling with the NVIDIA-inspired lime accent for primary CTAs, selected tabs, active rail state, focus, and ready/running emphasis.
- Preserved workflow definitions, operation IDs, backend requests, dependency/readiness logic, and operation execution behavior.
- Static validation: source grep found no old footer/blue-primary remnants in launcher shell code, and `git diff --check` passed with only normal CRLF warnings.
- No build, launch, package assembly, or final visual validation was run in this reset phase.

### Visual Redesign Phase 1: NVIDIA-Style Home

Goal:

- rebuild Home as a product launcher surface inspired by NVIDIA App Home

Work:

- replace the current six-card-first Home with hero + library + discover/evidence sections
- hero presents Sparkle identity, selected project, launch provenance, strongest CTA, and one secondary path
- library cards represent runnable products: Showcase Editor, Showcase Runtime, and future projects/tools
- discover/evidence cards represent Architecture, Dependency Tiers, Validation, Package, and optional tools
- source preparation appears as a secondary state unless it is the smallest blocker

Positive guardrails:

- make Home visually understandable within 5 seconds
- use a large hero with one headline, one short explanation, one lime CTA
- make package/local artifact provenance visible without making it the headline unless launch depends on it
- use product/evidence tiles with enough size and spacing to feel intentional

Negative guardrails:

- do not begin Home with six equal status cards
- do not turn stale workspace into the emotional headline if a package/local launch is available
- do not show raw dependency/tool/path inventories on Home
- do not make Home a reviewer-specific showcase page
- do not build or launch in this phase

Validation:

- static inspection confirms Home has hero, library, discover/evidence sections
- Home has one primary CTA and at most two secondary hero links
- no raw paths or full inventories appear in Home primary content
- no build, launch, or package validation was run

Prompt:

```text
Please perform Visual Redesign Phase 1 from docs/plans/launcher-principal-ux-concept.md. Rebuild Home as an NVIDIA App-inspired product launcher surface: hero, library/product cards, and discover/evidence tiles. Preserve the real readiness and next-action model. The hero should prefer launch-first when possible and show the smallest blocker otherwise.

Positive guardrails: one strong hero CTA; library cards for Showcase Editor/Runtime; evidence/discover tiles for architecture, dependency tiers, validation, package, and tools; package/local provenance visible but calm.

Negative guardrails: do not use the old six-equal-card grid; do not show raw paths or dependency inventories; do not make reviewer-specific UI; do not build or launch in this phase.

Validation: inspect Home structure and copy; confirm one primary CTA; confirm no build, launch, or package validation was run.
```

Phase 1 implementation handoff:

- Replaced the previous Home status grid with a product-led Home: identity/context bar, one launch-first hero, `Library`, and `Discover` sections.
- Hero now presents the strongest available action first: open editor, open runtime, or the smallest real repair action when launch is blocked.
- Library now contains runnable product tiles for `Showcase Editor` and `Showcase Runtime`, with calm package/local artifact provenance and direct open/build actions.
- Discover now contains evidence and production-extension tiles for `Architecture`, `Dependency Tiers`, `Validation`, `Package`, `Content`, and `Tools`.
- Removed the old Home evidence overflow helper so Home no longer starts from six equal status cards or a raw audit/dashboard model.
- Kept readiness, next-action selection, operation IDs, backend requests, and workflow execution behavior intact.
- Static validation target: Home has hero, library, and discover/evidence sections; one primary hero CTA; no raw paths, dependency inventories, or reviewer-specific copy in primary Home content.
- No build, launch, package assembly, or final visual validation was run in this phase.

### Visual Redesign Phase 2: Tabbed Workflow Pages, Installer Complexity, And Rider Expert Surfaces

Goal:

- make Prepare, Launch, Build, Cook, Validate, Package, System, and Settings follow the NVIDIA App page rhythm
- use Visual Studio Installer-style selection details and commit bars where workflows become component-heavy or consequential
- use Rider-style compact expert surfaces for Settings, Activity, logs, diagnostics, action menus, and advanced workflow details

Work:

- introduce horizontal tabs for workflow subdomains
- convert technical inventories into section rows with thin dividers
- reserve cards for stat tiles, product tiles, package highlights, and evidence tiles
- create a `System` page for host tools, toolchain, source tiers, artifacts, and diagnostics
- create or strengthen a `Settings` page for launcher defaults, IDE/toolchain/Qt kit, logs/privacy, and about info
- model source tiers as workload/capability cards with selected-state accents, optional checkboxes, and "what this unlocks" copy
- add selection detail or workflow impact summaries for Sync Source Tiers, Build, Cook, Package, and Clean workflows
- move individual dependency/package-file lists into searchable detail tabs
- move path/location decisions into `Locations` tabs or details surfaces
- add settings search/category/breadcrumb structure where settings become dense
- make Activity/logs behave like a contextual tool window or drawer, not permanent page chrome
- use compact `More` menus for secondary expert actions on cards, rows, dependency tiers, packages, and activity runs

Positive guardrails:

- use NVIDIA App-style title band and tab underline
- settings and system pages should be calm text/row surfaces, not dashboards
- use toggles/dropdowns/action links aligned to the right where appropriate
- preserve command previews and details in advanced tabs or drawers
- use Visual Studio Installer-style details for selected components, included/optional items, output roots, and final action consequences
- keep simple workflows simple; only introduce installer-style complexity where the user is selecting tiers, components, package contents, clean scopes, or output locations
- use Rider-style density only in intentional expert surfaces, never as the default first-contact Home
- keep settings, logs, and diagnostics fast for daily production users

Negative guardrails:

- do not turn every page into cards
- do not expose full host/dependency inventories by default
- do not keep secondary vertical operation menus when tabs are more appropriate
- do not hide real expert controls; move them to Advanced/Details
- do not make NVIDIA App Home look like Visual Studio Installer
- do not make Sparkle Launcher look like a full IDE
- do not use the Visual Studio Installer color palette as the Sparkle visual target
- do not add a commit bar to simple launch/evidence actions
- do not let Rider-style tool windows become permanent Home clutter
- do not build or launch in this phase

Validation:

- each major page has a title band and tab/section model
- settings/system rows use dividers and right-aligned controls
- raw logs, command lines, and full inventories are secondary
- source tiers look like capability/workload cards, not dependency logs
- consequential workflows show a clear impact/selection summary before the primary action
- Settings has search/category/breadcrumb structure when dense
- Activity/logs are contextual and collapsible
- expert `More` menus are compact and predictable
- no build, launch, or package validation was run

Prompt:

```text
Please perform Visual Redesign Phase 2 from docs/plans/launcher-principal-ux-concept.md. Convert workflow pages to NVIDIA App-inspired title bands, tabs, section rows, stat cards only where appropriate, and advanced/details surfaces. Add or strengthen System and Settings pages for host/toolchain/artifact/dependency/diagnostic state and launcher defaults. Use Visual Studio Installer as a secondary reference for source tier/workload cards, individual dependency search/detail tabs, selection details, locations tabs, and commit bars for consequential workflows. Use Rider as a tertiary reference for compact expert surfaces: settings search/category/breadcrumbs, contextual Activity/log tool windows, diagnostics, and predictable compact More menus.

Positive guardrails: use NVIDIA App for visual shell and page rhythm; use Visual Studio Installer for complexity management inside Prepare, Package, Build, Cook, and Clean; use Rider for Settings, Activity, logs, diagnostics, and expert menus; use rows/dividers for settings and inventories; keep expert controls in Advanced/Details; preserve workflow behavior.

Negative guardrails: do not use cards for every row; do not show full inventories by default; do not keep secondary vertical operation menus when tabs fit better; do not use Visual Studio Installer colors; do not make Sparkle look like a full IDE; do not let Rider-style tool windows become Home clutter; do not add commit bars to simple launch/evidence actions; do not build or launch in this phase.

Validation: inspect page structure and information hierarchy; confirm source tiers use workload/capability-card behavior; confirm consequential workflows show impact/selection summaries; confirm Settings has search/category/breadcrumb structure where dense; confirm Activity/logs are contextual and collapsible; confirm no build, launch, or package validation was run.
```

Phase 2 implementation handoff:

- Added shared NVIDIA-style page tabs below workflow headers so major pages have a title band plus section-tab rhythm.
- Added `System` as an inspection page for project/root mode, toolchain, workspace files, source tiers, artifact roots, dist roots, and diagnostics locations.
- Added `Settings` as a compact Rider-style preferences page with search placeholder, breadcrumb, launcher defaults, toolchain controls, and logs/diagnostics details.
- Replaced Sync Source Tiers' default dependency dump with Visual Studio Installer-inspired source tier workload cards that explain capability unlocks first.
- Kept individual dependency rows in secondary details so raw dependency inventories remain available without dominating the primary page.
- Added package assembly selection details for launcher, Showcase products, manifests, and symbols so package consequences are clear before the primary action.
- Added page-tab models for Prepare, Launch, Build, Cook, Validate, Package, Maintain, System, and Settings while preserving operation IDs and backend behavior.
- Kept Activity/log output as the existing contextual drawer; no permanent Home or workflow terminal was reintroduced.
- Static validation target: no full inventories by default, source tiers use workload cards, System/Settings use rows and compact controls, and package/clean-style consequential workflows expose selection/impact context.
- No build, launch, package assembly, or final visual validation was run in this phase.

### Visual Redesign Phase 3: NVIDIA-Style Polish And Final Visual Validation

Goal:

- make the app feel product-grade before final UX validation

Work:

- apply the visual token contract: near-black background, charcoal surfaces, lime accent, muted text, thin dividers
- replace old blue selected/primary states
- tune icon size, rail width, title spacing, tabs, card radii, row heights, dropdowns, toggles, and buttons
- tune expert surfaces against Rider clarity: settings tree/search/breadcrumbs, compact rows, contextual tool windows, readable logs, and predictable More menus
- validate wide and normal window sizes against the NVIDIA-inspired acceptance criteria
- update screenshots and final report honestly

Positive guardrails:

- polish should strengthen clarity, not add novelty
- the final app should look restrained and operational
- dense expert areas should feel Rider-clear: compact, aligned, searchable, and controllable
- screenshots must include Home, Launch, Prepare, Package, System, and Settings
- final report must distinguish functional pass from visual acceptance

Negative guardrails:

- do not claim visual acceptance based only on build success
- do not keep old accepted screenshots if the user rejects them
- do not overfit to one ultrawide screenshot
- do not let expert density bleed into first-contact Home
- do not hide blocked criteria

Validation:

- build and launch only after the reset and phases are complete
- capture 1280x760 and wide desktop screenshots
- pass/fail every NVIDIA-inspired, Visual Studio Installer-inspired, and Rider-inspired acceptance criterion
- list remaining fixes explicitly

Prompt:

```text
Please perform Visual Redesign Phase 3 from docs/plans/launcher-principal-ux-concept.md after the shell/Home/page redesign phases are complete. Apply final NVIDIA App-inspired polish, replace old blue primary/selected states, tune Rider-inspired expert surfaces, validate Home/Launch/Prepare/Package/System/Settings visually at 1280x760 and wide desktop sizes, build and launch once, capture screenshots, and produce a pass/fail visual acceptance report.

Positive guardrails: prioritize restraint, clarity, lime accent consistency, bounded content width, Rider-clear expert density, and honest acceptance reporting.

Negative guardrails: do not claim visual acceptance from build success alone; do not keep rejected screenshots as accepted evidence; do not overfit to ultrawide; do not let expert density bleed into first-contact Home; do not hide blocked criteria.

Validation: build, launch, capture screenshots, and report pass/fail for every NVIDIA-inspired, Visual Studio Installer-inspired, and Rider-inspired acceptance criterion.
```

## Archived Prototype Implementation

The earlier implemented Phase 0-5 prototype, handoff notes, build/launch validation, screenshot list, and visual rejection history were moved to:

- `docs/plans/launcher-principal-ux-implemented-prototype-archive.md`

That archive is historical context only. The current target direction is the NVIDIA App-inspired launcher shell, Visual Studio Installer-inspired complexity/commit model, and Rider-inspired expert/settings/activity model defined above.

Do not use the archived prototype screenshots as acceptance evidence for the next redesign pass.

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
- The implemented UX matches the NVIDIA App-inspired command-center architecture as the primary product direction.
- The visual execution matches the NVIDIA App-inspired contract: narrow rail, lime accent, title band, tabbed page rhythm, hero/library/discover Home, bounded content width, quiet settings/system rows, and no persistent bottom terminal.

## Archived Example Direction Sketches

The earlier Direction A/B/C sketches were moved to:

- `docs/plans/launcher-principal-ux-implemented-prototype-archive.md`

The active direction is no longer selected from those sketches. The current target is the combined NVIDIA App + Visual Studio Installer + Rider reference strategy above.
