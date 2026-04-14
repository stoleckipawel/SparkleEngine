## Plan: Explicit EditorApp And ProjectApp

Replace the renderer-owned overlay abstraction with an explicit Unreal-style split: an editorless `ProjectApp` as the runtime/game application layer, and a separate `EditorApp` above it that owns editor UI, editor frame policy, and editor-specific launch behavior. Renderer should go back to being purely runtime-facing. Editor code should stop being injected downward through Renderer or through `App` constructor plumbing. The architecture should read clearly as `ProjectApp` for game/runtime and `EditorApp` for tools/editor.

**Closeout Snapshot — 2026-04-14**
- Implemented host split: `SparkleApplication` now exposes `ProjectApp` and `EditorApp`; the legacy `App` compatibility wrapper has been removed.
- Implemented renderer/editor split: the old overlay seam is gone from the live engine and project sources; there are no remaining `IRendererOverlay`, `RendererOverlayFactory`, `CreateEditorOverlay`, or `AddUiCompositionPass` references in `Engine/**` or `Projects/**`.
- Implemented launch split: `Projects/Showcase` and `Projects/TemplateProject` now provide explicit editor/runtime entrypoints and split launch targets.
- Implemented runtime boundary guardrail: `runtime_cooked_boundary_check` validates `Engine/Application`, `Engine/GameFramework`, and `Engine/Renderer` against source-import and authoring leakage.
- Validation status: build, launch smoke tests, and cooked-runtime checks passed. Visual pixel inspection of the in-editor viewport remains a manual follow-up when a human needs to confirm layout or image correctness on screen.
- Remaining follow-up scope is discrete rather than architectural: the main renderer debt still worth pursuing is private D3D12-specific implementation under `Engine/Renderer/Private`, not host ownership or overlay coupling.

**Target Shape**
- `ProjectApp`: editorless runtime application. Owns timer, window, input, game scene, scene asset manager, level manager, camera controller, and renderer.
- `EditorApp`: higher-level tools host. Owns `ProjectApp`, owns `UI`, orchestrates the high-level editor host loop, and issues editor viewport and tooling requests through generic renderer-facing APIs.
- `Renderer`: runtime-only renderer. Knows nothing about editor UI types, editor factories, or editor service bags.
- Project executables: explicit launch choice. A project can have an editor entry point using `EditorApp` and a runtime/game entry point using `ProjectApp`.

**Viewport Boundary**
- The editor still has a game viewport, but that viewport is editor-owned UI, not a renderer-owned overlay.
- `ProjectApp` owns runtime world state and runtime render submission. It may expose a narrow runtime-facing viewport contract, but it must not know about ImGui, docking, panels, or editor chrome.
- `Renderer` should render scene output into a runtime-owned viewport surface when hosted by `EditorApp`, rather than assuming the scene always renders directly to the swapchain backbuffer.
- `EditorApp` owns the viewport panel, decides its size and active view, requests the corresponding runtime scene output from `ProjectApp`, and embeds that output into editor UI.
- The boundary should stay narrow: viewport dimensions, active runtime camera or view selection, and a runtime-rendered texture handle or view descriptor that editor UI can display. Do not pass editor widgets or editor service bags through Renderer APIs.
- `EditorApp` specifies what editor-facing outputs it needs and where they are shown, while `Renderer` decides how to fulfill those requests through generic composition and present paths: runtime scene output is shown inside the editor viewport panel, editor panels are drawn by editor UI, and renderer services execute the required rendering work without becoming editor-specific.
- In the editor path, the window backbuffer is editor host output. In the runtime-only path, `ProjectApp` can still present directly with no editor viewport indirection.

**Design Principles**
- Separate policy from mechanism. `EditorApp` owns editor policy and requirements. `Renderer` owns generic rendering mechanism and execution. `RHI` owns GPU/backend mechanics.
- The editor requests capabilities and outputs; the renderer decides the internal execution strategy. Do not make `EditorApp` responsible for low-level rendering decisions.
- Prefer generic renderer capabilities over `#if WITH_EDITOR`. Treat compile-time editor conditionals inside Renderer as a last resort for tiny unavoidable hooks, not as the default extension model.
- Express editor needs as renderer-facing data, not editor-facing interfaces. Examples: view requests, viewport surface descriptors, optional auxiliary outputs, composition plans, and debug flags.
- Keep runtime app types editorless by construction. `ProjectApp` should remain valid for a shipping game executable, test harness, or headless runtime host without editor code present.
- Keep the editor above runtime. `EditorApp` may depend on runtime systems and renderer services, but runtime modules should not depend on editor modules.
- Build for future multi-viewport and tool rendering now. The design should naturally support game view, scene view, asset preview, thumbnails, picking, and debug visualization without a second refactor.

**Unreal Alignment Guardrails**
- Keep the runtime executable path and editor executable path explicitly separate, even if they share most underlying runtime modules. This matches Unreal's game-versus-editor launch split more closely than a single mixed host.
- Keep editor-only orchestration above runtime modules. Editor code may coordinate runtime systems, but runtime modules must not gain awareness of editor hosts, editor widgets, or editor data models.
- Prefer thin editor specializations over forking runtime architecture. Shared runtime systems should stay in runtime modules, while editor-specific behavior should live in editor hosts, editor modules, or editor-only adapters above them.
- Keep renderer features generic and reusable. If a feature is useful for game, editor, preview, or tooling views, model it as a generic renderer capability rather than an editor-only special case.
- Treat backend abstraction as non-negotiable. Unreal-style layering only holds if renderer logic stays above the graphics API layer and backend details stay in the RHI/backend implementation.
- Keep authoring, cooking, and source import out of the runtime path. Runtime should consume cooked data only; import/cook workflows belong to editor/tooling code.
- Prefer explicit module and target boundaries over convenience links. If a dependency direction is architecturally wrong, do not hide it behind factories or service bags.
- If a feature genuinely cannot fit cleanly into generic renderer APIs, prefer an editor-only bridge above Renderer before adding backend-specific or editor-specific conditionals inside core renderer code.

