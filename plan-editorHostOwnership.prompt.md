## Plan: Explicit EditorApp And ProjectApp

Replace the renderer-owned overlay abstraction with an explicit Unreal-style split: an editorless `ProjectApp` as the runtime/game application layer, and a separate `EditorApp` above it that owns editor UI, editor frame policy, and editor-specific launch behavior. Renderer should go back to being purely runtime-facing. Editor code should stop being injected downward through Renderer or through `App` constructor plumbing. The architecture should read clearly as `ProjectApp` for game/runtime and `EditorApp` for tools/editor.

**Target Shape**
- `ProjectApp`: editorless runtime application. Owns timer, window, input, game scene, scene asset manager, level manager, camera controller, and renderer.
- `EditorApp`: higher-level tools host. Owns `ProjectApp`, owns `UI`, controls editor frame ordering, and specifies editor viewport and tooling requirements using generic renderer capabilities.
- `Renderer`: runtime-only renderer. Knows nothing about editor UI types, editor factories, or editor service bags.
- Project executables: explicit launch choice. A project can have an editor entry point using `EditorApp` and a runtime/game entry point using `ProjectApp`.

**Viewport Boundary**
- The editor still has a game viewport, but that viewport is editor-owned UI, not a renderer-owned overlay.
- `ProjectApp` owns runtime world state and runtime render submission. It may expose a narrow runtime-facing viewport contract, but it must not know about ImGui, docking, panels, or editor chrome.
- `Renderer` should render scene output into a runtime-owned viewport surface when hosted by `EditorApp`, rather than assuming the scene always renders directly to the swapchain backbuffer.
- `EditorApp` owns the viewport panel, decides its size and active view, requests the corresponding runtime scene output from `ProjectApp`, and embeds that output into editor UI.
- The boundary should stay narrow: viewport dimensions, active runtime camera or view selection, and a runtime-rendered texture handle or view descriptor that editor UI can display. Do not pass editor widgets or editor service bags through Renderer APIs.
- Final composition policy belongs to `EditorApp`, while composition execution should stay in generic renderer or render-backend paths: runtime scene output is shown inside the editor viewport panel, editor panels are drawn by editor UI, and renderer services execute the necessary rendering work without becoming editor-specific.
- In the editor path, the window backbuffer is editor host output. In the runtime-only path, `ProjectApp` can still present directly with no editor viewport indirection.

**Design Principles**
- Separate policy from mechanism. `EditorApp` owns editor policy and requirements. `Renderer` owns generic rendering mechanism and execution. `RHI` owns GPU/backend mechanics.
- Prefer generic renderer capabilities over `#if WITH_EDITOR`. Treat compile-time editor conditionals inside Renderer as a last resort for tiny unavoidable hooks, not as the default extension model.
- Express editor needs as renderer-facing data, not editor-facing interfaces. Examples: view requests, viewport surface descriptors, optional auxiliary outputs, composition plans, and debug flags.
- Keep runtime app types editorless by construction. `ProjectApp` should remain valid for a shipping game executable, test harness, or headless runtime host without editor code present.
- Keep the editor above runtime. `EditorApp` may depend on runtime systems and renderer services, but runtime modules should not depend on editor modules.
- Build for future multi-viewport and tool rendering now. The design should naturally support game view, scene view, asset preview, thumbnails, picking, and debug visualization without a second refactor.

**Policy Vs Mechanism**
- `EditorApp` policy:
	- which viewports exist
	- what each viewport is showing
	- viewport layout and sizing
	- which editor tools are active
	- whether a view is game, scene, preview, thumbnail, or debug-oriented
- `Renderer` mechanism:
	- allocate or resize render surfaces
	- build scene views from generic view requests
	- execute passes and composition plans
	- manage view families, render outputs, and present paths
	- translate requests into command recording through `SparkleRHI`
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
	- Backend rendering abstraction layer.
	- Owns device resources, swapchains, descriptor heaps, command contexts, shader/backend integration, and low-level render resource lifetime.
	- Should expose GPU/resource primitives used by Renderer and possibly UI backends, but not editor policy.
- `SparkleRenderer`
	- Generic rendering layer.
	- Owns scene view rendering, framegraph assembly, render surface management, view families, generic composition, debug visualization primitives, and present execution.
	- Must not know about editor panels, editor lifetime, or editor-owned interfaces.
- `SparkleGameFramework`
	- Runtime world and asset layer.
	- Owns levels, game scene state, runtime camera state, cooked scene asset resolution, scene asset manager, and runtime-facing simulation state.
	- Must remain free of editor UI and source import workflows.
- `SparkleApplication`
	- Runtime host layer today.
	- Best candidate to evolve into or host `ProjectApp`.
	- Owns runtime loop orchestration across Platform, GameFramework, and Renderer.
