# Sparkle Launcher UI System Review

Date: 2026-06-05

## Purpose

This review looks at Sparkle Launcher as a product UI system before another implementation pass. The goal is to identify duplicated behavior, one-off UI logic, hardcoded product assumptions, and places where the code does not yet reflect the way users actually navigate the launcher.

This document started as an analysis-only pass. The first implementation pass has now been completed below as a controlled metadata/coherence refactor; larger widget/page extraction is intentionally deferred so the current approved NVIDIA-inspired visuals stay stable.

Review evidence:

- `artifacts/diagnostics/launcher-system-review/review-contact-sheet.png`
- `artifacts/diagnostics/launcher-system-review/review-quick-start.png`
- `artifacts/diagnostics/launcher-system-review/review-launch.png`
- `artifacts/diagnostics/launcher-system-review/review-sync.png`
- `artifacts/diagnostics/launcher-system-review/review-build.png`
- `artifacts/diagnostics/launcher-system-review/review-cook.png`
- `artifacts/diagnostics/launcher-system-review/review-test.png`
- `artifacts/diagnostics/launcher-system-review/review-package.png`
- `artifacts/diagnostics/launcher-system-review/review-maintain.png`

Coherence pass evidence:

- `artifacts/diagnostics/launcher-coherence-pass/before/before-contact-sheet.png`
- `artifacts/diagnostics/launcher-coherence-pass/after/after-contact-sheet.png`
- `artifacts/diagnostics/launcher-coherence-pass/before/`
- `artifacts/diagnostics/launcher-coherence-pass/after/`

Primary visual references:

- NVIDIA App: narrow rail, title band, strong hero imagery, restrained lime accent, bounded content, product tiles, calm settings/system pages
- Visual Studio Installer: workload cards, selection details, included/optional item hierarchy, clear commit area for consequential changes
- Rider: compact expert surfaces, searchable settings, predictable action menus, dense logs/diagnostics without polluting the main product view

## Executive Summary

The launcher now has a much better product direction than the earlier raw workflow dashboard. The narrow rail, Quick Start first-contact page, lime accent, product tiles, and bottom Activity area are the right direction.

The main problem is that the implementation does not yet have a UI system that matches the product system. The app visually contains repeated families, but the code still builds many of those families through local branches and operation-specific widget construction.

The highest-risk file is `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`. It currently owns shell composition, page scaffolding, card construction, workflow-specific options, action dependency rendering, activity behavior, launch options, clean scope UI, image treatment, and routing. That makes future work fragile because every new workflow can easily become another custom branch.

This should be fixed as an evolution, not a revolution. The current visual direction should stay, but the app needs explicit UI families and shared component contracts so similar things behave similarly by default.

## Current Product Model

The launcher now appears to have these user-facing domains:

- Quick Start: first-contact product launch and evidence discovery
- Launch: editor/runtime/IDE startup with project launch settings
- Sync: host validation and source tier synchronization
- Build: generated workspace files and local binary artifacts
- Cook: local generated content
- Test: smoke tests, formatting, and future quality gates
- Package: release assembly and package review
- Maintain: cleaning generated state and formatting support
- Activity: contextual run history, logs, and recovery

These domains are sound. The issue is that the implementation does not consistently treat them as reusable page families.

## Reference Translation

NVIDIA App patterns that fit Sparkle:

- Use a narrow icon rail for top-level product areas.
- Use a title band plus tabs for pages with subdomains.
- Use imagery for product identity, launchable products, and discovery/evidence areas.
- Keep host/tool/dependency details secondary unless they are the current blocker.
- Avoid permanent terminal-like chrome in first-contact views.
- Let cards fill horizontal space responsively while preserving proportions.

Visual Studio Installer patterns that fit Sparkle:

- Source dependency tiers should behave like workload cards.
- Clean scopes, package contents, and dependency tiers need selection summaries before running.
- Consequential actions need a clear "what will change / what will stay" commit model.
- Individual dependency lists belong in detail/search surfaces, not primary pages.

Rider patterns that fit Sparkle:

