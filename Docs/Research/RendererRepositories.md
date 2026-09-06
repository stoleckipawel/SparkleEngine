# E. External Renderer Repository Comparison

Status: research; source-linked external comparison whose patterns are precedent, not local implementation proof
Research snapshot: 2026-07-24
Last local reconciliation: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration are unchanged from implementation revision `99af6d5b`
Scope: vendor reference repositories/frameworks compared against SparkleEngine architecture, developer-technology transfer, path tracing, neural graphics, code construction, extensibility, productization, feature scope, and deletion-first improvement targets

## Intent

This document compares SparkleEngine against top-tier rendering repositories and SDKs. The purpose is not to copy their code. The purpose is to identify what makes those repositories production-grade or reviewable, then use that standard to slim and sharpen Sparkle.

This document owns source-linked precedent analysis only. Local architecture decisions are routed from the [Whole Repository Architecture Map](../Architecture/WholeRepositoryMap.md), implementation rules are routed by the [Engineering task map](../Engineering/README.md#choose-by-task), and capability grades belong in [Principal Graphics Requirements](../Strategy/Requirements.md).

Repository links below may track mutable default branches. They prove only the narrow reviewed pattern as of the research snapshot; a specific API, capability, performance, or support claim must be revalidated against a pinned revision or current primary manual before use. A vendor design becomes good Sparkle practice only after its problem exists here, the owning local document accepts it, a simpler option is considered, and workload evidence validates the practical result.

The main lesson is not "add more features." It is "make scope and ownership painfully clear."

The supplied principal graphics engineering role set adds a second comparison lens. The repository must eventually demonstrate the canonical `PGE-01` through `PGE-15` requirements in [A. Principal Graphics Engineering Requirements](../Strategy/Requirements.md): partner adoption, path tracing, a real neural graphics feature, neural model/workload tuning, low-level CPU/GPU optimization, architecture/driver diagnosis, mathematical rigor, AI fundamentals, and principal-quality communication. Vendor repositories remain precedents and study material; they do not by themselves prove Sparkle satisfies the role.

## Sources Reviewed

| Source | Link | Why it matters |
| --- | --- | --- |
| NVIDIA Donut | https://github.com/NVIDIA-RTX/Donut | Rendering framework used by NVIDIA samples; clear `core`, `engine`, `render`, `app` split. |
| NVIDIA NVRHI | https://github.com/NVIDIA-RTX/NVRHI | Higher-level RHI used by multiple NVIDIA SDKs, with optional state tracking, lifetime tracking, barriers, descriptor/binding model, native API escape hatches. |
| NVIDIA NVRHI Programming Guide | https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md | Explains resource lifetime, reference-counted handles, barrier tracking, binding sets, bindless model. |
| NVIDIA NRI | https://github.com/NVIDIA-RTX/NRI | Low-level explicit render interface with non-goals against hidden management and automatic barriers. |
| NVIDIA RTXDI | https://github.com/NVIDIA-RTX/RTXDI | ReSTIR DI/GI SDK that pushes resource/API ownership to the application through bridge functions. |
| RTXDI Integration Guide | https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md | Strong model for feature integration boundaries. |
| NVIDIA RTX Path Tracing | https://github.com/NVIDIA-RTX/RTXPT | Real-time pure path tracer sample with guide buffers, path-space decomposition, DLSS-RR support. |
| NVIDIA SHARC | https://github.com/NVIDIA-RTX/SHARC | Shader-only radiance cache library with integration guide. |
| NVIDIA RTX Neural Shading | https://github.com/NVIDIA-RTX/RTXNS | Slang/neural examples, training/inference structure, cooperative vector requirements. |
| NVIDIA Streamline | https://github.com/NVIDIA-RTX/Streamline | Cross-IHV integration framework with include/source/shaders/tools and binary release separation. |
| NVIDIA NvRTX | https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch | Public page for gated Unreal RTX branches; shows versioned validated branches and experimental branches. |
| Cauldron | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron | Simple static framework for D3D12/Vulkan FidelityFX prototypes and samples. |
| FidelityFX SDK | https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK | SDK layout with Kits, Samples, Tools, docs, and delivery concerns. |
| Epic Render Dependency Graph | https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine | Core commercial-engine precedent for graph-owned lifetime/scheduling, shader-parameter reuse, graph-only pass parameters, and semantic event names. |
| Epic PSO Precaching | https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine | Separates shader availability, complete pipeline descriptors, asynchronous preparation, and runtime miss/completeness evidence. |
| Epic Shader Development | https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine | Changed-shader iteration and direct source errors show an intent-first author loop. |
| Epic Timing Insights | https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5 | Frames/tracks plus selection-driven aggregates and callers/callees supply the overview-to-detail product pattern. |
| NVIDIA Nsight GPU Trace UI | https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-ui.html | Explicit collection state, frames/queues, range selection, event detail, contextual analysis, and report management. |
| AMD RGP Overview | https://gpuopen.com/manuals/rgp_manual/overview_windows/ | Summary and most-expensive-event orientation with contextual navigation into deeper event/pipeline/occupancy/ISA panes. |

NvRTX code is gated behind Epic/GitHub access, so this document uses the public NVIDIA developer page for that comparison rather than claiming direct code inspection.

## Adversarial Local Reconciliation

The 2026-08-28 local pass treated every earlier Sparkle comparison as potentially stale. The durable external principles survive, but several local findings no longer do:

| Earlier local statement | Reconciled result | Current owner |
| --- | --- | --- |
| Sparkle lacks a crisp product identity. | Resolved in documentation: Sparkle is a compact renderer-first engine and evidence platform. Implementation/evidence still determines whether the repository behaves consistently with that identity. | [Executive Summary](../Strategy/ExecutiveSummary.md) |
| Renderer/RHI policy is implicit and needs a one-page policy. | Resolved as a documentation action; the explicit RHI/frame-graph boundary now exists. | [Renderer and RHI Architecture Boundary](../Architecture/Decisions/RendererRhiBoundary.md) |
| Heavy Showcase/Bistro content lives in the main depot. | Stale: acceptance content is cataloged/externalized. Future media must continue through the workload/content-pack policy. | [Acceptance Workloads](../Acceptance/GraphicsWorkloads.md) and [Whole Repository Map](../Architecture/WholeRepositoryMap.md) |
| Renderer public API exposes direct RHI access. | Stale: the reconciled `Renderer` public surface exposes bounded resource/memory/capture snapshots but no direct RHI accessor. Those remaining observation surfaces still require current consumers and bounded cost. | Code and [Renderer/RHI Boundary](../Architecture/Decisions/RendererRhiBoundary.md) |
| Vendor branch separation implies Sparkle should create product/research branches. | Overreach: the transferable requirement is explicit maturity/capability status and safe defaults. Branch topology is one vendor delivery choice, not a required Sparkle architecture. | Focused feature Architecture and executable capability policy |

This table is a dated reconciliation, not a replacement readiness grade. [Gap Assessment](../Strategy/Assessments/GapAssessment.md) remains the owner of current grading.

## External Repository Patterns

### 1. Scope Is Declared Explicitly

Donut states that it is a real-time rendering framework and explicitly says it is not a game engine. Its README describes four static-library subsystems:

- `donut_core`: math, VFS, logging, JSON/utilities, no graphics.
- `donut_engine`: scene import/maintenance, animations, materials, texture cache, descriptor tables, CVars, audio.
- `donut_render`: rendering passes.
- `donut_app`: interactive application/device/UI/camera/media utilities.

Sparkle comparison:

- Sparkle now declares itself a compact renderer-first engine and evidence platform.
- Its broad module/tool surface is valid only where it advances that identity and the principal-graphics evidence requirements.
- The remaining test is practical: a production reviewer must see the same product boundary in default builds, public APIs, workloads, and artifacts within one minute.

Action:

- Preserve the declared compact renderer-first identity as the authority.
- Cut, move, or explicitly reject workflows that do not serve it; do not reopen the identity as an option list in downstream documents.

### 2. RHI Tradeoffs Are Stated, Not Implied

NVRHI advertises higher-level conveniences:

- optional resource state tracking and barrier placement
- resource usage/lifetime tracking and deferred safe destruction
- efficient binding model
- easy native API interaction when necessary
- upload buffer suballocation and constant buffer versioning
- parallel command list recording and multi-queue rendering
- validation/reflection support

NRI advertises a different philosophy:

- low-level explicit rendering interface
- low overhead
- no hidden high-level management
- no automatic barriers
- direct support for D3D12/Vulkan-style concepts

Sparkle comparison:

- Sparkle is between NVRHI and NRI.
- The RHI is explicit and backend-owned, but the renderer frame graph performs state tracking, barriers, resource products, and transient planning.
- That is a good policy if documented: "RHI is explicit; Renderer frame graph is the higher-level scheduler."

Action:

- Keep [Renderer and RHI Architecture Boundary](../Architecture/Decisions/RendererRhiBoundary.md) as the single policy owner for what RHI tracks, what the frame graph automates, what passes state, native interop, and lifetime protection.
- Prove the written boundary through architecture checks and representative D3D12/Vulkan paths; do not create another abstraction or duplicate policy here.

### 3. SDK Integrations Keep Application Ownership

RTXDI is very clear: the SDK supplies sampling/resampling math, while the application owns graphics API interaction, resources, light data, GBuffer addressing, shaders, passes, and bridge callbacks.

Sparkle comparison:

- Sparkle already owns renderer resources and has a native ReSTIR DI-shaped direct lighting path.
- That is architecturally respectable.
- But Sparkle should not imply RTXDI SDK equivalence unless the SDK is integrated and validated.

Action:

- Describe direct lighting as "native reservoir-based ReSTIR DI-shaped direct lighting."
- If RTXDI is ever integrated, keep it as a feature provider/math library, not as a new renderer owner.
- Keep light buffers, GBuffer, TLAS, material model, and shader scheduling in Sparkle.

### 4. Research Features Are Often Shader-Only Or Sample-Only

SHARC ships as shader-only sources plus an integration guide. RTXNS ships examples/samples, docs, helper code, assets, and external dependencies. RTXPT is a focused path tracing sample/reference.

Sparkle comparison:

- A vendor's shader-only/sample-only packaging does not determine PTLAS or any Sparkle feature's maturity.
- The relevant risk is allowing unexercised planner, metric, or future-feature scaffolding to become a renderer-wide authority before an accepted consumer exists.

Action:

- Classify each capability as product, preview, compiler/schema-only, or experimental at its owning declaration and in executable support evidence.
- Choose shader-only, sample-only, provider, or main-renderer integration from actual ownership/lifetime requirements. Do not use packaging or branch labels as a substitute for runtime capability proof.
- Do not let an experimental feature shape the main renderer data model unless a current product consumer requires that shared contract.

### 5. Sample Frameworks Keep Samples Separate

Cauldron is a static framework used by many FidelityFX sample projects. FidelityFX SDK separates `Kits`, `Samples`, `Tools`, and `docs`. Donut points examples to a separate Donut-Samples repository.

Sparkle comparison:

- Sparkle has already externalized/cataloged the heavy Bistro/San Miguel acceptance content while retaining bounded smoke/reviewer routes.
- The external pattern remains a regression guard: uncataloged optional media must not drift back into the default clone/runtime product.

Action:

- Preserve the workload catalog/content-pack boundary and its clean-environment acquisition evidence.
- Keep only the curated in-repo assets needed for build/runtime smoke and review.

### 6. Product Repos Separate Validated And Experimental Branches

NvRTX publicly separates Unreal Engine version branches and an experimental branch. The public page states the experimental branch is not fully validated and can contain limitations/regressions not suitable for shipping.

Sparkle comparison:

- Sparkle still needs explicit capability maturity and safe default selection for experimental paths, but a separate Git branch is not automatically the right mechanism.

Action:

- Record product, preview, compiler/schema-only, and experimental status in the owning declaration, build/runtime capability matrix, tests, and documentation.
- Shipping/default paths must not depend on unvalidated research scaffolding. Use a branch only when release/validation workflow evidence makes it the simplest delivery mechanism.

### 7. Binary/Source Distribution Is Treated As Product Policy

Streamline's README separates GitHub source from release binary artifacts. It states that shipping developers usually use prebuilt directories/releases and that rebuilding Streamline from source is optional.

Sparkle comparison:

- Sparkle intentionally keeps release assembly outside the launcher; distribution packaging is performed manually.
- The launcher owns local development setup, build, cook, launch, and cleanup workflows only.

Action:

- Keep distribution packaging policy and execution outside the launcher unless a separate, explicitly owned release system is introduced.

### 8. Developer Technology Requires Transferable Evidence

The strongest NVIDIA/AMD repositories do more than contain an algorithm:

- the integration boundary and prerequisites are explicit;
- application-owned resources and scheduling responsibilities are stated;
- supported hardware/API paths are classified honestly;
- sample or test content demonstrates the intended workload;
- shader/model assets and runtime dependencies have deliberate packaging;
- limitations and fallback behavior are visible;
- performance guidance is tied to workload and architecture rather than presented as a universal slogan.

Sparkle comparison:

- Sparkle has strong internal RHI, shader, frame-graph, and provider foundations.
- It does not yet have an externally adoptable neural-graphics integration case, model-training/export evidence, or a complete path from neural algorithm math to optimized runtime result.
- Current internal documentation is extensive, but principal graphics engineering evidence also requires a concise handoff, reduced reproducer, live demo, and result-focused technical note.

Action:

- Treat one future neural graphics feature as a technology-transfer vertical slice, not a framework milestone.
- Record requirements, classical baseline, model/operator provenance, tensor shape/layout/precision, training/export, runtime integration, capability/fallback, D3D12/Vulkan behavior, quality, latency, memory, and hardware limits.
- Produce a minimal adoption guide and reproducible workload after the implementation is real.
- Keep all training/offline dependencies out of the runtime package and delete experimental scaffolding that the accepted feature does not need.
- Preserve negative results and rejected layouts/precision/concurrency choices so the guidance reflects engineering judgment.

### 9. Production Frontends Preserve Intent And Hide Mechanics

Epic's shader workflow asks the developer to save edits and recompile changed shaders; it does not ask them to build compiler jobs, choose cache files, or reload individual native objects. Timing Insights starts with frames and tracks, then lets one selection drive aggregate and caller/callee detail. Nsight GPU Trace exposes capture readiness/state and frames/queues before event, metric, source, and analysis panes. RGP leads with summary and most-expensive events and preserves selection while navigating to deeper panes.

Sparkle comparison:

- Current backend ownership is stronger than the current frontend hierarchy. Shader Tools leads with refresh, reload, and recook operations plus a nine-column catalog of shader identity, source, backend/target, generation, and artifact mechanics; the proposed diagnostics menu can similarly become cluttered if every stat group is a first-level choice.
- Hiding detail must not mean deleting expert capability. It means deriving safe defaults and moving raw identities, manifests, hashes, compiler/capture settings, and specialized panes behind the selected task/object.

Action:

- Lead with a small task vocabulary: `Apply Changed`, `Quick Check`, `Investigate CPU/GPU/Memory`, and `Capture Evidence`.
- Preserve one shader/frame/range/marker/configuration selection across summary, detail, retry, and external handoff.
- Generate dependency closure, target/capability choice, cache use, collection mode, manifest identity, and validated publication in their backend owners.
- Keep raw group selection, catalog rows, compiler requests, generations, hashes, native handles, counters, and tool-specific settings searchable under contextual Diagnostics/Advanced surfaces.
- Do not copy Unreal Insights, Nsight, or RGP pane counts. Copy their progressive task-to-detail navigation and explicit state/failure behavior.

## Direct Comparison Matrix

| Reference | Production pattern | Sparkle today | Improvement |
| --- | --- | --- | --- |
| Donut | Framework explicitly not a game engine; four module libraries; app/device management separated from render passes. | Sparkle now declares a compact renderer-first engine/evidence identity and is intentionally broader. | Enforce that identity through default product surface and delete non-serving workflows. |
| NVRHI | Higher-level RHI with resource/lifetime/state/barrier helpers and native escape hatches. | Sparkle's explicit RHI plus managed renderer frame graph is now documented. | Keep the boundary executable and avoid a parallel convenience layer. |
| NRI | Low-level explicit API, low overhead, no hidden management/automatic barriers. | Sparkle RHI is explicit but its renderer frame graph is higher-level. | Keep this layered: RHI explicit, frame graph managed. |
| RTXDI | App owns resources, light buffers, shaders, render passes, GBuffer addressing; SDK supplies sampling/resampling math. | Sparkle owns direct lighting path natively. | Keep ownership; do not overclaim SDK equivalence. |
| RTXPT | Pure path tracer, guide buffers, path-space decomposition, DLSS-RR support, focused sample. | Sparkle has reference path tracing plus realtime/deferred and provider paths. | Declare the reference mode's purpose, determinism, accumulation/budget, supported use, and evidence role; it may be progressive, debug, or product only when those contracts prove it. |
| SHARC | Shader-only library with integration guide. | Sparkle tends to promote feature scaffolding into renderer core. | For experimental lighting/cache features, prefer shader-only/sample-only first. |
| RTXNS | Slang/neural examples, training/inference structure, capability requirements, and sample ownership are explicit. | Sparkle has Slang pipeline support but no completed neural graphics feature or model workload evidence. | Keep the ABI flexible, then implement one replacement-based feature after resource/provider contracts are crisp; do not copy RTXNS into renderer core. |
| Streamline | Include/source/shader/tools layout; release binaries outside repo; shipping integration guidance. | Sparkle has Streamline bridge in renderer provider target. | Keep bridge narrow and binary/package policy explicit. |
| Cauldron | Static rapid-prototyping framework for D3D12/Vulkan FidelityFX samples. | Sparkle has more engine/editor/tooling scope. | Treat SDK integrations as sample/provider vertical slices, not renderer-wide design drivers. |
| FidelityFX SDK | Kits/Samples/Tools/docs product split. | Sparkle has Engine/Tools/Projects/Docs and has externalized the heavy acceptance content. | Preserve cataloged optional delivery; admit tools only through an owned workflow. |
| NvRTX | Versioned validated branches and an explicitly experimental branch. | Sparkle needs honest maturity/capability status and safe defaults, not necessarily the same branch structure. | Gate, extract, or branch only according to the owning feature's delivery and validation needs. |
| Epic/Nsight/RGP frontends | Task/overview first, explicit state and selection, contextual navigation into deeper detail. | Sparkle has capable backend data but current shader/diagnostic entry surfaces risk exposing catalogs and mechanics too early. | Use intent-first actions, automatic validated defaults, one preserved selection, and contextual Diagnostics/Advanced disclosure. |

## Sparkle Construction Differences

### Sparkle Is Broader Than Most References

Vendor rendering repositories are usually narrow:

- RHI library
- rendering framework
- sample framework
- path tracing sample
- feature SDK
- shader-only library
- Unreal branch

Sparkle is broad:

- runtime engine modules
- RHI
- renderer
- editor
- application host
- launcher GUI/CLI
- shader compiler
- importer/cookers
- sample project and content
- packaging

This broadness is consistent with the declared compact renderer-first engine only while every subsystem serves a current runtime, authoring, evidence, or adoption workflow. The renderer portfolio/research goal does not independently justify general engine breadth.

### Sparkle Has Strong Internal Ownership But Too Many Observation Surfaces

Strong:

- Renderer complexity is mostly private.
- RHI backends are private.
- Shader compiler is CLI-private.
- Tool public APIs are small.

Weak:

- Renderer public API still exposes bounded memory, mesh, texture, and viewport-capture observations. Each needs a current consumer, immutable publication, bounded cost, and removal when superseded; direct RHI access is no longer exposed in the reconciled public surface.
- RHI diagnostics and capture paths must remain neutral, narrow, and owner-published; UI convenience cannot widen native ownership.
- Launcher and cooker diagnostic options/artifacts are acceptable only when explicit, bounded, and part of a documented development/evidence workflow; default incidental files remain rejected by the engineering standards.

Top-tier pattern:

- Observation is either a profiler/debugger integration, a sample UI, or an SDK-documented API.
- It is not scattered through every public surface.

### Sparkle Owns More Renderer Policy Than Donut

Donut explicitly avoids ray tracing acceleration structure ownership because AS requirements differ by application. Sparkle owns ray tracing scene/TLAS/BLAS strategy inside the renderer.

This is acceptable for an engine, but it raises the bar:

- selectable classic TLAS and PTLAS policies
- minimal PTLAS implementation on supported D3D12/Vulkan backends
- no future GPU-pack placeholders in the default renderer
- measured perf/memory evidence

### Sparkle Is More Productized Than It Is Scoped

The launcher makes Sparkle feel like a product. The renderer still feels like a research platform in places. Production repositories usually align these:

- product repo: stable release story, strict scope
- sample repo: clear tutorial/demo goals
- research repo: explicit experimental status

Sparkle should decide which one each subsystem is.

## Qualities Of A Good Game Engine Repository

Across the reviewed sources, the reusable evaluation rubric is:

- declared product identity and non-goals;
- small public surfaces and visible module/dependency ownership;
- explicit renderer/RHI/backend contracts and honest capability parity;
- cohesive feature vertical slices with isolated experimental work;
- deterministic shader, content, model, package, and release boundaries;
- workload-specific correctness, memory, performance, and hardware/driver evidence;
- reproducible adoption material another engineer can use without private context;
- deletion of superseded paths and tooling that no longer serves a product workflow.

This research does not grade Sparkle or own its backlog. Current readiness belongs to [C. Gap Assessment](../Strategy/Assessments/GapAssessment.md), priority and sequence belong to [F. Roadmap](../Strategy/Roadmap.md), and accepted local design belongs to focused Architecture documents.

## What Not To Copy

- Do not copy Donut's "not a game engine" shape if Sparkle's real goal is a game engine.
- Do not copy NVRHI's reference-counted COM-style handle model unless replacing current resource ownership would delete more code. It likely would not.
- Do not copy NRI's no-automatic-barrier philosophy into the renderer if the frame graph already owns barriers well.
- Do not add RTXDI/SHARC/RTXNS just to look modern. Add them only when resource contracts are ready and when old code can be removed.
- Do not treat a vendor sample, model, chart, or optimization recommendation as proof on Sparkle's workloads or hardware.
- Do not copy a training stack into the runtime. Own a deterministic artifact boundary and the smallest inference implementation needed by the accepted feature.
- Do not generalize a vendor/device/driver workaround. Scope it to exact evidence and retain a capability/fallback path.
- Do not mistake a polished demo for partner readiness; adoption also requires ownership, failure, packaging, and debugging contracts.
- Do not add uncataloged levels/content to prove features. Use curated levels and optional content packs.

## Research Conclusion

The references consistently reward explicit scope, one-way ownership, narrow public contracts, backend honesty, reproducible evidence, focused deletion, and concise communication of completed results. Sparkle decisions should adopt those principles only through the owning Architecture, Standards, Requirements, and Roadmap documents rather than treating this comparison as local policy.