**Policy Vs Mechanism**
- `EditorApp` policy:
	- which viewports exist
	- what each viewport is showing
	- viewport layout and sizing
	- which editor tools are active
	- whether a view is game, scene, preview, thumbnail, or debug-oriented
	- which renderer features or outputs are requested by the editor
- `Renderer` mechanism:
	- allocate or resize render surfaces
	- build scene views from generic view requests
	- execute passes and composition plans
	- manage view families, render outputs, and present paths
	- translate requests into command recording through `SparkleRHI`
	- choose how requested features are implemented and scheduled internally
- `RHI` backend execution:
	- textures, descriptor views, swapchain, command queues, barriers, and backend synchronization

**Engine Module Inventory**
- `SparkleCore`
	- Foundation layer.
	- Owns math, containers, logging, events, diagnostics, hashing, strings, and time.
	- Must remain free of rendering, editor, and gameplay policy.
- `SparklePlatform`
	- OS abstraction layer.
	- Owns windowing, input, filesystem, OS message handling, and other platform services.
	- May know about native window handles and platform UI integration points, but not editor concepts.
- `SparkleRHI`
	- Graphics API abstraction layer.
	- Exists to keep Renderer focused on rendering logic while giving the engine a path to support multiple graphics backends such as D3D12 and Vulkan.
	- Owns device resources, swapchains, descriptor heaps, command contexts, shader/backend integration, and low-level render resource lifetime.
	- Should expose GPU/resource primitives used by Renderer and possibly UI backends, but not editor policy.
- `SparkleRenderer`
	- Generic rendering layer.
	- Should remain graphics-API agnostic so the engine can support multiple backends such as D3D12 and Vulkan without rewriting renderer logic.
	- Owns scene view rendering, framegraph assembly, render surface management, view families, generic composition, debug visualization primitives, and present execution.
	- Should avoid direct D3D12-specific code and types wherever possible; backend-specific implementation details should stay in `SparkleRHI` behind clean API-agnostic abstractions.
	- Must not know about editor panels, editor lifetime, or editor-owned interfaces.
- `SparkleGameFramework`
	- Runtime world and asset layer.
	- Owns levels, game scene state, runtime camera state, cooked scene asset resolution, scene asset manager, and runtime-facing simulation state.
	- Must remain free of editor UI and source import workflows.
- `SparkleApplication`
	- Host layer that now contains the explicit `ProjectApp` runtime host and `EditorApp` editor host.
	- Owns high-level host orchestration across Platform, GameFramework, Renderer, and editor UI coordination.
	- Keeps runtime-only flow in `ProjectApp` and editor policy/orchestration in `EditorApp`.
- `SparkleEditor`
	- Editor tools layer.
	- Owns panels, viewport widgets, menuing, selection, inspectors, editor commands, editor camera behavior, asset tools, and tool-mode policy.
	- Should consume generic renderer capabilities rather than being consumed by Renderer.
- `AssetConverter`
	- Offline tool boundary for source import, conversion, cooking, and authoring workflows.
	- Preferred separate module/tool for non-runtime asset processing.
	- Any shared authoring implementation should remain internal to `AssetConverter` or otherwise stay tool-scoped, rather than becoming a broad engine-facing module in the target architecture.
	- Must remain outside runtime load paths.

**Responsibility Matrix**
- `EditorApp`
	- owns editor frame policy
	- owns high-level host loop sequencing for editor mode
	- owns viewport layout and which views are needed
	- owns selection, tool mode, editor camera mode, and editor commands
	- asks for runtime render outputs and optional renderer features using generic renderer-facing requests
	- does not own low-level command recording, barriers, descriptor management, or swapchain transitions
- `ProjectApp`
	- owns runtime frame orchestration, runtime state, runtime camera selection, level flow, simulation update, and runtime-facing render requests
	- may translate runtime world state into renderer-facing scene/view requests
	- does not own editor panels, docking, or editor widget lifetime
- `Renderer`
	- owns generic rendering capabilities and execution
	- owns render surfaces, view families, render products, scene output bundles, and generic composition execution
	- may support editor use cases through generic features such as auxiliary outputs, picking buffers, and debug overlays
	- decides how requested features are realized in framegraph, passes, resources, and present flow
	- does not own `EditorApp`, `UI`, `IRendererOverlay`, or editor factory hooks
- `RHI`
	- owns backend texture handles, descriptor views, GPU queues, shader-visible heaps, and present mechanics
	- may provide generic view/handle export helpers needed by editor UI texture presentation
	- does not own scene, editor, or gameplay semantics
- `SparkleEditor` UI layer
	- owns actual editor panels and viewport widgets
	- presents runtime-produced surfaces inside editor UI
	- does not become the place where generic rendering is reimplemented

**Current Coupling To Remove**
- Renderer-owned overlay interfaces and factories in Renderer public headers.
- Application constructor plumbing that carries editor overlay creation downward into runtime layers.
- Editor UI inheriting from a renderer-owned overlay interface.
- Presentation passes that directly call editor UI render functions.
- Any path where Renderer conceptually owns editor lifetime instead of servicing editor requirements.
- Any direct D3D12-specific types, headers, command logic, or backend assumptions inside `SparkleRenderer` that bypass `SparkleRHI` and prevent clean Vulkan support.
- Any `SparkleRHI` APIs or implementations that reach upward into renderer policy, scene semantics, editor concepts, or gameplay ownership instead of staying focused on graphics API and GPU mechanics.
- Any `SparkleGameFramework` dependency on editor UI, source import, cooking orchestration, or other non-runtime authoring workflows.
- Any `SparkleEditor` path that starts owning low-level render execution, descriptor management, barriers, or backend-specific graphics logic instead of issuing requests through renderer-facing APIs.
- Any monolithic `SparkleApplication` ownership pattern that hides the distinction between runtime hosting (`ProjectApp`) and editor hosting (`EditorApp`).
- Long-term dependence on a standalone `SparkleAssetAuthoring` engine module if `AssetConverter` is intended to be the real tool boundary.
- Any plan that places the long-lived editor host conceptually below runtime modules instead of above them.
- Any plan that duplicates runtime systems in editor-specific code instead of reusing the same runtime modules with editor-side orchestration.