- Dense controls are acceptable in expert surfaces when alignment, search, and grouping are strong.
- Logs and diagnostics should feel like tool windows or drawers, not product content.
- Settings-like data should use rows, breadcrumbs, search, and right-aligned controls.
- Compact More menus should handle secondary actions without breaking row alignment.

## UX Families The App Should Recognize

These families should be explicit in code and behavior:

- Product launch surface: Editor, Runtime, future products/tools that can be opened from the selected project.
- Evidence tile surface: Architecture, Dependency Tiers, Validation, Package, Content, Tools.
- Workflow page: Launch, Sync, Build, Cook, Test, Package, Maintain.
- Action dependencies: collapsed readiness summary with expandable rows.
- Selection workflow: source tiers, clean scopes, package contents, cook targets.
- Commit workflow: actions that change source cache, generated files, build artifacts, cooked content, packages, or logs.
- Expert details: commands, logs, raw paths, full dependency inventory, file lists.
- Activity surface: current run, current failed workflow recovery, old run history.
- Global context: selected project, configuration, IDE/toolchain.
- Visual media: hero panorama, product cover, evidence ambient image, no-image row.

If a new operation belongs to one of these families, it should inherit default layout, state, action menu behavior, accessibility, and diagnostics behavior.

## Major Findings

### 1. Main Window Is Acting As The Whole Application

`LauncherMainWindow.cpp` is over 5,700 lines and contains too many responsibilities:

- shell and rail layout
- page construction
- workflow routing
- home hero/card layout
- option widgets
- dependency/readiness rendering
- activity/log panel behavior
- clean scope UI
- source tier UI
- image treatment
- per-operation branching

This makes the UI feel inserted over time because the code is inserted over time. Even when visuals look coherent, the implementation does not yet enforce coherence.

Recommended direction:

- Keep `LauncherMainWindow` as an orchestrator.
- Move reusable widgets and builders into focused files.
- Move operation/page metadata into a single catalog.

### 2. Operation Metadata Is Duplicated

Operation labels, verbs, impacts, and grouping are spread across:

- `LauncherWorkflowCatalog.cpp`
- `LauncherActionWidgets.cpp`
- `LauncherMainWindow.cpp`

This creates drift risk. For example, a workflow can be renamed in one place while button text, recovery text, or page routing still uses old language elsewhere.

Recommended direction:

- Create one operation/action catalog that owns display name, primary verb, impact kind, page family, dependency categories, artwork role, and suggested recovery actions.
- Other files should consume this metadata, not recreate it.

### 3. Page Behavior Is Hardcoded By Operation ID

`LauncherMainWindow::AddOptionsForOperation` is the clearest example of hardcoded behavior. It branches directly on operation IDs such as package, build, cook, launch, smoke test, format, and clean.

This is brittle because every new operation asks the developer to edit a central branching function. It also makes similar workflows diverge accidentally.

Recommended direction:

- Replace operation-ID page construction with page builders keyed by a `WorkflowPageKind`.
- Keep operation-specific data in metadata, not in handwritten widget branches.

Candidate page kinds:

- `ProductLaunch`
- `OpenWorkspace`
- `HostVerification`
- `SourceSync`
- `BuildArtifact`
- `CookContent`
- `QualityGate`
- `PackageAssembly`
- `MaintenanceClean`
- `Evidence`

### 4. Card Behavior Needs A Shared Contract

Home now has several card-like concepts:

- hero
- product cards
- discover/evidence tiles
- clean scope cards
- source tier/workload cards

Some cards are interactive, some are visual, some have action buttons, some have hidden actions, and some show status. This is fine, but it needs a contract. Right now behavior is partly encoded per card creation call.

Recommended direction:

- Define `CardKind`.
- Define which card kinds show image, title, status, subtitle, primary action, More menu, and hover affordance.
- Ensure every card kind has predictable click behavior.

Candidate card kinds:

- `Hero`
- `Product`
- `EvidenceTile`
- `WorkloadTier`
- `SelectionScope`
- `PackageItem`
- `DiagnosticSummary`

### 5. Discover Tiles Are Visually Cleaner But Need System Rules

Discover tiles now work better as title-only image tiles. That is a good direction because small tiles should not fight with cropped body text.

However, the rule should be generalized:

- Small visual discovery tiles show title only.
- Status is expressed through edge color, tooltip, and optional detail surface.
- Action buttons are hidden unless the tile is focused/opened or unless the action is the tile's primary purpose.

This should not be hardcoded to the current six tiles.

### 6. Quick Start Should Be Product-Led, Not Operation-Led

Quick Start should not duplicate Launch page controls. It should read the selected project, target, startup level, configuration, and IDE/toolchain from the shared global model and present the strongest available product path.

Risk to watch:

- Product names and artwork still appear tied to Showcase in places.
- Quick Start can accidentally become a second Launch page.

Recommended direction:

- Model `LaunchProduct` as Editor, Runtime, and future launchable products.
- Bind labels and images from selected project/product metadata.
- Keep Quick Start focused on opening a product, not configuring every launch parameter.

### 7. Sync And System Boundaries Need To Stay Clean

Removing the separate System page was the right call if it duplicated Sync. The underlying need remains valid: host tools, source tiers, artifact roots, and diagnostics are different information types.

Recommended direction:

- Sync owns host verification and source tier synchronization.
- Details inside Sync can expose host tools, source tiers, workspace files, and advanced diagnostics as tabs or expandable sections.
- Do not recreate a second System page unless it owns a different user outcome.

### 8. Action Dependencies Need A Standard State Model

The renamed "Action Dependencies" concept is better than "Readiness Summary", but it still needs a shared state model.

Recommended direction:

- Collapsed by default.
- Header shows aggregate state: Ready, Missing, Needs Refresh, Disabled, Failed, Warning.
- Rows use fixed right-side slots for status and More menu so alignment never breaks.
- Non-ready rows should expose granular recovery where possible.
- Granular recovery must run only the missing piece when the backend supports that scope.

### 9. Selection Workflows Should Share A Commit Pattern

Clean Workspace is much improved, but it should share a pattern with source tier sync, package assembly, and future quality gates.

Recommended direction:

- Selection cards show what is selected, present, missing, size/count if known, and what remains untouched.
- A summary states "Will change" and "Will keep".
- The primary action sits in a stable commit area.
- Destructive or expensive operations require confirmation.

This is the Visual Studio Installer lesson: complexity is allowed when the user is selecting components, but the selection consequences must be obvious.

### 10. Activity Is In The Right Place, But Needs A Component Contract

Moving Activity to the bottom was correct. It should remain a contextual tool surface.

Recommended direction:

- Default collapsed.
- Expands for active runs and current failures.
- Old unrelated failures remain accessible but quiet.
- Dismiss hides attention, not logs.
- Activity button should be icon-only if the rail/footer context already labels the area.

### 11. Images Need A Media System

The launcher is starting to use imagery in the NVIDIA-inspired way, but images should be attached to roles rather than specific widgets.

Recommended direction:

- `HeroPanorama`: wide image, top-level fade to app background, text area on left, proportional scaling.
- `ProductCover`: product card image, edge-to-edge top image, no top/side padding, proportional crop/fill.
- `EvidenceAmbient`: subtle abstract/engine image, title-only or very short copy.
- `None`: row/setting surfaces that should not use imagery.

Images should be selected from project/product metadata when possible, not hardcoded to Showcase.

### 12. Resizing Rules Need To Be Codified

The current desired behavior is clear:

- Hero scales proportionally and remains left-anchored.
- Product cards preserve proportions.
- Discover cards are exactly half the product card footprint when layout allows.
- Grids fill horizontal space by changing column count.
- No horizontal scrollbar should appear in normal supported window sizes.
- The app should enforce a practical minimum window size instead of collapsing into unusable layouts.

This belongs in layout code, not per-widget trial and error.

## Proposed Software Architecture

### Core Model Layer

Add or strengthen a presentation model that is independent of Qt widgets:

- `LauncherActionCatalog`
- `LauncherWorkflowModel`
- `LauncherProductModel`
- `LauncherReadinessModel`
- `LauncherSelectionModel`
- `LauncherMediaCatalog`

These should describe what exists and what it means.

### UI Component Layer

Move reusable widgets out of `LauncherMainWindow.cpp`:

- `LauncherShell`
- `LauncherPageScaffold`
- `LauncherHeroBanner`
- `LauncherResponsiveGrid`
- `LauncherProductCard`
- `LauncherEvidenceTile`
- `LauncherReadinessPanel`
- `LauncherSelectionSummary`
- `LauncherCommitBar`
- `LauncherActivityPanel`
- `LauncherOptionRows`

These should define how families look and behave.

### Page Builder Layer

Create page builders that consume models:

- `QuickStartPageBuilder`
- `LaunchPageBuilder`
- `SyncPageBuilder`
- `BuildPageBuilder`
- `CookPageBuilder`
- `TestPageBuilder`
- `PackagePageBuilder`
- `MaintainPageBuilder`

The page builders should assemble existing components rather than manually creating every row, card, and button.

### Main Window Layer

`LauncherMainWindow` should keep:

- application shell ownership
- route selection
- project/config/IDE context wiring
- backend signal wiring
- high-level refresh/update calls

It should not contain every page's widget implementation.

## Recommended Evolution Plan

### Phase 1: Metadata Unification

Goal:

- make operation metadata single-source before moving widgets

Work:

- unify display names, primary verbs, impact text, page kind, dependency kind, and recovery action metadata
- remove duplicated operation label functions
- map every operation to a workflow family

Validation:

- adding or renaming an operation requires changing one catalog entry, not several switch statements

### Phase 2: Shared Component Extraction

Goal:

- move repeated UI families out of `LauncherMainWindow`

Work:

- extract hero, responsive grid, product card, evidence tile, readiness panel, activity panel, option rows
- preserve current visuals while moving code

Validation:

- Quick Start screenshots should remain visually equivalent after extraction

### Phase 3: Page Builder Refactor

Goal:

- replace operation-specific page branching with workflow page builders

Work:

- split page construction by workflow family
- make `AddOptionsForOperation` disappear or become a small dispatcher

Validation:

- Launch, Sync, Build, Cook, Test, Package, and Maintain use shared scaffolds and family builders

### Phase 4: Selection And Commit Standard

Goal:

- standardize consequential workflows

Work:

- source tiers, clean scopes, package assembly, cook targets, and future quality gates use the same selection summary and commit model

Validation:

- every consequential workflow clearly shows what changes and what remains untouched

### Phase 5: Click And State Audit

Goal:

- ensure every visible control either works or is clearly disabled

Work:

- audit every button, tile, menu, tab, checkbox, dropdown, and status More menu
- remove no-op controls
- ensure non-ready states expose scoped recovery where supported

Validation:

- no clickable UI has no effect
- no disabled UI lacks a reason

### Phase 6: Visual Regression Pass

Goal:

- keep the current approved direction while improving consistency

Work:

- recapture Quick Start, Launch, Sync, Build, Cook, Test, Package, Maintain at normal and wide sizes
- compare against NVIDIA-inspired behavior: bounded content, image hierarchy, adaptive columns, restrained expert density

Validation:

- visual acceptance is based on screenshots, not build success

## Acceptance Criteria For The Refactor

- Similar actions share layout and behavior through a family, not copied branches.
- Operation display names, verbs, impact text, and page kind come from one source.
- Quick Start is product-led and does not duplicate Launch settings.
- Launch product names come from selected project/product metadata, not hardcoded Showcase strings.
- Source tiers behave like capability/workload cards.
- Clean, package, source tier sync, and future quality gates share a selection/commit pattern.
- Action Dependencies are collapsed by default and use stable row alignment.
- Every non-ready actionable row exposes a scoped recovery action where supported.
- Every clickable control either performs an action, opens a detail surface, or is visibly disabled with a reason.
- Images are assigned by media role and project/product metadata, not by local one-off widget code.
- Product cards, discover cards, and hero preserve proportions during resizing.
- The app enforces a practical minimum window size instead of showing horizontal scrollbars in normal usage.
- Activity remains bottom/contextual and does not compete with primary product content.
- `LauncherMainWindow.cpp` stops being the owner of every UI detail.

## Immediate Priority

The first implementation task should not be another visual tweak. The next best step is metadata unification plus component extraction while preserving the current screenshots as the visual baseline.

If that is done first, future visual iteration becomes safer because the launcher will finally understand its own UI system: products, workflows, actions, dependency states, selection scopes, media roles, and expert details.

