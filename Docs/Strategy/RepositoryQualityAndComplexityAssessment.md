# Repository Quality and Complexity Executive Assessment

Status: dated strategy assessment and refactoring decision brief

Snapshot date: 2026-08-23

Source revision: `1a70a64c` (`shader & rt doc merge`); engine and tool source remains the `44c2f192` code snapshot assessed below.

## Responsibility And Authority Boundary

This document answers one executive question: **how should SparkleEngine preserve its current capability set while becoming materially thinner, easier to reason about, and more credible as a production graphics-engineering repository?** It assesses current structure and prioritizes refactoring. It does not redefine requirements, architecture, engineering rules, workload gates, or the roadmap.

- [Principal Graphics Engineering Requirements](Requirements.md) owns the capability and evidence target.
- [Advanced Graphics Engine Executive Summary](ExecutiveSummary.md) owns the product identity.
- [Advanced Graphics Engineer Persona](EngineerPersona.md) owns the expected engineering behavior.
- [Candidate and Repository Gap Assessment](GapAssessment.md) owns the dated readiness assessment at its stated revision.
- [Principal Graphics Roadmap](Roadmap.md) owns feature and evidence sequence.
- [Whole Repository Architecture Map](../Architecture/WholeRepositoryMap.md) and focused architecture documents own accepted system boundaries.
- [Engineering Standards](../Engineering/Standards/README.md) own implementation and review rules.
- Code, CMake, cooked artifacts, runtime behavior, captures, and measurements remain proof of what is implemented.

The source scan used committed code at the revision above. Pre-existing or concurrent uncommitted edits under `Docs/Architecture/Shaders` were treated as provisional context, not implementation proof, and were not modified by this assessment.

## Executive Decision

SparkleEngine should **preserve its current feature set and reduce the amount of structure required to carry it**. The repository already contains a serious graphics core: explicit D3D12 and Vulkan backends, a managed frame graph, persistent scene and view preparation, raster and ray-traced paths, reference and ReSTIR lighting, shader cooking, content import and cooking, task execution, editor workflows, and a launcher. The problem is not that those capabilities exist. The problem is that several of them are presented through more public surface, more coordination layers, more repeated metadata, and more navigation hops than their current ownership model requires.

The target is not a smaller demo. It is the same engine expressed as a compact, reproducible set of complete vertical slices:

1. one owner for mutable state and policy;
2. one production path for each artifact or runtime input;
3. public contracts only at real module or host boundaries;
4. private code organized by capability and change locality;
5. explicit D3D12/Vulkan behavior without a second convenience RHI;
6. bounded work, memory, queues, and snapshots;
7. external captures and workload evidence instead of a growing internal diagnostics product;
8. documentation that routes to code that exists and behavior that is proven.

The refactor must remain subordinate to the roadmap. It must not become a parallel multi-month rewrite, a new framework, or an excuse to defer Bistro, San Miguel, paired-backend evidence, neural work, or adoption material. Each structural change should travel with the capability or evidence gate it makes easier to deliver.

## What The Repository Should Communicate

The desired values are behavioral. Naming, folders, diagrams, and clean screenshots are not sufficient if the underlying path is still duplicated or difficult to change.

| Value | Production behavior that proves it | Failure signal to eliminate |
| --- | --- | --- |
| Depth over feature count | A few end-to-end graphics workloads are correct, measurable, reproducible, and transferable. | More panels, providers, effects, formats, or reports without stronger evidence for the existing path. |
| Additive capability, reductive structure | A feature adds the minimum necessary owner and deletes the scaffolding or path it replaces. | Every change leaves behind an adapter, alias, second representation, compatibility branch, or permanent diagnostic. |
| Direct ownership | State has one mutable authority; producers publish through a named seam; consumers receive focused inputs. | Orchestrators, facades, callbacks, and snapshots become alternate authorities. |
| Narrow contracts | Public headers are smaller than private implementation and have verified external consumers. | Private implementation types live under `Public`, or consumers compile only because they explicitly link a hidden dependency. |
| Honest backend support | D3D12 and Vulkan use the same engine contract while keeping native lifetime and capability policy explicit. | A lowest-common-denominator abstraction, backend-specific behavior hidden behind generic names, or parity claimed without paired evidence. |
| Deterministic artifacts | Import, cook, shader compilation, loading, and capture have explicit identity and publication boundaries. | Cache/version machinery, repeated path discovery, or multiple catalogs decide what the same artifact means. |
| Evidence before instrumentation | Stable workloads and external tools provide the primary performance and correctness record. | Logs, dashboards, statistics structs, or profiling hooks grow faster than reproducible captures and measurements. |
| Change locality | A reviewer can follow a capability through a small, cohesive set of owners and verify its invariant. | A simple change touches dozens of thin files, several registries, or one god object split across many translation units. |
| Adoption quality | A graphics engineer can build, run, inspect, and explain a vertical slice without repository archaeology. | The architecture looks complete in prose while entry points, links, target dependencies, or failure contracts are stale. |

This is the repository-level interpretation of `PGE-07`, `PGE-09`, `PGE-13`, and especially `PGE-15`: the strongest result is advanced graphics work that makes the engine easier to understand and adopt.

## Feature Preservation Boundary

No recommendation in this assessment authorizes removal of a user-visible or graphics capability. The following remain in scope and must survive refactoring:

- D3D12 and Vulkan backends, capability reporting, debug support, presentation, and native lifetime correctness;
- rasterized and ray-traced GBuffer paths;
- reference path-traced lighting and ReSTIR direct/indirect lighting;
- ray queries and native ray-tracing pipeline execution where currently implemented;
- frame-graph declaration, compilation, barriers, transient planning, recording, and execution;
- persistent render scene, view, frame, GPU-scene, resource, texture, mesh, and material behavior;
- shader registration, DXC and Slang compilation, typed parameter validation, cooked packages, runtime loading, reload, and inspection;
- source import for the formats currently supported, canonical coordinate conversion, cooking, deterministic asset identity, and cooked-only runtime loading;
- the one shared task runtime, serial/threaded modes, and existing deterministic execution semantics;
- the editor viewport, scene inspection/editing, rendering settings, asset inspection, console, capture, and essential diagnostics workflows;
- launcher sync, build, cook, clean, quick-start, level, dependency, maintenance, preview, cancellation, and activity workflows;
- Showcase runtime/editor hosts, fallback behavior, and the Sponza, Bistro, and San Miguel evidence route;
- the current source assets and optional research entries, even when they are not part of the default acceptance path.

What may be removed is **structural residue**: unused public exposure, duplicate metadata, redundant snapshots, forwarding-only wrappers, one-consumer generic machinery, dead flags, stale documents, and superseded paths. Such removal is valid only after current producers and consumers are traced and feature equivalence is demonstrated.

## Current Repository Shape

### Directional source inventory

The following inventory counts owned `.c`, `.cpp`, `.h`, `.hlsl`, and selected CMake source in the principal repository areas. It excludes third-party implementation and generated outputs. It is a locator for review effort, not a quality score or line budget.

| Area | Files | Approximate lines | Assessment use |
| --- | ---: | ---: | --- |
| `Engine/Renderer` | 587 | 37,293 | Largest high-level capability surface; both real graphics depth and excessive private fragmentation are present. |
| `Engine/RHI` | 291 | 33,595 | Dense backend implementation; most complexity is capability-bearing, but public/service and backend ownership need focused audits. |
| `Tools/Launcher` | 163 | 19,267 | Disproportionate to roadmap priority; typed operations are valuable, while UI and metadata coordination are overextended. |
| `Engine/GameFramework` | 272 | 13,779 | Compact in lines but fragmented; its fixed system topology carries more generic scheduling machinery than current use requires. |
| `Engine/Editor` | 97 | 9,211 | Essential workflows exist, but a broad public `UI` contract and panel/diagnostic surface enlarge integration cost. |
| `Tools/Shaders` | 112 | 7,517 | Important deterministic boundary; recent cache removal is directionally correct. |
| `Tools/Cooking` | 126 | 7,029 | Real vertical-slice work; multiple plans, dispatch layers, subprocesses, and reports should remain one deterministic publication path. |
| `Engine/Core` | 109 | 6,915 | Small foundation with several true shared contracts; project/content policy and broad utilities risk becoming a catch-all. |
| `Engine/Assets` | 117 | 6,138 | Primarily shader and source asset content, not an engine module; protect it as capability-bearing product input. |
| `Tools/Import` | 88 | 5,687 | Format-specific complexity is justified; normalization and imported-domain contracts should remain singular. |
| `Engine/Tasks` | 37 | 2,854 | Appropriately compact shared execution substrate; freeze its scope rather than build another scheduler. |
| `Engine/Application` | 46 | 2,843 | Small host module, but public accessors and editor/tool coordination leak more internals than its role requires. |
| `Engine/Platform` | 12 | 1,711 | Small and focused; retain as a thin platform owner. |
| CMake/build helpers | 8 | 1,626 | Directional subset only; target dependency truth and dependency acquisition are more important than line count. |
| `Tools/Support` | 2 | 182 | Small shared console support; no expansion needed. |
| `Projects/Showcase` sources | 2 | 10 | Correctly thin product entry points. |

Approximately 45% of this scanned surface belongs to Renderer and RHI. That is reasonable for a renderer-first engine with two native APIs and path tracing. The more concerning signal is shape:

- Renderer has 320 C/C++ files at or below 40 lines and 71 at or below 10 lines.
- `Renderer/Private/Passes` alone has 158 files for roughly 6.6k lines; Lighting has 70 files for roughly 2.3k lines.
- GameFramework has 169 C/C++ files at or below 40 lines and 97 at or below 20 lines.
- Launcher is the opposite pressure: `LauncherMainWindow` owns dozens of responsibilities and methods across many `.cpp` fragments while retaining one shared state object.
- RHI has 28 files over 300 lines, concentrated in native device, descriptor, allocator, and bootstrap work where semantic density is expected.

This combination matters. SparkleEngine does not have one simple “large files” problem. It has **over-fragmented private capability code, broad integration objects, and a few legitimately dense backend owners at the same time**. A blanket split or merge campaign would make at least one of those problems worse.

### Executive health assessment

| Surface | Current verdict | Decision |
| --- | --- | --- |
| Graphics algorithms and dual backends | Strong capability, incomplete evidence | Protect; simplify only through measured vertical slices. |
| Frame graph and task runtime | Valuable singular infrastructure | Maintain and harden; do not create competing convenience layers. |
| Persistent scene/view/frame path | Directionally correct recent ownership work | Consolidate navigation and remove migration residue; do not restart the architecture. |
| Runtime host and world extraction | Sound one-way flow with leaky public seams | Narrow contracts and collapse closed-world coordination. |
| Content and shader pipeline | Essential deterministic boundary | Preserve formats and products; remove duplicate discovery, metadata, and publication decisions. |
| Editor and launcher | Useful supporting workflows, oversized coordination surface | Freeze breadth; simplify existing tasks within the roadmap's supporting-work budget. |
| Diagnostics | Useful raw observations, too much product-shaped surface | Keep bounded facts and capture handoff; delete or privatize unconsumed policy projections. |
| Build graph and documentation | Several truthful mechanisms, visible drift | Repair immediately because adoption and review depend on them. |