**Actions To Take**
1. Establish a strict `SparkleRenderer` <-> `SparkleRHI` boundary.
	- Move backend-specific rendering code, backend headers, native API types, and backend resource manipulation behind `SparkleRHI` abstractions.
	- Define the renderer-facing RHI surface in terms of API-agnostic concepts such as textures, buffers, pipelines, descriptor abstractions, command contexts, and presentation interfaces.
	- Treat direct D3D12 calls from renderer code as migration debt to remove so Vulkan support remains viable.
2. Split the host layer into explicit runtime and editor hosts.
	- Evolve `SparkleApplication` toward an explicit `ProjectApp` runtime host.
	- Add `EditorApp` as the editor host above `ProjectApp`.
	- Remove constructor shapes and service plumbing that inject editor-owned objects downward into runtime layers.
3. Replace renderer-owned editor composition with request/response rendering contracts.
	- Remove `IRendererOverlay`, `RendererOverlayFactory`, and renderer-owned editor lifetime hooks.
	- Introduce viewport and render-output request structures owned at the app/editor boundary and fulfilled by Renderer.
	- Make editor UI consume runtime-produced surfaces instead of being invoked directly by renderer presentation code.
4. Keep `SparkleEditor` focused on tools and UI.
	- Let editor code own layout, viewport widgets, inspectors, menuing, selection, and tool commands.
	- Do not let editor code become the place where backend-specific rendering or low-level GPU orchestration lives.
	- If the editor needs more rendering features, request them through generic renderer-facing APIs first.
5. Keep `SparkleGameFramework` runtime-only.
	- Preserve cooked-scene loading, runtime asset resolution, level flow, runtime camera state, and runtime simulation here.
	- Remove or block any dependency on editor panels, import/cook tooling, and source asset workflows.
6. Make `AssetConverter` the clear non-runtime authoring boundary.
	- Keep source import, conversion, and cooking workflows under the `AssetConverter` tool boundary.
	- Avoid growing a broad engine-facing standalone authoring module if the real architectural boundary is the tool.
	- Any reusable authoring code should stay tool-scoped unless a narrower, well-justified interface emerges.
7. Audit module dependencies and includes against the target architecture.
	- Renderer should depend on RHI, not D3D12 directly.
	- Editor should depend on runtime and renderer-facing APIs, not the reverse.
	- RHI should not depend on editor policy or gameplay semantics.
	- Runtime modules should remain free of editor and authoring dependencies.
8. Validate the architecture with target-level and code-level checks.
	- Confirm public headers follow the intended direction of dependencies.
	- Confirm build targets can express editor-enabled and runtime-only launch paths cleanly.
	- Confirm backend portability remains plausible by keeping renderer code API-agnostic.

**Unreal-Style Review Notes**
- Good fit with Unreal-style practice:
	- explicit editor-versus-runtime host split
	- runtime-only renderer and cooked-only runtime asset path
	- editor above runtime rather than injected into renderer
	- backend abstraction pushed into RHI
- Areas to keep watching:
	- do not let `EditorApp` become a second engine or second renderer
	- do not let `SparkleApplication` remain a mixed host after `ProjectApp` and `EditorApp` are introduced
	- do not let RHI leak scene- or gameplay-aware concepts upward or downward
	- do not reintroduce editor composition through renderer-owned public interfaces for convenience

**Preferred Container Model**
```text
Project Editor Executable
		|
		v
EditorApp
	- editor policy
	- host loop sequencing
	- viewport requirements
	- tool state
		|
		+---------------------------+
		|                           |
		v                           v
ProjectApp                  SparkleEditor UI
	- runtime state             - panels
	- runtime update            - viewport widgets
	- level / scene flow        - inspectors / outliner
	- runtime render requests   - tool interaction
		|
		v
SparkleRenderer
	- generic scene rendering
	- viewport surfaces
	- composition execution
	- present execution
		|
		v
SparkleRHI
	- textures
	- descriptors
	- command lists / queues
	- swapchain
		|
		v
SparklePlatform
	- window
	- input
	- OS events
```

**Recommended Viewport Contract Shape**
- Editor-facing request data:
	- viewport identifier
	- viewport dimensions
	- requested view type: game, scene, preview, thumbnail, debug
	- active camera or camera provider selection
	- optional render features: picking, wireframe, lighting-only, debug overlays, object ID, gizmo overlay support
- Runtime/renderer-facing output data:
	- scene color surface handle or descriptor
	- optional depth surface handle
	- optional object ID / selection / auxiliary buffers
	- metadata such as rendered extent and format
- Keep this contract generic enough that the same mechanism can power editor viewport panels, asset preview windows, and tooling thumbnails.

**Renderer Capability Model**
- Good renderer concepts:
	- render surface
	- viewport surface
	- scene view request
	- scene view family
	- render output bundle
	- composition plan
	- debug visualization flags
	- optional auxiliary outputs such as depth, normals, object ID, or overlay masks
- Bad renderer concepts:
	- editor panel
	- outliner
	- inspector
	- `EditorApp`
	- `IRendererOverlay`
	- `CreateEditorOverlay`
	- ImGui widget concepts

**Engine Pattern Comparison**
- Unreal-style pattern:
	- editor host sits above runtime/game systems
	- editor owns tool windows and viewport clients
	- renderer remains a shared capability used by both runtime and editor
	- strong fit for explicit `EditorApp` + `ProjectApp`
- Unity-style pattern:
	- editor windows host texture-backed views
	- editor owns layout and tooling while renderer/services provide outputs
	- strong fit for editor viewport panels backed by generic render surfaces
- Frostbite-style pattern:
	- tools often specify requirements as data and renderer executes them through generic systems
	- strong fit for view request / render output contracts and low reliance on `#if WITH_EDITOR`
