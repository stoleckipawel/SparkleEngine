# Renderer Pipeline Materialization and Typed Binding

Status: current feature dossier; source-backed, not backend-validation, hot-reload stress, or release evidence

Verified: 2026-09-06 against committed `master` revision `baa0adc9`; current parameter, pipeline, shader-generation, CMake, and RHI service routes inspected

Scope: `REN-PIPE-01` through `REN-PIPE-05`; owns the bridge from registered typed pass contracts and cooked shader metadata to binding layouts, graphics/compute/ray pipelines, per-pass bindings, runtime caches, and completion-safe shader-generation replacement

## Feature Contract

Frame-graph declarations describe semantic work; cooked shader artifacts contain backend code and reflected metadata; RHI owns native pipeline objects. Renderer pipeline materialization is the checked bridge between them. A pass may record only after its typed parameter layout, registered shader contract, active cooked-map entry, backend capabilities, render state, attachments, and binding values agree.

```text
typed C++ pass struct + shader registration
  -> cook-time parameter signature and shader-map entry
  -> active generation validation
  -> Renderer binding layout + graphics/compute/ray pipeline
  -> PassBinder resolves current graph/native resources
  -> RHI command binding and draw/dispatch/TraceRays
```

The benefit is fail-fast ABI and state validation with one cache owner. Feature passes do not hand-build backend root signatures/descriptor layouts, and a shader reload cannot partially replace only some materialized programs.

## Build And Registration Membership

| Target/owner | Current role | Boundary |
| --- | --- | --- |
| `SparkleRendererShaderRegistrations` | compiles Renderer registration descriptors for runtime use | linked privately for shared builds or through the interface object target for static builds |
| `SparkleRendererShaderCookRegistrations` | compiles the same descriptors with `SPARKLE_SHADER_CONTRACTS_ONLY=1` | exposes contracts to ShaderCompiler without runtime auto-registration side effects |
| `SparkleRenderer` | owns typed parameter instances, runtime materialization/cache, binding, graph execution, and reload facade | public dependencies are Core/RHI; Platform/GameFramework/Tasks/provider/imgui stay private |
| RHI pipeline service | creates backend binding layouts and graphics/compute/ray pipeline/table objects | does not select Renderer feature technique, shader composition, or graph topology |

The [Shader Program Catalog](ShaderPrograms.md) owns exact program membership and entry points. This dossier owns how a selected program contract becomes executable. The [Ray-Tracing dossier](RayTracing/README.md) owns effect semantics and scene-to-SBT identity; this dossier owns checked native materialization.

## Parameter And Binding Model

Typed pass structures register uniform buffers, texture/buffer SRV/UAVs, samplers, acceleration structures, push constants, and raster attachments. The cooked parameter signature and runtime `PassParameterLayout` must match the registration and active map entry.

`PassBinder` has graphics, compute, and ray domains. It can bind a full compiled layout or named subsets and resolves:

- uniform data through frame upload allocation;
- graph textures/buffers through current SRV/UAV descriptor resolution;
- acceleration-structure handles through graph resource commands;
- shared/unique sampler tables through the descriptor service;
- explicit address/table/push-constant overrides where the contract requires native or externally supplied bindings.

Missing bindings, incompatible value kinds, attachment-as-SRV misuse, wrong array cardinality, unresolved resources/descriptors, absent required overrides, unknown named bindings, layout mismatch, or unsupported compiled binding type are fatal contract violations. There is no silent null binding policy.

## Runtime Materialization Matrix

| Pipeline kind | Key/materialization input | Current cache/lifetime behavior |
| --- | --- | --- |
| graphics | active shader generation and VS/PS code hashes; binding-layout signature; blend, rasterizer, depth, stencil, topology, vertex input; color/depth formats and sample count | shader pair owns a map of complete `GraphicsPipelineKey` to pipeline; changing any state cell materializes a distinct object |
| compute | active shader map entry/code, parameter signature, capability requirements, optional configured pipeline description | one typed runtime storage per shader/generation |
| ray tracing | selected registered raygen/miss/hit/callable composition, parameter contract, payload/attribute/recursion/local-record metadata, capability, generation | composition-keyed runtime plus checked shader-table creation; scene table requires one opaque and one alpha-tested group and a valid plan |

Backend capability validation currently rejects acceleration-structure, inline-ray-query, or descriptor-indexing requirements when the selected RHI capability report cannot satisfy them. Native creation returning no layout/pipeline/table is an explicit error.

## Shader Generation And Reload

Generation 1 opens the cooked shader library and global shader map and validates every registered shader type for identity, entry point, stage, feature flags, ray metadata, local-record contract, and parameter signature. A reload:

1. opens and validates a complete replacement generation;
2. recreates every runtime holder already materialized by the active generation;
3. leaves the active generation unchanged if any replacement validation/materialization throws;
4. atomically swaps only after the replacement is complete;
5. retires the prior generation with the last submitted token for every queue and polls until all are complete.