## Repository-Wide Findings

### 1. The main topology is better than the integration surface suggests

The primary runtime flow is already comprehensible:

```text
Showcase entry point
  -> Application host
  -> GameWorld update and immutable read/extraction
  -> RenderFrameSubmission
  -> Renderer coordinator
  -> persistent scene + prepared view + frame pipeline
  -> frame graph
  -> RHI contract
  -> D3D12 or Vulkan backend
```

This is the architecture to keep. `RuntimeApplication`, `GameWorld`, the submission extractor, `RenderCoordinator`, `FramePipeline`, the frame graph, and the RHI are recognizable owners. The largest gains will come from making their seams honest and removing auxiliary machinery around them, not replacing the flow.

### 2. Public headers and the build graph do not always describe the same dependency

Several exported surfaces rely on types from modules that CMake marks private:

- [Renderer.h](../../Engine/Renderer/Public/Renderer.h) exposes `RenderFrameSubmission`, which physically lives in GameFramework, while Renderer links GameFramework privately.
- [RuntimeApplication.h](../../Engine/Application/Public/RuntimeApplication.h) includes GameFramework camera types, while Application describes and links GameFramework as implementation-only. Showcase explicitly links GameFramework, which masks the mismatch for the current product.
- [UI.h](../../Engine/Editor/Public/UI.h) directly includes GameFramework world/edit/view types, while Editor links GameFramework privately.
- GameFramework publicly links Platform even though its public headers do not expose a Platform contract; Tasks is a real public dependency because public constructors use `TaskExecutor` and `TaskScope`.

These are not cosmetic CMake details. They make the public API appear narrower than it is, force top-level projects to compensate, and weaken shared-library/adoption credibility. Every public header should compile from the dependencies its target publishes, and every published dependency should be required by the public contract.

### 3. `Public` contains substantial engine-private implementation vocabulary

The Renderer `Public/FrameGraph`, `Public/ShaderParameters`, and `Public/SceneData` families are predominantly consumed inside Renderer and its shader-registration build target. No external engine or tool consumers were found for the public frame-graph or scene-data headers in this snapshot. Exporting these types enlarges the apparent engine API, build coupling, and review surface without creating a real adoption seam.

The right correction is not to hide useful renderer settings, viewport requests, capture results, or RHI contracts. It is to move internal graph construction and pass-binding vocabulary behind the Renderer boundary, then give the shader-registration target deliberate private access as part of the same owner.

### 4. File boundaries often encode call steps rather than ownership boundaries

Representative pass traversal shows multiple layers such as `Lighting`, `RestirLighting`, `RestirDirectLighting`, individual reservoir stages, pass declarations, and pass operations. Some are valuable because they isolate a shader ABI, resource lifetime, or independently selectable pass. Others are one-use forwarding functions or tiny declaration/definition pairs that add a navigation hop without isolating an invariant.

The same tendency appears in GameFramework systems and tool stages. This is why line-count-only action is dangerous:

- do not combine unrelated passes into a renderer god file;
- do co-locate a private one-use helper with the capability owner when it has no independent lifetime, failure mode, shader contract, reuse, or test seam;
- keep separate units for native backend lifetimes, independently scheduled tasks, shader ABI declarations, and independently selectable providers;
- judge the result by the number of files and concepts a reviewer must traverse for one behavior, not by a maximum file size.

### 5. Some closed sets are implemented as if they were open platforms

GameFramework currently builds a fixed set of world systems, describes them in one graph-building path, binds them in another, and compiles them through a generic system graph compiler with identifier, name, access, hazard, phase, cycle, and partition machinery. The validation intent is good. The open-ended framework is not justified by a public plugin model or multiple independent topologies in the current code.

Keep deterministic phase ordering, declared access, hazard checks, serial/parallel parity, and the shared SparkleTasks runtime. Replace duplicate descriptor and binding authorities with one closed table or direct task-graph assembly for the actual system set. Retain only validation that can catch a real current error. This preserves behavior while deleting maps, string identity, and coordination that exist only to make one fixed graph look generic.

### 6. God objects have been distributed across files without distributing ownership

[LauncherMainWindow.h](../../Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindow.h) owns workflow navigation, build/cook/clean options, dependency sync, level sync, quick start, operation confirmation, process state, activity history, output, styling, focus order, and large widget/state maps. Its methods are spread across many translation units, but all mutate one class. The navigation burden fell; the change-coupling burden did not.

[UI.h](../../Engine/Editor/Public/UI.h) similarly presents host services, world read/edit callbacks, renderer diagnostic providers, viewport requests/products, shader operations, capture, panel ownership, input routing, scene models, transactions, and render-packet production through one exported type.

The production target is not more wrappers. It is a small composition root plus durable task owners:

- launcher domain operations remain typed and non-Qt where possible;
- each durable launcher workflow owns its projection and local UI state;
- one operation descriptor supplies identity, readiness, impact, confirmation, and display metadata;
- Editor exposes one narrow session/frame contract to Application while panels, callbacks, diagnostic presentation, and ImGui integration remain private;
- shared state moves only when there is a true independent owner, not merely to shorten a class.