- Preferred Sparkle direction:
	- Unreal-style ownership
	- Unity-style viewport hosting
	- Frostbite-style data-driven renderer requests

**Decision Trees**
- Where should editor viewport policy live?
	- `EditorApp`
		- preferred
		- editor owns which views exist and what they mean
	- `Renderer`
		- reject
		- would force renderer to own tool semantics
- Where should composition execution live?
	- generic renderer paths
		- preferred
		- renderer already owns the backbuffer/present path and render graph execution
	- `EditorApp`
		- reject for low-level execution
		- would duplicate renderer mechanism in the app host
- How should editor consume renderer functionality?
	- generic view/surface/output APIs
		- preferred default
		- keeps renderer editor-agnostic
	- `#if WITH_EDITOR` inside Renderer
		- last resort
		- acceptable only for tiny unavoidable hooks
	- an editor-render bridge module above Renderer
		- preferred fallback if editor rendering grows beyond what generic data-driven requests can express cleanly
- How should the app layer split?
	- keep one `App` with injected editor factories
		- reject
		- keeps ownership muddy
	- explicit `ProjectApp` + `EditorApp`
		- preferred
		- clearest boundary and best long-term maintainability

**Future-Ready Extension Path**
- Phase A
	- split `ProjectApp` and `EditorApp`
	- remove overlay ownership inversion
	- introduce generic viewport surface and render-output contracts
- Phase B
	- add generic auxiliary outputs for picking, selection, debug overlays, and preview rendering
	- keep these as renderer features, not editor-specific hard links
- Phase C
	- if needed, introduce a dedicated editor-render bridge module above Renderer for truly editor-specific rendering translators rather than expanding `#if WITH_EDITOR` in core Renderer

**Detailed Architectural Guidance**
- The editor should not become a second renderer.
- The renderer should not become a tool framework.
- The app layer should not be the place where editor and renderer are glued together through ad hoc factories.

- The preferred handshake is:
	- editor expresses required views and constraints
	- editor may request optional renderer features or outputs
	- runtime provides world and scene state
	- renderer decides how to realize those requests and executes generic rendering work
	- UI presents renderer outputs
- A game viewport inside the editor is not special because of where it appears on screen. It is special because the editor chooses to host a runtime-produced view inside editor chrome.
- That means the viewport is editor-owned as UX, runtime-owned as scene source, and renderer-owned as rendering execution.

**Implementation Phases (Historical Execution Plan)**

Current repo-state truth now lives in the Closeout Snapshot, Relevant Files, and Final Validation Results sections. The phase notes below are kept as historical execution context.

1. Phase 0 — Freeze the target architecture and block further drift.
	- Goal:
		- lock the host split, renderer/RHI split, cooked-runtime-only rule, and `AssetConverter` tool boundary before code refactors begin
	- Current repo state to account for:
		- `Engine/Application/Public/App.h` and `Engine/Application/Private/App.cpp` still expose and store `RendererOverlayFactory`
		- `Engine/Renderer/Public/Renderer.h` and `Engine/Renderer/Private/Renderer.cpp` still own overlay construction and overlay lifetime
		- `Engine/Renderer/Public/Overlays/RendererOverlay.h` still defines the renderer-owned editor seam
		- `Engine/Editor/Public/UI.h` and `Engine/Editor/Private/UI.cpp` still implement `IRendererOverlay` and `CreateEditorOverlay`
		- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h/.cpp` and `Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.h/.cpp` still route UI through renderer-owned overlay plumbing
		- `Projects/Showcase/Src/main.cpp` and `Projects/TemplateProject/Src/main.cpp` still construct `App app(CreateEditorOverlay);`
	- Suggested implementation prompt count:
		- 1 prompt
	- Prompt 0.1 — Mark the old seams as transitional debt and prevent new work from landing on them.
		- Files to touch:
			- `Engine/Application/Public/App.h`
			- `Engine/Renderer/Public/Renderer.h`
			- `Engine/Renderer/Public/Overlays/RendererOverlay.h`
			- `Engine/Editor/Public/UI.h`
			- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h`
			- `Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.h`
		- Concrete actions:
			- add short architecture notes or TODO markers that these seams are migration-only and must not be expanded
			- update comments that still imply the overlay path is the intended long-term design
			- update any misleading host/renderer wording that could cause future prompts to build on the wrong abstraction
		- Validation:
			- no behavior change
			- the repository comments and prompt text consistently identify overlay plumbing as debt, not destination architecture
	- Completion gate:
		- the plan text, terminology, and module rules are stable enough to drive implementation prompts without reopening basic ownership questions

