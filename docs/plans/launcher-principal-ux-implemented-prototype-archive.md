# Sparkle Launcher Implemented Prototype Archive

Date: 2026-06-04

Status: historical implementation record. This document preserves the already-implemented launcher UX prototype phases, handoffs, and validation notes that were moved out of launcher-principal-ux-concept.md so the main concept can focus on the new NVIDIA App + Visual Studio Installer + Rider-inspired redesign.

Important reading note:

- This archive is not the current desired visual direction.
- The prototype improved workflow metadata, activity handling, readiness copy, and some command-center structure, but the final visual result was rejected.
- Use this file for implementation history and lessons learned only.
- Use docs/plans/launcher-principal-ux-concept.md for the current target direction.

## Archived Prototype Phases And Validation

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

Phase 1 validation result:

- Workflow groups now use Direction A naming in launcher UI and shell grouping: Launch, Prepare, Build, Cook, Validate, Package, Maintain.
- Primary action labels now use outcome names: Sync Source Tiers, Generate Workspace Files, Open IDE, Build Cooking Tools, Cook Scenes And Meshes, Assemble Review Package, Clean Generated Files.
- Operation IDs, planner kinds, process requests, command construction, and workflow behavior were intentionally left unchanged.
- Ready host tool inventory is demoted on build/cook pages; full installed-tool detail remains available through Verify Host Environment.
- Source dependency/cache rows no longer expose machine-specific absolute cache paths in primary details.
- Stored per-workflow run summaries can be dismissed from the main workflow attention area while preserving logs/history files.
- Phase-number language was removed from primary launcher package copy.
- Final build, launch, package, and visual validation were not run in this phase.

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

Phase 2 implementation handoff:

- Added `Home > Command Center` as the default first workflow surface.
- Home uses existing workspace, package, launch, dependency, and action-history planning data rather than a separate synthetic status model.
- The hero shows workspace/package mode, selected project state, one next best action, and one secondary path.
- The default card set now covers Launch, Source, Content, Package, Evidence, and Activity.
- Launch cards use editor/runtime launch plans and only offer direct launch actions when those plans are ready.
- Missing local executable, stale workspace, missing cooked content, and blocked package states route users to the existing Build, Prepare, Cook, or Package workflow pages instead of bypassing safety checks.
- Evidence actions open concrete docs or release folders when present; missing evidence remains explicit.
- Stored failed workflow history appears as a compact Activity insight instead of dominating first contact.
- Home is UI-only: the bottom Run/Clean controls are disabled for the Command Center, and existing workflow pages remain intact.
- Launcher source was checked for role-specific reviewer/developer/portfolio app copy; none was added.
- Static validation used `rg` and `git diff --check`; final build, launch, package, and visual validation were not run in this phase.

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

Phase 3 implementation handoff:

- Added a shared `Workflow Guide` section to non-Home workflow pages so each page starts with the recommended action and scoped impact before options or diagnostics.
- Standardized primary readiness sections under `Readiness Summary` across Prepare, Build, Cook, Launch, Validate, Package, and Maintain workflows.
- Added a reusable collapsed details drawer for secondary diagnostics and inventories.
- Moved full host tool inventories, raw tool paths, source tier contents, and dependency-entry rows into details surfaces.
- Collapsed shader cache/debug/compiler diagnostics under `Advanced Shader Options` while keeping common shader target choices visible.
- Reordered Launch and Validate pages so readiness appears before launch/runtime options.
- Reordered Maintain cleanup so confirmation/readiness appears before destructive scope choices.
- Existing operation IDs, backend requests, run behavior, settings, clean scopes, and advanced controls were preserved.
- Static validation used `rg` and `git diff --check`; final build, launch, package, and visual validation were not run in this phase.

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

Phase 4 implementation handoff:

- Added a raw-log toggle to the Activity/output panel so logs are collapsed by default but preserved.
- Raw logs now auto-expand when an operation starts and when the selected/current run is running or failed.
- Successful or unrelated old runs remain compact in Activity unless the user explicitly opens raw output.
- Added scoped current-workflow recovery rows to workflow pages when that exact workflow has a failed stored result.
- Recovery rows map to real follow-up workflows such as Verify Host Environment, Generate Workspace Files, Build Editor/Runtime, Build Cooking Tools, or the relevant Cook action.
- Updated per-workflow run history copy so failed results show recovery guidance while successful history stays calm.
- Dismiss removes stored UI attention for the selected workflow and refreshes the page; raw run logs remain available.
- Home Activity remains a compact summary card and does not become a permanent terminal panel.
- Static validation used `rg` and `git diff --check`; final build, launch, package, and visual validation were not run in this phase.

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

Phase 5 implementation handoff:

