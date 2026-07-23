# E. External Renderer Repository Comparison

Status: source-linked external comparison
Date: 2026-07-24
Scope: vendor reference repositories/frameworks compared against SparkleEngine architecture, developer-technology transfer, path tracing, neural graphics, code construction, extensibility, productization, feature scope, and deletion-first improvement targets

## Intent

This document compares SparkleEngine against top-tier rendering repositories and SDKs. The purpose is not to copy their code. The purpose is to identify what makes those repositories production-grade or reviewable, then use that standard to slim and sharpen Sparkle.

The main lesson is not "add more features." It is "make scope and ownership painfully clear."

The supplied NVIDIA Principal Developer Technology Engineer posting adds a second comparison lens. The repository must eventually demonstrate the canonical `NV-PDTE-01` through `NV-PDTE-15` requirements in [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md): partner adoption, path tracing, a real neural graphics feature, neural model/workload tuning, low-level CPU/GPU optimization, architecture/driver diagnosis, mathematical rigor, AI fundamentals, and principal-quality communication. Vendor repositories remain precedents and study material; they do not by themselves prove Sparkle satisfies the role.

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

NvRTX code is gated behind Epic/GitHub access, so this document uses the public NVIDIA developer page for that comparison rather than claiming direct code inspection.

## External Repository Patterns

### 1. Scope Is Declared Explicitly

Donut states that it is a real-time rendering framework and explicitly says it is not a game engine. Its README describes four static-library subsystems:

- `donut_core`: math, VFS, logging, JSON/utilities, no graphics.
- `donut_engine`: scene import/maintenance, animations, materials, texture cache, descriptor tables, CVars, audio.
- `donut_render`: rendering passes.
- `donut_app`: interactive application/device/UI/camera/media utilities.

Sparkle comparison:

- Sparkle has a similar conceptual split, but it does not state a crisp product identity.
- Sparkle currently looks like an engine, renderer SDK, content pipeline, launcher, validation lab, and showcase all at once.
- A production reviewer should be able to tell what Sparkle is in one minute.

Action:

- Declare Sparkle as one of:
  - a compact renderer-first engine,
  - a renderer framework plus sample,
  - or a full game engine prototype.
- Cut or move code that serves the other identities.

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

- Document Sparkle's RHI policy in one page:
  - what RHI tracks
  - what frame graph tracks
  - what remains explicit to passes
  - what native interop is allowed for providers
  - how resource lifetimes are protected
- Do not add another abstraction. Clarify the one you have.

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

- Sparkle should keep PTLAS as a product ray tracing capability, but its implementation should stay close to the original minimal reference shape instead of spreading planner, metric, and future-pack scaffolding through the renderer.
- The problem is not PTLAS itself; the problem is letting experimental scaffolding feel like product code.

Action:

- Treat experimental features as:
  - shader-only library plus integration notes,
  - sample-only path,
  - or build/runtime-gated provider.
- Do not let experimental feature scaffolding shape the main renderer data model.

### 5. Sample Frameworks Keep Samples Separate

Cauldron is a static framework used by many FidelityFX sample projects. FidelityFX SDK separates `Kits`, `Samples`, `Tools`, and `docs`. Donut points examples to a separate Donut-Samples repository.

Sparkle comparison:

- Sparkle keeps a huge Showcase project and content inside the main depot.
- This gives reviewers immediate demo value, but it dominates depot size and slows every clone/review.

Action:

- Move heavy Showcase content to optional content delivery:
  - separate repository,
  - release asset pack,
  - Git LFS,
  - or media delivery script.
- Keep a curated in-repo level set for build/runtime smoke and review, while arbitrary additional levels remain selectable through manifests/content packs.

### 6. Product Repos Separate Validated And Experimental Branches

NvRTX publicly separates Unreal Engine version branches and an experimental branch. The public page states the experimental branch is not fully validated and can contain limitations/regressions not suitable for shipping.

Sparkle comparison:

- Sparkle currently keeps experimental and product code paths in the same default architecture.
- PTLAS is the clearest example where product capability and experimental scaffolding need separation.

Action:

- Add the same distinction inside the repo:
  - product path
  - developer preview
  - research/experimental
- Shipping defaults should not depend on research scaffolding.

### 7. Binary/Source Distribution Is Treated As Product Policy