2. Phase 1 — Create the explicit host split without changing runtime responsibilities.
	- Goal:
		- make the runtime host and editor host explicit in code structure
	- Current repo state to account for:
		- `App` currently owns the entire runtime loop, subsystem creation, and renderer bootstrapping
		- there is no `ProjectApp` or `EditorApp` type in the repository yet
		- the project entrypoints instantiate `App` directly and use the overlay seam as the editor bootstrap path
	- Suggested implementation prompt count:
		- 3 prompts
	- Prompt 1.1 — Extract `ProjectApp` from the current `App` monolith while preserving current runtime behavior.
		- Files to touch:
			- `Engine/Application/Public/App.h`
			- `Engine/Application/Private/App.cpp`
			- new `Engine/Application/Public/ProjectApp.h`
			- new `Engine/Application/Private/ProjectApp.cpp`
		- Concrete actions:
			- move runtime-owned subsystem members and initialization logic into `ProjectApp`
			- split the current run loop into hostable pieces such as initialize, tick, and shutdown or an equivalent one-frame API
			- keep a temporary runtime-only `Run()` wrapper if it helps maintain current behavior while the host split is incomplete
		- Validation:
			- runtime-only behavior still works after `ProjectApp` extraction
			- `ProjectApp` can be constructed without introducing editor-specific state bags beyond the existing temporary seam
	- Prompt 1.2 — Introduce `EditorApp` above `ProjectApp` as the long-lived editor host.
		- Files to touch:
			- new `Engine/Application/Public/EditorApp.h`
			- new `Engine/Application/Private/EditorApp.cpp`
			- `Engine/Application/Public/ProjectApp.h`
			- `Engine/Application/Private/ProjectApp.cpp`
		- Concrete actions:
			- add `EditorApp` that owns `ProjectApp`
			- move high-level host loop sequencing, editor session lifetime, and editor-mode orchestration into `EditorApp`
			- keep render-pass ordering and GPU execution inside Renderer
			- if needed, keep the existing overlay seam only as a temporary bootstrap dependency while later phases remove it
		- Validation:
			- `EditorApp` can host `ProjectApp`
			- the high-level loop is visibly split between editor host and runtime host
	- Prompt 1.3 — Switch project entrypoints to the explicit editor host.
		- Files to touch:
			- `Projects/Showcase/Src/main.cpp`
			- `Projects/TemplateProject/Src/main.cpp`
			- optionally `Projects/Showcase/CMakeLists.txt`
			- optionally `Projects/TemplateProject/CMakeLists.txt`
		- Concrete actions:
			- instantiate `EditorApp` instead of `App`
			- keep current working-directory and startup assumptions intact
			- do not split targets yet unless it is necessary to land the host type changes cleanly
		- Validation:
			- editor-enabled projects still launch through the new host type
	- Must not do:
		- do not move render-pass ordering or low-level GPU work into `EditorApp`
		- do not add new editor-specific hooks to renderer public API during the host split
	- Completion gate:
		- `EditorApp` can host `ProjectApp`
		- `ProjectApp` is constructible as an editorless runtime host
		- the host split is visible in code, even if some transitional rendering seam still exists internally for a short period

3. Phase 2 — Rebuild the renderer/editor boundary around requests and outputs.
	- Goal:
		- replace renderer-owned editor composition with a request/response boundary that matches the final architecture
	- Current repo state to account for:
		- overlay concepts live in `Engine/Renderer/Public/Overlays/RendererOverlay.h`
		- `Renderer` constructor still accepts `RendererOverlayFactory`
		- `FrameGraphDependencies` still carries `IRendererOverlay* overlay`
		- `PresentationPasses.cpp` still adds `AddUiCompositionPass` that calls `overlay.Render(...)`
	- Suggested implementation prompt count:
		- 3 prompts
	- Prompt 2.1 — Define generic viewport request and renderer output contracts.
		- Files to touch:
			- new renderer-public headers under a location such as `Engine/Renderer/Public/Viewport/` or equivalent
			- `Engine/Renderer/Public/Renderer.h`
		- Concrete actions:
			- add API-agnostic types for viewport requests, requested outputs, view kinds, feature flags, and render products
			- keep the contracts editor-agnostic so they can serve editor viewports, preview windows, and runtime views alike
			- make sure the contracts do not contain ImGui types, editor panel types, or renderer-owned editor callbacks
		- Validation:
			- renderer-facing request/output types exist and are editor-agnostic
	- Prompt 2.2 — Remove overlay plumbing from `ProjectApp` and `Renderer` public APIs.
		- Files to touch:
			- `Engine/Application/Public/App.h` or `Engine/Application/Public/ProjectApp.h`
			- `Engine/Application/Private/App.cpp` or `Engine/Application/Private/ProjectApp.cpp`
			- `Engine/Renderer/Public/Renderer.h`
			- `Engine/Renderer/Private/Renderer.cpp`
			- `Engine/Renderer/Public/Overlays/RendererOverlay.h`
		- Concrete actions:
			- remove constructor-time overlay factory injection from the host/renderer boundary
			- replace service-bag editor bootstrap with request submission and output retrieval methods
			- either delete `RendererOverlay.h` or reduce it to a short-lived transitional shim that is immediately scheduled for deletion in a follow-up prompt
		- Validation:
			- hosts and renderer no longer communicate through `RendererOverlayFactory`
	- Prompt 2.3 — Remove overlay usage from framegraph and presentation flow.
		- Files to touch:
			- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h`
			- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp`
			- `Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.h`
			- `Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.cpp`
			- `Engine/Renderer/Private/Renderer.cpp`
		- Concrete actions:
			- remove `IRendererOverlay* overlay` from framegraph dependencies
			- remove `AddUiCompositionPass`
			- make renderer produce generic scene outputs or presentation products instead of calling editor UI from inside the framegraph
		- Validation:
			- framegraph and presentation code compile without any overlay interface dependency
	- Must not do:
		- do not replace one editor-specific renderer seam with another editor-specific renderer seam under a different name
		- do not pass editor widgets, editor state bags, or UI framework types through renderer APIs
	- Completion gate:
		- Renderer accepts generic requests and returns generic render products
		- editor-side code consumes renderer outputs instead of being called by renderer code