- `SparkleEditor`
	- Editor tools layer.
	- Owns panels, viewport widgets, menuing, selection, inspectors, editor commands, editor camera behavior, asset tools, and tool-mode policy.
	- Should consume generic renderer capabilities rather than being consumed by Renderer.
- `SparkleAssetAuthoring`
	- Offline conversion and authoring support layer.
	- Owns source import, conversion, cooking, and editor-only content authoring workflows.
	- Must remain outside runtime load paths.
- `AssetConverter`
	- Offline executable host for authoring and cooking workflows.

**Responsibility Matrix**
- `EditorApp`
	- owns editor frame policy
	- owns viewport layout and which views are needed
	- owns selection, tool mode, editor camera mode, and editor commands
	- asks for runtime render outputs using generic renderer-facing requests
	- does not own low-level command recording, barriers, descriptor management, or swapchain transitions
- `ProjectApp`
	- owns runtime frame orchestration, runtime state, runtime camera selection, level flow, simulation update, and runtime-facing render requests
	- may translate runtime world state into renderer-facing scene/view requests
	- does not own editor panels, docking, or editor widget lifetime
- `Renderer`
	- owns generic rendering capabilities and execution
	- owns render surfaces, view families, render products, scene output bundles, and generic composition execution
	- may support editor use cases through generic features such as auxiliary outputs, picking buffers, and debug overlays
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

**Preferred Container Model**
```text
Project Editor Executable
		|
		v
EditorApp
	- editor policy
	- frame ordering
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
	- runtime provides world and scene state
	- renderer executes generic rendering work
	- UI presents renderer outputs
- A game viewport inside the editor is not special because of where it appears on screen. It is special because the editor chooses to host a runtime-produced view inside editor chrome.
- That means the viewport is editor-owned as UX, runtime-owned as scene source, and renderer-owned as rendering execution.

**Steps**
1. Phase 1 — Lock the ownership rule. Treat Renderer as runtime-only again: no `IRendererOverlay`, `RendererOverlayContext`, or `RendererOverlayFactory` in Renderer public headers, and no editor lifetime management inside Renderer. This decision blocks the rest of the refactor.
2. Rename and clarify the runtime app layer. Split the current `App` concept into an explicit editorless `ProjectApp` type so the runtime path is named clearly and can stand on its own without editor concerns. Depends on 1.
3. Add an explicit `EditorApp` above `ProjectApp`, following the Unreal-style shape where editor code hosts the runtime/game app instead of being injected into it. `EditorApp` owns editor lifetime and composes editor behavior on top of the runtime app. Depends on 2.
4. Refactor the current application code so `ProjectApp` owns only runtime systems: timer, window, input, game scene, scene asset manager, level manager, camera controller, and renderer. Remove all editor-factory concerns from that layer. Depends on 3.
5. Move frame ownership to `EditorApp`. The editor host should explicitly control order of operations: window/input pump, runtime simulation/update, viewport sizing and view selection, render-request submission, editor UI update, and final present orchestration. Renderer remains responsible for generic composition execution and present-path work. Depends on 4.
6. Introduce a runtime-facing viewport surface contract between `EditorApp`, `ProjectApp`, and `Renderer`. This contract should describe only what runtime needs to render and what editor needs to display: dimensions, active runtime view, requested outputs, and returned scene output handles. It must not be a renderer-defined editor overlay interface or a bag of editor services. Depends on 4 and 5.
7. Replace the current overlay seam with renderer-executed generic composition plus editor-hosted viewport presentation. `EditorApp` should own viewport requirements and pass runtime scene surfaces into `UI`; Renderer should no longer call editor UI render code from a presentation pass. Depends on 6.
8. Update `UI` to become a pure editor-owned object again. Remove the renderer-owned interface implementation and delete `CreateEditorOverlay`; let `EditorApp` construct `UI` directly and give it the runtime/editor services it genuinely needs, including viewport presentation data. Depends on 7.
9. Make project launch targets explicit. Update Showcase and TemplateProject to use an editor entry point built around `EditorApp`, and introduce or plan a separate editorless `ProjectApp` entry point so the distinction is visible in code and target structure. Depends on 8.
10. Audit module and target boundaries after the split. Confirm Renderer and its framegraph plumbing no longer mention editor abstractions, `ProjectApp` no longer carries overlay plumbing, and `SparkleEditor` stays above runtime modules. Depends on 9.
11. Cleanup and documentation. Remove `RendererOverlay.h` and related plumbing only after the new `EditorApp` plus `ProjectApp` path compiles and the in-editor viewport plus other editor panels still render correctly. Update docs and repo memory to reflect the explicit two-app model and viewport boundary. Depends on 10.

**Relevant files**
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Public\App.h` — split or rename this runtime app type into `ProjectApp`, removing editor-factory constructor shape.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Private\App.cpp` — restore editor-free runtime initialization and convert it toward `ProjectApp` implementation.
- New runtime app files such as `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Public\ProjectApp.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Private\ProjectApp.cpp` if you choose a hard rename instead of evolving `App` in place.
- New editor host files such as `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Public\EditorApp.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Application\Private\EditorApp.cpp`, or editor-owned equivalents if you decide `EditorApp` should live under Editor.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\Overlays\RendererOverlay.h` — delete after replacing it with an app-level composition seam.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\Renderer.h` — restore a runtime-oriented constructor and remove editor overlay abstractions.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Renderer.cpp` — remove overlay creation/update ownership and keep only runtime render work.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Builder\FrameGraphBuilder.h` — remove editor overlay references from framegraph dependencies.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Builder\FrameGraphBuilder.cpp` — replace explicit UI composition wiring with runtime viewport output creation.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Features\PresentationPasses.h` — remove renderer-owned editor interface references and separate runtime scene output from swapchain presentation concerns.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Features\PresentationPasses.cpp` — keep only generic runtime presentation and viewport-surface behavior.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Public\UI.h` — stop implementing the renderer-owned overlay interface.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\UI.cpp` — delete `CreateEditorOverlay`, move construction assumptions to `EditorApp`, and host the runtime scene texture inside an explicit viewport panel.
- New editor viewport panel files such as `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Public\Panels\ViewportPanel.h` and `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\Panels\ViewportPanel.cpp` if the current monolithic `UI` needs a dedicated viewport surface owner.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\Showcase\CMakeLists.txt` — make editor and runtime launch targets explicit at the project boundary.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\Showcase\Src\main.cpp` — construct `EditorApp` explicitly instead of routing editor UI through `App`.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\TemplateProject\CMakeLists.txt` — mirror Showcase target split.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Projects\TemplateProject\Src\main.cpp` — mirror Showcase entry-point split.