Streamline's README separates GitHub source from release binary artifacts. It states that shipping developers usually use prebuilt directories/releases and that rebuilding Streamline from source is optional.

Sparkle comparison:

- Sparkle's release assembly CMake exists, but packaging/product policy is not yet as crisp as the code.
- The launcher has package workflows, but the repo also carries large content and diagnostics artifacts.

Action:

- Define release package ownership:
  - runtime package
  - editor package
  - symbols package
  - development tools package
  - optional content package
- Delete packaging code that is not going to be owned.

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
- Current internal documentation is extensive, but principal developer-technology evidence also requires a concise handoff, reduced reproducer, live demo, and result-focused technical note.

Action:

- Treat one future neural graphics feature as a technology-transfer vertical slice, not a framework milestone.
- Record requirements, classical baseline, model/operator provenance, tensor shape/layout/precision, training/export, runtime integration, capability/fallback, D3D12/Vulkan behavior, quality, latency, memory, and hardware limits.
- Produce a minimal adoption guide and reproducible workload after the implementation is real.
- Keep all training/offline dependencies out of the runtime package and delete experimental scaffolding that the accepted feature does not need.
- Preserve negative results and rejected layouts/precision/concurrency choices so the guidance reflects engineering judgment.

## Direct Comparison Matrix

| Reference | Production pattern | Sparkle today | Improvement |
| --- | --- | --- | --- |
| Donut | Framework explicitly not a game engine; four module libraries; app/device management separated from render passes. | Similar module idea, but Sparkle's product identity is wider and less declared. | Write a one-page product identity and cut non-matching workflows. |
| NVRHI | Higher-level RHI with resource/lifetime/state/barrier helpers and native escape hatches. | RHI plus renderer frame graph gives similar helper behavior, but policy is implicit. | Document automation boundaries and native interop rules. |
| NRI | Low-level explicit API, low overhead, no hidden management/automatic barriers. | Sparkle RHI is explicit but its renderer frame graph is higher-level. | Keep this layered: RHI explicit, frame graph managed. |
| RTXDI | App owns resources, light buffers, shaders, render passes, GBuffer addressing; SDK supplies sampling/resampling math. | Sparkle owns direct lighting path natively. | Keep ownership; do not overclaim SDK equivalence. |
| RTXPT | Pure path tracer, guide buffers, path-space decomposition, DLSS-RR support, focused sample. | Sparkle has reference path tracing but also realtime/deferred path and provider handoff. | Make reference mode clearly offline/progressive or clearly debug. |
| SHARC | Shader-only library with integration guide. | Sparkle tends to promote feature scaffolding into renderer core. | For experimental lighting/cache features, prefer shader-only/sample-only first. |
| RTXNS | Slang/neural examples, training/inference structure, capability requirements, and sample ownership are explicit. | Sparkle has Slang pipeline support but no completed neural graphics feature or model workload evidence. | Keep the ABI flexible, then implement one replacement-based feature after resource/provider contracts are crisp; do not copy RTXNS into renderer core. |
| Streamline | Include/source/shader/tools layout; release binaries outside repo; shipping integration guidance. | Sparkle has Streamline bridge in renderer provider target. | Keep bridge narrow and binary/package policy explicit. |
| Cauldron | Static rapid-prototyping framework for D3D12/Vulkan FidelityFX samples. | Sparkle has more engine/editor/tooling scope. | Treat SDK integrations as sample/provider vertical slices, not renderer-wide design drivers. |
| FidelityFX SDK | Kits/Samples/Tools/docs product split. | Sparkle has Engine/Tools/Projects/Docs but content and workflows are intermingled. | Move uncataloged heavy media and optional tools out of default runtime product. |
| NvRTX | Versioned branches and experimental branch separated. | Experimental features live in normal renderer paths. | Gate or extract experimental systems. |

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

This broadness is valuable only if the product is "small engine." If the product is "rendering portfolio/research engine," it is too much.

### Sparkle Has Strong Internal Ownership But Too Many Observation Surfaces

Strong:

- Renderer complexity is mostly private.
- RHI backends are private.
- Shader compiler is CLI-private.
- Tool public APIs are small.

Weak:

- Renderer public API exposes memory, mesh, texture diagnostics, viewport BMP capture, and direct RHI access.
- RHI public API exposes broad diagnostics; screenshot/BMP capture should remain, but with narrow ownership and low runtime cost.
- Launcher exposes debug artifacts and diagnostic-flavored options.
- Cookers write plan/timing diagnostics by default.

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