## Coherence Pass 1: Metadata Unification

Status: completed on 2026-06-05.

What changed:

- Added `LauncherUiModel` as the shared presentation model for operation display name, primary verb, page family, impact category, impact text, visual title, visual text, and visual asset role.
- Wired `LauncherWorkflowCatalog`, `LauncherActionWidgets`, and `LauncherMainWindow` banner copy through the shared model instead of maintaining separate operation-name and operation-verb branches.
- Preserved existing backend operation IDs and workflow behavior.
- Preserved the current NVIDIA-inspired shell, Quick Start hero, product card layout, bottom Activity surface, and rail navigation.

Files changed:

- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherUiModel.h`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherUiModel.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherWorkflowCatalog.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherActionWidgets.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/LauncherMainWindow.cpp`
- `Tools/Launcher/SparkleLauncher/CMakeLists.txt`

Validation performed:

- Configured with `build/ux-validation-msvc`.
- Built `SparkleLauncher` in `DevelopmentEditor`.
- Launched the rebuilt launcher.
- Captured before/after screenshots for Quick Start, Launch, Sync, Build, Cook, Test, Package, and Maintain.

Commands:

```powershell
cmake -S . -B build\ux-validation-msvc -G "Visual Studio 18 2026" -A x64 -DSPARKLE_ENABLE_CONTENT_PIPELINE=OFF -DSPARKLE_ENABLE_SHADER_COMPILER=OFF -DSPARKLE_ENABLE_KTX_SUPPORT=OFF -DSPARKLE_QT_ROOT="C:/Qt/6.11.1/msvc2022_64"
cmake --build build\ux-validation-msvc --config DevelopmentEditor --target SparkleLauncher --parallel
artifacts\dev\launcher\DevelopmentEditor\SparkleLauncher.exe
```

Result:

- Build passed.
- Launcher started successfully.
- Qt deployment still emits the known `VCINSTALLDIR is not set` warning when launched from this shell; it did not block this validation.
- The after contact sheet confirms the app is visually coherent across all major rail modules after the metadata refactor.

Remaining intentional hardcoded areas:

- `LauncherMainWindow.cpp` still owns page construction, option sections, action dependency rendering, Quick Start card construction, clean scope presentation, execution request assembly, and prerequisite routing.
- `AddOptionsForOperation` still branches by operation ID and should become a page-family builder dispatcher.
- Recovery and dependency rows still contain operation-specific logic and should move into a shared action dependency state model.
- Product/media metadata still needs a dedicated project/product media model so future projects are not tied to Showcase-specific assumptions.

Next safe refactor:

- Extract shared component builders without changing visual output: hero banner, responsive card grid, product card, evidence tile, action dependency panel, option rows, and bottom Activity panel.
- Add a `LauncherWorkflowPageModel` or equivalent layer that consumes `LauncherUiModel` and backend readiness data, then feeds reusable widgets.
- Keep this as an evolution pass; do not rewrite the visual shell while extracting components.

## Coherence Pass 2: Design Families And Layout Tokens

Status: completed on 2026-06-05.

Goal:

- identify padding, scaling, layout, decoration, image, and button patterns that were still behaving like local one-offs
- move repeated measurements and interaction families into shared design tokens
- verify the launcher looks coherent across all major rail modules after the pass

What changed:

- Added `LauncherUiDesign` as the shared design-token contract for colors, spacing, window bounds, rail sizing, title band metrics, header controls, rows, buttons, page margins, activity panel sizing, clean scope cards, source tier cards, option rows, workflow visuals, hero geometry, Quick Start grids, and card bodies.
- Rewired `LauncherMainWindow` to consume the shared token families instead of repeating local constants for shell, title, header controls, option rows, clean scopes, source tiers, workflow banners, Quick Start cards, Activity rows, and hero copy spacing.
- Rewired `LauncherOutputWidgets` and `LauncherActionWidgets` to share Activity and overflow-menu sizing tokens.
- Added shared state-chip styling in `LauncherVisualStyle` so command cards, status rows, and source tiers use one semantic state family instead of repeated ready/warning/bad CSS blocks.
- Kept operation behavior unchanged; this pass only normalized presentation rules and layout families.