### 7. Observation surfaces sometimes imply policy that the engine does not execute

RHI memory facts, renderer memory snapshots, mesh/texture statistics, preview geometry, and viewport capture are useful. The problematic layer is derived policy-shaped data that has no behavioral consumer. In the current memory path, texture-streaming policy/pressure projections are produced for diagnostics but are not consumed by a texture-streaming owner. That makes the repository look more productized than it behaves.

Keep raw budget, committed/resident/allocation, resource, and capture facts. Compute display-only labels in the Editor. Publish a policy object only when an owning runtime system consumes it to make a bounded decision. Keep detailed inspection on demand or editor-only, and prefer PIX, RenderDoc, Nsight Graphics, and workload artifacts for durable performance evidence.

### 8. Documentation currently overstates navigation truth

The documentation root, Whole Repository Map, Editor Viewport Camera document, and Debug View proposal route readers to `Docs/Architecture/RendererSceneViewFrameArchitecture.md`. That document was deleted after the recent renderer migration, while the links and “migration completed” navigation claim remained. This is a concrete production-quality defect: the advertised reviewer path is broken at the point where the repository claims its newest ownership model is easiest to understand.

Do not restore a large implementation-plan document merely to satisfy the links. Move the small amount of durable owner/navigation truth into the current canonical architecture owner, route the links there, and remove stale phase/prompt language. Documentation consolidation must follow the same clean-break rule as code.

### 9. The largest gap remains proof, not another internal subsystem

The Gap Assessment and Roadmap correctly identify the decisive gaps: trustworthy setup, paired-backend evidence, demanding content correctness, rigorous path-traced comparison, a real fixed neural feature, reproducible captures, and adoption material. The repository should not spend the next phase pursuing an abstract minimum line count while those gates remain open.

Structural work is justified when it does at least one of the following:

- makes a roadmap workload easier to run or reproduce;
- removes a duplicate authority from the path being changed;
- narrows a public adoption boundary;
- makes D3D12/Vulkan parity easier to inspect;
- makes memory, lifetime, failure, or scheduling behavior more explicit;
- deletes a superseded path after the replacement is proven.

## Vertical Slice Assessment

### Runtime, world, and frame submission

**Current path**

```text
RuntimeApplication::BeginFrame / UpdateRuntime
  -> GameWorld::Update
  -> typed world systems through SparkleTasks
  -> GameWorld::ExtractRenderFrameSubmission
  -> object/resource/dynamic extractors
  -> Renderer::SubmitRenderFrame
```

**What is already right**

- Application owns host sequencing rather than world or renderer policy.
- GameWorld is the mutable world authority and extraction is explicit.
- Render submission is a one-way value boundary.
- SparkleTasks is the one shared execution substrate.
- Showcase entry points are only a few lines and do not duplicate engine setup.

**Current complexity tax**

- RuntimeApplication publicly exposes Timer, Window, InputSystem, LevelSession, Renderer, TaskExecutor, and TaskScope accessors. Current consumers are EditorApplication internals, not independent products.
- Application's public camera and viewport types expose private target dependencies.
- The world-system descriptor, binding, and compiler paths repeat knowledge for one fixed topology.
- Some task metadata, such as scope kind, has no behavioral reader in the current tree.

**Target**

Keep the public launch/options entry and the frame lifecycle needed by real hosts. Move editor-only host access behind a private Application-to-Editor bridge. Define the render submission dependency honestly. Collapse the fixed system topology into one source of identity, access, order, and executor binding while preserving all systems and parallel semantics.

**Proof**

Use serial, one-worker, and normal-worker world/frame runs; compare produced submission identity and ordering; exercise runtime and editor hosts; verify public-header compilation from declared target dependencies.

### Renderer scene, view, frame graph, and passes

**Current path**

```text
RenderCoordinator
  -> FramePipeline::BeginFrame
  -> persistent render-scene acceptance/preparation
  -> RenderView preparation
  -> technique-neutral frame resources
  -> GBuffer + lighting + reconstruction + post + presentation passes
  -> FrameGraph compile/execute
  -> RHI submission and presentation
```

**What is already right**

- The current Scene/View/Frame/Passes/FrameGraph axes are appropriate.
- FramePipeline stages are named and ordered.
- Frame-graph resource access and pass parameter binding are explicit.
- Reference and ReSTIR techniques are separate selectable behaviors.
- Presentation applies tone mapping and output encoding as an explicit final boundary.

**Current complexity tax**

- Small satellites remain around the accepted axes: `Resources`, `Pipeline`, `PipelineRuntime`, `Providers`, `Host`, `Integrations`, and diagnostic/support roots can require cross-folder traversal for one capability.
- Pass implementation is often split into orchestration, helper, pass declaration, pass operations, settings, and CVar pairs even when several pieces have one caller and one lifecycle.
- A large internal frame-graph and shader-parameter vocabulary is exported publicly.
- The Renderer facade combines core frame submission with editor-only diagnostics, preview, reload, and capture operations.

**Target**

Preserve Scene/View/Frame/Passes/FrameGraph as the main navigation. For each renderer capability, draw one owner map and co-locate one-use private helpers under it. Keep a file separate when it owns a shader contract, scheduled pass, resource lifetime, provider boundary, or independent testable policy. Make frame-graph construction private to Renderer. Keep only viewport/settings/capture contracts that have real external consumers. Split editor inspection from the minimal runtime submission facade without adding a general service locator.