4. Phase 3 — Make the in-editor viewport a pure editor-hosted presentation of runtime output.
	- Goal:
		- keep the game viewport in the editor while preserving the runtime/editor boundary
	- Current repo state to account for:
		- `UI` still inherits `IRendererOverlay`
		- `UI.cpp` still owns ImGui frame build and direct DX12 draw submission
		- current editor UI contains menu/outliner/inspector panels but no explicit viewport panel abstraction yet
	- Suggested implementation prompt count:
		- 3 prompts
	- Prompt 3.1 — Make `UI` a pure editor-owned object and delete overlay inheritance.
		- Files to touch:
			- `Engine/Editor/Public/UI.h`
			- `Engine/Editor/Private/UI.cpp`
		- Concrete actions:
			- remove `IRendererOverlay` inheritance
			- delete `CreateEditorOverlay`
			- keep editor-owned frame build methods but detach them from renderer-owned interfaces
		- Validation:
			- `UI` no longer depends on renderer-owned overlay types in its public API
	- Prompt 3.2 — Add an explicit editor viewport panel that consumes runtime-produced surfaces.
		- Files to touch:
			- new `Engine/Editor/Public/Panels/ViewportPanel.h`
			- new `Engine/Editor/Private/Panels/ViewportPanel.cpp`
			- `Engine/Editor/Public/UI.h`
			- `Engine/Editor/Private/UI.cpp`
		- Concrete actions:
			- create a dedicated viewport panel type
			- thread renderer output handles and viewport metadata into editor UI
			- integrate the viewport panel into the existing editor layout next to the current menu/outliner/inspector panels
		- Validation:
			- the editor has an explicit viewport panel abstraction rather than an implicit overlay-only presentation model
	- Prompt 3.3 — Let `EditorApp` own viewport presentation and editor draw sequencing.
		- Files to touch:
			- `Engine/Application/Public/EditorApp.h`
			- `Engine/Application/Private/EditorApp.cpp`
			- `Engine/Editor/Private/UI.cpp`
			- any narrow helper needed for editor draw submission
		- Concrete actions:
			- have `EditorApp` request runtime outputs each frame and feed them into `UI`
			- keep the current ImGui backend in editor/tool code for now if necessary, but do not route it through Renderer ownership
			- make editor draw/update sequencing visible in the host, not hidden in renderer callbacks
		- Validation:
			- the editor viewport renders correctly from renderer-produced outputs
			- no renderer presentation pass directly calls editor UI render functions
	- Must not do:
		- do not let `UI` become a place where generic rendering is reimplemented
		- do not let renderer public headers gain editor-facing viewport panel concepts
	- Completion gate:
		- the editor viewport renders correctly from renderer-produced outputs
		- editor panels and viewport chrome are fully editor-owned
		- no renderer presentation pass directly calls editor UI render functions

5. Phase 4 — Enforce the Renderer/RHI backend boundary.
	- Goal:
		- make renderer logic cleanly graphics-API agnostic so D3D12 and Vulkan remain viable targets
	- Current repo state to account for:
		- `Renderer.h` still names concrete D3D12 classes such as `D3D12Rhi`, `D3D12SwapChain`, and `D3D12DescriptorHeapManager`
		- `Renderer.cpp` still constructs D3D12 concrete systems directly
		- `Engine/Renderer/CMakeLists.txt` still includes `Engine/third_party` for `d3dx12.h`
		- `Engine/RHI/CMakeLists.txt` still privately includes `Renderer/Public` and `GameFramework/Public`
		- `UI.cpp` still includes D3D12-specific types for editor draw submission
	- Suggested implementation prompt count:
		- 4 prompts
	- Prompt 4.1 — Introduce renderer-facing, API-agnostic RHI abstractions for the orchestration layer.
		- Files to touch:
			- `Engine/Renderer/Public/Renderer.h`
			- new `Engine/RHI/Public/*` abstraction headers as needed
			- any renderer-private helper headers that currently expose D3D12 types unnecessarily
		- Concrete actions:
			- define API-agnostic interfaces or service objects for the parts of RHI that renderer orchestration must touch
			- keep the abstraction surface focused on textures, buffers, descriptors, queues, command contexts, swapchain/present, and similar backend-neutral concepts
		- Validation:
			- renderer orchestration has a credible path to stop naming D3D12 types directly
	- Prompt 4.2 — Remove direct D3D12 construction and ownership from renderer orchestration code.
		- Files to touch:
			- `Engine/Renderer/Private/Renderer.cpp`
			- `Engine/Renderer/Public/Renderer.h`
			- relevant RHI implementation and factory files
		- Concrete actions:
			- move creation of backend-specific objects behind RHI factories, adapters, or backend-owned bootstrap code
			- reduce renderer ownership of D3D12 concrete objects in favor of RHI-facing abstractions
		- Validation:
			- `Renderer.cpp` is materially less D3D12-aware than it is today
	- Prompt 4.3 — Clean include and shared-type direction between RHI, Renderer, and GameFramework.
		- Files to touch:
			- `Engine/RHI/CMakeLists.txt`
			- `Engine/Renderer/CMakeLists.txt`
			- any shared headers such as depth convention or asset/path types that currently force upward includes
		- Concrete actions:
			- move shared non-backend types to neutral modules if they are genuinely shared
			- remove upward include leakage where RHI depends on renderer/gameframework headers for basic type access
		- Validation:
			- target include direction matches the intended architecture more closely
	- Prompt 4.4 — Decide the transitional home for editor backend glue without re-polluting Renderer.
		- Files to touch:
			- `Engine/Editor/Public/UI.h`
			- `Engine/Editor/Private/UI.cpp`
			- optionally a narrow RHI helper if needed
		- Concrete actions:
			- either keep ImGui DX12 backend glue explicitly tool-local for now or move only the minimal backend helper surface into RHI
			- do not expose editor backend glue through `SparkleRenderer`
		- Validation:
			- editor draw submission has a clear transitional home that does not violate renderer ownership rules
	- Must not do:
		- do not let RHI grow upward into scene semantics, editor concepts, or gameplay ownership
		- do not hardcode renderer logic to one backend while claiming multi-backend support
	- Completion gate:
		- renderer code is substantially API-agnostic
		- backend-specific work is localized to RHI/backend layers
		- Vulkan support remains architecturally plausible without rewriting renderer systems