Reload does not prove source compilation occurred, that the ShaderCompiler published a coherent artifact set, or that every feature pass was exercised. Those are cross-module publication and feature evidence concerns.

## Horizontal Coverage

| Axis | Required cells | Invariant |
| --- | --- | --- |
| pipeline | graphics, compute, native ray; every active graphics state/attachment signature | cache identity contains every native-semantic input and never aliases incompatible state |
| binding | uniform, SRV/UAV texture, SRV/UAV buffer, sampler, acceleration structure, push constant, explicit override, named subset | typed/runtime/cooked layouts agree and the current resource is bound in the correct domain |
| lifecycle | first materialization, cache hit, graph rebuild, reload success/failure, in-flight retirement, generation exhaustion/shutdown | no partial generation or early pipeline destruction |
| backend | D3D12/Vulkan and supported/unsupported capability cells | rejection is explicit; backend-native objects implement the same Renderer contract |
| build | shared/static runtime and contract-only ShaderCompiler consumption | registration is present exactly once with no runtime side effect in contract-only mode |

## Acceptance Criteria

- `AC-PIP-01` — every registered runtime shader resolves through the active target map/library and matches name, entry, stage, features, ray metadata, local-record contract, and typed parameter signature.
- `AC-PIP-02` — graphics cache identity changes for every blend/raster/depth/stencil/topology/vertex-input/attachment/sample/shader/layout difference and reuses only an exactly equal request.
- `AC-PIP-03` — compute and ray materialization reject unsupported capabilities and invalid compositions before command recording; valid objects carry the active generation.
- `AC-PIP-04` — every parameter field binds the intended current resource/value in graphics, compute, and ray domains; missing, incompatible, or unresolved values fail before draw/dispatch/trace.
- `AC-PIP-05` — reload activates all replacement runtime holders atomically, preserves the previous active generation on failure, and retires replaced objects only after all queue tokens complete.
- `AC-PIP-06` — shared/static Renderer and contract-only ShaderCompiler builds contain the intended registration set exactly once.
- `AC-PIP-07` — D3D12 and Vulkan native validation accept the representative pipeline/binding matrix with identical semantic outputs.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-PIP-01` | missing/duplicate registration, missing map/code entry, or metadata/signature mismatch | generation fails before activation and identifies shader/contract | `CHK-PIP-01` |
| `FM-PIP-02` | omit one graphics-key field or request unsupported backend capability | incompatible pipeline is never reused/created as if supported | `CHK-PIP-02` |
| `FM-PIP-03` | missing/wrong parameter, array count, descriptor, AS, override, or named binding | binding fails before command emission with field/domain reason | `CHK-PIP-03` |
| `FM-PIP-04` | invalid ray composition/table plan/local record | no pipeline/table reaches TraceRays; exact composition defect is reported | `CHK-PIP-02`, `CHK-PIP-03` |
| `FM-PIP-05` | replacement holder creation fails or queue completion stalls | old generation stays active or retired resources remain alive and measurable until safe | `CHK-PIP-04` |
| `FM-PIP-06` | registration omitted/duplicated across shared/static/contract-only build route | membership check or minimal link/cook target fails | `CHK-PIP-05` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-PIP-01` | enumerate registrations and compare all active map/library metadata and computed parameter signatures | `AC-PIP-01`; `FM-PIP-01` |
| `CHK-PIP-02` | focused materialization matrix varying one complete key/capability/composition field at a time; inspect cache identity and rejection | `AC-PIP-02`, `AC-PIP-03`; `FM-PIP-02`, `FM-PIP-04` |
| `CHK-PIP-03` | typed binder harness covering every field/domain plus each missing/incompatible/unresolved/override/table failure | `AC-PIP-04`; `FM-PIP-03`, `FM-PIP-04` |
| `CHK-PIP-04` | in-flight reload success/failure stress with delayed graphics/compute/copy completion; inspect active/retired generations and native validation | `AC-PIP-05`, `AC-PIP-07`; `FM-PIP-05` |
| `CHK-PIP-05` | configure/build the smallest shared, static, runtime-registration, and contract-only registration consumers; enumerate linked registrations | `AC-PIP-06`; `FM-PIP-06` |

This contract is **defined but unproved**. Source-level validation paths and caches do not establish native correctness, reload boundedness, build-route uniqueness, or feature output parity.

## Primary Source Routes

- [`RenderPassRuntimeCache.h`](../../../../../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.h) and [`RenderPassRuntimeCache.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [`PipelineRuntimeLibrary.cpp`](../../../../../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.cpp)
- [`GraphicsPipelineMaterialization.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/GraphicsPipelineMaterialization.cpp)
- [`RayTracingPipelineRuntime.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/RayTracingPipelineRuntime.cpp)
- [`PassBinder.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/PassBinder.cpp) and [`PassBinderCommands.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/PassBinderCommands.cpp)
- [`PassParameterSet.cpp`](../../../../../../Engine/Renderer/Private/ShaderParameters/PassParameterSet.cpp)
- [`CMakeLists.txt`](../../../../../../Engine/Renderer/CMakeLists.txt)