**Proof**

For each touched capability, compare graph pass/resource identity, output format, history invalidation, serial/threaded behavior, D3D12/Vulkan results, and the relevant workload capture. Measure build and frame cost only where the change claims an improvement.

### RHI and native backends

**Current path**

```text
Renderer frame graph
  -> RenderHardwareInterface service contracts
  -> common resource/pipeline/descriptor/upload/capture interfaces
  -> D3D12 or Vulkan implementation
  -> native queues, descriptors, allocators, barriers, and presentation
```

**What is already right**

- There is one explicit RHI rather than two renderers.
- Native backend code remains private and substantial.
- Capability negotiation and fallback are explicit concerns.
- Descriptor, allocator, queue, ray-tracing, interop, and presentation lifetimes are visible.

**Current complexity tax**

- The RHI facade exposes many service getters and observation services, increasing the number of contracts every backend must implement.
- Vulkan allocator and descriptor owners are dense and cover several lifecycle stages; adjacent service/allocator objects may duplicate registry, publication, or recording state.
- Capability structures and virtual methods can persist after their only consumer disappears.

**Target**

Do not introduce a second abstraction layer or force D3D12/Vulkan into identical internal shapes. Audit one native operation at a time—resource create/destroy, descriptor publication, upload, queue retirement, acceleration structure, or capture—and draw its state/lifetime owner. Remove forwarding and duplicate state only when the trace proves it. Keep backend-specific implementation dense when one invariant spans the code. Retain only queried capabilities and externally needed diagnostic facts.

**Proof**

Use paired API/ABI builds and runs for the touched path, validation layers, resource-lifetime checks, capture inspection, and the smallest workload that falsifies parity. RHI-wide cleanup without a named behavior claim is out of scope.

### Source import, cooking, loading, and world publication

**Current path**

```text
project level/catalog selection
  -> AssetCooker discovery and plan
  -> glTF or FBX importer -> SourceImportOutput
  -> texture/material/mesh/scene cookers
  -> cooked product publication
  -> LevelSession and GameWorld load
  -> immutable resource tables + scene delta
  -> persistent renderer resources
```

**What is already right**

- Runtime stays cooked-only.
- Importers normalize source formats into one imported-domain model.
- Coordinate and unit normalization have a canonical contract.
- Cooking separates texture, material, mesh, scene, and shader products.
- Scene publication distinguishes structural tables from dynamic updates.

**Current complexity tax**

- Project catalog parsing, level parsing, discovery, launcher content models, cook plans, and runtime resolution repeat identifiers and validation in different shapes.
- AssetCooker carries discovery, category planning, stage dispatch, subprocess formatting, batch scheduling, diagnostics, and output records around a small fixed set of stages.
- Source catalogs mix acceptance workloads, built-in content, optional research packs, acquisition policy, and unsupported future-format notes in one operational surface.
- Tool reports can become permanent schemas without a current acceptance consumer.

**Target**

Keep every supported format and catalog entry. Separate the default evidence/product set from optional research metadata without deleting either. Establish one canonical project/level/content identity contract consumed by launcher, cook, and runtime owners. Reuse one narrow section/key-value syntax reader where syntax is truly shared, while domain validation remains with each owner. Keep one cook plan and one atomic publication path; avoid another asset database, dependency graph, cache framework, or tool scheduler.

**Proof**

Re-cook representative built-in, glTF, FBX, Sponza, Bistro, and San Miguel inputs affected by a change; compare canonical identity, material/texture/mesh/animation/skin/light/camera outputs, deterministic manifests, load activation, and failure diagnostics. Do not run a whole cook when a smaller changed category falsifies the claim.

### Shader authoring, compilation, packaging, and reload

**Current path**

```text
renderer shader registrations + HLSL/Slang source
  -> contract catalog and package layout
  -> DXC or Slang backend compile
  -> reflection/ABI verification
  -> cooked shader package + registry
  -> RHI runtime package load
  -> renderer pipelines and optional editor recook/reload
```

**What is already right**

- Registration and source feed one offline compiler.
- Typed parameter and ABI verification are explicit.
- Cooked packages carry backend/codegen identity.
- Inspection commands provide artifact-level evidence.
- Persistent compile-cache removal reduced policy and invalidation burden without removing compilation features.

**Current complexity tax**

- Static backend registration, shader registration, contract catalog assembly, package layout, command registry, inspection models, analysis reports, and editor recook coordination form a long path for a currently fixed product set.
- Renderer shader-registration targets reach into Renderer private headers through special build access, while much of the enabling type system is public.
- Application owns more than half of its private code in editor operations, shader recook, and runtime console support, blurring the minimal runtime-host story.

**Target**

Retain compile-every-selected-input behavior, both compiler backends, typed contracts, inspection, and reload. Make the registration-to-package model one immutable source of shader identity. Keep compiler command presentation separate from compilation policy, but delete duplicate catalogs and derived records with no independent consumer. Treat renderer shader registrations as an internal friend target, not a reason to publish the entire graph/pass type system. Keep recook process orchestration editor-only and the runtime reload seam narrow.

**Proof**

Compile a representative graphics, compute, ray-query, and ray-tracing-library package for each supported backend/target; run contract validation and package inspection; verify runtime generation swap and failure behavior. Do not add cache/version compatibility paths.

### Editor interaction and diagnostics

**Current path**