6. Phase 5 — Reassert runtime-only versus tool-only boundaries.
	- Goal:
		- keep runtime cooked-only and keep authoring/cooking out of runtime modules
	- Current repo state to account for:
		- runtime grep checks are currently clean for `SceneImporter`, `SceneImportResult`, `cgltf`, and `assimp` in `Engine/Application`, `Engine/GameFramework`, and `Engine/Renderer`
		- `Engine/Editor/CMakeLists.txt` still publicly links `SparkleAssetAuthoring`
		- `Tools/AssetConverter/CMakeLists.txt` still defines both `SparkleAssetAuthoring` and `AssetConverter`
	- Suggested implementation prompt count:
		- 3 prompts
	- Prompt 5.1 — Codify and preserve the cooked-runtime-only boundary in runtime modules.
		- Files to touch:
			- runtime CMake files or validation scripts if needed
			- any runtime files that still carry stale comments implying source import access
		- Concrete actions:
			- add or update checks, comments, or validation helpers that preserve the already-clean runtime boundary
			- keep runtime modules free of authoring imports as later phases land
		- Validation:
			- runtime modules remain free of source import/cook dependencies
	- Prompt 5.2 — Remove `SparkleEditor`'s direct dependence on `SparkleAssetAuthoring`.
		- Files to touch:
			- `Engine/Editor/CMakeLists.txt`
			- any editor-side files that currently rely on authoring code directly
		- Concrete actions:
			- replace the direct broad authoring-module dependency with the intended tool boundary approach
			- if a narrow tool-client interface is necessary, keep it small and clearly tool-scoped
		- Validation:
			- editor no longer depends on a broad standalone authoring engine module by default
	- Prompt 5.3 — Restructure the `AssetConverter` boundary so the tool is the visible seam.
		- Files to touch:
			- `Tools/AssetConverter/CMakeLists.txt`
			- any public headers under `Tools/AssetConverter/Public`
			- any integration code that assumes `SparkleAssetAuthoring` is the long-term reusable module
		- Concrete actions:
			- internalize, narrow, or rename `SparkleAssetAuthoring` so `AssetConverter` is the real visible boundary
			- keep source import, conversion, and cooking tool-scoped
		- Validation:
			- tool-side authoring code is clearly separated from runtime and no longer presented as a broad engine-facing dependency by default
	- Must not do:
		- do not let runtime code call source importers, cookers, or editor authoring flows
		- do not let tool code become a hidden runtime dependency through convenience links
	- Completion gate:
		- runtime consumes cooked data only
		- authoring/import/cook workflows are clearly tool-scoped
		- the tool boundary is explicit in modules and targets

7. Phase 6 — Make launch targets and dependency direction explicit.
	- Goal:
		- make the final architecture visible in targets, public APIs, and dependency graphs
	- Current repo state to account for:
		- `Projects/Showcase/CMakeLists.txt` and `Projects/TemplateProject/CMakeLists.txt` still define a single executable linked to `SparkleApplication` and `SparkleEditor`
		- project `main.cpp` files still instantiate the old overlay-driven app path
		- `SparkleApplication`, `SparkleEditor`, `SparkleRenderer`, and `SparkleRHI` CMake files still reflect transitional dependency assumptions
	- Suggested implementation prompt count:
		- 3 prompts
	- Prompt 6.1 — Split Showcase into explicit editor and runtime launch targets.
		- Files to touch:
			- `Projects/Showcase/CMakeLists.txt`
			- `Projects/Showcase/Src/main.cpp`
			- new runtime-only and editor-only entry files as needed
		- Concrete actions:
			- add explicit editor-enabled and runtime-only Showcase entrypoints
			- make the editor target use `EditorApp` and the runtime target use `ProjectApp`
		- Validation:
			- Showcase target structure communicates the split directly
	- Prompt 6.2 — Mirror the explicit split in `TemplateProject`.
		- Files to touch:
			- `Projects/TemplateProject/CMakeLists.txt`
			- `Projects/TemplateProject/Src/main.cpp`
			- new template entry files as needed
		- Concrete actions:
			- make the generated/default project template reflect the final architecture instead of the transitional overlay path
		- Validation:
			- new projects start from the intended host split instead of inheriting obsolete seams
	- Prompt 6.3 — Audit engine target links and public includes to match the final direction of dependencies.
		- Files to touch:
			- `Engine/Application/CMakeLists.txt`
			- `Engine/Renderer/CMakeLists.txt`
			- `Engine/Editor/CMakeLists.txt`
			- `Engine/RHI/CMakeLists.txt`
			- `Tools/AssetConverter/CMakeLists.txt`
		- Concrete actions:
			- remove transitional links and includes that contradict the target architecture
			- ensure editor targets depend downward on runtime/renderer APIs rather than the reverse
			- ensure renderer depends on RHI, not D3D12 directly
		- Validation:
			- target dependency direction matches the intended module inventory and responsibility matrix
	- Must not do:
		- do not leave old overlay or mixed-host abstractions in public API once the replacement path is working
	- Completion gate:
		- target structure clearly communicates runtime versus editor paths
		- public APIs no longer expose transitional editor/render coupling
		- dependency direction matches the intended module inventory and responsibility matrix

8. Phase 7 — Final cleanup, validation, and prompt-ready closeout.
	- Goal:
		- leave a clean architectural base that future implementation prompts can extend without ambiguity
	- Closeout status:
		- Prompt 7.1 completed: removed the obsolete `App` compatibility layer, removed the empty renderer `Public/Overlays` seam, and verified the old overlay path is absent from live engine/project sources.
		- Prompt 7.2 completed: ran final validation passes and updated prompt/memory state to match the implemented repository.
	- Current repo state after closeout:
		- `Engine/Application/Public` now exposes `ApplicationAPI.h`, `ProjectApp.h`, and `EditorApp.h`.
		- `Projects/Showcase` and `Projects/TemplateProject` use explicit `EditorMain.cpp` and `RuntimeMain.cpp` entrypoints plus split editor/runtime targets.
		- `runtime_cooked_boundary_check` passes for `Engine/Application`, `Engine/GameFramework`, and `Engine/Renderer`.
		- `Engine/Renderer/Public/**` and the renderer-facing target graph no longer carry direct D3D12 linkage, while private renderer implementation still contains backend-specific debt that can be addressed as a discrete follow-up.
	- Validation summary:
		- `cmake --build build --config Debug --target SparkleApplication SparkleRenderer SparkleEditor ShowcaseEditor ShowcaseRuntime` completed successfully after reconfigure.
		- `cmake --build build --config Debug --target runtime_cooked_boundary_check` passed.
		- `ShowcaseRuntime.exe` and `ShowcaseEditor.exe` both launched successfully from `Projects/Showcase` in smoke tests and stayed alive until terminated.
		- Live engine/project source contains no `IRendererOverlay`, `RendererOverlayFactory`, `CreateEditorOverlay`, or `AddUiCompositionPass` references.
		- Manual on-screen inspection remains the last mile for viewport pixel/layout confirmation, but the editor host/UI/render path now boots cleanly.
	- Completion gate:
		- satisfied for the host split, launch split, public/runtime boundary cleanup, and cooked-runtime validation
		- future prompts can now target isolated follow-up work such as deeper private renderer backend cleanup, viewport polish, or editor tooling improvements without reopening base ownership decisions