Design families now explicitly represented:

- `Window`: minimum and initial launcher size so normal usage does not collapse into horizontal-scroll failure states.
- `Shell`: rail width, rail item height, icon size, workflow tab spacing, and root panel margins.
- `TitleBand`: title/header strip spacing and margins.
- `HeaderContext`: Project, Config, and IDE selector sizing and spacing.
- `Page`: bounded content width and source/package page margins.
- `Hero`: proportional design size, copy bounds, divider, copy scale limits, and hero spacing.
- `Home`: Quick Start body margins, section spacing, product/discover card widths, and adaptive card limits.
- `Card`: product/discover artwork, body, spacing, and flush-artwork rules.
- `WorkflowVisual`: shared banner artwork and copy treatment for non-Home workflow pages.
- `Option`: label/value row spacing, details spacing, and option-group margins.
- `Clean`: cleanup scope card spacing and grid margins.
- `SourceTier`: source tier workload-card sizing and margins.
- `Activity`: collapsed/expanded height, list width, row height, toggle glyphs, and output margins.
- `Overflow`: consistent More-menu icon/button sizing.

Validation performed:

- Built `SparkleLauncher` in `DevelopmentEditor`.
- Launched the rebuilt launcher.
- Captured all primary rail modules after the token pass.

Commands:

```powershell
cmake --build build\ux-validation-msvc --config DevelopmentEditor --target SparkleLauncher --parallel
artifacts\dev\launcher\DevelopmentEditor\SparkleLauncher.exe
```

Screenshot evidence:

- `artifacts/diagnostics/launcher-ui-system-review/pass2/01-quick-start.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/02-launch.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/03-sync.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/04-build.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/05-cook.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/06-test.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/07-package.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/08-maintain.png`
- `artifacts/diagnostics/launcher-ui-system-review/pass2/contact-sheet.png`

Pass/fail review:

- PASS: rail sizing, active state, icon size, and rail spacing are now one shell family across every module.
- PASS: title band and Project/Config/IDE selectors are now one header family instead of duplicated control sizing.
- PASS: Quick Start, Launch, Sync, Build, Cook, Test, Package, and Maintain use the same title band, bounded content, bottom Activity strip, and primary-action placement.
- PASS: product cards and discover cards use declared card families instead of ad-hoc padding and image sizes.
- PASS: source tiers, clean scopes, option rows, text edits, and workflow banners use named layout families.
- PASS: state chips now share semantic styling rules for neutral, ok, warning, and bad states.
- PASS: the screenshot set confirms the app is visually coherent across all major modules after this pass.
- PARTIAL: hero/image fade math is centralized around `LauncherUiDesign`, but the custom renderer still contains paint-level gradient stops. This is acceptable for the custom hero component, but should move into a dedicated hero widget file during component extraction.
- PARTIAL: `LauncherVisualStyle.cpp` centralizes QSS, but still contains many raw style literals. This is acceptable as the stylesheet owner, but future polish should split semantic style fragments if the stylesheet grows further.
- PARTIAL: `LauncherMainWindow.cpp` still owns too much UI assembly. The token pass makes it safer, but it is not yet a clean component architecture.

Important non-goals:

- This pass did not click destructive actions such as Clean, Build, Cook, Package, or Sync. Those controls were reviewed visually and by code wiring, while behavior validation remains a separate action-safety pass.
- This pass did not redesign the approved NVIDIA-inspired shell or hero behavior.
- This pass did not change backend operation IDs, build commands, cook commands, or dependency synchronization semantics.

Remaining architecture cleanup:

- Extract hero rendering, responsive card grid, product/discover card, workflow visual banner, option groups, source tier cards, clean scope cards, and Activity panel into reusable widget/component files.
- Move remaining page construction from `LauncherMainWindow.cpp` into workflow-family page builders.
- Replace operation-ID branching inside option assembly with `LauncherUiModel` or a new `LauncherWorkflowPageModel`.
- Give media usage a stronger product/project media model so images are assigned by product role instead of UI construction code.
- Add a dedicated click/state audit that checks every safe control and inspects destructive controls without running them accidentally.