```text
EditorApplication
  -> EditorHostServices callbacks
  -> UI scene model / viewport session / panels
  -> semantic world and rendering commands
  -> UiRenderPacket + viewport request
  -> Renderer products, captures, and diagnostics
```

**What is already right**

- World edits are semantic commands rather than direct ECS mutation.
- Scene models and viewport session concepts exist.
- The editor consumes renderer products rather than owning render execution.
- Panels correspond to recognizable inspection tasks.

**Current complexity tax**

- One exported `UI` type owns host callbacks, panels, input, transactions, viewport, diagnostics, operations, and ImGui packet production.
- Host services and diagnostic providers are collections of `std::function`, obscuring which module owns lifetime and failure policy.
- Used-shader/mesh/texture and memory panels encourage broad public snapshot types and repeated presentation metadata.
- `Private/Util` is a 1.1k-line catch-all spread across generic widget and detail helpers.

**Target**

Expose one focused Editor session/frame contract to Application. Keep ImGui, panel classes, scene-model construction, transaction history, diagnostic formatting, and renderer-specific providers private. Replace callback bags with a few typed domain bridges only where there is a real cross-module boundary; do not create generic dependency injection. Move utility behavior to the capability that owns its semantics, retaining only genuinely shared widget primitives. Freeze new panel breadth until a roadmap or evidence gate requires a distinct durable task.

**Proof**

Exercise viewport navigation, scene read/edit/undo, settings, shader reload/recook, asset inspection, capture, DPI, and input routing. Verify the runtime host remains editor-free and the editor target's public header closure matches CMake.

### Launcher, build, cook, and quick start

**Current path**

```text
workflow catalog and settings
  -> LauncherMainWindow projection
  -> LauncherBackend preview/run/cancel
  -> typed workspace/cook/level/maintenance plan
  -> operation service and process runner
  -> output/activity/refresh/quick-start continuation
```

**What is already right**

- Operations are typed and can be previewed before execution.
- Destructive scope, readiness, effects, output, cancellation, and completion are explicit.
- Process execution is behind a testable runner seam.
- Quick start sequences existing operations instead of creating a second build/cook implementation.

**Current complexity tax**

- Operation identity and UI policy are distributed across domain definitions, `LauncherBackend`, `LauncherWorkflowCatalog`, `LauncherUiModel`, settings, and MainWindow switch/helper methods.
- MainWindow owns all workflow widget state and activity state even though implementations are split among many files.
- Build planning/execution and GUI readiness/status models can represent the same prerequisites more than once.
- Launcher volume is high relative to its roadmap role and can absorb effort without strengthening graphics evidence.

**Target**

Make one operation descriptor the source for ID, category, display, readiness, effect, confirmation/destructive scope, supported options, and execution plan. Let the shell compose Home, Sync, Build, Cook, Clean, and Activity owners with local state; keep a single backend execution service. Derive status and buttons from typed operation state instead of operation-ID switches. Preserve every workflow and keep Qt out of domain planning. Apply this only within the roadmap's supporting-tool budget.

**Proof**

Preview, run, cancel, fail, retry, and clean each operation category; verify exact commands, scope, confirmation, output, refresh, and quick-start handoff. A responsive initial launcher process is not sufficient evidence that the generated editor/runtime path launched.

### Diagnostics, capture, and portfolio evidence

**Current path**

```text
RHI/renderer raw facts
  -> on-demand snapshots and viewport readback
  -> editor presentation or external capture
  -> workload artifact + metrics + reproduction steps
```

**Target**

Keep one bounded live summary for immediate diagnosis and explicit on-demand snapshots for inspection. Hand off durable performance work to external profilers and the accepted workload evidence schema. Do not create a parallel dashboard, trace store, anomaly engine, or public diagnostics API unless an approved product requirement names its user and acceptance gate. Compile or erase editor-only observation code from non-editor/shipping paths where practical.

**Proof**

Demonstrate that raw values originate from the owning allocator/resource system, snapshots are bounded and on demand, capture identity matches the workload, and no diagnostic claim is presented as runtime policy unless an owner consumes it.

## Horizontal Module Decisions