- Added an always-visible root-mode chip in the launcher footer so package roots, source checkouts, and fallback workspace roots are identifiable without adding role-specific UI.
- Added grounded footer shortcuts for declared generated roots: `artifacts/`, `dist/`, and `artifacts/dev/launcher-state/Logs`.
- Added a copyable diagnostics summary that records root mode, root path, selected project, build configuration, IDE, profiles, selected workflow, artifact roots, package root, log root, build tree, and active run state when present.
- Refined visual hierarchy around Direction A: stronger page titles, calmer secondary actions, clearer primary CTA weight, consistent neutral/ready/warning/error status chip styling, and restrained footer utility styling.
- Preserved the accepted workflow model and existing operations; Phase 5 did not add new workflow behavior just because helper functions existed.
- Static validation scope remains UI/code inspection only; final build, launch, package, and visual validation were not run in this phase.

Prepared final validation scenarios:

- Desktop command-center pass: open at a wide desktop size, verify Home first, then inspect Launch, Prepare, Build, Cook, Validate, Package, and Maintain for one dominant recommended action and compact readiness.
- Smaller-window pass: inspect the same pages at a constrained window size and verify footer context, root-mode chip, CTAs, Activity, and Details remain reachable without crowding primary content.
- Package-root pass: from a packaged root, verify the mode chip says `Package Root`, Home prefers launch-first bundled components, and diagnostics/folder shortcuts point to package/artifact/log roots.
- Source-checkout pass: from a source root, verify the mode chip says `Source Checkout`, rebuild/recook/sync paths are optional production extensions, and package outputs remain clearly distinct from local artifacts.
- Activity pass: verify old unrelated failures are compact Activity insights, current-workflow failures show recovery actions, and raw logs remain behind `Show raw log`.
- Diagnostics pass: use `Copy diagnostics`, paste the summary, and confirm every listed path is a declared root from the artifact/package contract rather than a hard-coded machine-specific assumption.

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

Final UX validation result - 2026-06-04:

Visual acceptance correction:

- The first validation pass proved the launcher built and opened, but the captured Home layout still looked like the legacy workflow/audit shell with new copy. That visual result was rejected because it did not match Direction A closely enough.
- Home was corrected into an actual Command Center surface: the redundant operation-title chrome is hidden, the legacy Home run/footer strip is removed, and Home now owns a hero plus Launch, Prepare, Content, Package, Evidence, and Activity cards.
- Evidence is now a visible card-level action instead of a tiny overflow-only affordance.
- Post-review status: this corrected Command Center is still not visually accepted. It remains a functional prototype and must be replaced by the NVIDIA App-inspired visual direction defined above.
- The prior information architecture remains useful, but the visual execution is rejected because it lacks the NVIDIA App's restraint, shell discipline, lime-accent identity, bounded content rhythm, tabbed page model, and hero/library/discover hierarchy.
- Corrected screenshot evidence:
  - `artifacts/diagnostics/launcher-ux-validation/command-center-home-corrected.png`
  - `artifacts/diagnostics/launcher-ux-validation/command-center-home-polished.png`
  - `artifacts/diagnostics/launcher-ux-validation/command-center-home-final-1280.png`
- This correction supersedes the earlier Home visual acceptance claim, but it is not the final visual target. Package-root first-run validation is still conditional until a real packaged runtime fixture exists.

Validation environment:

- Repository root: `C:/Users/stole/Documents/GitHub/SparkleEngine`
- Build tree: `build/ux-validation-msvc`
- Launcher artifact: `artifacts/dev/launcher/DevelopmentEditor/SparkleLauncher.exe`
- Runtime launcher process: `artifacts/dev/launcher-state/Live/SparkleLauncher.exe`
- Qt kit: `C:/Qt/6.11.1/msvc2022_64`
- Generator: `Visual Studio 18 2026`
- Platform: `x64`
- CMake: `4.3.3`
- Git: `2.54.0.windows.1`
- MSVC compiler: `19.51.36246.0`
- Windows SDK: `10.0.26100.0`
- Optional source tiers for this launcher validation: content pipeline `OFF`, shader compiler `OFF`, KTX `OFF`

Commands run:

```powershell
cmake -S . -B build\ux-validation-msvc -G "Visual Studio 18 2026" -A x64 -DSPARKLE_ENABLE_CONTENT_PIPELINE=OFF -DSPARKLE_ENABLE_SHADER_COMPILER=OFF -DSPARKLE_ENABLE_KTX_SUPPORT=OFF -DSPARKLE_QT_ROOT="C:/Qt/6.11.1/msvc2022_64"
cmake --build build\ux-validation-msvc --config DevelopmentEditor --target SparkleLauncher --parallel
artifacts\dev\launcher\DevelopmentEditor\SparkleLauncher.exe
```

Build and launch results:

- Configure passed from a clean validation tree.
- The attempted generic `Development|x64` build failed because Sparkle's actual CMake configurations are profile names such as `DevelopmentEditor` and `DevelopmentGame`; validation continued with the correct `DevelopmentEditor` profile.
- `SparkleLauncher` built successfully into `artifacts/dev/launcher/DevelopmentEditor`.
- Qt runtime deployment completed into the launcher artifact folder.
- `windeployqt` emitted `VCINSTALLDIR is not set`, but did not block artifact deployment or launcher startup.
- Launcher startup passed; the executable starts the live-copy instance under `artifacts/dev/launcher-state/Live`.
- Screenshots captured into `artifacts/diagnostics/launcher-ux-validation/`.

Implementation fixes made during validation:

- Rebuilt Home after project selection/discovery changes so the Command Center no longer shows `Project: Missing` while the footer selects `Showcase`.
- Changed Launch/Validate workflow guide copy from unconditional launch/run verbs to `Readiness first` so missing executable or cooked content blockers are not contradicted by the top recommendation.
- Changed the package primary CTA from stale `Planned` copy to `Assemble`.
- Compacted footer utility labels and control widths so the 980x620 smaller-window pass no longer overlaps Project, Config, and IDE context controls.
- Removed an MSVC `C4804` bool-comparison warning in dismissed history handling.

Screenshot evidence:

- `artifacts/diagnostics/launcher-ux-validation/accepted-home-wide.png`
- `artifacts/diagnostics/launcher-ux-validation/accepted-package-wide.png`
- `artifacts/diagnostics/launcher-ux-validation/accepted-home-small-final.png`
- Supporting workflow captures: `final-launch-wide.png`, `final-prepare-wide.png`, `final-build-wide.png`, `final-cook-wide.png`, `final-validate-wide.png`, `final-maintain-wide.png`

Acceptance checklist:

- PASS: A first-time user can understand project identity, source/package mode, launch status, and next action from Home within 30 seconds in the source-checkout state.
- CONDITIONAL: A first-time user can launch packaged editor/runtime within 2 minutes only when a runtime package with bundled components exists. This source-checkout validation did not include a package-root runtime fixture, so package-root launch-first must be revalidated after package assembly.
- PASS: Concrete engineering evidence is reachable from Home through evidence actions when files/folders exist, and footer diagnostics/folder utilities are one click away.
- PASS: Optional source rebuild/cook/sync paths are visible as production extensions rather than first-contact blockers.
- PASS: Launcher source contains no role-specific `Reviewer Mode`, `Developer Mode`, portfolio, or interview UI copy.
- PASS: Primary workflow pages now use a shared guide/readiness/options/details hierarchy with one dominant recommendation area.
- PASS: Ready workflows keep full prerequisite inventories in details or scoped readiness sections instead of raw audit tables.
- PASS: Missing launch/content/build states point to specific missing executable, stale workspace, texture, shader, or cook/build actions instead of a generic dependency checklist.
- PASS: Old unrelated failures are compact Activity/history information; raw logs stay behind the Activity log toggle.
- PASS: Source dependencies are described as source tiers and capability unlocks.
- PASS: Package artifacts and local source artifacts are distinguished through source/package mode, package copy, artifact roots, and `dist/releases/<version>` language.
- PASS: Advanced controls remain available but are not the first attention field.
- FAIL VISUAL: The implemented launcher does not yet read like the intended NVIDIA App-inspired engine launcher. It still feels too much like a custom Qt dashboard rather than a restrained product shell.
- PARTIAL: Direction A is still the primary information architecture, but final acceptance now requires the NVIDIA App-inspired shell, Home, tabs, settings/system rows, lime accent language, bounded content width, and hidden-by-default Activity/log model.

Remaining risks / follow-up:

- Package-root first-run behavior needs a real assembled runtime package fixture with bundled editor/runtime/cooked assets before the conditional launch-first acceptance criterion can be fully closed.
- `windeployqt` warns that `VCINSTALLDIR` is not set when launched from this shell. The build remains usable, but final release validation should run from a Visual Studio developer environment or set the expected deployment environment variables.
- The footer `Build Configuration` selector intentionally shows simplified state names (`Development`, `Debug`, `Shipping`) while CMake uses full profile configurations (`DevelopmentEditor`, `DevelopmentGame`, etc.); this is acceptable UX, but final developer docs should call out the mapping.
- The current visual implementation should not be used as acceptance evidence for the next UX pass. Future screenshots must be judged against the NVIDIA-inspired acceptance criteria.
## Archived Early Direction A Card-Grid Concepts

These concepts predate the NVIDIA App + Visual Studio Installer + Rider redesign direction. They are preserved for context only.

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
[Generate Workspace Files]    Secondary: Clean Build Files

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

[Generate Workspace Files]

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
* Scenes/Meshes  Missing  [Cook Scenes And Meshes]  Uses Content Pipeline Source Tier

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
Clean Generated Files                                    Safe scopes selected
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
## Archived Example App View Directions

These sketches predate the current NVIDIA App + Visual Studio Installer + Rider redesign direction. They are preserved for context only.

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
Clean Generated Files failed: launcher artifacts are locked. [Resolve] [Dismiss]
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