The launcher and package assembly make Sparkle feel like a product. The renderer still feels like a research platform in places. Production repositories usually align these:

- product repo: stable release story, strict scope
- sample repo: clear tutorial/demo goals
- research repo: explicit experimental status

Sparkle should decide which one each subsystem is.

## Qualities Of A Good Game Engine Repository

A strong game engine repo has:

| Quality | Production expectation | Sparkle grade |
| --- | --- | ---: |
| Declared product identity | The repo says exactly what it is and is not. | 2.5 |
| Small public API | Public headers are stable contracts, not convenience/status/report code. | 3.2 |
| Strong module boundaries | Ownership follows build modules and include direction. | 4.0 |
| Renderer graph clarity | Pass/resource/history ownership is visible and predictable. | 3.8 |
| RHI explicitness | D3D12/Vulkan concepts remain recognizable. | 4.0 |
| Backend parity honesty | Feature/support matrix names what works per backend. | 3.0 |
| Feature vertical slices | Each feature owns settings, resources, passes, shaders, fallback. | 3.4 |
| Experimental isolation | Research code is gated/sample-only/shader-only. | 2.4 |
| Shader ABI discipline | Source, reflection, package, runtime layout align. | 4.4 |
| Content delivery discipline | Heavy media does not dominate the core repo. | 1.5 |
| Tooling focus | Tools serve current product workflows only. | 2.8 |
| Memory/perf evidence | Memory budgets and CPU/GPU timings guide decisions. | 3.2 |
| Package/release contract | Runtime/editor/dev/content packages are intentional. | 3.0 |
| Deletion culture | Stale scaffolding is removed aggressively. | 3.5 |
| Partner adoption quality | Integration prerequisites, contracts, fallback, repro, and handoff are usable outside the authoring team. | 2.0 |
| Neural graphics evidence | A real model/operator feature reports artifact provenance, quality, runtime cost, and fallback. | 1.0 |
| Math and performance modeling | Algorithm derivation and predicted cost are checked against implementation and captures. | 2.0 |
| Hardware/driver diagnosis | Exact configuration, native validation, reduced reproducers, and scoped conclusions distinguish app from driver behavior. | 2.8 |
| Technical communication | Completed strategic work has a concise case study, live demo, and whitepaper/talk-quality explanation. | 2.5 |

## Gaps To Close Before Sparkle Feels Top-Tier

1. Declare product scope and non-goals.
2. Catalog Showcase levels and externalize heavyweight assets.
3. Remove default-path validation/report/debug artifact systems.
4. Refactor PTLAS into a minimal product path while deleting research scaffolding.
5. Shrink public diagnostics APIs while preserving hardened screenshot/BMP capture.
6. Make RHI policy explicit: low-level service layer plus managed renderer frame graph.
7. Choose one release runtime smoke/demo path rather than many validation paths.
8. Collapse launcher to current workflows.
9. Keep shader compiler strong but make debug artifacts opt-in or external.
10. Keep profiler/debug-layer and screenshot/BMP capture support while deleting bespoke reports.
11. Implement one real neural graphics feature only after its owning renderer/resource/artifact contracts are ready; readiness alone does not meet the final target.
12. Add deterministic model provenance/export/cook and separate training versus inference performance evidence without adding a general ML runtime.
13. Build one partner-shaped adoption case with explicit prerequisites, fallback, reduced repro, hardware/driver matrix, and handoff guidance.
14. Tie material rendering/neural algorithms to math/reference tests and predicted-versus-measured CPU/GPU cost.
15. Convert one completed strategic result into a concise live demo, whitepaper-quality note, and talk outline.

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

## Bottom Line

Sparkle is architecturally credible, especially in RHI, frame graph, and shader pipeline. It is not yet as sharp as the best reference repositories because its scope is too wide and its observation/test/report surfaces are too durable.

To reach that level, Sparkle should remove more than it adds:

- fewer public APIs
- fewer launcher/debug options
- fewer diagnostic artifacts
- fewer default experimental paths
- less content in the depot
- clearer feature ownership
- stronger release identity
- one real path from neural model/operator math through deterministic artifacts to optimized renderer execution
- partner-quality integration and driver/hardware diagnosis
- concise, reproducible communication of completed results