**Prompt Authoring Guidance For Future Implementation Prompts**
- Prefer one phase per implementation prompt unless a tiny follow-up is needed to finish a coherent boundary.
- Each implementation prompt should state:
	- the exact phase being executed
	- the concrete files expected to change
	- what architectural rule must not be violated
	- the completion gate that proves the phase is done
- Do not combine host split, renderer/RHI cleanup, viewport contract design, and tool-boundary cleanup into one implementation prompt unless the change is genuinely trivial.

**Relevant files**
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Public\ProjectApp.h` — explicit runtime host public API.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Private\ProjectApp.cpp` — runtime host loop, subsystem creation, and editorless launch behavior.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Public\EditorApp.h` — explicit editor host public API.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Private\EditorApp.cpp` — editor host sequencing, viewport request handoff, and editor present path.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\Viewport\ViewportContracts.h` — generic viewport request/output contract used by runtime and editor hosts.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\Renderer.h` — runtime-oriented renderer surface with no overlay seam in public API.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Renderer.cpp` — runtime render execution and host-frame orchestration.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Builder\FrameGraphBuilder.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Builder\FrameGraphBuilder.cpp` — framegraph dependencies after overlay removal.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Features\PresentationPasses.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Features\PresentationPasses.cpp` — generic runtime presentation and scene-target setup.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Public\UI.h` — editor-owned UI surface with no renderer-owned overlay inheritance.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\UI.cpp` — editor-owned ImGui/backend submission and viewport presentation.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Public\Panels\ViewportPanel.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\Panels\ViewportPanel.cpp` — explicit editor viewport panel abstraction.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\Showcase\CMakeLists.txt`, `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\Showcase\Src\EditorMain.cpp`, and `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\Showcase\Src\RuntimeMain.cpp` — explicit sample launch targets and entrypoints.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\TemplateProject\CMakeLists.txt`, `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\TemplateProject\Src\EditorMain.cpp`, and `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\TemplateProject\Src\RuntimeMain.cpp` — template launch split for future projects.

**Final Validation Results**
1. Renderer public/API-facing validation passed: there are no direct `D3D12`, `d3d12`, `dx12`, or `D3DX12` matches under `Engine/Renderer/Public/**`, and the renderer/application/editor CMake files do not link D3D12 system libraries directly.
2. Runtime host validation passed: `Engine/Application/Public` now exposes only `ApplicationAPI.h`, `ProjectApp.h`, and `EditorApp.h`; the old `App` compatibility wrapper has been removed.
3. Editor host validation passed: `EditorApp.cpp` owns `UI` lifetime, viewport product handoff, and editor present-pass sequencing above `Renderer`.
4. Editor launch smoke test passed from `Projects/Showcase`: `ShowcaseEditor.exe` started successfully from the project working directory and stayed alive until explicitly terminated.
5. Runtime launch smoke test passed from `Projects/Showcase`: `ShowcaseRuntime.exe` started successfully from the project working directory and stayed alive until explicitly terminated.
6. Cooked-runtime boundary validation passed: `runtime_cooked_boundary_check` succeeded for `Engine/Application`, `Engine/GameFramework`, and `Engine/Renderer`.
7. Overlay seam removal validation passed: live `Engine/**` and `Projects/**` sources contain no `IRendererOverlay`, `RendererOverlayFactory`, `CreateEditorOverlay`, or `AddUiCompositionPass` references.
8. Residual risk is explicit rather than ambiguous: private renderer implementation still contains backend-specific D3D12 debt under `Engine/Renderer/Private`, but that work is now isolated from host ownership and overlay architecture.

**Decisions**
- Preferred target: explicit `EditorApp` over explicit editorless `ProjectApp`, matching the Unreal-style editor-versus-game split more directly than a generic host abstraction.
- `ProjectApp` is the runtime/game application layer and should stay editorless by construction.
- `EditorApp` is the only layer allowed to own editor UI, editor frame policy, and editor-facing viewport/tooling requests.
- The game viewport is editor-owned chrome around a runtime-rendered scene surface; it is not a renderer-owned overlay.
- `Renderer` owns generic rendering and composition execution, while `EditorApp` only requests which editor-facing views, surfaces, and optional features are required.
- Acceptable migration seam: a generic runtime viewport contract carrying view, surface, and output requirements plus returned render products, not a renderer-defined editor interface or context bag.
- `UI` may still depend on runtime services like `LevelManager`, `GameScene`, `Timer`, and `Window`, but that dependency must be owned by `EditorApp`, not by Renderer public API or `ProjectApp`.
- Do not reintroduce Renderer -> Editor or `ProjectApp` -> Editor hard links below the editor target boundary.
- Prefer generic renderer capabilities first, then an editor-render bridge module above Renderer if needed, and only then isolated `#if WITH_EDITOR` hooks as a last resort.
- Preferred tool boundary: `AssetConverter` should remain the explicit separate asset-processing module/tool. Authoring and conversion code should not stay exposed as a first-class standalone engine module in the long-term target architecture unless a narrower, clearly justified boundary emerges.

**Further Considerations**
1. The highest-value remaining renderer cleanup is deeper backend-neutralization inside `Engine/Renderer/Private`, especially private D3D12 utility, pipeline, and resource code.
2. If executable-level editor/runtime separation later needs to become module-level separation as well, `SparkleApplication` can be split further so `EditorApp` lives in an editor-only target rather than sharing the host module.
3. Viewport UX, multi-viewport support, picking, preview rendering, and editor tooling can now be pursued as isolated follow-up prompts on top of the established host and viewport contracts.