**Verification**
1. Build the editor target and confirm Renderer public compilation no longer depends on editor headers or editor-owned public interfaces.
2. Confirm the runtime app layer is explicit and editorless: `ProjectApp` can be constructed without editor-specific factories, interfaces, or service bags.
3. Confirm `EditorApp` owns editor UI lifetime, viewport layout, and frame ordering rather than Renderer.
4. Launch the editor-enabled Showcase path from the project directory and verify the in-editor game viewport renders the runtime scene inside editor UI rather than as a renderer-owned overlay on the backbuffer.
5. Verify viewport resize behavior: resizing the editor viewport should resize the runtime scene surface without forcing editor concepts into Renderer public API.
6. Build or stub the separate editorless project launch path and confirm it depends only on runtime modules plus `ProjectApp`.
7. Re-run the cooked-scene-boundary audit and confirm Application, GameFramework, and Renderer still contain no `SceneImporter`, `SceneImportResult`, `cgltf`, or `assimp` references.
8. Confirm `SparkleEditor` stays above runtime modules: linked by the editor executable or `EditorApp`, not by Renderer, GameFramework, or `ProjectApp`.

**Decisions**
- Preferred target: explicit `EditorApp` over explicit editorless `ProjectApp`, matching the Unreal-style editor-versus-game split more directly than a generic host abstraction.
- `ProjectApp` is the runtime/game application layer and should stay editorless by construction.
- `EditorApp` is the only layer allowed to own editor UI, editor frame policy, and editor-facing viewport/tooling requirements.
- The game viewport is editor-owned chrome around a runtime-rendered scene surface; it is not a renderer-owned overlay.
- `Renderer` owns generic rendering and composition execution, while `EditorApp` owns the policy for which editor-facing views and surfaces are required.
- Acceptable migration seam: a generic runtime viewport contract carrying view, surface, and output requirements plus returned render products, not a renderer-defined editor interface or context bag.
- `UI` may still depend on runtime services like `LevelManager`, `GameScene`, `Timer`, and `Window`, but that dependency must be owned by `EditorApp`, not by Renderer public API or `ProjectApp`.
- Do not reintroduce Renderer -> Editor or `ProjectApp` -> Editor hard links below the editor target boundary.
- Prefer generic renderer capabilities first, then an editor-render bridge module above Renderer if needed, and only then isolated `#if WITH_EDITOR` hooks as a last resort.

**Further Considerations**
1. Recommendation: do the hard type split early. An explicit `ProjectApp` plus `EditorApp` will be clearer than leaving the current `App` name in place and retrofitting meaning around it.
2. Recommendation: if a full target split is too much for one patch, first land the code-level split (`ProjectApp` and `EditorApp` types) and then add separate project targets in a follow-up change.
3. Recommendation: keep `EditorApp` close to Application at first if that lowers refactor cost, then revisit whether it should move under Editor once the ownership model is stable.