| Area | Preserve | Simplify or consolidate | Priority |
| --- | --- | --- | --- |
| `Engine/Core` | Math, events, logging, paths, process/file primitives, true cross-product project identity. | Audit public headers by real consumers; keep syntax helpers policy-free; prevent project/content/diagnostics policy from accumulating in Core. | P1 when touched |
| `Engine/Tasks` | One executor, task graph, scopes, deterministic topology, bounded workers. | Remove unused descriptive metadata and one-consumer conveniences; do not add subsystem pools or another scheduler. | Maintain / P3 |
| `Engine/Platform` | Window, input, filesystem/platform adaptation. | Keep thin; move no editor, renderer, or product policy downward. | Protect |
| `Engine/GameFramework` | World authority, private ECS, typed systems, levels, scene semantics, explicit render extraction. | One closed system table/graph; one identity per world/render resource; reduce file-per-step fragmentation; make public dependencies exact. | P1 |
| `Engine/RHI` | Explicit API contract, D3D12/Vulkan backends, native lifetimes, ray tracing, diagnostics facts, presentation. | Audit one native lifecycle at a time; trim unqueried capabilities/virtual methods and duplicate backend state; no second abstraction. | P2, evidence-driven |
| `Engine/Renderer` | Scene/view/frame/frame graph, raster/ray paths, reference/ReSTIR, providers/fallback, settings, capture. | Privatize internal graph/pass types; consolidate capability-local helpers; separate runtime submission from editor diagnostics; reconcile satellite folders. | P1 |
| `Engine/Editor` | Viewport, semantic scene editing, settings, console, asset inspection, capture. | Replace exported god UI/callback bags with a narrow session boundary; keep panels/models/providers private; retire catch-all utilities. | P1 boundary, P2 internals |
| `Engine/Application` | Minimal runtime and editor composition, frame lifecycle, cooked-only runtime. | Privatize editor-only service access; move recook/editor coordination behind the editor host; publish only real host contracts. | P0/P1 |
| `Engine/Assets` | Current shaders, includes, source meshes/textures, defaults, and evidence inputs. | Organize by the capabilities that consume them; do not duplicate shader truth in documentation or generated registries. | Maintain |
| `Tools/Import` | glTF/FBX support, canonical coordinate/unit normalization, imported-domain model. | Share domain-neutral parsing/math only; keep format translation explicit; remove duplicate intermediate copies where ownership permits moves/views. | P1 with content gates |
| `Tools/Cooking` | Deterministic texture/material/mesh/scene/asset products and atomic publication. | One cook plan, one identity, one diagnostics record, one publication path; avoid permanent reports without acceptance consumers. | P1 with content gates |
| `Tools/Shaders` | DXC/Slang, typed contracts, packages, registry, inspection, compile-every-input policy. | One shader identity/catalog; internalize registration support; keep CLI and analysis as thin views over the same product. | P1 with shader gates |
| `Tools/Launcher` | All current workflow capabilities and typed operation execution. | One metadata authority; workflow-local UI state; thin composition shell; derive readiness/status rather than switch on IDs. | P2 / <=10% support lane |
| `Tools/Support` | Shared console-process conventions. | Keep at current small scope. | Protect |
| `Projects/Showcase` | Separate runtime/editor launch products and evidence entry point. | Keep entry points thin; remove compensating module links only after public target closure is corrected. | P0 build truth |
| CMake/configuration | Profiles, artifact contract, explicit product targets, boundary check, pinned dependencies. | Make PUBLIC/PRIVATE closure truthful; review always-built editor/launcher cost; break dependency acquisition only by vendor/lifetime owner, not arbitrary size. | P0 |
| Documentation | One authority per decision, reviewer routes, standards, workload gates, dated plans. | Repair missing routes; remove completed implementation prompts; merge durable truth into current owners; delete superseded documents in the same change. | P0 |

## Prioritized Refactoring Program

### P0 — restore trust before broad structural work

1. **Repair documentation authority.** Reconcile every link to the deleted scene/view/frame document. Put the current owner map in a live canonical document and remove stale migration-complete or phase language.
2. **Make target boundaries executable truth.** For Renderer, Application, Editor, GameFramework, and Showcase, align public-header includes with PUBLIC/PRIVATE link dependencies. Add the cheapest configuration/header-closure check that detects recurrence.
3. **Record a feature-preservation matrix for each slice.** Name affected capabilities, both APIs, artifacts, fallbacks, workflows, and the smallest falsifying checks before editing. This is not a new feature backlog.
4. **Audit public contracts by consumer.** Move internal-only Renderer graph/parameter/scene headers and internal Editor panel surfaces to private ownership. Delete export annotations and transitive dependencies that cease to be necessary.
5. **Stop new breadth while consolidating.** No new editor panel, launcher workflow, provider framework, import format, diagnostics product, or generic runtime should enter without a current roadmap gate.

P0 is small, reviewable, and mostly behavior-neutral. It raises trust immediately and establishes safe boundaries for later deletion.

### P1 — simplify the gate-carrying vertical slices

Run these as independent clean-break changelists, with one primary work item at a time:

1. **Runtime/editor host seam:** keep RuntimeApplication's product API narrow; replace public internal getters and Editor callback bags with focused private host bridges.
2. **GameFramework execution:** replace duplicate fixed-system descriptors/bindings and open-ended compilation machinery with one closed topology while retaining hazards and parallel behavior.
3. **Renderer capability cohesion:** start with a roadmap-touched path such as lighting, view preparation, or presentation; collapse one-use forwarding layers and reconcile satellite folders without combining independent passes.
4. **Content identity and cook path:** make project/level/asset identity singular from launcher selection through cooker publication and runtime load; reuse syntax only where actually shared.
5. **Shader identity and publication:** keep one registration/contract/package authority and narrow editor recook/runtime reload seams.

Each slice must produce net structural deletion or a demonstrably narrower boundary. If it merely moves files or replaces concrete code with more abstractions, it does not meet the objective.

### P2 — reduce supporting-product and backend friction where evidence justifies it

1. Decompose LauncherMainWindow by durable workflow ownership and delete duplicate operation metadata/switches.
2. Internalize Editor panel/diagnostic implementation and eliminate catch-all utility ownership.
3. Audit one D3D12/Vulkan resource, descriptor, queue, or ray-tracing lifecycle at a time after paired captures identify review or performance friction.
4. Simplify dependency acquisition and build membership where configuration time, target closure, or review evidence shows a cost.

P2 must not displace the primary graphics/evidence roadmap. Launcher and general editor work remain within the roadmap's supporting-work ceiling.

### P3 — opportunistic deletion

Delete unused scope kinds, fields, flags, getters, wrappers, snapshots, CVar projections, report columns, and no-consumer headers when their owner is already being changed. Do not launch repository-wide micro-cleanup churn for these items.

## Acceptance Contract For Every Refactoring Slice

A slice is complete only when all applicable statements are true:

1. **Feature preservation:** every named pre-change capability and fallback still exists; no scope cut silently removes a workflow, backend, format, effect, or workload.
2. **Single truth:** the change leaves one mutable authority and one production/publication path for the affected concept.
3. **Narrower surface:** at least one public type, duplicated representation, coordination layer, branch, or navigation hop is removed, or a measured runtime/build cost improves without adding equal structural burden elsewhere.
4. **No compatibility residue:** all owned producers and consumers move together; the replaced internal path is deleted in the same change.
5. **Bounded behavior:** memory, task, queue, snapshot, and artifact lifetimes remain explicit and bounded.
6. **Backend honesty:** a cross-RHI change is verified for both D3D12 and Vulkan, or the limitation is reported rather than generalized.
7. **Claim-driven evidence:** the smallest check that could falsify the claim runs first; broader builds, cooks, workloads, and captures run only when the claim requires them.
8. **Review locality:** the changelist has one behavioral title and a reviewer can follow the owner, producers, consumers, invariant, and evidence without reconstructing unrelated systems.
9. **Documentation clean break:** current maps and contracts are updated, completed plans are retired or reduced to durable decisions, and no dead link or duplicate rule remains.

Line reduction is directional evidence, not the acceptance criterion. A 200-line owner can be better than twelve 20-line forwarding files; a 700-line allocator can be correct when one native lifetime invariant requires it. Conversely, a short facade can be expensive if it creates a second authority or hides a dependency.

## Executive Scorecard

Track these measures per capability before and after a slice. Do not set repository-wide quotas.

| Measure | Desired direction | Why it matters |
| --- | --- | --- |
| Mutable authorities for one concept | Exactly one | Prevents drift and synchronization code. |
| Production/publication paths | Exactly one | Makes behavior and failure reproducible. |
| Public types and methods | Down unless a real external consumer is added | Measures adoption surface and compatibility burden. |
| Public headers with zero external consumers | Toward zero | Finds implementation accidentally presented as API. |
| Metadata/catalog representations for one operation or artifact | Toward one | Removes switches, mapping code, and inconsistent UI/tool behavior. |
| Files/directories traversed for one capability change | Down | Measures navigation and change locality better than file size. |
| Forwarding-only functions/classes | Down | Removes concepts without losing behavior. |
| Per-frame copies, snapshots, allocations, and waits | Down or explicitly bounded | Aligns structure with performance behavior. |
| D3D12/Vulkan behavior and evidence | Equal and explicit | Prevents surface-level portability. |
| Default build/cook/run steps | Stable or fewer | Improves adoption without removing optional capability. |
| Reproducible workload artifacts | Up | Converts implementation into principal-level evidence. |
| Net owned source for the same capability | Usually down | Useful outcome signal after ownership and behavior are proven. |

A quarterly executive view should report completed capability slices, structural deletion, public-surface change, paired-backend evidence, workload gates, and residual limitations. It should not celebrate raw negative line count detached from behavior.

## Recommended Order Of Attack

The safest high-leverage order is:

```text
truthful docs and target boundaries
  -> narrow public runtime/editor/renderer seams
  -> simplify one fixed GameFramework execution path
  -> consolidate one roadmap-touched renderer/content/shader slice
  -> simplify launcher/editor projections around the now-stable contracts
  -> audit native backend internals only where paired evidence identifies friction
```

This order prevents UI and tool cleanup from hardening the wrong contracts, prevents backend rewrites from consuming the roadmap, and makes each later deletion easier to verify.

## Explicit Non-Recommendations

- Do not remove features, backends, import formats, current assets, editor workflows, launcher workflows, effects, providers, fallbacks, or evidence workloads to improve a line-count graph.
- Do not rewrite the engine or introduce a new ECS, renderer, frame graph, scheduler, content database, dependency-injection framework, service locator, plugin framework, diagnostics application, or shader cache.
- Do not merge files solely because they are short or split files solely because they are long.
- Do not hide necessary native backend complexity behind a lowest-common-denominator wrapper.
- Do not move product policy into Core or RHI to make higher layers look smaller.
- Do not add adapters, aliases, version readers, or dual representations for internal clean-break changes.
- Do not treat logs, statistics, panels, documents, or a responsive process as proof of correctness or performance.
- Do not run broad builds, cooks, or workloads by default when a cheaper claim-falsifying check exists.
- Do not let this refactor become a second roadmap. It is the structural discipline applied to the existing roadmap.

## Final Assessment

SparkleEngine is not rotten at its technical core. Its strongest systems already show the intended engineering values: explicit native backends, one frame graph, one task runtime, cooked-only runtime loading, one-way render extraction, persistent renderer state, typed shader contracts, and thin product entry points. The repository's credibility is weakened when those strengths are surrounded by public implementation vocabulary, hidden target dependencies, repeated catalogs, generic machinery for fixed sets, observation surfaces without behavioral owners, god integration objects, extreme file fragmentation, and documentation routes that no longer resolve.

The correct executive goal is therefore:

> Preserve every current capability, freeze speculative breadth, and make each roadmap vertical slice leave fewer authorities, fewer public contracts, fewer representations, fewer navigation hops, and stronger reproducible evidence than it found.

That outcome demonstrates the target graphics-engineering persona more convincingly than either a larger feature list or an arbitrary negative line count. It shows an engineer who can carry research-grade graphics work into a compact production system another engineer can build, inspect, trust, and extend.
