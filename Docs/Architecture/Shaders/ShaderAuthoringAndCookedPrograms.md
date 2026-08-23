# Shader Authoring and Cooked Shader Architecture

Status: plan and implementation authority for unified shader, graphics-pipeline, and ray-tracing delivery; Phase 0 is frozen and the current unstaged `master` worktree contains the Phase 1-2 source-consistency checkpoints, while the complete target and executable proof still require Phases 3-12
Current audit: 2026-08-23 against the unstaged `master` worktree based on `5b0bd1469339897eef1fde3e5c9ab07137860d0f`
Scope: one clean-break delivery covers virtual sources, lean shader classes and parameters, compilation/validation without persistent compile-result caching, global shader map/code library publication, typed raster/compute/ray-tracing lookup, granular raster intent and attachment-derived graphics pipeline materialization, paired D3D12/Vulkan ray-tracing pipelines and shader tables, graph draw/dispatch/trace, dual-execution effects, reload, diagnostics, and evidence; permutations, precaching/prewarming, preload/streaming, and native driver caches remain deferred follow-ups

## Purpose

This document decides how SparkleEngine should identify render passes and shaders without requiring authors to maintain parallel strings such as `DirectLighting`, `RendererShaderPackages::DirectLighting`, binding-layout names, pipeline names, source basenames, and cooked-package names.

It also maps Epic's global-shader lifecycle onto Sparkle from source import through runtime pipeline creation, explains which Unreal patterns are worth adopting, traces the design to Sparkle's engineering and portfolio requirements, and defines atomic implementation phases for the migration. It refines the shader-specific conclusions from [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md). That comparison remains the source-linked broad research document. This document owns the target shader-authoring architecture, current shader-lifecycle audit, implementation sequence, clean-break boundaries, and final acceptance gates. Code, tests, executable build configuration, and captured evidence remain the authority for what is implemented and proven today.

The implementation phases apply directly to the unstaged `master` worktree. They do not authorize creating or switching branches, staging, committing, pushing, or submitting; the user owns review and every source-control action. Phase 0 is documentation-only; Phases 1-3 are source-consistency checkpoints; Phases 4-10 run the focused executable evidence required by their completed vertical slices; Phase 11 performs the final semantic legacy/compatibility eradication audit, and Phase 12 validates the complete candidate after every obsolete authoring, package, parameter, cook, runtime, graphics-state, RT capability, effect-selection, and frontend path is gone.

The [Ray-Tracing Pipeline and Dual-Execution Target Architecture](RayTracingPipelineImplementationPlan.md) owns the ray-query/pipeline target semantics, effect-level selection contract, shader-table indexing, and lifetime invariants. This document is the sole owner of implementation phases, prompts, references, CL boundaries, and acceptance order for both systems.

## Adversarial Review Verdict

The 2026-08-15 review assumed every proposed Unreal/vendor analogy was wrong until a primary source, current Sparkle code, and a local requirement established its exact scope. The result keeps the end-to-end direction but narrows several earlier overclaims:

| Challenged claim | Verdict and proof boundary |
| --- | --- |
| The shader filename can replace pass identity. | Rejected. Epic global shaders, Donut/NVRHI-style shader creation, current multi-entry/multi-stage cases, and Sparkle's `GBuffer` prove source location, entry, shader stage, pipeline composition, and graph operation are different identities. |
| Every pass needs a handwritten `PassName`. | Rejected. The semantic event label is necessary, but the one-to-one default can derive from the concrete shader class; only instance-specific text remains authored at scheduling. |
| Authors need named shader packages. | Rejected. Authors need concrete shader classes and cooked runtime artifacts; physical map/library membership is generated delivery policy. |
| One shader parameter struct must be every pass struct. | Overconstrained and corrected. Epic RDG intentionally reuses shader parameters for common one-to-one passes but also supports pass parameters without shader semantics. Sparkle selects one owner per shader-visible field plus explicit pass-envelope composition. |
| A compile-input hash requires a persistent compile-result cache. | Rejected. Sparkle needs deterministic input identity for diagnostics, deduplication within one operation, and provenance, but every selected job still invokes the compiler. |
| A deduplicated compressed/streamed physical code library is always required. | Not proven for the current catalog. Exact hashes, maps, and code-record schemas are required; physical merging, compression, chunks, and streaming require measured byte/I/O value. |
| D3D12 pipeline libraries and a long-lived shader/module cache are universal baseline best practice. | Rejected as universal. Complete pipeline descriptions, correct lazy graph-time materialization, and first-use measurement are the base contract. Async preparation, native caches, and shader/module object retention are backend/capability-specific measured follow-ups. |
| A graphics graph caller should author one complete pipeline-state aggregate. | Rejected. Epic keeps a complete RHI initializer internally, but mesh-pass code supplies narrow render-state overrides, mesh/material processing supplies vertex/raster facts, and RDG attachments supply target compatibility. Sparkle's current `GraphicsShaderPipelineState` duplicates graph-owned formats and exposes backend-shaped mechanics at the feature call. |

“Epic-aligned” therefore means matching responsibility boundaries, invariants, failure behavior, and authoring ergonomics—not copying class names, macro volume, material-system scale, or every optional cache.

## Decision Summary

1. Sparkle should adopt Unreal's global-shader flow as its core mental model: one concrete shader class owns its parameter contract and compile hooks; one implementation declaration supplies virtual source, entry point, and stage; cooking produces a global shader map and code records; runtime resolves a typed shader reference from the active map.
2. A one-shader compute pass does not need an authored program alias, pass-registration macro, duplicate pass-parameter struct, or forwarding pass class. The frame graph should accept the shader type, its nested `Parameters`, a diagnostic label only when the generated default is insufficient, and the dispatch dimensions.
3. Graphics work names the concrete vertex/pixel shader types at the draw site and supplies a real draw collaborator. Pass code sets only narrow blend/depth/stencil intent; graph attachments, mesh geometry, material policy, shader references, and the runtime pipeline owner contribute the rest. Ray tracing uses one narrowly scoped `RayTracingPipelineComposition` only because exports and hit groups form a real multi-stage execution unit. Sparkle does not add a universal authored `ShaderProgram` or frontend pipeline-description abstraction.
4. A shader class declares nested `Parameters` only when that shader is a direct graph-dispatch binding owner. Compute and ray-generation shaders normally own them; miss, hit, intersection, and callable shaders do not repeat the ray-generation root schema. The frame graph consumes the selected dispatch shader's schema to declare resource access, and runtime binding/reflection validation derive from it. A multi-shader, shaderless, or local-record owner may add one small envelope only for fields that no shader root schema already owns.
5. A semantic render-pass label remains necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling, but it does not select code. Generate the one-to-one default from the shader type and accept an instance override for mip, cascade, phase, view, or repeated use.
6. A shader filename is a virtual source input, not the pass, shader-map, pipeline, or diagnostic identity.
7. Sparkle should keep cooked shader data but replace handwritten packages with two generated authorities: `GlobalShaderMap` for typed logical lookup and `CookedShaderLibrary` for validated code records addressed by hash. The runtime pipeline owner holds generation-safe binding layouts and lazily materialized compute/graphics pipelines derived from active-map shader references plus complete internal state keys.
8. Permutation infrastructure is deliberately postponed until after the non-permuted shader-map path is complete and accepted. The base map key is `(ShaderTypeId, Target)`; a later measured follow-up may extend it with a typed `PermutationId` without changing the lean one-variant authoring path.
9. PSO precaching/prewarming, preload controls, and driver-cache integration remain outside this implementation plan. Phase 5 replaces eager speculative variants with exact lazy materialization before command recording; add earlier preparation only after measured first-use hitch evidence justifies it.
10. Shader Tools should center `Apply Changed`, semantic shader/source selection, one operation state, source-located errors, and contextual next actions. Package/layout IDs, hashes, raw artifacts, full rebuild/reload, backend flags, and runtime materialization mechanics remain expert details.
11. Inline ray queries and full ray-tracing pipelines remain different execution systems that share shader identity, parameters, maps, scene/TLAS/material data, semantic HLSL kernels, runtime generation, and retirement. Their native invocation and SBT mechanisms remain distinct, while this one plan delivers both without a parallel shader architecture.
12. Classic TLAS versus partitioned TLAS and descriptor encoding versus device-address storage are RHI binding mechanics, not shader identities or effect choices. One shader class declares one semantic acceleration-structure parameter and one HLSL entry. The selected backend/provider lowers that parameter to its exact native descriptor representation. Sparkle deletes the duplicate device-address shader, raw-address shader uniform fields, access-mode frontend, and no-query shadow shader; it does not replace them with a hidden compiler variant or author-facing permutation.
13. Shader resources use explicit view vocabulary: `CreateSRV` for read-only texture/buffer views and `CreateUAV` for writable texture/buffer views. Acceleration structures use one semantic `CreateAccelerationStructureBinding`; raster/depth outputs use neutral render-target/depth-target attachment bindings. Generic `Read` aliases and neutral `CreateRTV` / `CreateDSV` spellings do not survive.
14. Raster attachments carry load/store/clear and depth/stencil access. Their graph resource descriptions derive target formats/count/sample compatibility automatically. `GpuMesh` supplies vertex input and topology; the mesh-pass owner supplies material-dependent fill/cull and pass render state; the pipeline owner assembles and keys the complete neutral descriptor. No caller repeats these facts.

The short answer is therefore:

> Make the shader class the lean authoring unit, keep its parameters inside the class, remove manual package/program/pass duplication, resolve it through a generated global shader map, and let the frame graph dispatch that shader directly with the same typed parameters.

## Why the Shader Filename Is Not the Pass Name

A filename answers "where is source text?" A pass name answers "what GPU operation is this?" Coupling them creates incorrect behavior in ordinary cases:

- One source file can contain several entry points or stages.
- One graphics pipeline can use shader stages from several files. Sparkle's current `GBuffer` package already uses `GBufferVS.hlsl` and `GBufferPS.hlsl`.
- The same compiled shader can be reused by multiple semantic passes.
- A pass type can be scheduled several times with instance-specific labels such as a mip number, eye, cascade, phase, or view.
- Renaming or moving a source file should invalidate compilation, but it should not silently rename profiler history, GPU markers, frame-graph nodes, or the shader type that references it.
- Two directories can contain the same source basename. Sparkle's current basename-derived fallback cannot distinguish them.
- Include files and shader libraries are source dependencies but are not independently executable passes.

Using a filename as a default shader debug name is reasonable. Using it as the durable shader-type, pass, pipeline, layout, and artifact identity is not.

## Required Identity Model

Each identity has one responsibility and one owner.

| Identity | Meaning | Proposed authority | Must not be derived solely from |
| --- | --- | --- | --- |
| Render-pass label | frame-graph, GPU marker, diagnostic, and profiler text | generated from the one-to-one shader type or supplied as an instance override | cooked artifact name |
| Shader type | one compilable source/entry/stage plus its nested parameter contract | concrete shader class and implementation declaration | pass label |
| Source identity | normalized virtual path plus transitive include closure | shader compiler dependency graph | basename |
| Entry identity | source path, entry point, and stage | typed shader registration | source path alone |
| Binding layout | full structural binding contract | generated parameter/reflection contract hash | parameter count |
| Cooked artifact | backend binaries, reflection, layouts, features, and provenance | cooker-generated content key | pass label or filename |
| Shader-map entry | target-specific typed lookup from shader class to validated code record | generated `GlobalShaderMap` | source basename or authored package |
| Pipeline | concrete shader references plus fixed-function and target state | complete pipeline description/cache key assembled by the runtime materialization owner | one shader, diagnostic label, or caller-authored partial aggregate alone |

Pass labels and debug names may be human-readable. Lookup and reuse identities must be typed or hash-backed and collision checked.

## Target Shape

```text
  AUTHORING                         COOKED/RUNTIME                    FRAME GRAPH
  =========                         ==============                    ===========

  DirectLightingCS                  GlobalShaderCatalog              AllocParameters<DirectLightingCS>()
  + virtual source/entry/stage ---> ShaderCompileJob                 fill typed inputs/outputs
  + nested Parameters               bounded compile jobs             Dispatch<DirectLightingCS>()
  + optional compile hooks          GlobalShaderMap                  optional instance label
            |                       CookedShaderLibrary                        |
            |                                  |                              |
            +---------------------------------> ShaderRef<DirectLightingCS> <---+
                                               |
                                               v
                                      RenderPassRuntimeCache
                                      binding layout / pipeline
                                               |
                                               v
                                      bind declared resources
                                      record draw/dispatch
```

There is one lookup chain, not a catalog plus handwritten program registry plus package cache. `GlobalShaderCatalog` describes what can be compiled. `GlobalShaderMap` is the active target-specific typed lookup. `CookedShaderLibrary` supplies validated bytes by hash. `RenderPassRuntimeCache` derives generation-bound layouts and pipelines from shader references plus the actual draw/dispatch description. The RHI owns neutral descriptors, record validation primitives, and backend objects; Renderer owns concrete shader classes, graph use, and runtime generation policy; ShaderCompiler owns source dependency, bounded compilation, map/library generation, and transactional publication.

## Unreal-Aligned System Model

Unreal does not have one object called a "shader package" that owns every concern. Its useful architectural pattern is a pipeline of distinct authorities. Sparkle should preserve that separation even when its implementation is much smaller.

| Unreal concept | Responsibility | Sparkle target | Current Sparkle approximation |
| --- | --- | --- | --- |
| virtual shader source paths | stable source namespace independent of checkout location | `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots | project-first then engine physical path search |
| `FShaderType` / `FGlobalShaderType` | immutable shader metadata and compile hooks | `ShaderTypeDesc` emitted by typed registration | `ShaderRegistrationDesc` |
| `TShaderPermutationDomain` | typed, bounded permutation dimensions and stable IDs | deferred follow-up after the one-variant shader-map path is accepted | free-form compile defines and package variants |
| `FShaderCompilerInput` | complete read-only input for one compilation | `ShaderCompileRequest` | `ShaderCompileOptions` plus cook-node metadata |
| `FShaderCompileJob` / `FShaderCompileJobKey` / input hash | scheduled unit and logical shader/target key, plus a separate hash over all compiler-affecting inputs | `ShaderCompileJob` plus `ShaderCompileInputHash`; a small logical job ID may exist only for scheduling | `CookNode` plus `TaskExecutor` task |
| `FShaderCompilingManager` and Shader Compile Workers | asynchronous coordination and compiler-process isolation | compile-job coordination on the existing cooker `TaskExecutor`; optional worker processes only when justified | one out-of-process cooker with bounded in-process stage tasks |
| Global Shader Map / `TShaderMapRef<T>` | typed target-specific lookup and shader lifetime | `GlobalShaderMap` / `ShaderRef<Shader>` | pass runtime cache loading by package ID |
| `FShaderMapResourceCode` | code hashes and map resource content | generated map resource record | code embedded in each `.sparkshader` program package |
| `FShaderCodeLibrary` | cook-time collection of unique code and runtime loading by hash | `CookedShaderLibrary` | no cross-package code library |
| `FShaderPipelineType` | optional declared stage grouping | no universal authoring abstraction; graphics names stage types at the draw site and ray tracing alone uses a focused typed pipeline composition | stages grouped by repeated package string |
| RDG shader/pass parameter structs | shader parameters can directly serve a one-to-one pass; pass envelopes and shaderless pass parameters are also valid | one shader-visible `Parameters` schema, reused or composed into a pass envelope without duplicating shader fields | separate pass parameters and registered shader parameters |
| RDG event name | diagnostic/profiler identity of one graph operation | generated default `RenderPassLabel`, optional instance override | repeated `PassName` literal |
| `FMeshPassProcessorRenderState` | narrow pass-wide blend, depth/stencil, access, stencil-reference, and uniform-buffer overrides | smaller `RasterPassRenderState` containing only semantic blend/depth-stencil and dynamic stencil-reference choices; Sparkle attachment access stays graph-owned | caller-authored `GraphicsShaderPipelineState` containing attachment and mesh facts |
| `FGraphicsMinimalPipelineStateInitializer` | mesh-draw fixed-function and shader state without render-target state | internal `GraphicsPipelineKey` inputs contributed by shader references, mesh/material facts, and pass render state | one state value cached only by the vertex/pixel shader type pair |
| `FGraphicsPipelineRenderTargetsInfo` / `ExtractRenderTargetsInfo` | render-target information extracted from RDG attachment bindings | compatibility signature derived from graph resource descriptions; load/store/clear/access remain graph execution and validation facts | formats, count, and depth format copied manually beside the same graph attachments |
| `FGraphicsPipelineStateInitializer` / `SetGraphicsPipelineState` | complete RHI-facing state and final materialization/binding | private complete `GraphicsPipelineDesc` assembled and lowered by Renderer/RHI owners | backend defaults fill omitted blend, sample, input-assembly, and raster facts |
| PSO precache / shader pipeline cache | earlier pipeline preparation and hitch tracking | deferred until current lazy materialization shows a measured product hitch | pipeline creation coupled to pass runtime creation |

The mapping is architectural, not a request to copy Unreal class names or source code. Sparkle should keep names that fit its own standards while retaining the responsibility boundaries.

### Full Lifecycle

```text
AUTHORING
  shader class
    + nested Parameters when it owns direct graph bindings
    + optional compile eligibility/environment/validation hooks
  IMPLEMENT_GLOBAL_SHADER(class, virtual source, entry, stage)
                         |
                         v
CATALOG / COMPILE
  validate and freeze ShaderTypeDesc records
  create one ShaderCompileJob per (ShaderTypeId, Target)
  preprocess virtual include closure -> input hash -> bounded compile
  verify reflection against Shader::Parameters
                         |
                         v
COOK / PUBLICATION
  GlobalShaderMap: (ShaderTypeId, Target) -> ShaderCodeHash + ABI metadata
  CookedShaderLibrary: ShaderCodeHash -> validated backend code record
  development provenance: source/dependencies/compiler/symbols
  publish the complete generation transactionally
                         |
                         v
RUNTIME / FRAME GRAPH
  open map + library -> ShaderRef<Shader>
  AllocParameters<Shader>() -> fill declared graph inputs/outputs
  Dispatch<Shader>(parameters, groupCount) or Draw<VS, PS>(parameters, draws)
  RenderPassRuntimeCache lazily materializes generation-bound layout/pipeline before recording
  Execute binds only declared resources and records commands
                         |
                         v
DEVELOPMENT RELOAD
  changed virtual paths -> reverse dependencies -> affected shader jobs
  complete replacement map/library -> validate -> atomic swap
  old generation retires after all recorded GPU submissions complete
```

### The Separation That Prevents Bloat

The lean design is not "compile every file automatically." It is automatic generation from small, explicit declarations:

- source files provide implementation text and includes;
- shader classes provide executable entry points, direct-binding parameter metadata only where used, and only the compile hooks they actually need;
- graph dispatch names the concrete shader type(s) and supplies execution dimensions or draw work;
- parameter metadata provides the shared graph-resource and shader-binding contract;
- shader maps provide logical typed lookup;
- code records provide exact hashed physical delivery; a library may merge duplicate blobs when measured useful;
- pipeline descriptors add fixed-function state;
- render-pass labels provide diagnostics only.

This is why filenames cannot safely replace shader types, graph operations, or pipelines. Automation derives map identity, code-library records, binding metadata, and runtime lookup. Authors state only facts the system cannot infer: class, virtual source, entry, stage, parameter fields, optional capability policy, and the actual dispatch/draw request.

## Proposed Authoring Experience

Unreal's production pattern is intentionally direct: the shader class declares `FParameters`; `IMPLEMENT_GLOBAL_SHADER` binds the class to source, entry, and stage; RDG allocates that same parameter type; `FComputeShaderUtils::AddPass` receives the typed shader reference, parameters, and group count. Sparkle should preserve that experience while using Sparkle naming and its existing graph/runtime owners. It should not copy Unreal-only `F` prefixes, constructor/type-layout macros, or systems that Sparkle's CRTP registration already derives.

The exact lean Sparkle target for a one-to-one compute shader is:

```cpp
class DirectLightingCS final : public GlobalShader<DirectLightingCS>
{
public:
    BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
        SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectDiffuse)
        SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ShadowVisibility)
        SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
    END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(
    DirectLightingCS,
    "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl",
    "main",
    Compute);
```

`DirectDiffuse`, `ShadowVisibility`, and `View` are each the one C++ member, graph/layout key, and reflected HLSL binding name. The type token is type information, not a second binding name; there is no author-facing alias or `_NAMED` escape hatch.

Graph construction uses that class directly:

```cpp
auto& parameters = builder.AllocParameters<DirectLightingCS>();
parameters.DirectDiffuse = builder.CreateUAV(directDiffuse);
parameters.ShadowVisibility = builder.CreateSRV(shadowVisibility);
parameters.View = viewUniforms;

builder.Dispatch<DirectLightingCS>(parameters, groupCount);
```

### Resource-View and Raster-Attachment Vocabulary

Unreal exposes two legitimate read-only RDG forms: a direct `SHADER_PARAMETER_RDG_TEXTURE` resource reference and an explicit `SHADER_PARAMETER_RDG_TEXTURE_SRV` created with `FRDGBuilder::CreateSRV`. It exposes writable shader views through `CreateUAV`. It does not expose `FRDGBuilder::Read`, and it does not create raster attachments through a symmetric `CreateRTV`; graphics parameters use `FRenderTargetBinding` / `RENDER_TARGET_BINDING_SLOTS`. NVRHI makes the same semantic split: binding layouts distinguish `Texture_SRV`, buffer SRVs, texture/buffer UAVs, and `AccelStruct`, while render targets and depth targets belong to framebuffer attachments rather than shader binding sets.

Sparkle chooses the explicit-view form because its shader parameter metadata already describes a concrete shader binding and must drive graph access, descriptor materialization, and reflection from one field. The author-facing vocabulary is frozen as follows:

| Use | Shader parameter declaration | Graph assignment |
| --- | --- | --- |
| read-only texture | `SHADER_PARAMETER_TEXTURE_SRV(Type, Name)` | `builder.CreateSRV(texture, optionalViewDesc)` |
| read-only buffer | `SHADER_PARAMETER_BUFFER_SRV(Type, Name)` | `builder.CreateSRV(buffer, optionalViewDesc)` |
| collaborator-owned read-only buffer | `SHADER_PARAMETER_EXTERNAL_BUFFER_SRV(Type, Name)` | focused draw collaborator supplies the already-materialized SRV as a binding override |
| read/write texture | `SHADER_PARAMETER_TEXTURE_UAV(Type, Name)` | `builder.CreateUAV(texture, optionalViewDesc)` |
| read/write buffer | `SHADER_PARAMETER_BUFFER_UAV(Type, Name)` | `builder.CreateUAV(buffer, optionalViewDesc)` |
| scene acceleration structure | `SHADER_PARAMETER_ACCELERATION_STRUCTURE(Name)` | `builder.CreateAccelerationStructureBinding(sceneTlas)` |
| raster color attachment | `SHADER_PARAMETER_RENDER_TARGET(Name)` in the narrow graphics envelope | `builder.CreateRenderTarget(texture)` |
| raster depth attachment | `SHADER_PARAMETER_DEPTH_TARGET(Name)` in the narrow graphics envelope | `builder.CreateDepthTarget(texture)` |

`CreateSRV` and `CreateUAV` create graph-tracked shader views; their typed return values carry the parent resource, subresource/format selection, and access declared by the parameter metadata. `SHADER_PARAMETER_EXTERNAL_BUFFER_SRV` is the narrow exception for a buffer whose lifetime and descriptor are intentionally owned by a real draw collaborator, such as the GBuffer mesh cache's skinning and morph buffers. It remains part of the shader's one parameter schema, is explicitly excluded from graph-resource declaration, and must be supplied by that collaborator at draw binding. It is not a bypass for graph-owned resources. `CreateRenderTarget` and `CreateDepthTarget` create graph attachment bindings, not HLSL parameters or shader-visible descriptors. Load/store, mip, slice, resolve, and clear policy belong to those attachment bindings or the focused graphics envelope.

There is deliberately no author-facing `Read(texture)` / `Read(buffer)` alias: it hides whether the shader receives an SRV, a copy source, an attachment load, or another access kind. There is deliberately no neutral `CreateRTV` / `CreateDSV`: those acronyms name backend-native D3D views and conflict with Sparkle's [neutral render-target vocabulary](../../Engineering/Standards/NamingAndVocabulary.md#canonical-concurrency-and-rendering-terms). An acceleration structure is also not generalized into `CreateSRV`; NVRHI models it as `AccelStruct` and Vulkan gives it a distinct descriptor kind, so Sparkle retains one semantic acceleration-structure binding and lowers it privately per backend.

Primary references:

- [Epic `FRDGBuilder::CreateSRV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateSRV)
- [Epic `FRDGBuilder::CreateUAV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateUAV)
- [Epic RDG shader and render-target parameter examples](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic `FRenderTargetBinding`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRenderTargetBinding)
- [NVIDIA NVRHI binding sets and framebuffers at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)

`AllocParameters<Shader>()` means `Shader::Parameters`; it does not allocate a second schema. `Dispatch<Shader>()` resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default diagnostic label from the shader type, declares resource usages from the same parameter metadata, materializes the generation-bound layout/pipeline through the backend owner, and records the dispatch later. The caller sees none of the package, map, code-library, binding-layout, or pipeline-cache mechanics.

An explicit label is only for a genuinely distinct graph instance:

```cpp
builder.Dispatch<ExposureDownsampleCS>(
    RenderPassLabel::Format("ExposureDownsample mip {}", mip),
    parameters,
    groupCount);
```

That label describes this graph node. It does not select shader bytecode.

Graphics follows the same rule without inventing a program alias or making the pass author restate a complete PSO. The draw path requests typed `ShaderRef<GBufferVS>` and `ShaderRef<GBufferPS>` from the map. The pass sets only the blend/depth/stencil intent it owns; graph attachments derive target formats, count, samples, load/store, and depth/stencil access; the mesh supplies vertex input and topology; material/pass policy supplies fill and cull; the runtime owner assembles and keys the complete neutral descriptor before recording. A focused mesh-pass collaborator may remain when it owns real draw-list/cache behavior; one-method pass wrappers that only forward parameters to a shader do not.

The target authoring shape is granular and semantic:

```cpp
auto& parameters = builder.AllocGraphParameters<GBufferGraphParameters>("GBuffer");
parameters->Shader.Pixel.View = viewUniforms;
parameters->BaseColor = builder.CreateRenderTarget(baseColor, RenderTargetLoadAction::Clear);
parameters->DeviceZ = builder.CreateDepthTarget(
	deviceZ,
	DepthStencilAccess::DepthWrite,
	RenderTargetLoadAction::Clear);

RasterPassRenderState renderState;
renderState.SetDepthStencil(DepthStencilState::WriteNearOrEqual);
renderState.SetBlend(BlendState::Opaque);

builder.Draw<GBufferVS, GBufferPS>(parameters, renderState, meshDraws);
```

This is deliberately not a second aggregate with the old fields under a new name. `RasterPassRenderState` contains only pass-selected semantic overrides and has granular setters. It cannot name vertex layouts, topology, render-target formats/count, depth format, sample count, shader stages, or backend objects. Attachment bindings are authoritative for their own compatibility and load/store behavior. The complete immutable pipeline description still exists where D3D12 and Vulkan require it, but only inside the runtime/RHI materialization boundary.

Ray-tracing stages use the same concrete `GlobalShader` registration and `ShaderRef` lookup as raster and compute, but they do not pretend to have identical binding roles. The selected ray-generation shader owns the dispatch-global nested `Parameters` used by the graph. Miss, closest-hit, any-hit, intersection, and callable classes declare only stage identity and optional compile hooks; they do not repeat the ray-generation root parameters or introduce empty placeholder structs. A focused `RayTracingPipelineComposition` names typed stage membership and hit groups and owns the composition-wide payload type, attribute type, minimum recursion, and optional bounded local-record schemas exactly once. It is not a universal `TShaderProgram`, package, or second registration framework. The target semantics remain in the [ray-tracing target architecture](RayTracingPipelineImplementationPlan.md), while the phases below deliver the entire path.

The intended authoring surface mirrors Unreal's useful production split without copying its prefixes or legacy binding adapters. This abridged example shows the declaration shape; the product implementation supplies the complete GBuffer output schema required by Phase 7.

```cpp
class RayTracedGBufferRGS final : public GlobalShader<RayTracedGBufferRGS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
		SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
		SHADER_PARAMETER_ACCELERATION_STRUCTURE(RayTracingScene)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, BaseColor)
	END_SHADER_PARAMETER_STRUCT()
};

class RayTracedGBufferMissMS final : public GlobalShader<RayTracedGBufferMissMS> {};

class RayTracedGBufferClosestHitCHS final : public GlobalShader<RayTracedGBufferClosestHitCHS> {};

IMPLEMENT_GLOBAL_SHADER(
	RayTracedGBufferRGS,
	"/Engine/Passes/GBuffer/RayTracedGBuffer.hlsl",
	"RayGeneration",
	RayGeneration);

IMPLEMENT_GLOBAL_SHADER(
	RayTracedGBufferMissMS,
	"/Engine/Passes/GBuffer/RayTracedGBuffer.hlsl",
	"Miss",
	Miss);

IMPLEMENT_GLOBAL_SHADER(
	RayTracedGBufferClosestHitCHS,
	"/Engine/Passes/GBuffer/RayTracedGBuffer.hlsl",
	"ClosestHit",
	ClosestHit);
```

The effect owner declares one `RayTracingPipelineComposition` relating those typed stages and its hit group. That declaration is also the single author-facing location for `GBufferRayPayload`, `BuiltInTriangleAttributes`, recursion, and any genuinely consumed local record; individual shader classes do not restate those facts. Graph construction then remains as lean as compute dispatch:

```cpp
auto& parameters = builder.AllocParameters<RayTracedGBufferRGS>();
parameters.View = viewUniforms;
parameters.RayTracingScene = builder.CreateAccelerationStructureBinding(rayTracingScene);
parameters.BaseColor = builder.CreateUAV(baseColor);

builder.TraceRays<RayTracedGBufferRGS>(rayTracedGBufferPipeline, parameters, renderExtent);
```

`TraceRays<RayGenerationShader>` is the Renderer/frame-graph frontend. It resolves the typed ray-generation shader and focused composition, derives the global layout from `RayGenerationShader::Parameters`, asks the existing runtime owner for the exact pipeline/table generation, declares all graph resources, and records later. The author does not pass a native pipeline, SBT, global binding writer, code hash, export string, table address, stride, or backend command. An explicit `RenderPassLabel` overload changes diagnostics only.

The final form has these invariants:

- the direct dispatch shader is the sole author-facing owner of its global shader-visible parameters; optional local-record fields have one separate group/stage owner and are never mirrored;
- the implementation declaration states only class, virtual source, entry, and stage/export facts;
- optional compile hooks appear only on shader classes that use them; composition-wide payload/attribute facts and bounded local records appear once on the focused composition or owning hit group;
- no precache hook or permutation-domain boilerplate exists in the base migration;
- no `DirectLightingProgram`, `SPARKLE_RENDER_PASS`, duplicated `DirectLightingPassParameters`, or forwarding `DirectLightingPass` is required for the one-to-one compute case;
- raw shader directory scanning does not replace registration because include/library files are not executable entries;
- binding-layout and pipeline debug names are generated and never lookup authorities;
- compiler RTTI strings and implementation-specific pretty-function text are not serialized; generated catalog records carry deterministic IDs and declaration locations.

### Intent-First Shader Frontend Contract

The normal shader workflow is one `Shader Tools` surface plus the existing console/shortcut route, not a panel for every compiler, package, pipeline, or artifact layer. The user supplies intent; the system resolves dependency closure, supported targets, job scheduling, validation, publication, reload, and lifetime safety.

| User intent | Primary frontend action/result | Automatically derived and validated | Contextual expert access |
| --- | --- | --- | --- |
| Apply saved source edits | `Apply Changed` with one shortcut and nonblocking status | changed virtual paths, reverse dependencies, affected shader types, active development targets, bounded jobs, complete replacement generation, safe activation | selected/compiled counts; exact job list under Details |
| Understand a compile failure | source-located root cause, affected shader type, `Open Source`, `Retry`, and reassurance that the previous generation remains active | duplicate diagnostics collapsed by root cause, dependent jobs classified as skipped, portable virtual paths, failure bundle creation | compiler output, command, preprocessed source, dependency hashes, reflection/layout comparison, one-job replay |
| Inspect what the running frame uses | select/search a semantic graph operation or shader and see active, changed, compiling, failed, or unsupported state | join from graph label/shader type to active shader-map entry, code record, source, symbols, pipeline, and captured marker identity | target/backend, hashes, reflection, disassembly, map record, and pipeline provenance |
| Validate a development or release build | `Validate Shaders` through the normal build/cook preset | complete registered catalog, compile-policy filtering, D3D12/Vulkan targets required by that preset, deterministic publication and conformance checks | support matrix, compiled-job counts, compiler time, and memory statistics |
| Investigate a shader or pipeline hitch | `Open Shader` from the selected Performance/GPU marker or pipeline event | preserve frame, marker, shader/code/pipeline hashes, backend, generation, and symbol identity | PIX/RenderDoc/Nsight/RGP guidance and copied replay/capture identity |
| Reproduce one expert failure | `Replay Failed Job` from Diagnostics details | exact compiler/tool version, virtual source closure, target, environment, and bounded output location | editable command copy and machine-readable request |

The common Editor layout is deliberately small:

```text
+ Shader Tools ---------------------------------------------------------------+
| 3 changed files ready       [Apply Changed]                    [Search ...] |
| Status: Ready | active generation 42 | last apply compiled 18 shaders |
+ Shaders ---------------------------------+ Selection -----------------------+
| DirectLighting        Ready              | DirectLighting / Compute          |
| GBuffer               Ready              | source + entry                    |
| Exposure              Changed            | used by 1 pass                    |
| ...                                        | [Open Source] [Apply Selection]  |
+ Diagnostics ----------------------------------------------------------------+
| No active errors. [Show last operation] [Advanced...]                       |
+------------------------------------------------------------------------------+
```

The primary list should normally expose shader name, stage, virtual source, active status, and graph consumers. Package ID, binding-layout ID, generation number, backend/profile, artifact directory, code/input hashes, compiler command, raw reflection, and disassembly are contextual details—not ten default columns. `Reload Cooked`, `Recook All`, package-targeted recook, compiler backend/target listing, and worker controls do not belong beside the common `Apply Changed` action. Validated publication activates automatically; `Rebuild All` and manual reload remain searchable expert recovery actions with an explanation of cost and risk.

```text
Idle -> ChangesReady -> Compiling -> Validating -> Active
                         |              |
                         +-> Failed <---+
                               |
                     previous Active generation remains
```

- One operation card owns progress, cancellation, queued/coalesced follow-up state, result, elapsed time, and the active-generation guarantee. Do not emit a dialog or toast per shader job.
- Source watching may offer coalesced `Auto Apply Changed` as a development preference after debounce/cost measurement. The default interaction must always reveal what will compile; a widely included source or release-target expansion shows the estimated affected scope before expensive work starts.
- The frontend chooses the named development/release preset and capabilities. An advanced override shows its difference from that preset, validates it before launch, persists only at the declared project/platform scope, and offers `Reset To Preset`.
- A successful apply needs a short result, not raw cooker stdout. A failure leads with one actionable source diagnostic. All raw artifacts remain available without dominating the workflow.
- Opening Shader Tools from a GPU marker, error, or pipeline event preserves that selection. The user never retypes a package, shader ID, hash, source path, or backend to continue the investigation.
- The panel reads one immutable shader-operation/catalog model and submits typed requests through `EditorOperationService`; it never scans artifact directories, invokes the compiler, mutates the active shader map, or creates RHI objects itself.

Epic's current shader-development workflow similarly centers a saved edit plus `recompileshaders changed`, asynchronous compilation, direct source diagnostics, and retry. Sparkle adopts that intent and safe iteration loop while generating more of the target/dependency/publication detail, compiling every selected job, and retaining the previous valid generation. [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)

## Cooked Artifact Model

Sparkle does need cooked shader data; it does not need authors to name each physical package manually.

The current package already carries information that a loose compiled shader file cannot represent safely:

- declared stages and package kind
- DXIL and SPIR-V records
- entry points and ray-tracing exports/hit groups
- reflection and merged resource bindings
- backend-specific pipeline layouts
- specialization inputs
- required feature flags
- compiler backend and target provenance
- source, bytecode, binding-layout, and package hashes

The base target uses three identities with different scopes. The names deliberately distinguish Unreal's logical job identity from its full compiler-input hash:

1. `ShaderTypeId`: a stable ID emitted from the concrete shader declaration. Together with target it addresses a base shader-map entry.
2. `ShaderCompileInputHash`: a content hash of every input that can change one stage compile, including virtual source path, transitive source contents, entry point, stage, environment, target, compiler backend, and compiler version. Parameter metadata enters this hash only when it changes generated declarations, resource-table/environment input, or compiler output; a validation-only signature belongs in the map record instead. Package/pass/display identity is excluded.
3. `ShaderCodeHash`: a hash of the exact validated backend bytecode. Stage, entry, reflection, feature, layout, and provenance metadata remain adjacent current-contract map/record fields and are covered by artifact integrity; they are not silently conflated with raw code identity. This preserves exact byte deduplication and symbol correlation without treating equal bytes with incompatible metadata as interchangeable records.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical compile jobs and identical output code to be deduplicated at their proper layers, and typed shader lookup to remain independent of a file basename.

The current one-`.sparkshader`-per-package layout supplies migration inputs only; it is not a target checkpoint or compatibility container. Phase 4 atomically replaces it with shader-map entries and code records, deletes the old readers/writers, and leaves physical grouping as generated policy. The initial library may use one or several generated files, but that choice must not expose authored package identity. Later regrouping requires current startup I/O, file-open, compression, and patching evidence and regenerates the one current representation in place.

```text
ShaderTypeId + Target --------------------------> GlobalShaderMap entry
ShaderCodeHash --------------------------------> CookedShaderLibrary record
                                                            |
typed shader refs + pass state + mesh/material facts + attachment signature
                                                            |
                                                            v
                                                 GraphicsPipelineKey
                                                            |
                                                            v
                                      complete descriptor -> RHI pipeline
```

## External Precedent and What Sparkle Adopts

The sources below are precedents, not local implementation authority. Repository links are pinned to the reviewed revisions where possible.

### Unreal Engine as the Core Model

Epic's global-shader documentation registers a C++ shader type against a source file, entry point, and shader stage. Its example deliberately registers vertex and pixel shader types from the same file with different entry points, then retrieves instances by type from the Global Shader Map. `FShaderType` metadata also carries the source filename, function, frequency, permutation count, parameter metadata, and hooks for compile eligibility, environment modification, output validation, and precaching. A source file is therefore an input to a shader type, never the universal runtime identity.

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and deterministic input identity. `FShaderCompileJob` pairs input, a logical job key, preprocess output, compiler output, and diagnostics; the logical `FShaderCompileJobKey` is not the full content hash. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, and worker processes. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

Unreal cooking collects unique shader code into shader libraries. At runtime `FShaderCodeLibrary` can open project/plugin libraries and test for code by hash. `FShaderMapResource` separately owns the render-resource lifetime. Sparkle adopts the logical shader map, cooked code library, and live RHI-object separation while deliberately omitting persistent storage of compiler results.

Unreal's Render Dependency Graph intentionally lets a shader parameter structure also describe the common one-to-one pass, while `FRDGBuilder::AddPass` still receives a separate event name for debugging and profiling. Epic also documents pass parameter structures without shader semantics, such as copy passes. Sparkle should therefore reuse or compose one authoritative shader-visible schema, not force every graph pass into one shader struct. Epic's parameter metadata exposes both a layout hash and a strong persistable layout signature; that is stronger than Sparkle's current independent Direct Lighting declarations plus count-only runtime fallback.

Unreal's shader frontend is the more important precedent for this migration: a concrete global-shader class declares its `FParameters`; the implementation macro binds source, entry, and stage; RDG allocates the same parameter type; and `FComputeShaderUtils::AddPass` consumes a typed shader reference, those parameters, and group count. Unreal also has optional shader-pipeline and PSO-precaching systems, but Sparkle does not adopt them in this plan because the current workload has not justified their authoring or runtime cost.

Adopt now:

- typed shader registration
- explicit virtual source path, entry point, and stage
- shader-class-local compile eligibility/environment/result-validation hooks only where currently needed
- complete immutable compile inputs, deterministic input hashes and logical job IDs, in-flight deduplication, priorities, and cancellation
- typed shader-map lookup and independently owned RHI shader lifetime
- compile-input hashes derived from the full compiler-affecting closure and compiler provenance
- cook-time collection and deduplication of global shader code
- a nested shader parameter contract reused directly by one-to-one graph dispatch, with a pass envelope only for real multi-shader or graph-only composition
- render-graph event labels kept separate from shader identity
- direct typed compute dispatch utilities and typed shader-map lookup

Do not copy yet:

- Unreal's full material, vertex-factory, and permutation universe
- authored shader-pipeline types, PSO precaching/prewarming, driver-cache integration, and preload/residency controls before measurements justify them
- engine-scale macro, plugin, chunking, and patching complexity where Sparkle has no corresponding workload

Sources:

- [Epic: Adding Global Shaders](https://dev.epicgames.com/documentation/en-us/unreal-engine/adding-global-shaders-to-unreal-engine)
- [Epic: `GetShaderSourceFilePath`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)
- [Epic: `FShaderType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderType)
- [Epic: `TShaderPermutationDomain`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/TShaderPermutationDomain)
- [Epic: `FShaderCompilerInput`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompilerInput)
- [Epic: `FShaderCompileJob`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)
- [Epic: `FShaderCompileJobKey`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJobKey)
- [Epic: `FShaderCommonCompileJob` and full `InputHash`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCommonCompileJob)
- [Epic: `FShaderCompilingManager`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FShaderCompilingManager)
- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)
- [Epic: `FShaderLibraryCooker`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderLibraryCooker)
- [Epic: `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic: `FShaderPipelineType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderPipelineType)
- [Epic: `FShaderParametersMetadata`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderParametersMetadata)
- [Epic: Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic: `FRDGBuilder::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/AddPass)
- [Epic: PSO Precaching](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine)
- [Epic: Debugging the Shader Compile Process](https://dev.epicgames.com/documentation/en-us/unreal-engine/debugging-the-shader-compile-process-in-unreal-engine)

#### Graphics Pipeline and Mesh-Draw State Study

The review followed Epic's documented entry point at `FDeferredShadingSceneRenderer::Render`, then followed the source-derived API responsibilities into mesh-pass processing, RDG attachment extraction, pipeline-state assembly, and RHI binding. No licensed Unreal source checkout is present in the reviewed workspace, so this plan does not claim a line-by-line local reading of private implementation bodies. It relies on Epic's official architecture documentation and generated API pages, whose source locations identify `DeferredShadingRenderer.cpp`, `MeshPassProcessor.h/.cpp`, `RenderGraphResources.h/.inl`, `PipelineStateCache.h/.cpp`, and RHI headers. NVIDIA NVRHI provides the independent primary-source cross-check.

| Read path | Production responsibility observed | Sparkle conclusion |
| --- | --- | --- |
| `FDeferredShadingSceneRenderer::Render` and documented base-pass flow | sequences scene-rendering phases and calls focused draw systems | frame/feature orchestration does not author a backend-complete PSO |
| `FMeshBatch` -> `FMeshPassProcessor` -> `BuildMeshDrawCommands` | filters mesh work, selects shaders and pass state, and builds stateless draw commands | keep a focused GBuffer mesh collaborator; do not make graph construction enumerate native pipeline fields |
| `FMeshPassProcessorRenderState` | carries only pass-wide blend/depth-stencil/access/stencil and uniform-buffer overrides, then applies them to PSO state | adopt the narrow-state idea as an even smaller `RasterPassRenderState`; attachment access remains graph-owned and the complete aggregate is deleted |
| `FGraphicsMinimalPipelineStateInitializer` | holds mesh-draw shader/fixed-function state without render-target state | key shader, raster, depth/stencil, blend, and primitive facts separately from attachments |
| RDG `RENDER_TARGET_BINDING_SLOTS`, `FGraphicsPipelineRenderTargetsInfo`, and `ExtractRenderTargetsInfo` | make attachments authoritative for formats, count, depth/stencil access, load/store, and samples | derive only formats/count/depth/sample compatibility from resource descriptions; keep load/store/clear/access in graph execution and validate them against pass depth/stencil intent |
| `FGraphicsPipelineStateInitializer`, `SetGraphicsPipelineState`, and pipeline-state cache | assemble/cache/bind the complete state required by the RHI | retain a complete internal descriptor and key; never require feature authors to fill it |
| `FMeshDrawCommand` submission | binds shader parameters, vertex streams, cached/minimal pipeline identity, topology, stencil reference, and draw arguments | prepared draw work contributes mesh facts; dynamic recording data is not stored in a frontend PSO bag |
| NVRHI `GraphicsPipelineDesc`, `FramebufferInfo`, and command `GraphicsState` | separate reusable pipeline state, attachment compatibility, and per-command binding state | Sparkle's split is not Unreal-specific and maps cleanly onto both backend APIs |

The important Unreal lesson is where completeness occurs. The RHI initializer is intentionally large because a native graphics pipeline is a compound object. The authored mesh-pass surface is intentionally smaller because no single feature caller owns all compound facts. Sparkle should reproduce that ownership split, not copy Unreal prefixes, its full material/vertex-factory/permutation system, cached mesh-draw-command framework, or PSO-precache machinery.

The current Sparkle Phase 2 checkpoint exposes the following incomplete ownership:

| Current surface | Problem | Owning disposition |
| --- | --- | --- |
| `GraphicsShaderPipelineState` at `RasterizedGBuffer` | repeats six color formats, count, depth format, vertex layout, and fixed state next to graph declarations | Phase 5 deletes it; the graph, mesh/material, and pass state become the separate authorities |
| `FrameGraphBuilder::Draw(..., pipelineState, ...)` | forces the graph caller to construct a backend-shaped aggregate | Phase 5 accepts only typed shaders, narrow render state, and real draw work |
| `RenderPassRuntimeCache` keyed only by shader pair | rejects a second legitimate state for the same vertex/pixel shaders | Phase 5 introduces a complete immutable key assembled from all contributing authorities |
| `RasterPassPipelineRuntime` base/wireframe/two-sided bundle | eagerly creates speculative variants for every graphics runtime | Phase 5 lazily materializes only exact requested variants before recording; it does not add precaching |
| generic `ShaderPassOperations` view-mode selection | leaks semantic wireframe policy into generic shader binding | Phase 5 moves fill/cull choice to the mesh/material/pass owner and leaves binding semantic-free |
| `GBufferMeshPass::PrepareTargets` plus graph attachments | manually binds and clears targets already declared by the graph | Phase 5 makes attachment actions authoritative and removes the duplicate target preparation path |
| `GBufferMeshPass::ConfigurePipeline` plus `GpuMesh::Bind` | sets triangle topology in two owners | Phase 5 leaves topology with prepared mesh draw work exactly once |
| public RHI fields plus backend defaults | omits authored blend/sample authority while D3D12/Vulkan hard-code opaque, sample-count-one, triangle, and raster defaults | Phase 5 completes the neutral descriptor and makes paired backends consume every supported field or reject it loudly |

The target state contribution is therefore:

```text
typed shader references ----------- shader code + binding-layout identity --+
RasterPassRenderState ------------- blend + depth/stencil -------------------+
prepared mesh/material draw ------- vertex input + topology + fill/cull -----+--> GraphicsPipelineKey
frame-graph attachments ------------ target/depth formats + samples ----------+        |
                                                                                      v
                                                                     complete GraphicsPipelineDesc
                                                                                      |
                                                                        D3D12 / Vulkan object

dynamic command state: viewport, scissor, blend constants, stencil reference,
vertex/index streams, parameter bindings, and draw arguments
```

Pipeline materialization remains lazy but must finish before parallel command recording begins. The graph/runtime owner collects the finite exact keys requested by prepared draw work, resolves them once per active shader-map generation, then recording performs lookup and binding only. This is not precaching, prewarming, or a readiness system: no unrequested wireframe/two-sided combination is created, no driver cache is introduced, and no speculative future variant becomes an authored obligation.

Primary graphics sources:

- [Epic: Graphics Programming Overview and `FDeferredShadingSceneRenderer::Render`](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- [Epic: Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- [Epic: `FMeshPassProcessor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessor)
- [Epic: `FMeshPassProcessorRenderState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessorRenderState)
- [Epic: `FMeshDrawCommand`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FMeshDrawCommand)
- [Epic: `FGraphicsMinimalPipelineStateInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateIni-)
- [Epic: `FGraphicsMinimalPipelineStateId`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateId)
- [Epic: `TStaticDepthStencilState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/TStaticDepthStencilState)
- [Epic: `FGraphicsPipelineRenderTargetsInfo`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FGraphicsPipelineRenderTargetsIn-)
- [Epic: `ExtractRenderTargetsInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/ExtractRenderTargetsInfo)
- [Epic: `FGraphicsPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FGraphicsPipelineStateInitialize-)
- [Epic: `SetGraphicsPipelineState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/SetGraphicsPipelineState/2)
- [NVIDIA NVRHI programming guide: pipelines, framebuffers, and graphics state](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI tutorial: pipeline and framebuffer separation](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

For ray tracing specifically, current Unreal 5.8 keeps the same shader-type center but separates three responsibilities that Sparkle must also keep separate:

1. `FShaderType`/`FShader` carry root-parameter metadata, compile hooks, and a ray-tracing payload hook.
2. `FRayTracingPipelineStateInitializer` relates ray-generation, miss, hit-group, and callable shader tables plus the shared binding layout. It is pipeline composition, not another shader declaration system.
3. `RayTraceDispatch` receives the materialized pipeline, selected ray-generation shader, shader binding table, global parameters, and dimensions. Unreal 5.5 began replacing the older ray-tracing binding writer with the normal batched shader-parameter path; Sparkle should start with its one typed `Parameters` path and copy no deprecated writer layer.

Sparkle retains those responsibilities but removes repetition that its smaller frontend does not need: `GlobalShader` stage classes, ray-generation-owned root `Parameters`, one focused typed `RayTracingPipelineComposition` owning the shared payload/attribute contract, and `TraceRays<RayGenerationShader>`. The compiler validates every participating export against that composition contract; the frame graph/runtime derives and retains the materialized pipeline, table, and binding state that Unreal's lower-level RHI call receives explicitly. This preserves Unreal's authoring experience while respecting Sparkle's graph and RHI ownership.

Primary sources:

- [Epic: `FShader` root parameters and ray-tracing payload hook](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FShader)
- [Epic: `FRHIRayTracingShader` stage hierarchy](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIRayTracingShader)
- [Epic: `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Epic: `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)
- [Epic 5.5 release note for batched ray-tracing shader parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-5-release-notes)

### NVIDIA Donut and NVRHI

Donut's `ShaderFactory` resolves bytecode using a filename plus entry point and selects an embedded DXBC, DXIL, or SPIR-V variant when available. It may use the filename as the shader object's default debug name. Its deferred-lighting implementation nevertheless creates the shader from `deferred_lighting_cs.hlsl` and separately emits the semantic GPU marker `DeferredLighting`.

NVRHI keeps `shaderType`, `debugName`, and `entryName` as distinct fields in `ShaderDesc`; a shader library resolves a shader by entry name and shader type. This is useful evidence that the low-level RHI description should not be responsible for Sparkle's source-package authoring policy.

NVRHI's RT API also confirms that pipeline description, shader table, command state, and ray dispatch are separate mechanism objects. Sparkle adopts that separation and paired D3D12/Vulkan contract, but not NVRHI's name-addressed mutable/versioned shader table. Sparkle's Renderer owns typed logical records, and backend-private RHI materializes an immutable table tied to one exact pipeline generation.

Adopt:

- source plus entry point as load/compile inputs
- automatic backend-bytecode selection
- separate semantic GPU markers and debug names
- a narrow RHI descriptor that consumes bytecode instead of inventing renderer package ownership

Sources:

- [NVIDIA Donut `ShaderFactory.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/engine/ShaderFactory.cpp)
- [NVIDIA Donut `DeferredLightingPass.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/DeferredLightingPass.cpp)
- [NVIDIA NVRHI `nvrhi.h` at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI ray-tracing tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

### AMD Cauldron and FidelityFX

Cauldron's reviewed D3D12 paths compile shaders with an explicit file, entry point, target profile, and define list. It hashes shader source recursively through includes and hashes defines. Its rendering code uses separate user-marker strings such as `GltfPbrPass::DrawBatchList`.

FidelityFX uses an effect-specific pass enum and permutation flags to select generated shader blobs, then gives the created pipeline a separate human-readable name. Its useful precedent for Sparkle is only the separation of executable identity from the human-readable pipeline label. Sparkle's unified migration does not adopt FidelityFX's effect/pass enum or generated-variant selector; the concrete shader class and typed draw/dispatch/trace already provide the required identity.

AMD's reviewed Vulkan capability path keeps ray-tracing-pipeline and ray-query feature structures distinct, and FidelityFX's denoiser sample keeps reusable ray construction/traversal/material helpers in shared HLSL. Sparkle adopts those two boundaries: independent capability truth and shared semantic kernels with thin execution frontends. Cauldron/FidelityFX do not provide a typed engine shader-class frontend comparable to Unreal's, so they are not evidence for adding another authoring macro, pass enum, program alias, or string-addressed pipeline layer.

Adopt:

- include-aware and define-aware compile identity
- typed shader selection at draw/dispatch
- separate pipeline/debug presentation names

Study generated permutation lookup only in the separately approved future permutation proposal.

Sources:

- [AMD Cauldron `ShaderCompiler.h` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/base/ShaderCompiler.h)
- [AMD Cauldron `ShadowResolvePass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/PostProc/ShadowResolvePass.cpp)
- [AMD Cauldron `GltfPbrPass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/GLTF/GltfPbrPass.cpp)
- [AMD FidelityFX optical-flow program selection at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/framegeneration/fsr3/internal/ffx_opticalflow.cpp)
- [AMD Cauldron ray-pipeline/ray-query capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [AMD FidelityFX shared ray-tracing HLSL at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)

### Acceleration-Structure Binding Decision

The Phase 0 baseline shadow duplication was not a legitimate shader-authoring variant. `DirectShadowSignalCS` and `DirectShadowSignalDeviceAddressCS` executed the same entry and shared algorithm; the latter existed only to define `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, omit the semantic `SceneTlas` parameter, and reconstruct an opaque acceleration structure from two address words stored in an effect uniform. The separate class, source root, pass wrapper, package, feature flags, parameter schema, address fields, and selection enum were one backend representation leaking through every frontend layer.

Production API precedent removes the reason for that leak:

- DirectX declares one HLSL `RaytracingAccelerationStructure` resource. D3D12 may bind it through an acceleration-structure SRV or a root SRV whose native value is a GPU virtual address; the HLSL shader identity does not change.
- NVRHI exposes one semantic `ResourceType::AccelStruct` binding and lets its D3D12/Vulkan implementations materialize API-specific layouts and bindings.
- Vulkan defines both `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` and `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV`. `VkWriteDescriptorSetPartitionedAccelerationStructureNV` carries the PTLAS device address inside the native descriptor write. Therefore a PTLAS address is backend descriptor data, not an HLSL constant or a reason for another shader class.
- Sparkle already resolves an acceleration-structure binding to a classic or partitioned native descriptor in Vulkan command recording. The missing contract is to select and validate that completed descriptor route consistently instead of advertising only the raw-address alternative.

The frozen target is one `SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)` field on `DirectShadowSignalCS` and every other shader that traces the scene. The graph binds one `FrameGraphAccelerationStructureHandle`. The selected TLAS provider is fixed before shader layout/pipeline materialization, and private RHI chooses the exact native descriptor type and write structure. Renderer shader/effect code never receives a TLAS GPU address or access-mode enum. If a provider cannot complete that semantic binding on the active device, that provider is unavailable and selection retains the supported classic provider; Sparkle does not compile a second shader or silently branch on a raw address.

The Phase 0 Vulkan mutable-descriptor feature/bootstrap/layout scaffold had no independent consumer outside this attempted classic/partitioned binding switch. Because provider selection is fixed before layout creation, Phase 1 deletes that scaffold and creates the exact descriptor layout for the selected provider. Sparkle does not retain a device feature, `pNext` chain, or generalized layout mechanism for hypothetical run-time switching.

This is not the postponed permutation system. The catalog and map retain one code record per `(ShaderTypeId, Target)`, the shader author writes no define or mode, and the shader bytecode does not fork for the binding representation. A future backend that truly cannot implement the semantic binding must extend the RHI capability and binding contract with evidence; it may not create `*DeviceAddressShader`, `*DescriptorShader`, or an internal pseudo-permutation by convention.

The no-query shadow program is deleted rather than generalized. Shadow visibility consumed by direct lighting is mandatory, so a shaderless clear is not a valid producer and the user cannot disable the only real frontend. Phase 1 requires inline-query capability before graph construction and schedules the one `DirectShadowSignalCS` producer unconditionally. Phase 8 replaces that inline-only precondition with selection between inline query and a complete pipeline/RGS implementation; absence of both still fails before scheduling. A compute shader that recompiles the same lighting code only to skip traversal is equally invalid.

Primary sources:

- [Microsoft DXR acceleration-structure resource binding](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [NVIDIA NVRHI acceleration-structure binding model at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Khronos `VK_NV_partitioned_acceleration_structure`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html)
- [Khronos partitioned-AS descriptor write](https://docs.vulkan.org/refpages/latest/refpages/source/VkWriteDescriptorSetPartitionedAccelerationStructureNV.html)

### Compiler, Capture, and Native Pipeline Precedent

Epic's development workflow treats a reproducible compile as a product feature: a debug dump contains the contributing sources and includes, preprocessed source, and a command file that replays the compiler invocation. Epic's cooked-shader debugging workflow can generate symbols separately from runtime shader data, so making a shader inspectable does not require permanently bloating the shipped bytecode.

The external GPU tools reinforce that provenance requirement. PIX resolves separate DXIL debug data by a compiler-suggested hash name and can show source and compile arguments from slim PDBs. NVIDIA Nsight Graphics needs source-level debug information and line mappings to correlate DXIL or SPIR-V hotspots and crashes back to HLSL. AMD Radeon GPU Analyzer inspects target ISA, register pressure, LDS, and scratch use. Sparkle therefore needs a bytecode-hash-to-source-symbol record and an opt-in analysis build, not only a text disassembly produced during a successful cook.

The native APIs also reinforce the separation between shaders and pipelines. Microsoft exposes cached D3D12 pipeline state and Vulkan exposes pipeline caches, while Epic layers a renderer-level precache policy above complete descriptors. Those are useful future precedents, not base-migration requirements. The current migration preserves correct lazy materialization before command recording and measures it; it does not add caches or precache orchestration without a demonstrated product hitch.

Adopt:

- write a self-contained replay bundle for failed as well as successful compile jobs
- store source mappings and separate debug symbols by shader/code hash so PIX, RenderDoc, and Nsight can resolve the exact cooked binary
- keep symbols and analysis products outside the lean runtime artifact unless an explicit development mode embeds them
- make representative DXIL and SPIR-V inspection first-class verification outputs
- feed exact shipped compile requests to RGA, Nsight, PIX, or equivalent analysis rather than recompiling an approximate shader by hand
- preserve complete D3D12/Vulkan pipeline descriptions and measure current lazy materialization so a later cache/precache proposal has evidence

Sources:

- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Shader Debugging Workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-debugging-workflows-unreal-engine)
- [PIX: automatic shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [RenderDoc: shader debug-symbol handling](https://github.com/baldurk/renderdoc/releases)
- [NVIDIA: using shader debug information with Nsight Graphics](https://developer.nvidia.com/blog/harness-powerful-shader-insights-using-shader-debug-info-with-nvidia-nsight-graphics/)
- [NVIDIA: Nsight Graphics shader compilation guidance](https://developer.nvidia.com/docs/drive/drive-os/7.0.3/public/nsight/nsight-graphics/UserGuide/index.html)
- [AMD: Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [Microsoft: managing D3D12 pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: D3D12 pipeline-state cache sample](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-pipeline-state-cache-sample-win32/)
- [Khronos: Vulkan pipeline cache](https://docs.vulkan.org/guide/latest/pipeline_cache.html)

## Phase 0 Baseline Findings

The 2026-08-23 Phase 0 source review at `5b0bd1469339897eef1fde3e5c9ab07137860d0f` traced the pre-migration production path below. This section is retained as frozen inventory and provenance; it is not a statement of the post-Phase 1 worktree.

```text
Authoring                     Host tooling                              Runtime and GPU
=========                     ============                              ===============

HLSL / HLSLI ----------+      ShaderCompiler CLI                        FrameGraphBuilder::Draw/Dispatch
                       |              |                                             |
C++ shader type -------+----> static GlobalShaderRegistry                           |
and FParameters        |              |                                             |
                       |              v                                             |
C++ pass Parameters ---+----> contract catalog ----> cook plan                      |
       |                              |                 |                            |
       |                              |                 v                            |
       +--> RDG resource use          |          bounded SparkleTasks jobs          |
                                      |                 |                            |
                         physical source/include closure                             |
                                                        |                            |
                                              +---------+---------+                  |
                                              |                   |                  |
                                             DXC                Slang                |
                                              |                   |                  |
                                              +--> bytecode + reflection             |
                                                        |                            |
                                              compile every selected stage           |
                                                        |                            |
                                              transactional .sparkshader             |
                                              + registry + recook signal             |
                                                        |                            |
                                                        +--------------------------->+
                                                                                     |
                                                                     package load + ABI validation
                                                                                     |
                                                                     binding-layout creation
                                                                                     |
                                                                     lazy pipeline creation
                                                                                     |
                                                                     pass Execute: bind + draw/dispatch
```

The separation at the right is important: graph construction may materialize a pass runtime, while graph execution does not load a package or create a pipeline. The following findings classify what that path actually provides today:

- 28 pass classes declare `static constexpr const char* PassName`.
- `RendererShaderPackages.h` declares 28 package strings.
- Renderer shader-registration sources make 29 calls to `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` and no calls to the package-inferred `IMPLEMENT_GLOBAL_SHADER` path.
- The three names normally repeat: pass name, package constant, and generated binding/pipeline debug names.
- Most current programs contain one compute stage. `GBuffer` demonstrates the legitimate multi-file, multi-stage case with a vertex shader and a pixel shader sharing one cooked program.
- `BuildShaderPackageIdFromSourcePath` derives only the filename stem, so same-basename sources in different directories collide and a source rename changes the fallback package ID.
- `GlobalShaderRegistry::Register` silently ignores a repeated shader name instead of reporting a conflicting declaration.
- The contract validator catches empty IDs, duplicate shader names, package layout/kind mismatch, and duplicate stages, but the registry's early duplicate suppression can prevent the validator from seeing the conflict.
- Direct Lighting and comparable passes declare resources twice: `DirectLightingPassParameters` drives frame-graph/pass binding, while `DirectLightingCS::FParameters` independently drives shader reflection verification. Both lists can compile while drifting semantically.
- `PassBinder` accepts either the same parameter-layout instance or merely the same parameter count. Equal counts do not prove equal binding order, kinds, names, visibility, or sizes.
- Source resolution is based on physical paths and searches project shaders before engine shaders. Includes search the includer's directory, a primary root, then additional roots, and absolute include paths are accepted. This is functional locally but lacks an explicit virtual namespace, permits machine-specific source identity, and can silently shadow an engine shader with a project file of the same relative path.
- The preprocessor already expands nested includes, detects recursion, handles `#pragma once`, and emits line directives. `IncludeClosureHasher` and compile-option hashing feed a deterministic `compileInputHash` used only for job identity, diagnostics, and provenance.
- `ShaderCookPlanExecutor` performs bounded parallel stage jobs through `TaskExecutor`, and every selected job invokes the compiler and validates parameter metadata. There is no persisted compile-result lookup, local artifact store, cache key/status, cache-directory option, or Launcher cache control.
- Cancellation is checked before a cook-node task begins, but an executing compiler session or child operation is not cooperatively cancelled. Jobs have no priority, no in-flight duplicate fan-out, no time-sliced result integration, and no measured worker-memory ceiling.
- Include-closure hashes serialize normalized physical paths, and compile-option hashes include physical include directories. Compile identity is therefore tied to a checkout/machine layout even when source bytes and virtual meaning are identical.
- Changed-source monitoring detects that some shader file changed, then invokes a full catalog `cook`; because selected jobs always compile, the missing reverse-dependency selection directly recompiles every registered shader.
- The cooker publishes selected `.sparkshader` files, a registry, and a recook signal as one transactional file set. The registry is not currently the runtime lookup authority; runtime computes a path directly from the manually repeated package ID.
- Bytecode is embedded per current package, so Sparkle has neither in-operation compile-job deduplication across identical shader inputs nor a cooked unique-code library addressed by code hash.
- DXC can emit disassembly, preprocessed source, compiler arguments, diagnostics, and separate debug data for a successful opt-in analysis cook. Slang's current analysis output is narrower and does not emit equivalent disassembly.
- The DXC backend applies requested debug information, optimization, warnings-as-errors, and debug stripping. The Slang backend currently does not apply equivalent cook-policy switches, and its stage mapping covers vertex, pixel/fragment, and compute rather than the full stage enum. Slang is therefore an architectural backend seam, not a proved release-equivalent compiler path.
- Debug-artifact publication begins only after compilation succeeds. A syntax error or backend failure throws before `ShaderDebugArtifactWriter` receives an artifact set, so the failure most in need of reproduction has no complete replay bundle, dependency manifest, or one-job command.
- `CookedShaderStats.csv` reports package/stage/backend/target/entry, bytecode bytes, reflection counts, and layout-record counts. It does not report queue/wall/CPU time, peak memory, source/code hash correlation, native pipeline creation, or runtime hitch percentiles.
- `inspect-shader` inspects registration/catalog metadata; `inspect-package` inspects the cooked container, reflection, layouts, and ray-tracing records. There is no single command that follows a typed shader from declaration through compile-input hash, code hash, active map/library record, runtime shader reference, pipeline, and captured GPU event.
- `RenderPassRuntimeCache` already builds and validates a complete replacement generation, atomically activates it, and retires the old generation only after recorded RHI submission tokens complete. This is a strong lifetime pattern and must be preserved.
- Runtime package validation is materially stronger than the authoring layer: it verifies schema/version, source and layout identities, bounds, complete logical binding records, required stages, runtime backend format, bytecode hashes, reflection, feature flags, and ray-tracing metadata before use.
- The editor already launches the shader cooker out of process, coalesces one follow-up request, rejects stale publications, and leaves the active generation unchanged after cook or runtime-validation failure.
- The current `Shader Tools` window presents Refresh, Reload Cooked, Recook All, and Recook Selected as equal toolbar actions above a ten-column table. Package ID, binding layout, parameter count, backend/target text, generation, and artifact availability are visible before the user has asked an expert question. This is an implementation-oriented inventory, not the selected intent-first frontend.
- The current selection area opens raw Source, Reflection, Disassembly, Param Match, and Compile Request artifacts and discovers their directory from shader/package identity. These are valuable expert details, but the panel lacks one operation/status model, source-located failure summary, shader consumer trace, and a guided next action. Raw artifacts must move behind contextual Diagnostics/Advanced disclosure rather than be removed.
- Current source tracking automatically schedules a changed recook, while the toolbar and console also expose manual global/package/shader actions. The target must converge these into one coalesced `Apply Changed` workflow with one visible state; full rebuild, manual reload, and identity-targeted recovery remain expert actions rather than parallel normal paths.
- A pass runtime is materialized lazily from `FrameGraphBuilder::Draw`, `Dispatch`, or `DispatchAsync`. Its package, binding layout, and pipeline are created during graph construction, not command recording. This preserves the frame-graph Execute boundary but can place first-use pipeline creation on a frame-critical construction path.
- Renderer frame orchestration now uses `BuildRenderFrameGraph`, explicit pass parameter records, and the infrastructure-only `PassCommandContext`. Shader work must preserve that boundary: graph setup may resolve a prepared immutable shader/pipeline runtime, while pass recording may only bind declared resources and record commands.
- `RenderFrameIdentity` captures the active shader generation from `RenderPassRuntimeCache`; `RenderViewState` invalidates history from the generation transition. The shader migration must keep one renderer-owned generation source and must not publish package/cache/compiler mechanics through GameFramework, `RenderFrameSubmission`, `RenderView`, or `PreparedRenderScene`.
- `FrameGraph::HasBeenProduced` is now the authority for whether a declared graph resource has prior contents. Shader and pipeline work must not reintroduce one-field history-validity records, mirrored production flags, or broad frame/pass contexts.
- `FrameGraphBuilder` and `FrameGraph` currently expose `Read(texture)` / `Read(buffer)` and `CreateSRV(texture)` / `CreateSRV(buffer)` as duplicate aliases returning the same typed shader fields. Production pass call sites already use `CreateSRV` for texture/buffer shader reads; the stale proposal example was the only `builder.Read(texture)` spelling found. Acceleration-structure call sites use a separate `Read(sceneTlas)` overload. Phase 1 replaces the AS overload with `CreateAccelerationStructureBinding`; Phase 2 deletes the unused texture/buffer `Read` aliases and freezes explicit SRV/UAV versus attachment vocabulary.
- Renderer C++ passes now live under semantic `Passes/GBuffer`, `Passes/Lighting/<Direct|Reference|Restir|Shadows|Sky>`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners. Shader sources still collect 18 files under `Engine/Assets/Shaders/Passes/Deferred`; the source-namespace phase must move those files to matching semantic owners and delete the broad directory rather than treating `Deferred` as renderer-wide architecture.
- D3D12 creates every graphics/compute pipeline with an empty `CachedPSO`; Vulkan calls `vkCreateGraphicsPipelines` and `vkCreateComputePipelines` with `VK_NULL_HANDLE` for the pipeline cache. Vulkan computes a local cache-key-shaped struct and then discards it. There is no renderer pipeline cache, native persistent cache, asynchronous precache coordinator, or hit/miss/too-late telemetry.
- Pass labels, D3D12 PIX events, Vulkan object names, binding-layout names, and pipeline names are readable. They do not carry stable shader-type/code identity that can join a capture event to the cooker artifacts and external shader symbols.
- Ray-tracing library registration, cooking, inspection, and metadata validation exist in the generic schema/tooling, but there are no renderer `IMPLEMENT_RAY_TRACING_SHADER` registrations. Runtime validation explicitly rejects a ray-tracing library because RT state-object/pipeline execution is not enabled. Current ray-query lighting passes are compute shaders and must not be presented as proof of a ray-tracing shader pipeline.
- `DirectShadowSignalNoRayQuery` and `DirectShadowSignalDeviceAddress` are registered and have pass implementations, but the production frame path always dispatches `DirectShadowSignal`. `CanUseInlineRayQueryShadows` has no selection consumer, and top-level provider selection falls back to classic descriptor access rather than choosing the device-address shader. These alternatives are not verified fallbacks. The device-address root duplicates the live shader solely because native AS binding representation leaked into shader identity; the no-query root duplicates lighting work only to skip traversal. Phase 1 deletes both instead of moving or retaining them.
- Geometry, hull, and domain stages appear in shader enums, package validation, and Vulkan shader-module mapping, but current graphics pipeline descriptions/backends wire only vertex plus optional pixel stages. Mesh/task stages are absent and both RHI capability reports mark them unsupported. Schema awareness is not executable stage support.
- Runtime capability checks directly reject missing acceleration-structure and inline-ray-query support, but do not explicitly check every declared package feature such as acceleration-structure device-address access and descriptor indexing in the same capability gate.
- CLI validation structurally validates the catalog but cooks and inspects only the representative `ComputeClear` artifact. `ShaderCompilerCliValidation` is a custom build target rather than a registered CTest, and no shader-specific unit/integration test suite was found.
- The existing build already discovers and links the shader-registration object library into compiler/runtime consumers, so authoring automation does not require scanning all shader source files.
- 56 generated `.sparkshader` files were present in the reviewed development artifacts: 28 files totaling 2,004,452 bytes under each of the `Shared` and `Showcase` project roots, for 4,008,904 bytes in aggregate. The two roots contain the same 28 artifact names. This is not evidence that 28 unique physical programs are a performance problem.

Relevant implementation entry points:

- [`GlobalShader.h`](../../../Engine/RHI/Public/Shaders/Authoring/GlobalShader.h)
- [`ShaderAuthoring.cpp`](../../../Engine/RHI/Private/Shaders/ShaderAuthoring.cpp)
- [`RendererShaderPackages.h`](../../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h)
- [`DirectLighting.h`](../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLighting.h)
- [`DirectLighting.cpp`](../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLighting.cpp)
- [`DirectLightingShaders.cpp`](../../../Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp)
- [`FrameGraphBuilder.h`](../../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h)
- [`ShaderContractCatalogBuilder.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp)
- [`ShaderContractValidator.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp)
- [`ShaderCookPlanExecutor.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookPlanExecutor.cpp)
- [`ShaderCookNodeExecutor.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookNodeExecutor.cpp)
- [`ShaderDebugArtifactWriter.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderDebugArtifactWriter.cpp)
- [`IncludeClosureHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/IncludeClosureHasher.cpp)
- [`ShaderCompileOptionsHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/ShaderCompileOptionsHasher.cpp)
- [`CookedShaderPackage.h`](../../../Engine/RHI/Public/Shaders/CookedShaderPackage.h)
- [`CookedPackageWriter.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp)
- [`CookedShaderPackageCache.cpp`](../../../Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp)
- [`CookedShaderPackageValidation.cpp`](../../../Engine/RHI/Private/Shaders/CookedShaderPackageValidation.cpp)
- [`PassBinder.cpp`](../../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [`RenderPassShaderRuntime.h`](../../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h)
- [`PipelineRuntimeLibrary.cpp`](../../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.cpp)
- [`RenderPassRuntimeCache.cpp`](../../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [`D3D12Pipeline.cpp`](../../../Engine/RHI/Private/D3D12/Pipeline/D3D12Pipeline.cpp)
- [`VulkanPipeline.cpp`](../../../Engine/RHI/Private/Vulkan/Pipeline/VulkanPipeline.cpp)
- [`ShaderSourceChangeTracker.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderSourceChangeTracker.cpp)
- [`ShaderRecookCoordinator.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp)
- [`ShaderCompilerProcess.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp)
- [`ShaderCompiler` build definition](../../../Tools/Shaders/ShaderCompiler/CMakeLists.txt)
- [`ValidateShaderCompilerCli.cmake`](../../../Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake)

## Phase 0 Frozen Inventory and Deletion Ledger

This section is the completed Phase 0 contract. Counts are exact for the unstaged `master` worktree at `5b0bd1469339897eef1fde3e5c9ab07137860d0f`; generated-artifact counts are a filesystem observation made on 2026-08-23 and are not attributed to that revision unless stated otherwise. Before this Phase 0 inventory edit, the staging area was empty and this document already contained unstaged, in-scope delivery-contract refinements; no unrelated dirty path existed, so the Phase 0 exclusion list is **empty**. The source/tool diff from the previous audit revision `44c2f192a82947d9dcdd0e4bbd7ba0cb1a7145e4` to the current revision is empty, but all counts and owner traces below were nevertheless re-run against the current worktree. Later phases must re-run `git status --short --branch` and establish their own exclusions rather than assuming that remains true.

The inventory used `rg`, `rg --files`, file counts, and bounded reads over Renderer shader registrations and passes, RHI shader contracts/runtime, ShaderCompiler source/cook/publication/inspection, Application recook routing, Editor Shader Tools, CMake membership, ignored development artifacts, diagnostics, and current documentation. No configure, build, compiler invocation, cook, launch, test, capture, or runtime check contributed to this inventory.

### Exact count baseline

| Surface | Exact current count | Frozen interpretation and deletion owner |
| --- | ---: | --- |
| renderer shader-registration `.cpp` files / `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` calls | 29 / 29 | One registration per current stage. Phase 1 deletes the two shadow representation duplicates; Phase 2 replaces the macro on the remaining 27 declarations. |
| concrete shader classes / nested `FParameters` schemas | 29 / 29 | Phase 1 deletes the two catalog-only shadow variants; the remaining 27 are retained as authoring authorities and renamed to target `GlobalShader`/`Parameters` vocabulary in Phase 2. |
| logical program/package IDs | 28 | `GBufferVS` and `GBufferPS` share `GBuffer`. Phase 1 deletes the two rejected shadow identities; Phase 4 deletes the remaining 26 handwritten package identities. |
| `RendererShaderPackages` constants | 28 | Phase 1 deletes the two rejected shadow constants; Phase 4 deletes the remaining constants and then the header. |
| authored pass classes / `PassName` constants | 28 / 28 | Phase 1 deletes the two rejected shadow wrappers and labels; Phase 2 deletes the remaining 25 compute wrappers and narrows `GBufferPass` to real graphics behavior. |
| pass wrapper files | 56 | Twenty-eight matching `*Pass.h`/`*Pass.cpp` pairs. Phase 1 deletes the two rejected shadow pairs; Phase 2 deletes the remaining 25 forwarding pairs and rewrites the retained GBuffer pair in place. |
| pass API declarations and definitions | 28 `GetDefinition`, 28 `GetParameterMetadata`, 28 `Execute` pairs | Phase 1 deletes the two rejected-shadow forwarding surfaces. Phase 2 deletes the remaining generic accessors/Execute forwarding surfaces; the retained graphics collaborator receives a behavior-specific draw surface. |
| repeated binding-layout / pipeline string literals | 28 / 28 | Phase 1 deletes the four strings owned by the two rejected shadow wrappers. Phase 2 derives remaining diagnostics from shader type or accepts an instance label and deletes the other 52 strings. |
| nested shader parameter field declarations | 392 | Phase 1 deletes 32 fields belonging to the two rejected shadow variants. Phase 2 retains/renames the remaining 337 compute fields and 23 GBuffer-stage fields as the one declaration authority. |
| directly authored shader-visible fields in pass-side records | 352 | Duplicate declarations: 324 in compute pass records, 10 in `GBufferPassParameters`, and 18 in `GBufferDrawParameters`. Phase 2 deletes them and uses nested shader metadata. |
| effective concrete compute pass-mirror fields after common-base expansion | 369 | Exactly mirrors the 369 nested compute fields. Phase 1 deletes the 32 rejected-shadow mirrors; Phase 2 deletes the remaining 337 mirrors, including inherited common fields. |
| effective GBuffer shader-visible pass/draw field uses | 28 | Mirrors 23 stage fields across `GBufferPassParameters` and `GBufferDrawParameters`; Phase 2 removes the copies while preserving the real draw collaborator. |
| graph-only GBuffer attachment fields | 7 | `BaseColor`, `Normal`, `Material`, `Emissive`, `Subsurface`, `MotionVector`, and `DeviceZ` remain only in a narrow graphics graph envelope in Phase 2. |
| semantic graph resource-view calls | 178 `CreateSRV`, 49 `CreateUAV`, 7 AS `Read`, 6 `CreateRenderTarget`, 1 `CreateDepthTarget` | The seven `Read` calls are all acceleration-structure bindings and become `CreateAccelerationStructureBinding` in Phase 1. Phase 2 removes the duplicate texture/buffer `Read` aliases while retaining canonical SRV/UAV and raster-attachment routes. |
| files under `Engine/Assets/Shaders/Passes/Deferred` | 18 | Thirteen `.hlsl` and five `.hlsli` files. All move to semantic owners and the old directory is deleted in Phase 1. |
| generated `.sparkshader` files | 56 files / 4,008,904 bytes | Two identical-name sets: 28 files / 2,004,452 bytes under `Shared` and the same under `Showcase`. All old files are disposable Phase 4 outputs. |
| generated shader package registries | 2 | One `ShaderPackageRegistry.sreg` under each `Shared` and `Showcase`; both are disposable Phase 4 outputs. |

The parameter counts deliberately distinguish declarations from effective use. `352` is the number of authored duplicate shader-visible field declarations. `397` is the larger effective mirror-use count (`369` concrete compute fields plus `28` GBuffer pass/draw uses) after inheritance and reuse. Phase 1 removes the 32 rejected-shadow fields from both authorities; Phase 2 removes the remaining mirrors. Neither count includes the seven graph-only GBuffer attachments.

The 2026-08-23 current-worktree PSO audit was run after the staged/unstaged Phase 2 checkpoint and is therefore separate from the revision-pinned pre-migration counts above. Exact runtime/build occurrence counts are: `GraphicsShaderPipelineState` 9, `RasterPassPipelineRuntime` 24, `GraphicsPipelineDesc` 15, `RhiVertexLayoutKind` 10, `RhiDepthTestDesc` 7, `RhiStencilTestDesc` 7, `WireframePipeline` 9, `TwoSidedPipeline` 8, `RenderTargetFormats` 8, `RenderTargetCount` 12, `DepthStencilFormat` 16, and `SetPrimitiveTopology` 11. There is one typed graphics graph call, `Draw<GBufferVS, GBufferPS>`, in `RasterizedGBuffer.cpp`. Phase 5 owns the complete field/consumer re-inventory at implementation time and the clean-break dispositions below; these occurrence counts are evidence of the reviewed checkpoint, not target quotas.

| Current graphics-state owner/consumer | Current authority problem | Phase 5 disposition |
| --- | --- | --- |
| `RasterizedGBuffer.cpp` graph setup | graph attachments and the caller aggregate both state target compatibility; the aggregate also chooses mesh and pass facts | keep attachments and granular pass intent; delete the complete caller aggregate |
| `FrameGraphBuilder::Draw` | accepts and forwards the aggregate | accept typed shaders, narrow render state, and prepared draws only |
| `RenderPassRuntimeCache` | one shader-pair key stores one state and rejects another legitimate state | assemble/hash the complete key from all authorities and retain it in the existing generation owner |
| `RenderPassShaderRuntime` / `RasterPassPipelineRuntime` | eagerly creates base, two-sided, and wireframe variants | materialize exact requested keys lazily before recording and delete the bundle |
| `ShaderPassOperations` | selects wireframe from view semantics while binding | remove semantic policy from generic binding; the mesh/material/pass owner requests fill/cull |
| `GBufferMeshPass` and `GpuMesh` | targets/clears and triangle topology have duplicate routes | graph attachment execution owns target actions; prepared mesh draw owns topology once |
| public RHI and D3D12/Vulkan graphics creation | neutral state is incomplete while backends invent opaque blend, sample one, triangle topology, and raster defaults | complete the neutral descriptor and require both backends to consume or reject every supported field |

### Complete shader-class and nested-parameter inventory

Every live semantic row retains the concrete shader and its nested schema as the Phase 2 authority. The current `FParameters` spelling becomes `Parameters`; the corresponding pass schema, pass metadata accessor, package selector, and forwarding Execute body do not survive. The two catalog-only shadow variants are explicit Phase 1 deletions and never reach the target catalog.

| Shader class | Stage | Nested fields | Logical program | Current graph status |
| --- | --- | ---: | --- | --- |
| `ComputeClearCS` | compute | 1 | `ComputeClear` | live, reused with instance labels |
| `DirectLightingCS` | compute | 19 | `DirectLighting` | live |
| `DirectLightReservoirSpatialCS` | compute | 19 | `DirectLightReservoirSpatial` | live |
| `DirectLightReservoirTemporalCS` | compute | 20 | `DirectLightReservoirTemporal` | live |
| `DirectShadowSignalDeviceAddressCS` | compute | 20 | `DirectShadowSignalDeviceAddress` | Phase 1 deletion; backend binding detail, no graph consumer |
| `DirectShadowSignalNoRayQueryCS` | compute | 12 | `DirectShadowSignalNoRayQuery` | Phase 1 deletion; graph fallback policy, no graph consumer |
| `DirectShadowSignalCS` | compute | 21 | `DirectShadowSignal` | live |
| `ExposureDownsampleSceneCS` | compute | 2 | `ExposureDownsampleScene` | live |
| `ExposureDownsampleTextureCS` | compute | 2 | `ExposureDownsampleTexture` | live |
| `ExposureReduceSceneCS` | compute | 2 | `ExposureReduceScene` | live |
| `ExposureReduceTextureCS` | compute | 2 | `ExposureReduceTexture` | live |
| `ExposureCS` | compute | 5 | `Exposure` | live asynchronous dispatch |
| `GBufferVS` | vertex | 11 | `GBuffer` | live raster branch |
| `GBufferPS` | pixel | 12 | `GBuffer` | live raster branch |
| `LightingCompositeCS` | compute | 8 | `LightingComposite` | live |
| `LinearUpscaleCS` | compute | 3 | `LinearUpscale` | live |
| `OutputEncodingCS` | compute | 3 | `OutputEncoding` | live presentation branch |
| `PathTracedDirectLightingCS` | compute with inline ray query | 26 | `PathTracedDirectLighting` | live reference-lighting branch |
| `PathTracedIndirectLightingCS` | compute with inline ray query | 32 | `PathTracedIndirectLighting` | live reference-lighting branch |
| `RaytracedGBufferCS` | compute with inline ray query | 25 | `RaytracedGBuffer` | live ray-query GBuffer branch |
| `ReferenceLightingAccumulationCS` | compute | 7 | `ReferenceLightingAccumulation` | live reference-lighting branch |
| `RestirIndirectResolveCS` | compute with inline ray query | 37 | `RestirIndirectResolve` | live ReSTIR branch |
| `RestirIndirectSpatialCS` | compute with inline ray query | 35 | `RestirIndirectSpatial` | live ReSTIR branch |
| `RestirIndirectTemporalCS` | compute with inline ray query | 36 | `RestirIndirectTemporal` | live ReSTIR branch |
| `SceneDepthCS` | compute | 3 | `SceneDepth` | live |
| `SkyMotionVectorCS` | compute | 5 | `SkyMotionVector` | live |
| `SkyCS` | compute | 8 | `Sky` | live |
| `ToneMappingCS` | compute | 4 | `ToneMapping` | live presentation branch |
| `VisualizeBuffersCS` | compute | 12 | `VisualizeBuffers` | live debug branch |

The only current one-field pass schema is `ComputeClearPassParameters::Output`. It is not a justified strong handle, ABI wrapper, policy type, or extensible result: it duplicates `ComputeClearCS::FParameters::Output` and is deleted in Phase 2. Type-only shader parameter wrappers such as `StructuredBuffer<T>` are template vocabulary rather than one-property runtime records. No new one-field carrier is authorized by this plan.

### Pass, consumer, and collaborator dispositions

| Classification | Complete current set | Frozen disposition |
| --- | --- | --- |
| direct one-shader compute wrappers | `ComputeClearPass`; `DirectLightingPass`; `DirectLightReservoirSpatialPass`; `DirectLightReservoirTemporalPass`; `DirectShadowSignalPass`; `DirectShadowSignalDeviceAddressPass`; `DirectShadowSignalNoRayQueryPass`; `ExposureDownsampleScenePass`; `ExposureDownsampleTexturePass`; `ExposureReduceScenePass`; `ExposureReduceTexturePass`; `ExposurePass`; `LightingCompositePass`; `LinearUpscalePass`; `OutputEncodingPass`; `PathTracedDirectLightingPass`; `PathTracedIndirectLightingPass`; `RaytracedGBufferPass`; `ReferenceLightingAccumulationPass`; `RestirIndirectResolvePass`; `RestirIndirectSpatialPass`; `RestirIndirectTemporalPass`; `SceneDepthPass`; `SkyMotionVectorPass`; `SkyPass`; `ToneMappingPass`; `VisualizeBuffersPass` | Phase 1 deletes the two catalog-only shadow variant wrappers with their shader roots. Phase 2 deletes the remaining 25 forwarding classes/files. Twenty-six current Execute bodies only call sized compute forwarding; `ExposurePass` forwards a fixed `1x1x1` dispatch. `AddComputeClearPass` may remain only as a narrow repeated graph-intent helper after it dispatches `ComputeClearCS` directly. |
| multi-stage graphics | `GBufferPass` using `GBufferVS` and `GBufferPS` | Retain and narrow in Phase 2. It owns target clears/binds, viewport/scissor/topology, mesh-batch traversal, material table binding, skeletal validity, two-sided/wireframe pipeline choice, and draw calls. Remove its package/runtime-definition/metadata boilerplate and shader-visible copies. |
| real focused collaborator | `GBufferMeshBatchDrawer` | Retain with explicit GBuffer draw inputs. It owns mesh/cache iteration and per-draw behavior; it must not become a shader/package/runtime service locator. |
| shaderless graph work | texture copy, buffer copy, `LightingTargetClear`, `RaytracedGBufferTargetClear`, `RayTracingSceneBuild`, external upscaler evaluation, external ray-reconstruction evaluation | Retain as graph resource operations or focused provider/RT collaborators. They declare resources but do not need a fake shader class, shader parameters, or forwarding pass wrapper. |
| RT scene capability | `RenderRayTracingScene::Build` and its prepared scene/view plan | Retain under the RT scene owner. It owns acceleration-structure preparation/lifetime, not shader-package or frame-graph infrastructure. |
| external provider capability | upscaler and ray-reconstruction graph-generation objects | Retain at the graph generation that owns provider evaluation/native interop; pass recording receives only declared resources. |

The live typed compute consumers are the semantic graph builders in `Passes/Debug/VisualizeBuffers.cpp`, `Passes/GBuffer/{RaytracedGBuffer,SceneDepth,SkyMotionVectors}.cpp`, `Passes/Lighting/Direct/{DirectLighting,DirectLightReservoir}.cpp`, `Passes/Lighting/LightingComposite.cpp`, `Passes/Lighting/Reference/{PathTracedDirectLighting,PathTracedIndirectLighting,ReferenceLightingAccumulation}.cpp`, `Passes/Lighting/Restir/{RestirIndirectResolve,RestirIndirectSpatial,RestirIndirectTemporal}.cpp`, `Passes/Lighting/Shadows/DirectShadowSignal.cpp`, `Passes/Lighting/Sky/Sky.cpp`, `Passes/PostProcessing/{Exposure,ExposureMomentChain}.cpp`, `Passes/Presentation/{LinearUpscaling,Presentation}.cpp`, and the compute-clear helper. `RasterizedGBuffer.cpp` is the GBuffer draw consumer. Phase 2 updates every one directly; it does not add old/new dispatch overloads.

Parameter-metadata consumers have one destination each:

- graph setup and resource declaration consume `Shader::Parameters` directly in Phase 2;
- `PassBinder` and typed parameter instances consume the same structural metadata in Phase 2, and count-only compatibility acceptance is deleted;
- `GlobalShaderRegistry`, `ShaderContractCatalogBuilder`, `ShaderContractValidator`, and `ShaderParameterStructCookVerifier` consume the registered shader schema until Phase 3 renames their compile-job/catalog surfaces and Phase 4 publishes the map/library result;
- runtime layout/pipeline materialization remains in `RenderPassRuntimeCache`; Phase 2 removes pass-class lookup from its frontend, while Phase 4 changes its backing package storage atomically;
- the GBuffer material table is consumed only by `GBufferPass`/`GBufferMeshBatchDrawer`; it is a real draw dependency, not justification for a generic pass context or duplicated shader schema.

### Field authority ledger

| Current field family | Current producer and consumers | Target owner | Exact phase |
| --- | --- | --- | --- |
| every live semantic shader `FParameters` field | concrete shader class; registry/catalog/verifier; pass-side mirror | same concrete shader class as nested `Parameters`; graph and binding use the same metadata | Phase 2 retain/rename |
| 103 authored `_NAMED` parameter declarations plus three `_NAMED` macro definitions; field/layout/shader name triples in parameter metadata | shader headers; Renderer metadata builder; RHI layout/package/reflection; ShaderCompiler verification/cooking | one member/binding `Name` from shader class through HLSL reflection and runtime binding | Phase 2 delete aliases and rename HLSL bindings atomically |
| 32 fields on the two catalog-only shadow variant schemas and their pass mirrors | duplicate registrations/wrappers with no graph producer | deletion; live `DirectShadowSignalCS::Parameters` owns the semantic AS binding and no-query behavior is rejected before graph construction | Phase 1 delete |
| remaining effective compute `*PassParameters` fields, including `DirectLightReservoirCommonParameters`, `DirectShadowSignalCommonPassParameters`, and `DirectShadowSignalRayQueryPassParameters` expansion | semantic graph builder copies values into pass record; Execute/binder reads it | corresponding concrete compute shader `Parameters` field with the same semantic name | Phase 2 delete mirrors |
| ten shader-visible `GBufferPassParameters` fields and all eighteen `GBufferDrawParameters` field uses | raster graph builder/pass/drawer | `GBufferVS::Parameters` and `GBufferPS::Parameters`, composed at the real draw owner without another schema | Phase 2 delete mirrors |
| seven GBuffer attachment fields | raster graph builder and GBuffer pass | narrow graph-only GBuffer envelope | Phase 2 retain/narrow |
| `RenderPassDefinition::PassName` and each pass `PassName` | pass class; graph diagnostics | shader-type-derived default or graph-instance `RenderPassLabel` override | Phase 2 delete/derive |
| `RenderPassDefinition::PipelineKind`, `AllowInputAssemblerInputLayout`, and `Graphics` | pass definition/runtime factory | typed compute dispatch or complete graphics draw description at graph setup | Phase 2 delete bag |
| `BindingLayoutDebugName` and `PipelineDebugName` in definition/runtime request types | pass class/runtime pipeline creation | derived bounded diagnostics at pipeline owner | Phase 2 delete authored fields |
| `RenderPassDefinition::ShaderPackage` | pass class forwards `ShaderPackageDefinition` | no pass field; typed shader lookup through active map | Phase 2 deletes the containing bag; Phase 4 deletes `ShaderPackageDefinition` itself |
| `RenderPassRuntimeCache::ShaderRuntimeGeneration::{Generation,ShaderPackages,RuntimeStorageByPassType}` | renderer runtime cache; frame identity and typed materialization | one retained generation containing active `GlobalShaderMap`, `CookedShaderLibrary`, and derived runtime objects | Phase 4 atomic refactor |
| `RenderFrameIdentity::ShaderPackageGeneration` and `GetShaderPackageGeneration` | renderer runtime cache/frame pipeline; view-history invalidation | renderer-owned shader generation with no package/cache wording | Phase 4 rename; behavior retained |
| package IDs, keys, expected stages/features, package paths, schema versions, package load reports/cache generations | registrations/cooker/RHI runtime/editor diagnostics | typed catalog/map entry, code hash/library record, and one renderer shader generation | Phase 1 deletes the two rejected shadow package identities; Phase 4 deletes/replaces the remaining package surface |
| package-targeted recook request/model fields, package registry/publication fields, package arguments, package columns, and package artifact-path discovery | Editor/Application/CLI | typed shader/map/library vocabulary without a package compatibility spelling | Phase 4 delete/replace |
| parallel recook/reload actions, manual normal-path reload, implementation-first table layout, artifact-directory scans, and duplicated status formatting | Editor/Application | semantic shader/source `Apply Changed`, expert typed shader target, immutable operation/provenance views | Phase 10 delete/replace after Phase 4 has removed package identity |

### Resource declaration and attachment ledger

`FrameGraphBuilder` and `FrameGraph` currently each expose texture/buffer `Read` aliases beside texture/buffer `CreateSRV`, texture/buffer `CreateUAV`, acceleration-structure `Read`, `CreateRenderTarget`, and `CreateDepthTarget`. Production semantic graph setup contains 178 `CreateSRV`, 49 `CreateUAV` including the compute-clear graph helper, six `CreateRenderTarget`, one `CreateDepthTarget`, zero texture/buffer calls to `builder.Read`, and seven `builder.Read` calls that all bind `SceneTlas`. These author-facing counts exclude current `*Pass.cpp` recording/binding bodies; those are inventoried separately as forwarding surfaces. This confirms two separate clean breaks rather than a blanket rename:

- Phase 1 adds the one semantic `CreateAccelerationStructureBinding` route and updates all seven AS consumers. Backend descriptor type, address, and mutable-descriptor mechanics stay private to RHI lowering.
- Phase 2 deletes the duplicate texture/buffer `Read` declarations from both graph surfaces and retains only `CreateSRV`/`CreateUAV` for shader views plus `CreateRenderTarget`/`CreateDepthTarget` for raster attachments.
- `PassResourceBuilder::Read` and internal `resources.Read(...CopySource...)` remain declared-resource resolution/copy infrastructure. They are not shader-view authoring aliases and must not be renamed merely to satisfy a spelling search.

### Package, cook, runtime, publication, and frontend deletion ledger

Every old package definition, spelling, and consumer is owned below. A path named in one row is not deferred to a generic cleanup phase.

The frozen broad package floor, excluding documentation, third-party code, and generated artifacts, contains 942 matching lines across 187 source or build files for the exact case-insensitive expression `ShaderPackage|shader package|shader-package|\.sparkshader|ShaderPackageRegistry|ShaderRecookRequestType::PackageId|--package|inspect-package|PackageId|PackageKey|PackagePath`. The number is evidence for this revision, not a future API quota; the owner rows below are the stable deletion contract.

| Owner surface | Exact current definitions/paths | Disposition |
| --- | --- | --- |
| Renderer authored identity | `RendererShaderPackages.h`, 28 constants, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `ShaderPackageDefinition` references in registration/pass/runtime code | Phase 4 deletes package identity and changes remaining typed registrations/runtime lookup to map identity. Phase 2 may remove a containing pass bag but does not create a package replacement. |
| Renderer pass facade | `RenderPassDefinition`, `RenderPassGraphicsPipelineDefinition`, `RenderPassDefinitionPipelineKind`, `RenderPassDefinitionRuntime`, `RenderPassShaderRuntimeDesc`, generic `GetDefinition`/`GetParameterMetadata`, `ComputePassOperations`, and package/debug forwarding fields | Phase 2 deletes these pass-facing bags and forwarding surfaces. This is the only deletion phase for these definitions. |
| Renderer package-backed runtime | `PipelineRuntimePackageRequest`; package load/capability methods in `PipelineRuntimeLibrary`; `RenderPassShaderRuntimeStorage::ShaderPackage`; `CookedShaderPackageCache` and package-keyed pass-type holders in `RenderPassRuntimeCache`; reload/get-generation names | Phase 4 replaces storage and lookup atomically with map/library-backed shader references while preserving one cache owner, lazy graph-time materialization, replacement validation, and submission retirement. |
| RHI public package schema | `CookedShaderPackage.h`, `CookedShaderPackageContract.h`, `CookedShaderPackageIdentity.h`, `CookedShaderPackageCache.h`, `LoadedShaderPackage.h`, and `ShaderPackageLayoutBuilder.h` | Phase 4 deletes package containers/identity/cache. Only genuinely neutral code-record, reflection, parameter-layout, and RT metadata validation primitives survive under map/library-neutral names. |
| RHI private package implementation | `CookedShaderPackage.cpp`, `CookedShaderPackageReader.cpp`, `CookedShaderPackageValidation.cpp`, `CookedShaderPackageIdentity.cpp`, `CookedShaderPackageCache.cpp`, `LoadedShaderPackage.cpp`, `ShaderPackageLayoutBuilder.cpp`, plus package-shaped binding/RT validation entry points | Phase 4 deletes readers/cache/path/schema dispatch and renames or narrows only neutral validation used by map/library records. No old reader remains. |
| Core paths and extensions | `GetCookedShaderPackageRootPath`, `GetCookedShaderRegistryPath`, package path helpers, `ShaderPackageRegistry.sreg`, package directories, and `.sparkshader` extension/path construction | Phase 4 replaces publication paths with the generated map/library paths and deletes every old helper/extension consumer. |
| ShaderCompiler physical package output | `CookedPackageWriter`, `CookedRegistryWriter`, `CookedShaderPackageEmitter`, `CookedShaderPackageOutput`, `ShaderPackageCooker`, package output/registry/signal payload fields, package inspection, and package-shaped stats/debug fields | Phase 4 replaces them with deterministic map/library writers and provenance joins. Phase 3 separately renames compile work (`CookNode`, cook-plan/job records) without deleting physical package authority early. |
| CLI/build validation | `--package`, `inspect-package`, package help/diagnostics, `ValidateShaderCompilerCli.cmake` `.sparkshader` expectation, and recursive source-group membership of deleted/moved files | Package selection and `.sparkshader` validation are Phase 4 deletion; semantic shader targeting remains. Each code phase reconciles recursive glob/source-group results and explicit validation paths in the same CL. |
| Application package identity/publication | `ShaderRecookRequestType::PackageId`, package argument construction, package registry/signal payload fields and readers, `ReloadCookedShaders` package wording, and package-generation status text | Phase 4 replaces every package spelling and publication field with typed shader/map/library vocabulary. This occurs with the format switch, not in the later UX phase. |
| Editor package identity | package ID/layout columns, package-targeted handler/model fields, package artifact-path derivation, package help/autocomplete, and package-generation wording | Phase 4 deletes or replaces every package identity consumer. The UI may remain implementation-oriented until Phase 10, but it may not retain package vocabulary or readers. |
| Editor/Application workflow | parallel changed/global/selected/manual-reload actions, implementation-first table presentation, general artifact-directory scans, and duplicated lifecycle/status formatting | Phase 10 converges these already package-free surfaces on `Apply Changed`, immutable shader/source views, and contextual expert provenance. |
| ignored generated output | 56 `.sparkshader` files and two `ShaderPackageRegistry.sreg` files observed under `artifacts/dev/projects/{Shared,Showcase}/cooked/Shaders` | Phase 4 removes/regenerates these disposable products after the clean-break format switch. They are not source and must not be retained as compatibility fixtures. |

`Engine/Renderer` and `Engine/RHI` use recursive `CONFIGURE_DEPENDS` source membership; Renderer has a separate recursive shader-registration object-library glob. ShaderCompiler recursively includes `Private` and `Backends`, while `ValidateShaderCompilerCli.cmake` explicitly names a representative `.sparkshader`. Consequently a move/delete phase must verify both glob inclusion/exclusion and explicit validation paths: relying on recursive discovery alone is insufficient.

### Source/include and diagnostic dispositions

The 18 files under `Passes/Deferred` are `DirectLighting.hlsl`, `DirectLightReservoirSpatial.hlsl`, `DirectLightReservoirTemporal.hlsl`, `DirectShadowSignal.hlsl`, `DirectShadowSignalCommon.hlsli`, `DirectShadowSignalDeviceAddress.hlsl`, `DirectShadowSignalNoRayQuery.hlsl`, `GBufferPacking.hlsli`, `GBufferPS.hlsl`, `GBufferUtils.hlsli`, `GBufferVS.hlsl`, `LightingComposite.hlsl`, `MotionVector.hlsli`, `SceneDepth.hlsl`, `SceneDepthUtils.hlsli`, `Sky.hlsl`, `SkyMotionVector.hlsl`, and `VisualizeBuffers.hlsl`. Phase 1 deletes the two redundant shadow roots and moves the remaining sixteen files, registration source paths, root/relative includes, source resolver inputs, dependency/hash diagnostics, CMake/source-group presentation, and documentation spellings to semantic virtual-source ownership, then deletes `Passes/Deferred`.

Current author-written identity/diagnostic repetition comprises 28 pass labels, 28 package constants, 28 binding-layout labels, and 28 pipeline labels. Phase 1 deletes the two rejected shadow identities completely; Phase 2 deletes or derives the remaining pass/layout/pipeline presentation; Phase 4 deletes the remaining package constants. Compiler/cook/runtime/frontend diagnostics that currently say package ID/key/path/generation change to shader type, virtual source, target, code hash, map/library record, renderer generation, and when applicable RT composition/pipeline/table/effect identity in Phases 3 through 9. Diagnostic labels remain bounded presentation and never become lookup keys.

### Legacy-eradication search floor

The owning phase is incomplete while its exact floor returns a runtime/tool/build/current-document definition or consumer:

- Phase 1: `Passes/Deferred`, physical authored registration roots, project-first source shadowing, basename fallback, absolute authored shader includes, `DirectShadowSignalDeviceAddress*`, `DirectShadowSignalNoRayQuery*`, `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, `SPARKLE_RAY_TRACED_SHADOWS_DISABLED`, `DeviceAddressRayQuery`, `UsesAccelerationStructureDeviceAddress`, `RayTracingSceneTlasShaderAccessMode`, `SupportsShaderDeviceAddress`, `SupportsShaderDeviceAddressAccess`, `SupportsMutableDescriptorType`, `EnabledMutableDescriptorType`, `VK_EXT_mutable_descriptor_type`, shader/effect `SceneTlasGpuAddress*` fields, shader-visible raw-address conversion, and `Read(FrameGraphAccelerationStructureHandle)` as a shader-binding spelling.
- Phase 2: `TGlobalShader`, `TShaderRef`, nested `FParameters`, remaining `*PassParameters` mirrors, `RenderPassDefinition`, `RenderPassDefinitionRuntime`, `GetDefinition`, `GetParameterMetadata`, `ComputePassOperations`, the remaining 25 forwarding pass class names/files, authored `_BindingLayout`/`_Pipeline` strings, count-only parameter-layout acceptance, every `SHADER_PARAMETER_*_NAMED` / `SPARKLE_REGISTER_NAMED_GRAPH_SHADER_PARAMETER` spelling, parameter-field `LayoutName`/`ShaderName` aliases and reflection fallback, `Read(FrameGraphTextureHandle)`, `Read(FrameGraphBufferHandle)`, generic shader texture/buffer macros that conceal SRV/UAV kind, and neutral `RTV`/`DSV` authoring spellings.
- Phase 3: `CookNode`, cook-plan records used as compile identity, checkout-path-bearing include/option hashes, full-catalog changed fallback, and any persistent compiler-result/cache spelling.
- Phase 4: `RendererShaderPackages`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `ShaderPackageDefinition`, `BuildShaderPackageIdFromSourcePath`, `CookedShaderPackage`, `LoadedShaderPackage`, `ShaderPackageLayoutBuilder`, `PipelineRuntimePackageRequest`, `ShaderPackageGeneration`, `GetShaderPackageGeneration`, package readers/writers/cache/identity/path/schema dispatch, `ShaderPackageRegistry`, `.sparkshader`, `--package`, `inspect-package`, `ShaderRecookRequestType::PackageId`, and all package-named Application/Editor request/publication/model/help/diagnostic fields.
- Phase 5: `GraphicsShaderPipelineState`, `RasterPassPipelineRuntime`, one-value `RhiVertexLayoutKind`, eager base/wireframe/two-sided runtime bundles, caller-authored target formats/count/depth/sample facts, shader-pair-only graphics keys, generic binding-time view/material policy, duplicate target bind/clear or topology routes, ignored neutral descriptor fields, and compatibility draw overloads.
- Phase 6: compiler-only RT records/rejection, ambiguous `SupportsRayTracing`, duplicate RT registry, disabled RT public facade, backend/graph bypass, native identifiers outside backend-private RHI, compute-disguised trace, second runtime cache/generation, and stale pipeline/table generation acceptance.
- Phase 7: ambiguous `GBufferMode::Raytraced` execution meaning, duplicated GBuffer semantic/output/scene owners, two scheduled frontends, hidden strict fallback, and compatibility execution enum values.
- Phase 8: constant-zero Renderer TLAS contributions where production mapping is required, duplicate scene/SBT mapping, effect-local table authority, large/transient local records, unconditional table/TLAS rebuild, and divergent alpha policy.
- Phase 9: deep per-pass API selection, partial strict-frame scheduling, hidden automatic fallback, duplicate execution settings/plan/history, ambiguous algorithm-versus-API labels, and unclassified ray-query effects.
- Phase 10: parallel changed/global/selected/manual-reload controls, implementation-first table presentation, general artifact-directory scans, duplicate shader operation status/log formatting, normal-path manual reload, and native backend controls in the frontend. The Phase 4 package floor must remain clean.
- Phase 11: any residual or semantic equivalent of the Phase 1-10 floor, plus compatibility aliases/adapters/readers/writers, dual authorities, migration diagnostics, and renamed legacy owners.

Historical rationale may quote a rejected name only when explicitly marked historical; executable paths, current-state descriptions, target examples, and implementation prompts must use the vocabulary valid for their phase boundary.

### Baseline provenance and blocked final claims

The only revision-pinned Phase 0 evidence is this static source/build/document inventory at `5b0bd1469339897eef1fde3e5c9ab07137860d0f`. The ignored `Shared` and `Showcase` cooked outputs were observed on disk on 2026-08-23, but they do not embed evidence sufficient to bind them to that revision, build configuration, compiler versions, backend, adapter/driver, workload, command line, or capture. They are therefore artifact inventory only.

No pre-edit, revision-pinned D3D12 runtime capture, Vulkan runtime capture, paired-backend shader-cook result, shader-reload/retirement run, first-use pipeline timing, frame-time distribution, memory/size comparison, or external PIX/RenderDoc/Nsight identity trace was found with the complete provenance required by [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md). The candidate logs under `artifacts/validation/renderer-scene-view-frame-phase7` were inspected and rejected as a shader baseline: they do not bind revision/compiler/cook/capture/performance provenance, the final D3D12 stderr contains only unrelated level warnings, and the final Vulkan stderr records provider fallbacks followed by `VK_ERROR_DEVICE_LOST`. The corresponding Phase 12 runtime, capture, retirement, backend-parity, and performance-regression claims are **blocked unless Phase 12 acquires fresh evidence for the complete candidate**. Phase 0 makes no build, cook, runtime, visual, capture, or performance claim and does not treat an old log or the presence of generated files as a baseline.

### Ray-tracing Phase 0 extension

The same revision-pinned source audit freezes the RT starting point:

| Surface | Current source-backed state | Unified disposition |
| --- | --- | --- |
| inline traversal | one shared `RayTracingTraceQuery.hlsli` implements `RayQuery`, `TraceRayInline`, and `Proceed`; GBuffer, shadow, path/reference, and ReSTIR HLSL reach it through `RayTracingSceneTlas`, `PathTrace`, `PathLighting`, and shadow helpers | preserve as the first-class inline frontend and parity oracle; semantic kernels become shared effect owners in Phases 7-9 |
| renderer RT shader registrations | zero uses of `IMPLEMENT_RAY_TRACING_SHADER` or `IMPLEMENT_RAY_TRACING_HIT_GROUP` outside their generic macro definitions | Phase 6 adds final typed RT declarations only with their map/library, native pipeline, SBT, graph, and conformance consumers |
| compiler/package RT metadata | generic stage types and macros plus package export/hit-group/local-record/payload/attribute/recursion schema, planning, writing, inspection, and validation exist | Phase 4 deletes the package-shaped compiler-only representation; Phase 6 introduces the final map/library representation with execution in the same CL |
| runtime behavior | valid RT-library packages are deliberately rejected because state-object execution is unavailable | Phase 6 removes the rejection only when paired D3D12/Vulkan typed graph execution replaces it |
| native execution | no production `ID3D12StateObject`, `SetPipelineState1`, `DispatchRays`, `vkCreateRayTracingPipelinesKHR`, `vkGetRayTracingShaderGroupHandlesKHR`, or `vkCmdTraceRaysKHR` call exists; a third-party D3D12 helper mentions state objects but is not a production path | Phase 6 owns the complete neutral and paired backend vertical slice; no backend-only or disabled public checkpoint |
| capability authority | `SupportsRayTracing` ambiguously means AS support while inline-query and Vulkan pipeline feature probes are separate | Phase 6 clean-breaks this into independent acceleration-structure, inline-query, and RT-pipeline readiness and updates every producer/consumer |
| frame graph and runtime | raster/compute/transfer/provider graph paths and generation retirement exist; no typed trace pass, RT pipeline runtime, SBT runtime, or trace command exists | Phase 6 extends the existing graph/cache/generation owners; it does not add an RT service locator or compute disguise |
| scene/SBT mapping | RHI classic and partitioned TLAS descriptors/native builders preserve `InstanceContributionToHitGroupIndex`; both Renderer builders publish constant zero | Phase 7 uses intentional all-zero opaque mapping; Phase 8 introduces the one authoritative nontrivial instance/geometry/ray-type contribution plan for classic and partitioned TLAS |
| effect selection | `GBufferMode::Raytraced` currently means inline compute; lighting modes select algorithms, not the execution API; `CanUseInlineRayQueryShadows` has no selection consumer | Phases 7-9 separate algorithm/effect choice from `RayTracingExecutionMode`, resolve one immutable plan before graph construction, and delete ambiguous/unused policy |

`RaytracedGBuffer` is frozen as the first product parity route. Both frontends use the same prepared scene, TLAS, geometry/material buffers, view, output attachments, hit reconstruction, motion/depth conventions, and explicit rasterized-GBuffer alternative. The initial pipeline uses ray generation, miss, and opaque triangle closest hit; one ray type; recursion depth one; global resources; and no local SBT data. Phase 8 adds alpha any-hit, shadow visibility, and the nontrivial index formula. Procedural intersection and callable support are proved through existing validation surfaces or a temporary local conformance harness removed before handoff rather than by fake empty stages in product effects or submitted test scaffolding.

No revision-pinned current inline D3D12/Vulkan parity capture, valid-library rejection transcript, compiler target matrix, native-feature absence report, or RT performance baseline with complete provenance was found. Those Phase 12 claims are blocked pending fresh final-candidate evidence.

### Phase 0 evidence and Code Review gate

The following read-only command families were run from the repository root at the frozen revision. Each was paired with bounded reads of the defining owner and representative producer/consumer in the ledgers above:

```powershell
git status --short --branch
git diff --cached --name-only
git diff --name-status 44c2f192a82947d9dcdd0e4bbd7ba0cb1a7145e4..5b0bd1469339897eef1fde3e5c9ab07137860d0f -- Engine Tools
rg -n "IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE|IMPLEMENT_GLOBAL_SHADER|IMPLEMENT_RAY_TRACING_SHADER|IMPLEMENT_RAY_TRACING_HIT_GROUP" Engine/Renderer
rg -n "TGlobalShader|BEGIN_SHADER_PARAMETER_STRUCT|SHADER_PARAMETER_" Engine/Renderer
rg -n "PassName|GetDefinition|GetParameterMetadata|::Execute\(" Engine/Renderer/Private/Passes
rg -n "builder\.(Read|CreateSRV|CreateUAV|CreateRenderTarget|CreateDepthTarget)\(" Engine/Renderer/Private/Passes Engine/Renderer/Private/Frame
rg -n -i --glob "!**/ThirdParty/**" --glob "!artifacts/**" --glob "!Docs/**" "ShaderPackage|shader package|shader-package|\.sparkshader|ShaderPackageRegistry|ShaderRecookRequestType::PackageId|--package|inspect-package|PackageId|PackageKey|PackagePath" Engine Tools CMakeLists.txt cmake
rg --files Engine/Assets/Shaders/Passes/Deferred
rg --files artifacts/dev/projects -g "*.sparkshader" -g "ShaderPackageRegistry.sreg"
rg -n "ID3D12StateObject|SetPipelineState1|DispatchRays|vkCreateRayTracingPipelinesKHR|vkGetRayTracingShaderGroupHandlesKHR|vkCmdTraceRaysKHR" Engine/RHI Engine/Renderer --glob "!**/ThirdParty/**"
rg -n "DirectShadowSignal(DeviceAddress|NoRayQuery)|CanUseInlineRayQueryShadows|SceneTlasGpuAddress|RayTracingSceneTlasShaderAccessMode" Engine Tools
rg -n "Passes/Deferred|ShaderPackage|sparkshader|RendererSceneViewFrameArchitecture" Docs
```

| Phase 0 AC | Cheapest claim-falsifying evidence | Result |
| --- | --- | --- |
| every shader/pass/package field and material consumer has one target owner or deletion | exact declaration/use searches above; complete shader table, pass classification, field-authority ledger, and package owner ledger; representative reverse reads from graph consumers to registration/runtime owners | **PASS** - 29 shader classes/schemas, 28 pass surfaces, 392 nested fields, 352 authored pass-side shader-field copies, GBuffer material drawing, and every package surface have an explicit retained owner or one deletion phase |
| every forwarding pass and duplicate parameter schema has one Phase 1 or Phase 2 disposition | exact `PassName`/definition/metadata/Execute searches plus all 28 wrapper pairs; bounded Execute-body inspection | **PASS** - two unconsumed shadow variants are Phase 1 deletions; 25 remaining compute wrappers are Phase 2 deletions; `GBufferPass` alone is retained for demonstrated graphics/draw behavior |
| every package reader/writer/cache/identity/generation spelling has one Phase 1 or Phase 4 disposition | the exact 942-line/187-file package floor and the Renderer/RHI/Core/ShaderCompiler/CLI/Application/Editor/generated-output owner rows | **PASS** - the two rejected shadow identities are deleted in Phase 1 and every remaining package producer/consumer/format/generation spelling is deleted or replaced atomically in Phase 4; Phase 10 owns only package-free workflow simplification |
| every resource view/attachment, graphics-state, and RT item has one disposition | exact graph vocabulary, graphics state/key/variant/attachment/topology, RT macro/native-call, capability, TLAS contribution, direct-shadow, runtime-rejection, graph/runtime, and effect-selection searches plus bounded backend/consumer reads | **PASS** - resource routes are split between Phases 1 and 2; package-shaped RT scaffolding is deleted in Phase 4; graphics-state ownership is Phase 5; complete RT runtime/effect/table/selection work is owned by Phases 6-10; the target-state RT document owns no implementation task |
| missing baseline evidence is explicit | ignored-artifact inventory and bounded inspection of candidate `artifacts/validation/renderer-scene-view-frame-phase7` logs against the provenance standard | **PASS** - generated files are inventory only; invalid/unbound candidate logs are rejected; all final runtime/backend/capture/retirement/performance claims remain blocked for Phase 12 |
| frozen eradication floor covers exact and semantic leftovers | phase-by-phase exact floor plus alias/adapter/fallback/parallel authority/generated-format/build/frontend/document semantic searches required by the common delivery contract | **PASS** - every current match is assigned to one later phase; no generic cleanup owner, compatibility checkpoint, permutation, precache, or persistent compiler-result store is authorized |
| scoped documentation and review gate | documentation diff inspection, local target/anchor resolution for touched documents, staging/scope checks, and `git diff --check`; review route: [SparkleEngine Code Review](../../Engineering/CodeReview.md) | **PASS** - documentation-only scope, no P0-P2 finding, no runtime/tool/build/generated edit, and no executable evidence claimed |

Phase 0 is documentation-only and has no runtime performance class. The target is not implemented by this evidence; it is now sufficiently owned, ordered, and falsifiable for Phase 1 to begin without an intermediate architecture or unassigned legacy cleanup.

## End-to-End Shader Pipeline Atlas

This section is the complete learning map requested for shader work. It deliberately includes stages that Sparkle does not implement yet so that source compilation, runtime loading, pipeline compilation, and GPU execution cannot be collapsed into one vague activity called "loading a shader."

Status language in the tables is normative:

- **Current** is proved by the reviewed Sparkle source.
- **Recommended** is the selected target design.
- **Optional** is a valid alternative that is justified only by a measured workload or platform need.
- **Deferred** is intentionally outside the current migration, not silently implemented.
- **Unsupported** means a declaration or API shape must not be advertised as an executable feature.

```text
AUTHORING                    COMPILATION AND COOK                 RUNTIME AND EXECUTION
=========                    ====================                 =====================

source mounts                catalog selection                   open shader map + code index
    |                             |                                      |
source + includes ----------> preprocess + dependency graph              |
    |                             |                                      |
shader type + entry --------> immutable request + scheduled job          |
    |                             |                                      |
parameter ABI --------------> front end -> IR -> optimize/codegen        |
    |                             |                                      |
shader class + Parameters ---> reflection + ABI validation              |
    |                             |                                      |
RDG pass                         bounded compile/publication               |
                                  |                                      |
                                  +-> shader map + code records ----------+
                                  +-> symbols/provenance                  |
                                  +-> map/library metadata                v
                                                                  resolve typed shader ref
                                                                          |
                                                               backend-specific native input
                                                                          |
                                                             complete pipeline description
                                                                          |
                                                          lazy native pipeline materialization
                                                                          |
                                               +--------------------------+--------------------+
                                               |                          |                    |
                                         graphics draw              compute dispatch      RT trace rays
                                                                                              |
                                                                       RT pipeline + identifiers + SBT

OPERATIONS ACROSS EVERY STAGE

diagnostics -> replay -> inspection/disassembly -> captures/counters -> hot reload
            -> generation swap -> GPU-safe retirement -> regression evidence
```

### Shader Definition and Graph Use

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 1. Platform and capability contract | lowest common feature set; per-platform shader platforms; runtime feature branching | Define explicit shader-platform capabilities and compile eligibility; reject unsupported combinations before jobs are scheduled. | More platform records, but failures become deterministic and the base request set stays bounded. | DXIL/SM and SPIR-V targets plus package feature flags exist; some feature validation and consumption branches are incomplete. |
| 2. Source namespace/import | physical paths; basename lookup; virtual mounted paths | Unreal-style `/Engine`, `/Project`, and `/Plugin/<Name>` virtual paths mapped to physical roots by tooling. | A mount table and collision rules are required; checkout-independent identity and portable diagnostics are gained. | Physical project-first/engine-second lookup, absolute includes, and basename package fallback. |
| 3. Language and modules | HLSL only; Slang only; HLSL plus opt-in Slang; generated kernels | Keep HLSL/DXC as the production reference path. Keep Slang as an explicit backend/language experiment until policy and output parity are proved. Permit generated source only through one provenance-recorded generator with source maps and deterministic regeneration. | One reference path is less flexible; two unqualified paths multiply conformance work. | `.hlsl` auto-selects DXC; `.slang` auto-selects Slang. Explicit backend selection can override that convention. |
| 4. Includes and reusable code | textual includes; language modules; generated amalgamation | Preserve small textual includes now, record the complete dependency graph, and evaluate Slang modules only where generics/composition remove measured duplication. | Textual preprocessing is simple but recompiles shared text; modules improve composition at toolchain and cache-complexity cost. | Recursive includes, `#pragma once`, recursion detection, and line directives exist. |
| 5. Executable shader type | infer every file; string registration; generated catalog; typed C++ registration | Lean immutable shader class with direct-binding parameters only when used, explicit virtual source/entry/stage, optional compile/RT ABI hooks, and declaration location. | Authors still state irreducible facts; invalid files and entry points are not guessed. | Mutable `GlobalShaderRegistry` entries with explicit source/entry/stage and repeated package string. |
| 6. Parameter and GPU ABI | hand-authored root signatures/layouts; reflection-only binding; typed parameter metadata | One typed shader-visible declaration drives reflection validation, backend layouts, and binding; reuse it directly or compose it into an RDG pass envelope with graph-only fields. | Code generation/metadata is needed; composition must preserve one owner for every shader-visible field. | Shader `FParameters` and pass parameters are separate; runtime has strong record validation but `PassBinder` retains a count-only compatibility path. |
| 7. Variants | preprocessor permutations; specialization constants; dynamic branches; separate source files | Keep one variant per shader/target in the unified migration; design typed permutations only as a later measured extension. | Avoids unused authoring/cache/map complexity now; later variants require one atomic extension. | Free-form defines and specialization records exist; no typed permutation domain. |
| 8. Stage composition | filename grouping; manual package grouping; typed stage types at use | Compute names one shader type; graphics names concrete vertex/pixel types with the real pipeline description; ray tracing uses a focused typed pipeline composition for exports and hit groups. | No universal program layer; genuine multi-stage call sites remain explicit. | Package ID groups stages; `GBuffer` is the only current VS+PS grouping. |
| 9. Pass/RDG declaration | pass strings select bytecode; pass owns shader references; implicit global lookup | Direct compute dispatch consumes `Shader::Parameters`; graphics/shaderless work uses a narrow real-owner envelope; default label derives from shader type with instance override. | Deletes forwarding wrappers while preserving real feature owners. | Pass name, package ID, binding-layout name, pipeline name, and duplicate parameters are repeated. |

### Compilation, Validation, Cooking, and Publication

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 10. Selection | compile all; package/shader selection; changed dependency closure; on-demand runtime compile | Cook all registered shader types for release; use shader-type selection for tools and reverse-dependency selection for development. Do not compile at shipping runtime. | Dependency metadata must be durable, integrity-checked, and regenerated with the current tool contract; development iteration becomes proportional to the edit. | CLI supports all, package, or shader ID. Editor `Changed` and `Global` both launch an unfiltered `cook`. |
| 11. Eligibility | compile every registered target; capability filter before scheduling; runtime compile | Apply target/stage/feature eligibility before one-variant jobs; do not compile at shipping runtime. | Simple bounded selection now; permutation enumeration is deferred. | Target capability skips exist. |
| 12. Preprocessing/dependencies | compiler-owned preprocessing; engine-owned preprocessing; both | Use one canonical Sparkle preprocessing/dependency pass for identity and diagnostics, then invoke the backend with controlled input. Validate that backend include behavior cannot introduce hidden inputs. | Engine preprocessing gives portability and replayability but must track compiler semantics accurately. | Sparkle preprocesses before both DXC and Slang and hashes the include closure. |
| 13. Compile request/input hash | path/timestamp key; package-scoped key; full content-addressed input hash | Immutable request plus `ShaderCompileInputHash` over virtual source closure, entry/stage, environment, compiler-affecting parameter metadata, target, backend, and compiler version. Exclude validation-only and package/pass presentation identity. | Larger hash construction cost; safe in-operation deduplication and deterministic invalidation. | `compileInputHash` combines source, include-closure, option, backend, and compiler-version identity for diagnostics; it is not a persisted-result lookup key. |
| 14. Scheduling/isolation | serial; in-process task parallelism; persistent local worker processes | Keep bounded `SparkleTasks` jobs now. Add priority, in-flight dedupe, cancellation, and memory budgets. Add local worker processes only when compiler isolation or scale is measured. | Processes isolate crashes/leaks and bypass compiler locks but add IPC, startup, deployment, and debugging cost. | One out-of-process cooker, 1-8 in-process compiler sessions, shallow cancellation, no priority/dedupe/memory ceiling. |
| 15. Front end and intermediate form | DXC HLSL to DXIL/SPIR-V; Slang to DXIL/SPIR-V; source-specific compilers | DXC is the release oracle for HLSL. Slang output is accepted only after the same options, reflection, diagnostics, symbols, and paired-backend tests pass. | Compiler diversity finds issues and enables language features but doubles versioning and reproducibility obligations. | DXC is feature-complete for current HLSL. Slang is narrower and does not honor all cook policy switches. |
| 16. Optimization/code generation | debug/O0; optimized/O3; separate analysis variants; profile-guided/vendor compilation | Cook optimized runtime bytecode; produce replayable debug/symbol artifacts by policy; compare disassembly and resource use for selected acceptance shaders. | Debuggable code may differ from optimized execution; analysis must preserve provenance to avoid comparing the wrong binary. | DXC applies optimization/debug/strip policy; Slang policy parity is incomplete. |
| 17. Reflection and ABI validation | trust hand-authored bindings; runtime reflection; offline reflection plus runtime signature | Extract reflection for every compilation, normalize it, compare against typed parameters, persist a strong signature in cooked output, and revalidate integrity at load. | More cooker work and schema data; binding failures move out of rendering. | Strong cooker/runtime record validation exists; the authoring side still has duplicate parameter authorities. |
| 18. Static shader analysis | compiler warnings only; lint/validation; DXIL/SPIR-V validation; vendor ISA analysis | Warnings-as-errors in owned release shaders, DXIL/SPIR-V validators, bounded statistics, and opt-in RGA/Nsight/PIX analysis for representative hot shaders. | Vendor analysis is hardware/tool-version specific and cannot be a hermetic gate. | Warnings policy and cooked statistics exist; no complete validation/ISA regression lane. |
| 19. Compile diagnostics | console errors; dump every job; dump failures; reproducible job bundle | Structured source diagnostics plus a failure-first replay bundle containing preprocessed source, dependencies, arguments, compiler identity, reflection, and a one-job command. | Bundles consume storage; failure-only default controls bloat. | Successful opt-in DXC bundles exist; failed compiles and Slang do not have equivalent replay artifacts. |
| 20. Compile execution | compile every selected job; deduplicate identical jobs within one cook | Compile every selected shader input and deduplicate identical in-flight jobs only within the active cook. Persist no compiler-result data and expose no storage/configuration surface. | Repeated cooks spend compiler time again; the implementation has fewer owners, formats, invalidation rules, failure modes, and cleanup paths. | The local artifact store, key/status fields, cache directory, CLI flags, and Launcher controls are deleted in the current worktree. |
| 21. Cooked logical map | per-pass file; global shader map; material/asset-local shader maps | Generated `GlobalShaderMap` from shader type/target to code hash and ABI metadata; no base program manifest. | Map indirection gives typed lookup and stable identity. | Per-package `.sparkshader`; generated registry is published but runtime does not use it as lookup authority. |
| 22. Code records and physical library | code embedded per package; indexed records; one library; project/plugin/chunk libraries | Emit exact code hashes and map references. Merge/compress/chunk only after measured byte, I/O, or patch benefit; never expose membership to passes. | A simple index is proportionate; richer libraries add formats and failure modes. | Bytecode is duplicated inside package containers. |
| 23. Compression/chunking | loose uncompressed records; block compression; whole-library compression; platform containers | Keep the first indexed migration format simple. Add independently compressed blocks/chunks only when startup, size, patch, or preload measurements justify them and retain integrity coverage. | Small blocks stream well but compress less; large blocks compress well but amplify reads and patch deltas; uncompressed records may win at current scale. | One container per program; no cross-program compression/chunk policy. |
| 24. Publication/security | overwrite in place; temporary files plus rename; generation manifest; signing | Deterministic transactional generation publication with hashes, schema/toolchain provenance, rollback, and optional platform signing at release packaging. | Keeping generations costs disk; it prevents partial activation and makes failures auditable. | Transactional packages, registry, and recook signal with stale-generation rejection already exist. |

### Runtime Loading, Residency, Pipelines, and Execution

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 25. Open and index | scan loose files; direct computed paths; open map/library indexes | Open and validate active generation map/library indexes once. | Index memory and startup validation buy deterministic lookup. | Runtime computes a path from handwritten package ID and caches loaded packages. |
| 26. Code loading | eager all; synchronous lazy; async preload; memory mapping; streaming | Preserve lazy materialization during graph construction and measure it; do not add preload/streaming state in the base migration. | Minimal new machinery; evidence can justify a later policy. | Per-package code is loaded on first graph materialization. |
| 27. Code lifetime/compression | uncompressed records; transient decompression; retained code; budgeted LRU | Tie map/library/live objects to one runtime generation and GPU retirement. Add compression/eviction only from measured need. | Simple retention is proportionate; extra states are deferred. | Live package/runtime objects follow materialized pass generation. |
| 28. RHI shader/module creation | backend consumes bytecode during pipeline creation; transient module; reusable shader object | Preserve graph-time lazy materialization outside Execute. Treat native shader/module lifetime as backend-specific: D3D12 consumes bytecode for pipelines; Vulkan modules may be transient after pipeline creation. | Retaining native objects may reduce repeated creation but consumes memory and is not portable. | D3D12 consumes bytecode in pipeline creation; Vulkan creates shader modules during runtime materialization. |
| 29. Binding layout/root signature | per-pass hand-authored; reflection-generated; typed ABI-generated and reflection-verified | Generate backend-neutral layout intent from typed parameters, validate cooked reflection, then create/cache native root signatures or pipeline layouts by strong layout signature. | Fully generated layouts constrain bespoke root-layout tuning; allow explicit, reviewed policy hooks rather than parallel declarations. | Cooked layouts and runtime creation exist, but debug names/package lookup and parameter declarations repeat. |
| 30. Logical pipeline descriptor/key | pointer identity; ad hoc hash; complete canonical descriptor | Canonical `RenderPipelineKey` over shader code hashes, layout signature, specialization, and every API-visible fixed-function field. | Canonicalization and completeness are exacting; incomplete keys cause incorrect reuse. | A Vulkan key-shaped value is computed then discarded; no shared renderer pipeline-key authority. |
| 31. Pipeline-description reuse | no cache; per-generation owner; in-memory dedupe; recorded usage database | Keep `RenderPassRuntimeCache` as the sole per-generation owner and preserve its current lazy reuse. Do not add a second descriptor cache or recorded database in the base migration. | This avoids a new authority; measured duplicate creation can justify a later focused cache proposal. | Each pass runtime owns pipelines; there is no cross-pass descriptor cache or recorded database. |
| 32. Native driver cache/binary | rely on opaque driver cache; D3D12 cached blob/library; Vulkan pipeline cache/binaries | Defer all explicit native-cache paths until measured lazy pipeline creation justifies a separate proposal. | Native data is adapter/driver/API-version sensitive. | D3D12 passes an empty `CachedPSO`; Vulkan passes `VK_NULL_HANDLE`. |
| 33. Pipeline compilation/linking | synchronous first use; async creation; partial libraries; shader objects | Preserve synchronous graph-time creation of complete conventional pipelines for the base migration. Defer async creation, graphics-pipeline libraries, partial programs, and shader objects until measured first-use evidence justifies them. | Monolithic pipelines are simple and optimizable but can be expensive to create; the base change avoids adding scheduling and state before that cost is demonstrated. | Graphics/compute pipelines are created synchronously during first runtime materialization. |
| 34. Prewarming policy | compile every PSO; explicit precache; recorded cache; hybrid | Explicitly deferred; add no prewarm state or authoring now. | Avoids unproved startup/runtime complexity. | No precache coordinator. |
| 35. First-use behavior | block; skip; fallback; lazy graph materialization | Preserve current graph-construction materialization, keep it out of Execute, and measure its cost. | A hitch remains possible but is evidence for a later proposal, not a reason to prebuild machinery now. | First use can synchronously create during graph construction. |
| 36. RDG declaration and scheduling | immediate calls; untyped graph node; typed graph pass | `Shader::Parameters` declares resources/queues; graph compilation owns hazards; typed shader lookup resolves before Execute. | Provides lifetime, synchronization, async compute, and diagnostics with less frontend code. | Frame graph already supports typed draw/dispatch but duplicates pass schemas. |
| 37. Binding and dispatch | bind by slot; reflection name; typed parameters | Bind validated shader/layout/pipeline and `Shader::Parameters`, then draw/dispatch. Execute performs no I/O, compile, lookup, layout, or pipeline creation. | Execution stays deterministic. | Current Execute binds and dispatches only. |

### Diagnostics, Iteration, Analysis, and Retirement

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 38. Semantic labels and provenance | human names only; hashes only; both | Carry readable event/shader names plus stable shader-type, code, pipeline, and generation identities into bounded markers, object names, captures, and crash records. | Extra metadata has storage/runtime cost; bounded development metadata makes evidence joinable without cluttering normal control flow. | Readable names exist, stable cooker-to-capture hash correlation does not. |
| 39. Source debugging | visual output; shader printf/asserts; source debugger; replay capture | Support all four: cheap visualization first, bounded debug instrumentation, external PIX/RenderDoc/Nsight debugging, and reproducible compiler/capture artifacts. | Debug builds and instrumentation alter timing and code generation; conclusions must identify the binary used. | Visualize Buffers and capture labels exist; symbol/provenance workflow is incomplete. |
| 40. Performance analysis | compiler statistics; IR/disassembly; vendor ISA; GPU counters/timelines | Preserve DXIL/SPIR-V inspection, add selected RGA/Nsight analysis, and correlate static register/LDS data with runtime occupancy, cache, bandwidth, and latency evidence. | Static estimates do not prove runtime bottlenecks; hardware results are device/driver specific. | Cooked byte/reflection stats exist; no joined static/runtime shader evidence. |
| 41. Hot reload invalidation | reload all; timestamp scan; content/dependency invalidation; runtime patch-in-place | Changed virtual paths select reverse dependencies; produce a complete validated generation; atomically swap; never patch individual live entries. | Whole-generation publication duplicates some data briefly but keeps rollback and lifetime coherent. | Timestamp polling notices `.hlsl`, `.hlsli`, and `.slang`; the all-catalog plan recompiles every job; generation swap is safe. |
| 42. Cancellation/failure | abort process; cooperative job cancellation; keep partial output; transactional rollback | Cooperative cancellation at job/backend/process boundaries; discard incomplete generation; keep previous active generation; initial startup failure is explicit. | Some compiler calls are not interruptible; process isolation may be needed to bound cancellation. | Child process can be cancelled, but running compiler sessions are not cooperatively stopped. Rollback is strong. |
| 43. Lifetime/retirement | device idle; immediate destroy; queue-fence/token retirement | Reference resources through the active generation and retire old shader/pipeline/library state only after every recorded queue submission token completes. | More bookkeeping and transient overlap; no global device-idle reload hitch. | Implemented by `RenderPassRuntimeCache`; preserve it. |
| 44. Release evidence | successful compile; one smoke frame; paired deterministic evidence | Prove clean and repeated cooks, repeated-operation recompilation, map/library lookup, lazy materialization cost, D3D12/Vulkan captures, hash-to-source lookup, representative disassembly, fallback behavior, and latency/memory. | Evidence takes hardware time and storage. | Catalog validation and one representative CLI artifact exist; paired hardware evidence is incomplete. |

The recommended base path is intentionally conventional at the API boundary: offline high-level compilation, typed maps/libraries, conventional complete pipelines, and lazy graph-time materialization outside Execute. Asynchronous preparation, Vulkan shader objects, graphics pipeline libraries, Vulkan pipeline binaries, D3D12 partial programs, and work graphs remain options, not assumed improvements. Each adds a capability branch and requires its own proposal only after current pipeline counts, first-use timings, and a representative workload show that the simpler path is insufficient.

## Current Sparkle Inventory

### Registered Programs and Actual Frame Consumption

This table covers every current renderer shader package/program. "Live" means a frame producer can dispatch it; it does not mean every configuration executes it every frame.

| Current program/package | Stage and required feature | Producer or selection branch | Code-backed status |
| --- | --- | --- | --- |
| `ComputeClear` | compute | utility helper; used for reservoir and other clears with per-instance labels | **Live and reused.** One shader program legitimately serves several graph operations. |
| `DirectShadowSignal` | compute; descriptor indexing + AS + inline ray query | ReSTIR direct-light shadow signal | **Live when inline ray-query shadows are available and enabled.** Otherwise a shaderless graph clear publishes the same unshadowed visibility product. |
| `DirectLightReservoirTemporal` | compute | ReSTIR direct temporal reservoir | **Live in `RestirPathTraced`.** |
| `DirectLightReservoirSpatial` | compute | ReSTIR direct spatial reservoir | **Live in `RestirPathTraced`.** |
| `DirectLighting` | compute | ReSTIR direct resolve/lighting | **Live in `RestirPathTraced`.** |
| `ExposureReduceScene` | compute | first level of parallel-reduction metering | **Live when metering is `ParallelReduction`.** |
| `ExposureReduceTexture` | compute | remaining parallel-reduction levels | **Live when the reduction requires more levels.** |
| `ExposureDownsampleScene` | compute | first level of downsample-pyramid metering | **Live when metering is `DownsamplePyramid`.** |
| `ExposureDownsampleTexture` | compute | remaining downsample-pyramid levels | **Live when the pyramid requires more levels.** |
| `Exposure` | compute | exposure history/resolve after either metering branch | **Live.** Dispatched asynchronously in the frame graph. |
| `GBuffer` | vertex + pixel | `GBufferMode::Rasterized` | **Live conditional graphics program.** It proves stages need not share a file. |
| `RaytracedGBuffer` | compute; descriptor indexing + AS + inline ray query | `GBufferMode::Raytraced` | **Live conditional ray-query compute program; not an RT pipeline.** |
| `SkyMotionVector` | compute | after either GBuffer producer | **Live.** |
| `SceneDepth` | compute | depth linearization after either GBuffer producer | **Live.** |
| `PathTracedDirectLighting` | compute; descriptor indexing + AS + inline ray query | reference path-traced lighting | **Live in `ReferencePathTraced`.** |
| `PathTracedIndirectLighting` | compute; descriptor indexing + AS + inline ray query | reference path-traced lighting | **Live in `ReferencePathTraced`.** |
| `ReferenceLightingAccumulation` | compute | accumulation/finalization of reference sample | **Live in `ReferencePathTraced`.** |
| `RestirIndirectTemporal` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect temporal reservoir | **Live in `RestirPathTraced`.** |
| `RestirIndirectSpatial` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect spatial reservoir | **Live in `RestirPathTraced`.** |
| `RestirIndirectResolve` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect resolve | **Live in `RestirPathTraced`.** |
| `LightingComposite` | compute | after either lighting-mode producer | **Live.** |
| `Sky` | compute | after lighting composite | **Live.** |
| `LinearUpscale` | compute | baseline upscaling and current pre-external-upscaler path | **Live.** The current graph schedules it before an enabled external upscaler too, so selection/cost should be verified separately from shader management. |
| `VisualizeBuffers` | compute | debug view-mode branch | **Live conditional diagnostic shader.** |
| `ToneMapping` | compute | presentation when a back buffer is requested | **Live conditional on presentation.** |
| `OutputEncoding` | compute | presentation after tone mapping | **Live conditional on presentation.** |

The current catalog therefore has 26 logical program/package IDs and 27 stage registrations. Every retained program has a frame producer, although configuration and capability policy determine which conditional producers execute. All current ray/path names still execute compute shaders with inline ray queries. There is no registered ray-generation, miss, closest-hit, any-hit, intersection, or callable shader in Renderer.

### Compile, Cook, and Runtime Decision Branches

| Branch point | Current choices | What actually happens | Required target decision |
| --- | --- | --- | --- |
| cook selection | all; `--package`; `--shader-id` | default and editor Changed/Global cook the catalog; targeted CLI paths select a package or entry | Keep release-all and targeted modes; add changed-path dependency closure. |
| source language/backend | DXC and Slang; `auto` or explicit | auto uses DXC for `.hlsl` and Slang for `.slang`; all current registrations are HLSL, so production auto-cooks use DXC | DXC remains oracle; reject or label backend-policy mismatches until Slang parity tests pass. |
| target | DXIL SM 6.0-6.7; SPIR-V 1.4-1.6 | default cook requests DXIL SM 6.6 and SPIR-V 1.6; runtime contract consumes the backend-matching one | Keep one declared release target per RHI/platform and use older/newer targets only in explicit compatibility/analysis matrices. |
| stage | VS, PS, GS, HS, DS, CS; RT library metadata | cooker/schema know six raster/compute stages; Slang maps only VS/PS/CS; runtime graphics descriptors create VS plus optional PS only | Treat GS/HS/DS as schema-only until RHI descriptors and tests exist; mesh/task are unsupported; RT library is compiler-only. |
| package kind | graphics; compute; RT library | graphics/compute can reach runtime; valid RT library packages are deliberately rejected at runtime | Keep the rejection explicit until paired state-object/pipeline, SBT, command, lifetime, and tests land. |
| compiler capability filter | supported; skipped target; no targets left | RT libraries are skipped per target if backend capability is absent; DXC advertises DXIL RT library but not SPIR-V RT library; Slang advertises neither | Capabilities must be target- and policy-probed, reported in manifests, and tested rather than inferred from backend name. |
| feature flags | inline ray query; AS; descriptor indexing | planning can filter some compiler capabilities; runtime library directly checks AS and inline ray query only | Retain descriptor indexing only where the shader's actual semantic resources require it, and validate every retained feature before materialization. |
| compile execution | selected jobs; worker bound; cancellation | every selected job compiles; 1-8 bounded compiler sessions; cancellation is checked before job execution | Preserve compile-every-time behavior and add only in-operation identical-job fan-out. |
| compile policy | debug info; optimization; warnings-as-errors; strip debug | DXC applies these controls; Slang currently does not apply equivalent policy | One canonical request must either be honored or rejected as unsupported by every backend. Never silently ignore release policy. |
| analysis | none; debug-artifact directory; `cooked-shader-stats` | DXC success can emit source/arguments/disassembly/debug data; Slang is narrower; failures have no full bundle | Failure-first portable replay bundles plus optional successful analysis and backend-specific extensions. |
| task execution | serial through 8 sessions | bounded tasks, one backend instance per node, cancellation before job start | Add job priority, dedupe/fan-out, cancellation boundary, timings, and memory budget before adding another worker pool. |
| publication | selected package files + registry + signal | staged files publish transactionally; runtime still resolves manual paths | Publish a complete generation manifest and make it the lookup authority. |
| runtime backend | D3D12/DXIL; Vulkan/SPIR-V | one runtime-format binary/layout is selected and strongly validated | Preserve backend-neutral shader-type/map identity and paired-backend validation. |
| runtime creation | first materialization; replacement generation | package/layout/pipeline creation is lazy during graph construction; generation replacement is eager-validation plus atomic swap | Preserve lazy graph-time materialization and safe generation swap; measure first use without adding readiness or preload state. |
| render feature | raster/ray-query GBuffer; ReSTIR/reference lighting; exposure method; debug/presentation/upscaler branches | frame CVars/settings choose producers; package catalog itself does not prove a branch is consumed, an alternate works, or a mandatory product fails correctly | Generate a shader-use inventory and test each supported branch, alternate, and mandatory-product failure on both backends. |

### Current Renderer Flow by Configuration

```text
Frame
 |
 +-- TLAS/BLAS preparation -----------------------------------------+
 |                                                                 |
 +-- GBufferMode                                                    |
 |    +-- Rasterized ----> GBuffer VS+PS                            |
 |    `-- Raytraced -----> RaytracedGBuffer CS + inline ray query <-+
 |             |
 |             +--> SkyMotionVector CS --> SceneDepth CS
 |
 +-- LightingMode
 |    +-- RestirPathTraced
 |    |    +--> require inline query before graph construction
 |    |    +--> Direct reservoirs --> DirectShadowSignal CS --------> inline ray query
 |    |    +--> DirectLighting CS
 |    |    `--> ReSTIR indirect temporal/spatial/resolve ----------> inline ray query
 |    |
 |    `-- ReferencePathTraced
 |         +--> PathTracedDirectLighting CS -----------------------> inline ray query
 |         +--> PathTracedIndirectLighting CS ---------------------> inline ray query
 |         `--> ReferenceLightingAccumulation CS
 |
 +--> LightingComposite CS --> Sky CS
 |
 +-- ExposureMeteringMethod
 |    +-- ParallelReduction --> ReduceScene CS --> ReduceTexture CS x N
 |    `-- DownsamplePyramid --> DownsampleScene CS --> DownsampleTexture CS x N
 |                    `----------------------------------------------> Exposure CS
 |
 +--> LinearUpscale CS --> optional external upscaler
 +--> optional VisualizeBuffers CS
 `--> ToneMapping CS --> OutputEncoding CS --> copy/present
```

This is why capability declaration, successful cooking, runtime creation, and frame consumption need separate evidence states. An alternate implementation is real only when a selection owner chooses it, its resources and parameter ABI are valid, and an exercised test or capture proves its output; a fabricated product is never an alternate implementation.

## Runtime Residency and Deferred Delivery Taxonomy

Compile inputs, cooked runtime data, live backend objects, and native driver acceleration have different owners and lifetimes. Sparkle persists no compiler result. The first four rows are unified-migration responsibilities; streaming, explicit native pipeline persistence, and prewarming remain deferred research.

| Layer | Prevents or reduces | Correct key/invalidation owner | Recommended policy | Current Sparkle |
| --- | --- | --- | --- | --- |
| dependency/preprocess record | rescanning and re-deriving include closure | virtual source contents, preprocessor provenance, defines; ShaderCompiler | persist a reverse-dependency index; regenerate when the current tool contract or integrity check rejects it | closure is rebuilt and hashed; reverse graph is not a durable selection authority |
| in-flight job coalescing | duplicate concurrent compilation | full `ShaderCompileInputHash`; compile coordinator | one producer with result fan-out, priority escalation, and cancellation reference counts; retain nothing after the operation | absent |
| global shader map | repeated discovery and unstable lookup | shader type/target; ShaderCompiler/Renderer generation | immutable per-generation entries with strong references to code hashes | package registry published but not runtime authority |
| cooked code records/library | duplicate bytecode and scattered I/O | exact code hash; ShaderCompiler/Renderer generation | independently indexed and integrity checked; merge/compress only after measured benefit | bytecode embedded per package |
| OS/file/decompression cache | disk reads and decompression | physical records/chunks; platform I/O/runtime service | measured reads; add block compression and decompressed residency only if the selected format needs them | ordinary per-file reads; no explicit streaming/decompression layer |
| code and transient backend-object reuse | repeated code reads or backend object creation | code hash + backend creation identity; runtime/RHI | code/object lifetime follows map/runtime generations; keep the current lazy owner and add no second reuse authority without evidence | live objects are owned by each materialized pass runtime |
| canonical pipeline descriptor reuse | duplicate logical pipeline requests | complete `RenderPipelineKey`; Renderer | deferred; first measure current generation-bound lazy materialization | absent |
| recorded/stable PSO database | unknown content-driven PSO enumeration | stable high-level descriptor plus build/platform version; Renderer/cooker | add only when content creates combinations that bounded declarations cannot enumerate | absent |
| native pipeline cache/library/binary | repeated driver compilation/linking | native API/device/driver/cache UUID identity; RHI backend | validated, transactionally persisted, recoverable miss; never source authority | D3D12 empty cached state and Vulkan null cache |
| opaque driver internal cache | vendor-specific repeated work | driver-owned | treat as helpful but unobservable/non-portable; do not use as readiness proof | implicitly relied upon |

The accepted base runtime model is deliberately smaller:

1. At generation activation, open and validate maps/library indexes without creating every pipeline.
2. Resolve code and lazily materialize binding layouts/pipelines during graph construction, never Execute.
3. Measure map/library open, code read, layout creation, and pipeline creation time with existing evidence surfaces.
4. Refuse release while an active/reloading generation or recorded GPU submission can still reference required state.

Do not add a residency state machine, preload queue, or streamer before file count, startup I/O, code size, memory, or a measured first-use hitch demonstrates a problem.

### Deferred PSO Prewarming Research

The strategies below are retained only as future decision context. None is selected or implemented by the base phases.

| Strategy | Strength | Cost/failure mode | Recommendation |
| --- | --- | --- | --- |
| compile synchronously on first use | minimal startup and enumeration work | visible frame hitch and nondeterministic driver work | Reject for required or common pipelines. Retain only as a classified late path. |
| enumerate every theoretical PSO | strongest nominal coverage | combinatorial cook/startup/memory bloat and unused work | Reject. A future proposal may consider only measured, reachable pipeline descriptions. |
| explicit precache declarations | deterministic and reviewable for bounded shaders | authors/collectors can omit a legal state | Defer until measured first-use evidence requires it. |
| recorded/bundled usage cache | captures content combinations actually exercised | misses unplayed branches and can go stale across shader/content changes | Add with material/content PSOs; use stable descriptions, never raw transient hashes alone. |
| native driver cache only | little engine work | opaque coverage, device/driver invalidation, first-run hitches | Use as an acceleration layer, never as the only plan. |
| hybrid explicit + recorded + native | covers known global state, observed content, and repeated driver work | most coordination and telemetry | Research option only; far beyond current need. |

A future prewarming proposal must define its own minimal state/evidence contract. This migration adds none of those states or diagnostics.

## Ray Query Versus Ray-Tracing Pipeline and SBT

Sparkle must teach and report these as two different execution architectures.

```text
INLINE RAY QUERY (CURRENT)

compute shader + compute PSO
        |
        +--> bind TLAS/AS descriptor (or a supported address representation)
        +--> RayQuery / Proceed inside normal compute invocation
        `--> Dispatch

Required: BLAS/TLAS, AS resource/binding, inline-ray-query compiler/API feature
Not required: RT library exports, RT state object/pipeline, shader identifiers, SBT, DispatchRays

FULL RAY-TRACING PIPELINE (TARGET OF PHASES 5-10)

RT shader types: raygen / miss / closest-hit / any-hit / intersection / callable
        |
        +--> libraries + exports + hit groups
        +--> global/local layout associations
        +--> payload / attribute / recursion / stack policy
        v
native RT state object or VkPipeline
        |
        +--> query shader identifiers / shader-group handles
        v
SBT buffer: raygen | miss[] | hit[] | callable[]
        |
        +--> aligned records: identifier/handle + bounded local record data
        +--> instance/geometry/ray-type indexing contract
        v
bind RT pipeline + SBT regions + DispatchRays / vkCmdTraceRaysKHR
```

### Full RT Pipeline Decisions

| Concern | Options | Recommended initial implementation | Tradeoff |
| --- | --- | --- | --- |
| library/pipeline granularity | one large library/RTPSO; collections/libraries plus link; one pipeline per effect | Begin with one small complete effect pipeline whose `RayTracingPipelineComposition` names concrete RT shader-class references; do not add a universal `TRayTracingProgram` authoring type. Measure compile/link before introducing collections. | Large pipelines maximize shared optimization but are slow and hard to replace; many small pipelines duplicate work and switches. |
| shader exports and hit groups | string lists; generated typed IDs; reflection discovery | Typed declared exports/hit groups validated against compiler output; persist stable logical IDs but query native identifiers per native pipeline. | Native shader identifiers/group handles are pipeline-specific and cannot be serialized as universal shader identity. |
| global/local parameters | all global descriptors; local root/record data; bindless indices/device addresses | Derive the global layout from the selected ray-generation shader's `Parameters`; keep most resources global/bindless and SBT record data to small stable indices/constants only. Do not repeat the root schema on miss/hit/callable stages. | Fat or duplicated local records multiply authoring drift, SBT memory, upload bandwidth, and update complexity. |
| SBT organization | per instance; per geometry/material/ray type; deduplicated records with indirection | Explicit formula and one ray type in the first vertical slice; use TLAS instance contribution plus geometry/ray offsets; deduplicate material/geometry data outside SBT. | Indirection reduces memory and churn but adds shader loads and indexing complexity. |
| SBT update | rebuild every frame; patch dirty ranges; persistent GPU-generated table | CPU-build an immutable/persistent table per validated scene generation first; add dirty-range or GPU generation only from measured update cost. | Full rebuild is simple but scales poorly; incremental/GPU updates complicate synchronization and validation. |
| alignment/layout | backend-specific code paths; one conservative cross-API layout; normalized builder with backend rules | One backend-neutral record builder that applies D3D12/Vulkan handle size, base alignment, record alignment/stride, region, and bounds rules explicitly. | A conservative maximum wastes memory; backend-specific packing needs paired tests. |
| pipeline/SBT lifetime | SBT independent; rebuild on pipeline change; cache native handles | Tie SBT records to the exact native RT pipeline generation whose identifiers/group handles they contain; retire both after GPU completion. | Rebuild/upload on pipeline reload is mandatory, but stale identifiers cannot execute. |
| recursion/stack | maximum device limits; fixed conservative values; shader-derived measured policy | Start with recursion depth 1 and explicit payload/attribute contracts; query/report stack information and increase only for a demonstrated algorithm. | Higher recursion/payload/stack improves expressiveness while reducing occupancy and raising memory/driver cost. |
| dispatch integration | direct command calls; neutral RHI `TraceRays`; RDG typed RT pass | Add backend-neutral RT pipeline/SBT descriptors and a typed frame-graph ray-dispatch pass in one vertical slice. | Partial schema support without command/execution ownership creates misleading dead infrastructure. |

The current cooked schema already models RT exports, hit groups, local parameter records, payload/attribute sizes, and recursion metadata. That is compiler-only scaffolding, not a runtime feature, and Phase 4 deletes it with the old package representation instead of preserving a placeholder translation. Phase 6 introduces the final representation and its first complete consumer together: Renderer RT shader classes and typed stage references; DXIL and SPIR-V compiler validation; map/library records; native D3D12 state-object and Vulkan RT-pipeline creation; group identifier retrieval; SBT construction/alignment/indexing; RHI command recording; graph resources and synchronization; pipeline/SBT generation lifetime; and paired conformance execution. Product effects, explicit alternate algorithms, mandatory-product failure, and parity then build on that same path in Phases 7-9. No phase adds precache telemetry.

## Feature Horizon and Adoption Gates

| Feature | Value | Why it is not the current default | Adoption gate |
| --- | --- | --- | --- |
| geometry and tessellation stages | legacy programmable raster expansion/tessellation | schema/compiler awareness is not runtime pipeline support; no registered workload uses them | complete neutral/native descriptors, paired shader/program tests, and a workload that beats simpler mesh/compute/raster alternatives |
| mesh/task shaders | modern GPU-driven geometry pipeline | absent from shader-stage schema and both RHI capability paths report unsupported | paired API support, fallback path, GPU-driven workload, pipeline/cache/debug evidence |
| specialization constants | reduce source-level variants and defer values toward pipeline creation | behavior and pipeline-cache identity differ by backend; current variant budget is tiny | cross-backend semantics, canonical PSO key, cold/warm timing, and code-size evidence |
| Slang modules/generics | reusable interfaces, generics, and specialization across targets | current backend lacks release-policy/diagnostic/stage parity and all current sources are HLSL | representative shaders match ABI, output validation, symbols/replay, performance, and paired-backend results |
| Vulkan graphics pipeline library | partial precompile/link for high pipeline/material counts | another capability/cache/link path with no measured current need | monolithic creation/link is a demonstrated bottleneck and driver coverage is acceptable |
| Vulkan shader objects | more dynamic shader/state binding | moves responsibility from pipeline objects to many dynamic states and changes cache/debug assumptions | a workload benefits after correctness, state completeness, and driver evidence across target devices |
| Vulkan pipeline binaries | explicit reusable native pipeline artifacts | newer capability with strict device/driver lifecycle and distribution concerns | target platform support plus measured improvement over robust `VkPipelineCache` |
| D3D12 partial programs/advanced shader delivery | reusable/precompiled state-object components | evolving platform capability and unnecessary for 28 bounded programs | target OS/driver availability and measured pipeline-delivery bottleneck |
| work graphs/execution graphs | GPU-driven scheduling of dependent work | distinct program, memory, synchronization, debugging, and capability model | accepted workload, conventional-compute comparison, fallback, and end-to-end evidence |
| runtime source compilation | modding or developer experimentation | nondeterministic shipping startup, toolchain deployment, security, and failure containment | development-only sandbox or explicit product requirement; never the normal cooked runtime |

### Primary References for the Atlas

These sources supplement the references in [External Precedent and What Sparkle Adopts](#external-precedent-and-what-sparkle-adopts):

- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Shader Debugging Workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-debugging-workflows-unreal-engine)
- [Epic: PSO Precaching](https://dev.epicgames.com/documentation/unreal-engine/pso-precaching-for-unreal-engine)
- [Epic: Manually Creating Bundled PSO Caches](https://dev.epicgames.com/documentation/unreal-engine/manually-creating-bundled-pso-caches-in-unreal-engine)
- [Epic: `FShaderPipelineCache`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FShaderPipelineCache)
- [Microsoft: D3D12 graphics pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: `ID3D12PipelineLibrary::LoadGraphicsPipeline`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12pipelinelibrary-loadgraphicspipeline)
- [Microsoft: DXR functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Microsoft: PIX automatic shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [Khronos: Vulkan pipelines and pipeline caches](https://docs.vulkan.org/spec/latest/chapters/pipelines.html)
- [Khronos: Vulkan ray tracing and shader binding tables](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [Khronos: `VK_EXT_graphics_pipeline_library`](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_graphics_pipeline_library.html)
- [Khronos: `VK_EXT_shader_object`](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_shader_object.html)
- [Khronos: `VK_KHR_pipeline_binary`](https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_pipeline_binary.html)
- [NVIDIA: Nsight Graphics shader debugger](https://docs.nvidia.com/nsight-graphics/UserGuide/shader-debugger-overview.html)
- [NVIDIA: configuring shader debug information for Nsight](https://docs.nvidia.com/nsight-graphics/UserGuide/configure-application.html)
- [NVIDIA: DXR shader binding table tutorial](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-2)
- [NVIDIA: SBT data-layout optimization](https://developer.nvidia.com/blog/efficient-ray-tracing-with-nvidia-optix-shader-binding-table-optimization/)
- [AMD: Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [AMD: occupancy and shader resource limits](https://gpuopen.com/learn/occupancy-explained/)
- [AMD: RGA and RGP pipeline-binary interop](https://gpuopen.com/manuals/rgp_manual/rga_and_rgp_interop/)
- [He et al.: *Slang: Language Mechanisms for Extensible Real-Time Shading Systems*](https://graphics.cs.cmu.edu/projects/slang/he18_slang.pdf)

## Repository Requirements Traceability

This audit routes to the owning repository authorities instead of duplicating their rules:

- [Portfolio requirements](../../Strategy/Requirements.md) define what the finished work must prove.
- [Engineer persona](../../Strategy/EngineerPersona.md) defines the end-to-end graphics-engineering behaviors the implementation and evidence should teach.
- [Engineering standards map](../../Engineering/Standards/README.md) selects the binding implementation, graphics, tools, concurrency, naming, and validation rules.
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md) define the paired-backend stress cases and evidence shape.
- [Renderer/RHI boundary](../RendererRhiBoundary.md) owns the architectural split among Renderer policy, frame-graph scheduling, and neutral backend creation.

`Meets` below means the reviewed code implements the shader-specific invariant. It does not mean the corresponding portfolio requirement is fully proven. `Partial` means a useful production path exists but misses a required invariant or evidence gate. `Missing` means no production path or executable proof was found. `Explicitly deferred` means the repository correctly exposes the feature as unavailable rather than silently pretending it works.

### Engineering-Standard Compliance

| Authority | Current shader-lifecycle status | Required architectural response |
| --- | --- | --- |
| [Integration style](../../Engineering/Standards/IntegrationStyleGuide.md) | **Partial.** There is one real cooker and one runtime-package path, but manually repeated package/pass/debug names and two parameter declarations create parallel authorities inside that path. | Replace each duplicated authority in a bounded vertical slice and delete its old macro, fallback, or declaration in the same slice. Do not add a second shader subsystem beside the current one. |
| [Repository ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md) and [Renderer/RHI boundary](../RendererRhiBoundary.md) | **Partial.** Renderer owns concrete registrations and passes; Tools owns cooking; RHI owns neutral pipeline creation and cooked validation. The RHI-facing `GlobalShader` authoring layer still carries source/package policy that should belong to the Renderer/tooling contract. | Keep neutral bytecode, reflection, layout, shader-object, and pipeline contracts in RHI. Keep concrete shader classes, nested parameters, graph use, and runtime generation in Renderer. Keep virtual-source resolution, compiler execution, map/library publication, and cooker policy in Tools. Application/editor only orchestrates recook and activation. |
| [Graphics engineering](../../Engineering/Standards/GraphicsEngineering.md) | **Partial.** DXIL/SPIR-V compilation, reflection, package validation, backend capabilities, readable labels, and runtime-format selection exist. Paired inspection, disassembly/counters, alternate-path/failure captures, and exact hardware/driver evidence are not automated. | Make a paired-backend vertical slice the first proof; inspect layouts and IL on both targets; preserve each explicit supported alternate and reject missing mandatory work; record exact compiler, backend, hardware, driver, workload, and capture. |
| [Editor and tools](../../Engineering/Standards/EditorAndTools.md) | **Partial to strong.** Cooking is out of process; publication is transactional; stale generations are rejected; previous accepted artifacts survive failure. Cancellation is shallow, compiler memory is unbudgeted, and compile failures lack replay artifacts. | Preserve transactional replacement. Add job cancellation boundaries, bounded compiler-session memory/parallelism, progress/results integration, deterministic failure bundles, and a stable command/API usable outside the editor. |
| [Concurrency](../../Engineering/Standards/Concurrency.md) | **Partial.** Bounded work uses `SparkleTasks`, with no second general pool. It lacks priority, in-flight dedupe, compiler-session cancellation, serial-vs-N evidence, and memory-ceiling proof. | Evolve cook nodes into explicit jobs coordinated through `SparkleTasks`; never add a shader-only general worker pool. Prove serial, 1/2/N, cancellation, stale generation, failure, and memory behavior. Add worker processes only from measured compiler isolation/throughput need. |
| [Data-oriented design](../../Engineering/Standards/DataOrientedDesign.md) | **Partial.** Cook plans, reflection arrays, and compact cooked records are batch-friendly, but runtime lookup is string/path-led and physical packages duplicate bytecode. | Use immutable sorted manifest/map records and content-addressed code tables; keep human-readable strings in diagnostics, not hot lookup or cache identity. Measure layout/memory changes rather than asserting them. |
| [Naming and vocabulary](../../Engineering/Standards/NamingAndVocabulary.md) | **Partial.** Neutral RHI uses `RenderPipeline`, `GraphicsPipelineDesc`, and `ComputePipelineDesc`, while semantic pass labels are useful. Authored package IDs and generic-looking `PassName` fields conflate identities. | Derive the default event label from the concrete shader class or call site, keep an explicit label overload for real pass instances, distinguish shader type, compile input, code, map entry, and pipeline identities, and reserve stable hashes for content/cache identity. Do not leak `PSO` into neutral public RHI names. |
| [Validation, performance, and evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) | **Missing as a complete gate.** One representative CLI target exists, but no paired all-shader validation, injected-defect proof, compile-time report, first-materialization evidence, or external capture pack was found. | Extend existing product/tool validation surfaces for every replaced contract, use temporary removed-before-handoff harnesses only where necessary, then run the paired DXIL/SPIR-V cook/load slice, negative ABI/compile/reload cases, serial/N matrices, first-materialization measurements, and capture-backed evidence. A document or successful build is not acceptance evidence. |
| Frame-graph execution ownership | **Partial to strong.** RDG setup declares resources and materializes runtime state before Execute; Execute only binds and records work. Duplicate parameter metadata and count-only compatibility weaken the dependency/binding proof. | Make one schema own each shader-visible field, reuse/compose it in the pass envelope, and reject structural mismatch; never permit Execute to discover resources, load shaders, or create pipelines. |
| Reload and GPU lifetime | **Meets the reviewed invariant.** A replacement generation is fully built before activation, a failure preserves the active generation, and retirement waits on `RhiSubmissionToken` state for all queues. | Preserve this path unchanged while replacing lookup and physical storage. Test delayed GPU completion, reload churn, invalid replacements, and device-loss/error paths. |

### Portfolio Requirement Contribution

The shader architecture directly owns `PGE-09` and contributes evidence to several broader requirements. It cannot alone complete requirements that also need scene, workload, GPU-capture, productization, or adoption evidence.

| Requirement | Current contribution | Missing proof or design gate |
| --- | --- | --- |
| `PGE-01` Partner adoption and collaboration | **Indirect partial.** CLI/editor workflows and architecture notes can become an adoption surface; no second-engineer shader integration record is proved here. | Capture adopter constraints, review history, setup/cook/debug/fallback steps, measured outcome, issue/reproducer, and second-person reproduction without requiring hidden repository knowledge. |
| `PGE-02` Real-time ray tracing, GI, and path tracing | **Partial.** Sparkle has BLAS/TLAS and several path/ReSTIR compute programs using inline ray query, but no full RT pipeline/SBT; mandatory shadow visibility currently has only its inline frontend. | Prove raster, ray-query, explicit supported-alternate, and missing-mandatory-product behavior under Bistro/San Miguel quality, temporal, latency, memory, and paired-capture gates. Add a full RT pipeline only through the complete conditional vertical slice. |
| `PGE-03` Neural graphics product feature | **No shader-lifecycle feature proof.** The architecture can carry future generated or fixed inference shaders, but it does not provide a trained model, runtime inference, or classical fallback. | Reuse this exact shader-class/map/library/ABI/pipeline/provenance path for a real model artifact and shader kernels; do not create a neural-only compiler/runtime authority. Evidence remains owned by the neural workload. |
| `PGE-04` Model-to-kernel translation | **Future contribution only.** Slang is a backend seam, but no model-to-shader generator, operator contract, or generated-kernel runtime was found. | Use provenance-recorded generated virtual sources/source maps, deterministic regeneration, the normal compile request and ABI validation, numerical reference checks, latency/memory, disassembly/counters, precision/layout/fusion decisions, and classical fallback. |
| `PGE-05` Whole-system performance | **Partial.** Package sizes and runtime load microseconds exist; compile, map/library loading, native pipeline, and hitch distributions do not. | Record compile queue/wall/CPU time for every selected job, map/library open time, lazy pipeline creation, frame pacing, memory high-water, and p50/p95/p99 under a pinned workload. Native-cache, streaming, and preloading experiments belong to later measured proposals. |
| `PGE-06` Workload analysis and hard debugging | **Partial.** The catalog targets DXIL/SPIR-V and both backends expose markers/debug names, but captured shaders are not joined to compile provenance. | Capture the same workload on both APIs; inspect queues, barriers, descriptors, memory, pipelines, shaders, symbols, and one hard incident with hypotheses, experiments, root cause, and minimal reproducer. |
| `PGE-07` C++ and Python software engineering | **Partial.** A C++ CLI, out-of-process orchestration, transactional cook, and runtime validation exist; no useful Python shader-analysis automation is required or proved by this design alone. | Keep the C++ ownership narrow and tested; add Python only for a concrete report/conformance/analysis workflow; provide clean-clone commands, deterministic artifacts, and documentation matching executable behavior. |
| `PGE-08` Applied mathematics and modeling | **Indirect.** Shader infrastructure cannot prove estimator, signal-processing, stability, or cost mathematics. | Let shader-type metadata link to the owning feature's math/reference tests and preserve exact compile-input/code/capture identity so predicted cost/quality can be compared with measurement. |
| `PGE-09` Explicit APIs, shaders, compilers, and GPU ABI | **Partial.** Explicit D3D12/Vulkan, HLSL to DXIL/SPIR-V, reflection, cooked ABI validation, and diagnostics exist. | Produce a paired shader-source-to-runtime trace; inspect both compiled forms; prove layout/resource states and complete support matrix; inject defects; verify real fallbacks; join code/pipeline hashes to GPU events. |
| `PGE-10` CPU/GPU architecture and concurrency | **Partial.** Bounded cooker tasks and async-compute scheduling exist. | Compare serial/1/2/N compile execution with time/memory/cancellation; correlate IR/ISA register/LDS/scratch findings with runtime occupancy, divergence, cache/bandwidth, and synchronized queue evidence. |
| `PGE-11` Machine-learning fundamentals | **Out of shader-lifecycle scope.** No compile/package design demonstrates training, objectives, splits, optimization, quantization, or generalization. | Do not claim coverage. A future generated shader path consumes an independently validated frozen model artifact and records provenance; training evidence stays in its owning workflow. |
| `PGE-12` Training and inference workload engineering | **Partial infrastructure only.** Cooked packages are versioned, validated, atomically published, and lazy-loaded, which can support deterministic inference deployment; no real inference workload exists. | Measure export, cook, map/library open, lazy materialization, and inference latency/memory separately from training; preserve an explicit classical fallback under one normal runtime path. Variant and preload policy remain outside this unified migration. |
| `PGE-13` Productization, tools, and communication | **Partial.** CLI discovery/inspection, editor recook, and this source-linked design are credible beginnings. | Deliver edit-to-failure/replay/reload/trace workflows, stable navigation, clean cook/run, troubleshooting, bounded reports, adoption feedback, and deletion evidence for replaced concepts. |
| `PGE-14` Platform and ecosystem breadth | **Partial.** Windows D3D12/Vulkan code paths and compiler/tool references exist; native Linux/Vulkan behavior is not proved by this document. | Record OS, SDK, compiler, driver, capture/profiler, and build setup. Add native Linux/Vulkan cook-run-capture only before claiming it; keep platform limitations in the support matrix. |
| `PGE-15` Principal judgment and sustained influence | **Design target, not proof.** The proposal removes repeated authority, rejects premature streaming/RT/compiler complexity, and selects measured gates. | Demonstrate completed vertical slices, deleted old paths, fewer authored concepts, preserved capability/error quality, causal evidence, review/adoption, and a repository that became easier to explain and maintain. |

### End-to-End Lifecycle Verdict

| Lifecycle stage | Verdict from reviewed code | Recommended end state | Acceptance evidence |
| --- | --- | --- | --- |
| Write shader source | **Partial.** HLSL/HLSLI and recursive includes work, but physical search roots, absolute includes, and project-first shadowing define identity. | Canonical `/Engine`, `/Project`, and `/Plugin/<Name>` mounts; deterministic include ownership; no authored absolute path. | Mount collision, traversal, case policy, same-basename, source-move, and cross-checkout key tests. |
| Declare shader type | **Partial.** Explicit source, entry, stage, feature flags, and parameter descriptor exist. Static registration silently drops duplicates and freezes implicitly on first snapshot. | Lean immutable shader class with nested `Parameters`, declaration location, explicit catalog freeze, collision errors, and only compile hooks consumed by the base implementation. | Duplicate/late registration negative tests and a readable catalog dump. |
| Declare parameters and RDG resources | **Unsafe partial.** Typed pass resources drive graph declarations; a separate shader `FParameters` drives reflection; count-only binding compatibility can accept different layouts. | One schema owns every shader-visible field and is reused directly or composed into a pass envelope with graph-only fields; binding and structural signatures derive from that schema. | Direct one-shader, graph-only/copy, and composed-pass tests; a field reorder/kind/name/visibility/array/size defect fails before execution on both backends. |
| Name executable stages | **Partial.** Shared package strings group stages and allow the valid multi-file `GBuffer` case. | Compute dispatch names one compute shader class; graphics draw names the concrete vertex/pixel classes, narrow pass state, and prepared draw work while other PSO facts derive from their owners; RT uses only the focused typed composition required for exports/hit groups. | Direct compute, VS+PS graphics, and all-stage RT composition tests; no authored package string, universal program alias, complete caller PSO, or pass-registration macro. |
| Select permutations | **Explicitly deferred.** Variants are represented through free-form defines and separate registrations/packages. | Keep one registered variant per shader/target in the unified migration and add no permutation frontend, enumeration, callback, or cache dimension. | Final searches prove no new permutation API or policy; a future proposal must provide its own workload, owner map, and acceptance evidence. |
| Build compile input and dependencies | **Partial.** Options and transitive include contents are hashed, but physical directories and path bytes leak into identity. | One immutable compile request with virtual source identity, normalized dependency graph, compiler provenance, platform/features, parameter signature, and debug policy. | The same source tree in different checkout roots produces the same input hash; every meaningful input change invalidates it. |
| Schedule compilation | **Partial.** Bounded `SparkleTasks` execution works. | Explicit logical job identity, `ShaderCompileInputHash`, result, priority, in-flight/completed dedupe, cooperative cancellation, bounded result application, and measured worker/session budget. | Serial/1/2/N, duplicate fan-out, cancellation, backend crash/failure, stale result, and memory-ceiling tests. |
| Compile DXIL/SPIR-V | **Strong foundation, incomplete parity proof.** DXC and Slang expose target capabilities; DXC produces DXIL/SPIR-V and rich successful analysis artifacts; reflection is extracted for both formats. | A backend-neutral result contract with equivalent diagnostics/provenance and declared analysis capability differences. | Representative optimized DXIL and SPIR-V builds for all supported stage kinds; reflection/layout comparison and compiler-version record. |
| Enforce compiler policy/capabilities | **Unsafe partial.** Target/package capability filters exist, but DXC and Slang do not honor the same policy controls and schema-known stages exceed executable runtime support. | Generated matrix for language, backend, target, stage, package kind, feature, and debug/optimization/warning/symbol policy; unsupported requests fail before jobs. | Every matrix cell is produced by a capability probe and executable positive/negative test; no ignored policy or unreported target skip. |
| Compile selected shader inputs | **Simplified in the current worktree.** Every selected job invokes the compiler; the local artifact store and its key/status/configuration surface are deleted. | `ShaderCompileInputHash` is independent from package/pass presentation identity and exists only for in-operation deduplication, diagnostics, and provenance. | Repeated identical cooks both compile; identical jobs within one operation fan out one result; cancellation and deterministic publication remain proven. |
| Cook and publish runtime data | **Strong publication, coarse storage.** Per-program packages, registry, and recook signal publish transactionally; code is duplicated and registry is not runtime authority. | One generated `GlobalShaderMap` plus unique code-hash library; physical file policy remains cooker-owned and invisible to Renderer callers. | Reproducible map/library hashes, one copy per unique bytecode, transactional rollback, and map/library validation. |
| Inspect and reproduce compilation | **Partial.** Catalog/package inspection and successful opt-in artifacts exist. | One trace command plus always-available failure bundle containing virtual dependencies, preprocess output where available, exact arguments/defines, compiler/version, parameter comparison, and replay command. | Injected syntax/include/ABI/backend failures replay outside the editor and navigate to portable virtual paths. |
| Load and validate runtime shader | **Strong.** Cooked schema, target format, hashes, records, stages, features, reflection, and logical layouts are validated before use. | `ShaderRef<Shader>` resolves through the active `GlobalShaderMap` and code hash, independent of physical library filenames. | Cold/warm map/library open time and memory, corrupt/truncated/wrong-backend/wrong-layout records, and typed lookup across generation replacement. |
| Prepare code and native objects | **No additional owner required in the base migration.** Current materialization is lazy and pass-runtime-owned. | Keep `RenderPassRuntimeCache` as the sole generation/lifetime owner, materialize during graph construction, and keep Execute free of loading or creation. Add preload/readiness only in a future measured proposal. | Map/library/native-object/pipeline timing and high-water evidence; backend-specific release tests; proof that Execute performs no loading or creation. |
| Integrate with RDG | **Partial to strong.** Resource uses are declared during setup; runtime is materialized before Execute; Execute only binds and records. | `Shader::Parameters` is the one shader-visible schema consumed directly by typed draw/dispatch; only real graph-only data uses a narrow owner envelope; optional per-instance event labels remain presentation. | Direct compute, graphics, composed, and shaderless pass tests; graph resource-state explanation, async-queue legality, no hidden creation in Execute, and meaningful capture markers. |
| Create/use graphics and compute pipelines | **Unsafe graphics frontend despite a correct lazy boundary.** Complete descriptors reach RHI, but GBuffer authors a duplicated aggregate, the key is only the shader pair, variants are eager, and backends supply unowned defaults. | Derive the complete graphics key/descriptor from typed shaders, narrow pass state, attachments, and prepared mesh/material work under `RenderPassRuntimeCache`; materialize exact requests before recording. Do not add precache or native-cache integration. | Key-field perturbation, attachment-derived compatibility, exact-only variant, paired native-state/capture, cold/warm timing, generation reuse, and no creation in recording. |
| Debug/profile on GPU | **Partial.** Semantic events and native object names exist; compiler artifacts are not joined to captured shader hashes or external symbols. | Stable capture correlation record from event label and pipeline identity to shader type, code hash, virtual source, compile request, and debug symbol. | PIX and RenderDoc paired captures plus Nsight or RGA analysis on the exact cooked shader; record counters, IL/ISA, hardware, driver, API, and workload. |
| Recook and hot reload | **Strong foundation, coarse invalidation.** Out-of-process cook, transactional signal, stale rejection, rollback, generation swap, and GPU-safe retirement exist; any change plans the whole catalog. | Persist reverse dependencies, select affected shader types, publish a complete new generation, and retain current rollback/lifetime behavior. | Root/include change selection, unrelated-shader exclusion, rapid edit coalescing, invalid replacement, delayed completion, and reload-churn tests. |
| Inline ray-query capability and mandatory shadow production | **Live primary path.** Compute shaders use TLAS/inline queries; duplicate no-query and device-address shadow shaders are deleted. Vulkan private RHI already contains classic/partitioned descriptor-writing mechanics, but PTLAS descriptor readiness is not selected coherently. | One semantic acceleration-structure shader parameter and graph handle serve classic/partitioned providers. Private RHI chooses and validates the native descriptor representation. Renderer requires the current inline producer before graph construction until Phase 8 can select inline or pipeline/RGS. | Forced-provider paired tests/captures prove one shader class/source/parameter schema on classic and partitioned TLAS, exact native descriptor writes, output parity, an actionable unavailable reason, and no fabricated shadow product. |
| Full RT pipeline and SBT | **Missing at runtime.** Schema/cooker inspection exists; no renderer RT registrations, native state-object/pipeline, group identifier, SBT, or trace-rays command path exists. | Phase 6 adds one paired complete vertical slice with typed exports/hit groups, layouts, payload/attribute/recursion policy, native pipelines, SBT builder/index formula, typed graph command/lifetime/cache integration, and all-stage conformance; Phases 7-9 add product effects, supported alternates, and mandatory failure behavior. | Raygen/miss/hit/intersection/callable execution on D3D12/Vulkan; identifier-generation, alignment/index/bounds, reload/retirement, corruption, alternate selection, mandatory-producer failure, cold/warm pipeline, SBT memory/update, and capture evidence. |
| Automated conformance | **Missing as a complete gate.** The CLI validation custom target covers catalog structure and one shader artifact only. | Extend existing tool/product validation and CI evidence gates for identity, dependency, compilation, layout, schema, and paired end-to-end cook/load/reload/pipeline behavior; use temporary local harnesses only when an existing surface cannot falsify the invariant. | Named executable checks, injected failures, all-shader catalog cook, paired backends, `architecture_boundary_check`, clean diff/build evidence, and no submitted test-only scaffold unless separately authorized. |

### Required Evidence Pack

Implementation is not accepted merely when the new API compiles. The completed shader lifecycle must produce one navigable evidence pack for a pinned engine commit and workload:

1. an authoring trace from the concrete shader class and nested `Parameters` to virtual source, entry, parameter signature, compile-input hash, code hash, global-shader-map record, runtime shader reference, pipeline identity, and GPU event, plus a graphics draw trace from pass state/attachments/mesh-material facts to the complete key and native object;
2. optimized DXIL and SPIR-V artifacts for representative graphics stages and compute shaders, with reflection/layout comparison and readable compiler diagnostics;
3. one deliberately broken binding contract and one deliberately broken compile/include, both rejected with portable source locations and replayable failure bundles;
4. repeated cook results for serial, 1, 2, and N workers, including selected/compiled job counts, wall/CPU time, peak memory, cancellation, in-operation deduplication, and identical-output checks;
5. cold and warm D3D12/Vulkan pipeline results, including exact requested variant counts, attachment/state parity, graph-time creation, generation reuse, first-use frame impact, and proof that recording performs no loading or creation;
6. paired PIX and RenderDoc captures for the same representative scene and settings, plus Nsight or RGA shader analysis where the finding requires source/IL/ISA counters;
7. exact engine commit, global-shader-map/library hash, compiler backend and version, target, optimization/debug policy, GPU, driver, API, scene, camera, resolution, warm-up, capture frame, and run count;
8. editor adoption evidence: edit, changed-dependency selection, successful reload, failed replacement rollback, direct error navigation, one-job replay, and GPU-safe old-generation retirement;
9. a clean build/cook/run from documented commands and a recorded issue or minimal reproducer created by a second adopter;
10. before/after authored-string, registration, parameter-declaration, forwarding-pass, compile-job, unique-code, cooked-code/library-byte, cook-time, map/library-open, and first-materialization counts so the simplification claim is measurable;
11. a generated support matrix proving backend/target/stage/shader-type/feature/policy status and a consumer report distinguishing registered, cooked, runtime-valid, selected, and captured shaders, supported alternates, and mandatory failures;
12. runtime-generation and lazy-materialization lifetime evidence, including state-object/pipeline, SBT layout/index/update/memory, trace dispatch, alternate selection, mandatory-product failure, reload, and paired capture evidence, plus only compression/eviction metrics that actually exist.

The [performance diagnostics architecture](../Performance/Diagnostics/PerformanceDiagnosticsArchitecture.md) owns the shared measurement and capture infrastructure. This document owns the shader-specific identities and joins that make those captures traceable. Evidence records belong under the repository's evidence path selected by the acceptance workload; they must not be embedded here as claims that age with hardware, drivers, or compiler versions.

## Target Capability Requirements

These requirements describe the accepted unified migration. They intentionally exclude permutations, PSO precaching/prewarming, preload/streaming controls, and native driver caches; full RT execution is included through the focused composition and ownership model above.

### 1. Canonical Virtual Shader Sources

Create one immutable `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots. Registrations and root includes use canonical virtual paths. Relative includes resolve from the including virtual file. Physical paths are read locations only and never portable identity. Unknown/overlapping mounts, absolute authored paths, mount escape, duplicate virtual paths, case-policy collisions, and late mounts fail deterministically.

### 2. One Lean Shader Class Contract

Replace the current mixed `F*`/`T*` Unreal-prefix vocabulary with Sparkle names such as `GlobalShader<Shader>` and `ShaderRef<Shader>`. A concrete shader class owns:

- nested `Parameters` when it is the direct graph-dispatch binding owner; compute and ray generation declare them inline for the normal case;
- optional compile eligibility, environment mutation, and compiled-result validation hooks only when that shader uses them;
- no empty parameter carrier or composition-wide payload/attribute declaration on a non-dispatch RT stage.

`IMPLEMENT_GLOBAL_SHADER(Class, VirtualSource, Entry, Stage)` remains the one implementation declaration. Sparkle does not need separate `DECLARE_GLOBAL_SHADER` or `SHADER_USE_PARAMETER_STRUCT` macros because its CRTP base and implementation declaration already know the concrete type and parameter owner. Raw source scanning never substitutes for typed registration.

### 3. Frozen Shader Catalog

`ShaderTypeDesc` records deterministic `ShaderTypeId`, readable type/stage, declaration location, virtual source/entry, optional direct-binding `Parameters` metadata/signature, capability requirements, and optional compile hooks. The focused RT composition record separately owns payload/attribute/recursion and hit-group compatibility. Collect declarations, validate collisions, sort, freeze, then query. Duplicate or late declarations report both source locations and fail; static initialization order is not catalog order.

### 4. One Parameter Schema from Graph Setup through Binding

For a one-shader pass, `Shader::Parameters` is simultaneously the graph dependency declaration, parameter storage, reflection contract, structural signature, and runtime binding input. `FrameGraphBuilder::AllocParameters<Shader>()` allocates that exact nested type. Inputs/outputs use typed graph resource wrappers, so access and lifetime derive from the fields the author fills.

A real multi-stage graphics operation may compose `VertexShader::Parameters`, `PixelShader::Parameters`, and graph-only attachments in one small envelope. A ray trace uses the selected ray-generation shader's `Parameters` as its global graph/binding schema; optional local data has one bounded hit-group/stage record schema and no empty or mirrored carrier. A shaderless copy/clear pass owns graph-only parameters. No envelope restates a shader-visible field. Delete count-only compatibility and report the first structural mismatch before pipeline creation.

### 5. Thin Typed Graph Dispatch

Provide Unreal-like focused entry points over existing owners:

- `Dispatch<Shader>(parameters, groupCount)` and async equivalent for compute;
- an optional leading `RenderPassLabel` only for a distinct graph instance;
- typed graphics draw helpers that take the actual vertex/pixel shader types, parameters, narrow `RasterPassRenderState`, and prepared draw collaborator/data; attachment and mesh/material facts remain with their owners and the complete descriptor is internal;
- `TraceRays<RayGenerationShader>(composition, parameters, dimensions)` for RT pipelines, with the same optional diagnostic-label rule;
- no package/program/layout/pipeline strings at the call site.

The helper resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default label from the shader type, declares resources from `Shader::Parameters`, asks `RenderPassRuntimeCache` for the generation-bound layout/pipeline, and records binding/dispatch later through `PassCommandContext`. Delete one-method pass classes, duplicate `*PassParameters`, `GetDefinition`, `GetParameterMetadata`, and forwarding Execute bodies where the shader type and graph helper express the entire operation. Keep focused pass collaborators only when they own real mesh/draw/feature behavior.

For graphics, `RasterPassRenderState` is the only pass-authored fixed-state surface and exposes granular semantic blend/depth-stencil choices plus a dynamic stencil-reference choice where consumed. Frame-graph attachments derive target/depth formats, counts, samples, load/store/clear, and depth/stencil access. Prepared mesh/material work derives vertex input, topology, fill/cull, streams, and draw arguments. Typed shader references derive stage code and layout identity. `RenderPassRuntimeCache` combines the pipeline-affecting facts into the complete immutable `GraphicsPipelineKey`, creates the complete neutral `GraphicsPipelineDesc`, and asks the paired backend to lower it. Dynamic viewport/scissor/blend-constant/stencil-reference values remain command state and do not enter that key. No old fact is repeated in another owner.

### 6. Reproducible Compile Jobs without Persistent Compiler Results

Create one immutable `ShaderCompileRequest` and `ShaderCompileJob` per `(ShaderTypeId, Target)`. `ShaderCompileInputHash` covers the virtual source closure, entry, stage, compiler-affecting parameters/environment, target, backend/tool provenance, and compile policy. It excludes pass labels, packages, pipelines, and presentation strings.

Replace package-shaped cook nodes with jobs on the existing `TaskExecutor`. Keep one out-of-process cooker, bounded compiler sessions/memory, deterministic ordering/result merge, cancellation settlement, and in-flight duplicate fan-out. Every selected input invokes the compiler; `ShaderCompileInputHash` is identity and provenance, never a persisted-result lookup key. There is no compiler-result store, cache directory, enable/disable flag, storage schema, cache browser, or cleanup operation.

### 7. Global Shader Map and Cooked Code Library

Cooking publishes one deterministic generation:

1. `GlobalShaderMap`: sorted `(ShaderTypeId, Target) -> ShaderCodeHash + parameter signature + required runtime metadata`.
2. `CookedShaderLibrary`: validated code records addressed by exact `ShaderCodeHash`.
3. development-only provenance joining code/input hashes to virtual dependencies, compiler invocation, symbols, reflection, and diagnostics.

There is no authored or generated `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram`, or mandatory program manifest in the architecture. Graphics pipeline stage composition is the typed draw request. Ray tracing alone uses `RayTracingPipelineComposition` because a native pipeline must name exports and hit groups. Phase 4 atomically deletes `.sparkshader` package readers/writers/cache/path derivation and publishes only the raster/compute map/library representation; Phase 6 extends that same representation to RT stages while delivering its native and graph consumer in the same CL. No compatibility container or compiler-only replacement remains.

### 8. Typed Runtime Lookup and One Generation Lifetime

`ShaderRef<Shader>` resolves through the active `GlobalShaderMap`; it never derives a path or package from a source basename. `RenderPassRuntimeCache` remains the sole active/replacement/retired generation owner and stores the map/library plus materialized binding layouts/pipelines for that generation. Lazy materialization may occur during graph construction, never pass recording. Reload validates a complete replacement before activation, preserves the current generation on failure, and retires old state only after all captured submission tokens complete.

### 9. Dependency-Directed Apply Changed

Persist forward and reverse virtual-source dependencies. Editor submits `Apply Changed`; Application snapshots changed virtual paths and routes one typed request; ShaderCompiler selects affected shader types, cooks, and publishes; Renderer validates and activates one replacement. Missing/corrupt dependency metadata fails with explicit `Rebuild All` guidance rather than silently broadening the normal operation. The Editor never scans artifact directories or mutates runtime state.

### 10. Bounded Diagnostics and Traceability

Failures identify shader type, virtual source, entry, stage, target, backend/tool, compile input hash, and declaration location. One bounded replay bundle contains the canonical request, dependency closure, preprocessed source where available, arguments, compiler diagnostics, and reflection mismatch. Successful debug artifacts remain opt-in.

One read-only trace joins shader type or graph/capture label to catalog declaration, virtual dependencies, compile job/input hash, compiler result, shader-map entry, code hash/record, generation, binding layout, pipeline key, graph consumers, and external symbol/capture paths. It reads authoritative metadata and creates no registry or permanent log stream.

### 11. Honest Backend and Ray-Tracing Boundaries

Report language/backend/target/stage/feature support as supported, unsupported, compiler-only, or runtime-validated. No backend silently ignores compile policy. Inline ray-query compute shaders use the normal compute class/map/dispatch route. Acceleration-structure storage and native descriptor representation are selected below Renderer shader/effect code: one semantic parameter and one shader code record cover classic and partitioned TLAS, and provider readiness is false unless the backend can bind that parameter through its exact native descriptor contract. Phase 6 may publish ray-generation/miss/hit/intersection/callable declarations only in the same CL that consumes them through typed composition, native pipelines, SBTs, graph trace dispatch, generation lifetime, and paired conformance; there is no accepted compiler-only RT checkpoint.

### 12. Explicitly Deferred Follow-Ups

After this unified migration is accepted, a separate measured proposal may add typed shader permutations. It must preserve a zero-boilerplate one-variant default, extend the shader-map key to `(ShaderTypeId, PermutationId, Target)`, keep permutation selection typed, and update compile-input identity, map publication, and diagnostics together. It does not reopen packages or program aliases.

PSO precaching/prewarming, preload/residency/streaming controls, Vulkan pipeline cache, D3D12 cached/library paths, material shaders, and vertex factories remain outside this plan. Persistent compiler-result storage is rejected rather than deferred. A future proposal for another listed capability requires current workload evidence and its own owner/AC; none is scaffolded here.

## Ownership and Dependency Boundaries

| Owner | Owns | Must not own |
| --- | --- | --- |
| Core | generic paths, hashing, files, processes, tasks, diagnostics | shader source policy, renderer shader types, RHI compilation |
| RHI public contract | shader stages/formats, backend-neutral cooked-record validation primitives, parameter metadata primitives, shader/pipeline descriptors, RHI handles | renderer pass catalog, project source discovery, compile orchestration, runtime map policy |
| RHI backend | create/destroy backend shader and pipeline objects from validated data | source preprocessing, renderer shader/pass policy, package naming |
| Renderer | concrete global shader classes and nested parameters, graph dispatch/draw use, shader-map references, runtime generation, lazy layout/pipeline materialization | filesystem source resolution, compiler backend selection, editor process UI, precache/preload policy in this migration |
| ShaderCompiler tool | virtual source mounts, preprocessing/dependencies, compile requests/jobs, backend invocation, reflection validation, shader map/library cook and atomic publication | persisted compiler-result storage, renderer scheduling, live RHI ownership, editor workflow state |
| Application | changed-path watching, typed operation dispatch, publication notification, and activation request routing | dependency resolution, compilation algorithms, catalog construction, shader-map mutation, editor presentation, direct RHI shader construction |
| Editor | immutable shader catalog/operation presentation, semantic selection, source navigation, cancellation intent, and contextual diagnostics | artifact-directory scanning, dependency calculation, compiler invocation, publication, shader-map mutation, cache policy, direct RHI shader construction |

The generic shader authoring primitives may remain in RHI public code if they stay renderer-agnostic. Concrete shader classes and graph use stay Renderer-owned. The runtime shader map may be implemented in Renderer over generic cooked/RHI primitives; it must not cause RHI to depend on Renderer. The standalone compiler may link the registration object catalog, but no runtime module may depend on the tool.

## Preserve, Improve, Delete, and Defer

| Disposition | Item |
| --- | --- |
| Preserve | explicit source/entry/stage registration; DXIL and SPIR-V targets; DXC/Slang backend boundary; recursive preprocessing; include-closure/options hashing; reflection extraction; parameter verification for every compilation; bounded task execution; out-of-process cook; transactional cooked-output publication; validated generation swap; GPU-token retirement; strict runtime rejection of RT libraries until execution is complete |
| Improve | physical paths into virtual paths; mutable registry into validated frozen metadata; cook nodes into input-hash-deduplicated jobs; mixed Unreal prefixes into Sparkle vocabulary; package registry into the runtime `GlobalShaderMap`; change polling into reverse-dependency selection; layout count check into a strong structural signature; diagnostics into replayable per-job bundles; package-led load into typed map lookup; silent backend-policy differences into a generated conformance matrix |
| Delete now and never replace | `ShaderCacheKey`; `IShaderArtifactStore`; `LocalDiskShaderArtifactStore`; cache key/status fields; compile-result serialization; `--no-cache`; `--cache-dir`; Launcher shader-cache settings; `Build/Cache/Shaders`; the standalone shader-cache cleanup scope |
| Delete after migration | `RendererShaderPackages.h`; `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`; basename-derived package fallback; authored program/pass aliases; repeated pass/package/layout/pipeline strings; one-method shader pass wrappers; duplicated shader-visible fields across pass and shader declarations; silent duplicate suppression; count-only layout acceptance; direct runtime path construction from author package IDs |
| Defer until measured | typed permutations; shader pipeline authoring types; PSO precaching/prewarming; preload/residency/streaming controls; native driver caches; material shaders; vertex factories; persistent local worker-process pool; complex plugin loading phases; library chunk/patch system; Vulkan shader objects/pipeline binaries; D3D12 partial programs; work graphs; mesh/task shaders |
| Require one complete future slice | RT state objects/pipelines, native identifiers/group handles, SBT build/indexing, trace-rays commands, graph integration, pipeline/generation lifetime, explicit alternate selection, mandatory-product failure, and paired execution evidence |

## Error Policy

No architecture can promise "error free." This design should make errors difficult to express and deterministic to detect.

The cooker or startup validation must fail on:

- invalid, unknown, overlapping, or late source mounts
- absolute authored source/include paths or virtual paths escaping their mount
- duplicate shader type IDs
- late shader-type registration after registry freeze
- missing source, include, entry point, parameter metadata, or backend output
- source-path canonicalization collisions
- inconsistent reflected layouts across stages/backends or a parameter layout/signature mismatch
- compiler output that fails a shader type's `ValidateCompiledResult`
- corrupt or incompatible cooked map/library records
- duplicate cooked records with the same identity but different content
- a shader-map entry whose code hash, source hash, layout signature, feature set, or declared stage does not match
- a shader-map reference whose code hash is absent from the opened library
- a graph dispatch/draw whose shader parameter contract is structurally incompatible
- a requested shader or pipeline that cannot be created for the active RHI target
- a compiler backend that silently ignores a requested release/debug/optimization/warning/symbol policy
- a registered shader or frame branch whose stage/feature combination is not executable on the selected runtime backend
- an active shader-map record whose descriptor-indexing or semantic acceleration-structure requirement is not supported end to end by the active provider's graph/RHI/native descriptor contract
- an RT pipeline descriptor whose concrete shader references, export/hit-group associations, payload/attribute contracts, local/global layouts, recursion/stack policy, or native metadata are inconsistent
- an SBT whose native identifier belongs to another pipeline generation or whose region, stride, alignment, record, instance/geometry/ray-type index, or referenced data lifetime is invalid

Errors must name the shader type, virtual source, entry point, stage, target, backend/version, logical job identity and compile input hash where relevant, plus both conflicting declaration locations. Development recook errors retain the previous active generation; initial startup has no safe previous generation and fails explicitly.

## Rejected Alternatives

### Use the shader filename as `PassName`

Rejected because it couples source organization to graph semantics and fails multi-entry, multi-file, reuse, instance-label, and rename cases.

### Keep all current strings because they are explicit

Rejected because the same fact is authored in several places and can drift. Explicit source/entry/stage is valuable; repeated package, pass, layout, pipeline, or program aliases for that same shader selection are not.

### Remove cooked packages and compile everything at runtime

Rejected because it weakens deterministic releases, startup behavior, backend validation, reflection/layout validation, and failure containment.

### Treat every shader source file as an executable shader

Rejected because includes and libraries are not entry shaders, and one file can contain several entry points.

### Build one global physical package immediately

Rejected as a default because no current measurement shows that 28 files are a material problem. Logical authoring cleanup does not require a physical mega-package.

### Use compiler-generated C++ type names as serialized IDs

Rejected because they are toolchain-specific and unstable. Macro stringization or generated catalog IDs are deterministic; RTTI spellings are not.

### Copy Unreal's complete shader subsystem

Rejected because Unreal's materials, vertex factories, cook-worker scale, plugin loading phases, platform count, and content scale solve workloads Sparkle does not currently have. Sparkle should copy responsibility boundaries, invariants, typed authoring, lifetime rules, and failure behavior while keeping the implementation proportional.

### Launch one compiler process per shader immediately

Rejected as a default because Sparkle already runs the cooker out of process and executes bounded stage tasks. A serializable job boundary is required now; a persistent worker pool is added only when compiler critical sections, crashes, leaks, or throughput measurements justify its process cost.

## Implementation Contract

The unified shader and ray-tracing migration is an ordered clean break, not a menu. Each phase is one manually reviewed changelist-sized checkpoint on `master`; none may leave two authoring, parameter, lookup, cook, runtime, capability, pipeline, table, graph, or effect-selection authorities active together. A difficult consumer blocks its owning phase rather than justifying an alias, adapter, wrapper, disabled placeholder, or cleanup ticket.

### Common phase delivery contract

Every implementation prompt and every acceptance-criteria list below inherits this contract. Phase-specific references are additive; they never replace the repository process or review authorities.

Mandatory references for every phase:

- [Documentation authority](../../README.md)
- [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md)
- [Change Process](../../Engineering/Standards/ChangeProcess.md)
- [SparkleEngine Code Review](../../Engineering/CodeReview.md)
- [Coding Style](../../Engineering/Standards/CodingStyle.md)
- [Repository Structure and Ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Data-Oriented Design](../../Engineering/Standards/DataOrientedDesign.md)
- [Naming and Vocabulary](../../Engineering/Standards/NamingAndVocabulary.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Whole Repository Architecture Map](../WholeRepositoryMap.md)
- [Ray-tracing target architecture](RayTracingPipelineImplementationPlan.md)
- [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md)

Before editing, the implementer must record the phase outcome, current authority being replaced or extended, mutable and lifetime owners, producer-to-product-to-consumer route, build/generated-artifact membership, copy and complexity budget, performance classification, selected standards/workload gates, exact rejected-name search set, semantic-equivalent search set, and unrelated dirty-path exclusions. The inventory must walk definitions to all uses and representative uses back to their owner; a name-only list is insufficient.

During implementation, complete the real production route before calling the target present. Update every owned producer, consumer, constructor, reset/reload/retirement path, include, filename, build entry, generated schema/artifact, diagnostic, tool/frontend model, and current document in the phase that replaces the contract. Inspect the scoped diff after each coherent batch. Any old-to-new converter, legacy overload, alias, fallback reader, dual writer, feature flag, parallel registry/cache/generation, copied schema, forwarding facade, or renamed equivalent is a failed clean break, not a temporary convenience.

Every phase closes with one evidence table mapping each AC to its cheapest claim-falsifying check, exact command or inspection route, result, and any unavailable evidence. Acceptance requires all of the following:

- prove the target is reachable through the intended production owner and is consumed by the real downstream path; a new definition, isolated fixture, dead registration, or test-only route does not count;
- prove the replaced path cannot still produce, load, publish, select, execute, or present a result, using exact rejected-name searches plus semantic searches for equivalent fields, adapters, aliases, fallbacks, duplicated layouts, alternate generated formats, parallel directories, and stale build/tool/document consumers;
- classify every touched site as authority, composition, producer, consumer, or duplicate, and leave one mutable authority, one lifetime/generation authority, and one production path for each responsibility;
- account for every permanent type, wrapper, field, log, diagnostic, setting, and file added; delete temporary instrumentation, fault injection, local harnesses, reports, and unauthorized test scaffolding before handoff;
- run pinned no-write formatting where applicable, `git diff --check`, local-link and file/include/CMake inventory checks, and `architecture_boundary_check` whenever the Renderer/RHI boundary changes;
- apply the [Code Review](../../Engineering/CodeReview.md) procedure to the final scoped diff. A phase is `PASS` only when it has no P0-P2 finding and all evidence authorized for that phase is present; otherwise report `BLOCKED` and do not describe the phase as complete.

Source-consistency phases must not claim compile, runtime, backend, capture, or performance success. Executable phases must include a negative or corruption case that would fail if the new authority, validation, lifetime, or selection route were bypassed; a happy-path launch alone is not proof.

### Common rules for every phase

- Work directly in the unstaged `master` worktree. Do not create or switch branches and do not stage, commit, push, or submit. The user owns every source-control action.
- Apply the [common phase delivery contract](#common-phase-delivery-contract), [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../Engineering/Standards/ChangeProcess.md), and applicable subject standards. This document controls shader-specific vocabulary and ordering.
- Phase 0 is documentation-only. Phases 1 through 3 use static/source-consistency checks and do not build or run. Phase 4 runs the focused shader-map/compiler/cook checks required to prove the new physical authority. Phases 5 through 10 run the smallest focused graphics-state, backend, graph, effect, or UI checks needed by their vertical slice. Phase 11 is static legacy/ownership closure. Phase 12 performs final paired product/capture/performance validation; no earlier phase substitutes a broad build for owner-local evidence.
- Preserve one `SparkleTasks` runtime, one out-of-process cooker, one transactional publication route, one active renderer shader generation, and all-queue `RhiSubmissionToken` retirement.
- Preserve `PassCommandContext` as command/declared-resource/diagnostic infrastructure only. Pass recording performs no file I/O, compilation, shader-map/library lookup, layout creation, pipeline creation, or hidden resource discovery.
- Do not add permutations, `ShouldPrecachePermutation`, pipeline precaching/prewarming, preload/readiness/streaming controls, native driver caches, or a universal authored shader-program layer. Full RT execution is delivered only through the focused composition, RHI, backend, graph, scene, and effect owners frozen here.
- Do not encode classic/partitioned or descriptor/device-address selection in shader class names, HLSL root filenames, authored defines, effect uniforms, or graph call-site mode parameters. One semantic AS parameter is lowered by private RHI.
- Treat every consumed render product as mandatory unless the owning architecture names a real alternate algorithm. Never add a clear/copy/no-op/dummy pass merely to satisfy graph production or make missing work look successful. Until Phase 8 delivers the pipeline/RGS shadow frontend, inline ray query is the one required shadow-visibility producer and its absence fails before graph construction.
- Do not add one-field carriers, broad context/service/resource bags, a second catalog/map/runtime-generation owner, permanent migration diagnostics, per-job logging, a compiler-result browser, report generators, feature flags, compatibility formats, or submitted test scaffolding.
- Update definitions, consumers, filenames, includes, CMake/source groups, CLI/help/autocomplete, editor models, diagnostics, current documentation, and disposable generated/cooked outputs in the phase that owns their replacement.
- Preserve unrelated dirty work. Phase 0 records the path-level exclusion list and every later phase rechecks it.

### Frozen base vocabulary and navigation

| Responsibility | Target vocabulary | Canonical owner |
| --- | --- | --- |
| virtual source identity | `ShaderSourceMountTable` and canonical `/Engine`, `/Project`, `/Plugin/<Name>` paths | ShaderCompiler source/dependency capability |
| shader authoring type | `GlobalShader<Shader>` with nested `Parameters` | generic primitive in RHI public; concrete class in semantic Renderer pass/feature ownership |
| implementation registration | `IMPLEMENT_GLOBAL_SHADER(Class, VirtualSource, Entry, Stage)` | concrete shader implementation |
| immutable metadata | `ShaderTypeDesc`, `ShaderTypeId`, `GlobalShaderCatalog` | catalog built from concrete Renderer declarations and frozen before query |
| compile work | `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, `ShaderCompileResult` | ShaderCompiler compilation capability |
| cooked logical lookup | `GlobalShaderMap` | generated by ShaderCompiler; opened read-only by Renderer runtime generation |
| cooked code | `ShaderCodeRecord`, `ShaderCodeHash`, `CookedShaderLibrary` | generated cook output; neutral validation records in RHI public |
| typed runtime lookup | `ShaderRef<Shader>` | Renderer resolves through the active `GlobalShaderMap` |
| graph use | `AllocParameters<Shader>`, `Dispatch<Shader>`, typed graphics draw helpers, `TraceRays`, `RenderPassLabel` override | `FrameGraphBuilder` focused helpers over existing graph/runtime owners |
| pass-wide raster intent | `RasterPassRenderState` with granular blend/depth-stencil and dynamic stencil-reference operations | semantic mesh-pass/feature setup; never a complete pipeline or attachment description |
| graphics attachment compatibility | derived immutable attachment signature | frame graph derives it from attachment bindings and resource descriptions |
| prepared graphics work | vertex-input identity, topology, material fill/cull, streams, and draw arguments | focused mesh/material draw collaborator |
| materialized graphics pipeline identity | `GraphicsPipelineKey` -> complete internal `GraphicsPipelineDesc` | existing Renderer runtime generation assembles/retains; RHI lowers to paired backend objects |
| shader-visible scene AS | one acceleration-structure field such as `SceneTlas` and one `FrameGraphAccelerationStructureHandle` value | concrete dispatch shader declares semantics; frame graph declares access; private RHI selects classic/partitioned native descriptor representation |
| RT stage composition | `RayTracingPipelineComposition` with typed shader refs, hit groups, payload/attribute/recursion policy, and optional bounded local data | Renderer semantic effect/shader owner; never used for one-shader compute or ordinary graphics |
| RT logical table mapping | `RayTracingShaderTablePlan` and the documented instance/geometry/ray-type formula | Renderer scene/effect owner |
| neutral/native RT mechanism | opaque `RayTracingPipeline`, `RayTracingShaderTable`, and `TraceRaysDesc` | RHI public contract and D3D12/Vulkan private implementations |
| materialized layout/pipeline/table and generation | existing `RenderPassRuntimeCache` | Renderer `Private/Pipeline`; one active/replacement/retired generation for maps, pipelines, and tables |
| frontend intent | `Apply Changed` and expert `Rebuild All` | Application routing and Editor Shader Tools presentation |

Do not introduce `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram`, `TRayTracingProgram`, `SPARKLE_RENDER_PASS`, `ShaderSystem`, `ShaderManager`, `ShaderServices`, `ShaderContext`, `ShaderData2`, `NewShader*`, or Unreal `F*`/`T*` prefixes in new target names. Do not keep `PackageId` as a shader identity synonym. `RayTracingPipelineComposition` is the only scoped multi-stage composition and must never become a generic shader/pass registry.

### Consolidation map from the former RT delivery plan

No former RT task is deferred back to the target-state document:

| Former RT delivery slice | Unified owner | Why this placement is coherent |
| --- | --- | --- |
| freeze RT contract/current baseline | Phase 0 | one inventory and provenance authority covers shader, inline query, compiler-only metadata, RHI/backend absence, effects, and final blocked claims |
| graphics-state ownership and materialization | Phase 5 | final map-backed shader references, graph attachments, mesh/material facts, and pass state replace the caller aggregate before RT adds another pipeline kind |
| complete RT-library compiler toolchain | Phase 6 | implementing it against the Phase 4-deleted package schema would be throwaway work; final map/library records land with their first runtime consumer |
| backend-neutral RT contract | Phase 6 | a public contract with no paired backend/graph consumer would be a disabled placeholder |
| D3D12/Vulkan native pipelines and tables | Phase 6 | both backends, neutral arithmetic, and all-stage sentinels form one honest capability gate |
| frame graph/runtime cache/lifetime | Phase 6 | native execution cannot bypass graph/resource/generation ownership even temporarily |
| opaque GBuffer parity | Phase 7 | first product effect builds directly on the complete foundation while preserving the explicit raster algorithm |
| alpha hit semantics, shadow ray type, scene indexing | Phase 8 | adds one meaningful production slice and one nontrivial shared scene-to-SBT mapping |
| intersection and callable proof | Phase 6 | focused existing validation or a removed-before-handoff local harness proves legal stage support with the native foundation; product effects do not receive fake empty stages and no test-only fixture is submitted |
| eligible effects and whole-frame switch | Phase 9 | selection expands only after two accepted dual-mode effects and production indexing exist |
| Shader Tools/provenance | Phase 10 | the frontend describes the final map/pipeline/table/effect owners rather than an intermediate package/runtime model |
| legacy/compatibility eradication | Phase 11 | the final semantic floor runs after all source owners exist and before artifacts/evidence are regenerated |
| failure, capture, performance, release evidence | Phase 12 | one final candidate is regenerated and measured after every shader, graphics-state, and RT legacy path is gone |

### Phase 0 - Freeze the lean shader/map/graphics-pipeline contract and inventory

#### Implementation prompt

> Implement Phase 0 as one documentation and inventory CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including its pre-edit ledger, semantic-equivalent search, and mandatory Code Review `PASS`/`BLOCKED` gate. Re-run exact shader class/parameter, pass-wrapper, package, source/include, registration, compile/cache/cook, publication, runtime-generation, graph-dispatch/draw/trace, graphics-state/attachment/mesh/pipeline, editor, build-membership, generated-artifact, diagnostic, and documentation searches. Freeze the owner map and assign every old field/type/file/consumer to one later phase. Reconcile stale documentation and baseline provenance. Do not edit runtime/tool source or run executable checks.

#### Phase-specific references

- [Documentation authority](../../README.md)
- [Integration Style Guide clean-break policy](../../Engineering/Standards/IntegrationStyleGuide.md#current-clean-break-policy)
- [Coding Style one-field types](../../Engineering/Standards/CodingStyle.md#one-field-types)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Ray-tracing target architecture](RayTracingPipelineImplementationPlan.md)
- [Epic RDG shader/pass parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- [Epic graphics pipeline-state initializer](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FGraphicsPipelineStateInitialize-)

#### Required work

- Inventory every shader class, nested `FParameters`, duplicate `*PassParameters` field, pass class, `RenderPassDefinition`, `GetDefinition`, `GetParameterMetadata`, Execute body, graph dispatch/draw consumer, and focused collaborator dependency.
- Inventory every shader resource/attachment parameter kind and every `FrameGraphBuilder::Read`, `CreateSRV`, `CreateUAV`, `CreateRenderTarget`, and `CreateDepthTarget` definition and call site. Classify each as a shader view, acceleration-structure binding, raster attachment, copy/resolve operation, or deletion; do not preserve two author-facing spellings for the same view.
- Classify each pass as direct one-shader compute, multi-stage graphics, shaderless graph work, or real feature collaborator. Assign the two rejected shadow variants and their wrappers to Phase 1 deletion, the remaining forwarding wrappers to Phase 2 deletion, and justify every retained class with behavior it owns.
- Inventory every field and consumer of the graphics pipeline state, raster runtime variants, RHI pipeline description, attachment signature/actions, vertex-input/topology, mesh/material policy, dynamic command state, and backend defaults. Assign the caller aggregate/eager variants/duplicate target and topology paths to Phase 5 deletion and freeze the granular target owner map.
- Inventory package identity/generation/cache/readers/writers. Assign the two rejected shadow package identities to Phase 1 deletion and the remaining package system to Phase 4 deletion.
- Inventory every inline-ray-query effect, RT shader/stage declaration, compiler capability, cooked RT export/hit-group/local-record field, deliberate runtime rejection, RHI capability field, AS/TLAS contribution, native pipeline/SBT/trace absence, frame-graph/runtime-cache seam, requested/active execution setting, explicit supported alternate, mandatory-product failure, and existing test/evidence consumer. Assign compiler-only RT package scaffolding to Phase 4 deletion and the complete target RT slice to Phases 6-10.
- Inventory the direct-shadow descriptor/device-address/no-query split end to end: shader classes and HLSL roots, parameters and uniforms, feature flags, graph handles and selection, capability-report fields, provider selection, Vulkan classic/partitioned descriptor layout and writes, and mutable-descriptor bootstrap/layout scaffolding. Assign the clean break to Phase 1; preserve GPU addresses only in backend AS construction and exact native descriptor writes.
- Freeze `RaytracedGBuffer` as the first parity effect; define the effect-level portability boundary, shared scene/TLAS/material/output authority, payload/attribute/miss/ray-flag contract, requested-versus-active mode semantics, explicit raster alternative, and the instance/geometry/ray-type SBT formula. Do not treat an arbitrary compute entry point as interchangeable with an RT stage.
- Record the exact current counts for registrations, handwritten labels/package constants, duplicate parameter fields, wrapper files, HLSL files under `Passes/Deferred`, generated `.sparkshader` artifacts, graphics-state/runtime/key/attachment fields, topology setters, and typed graphics graph calls.
- Record exact baseline provenance or mark final runtime/performance claims blocked. Record unrelated dirty exclusions.

#### Positive guardrails

- Use `rg`/`rg --files` and bounded owner/consumer reads.
- Keep inventory in this document or CL description, not a runtime reporting system.
- Every rejected definition/path has exactly one deletion phase.

#### Negative guardrails

- No runtime edits, target scaffolding, renames, adapters, branches, builds, cooks, or tests.
- No permutation or precache design hidden in the inventory.

#### Acceptance criteria

- Every current shader/pass/package field and material consumer has one target owner or deletion.
- Every graphics-state field and consumer has one target authority or Phase 5 deletion; attachment, mesh/material, pass-state, dynamic-command, complete-key/descriptor, and backend responsibilities do not overlap.
- Every forwarding pass and duplicate parameter schema has one disposition: the device-address/no-query shadow roots are Phase 1 deletions and the remaining forwarding surfaces are Phase 2 deletions.
- Every package reader/writer/cache/identity/generation spelling has one disposition: the two rejected shadow identities are Phase 1 deletions and the remaining package system is Phase 4 deletion/replacement.
- Every current RT schema/capability/rejection/effect/scene/graph/runtime/evidence item has one target owner or deletion phase, and no RT task remains owned by the target-state document.
- Missing revision-pinned inline D3D12/Vulkan parity, valid-library rejection, native-feature absence, capture, and performance baselines are explicitly blocked for Phase 12 rather than implied.
- The frozen eradication floor includes exact spellings and semantic equivalents for aliases, adapters, conversion helpers, fallbacks, copied schemas, parallel registries/generations, generated formats, directories, and build/tool/frontend/document consumers; every match has exactly one later deletion phase and no item is assigned to generic cleanup.
- The common phase evidence table and documentation-only Code Review gate report `PASS`; any unowned value, unresolved standards conflict, missing exclusion, or P0-P2 finding makes Phase 0 `BLOCKED`.
- Local links, scoped documentation diff, and `git diff --check` pass; no executable claim is made.

#### CL boundary

Suggested title: `Shaders: freeze lean shader and pipeline migration contract`.

### Phase 1 - Establish virtual sources, semantic navigation, and one AS binding

#### Implementation prompt

> Implement Phase 1 as one source-identity, physical-layout, and acceleration-structure binding cleanup CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including production-route tracing, semantic legacy eradication, and the mandatory Code Review gate. Introduce one canonical virtual source namespace, convert every registration/include/dependency/cache diagnostic input to virtual identity, move the broad `Passes/Deferred` shader bucket into semantic owners matching Renderer navigation, and delete old physical search/fallback paths and the old directory. Clean-break the duplicate direct-shadow roots so one semantic shader parameter serves classic and partitioned TLAS through backend-owned descriptor lowering; delete the unconsumed no-query shader and keep real shadow production mandatory. Do not add compatibility mounts, shader variants, or executable checks.

#### Phase-specific references

- [Repository Structure and Ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Data-Oriented Design identity rules](../../Engineering/Standards/DataOrientedDesign.md#identity-and-references)
- [Epic shader source-path precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)
- [Microsoft DXR acceleration-structure resource binding](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [NVIDIA NVRHI acceleration-structure binding model at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Khronos partitioned-AS descriptor type](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html)

#### Required work

- Add immutable `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` ownership, canonicalization, collision, traversal, case, and late-registration rules.
- Convert registered source paths and root includes to virtual paths; persist virtual dependency identities and use physical paths only for bounded reads.
- Remove project-first shadowing, absolute authored includes, basename identity/fallback, and checkout paths from portable hashes/diagnostics.
- Delete `DirectShadowSignalDeviceAddressCS`, `DirectShadowSignalNoRayQueryCS`, their registrations, package constants, parameter schemas, forwarding passes, root HLSL files, authored defines, diagnostics, and build membership. Do not move these rejected roots into the new namespace.
- Keep one `DirectShadowSignalCS`, one root HLSL entry, and one semantic `SceneTlas` acceleration-structure parameter. Bind it through `CreateAccelerationStructureBinding(sceneTlas)`, not generic `Read` or `CreateSRV`. Remove shader/effect uniform GPU-address words, raw-address conversion helpers, `RayTracingSceneTlasShaderAccessMode`, `DeviceAddressRayQuery`, `UsesAccelerationStructureDeviceAddress`, Renderer capability-report `SupportsShaderDeviceAddress`, `SupportsShaderDeviceAddressAccess`, and equivalent frontend access-mode policy. Preserve GPU addresses only inside RHI/AS build and native descriptor-writing mechanisms that genuinely require them.
- Make classic and partitioned TLAS publish the same semantic graph AS binding. Private RHI resolves the selected provider to its exact native descriptor representation; Vulkan uses `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` or `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV` and the matching write structure. Provider selection is fixed before layout/pipeline materialization. Delete the current otherwise-unconsumed `VK_EXT_mutable_descriptor_type` feature/bootstrap/layout scaffold; do not require an authored define, alternate bytecode record, mutable descriptor, or effect uniform address.
- Keep PTLAS unavailable unless its complete descriptor capability, layout, write, resource resolution, and ray-query chain is valid. Phase 1 removes the dead address variant without claiming PTLAS runtime proof; Phase 6 owns paired executable backend validation and may enable the provider only when that evidence passes.
- Delete the unconsumed no-query shader without replacement. Shadow visibility remains a mandatory product of real traversal: retain the inline-query producer, reject unavailable capability before graph construction, and delete `AddShadowVisibilityFallbackPass`, `ShadowVisibilityFallback`, `CVarRayTracedShadowsEnabled`, `r.RayTracedShadows.Enabled`, `EnableInlineRayQueryShadows`, and any semantic-equivalent clear/copy/no-op pass, default resource, enable flag, or mode boolean that would publish fabricated visibility. Do not dispatch a shader whose only distinction is compiling traversal out. Phase 8 owns the first valid alternate producer by adding the complete pipeline/RGS path and selecting exactly one real frontend.
- Move the remaining shader sources from `Engine/Assets/Shaders/Passes/Deferred` to `Passes/GBuffer`, `Passes/Lighting/...`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners as applicable.
- Convert the existing shared inline-ray-query sources and every GBuffer/shadow/path/ReSTIR include consumer to the same virtual namespace without duplicating them under an RT-pipeline tree. Reserve semantic sibling filenames for later inline/pipeline frontends, but do not pre-create those files.
- Update every C++ registration, HLSL include, ShaderCompiler resolver/hash/dependency consumer, CMake/source group, documentation link, and generated metadata spelling.

#### Positive guardrails

- One virtual path names one source regardless of machine.
- Relative includes remain relative to the including virtual source.
- Technique names remain only where a shader specifically implements that technique.
- One shader/effect parameter describes scene-AS access; backend/provider differences stop at private RHI binding.

#### Negative guardrails

- No old/new search order, alias mount, absolute fallback, duplicate source tree, raw directory registry, or renderer-wide `Deferred` owner.
- No `*DeviceAddressShader`, `*DescriptorShader`, no-query shader, authored AS-access define, raw TLAS address in an effect uniform, access-mode branch at graph setup, or hidden backend pseudo-permutation.
- No clear/copy/no-op shadow producer, feature-disable branch, nullable shadow product, or graph-call-site mode boolean that permits direct lighting to consume fabricated visibility.

#### Acceptance criteria

- Exact searches find zero authored old physical registration paths, zero `Passes/Deferred/` paths/files, zero basename fallback, and zero portable hashes containing checkout roots.
- Exact runtime/build searches find zero `DirectShadowSignalDeviceAddress*`, `DirectShadowSignalNoRayQuery*`, `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, `SPARKLE_RAY_TRACED_SHADOWS_DISABLED`, `DeviceAddressRayQuery`, `UsesAccelerationStructureDeviceAddress`, `RayTracingSceneTlasShaderAccessMode`, `SupportsShaderDeviceAddress`, `SupportsShaderDeviceAddressAccess`, `SupportsMutableDescriptorType`, `EnabledMutableDescriptorType`, `VK_EXT_mutable_descriptor_type`, or shader/effect `SceneTlasGpuAddress*` definitions/uses.
- `DirectShadowSignalCS::Parameters::SceneTlas` is the sole shadow traversal AS parameter; classic/partitioned resources reach one graph binding and native representation selection is private to RHI. No second shader class, source, code record, or frontend mode exists.
- Exact graph-setup searches find zero acceleration-structure assignments through `builder.Read`; all use the one typed `CreateAccelerationStructureBinding` route.
- Shadow visibility has one real inline-query producer. Missing inline-query capability rejects graph construction before scheduling; ReSTIR schedules exactly one `DirectShadowSignalCS`, and exact runtime/build searches return zero `AddShadowVisibilityFallbackPass`, `ShadowVisibilityFallback`, `CVarRayTracedShadowsEnabled`, `r.RayTracedShadows.Enabled`, or `EnableInlineRayQueryShadows` uses.
- Same-basename files in distinct virtual directories remain distinct; project/engine ownership cannot silently shadow.
- Static bidirectional traces prove each retained registration resolves through the canonical mount/dependency route and each classic/partitioned scene AS reaches the same graph semantic before private backend lowering; old physical paths, alternate shadow roots, and access-mode policy cannot be selected by any production caller.
- The common phase evidence table and source-only Code Review gate report `PASS` with no P0-P2 finding; no compile, backend, or runtime success is claimed.
- Includes/CMake/source groups/docs reconcile and `git diff --check` passes without compilation claims.

#### CL boundary

Suggested title: `Shaders: unify source identity and acceleration-structure binding`.

### Phase 2 - Make the shader class the complete lean frontend

#### Implementation prompt

> Implement Phase 2 as one shader-authoring, parameter-authority, graph-dispatch, and pass-wrapper deletion CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including definition-to-use closure, duplicate-authority review, and the mandatory Code Review gate. Rename the generic shader frontend to Sparkle vocabulary, make nested `Parameters` the single shader/graph ABI, add direct typed compute and graphics graph entry points, update every shader/pass consumer, and delete duplicate pass schemas, package/debug strings at graph call sites, `RenderPassDefinition` bags, and one-method forwarding pass classes. Retain only collaborators with real feature/draw behavior. Do not introduce programs, permutations, precaching, compatibility overloads, or executable checks.

#### Phase-specific references

- [Coding Style](../../Engineering/Standards/CodingStyle.md)
- [Naming and Vocabulary](../../Engineering/Standards/NamingAndVocabulary.md)
- [Data-Oriented Design single truth](../../Engineering/Standards/DataOrientedDesign.md#single-truth-and-copy-budget)
- [Ray-tracing target shader and composition contract](RayTracingPipelineImplementationPlan.md#shader-authoring-and-pipeline-composition)
- [Epic RDG shader parameters and utility passes](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic shader-parameter metadata member](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderParametersMetadata/FMember)
- [Epic `FRDGBuilder::CreateSRV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateSRV)
- [Epic `FRDGBuilder::CreateUAV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateUAV)
- [Epic `FRenderTargetBinding`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRenderTargetBinding)
- [NVIDIA NVRHI binding model at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Epic `FComputeShaderUtils::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FComputeShaderUtils__AddPass)

#### Required work

- Clean-break rename `TGlobalShader` to `GlobalShader`, `TShaderRef` to `ShaderRef`, and nested `FParameters` to `Parameters` across macros, traits, registrations, tools, and consumers. Delete or rename unused `FGlobalShader`/`FComputeShader`/`FRay*Shader` prefix types; no aliases remain.
- Keep shader-visible inputs/outputs inside their direct-dispatch shader class. Move class declarations beside their semantic feature owner where graph setup must name the type.
- Give every shader parameter one exact member/binding name across its C++ field, graph/layout metadata, HLSL declaration, reflection, cooked record, and runtime binding. Delete all `_NAMED` macros, dual `LayoutName`/`ShaderName` descriptor fields, shader-name setters/getters, and reflection fallback matching; rename the authored HLSL binding and C++ declaration together where they differ.
- Make `AllocParameters<Shader>()` allocate `Shader::Parameters`. Make `Dispatch<Shader>()` and async dispatch accept that same instance and group count, resolve the current shader runtime internally, and derive the default diagnostic label from the shader type.
- Clean-break shader resource declarations and graph assignments to the explicit table above: texture/buffer SRV fields use `CreateSRV`, texture/buffer UAV fields use `CreateUAV`, acceleration structures use the Phase 1 semantic binding, and raster/depth attachments use `CreateRenderTarget` / `CreateDepthTarget` only in narrow graphics envelopes. Delete the duplicate texture/buffer `Read` aliases and generic field macros that conceal view kind.
- Provide an explicit label overload for repeated graph instances. It changes diagnostics only.
- Make graphics graph setup name concrete vertex/pixel shader types plus real draw collaborator/data. Keep the current graphics-state checkpoint source-consistent but do not endorse its complete caller-authored aggregate; Phase 5 atomically replaces that surface after the final map/library exists. Do not require a program alias or add another state representation here.
- Delete duplicate `*PassParameters` fields/schemas and count-only layout acceptance. Compose a small envelope only for real multi-stage or graph-only fields.
- Delete `RenderPassDefinition`, `RenderPassDefinitionRuntime`, `ComputePassOperations`/equivalent forwarding paths, `GetDefinition`, `GetParameterMetadata`, repeated `PassName`/layout/pipeline strings, and pass classes whose body only constructs/binds/dispatches one shader.
- Retain GBuffer/mesh or feature collaborators only where they own real draw-list/cache/feature behavior; strip shader lookup/binding boilerplate from them.
- Keep the current package-backed runtime representation internally until its atomic Phase 4 replacement, but remove package details from graph/feature call sites and do not create a second representation.
- Keep generic stage traits capable of representing RT stages, but add no renderer RT declaration, hit-group registry, `RayTracingPipelineComposition`, or compiler-only map entry here. Phase 6 owns those definitions together with their first complete runtime consumer.

#### Positive guardrails

- The common author writes one shader class, nested parameters, one implementation declaration, and one graph dispatch.
- A parameter declaration contains one member/binding identifier; generated metadata propagates it without an override or alias.
- Graph resource usages and shader binding derive from the same metadata instance/signature.
- `FrameGraphBuilder` automates map/runtime/layout/pipeline mechanics; `PassCommandContext` remains semantic-free.
- Shader-view creation and raster-attachment binding remain visibly distinct while both feed the same parameter metadata and graph dependency authority.

#### Negative guardrails

- No `DirectLightingProgram`, `SPARKLE_RENDER_PASS`, pass traits duplicating shader metadata, generic program abstraction, separate shader/pass schemas, owner pointer, service bag, runtime reflection-name discovery, or copied bindings.
- No `_NAMED` parameter macro, layout-name/shader-name pair, metadata alias, or try-both-names reflection/binding fallback.
- No deletion of a class that owns real mesh iteration, draw cache access, feature policy, or several meaningful operations; narrow it instead.
- No permutation/precache callbacks or readiness frontend.
- No generic program alias disguised as RT preparation and no unused RT composition or pass surface.
- No author-facing `Read(texture)` / `Read(buffer)`, `CreateRTV`, `CreateDSV`, generic resource-access guess, or compatibility alias for the replaced view vocabulary.

#### Acceptance criteria

- A representative direct compute shader reads as class+nested `Parameters`+implementation declaration+`Dispatch<Shader>` with no authored package/program/pass/layout/pipeline string.
- Exact searches return zero `TGlobalShader`, `TShaderRef`, nested `FParameters`, `RenderPassDefinition`, count-only parameter acceptance, and phase-owned forwarding pass definitions/uses.
- Every shader-visible field exists once; graph setup and reflection/binding consume that authority.
- Exact runtime/tool/build searches return zero `SHADER_PARAMETER_*_NAMED`, `SPARKLE_REGISTER_NAMED_GRAPH_SHADER_PARAMETER`, parameter-field `LayoutName`/`ShaderName`, `GetLayoutName`, parameter `GetShaderName`, and `SetShaderName`; representative C++ members and HLSL declarations have identical names.
- Exact runtime/build searches return zero texture/buffer shader assignments through `builder.Read`, zero duplicate `FrameGraphBuilder`/`FrameGraph` texture/buffer `Read` aliases, and zero neutral `CreateRTV` / `CreateDSV`; representative SRV, UAV, render-target, depth-target, and AS bindings use their one canonical route.
- Graphics names concrete stage shader types without a `TShaderProgram`; the current state carrier remains single until its Phase 5 clean break and is not accepted as the target frontend.
- Definition-to-use and use-to-owner traces cover every authored shader: nested `Parameters` drives metadata, graph declaration, and binding; every retained pass/collaborator owns behavior beyond forwarding, and semantic searches find no renamed parameter mirror, generic program/pass bag, or copied binding schema.
- The common phase evidence table and source-only Code Review gate report `PASS` with no P0-P2 finding; no compile or recording result is claimed.
- `PassCommandContext` and recording remain infrastructure-only; includes/CMake/docs reconcile and `git diff --check` passes.

#### Phase 2 source-consistency evidence

This table records the current unstaged `master` checkpoint only. It does not claim compilation, shader recording, backend execution, cook, runtime, capture, or performance evidence.

| AC / claim | Cheapest claim-falsifying check or inspection route | Result |
| --- | --- | --- |
| Direct compute authoring is class + nested `Parameters` + implementation + typed dispatch, with no authored package/program/pass metadata at the graph call | Trace `DirectLightingCS` from `DirectLighting.h` through `DirectLightingShaders.cpp` to `AddDirectLightingPass`; exact searches for `DirectLightingProgram`, `TShaderProgram`, and `SPARKLE_RENDER_PASS` | `PASS`: the graph caller allocates and dispatches `DirectLightingCS` directly; rejected program/pass spellings have zero runtime/build hits. The package macro remains confined to the registration implementation until Phase 4. |
| Every authored shader closes definition -> parameters -> registration -> real graph consumer | Extract `class X final : public GlobalShader<X>`, `IMPLEMENT_GLOBAL_SHADER*`, `Dispatch*<X>`, and `Draw<VS, PS>` sets from Renderer headers/sources and compare both directions | `PASS`: 27 shader classes, 27 nested `Parameters` schemas, 27 registrations, and 27 consumed shader types close exactly; 25 are compute dispatch shaders and two are the GBuffer vertex/pixel pair. |
| Replaced frontend, definition bags, count-only acceptance, duplicate schemas, and forwarding pass owners are absent | Exact runtime/build `rg` floor for `TGlobalShader`, `TShaderRef`, nested `FParameters`, unused `FGlobalShader`/`FComputeShader`/`FRay*Shader`, `RenderPassDefinition*`, `RenderPassShaderRuntimeDesc`, `ComputePassOperations`, `RasterPassOperations`, `GetDefinition`, `GetParameterMetadata`, and `*PassParameters`; inspect `PassParameterLayout::Matches` and `PassBinder` | `PASS`: zero rejected-name definitions or uses; layout compatibility compares the complete structural signature rather than parameter count; phase-owned one-shader pass files are deleted. |
| Shader-visible fields have one schema and the graph and binder consume it | Trace macro registration -> `ShaderParameterStructBuilder` metadata -> `TypedPassParameterInstance` -> `SetupShaderParameters` -> `PassBinder`; inspect GBuffer composition and per-draw overrides | `PASS`: compute shaders use their nested schema directly. GBuffer composes `GBufferVS::Parameters` and `GBufferPS::Parameters`; its seven attachment fields are graph-only, while the two mesh-cache SRVs remain declared once in `GBufferVS::Parameters` and are supplied by the retained mesh collaborator. |
| Every parameter has one C++/metadata/HLSL binding identity | Exact runtime/tool searches for `SHADER_PARAMETER_*_NAMED`, `SPARKLE_REGISTER_NAMED_GRAPH_SHADER_PARAMETER`, parameter-field `LayoutName`/`ShaderName`, `GetLayoutName`, parameter `GetShaderName`, `SetShaderName`, and dual-name reflection fallback; compare representative cbuffer, SRV, and UAV declarations with HLSL | `PASS`: 103 authored named overrides and their three macro definitions are deleted. Parameter descriptors and pass layouts carry only `Name`; graph registration, package layout, cook verification, reflection validation, cooked validation, and runtime binding consume it directly. |
| Resource and attachment vocabulary has one canonical author route | Exact searches for `builder.Read`, typed `FrameGraphBuilder`/`FrameGraph` texture/buffer `Read` overloads, `CreateRTV`, and `CreateDSV`; count canonical calls beneath `Renderer/Private/Passes` | `PASS`: rejected author-facing spellings are zero. Current pass sources contain 178 `CreateSRV`, 49 `CreateUAV`, seven `CreateAccelerationStructureBinding`, six `CreateRenderTarget`, and two `CreateDepthTarget` calls. Generic `PassResourceBuilder::Read(handle, usage, label)` remains private graph-compiler declaration infrastructure, not an author-facing view alias. |
| Graphics setup names concrete stages and retains only real draw behavior; complete pipeline-state ownership remains a known checkpoint | Trace `Draw<GBufferVS, GBufferPS>` through `FrameGraphBuilder`, `RenderPassRuntimeCache`, `GraphicsShaderPipelineState`, and `GBufferMeshPass` / `GBufferMeshBatchDrawer` | `PARTIAL`: typed stage naming and real mesh collaborators are established, but the caller-authored aggregate duplicates attachment/mesh facts, the cache key is incomplete, targets/topology have duplicate owners, and variants are created eagerly. Phase 5 owns their atomic replacement; this row must not be used to claim the graphics-state target is complete. |
| Recording context remains semantic-free and package/runtime mechanics stay behind graph/runtime owners | Inspect `PassCommandContext`, `FrameGraphBuilder`, and `RenderPassRuntimeCache`; search pass sources for package/runtime lookup | `PASS`: the command context contains only commands, diagnostics, and declared-resource resolution. Typed graph helpers own current package-backed materialization; feature callers do not select packages, layouts, or pipelines. |
| Source/build/docs hygiene and mandatory source-only review | Recursive Renderer CMake source-glob inspection; pinned `clang-format 22.1.3 --dry-run --Werror --style=file` over changed C/C++; local Markdown link resolution; `cmake -DSPARKLE_REPO_ROOT=... -P CMake/ArchitectureBoundaryCheck.cmake`; `git diff --check`; Code Review procedure | `PASS`: recursive build membership covers the moved/deleted files, pinned no-write formatting, local links, the architecture boundary check, and whitespace validation pass. Final source-only review reports no P0-P2 finding. No executable check is claimed. |

#### CL boundary

Suggested title: `Renderer: make shader classes drive typed graph dispatch`.

### Phase 3 - Establish reproducible compile jobs and changed dependencies

#### Implementation prompt

> Implement Phase 3 as one compiler-job, dependency-selection, and replay-diagnostics CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including producer/consumer closure, semantic legacy eradication, and the mandatory Code Review gate. Replace package-shaped cook nodes with immutable one-variant shader compile requests/jobs/results keyed by compiler-affecting input, persist virtual dependencies, make Changed select the exact reverse closure, and delete old node identities and full-catalog Changed behavior. Preserve one cooker, `SparkleTasks`, and the compile-every-selected-input rule. Do not add permutations, persistent compiler-result storage, workers, or executable checks.

#### Phase-specific references

- [Concurrency](../../Engineering/Standards/Concurrency.md)
- [Editor and Tools](../../Engineering/Standards/EditorAndTools.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Epic shader compile job](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)

#### Required work

- Introduce `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, and `ShaderCompileResult` for `(ShaderTypeId, Target)`.
- Hash virtual source identity/content closure, entry, stage, compiler-affecting environment/ABI, target, backend/tool provenance, and policy. Exclude package, program, pass label, filename basename, and presentation text.
- Keep requests stage-agnostic enough for Phase 6 RT exports, no-entry-point libraries, and target capability checks, but do not translate the old package RT metadata or publish compiler-only RT jobs in this phase.
- Replace `CookNode`, node builders/executors, and package-led compile options in one path while retaining the already established `ShaderCompileInputHash` identity.
- Compile every selected input. Deduplicate only identical in-flight jobs inside the active cook and fan out their one result; persist no compiler output between operations.
- Persist forward/reverse dependencies. Change source tracking from a boolean to changed virtual paths and select only affected shader types.
- Use one `TaskExecutor` graph with deterministic ordering, duplicate fan-out, bounded compiler sessions/memory, cancellation settlement, and no worker waits.
- Emit one bounded failure replay bundle; successful analysis artifacts remain opt-in.

#### Positive guardrails

- Identical compiler-affecting requests compile once regardless of graph consumers.
- Cancellation/failure publishes nothing partial.
- Application supplies intent/changed paths; compiler owners select and schedule work.

#### Negative guardrails

- No shader thread pool, `std::async`, persistent worker, compiler-result store, cache configuration/service/browser, retry loop, per-job log stream, permutation dimension, or valid-dependency full-catalog fallback.

#### Acceptance criteria

- Exact searches return zero old cook-node/cache-key definitions and consumers.
- Checkout moves preserve the input hash; any compiler-affecting change invalidates it.
- Source-level routes and existing validation consumers are updated for duplicate fan-out, repeated-operation recompilation, cancellation settlement, and exact changed-dependency selection; Phase 4 owns their first executable proof.
- Static production traces connect changed virtual paths through reverse-dependency selection, immutable requests/jobs, `SparkleTasks`, result fan-out, and transactional publication; no old cook node, full-catalog fallback, cache-like store, alternate worker path, or presentation-owned compile policy remains reachable.
- The common phase evidence table and source-only Code Review gate report `PASS` with no P0-P2 finding; the behavioral claims above remain Phase 4 executable obligations rather than being inferred from source shape.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shaders: establish compile jobs and dependency-directed cooking`.

### Phase 4 - Replace cooked packages with the global shader map

#### Implementation prompt

> Implement Phase 4 as one cooked-map, code-library, typed-lookup, and generation-lifetime CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including corruption-sensitive authority/lifetime checks and the mandatory Code Review gate. Replace per-package cooked authority with `GlobalShaderMap` and content-addressed `CookedShaderLibrary` records, make `ShaderRef<Shader>` resolve through the active map, preserve `RenderPassRuntimeCache` as the sole generation/materialization owner, and delete every old package identity/file/reader/writer/cache/path/schema dispatch and package-generation spelling. Do not keep an adapter. Run the focused ShaderCompiler/map/library cook-load checks required to prove the new physical authority; do not run broad product/capture/performance validation.

#### Phase-specific references

- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Epic `FGlobalShaderMap`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FGlobalShaderMap)
- [Epic `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)

#### Required work

- Generate deterministic `GlobalShaderMap` entries for `(ShaderTypeId, Target)` containing `ShaderCodeHash`, parameter signature, stage/feature/runtime metadata, and provenance joins.
- Generate `CookedShaderLibrary` records keyed by exact `ShaderCodeHash`; deduplicate validated identical code bytes without conflating incompatible metadata.
- Open/validate map and library once per replacement generation. `ShaderRef<Shader>` resolves the active target entry; graph/runtime never computes a file path or package ID.
- Refactor `RenderPassRuntimeCache::ShaderRuntimeGeneration` to own the opened map/library and generation-bound materialized layouts/pipelines. Keep lazy graph-construction materialization and all-queue retirement.
- Delete `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `BuildShaderPackageIdFromSourcePath`, `CookedShaderPackage*`, `LoadedShaderPackage`, package cache/identity/path/schema-version dispatch, `.sparkshader` readers/writers, package generations, and obsolete generated outputs.
- Delete the old package-shaped RT export/hit-group/local-parameter/payload/attribute/recursion records, package inspection, and deliberate package-runtime rejection rather than copying them into an unconsumed map schema. Phase 6 owns the final RT records and their complete execution consumer.
- Update Application/Editor/CLI package targets, publication readers/payloads, model fields, help/autocomplete, diagnostics, and artifact-path consumers to typed shader/map/library vocabulary in the same CL. Phase 10 may simplify their workflow but cannot finish package removal.
- Keep neutral code/map validation records in RHI public, generation policy in Renderer, generation output in ShaderCompiler, and backend object creation in backend-private RHI.
- Preserve complete replacement validation, rollback on failure, renderer shader-generation capture in `RenderFrameIdentity`, view-history invalidation, and submission-token retirement.
- Execute the deferred Phase 1-2 source/compiler checks that this owner can now prove: canonical virtual resolution, same-basename distinction, collision/traversal/case rejection, checkout-independent dependency identity, and representative compute/graphics nested-parameter metadata matching the cooked signature and typed lookup. Phase 6 still owns native AS-lowering and ray-tracing graph-execution proof.
- Execute the deferred Phase 3 compiler-job checks through the focused cooker route: identical in-operation fan-out, repeated-operation recompilation, exact changed-dependency closure, cancellation settlement, failed-job nonpublication, and portable input hashes. Fix the owning Phase 3 path rather than weakening the oracle or adding a fallback.

#### Positive guardrails

- Catalog says what exists; map says what the active target resolves; library owns validated bytes; runtime cache owns derived live objects. No owner duplicates another.
- Physical file grouping is generated policy and remains simple for the current catalog.
- Lazy materialization stays before recording and is not called precaching.

#### Negative guardrails

- No package-to-map converter at runtime, dual emission, old reader, upgrade path, alias ID, directory scan, live map patch, second generation counter, program manifest, streaming/preload framework, or driver cache.
- No compiler-only RT map entries, translated package RT schema, disabled RT composition registry, or promise that stage enumeration equals runtime support.

#### Acceptance criteria

- Exact runtime/tool/build/Application/Editor/document searches return zero package types/names/paths/readers/writers/request fields/model fields/help, `.sparkshader` I/O, and old schema dispatch.
- Every catalog shader resolves through one map entry and every referenced code hash exists exactly once in the library index.
- Focused source/compiler evidence proves canonical virtual resolution and rejection rules, relocation-stable dependency identity, and representative compute/graphics nested parameter metadata matching the final map/library signature; no physical-path fallback or duplicate parameter authority satisfies the check.
- Focused compiler/cooker evidence proves the deferred Phase 3 fan-out, recompilation, dependency, cancellation, failure-publication, and checkout-independent hash claims; each result is tied to the new request/job/map production route rather than an isolated helper.
- Invalid replacement preserves the current generation; retired map/library/layout/pipeline state remains until all recorded submissions complete.
- RHI remains Renderer-independent; map/library/runtime owners have no duplicated identity or lifetime state.
- Focused corruption cases for a missing/incorrect code hash, duplicate logical key, incompatible parameter signature, truncated code record, stale generation, and invalid replacement fail at the owning validator and leave the previously accepted generation active; each check would fail if lookup or validation bypassed the new map/library route.
- Bidirectional production traces and semantic searches prove map/library lookup is the only runtime route and package behavior has not survived behind renamed records, compatibility readers, generated extensions, editor fields, or publication payloads.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; executable evidence is limited to the focused compiler/map/library cook-load and lifetime claims authorized here.
- Includes/CMake/generated policy/docs reconcile and `git diff --check` passes.
- Exact searches prove the old RT package metadata/rejection path is gone; RT runtime support remains honestly absent until Phase 6 delivers the whole replacement.

#### CL boundary

Suggested title: `Shaders: replace cooked packages with the global shader map`.

### Phase 5 - Split raster intent, attachment compatibility, and graphics pipeline materialization

#### Implementation prompt

> Implement Phase 5 as one graphics-state ownership and paired-backend vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including bidirectional state-authority traces, corruption-sensitive key checks, paired D3D12/Vulkan evidence, and the mandatory Code Review gate. Replace the caller-authored `GraphicsShaderPipelineState` and eager variant bundle with narrow granular `RasterPassRenderState`, graph-derived attachment compatibility, mesh/material-owned geometry/raster facts, and one complete internal immutable `GraphicsPipelineKey`/`GraphicsPipelineDesc` materialization path. Update the GBuffer draw route end to end and delete every old state field, variant pointer, duplicate target/topology operation, and compatibility overload in this CL. Do not add PSO precaching/prewarming, a renamed aggregate, Unreal-scale mesh-command caching, or a second pipeline system; do not stage, commit, push, or submit.

#### Phase-specific references

- [Graphics Pipeline State Ownership](../../Engineering/Standards/GraphicsEngineering.md#graphics-pipeline-state-ownership)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Data-Oriented Design single truth](../../Engineering/Standards/DataOrientedDesign.md#single-truth-and-copy-budget)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Epic Graphics Programming Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- [Epic Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- [Epic `FMeshPassProcessorRenderState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessorRenderState)
- [Epic `FGraphicsMinimalPipelineStateInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateIni-)
- [Epic `ExtractRenderTargetsInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/ExtractRenderTargetsInfo)
- [Epic `FGraphicsPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FGraphicsPipelineStateInitialize-)
- [Epic `SetGraphicsPipelineState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/SetGraphicsPipelineState/2)
- [NVIDIA NVRHI graphics pipeline/framebuffer/state split at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)

#### Required work

- Inventory every field and consumer of `GraphicsShaderPipelineState`, `RasterPassPipelineRuntime`, `GraphicsPipelineDesc`, `RhiVertexLayoutKind`, `RhiDepthTestDesc`, `RhiStencilTestDesc`, `WireframePipeline`, `TwoSidedPipeline`, render-target format/count/depth fields, attachment load/store/clear/access, blend/raster/sample backend defaults, and every topology setter. Assign each fact to shader identity, pass render state, graph attachment signature, prepared mesh/material draw, dynamic command state, internal pipeline key/descriptor, or deletion before editing.
- Delete `GraphicsShaderPipelineState` rather than renaming it. Add `RasterPassRenderState` with granular operations for only the pass-wide semantic blend/depth-stencil and dynamic stencil-reference choices that current passes consume. Attachment access remains graph-owned. The type must not contain shader stages, vertex layout, topology, target formats/count, depth format, samples, attachment access, render-target handles, mesh/cache pointers, view policy, or backend objects.
- Make the GBuffer graph attachment envelope the sole owner of render/depth target handles and load/store/clear/access actions. Derive one immutable attachment compatibility signature from graph texture descriptions and bindings, including color formats/count, depth/stencil format, sample count, and only backend-required pipeline-compatibility facts. Load/store/clear/access do not enter the graphics pipeline key; validate their consistency with pass depth/stencil intent and execute them once through the graph. Delete manually repeated GBuffer format/count/depth fields and manual bind/clear target preparation.
- Make prepared mesh draw work the sole source of vertex-input identity, primitive topology, vertex/index streams, draw arguments, and material-dependent raster policy. Delete the one-value `RhiVertexLayoutKind` selector; the mesh resource publishes a stable vertex-input declaration/identity derived from its actual attributes, and the neutral/backend pipeline consumes that declaration without a hard-coded static-mesh switch. Keep triangle topology in one real mesh owner and delete duplicate topology configuration. Resolve two-sided and wireframe fill/cull from explicit material/pass policy rather than generic view-mode inspection during shader binding.
- Make typed shader references contribute the exact code generation and binding-layout identities. Assemble a complete immutable `GraphicsPipelineKey` from shader, pass-state, mesh/material, and attachment authorities. Key equality/hash and diagnostics cover every pipeline-affecting field; render-pass labels do not affect identity.
- Retain one complete backend-neutral `GraphicsPipelineDesc` only at the Renderer/RHI materialization boundary. Add explicit neutral blend, raster, depth/stencil, topology, vertex-input, attachment-format, and sample facts needed by both backends. D3D12 and Vulkan must consume each supported field consistently or reject unsupported values before recording; remove silent opaque/sample-one/triangle/default-raster substitutions when those facts belong to the descriptor.
- Change `FrameGraphBuilder::Draw` to accept typed shader parameters, narrow render state, and real prepared draw work, not a complete pipeline aggregate. Before parallel recording, collect the exact finite keys requested by the graph/draw work and lazily materialize missing pipelines in the active shader-map generation. Recording performs lookup/bind/draw only and cannot create a pipeline or discover new state.
- Delete eager base/wireframe/two-sided creation and `RasterPassPipelineRuntime` variant pointers. Materialize only variants actually requested by prepared draw work. This is exact lazy construction, not PSO precaching, prewarming, preload, readiness, driver-cache, or retained Unreal-style cached mesh commands.
- Update graph compiler/executor, GBuffer setup and mesh collaborators, runtime cache/generation retirement, RHI descriptors and both backends, diagnostics/capture identity, focused validation consumers, includes, filenames/CMake if touched, and current documentation together. Preserve one active/replacement/retired runtime generation and all-queue retirement.

#### Positive guardrails

- Authors set only state they semantically own; every other pipeline fact is derived from its authoritative product.
- The complete neutral descriptor remains visible and exhaustive at the backend boundary, while the feature frontend stays granular.
- Required references and value types express mandatory state; absence of a required attachment, mesh fact, or supported backend mapping fails before recording.
- One exact requested key produces one generation-bound pipeline object reusable by compatible draws.

#### Negative guardrails

- No `GraphicsPipelineState`, `PipelineStateDesc`, `RasterPipelineSettings`, or other renamed caller-authored state bag; no generic setters for arbitrary backend fields.
- No manual target format/count/depth/sample repetition beside graph attachments, and no fallback default that fabricates a missing authority.
- No shader-pair-only key, label-based key, pointer-address key, native descriptor in Renderer authoring code, second graphics cache/generation counter, or service locator.
- No eager creation of all cull/fill/two-sided/wireframe combinations, PSO cache file, precache callback, readiness UI, speculative variant scan, full Unreal material/vertex-factory/permutation framework, or retained mesh-command cache.
- No pipeline creation, graph-resource discovery, material policy selection, target clearing, or semantic view-mode selection inside command recording/binding helpers.

#### Acceptance criteria

- Exact runtime/build searches return zero `GraphicsShaderPipelineState`, `RasterPassPipelineRuntime`, `RhiVertexLayoutKind`, `WireframePipeline`, `TwoSidedPipeline`, caller-authored `RenderTargetFormats`/`RenderTargetCount`/`DepthStencilFormat`, compatibility draw overloads, and generic binding-time `RenderViewMode` policy uses.
- `RasterPassRenderState` contains only actually consumed pass-wide blend/depth-stencil and dynamic stencil-reference values, exposes granular semantic operations, and cannot express attachment access, mesh, shader, graph, or backend ownership. A field-by-field owner table proves no old member was merely renamed or copied.
- The GBuffer declares each target, format, sample count, load/store/clear action, and depth/stencil access through one graph authority. A deliberate attachment format/sample mutation changes the derived pipeline key; an incompatible access/action mutation fails graph/pass validation without bloating that key; altering a deleted parallel field is impossible.
- One prepared mesh draw owns vertex-input identity and topology. Exact producer/consumer searches return one topology-setting route, and two-sided/wireframe choices are requested only for materials/modes that need them.
- Key perturbation checks change equality/hash for every pipeline-affecting shader, binding-layout, blend, raster, depth/stencil, topology, vertex-input, target-format, depth-format, and sample field; labels and dynamic viewport/scissor/draw arguments do not change it. Missing key fields, collisions, stale generations, and unsupported backend mappings fail loudly.
- Instrumented focused evidence proves requesting only the base GBuffer materializes no wireframe/two-sided pipeline; requesting each real variant materializes exactly that key before recording and reuses it thereafter. No result is described as precaching.
- D3D12 and Vulkan focused graphics runs/captures show matching attachment compatibility, blend/color-write, depth/stencil, cull/fill, topology, vertex input, and sample state; validation remains clean and every native object resolves back to one neutral key and shader-map generation.
- Graph execution begins/ends the render pass and applies attachment actions exactly once; GBuffer code contains no manual bind/clear duplication, and command recording creates no pipeline.
- Bidirectional owner traces prove feature code cannot reach the complete descriptor, backends cannot invent Renderer semantic policy, and no old aggregate/variant path survives under an alias, overload, default, diagnostic helper, test consumer, or generated/build entry.
- Focused build/runtime evidence, the architecture boundary check, no-write formatting, includes/CMake/docs, `git diff --check`, and the common Code Review gate pass with no P0-P2 finding. No unrelated broad validation or precache/performance claim is made.

#### CL boundary

Suggested title: `Renderer: split raster intent from graphics pipeline materialization`.

### Phase 6 - Deliver the complete paired ray-tracing runtime foundation

#### Implementation prompt

> Implement Phase 6 as one intentionally atomic shader-map-to-GPU ray-tracing vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including complete source-to-GPU reachability, paired negative/corruption evidence, and the mandatory Code Review gate. Extend the lean shader frontend and final `GlobalShaderMap`/`CookedShaderLibrary` representation to all six RT stages, add the focused typed pipeline composition, complete DXIL/SPIR-V compilation and validation, clean-break the RHI capability contract, materialize native D3D12/Vulkan pipelines and shader tables, add typed frame-graph trace execution and runtime-generation lifetime, and prove the complete path end to end on both backends through existing validation surfaces or a temporary local harness removed before handoff. Delete the old runtime rejection and ambiguous capability only when the complete replacement works. Do not split this into compiler-only, backend-only, disabled-public-API, or graph-bypass checkpoints; do not stage, commit, push, or submit.

#### Phase-specific references

- [Ray-tracing target architecture](RayTracingPipelineImplementationPlan.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Microsoft DXR functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos Vulkan ray-tracing chapter](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Unreal Engine hardware ray tracing](https://dev.epicgames.com/documentation/unreal-engine/hardware-ray-tracing-in-unreal-engine)
- [Unreal Engine `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Unreal Engine `FRayTracingShaderBindingTableInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRayTracingShaderBindingTableIni-)
- [Unreal Engine `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)

#### Required work

- Add concrete ray-generation, miss, closest-hit, any-hit, intersection, and callable shader classes through the same `GlobalShader`, virtual-source, implementation-registration, catalog, compile-job, map, code-library, provenance, and typed `ShaderRef` path as raster/compute. Ray-generation owns nested global `Parameters`; other stages declare no root-parameter struct or empty parameter carrier. Optional local data uses its focused hit-group/stage record schema rather than a second root contract. No second RT registry or package identity exists.
- Add one focused `RayTracingPipelineComposition` that names typed shader refs, triangle/procedural hit groups, payload/attribute compatibility, minimum recursion policy, and optional bounded local POD. Derive the global binding layout from the selected ray-generation shader's `Parameters`; local record schemas have one group/stage owner and are absent when unused. The composition is not used for compute or ordinary graphics and does not duplicate shader declarations.
- Extend final map/library records and ShaderCompiler validation/inspection for exports, groups, layout hashes, payload/attribute limits, recursion, local records, target/backend/source identity, and deterministic ordering. Prove DXC DXIL and SPIR-V RT-library output before advertising either target; keep Slang RT capability false until its own equivalent conformance passes.
- Exercise all six stage kinds, one triangle hit group, one procedural hit group, miss, callable indexing, global bindings, bounded local-record validation, and exact sentinel outputs through an existing validation route or a focused temporary local harness. Invalid cases cover duplicate/missing exports, illegal group composition, layout mismatch, malformed local data, payload/attribute/recursion limits, and target capability failure. Remove every test-only class, file, fixture, executable, registration, shader, and generated output before handoff unless the user separately authorizes submitted test code.
- Replace ambiguous `SupportsRayTracing` authority with independent acceleration-structure, inline-ray-query, and RT-pipeline readiness across every RHI/Renderer producer and consumer; delete the old field. Readiness requires the complete extension/feature/property/function/backend chain, not an extension bit.
- Complete and prove the one semantic acceleration-structure binding retained by Phase 1. Force classic and partitioned TLAS providers on D3D12/Vulkan where supported; verify the same shader type, source, parameter signature, map entry, and graph call bind the exact backend-native descriptor representation. A provider remains unavailable if its descriptor layout/write/resource-resolution chain is incomplete. Do not revive raw-address shader uniforms, access-mode enums, duplicate shaders, alternate code records, or mutable-descriptor machinery without a separately demonstrated need.
- Add immutable backend-neutral RT pipeline composition descriptors, opaque `RayTracingPipeline` and `RayTracingShaderTable` products with generation identity, a logical table materialization request containing names and bounded POD rather than native identifiers, and `TraceRaysDesc` with raygen/miss/hit/callable regions and dimensions.
- Extend existing RHI pipeline/device/command owners. Define global/local binding semantics, resource states, tracking, legal queues, checked 64-bit size/index arithmetic, region bounds, failure reporting, and `TraceRays`; expose no D3D12/Vulkan handle, identifier bytes, or native table layout to Renderer.
- D3D12 builds a validated state object, exports/groups/config/associations, queries identifiers from that exact generation, packs aligned raygen/miss/hit/callable records, binds with `SetPipelineState1`, and dispatches through the neutral command path.
- Vulkan validates every dependent extension/feature/property/function pointer, builds stages/groups/pipeline/layout, queries group handles from that exact generation, packs device-addressable regions with device limits, and dispatches through `vkCmdTraceRaysKHR` via the neutral command path.
- Add one typed frame-graph ray-tracing pass kind and `TraceRays` builder path. Declare TLAS, global resources, outputs, table buffers, states, transitions, dependencies, queue legality, culling, and labels. `PassCommandContext` remains semantic-free.
- Extend `RenderPassRuntimeCache` rather than creating an RT cache. Materialize map refs, binding layouts, native pipeline, and immutable table before `FrameGraph::Execute`; capture the exact generation in the pass; reject a mismatched table; atomically publish reload replacement; retire old map/library/pipeline/table/resources after all submission tokens complete.
- Replace the valid-library runtime rejection only after the paired typed graph route passes. Update includes, CMake/source groups, capability diagnostics, inspection/provenance, object/marker names, current documentation, and architecture-boundary enforcement in this CL.

#### Positive guardrails

- One source-to-GPU identity chain covers every stage: shader class -> compile job -> map entry -> code record -> typed composition -> native pipeline generation -> table generation -> graph event/capture.
- Identifier/group-handle bytes and backend alignment remain backend-private; Renderer owns only logical exports, groups, record meaning, and bounds.
- Classic/partitioned AS storage and descriptor differences remain backend-private; Renderer shader/effect code sees one semantic AS parameter and one graph handle.
- All-stage conformance uses the smallest existing product/tool route or removed-before-handoff local harness; product effects receive no fake empty stages and the submitted architecture contains no test-only consumer.
- Run focused compiler, map/library, RHI contract, D3D12/Vulkan construction, invalid-input, native-validation, typed-graph, reload, and retirement checks. Use the same sentinels and logical table oracle on both APIs.

#### Negative guardrails

- No compiler-only RT map checkpoint, disabled public facade, backend-only permanent path, Renderer native calls, `ExternalProvider` trace callback, compute-pass disguise, second cache/generation, runtime package adapter, or compatibility field.
- No device-address/descriptor shader pair, AS-access authored define, TLAS address effect uniform, provider choice in a shader-map key, or fallback shader that merely disables traversal.
- No universal `ShaderProgram`, `TRayTracingProgram`, string-only export lookup, raw native identifier storage in Renderer, fat material/descriptor records in SBT, GPU-generated tables, recursion above the conformance need, or product effect migration in this phase.
- No pipeline/table creation, code lookup, disk I/O, or hidden resource discovery inside pass execution; no precache/readiness framework beyond synchronous owner-local materialization before execute.
- Do not call the RT pipeline capability available unless both D3D12 and Vulkan complete the same source-to-typed-graph conformance contract.

#### Acceptance criteria

- All six RT stage kinds compile, validate, publish through the final map/library, resolve by typed class, compose legally, materialize, execute, capture, reload, and retire in focused D3D12/Vulkan evidence, with any temporary conformance harness removed before handoff.
- Exact searches return zero old package RT records/readers/rejection, ambiguous `SupportsRayTracing`, duplicate RT registry, native trace call outside backend-private RHI, graph bypass, disabled public RT facade, and stale-generation acceptance.
- Invalid export/group/layout/local-record/payload/attribute/recursion/capability/table arithmetic/queue/generation inputs fail before unsafe native execution with one bounded owner-local diagnostic.
- D3D12/Vulkan native validation is clean; table addresses, region sizes, strides, alignments, counts, and logical indices agree with the neutral oracle; sentinels prove raygen, miss, triangle hit, procedural intersection/hit, any-hit, and callable execution.
- Forced classic/partitioned provider evidence shows one shader/map/graph identity and the correct native AS descriptor type/write on each supported backend; exact searches keep every Phase 1 duplicate/access-mode spelling at zero.
- Runtime support becomes true only through complete readiness; map/library/pipeline/table generations publish and retire atomically through the existing submission-token owner.
- A source-to-GPU trace for every stage and a reverse trace from each native dispatch/capture event resolve to one shader type, map entry, code record, composition, pipeline generation, table generation, and graph pass; no test-only, direct-native, or disabled facade is the sole consumer of the new contract.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; any missing backend cell, retained bypass, temporary harness artifact, or unproven readiness transition makes the phase `BLOCKED`.
- Scoped formatting, architecture boundary, build membership, focused builds/tests/cooks, `git diff --check`, and exact evidence paths are reported without a broad product-performance claim.

#### CL boundary

Suggested title: `Shaders: deliver paired ray-tracing pipeline foundation`.

This phase is intentionally larger than an ordinary subsystem CL. Splitting its compiler, public contract, backend, graph, or lifetime portions would create the misleading placeholder states this unified plan forbids.

### Phase 7 - Deliver dual-execution ray-traced GBuffer parity

#### Implementation prompt

> Implement Phase 7 as one product-effect vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including shared-semantics ownership proof, paired parity/alternate-path evidence, and the mandatory Code Review gate. Refactor the existing inline ray-traced GBuffer into shared semantic HLSL plus thin inline and pipeline frontends, add one immutable effect execution plan and strict requested/active mode semantics, schedule exactly one typed frontend, and prove same-frame output parity and the explicit raster path on D3D12/Vulkan. Use the Phase 6 map, pipeline, table, graph, and lifetime owners directly; add no second scene/material/output or execution system.

#### Phase-specific references

- [Ray-tracing target dual-execution contract](RayTracingPipelineImplementationPlan.md#effect-level-dual-execution-contract)
- [Ray-tracing target shader/SBT contract](RayTracingPipelineImplementationPlan.md#pipeline-abi-and-shader-table-contract)
- [Frame graph typed resource precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [NVIDIA NVRHI tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)
- [AMD FidelityFX inline ray-tracing helper at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)

#### Required work

- Freeze and test field-specific output comparison for base color, normal, material, emissive, subsurface, device Z, motion vector, miss values, face/culling, near/TMin, transforms, indexed/deformed geometry, and resolution bounds using identical immutable inputs in one frame.
- Split current GBuffer HLSL into one owner for ray setup inputs, hit reconstruction, material lookup, barycentrics, transforms, motion/depth conventions, miss encoding, and output stores; keep `RayQuery` candidate traversal and RT stage intrinsics in thin sibling frontends.
- Keep the current compute shader as the inline frontend. Add raygen, miss, and opaque triangle closest-hit shader classes and one focused `RayTracingPipelineComposition`; use one ray type, recursion depth one, global resources, zero local SBT data, and intentionally all-zero SBT contributions.
- Add `RayTracingExecutionMode::{Automatic,Inline,Pipeline}` in the existing renderer settings vocabulary and one immutable per-frame effect plan containing requested mode, active mode, readiness, and reason. Separate it from `GBufferMode`/algorithm selection and delete ambiguous `Raytraced` execution-API wording rather than aliasing it.
- Make strict `Inline`/`Pipeline` requests fail before graph construction when unavailable. `Automatic` chooses an available frontend through one inspectable Renderer policy. Schedule exactly one frontend pass and keep target creation, prepared scene, TLAS, material/geometry data, view, downstream consumers, histories, and the explicit rasterized-GBuffer algorithm unchanged.
- Add bounded active-mode/readiness capture markers and provenance joins without per-ray/per-record logging.

#### Positive guardrails

- Both frontends consume the same prepared scene/view and write the same authoritative GBuffer attachments in the same frame.
- Exact integer/identity fields compare exactly; floating-point tolerances are field-specific and frozen before results are inspected.
- Rasterized GBuffer remains an explicit supported algorithm, not a substitute that fabricates a ray-traced result; algorithm choice stays separate from execution API.

#### Negative guardrails

- No execution-mode branch inside a low-level pass Execute callback, duplicate target set, second TLAS/material table, duplicated hit reconstruction/output encoding, compatibility enum value, any-hit/procedural/callable/multi-ray-type complexity, or new temporal owner.
- No consecutive-moving-frame comparison, screenshot-only acceptance, hidden fallback for an explicit request, or product capability inferred from the conformance fixture alone.

#### Acceptance criteria

- Inline and pipeline GBuffer outputs pass the frozen same-frame oracle and clean D3D12/Vulkan captures across hit/miss and selected geometry/view edge cases.
- Requested/active/readiness behavior is deterministic and visible; strict unsupported requests schedule no partial graph; `Automatic` records its reason; the explicit raster algorithm remains functional.
- One shared semantic implementation owns hit/material/motion/depth/output work and exact searches find no ambiguous old mode name, duplicate scene/output authority, or two scheduled frontends.
- Both frontend call graphs reach the same semantic hit/material/motion/depth/output functions, and adapter inspection shows only traversal/stage mechanics; a focused semantic perturbation or equivalent fault-sensitive oracle detects either frontend bypassing shared logic before that temporary perturbation is removed.
- Reload and several frames in flight prove the effect captures and retires the exact map/pipeline/table generation through Phase 6 owners.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; duplicate outputs, scene/material tables, execution plans, or hidden strict-mode fallback block completion.
- Focused shader/effect/backend checks, scoped diff, stale-name searches, and `git diff --check` pass with exact evidence.

#### CL boundary

Suggested title: `Renderer: deliver dual-execution ray-traced GBuffer`.

### Phase 8 - Add production hit semantics, shadow rays, and scene-to-SBT indexing

#### Implementation prompt

> Implement Phase 8 as one production hit-semantics and nontrivial table-mapping CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including formula-corruption evidence, scene/SBT authority closure, and the mandatory Code Review gate. Add alpha-tested any-hit parity, a second shadow-visibility ray type and dual frontend, and one authoritative instance/geometry/ray-type contribution plan shared by classic and partitioned TLAS construction. Preserve the Phase 6 runtime and Phase 7 effect contracts; do not put material data or transient addresses in the SBT.

#### Phase-specific references

- [Ray-tracing target SBT index formula](RayTracingPipelineImplementationPlan.md#sbt-organization-and-index-formula)
- [Ray-tracing target scene/effect ownership](RayTracingPipelineImplementationPlan.md#target-ownership)
- [Microsoft DXR hit-group indexing](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos shader binding table indexing](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [NVIDIA SBT data-layout optimization](https://developer.nvidia.com/blog/efficient-ray-tracing-with-nvidia-optix-shader-binding-table-optimization/)

#### Required work

- Add alpha/cutout cases covering accepted/rejected candidates, front/back faces, UV edges, opaque overrides, and miss-after-ignore. One shared alpha/material decision owns thresholds and texture policy; inline candidate commit and pipeline `IgnoreHit`/accept are thin adapters.
- Add typed any-hit export(s) only to triangle groups that need alpha. Do not add empty stages to opaque groups.
- Add a shadow-visibility pipeline/RGS frontend with distinct miss/hit payload/output semantics as the second ray type, sharing the existing direct-shadow product and semantics with the inline-query frontend. Replace the Phase 1 inline-only precondition with one pre-graph selection that schedules exactly one real shadow producer and fails when neither is available.
- Define one Renderer scene plan for geometry-segment ordering, ray-type ordering, logical records, and checked bounds. Compute `recordIndex = rayContribution + geometryMultiplier * geometryIndex + instanceContribution`; map it to Vulkan fields without changing its logical meaning.
- Extend existing classic and partitioned TLAS builders to publish the same nonzero `InstanceContributionToHitGroupIndex` plan instead of constant zero. Delete any parallel mapping or effect-local contribution table.
- Make material/geometry/ray-type changes invalidate the logical table generation; do not rebuild BLAS/TLAS when AS content is unchanged. Keep large material/geometry data in shared buffers and local records limited to stable bounded indices only when measured/required.
- Measure table bytes, record counts, build/update time, invalidation reason, and TLAS/BLAS work with bounded evidence surfaces rather than permanent per-record logs.

#### Positive guardrails

- Classic and partitioned TLAS, inline traversal, and pipeline trace use one scene/material identity and one logical mapping.
- Corrupt each formula term and table order independently; bounds failures are caught before dispatch or produce the expected sentinel in focused negative validation.
- Preserve GBuffer parity, the explicit raster GBuffer algorithm, mandatory shadow production, and all generation/lifetime invariants.

#### Negative guardrails

- No pointer identity, transient descriptor address, duplicated material payload, per-frame unconditional table/TLAS rebuild, second scene slot allocator, silent alpha-policy drift, or hidden ray-type order.
- No global recursion/stack increase, procedural geometry, callable product dependency, or migration of unrelated ray-query effects.

#### Acceptance criteria

- Alpha-tested GBuffer and shadow visibility agree between inline/pipeline modes on D3D12/Vulkan; the explicit raster GBuffer algorithm remains accepted, while unavailable shadow production fails before graph construction.
- The full index formula passes multi-instance, multi-geometry, multi-material, two-ray-type maximum-valid/first-invalid and independent corruption cases.
- Classic/partitioned builders publish one plan; exact searches return zero constant-zero renderer contributions where mapping is required, duplicate table authority, or large/transient local record data.
- Dirty-generation behavior avoids unrelated BLAS/TLAS/table rebuilds and reports bounded measured bytes/time/reasons.
- Bidirectional traces from scene primitive/material/ray-type changes to table invalidation and from native table records back to the one Renderer plan prove no effect-local mapping, second slot allocator, or backend-specific logical order can produce a record.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; any corruption case that reaches unsafe dispatch or any unexplained AS/table rebuild blocks completion.
- Focused effect/scene/backend/lifetime checks, architecture boundaries, scoped diff, and `git diff --check` pass.

#### CL boundary

Suggested title: `Renderer: unify production ray hit semantics and SBT indexing`.

### Phase 9 - Migrate eligible effects and deliver one whole-frame execution plan

#### Implementation prompt

> Implement Phase 9 as one whole-frame ray-tracing selection and eligible-effect migration CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including whole-frame selection reachability, strict-mode negative proof, and the mandatory Code Review gate. Classify every current ray-query effect, migrate only effects with a coherent pipeline design and accepted parity/quality oracle, resolve one immutable strict/automatic execution plan before graph construction, preserve all temporal and supported-alternate ownership, and delete deep feature-specific API selection. Do not require a mega-pipeline or force unsuitable effects into pipeline mode.

#### Phase-specific references

- [Ray-tracing target selection semantics](RayTracingPipelineImplementationPlan.md#selection-semantics)
- [Ray-tracing target shared HLSL boundary](RayTracingPipelineImplementationPlan.md#shared-hlsl-boundary)
- [NVIDIA RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT)
- [AMD Cauldron ray-tracing capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [Debug View Presentation Contract](../DebugViewPresentation.md)

#### Required work

- Inventory every selected ray-query GBuffer, direct/indirect/reference/ReSTIR/shadow effect and classify it `Dual`, `InlineOnly`, `PipelineOnly`, or `SupportedAlternate` with owner, reason, shared semantics, output/history contract, readiness, and accepted oracle. `SupportedAlternate` must be a real algorithm with its own contract and evidence; a dummy product is unclassifiable.
- Resolve global requested `Inline`, `Pipeline`, or `Automatic` once before frame-graph construction into an immutable per-effect plan. Strict modes list every incompatible selected effect and schedule no partial frame. `Automatic` may mix modes but records capability, availability, measured policy input, active choice, and reason.
- Migrate only effects with a useful typed composition and accepted parity/quality route. Share ray setup, hit/material/light/BSDF/output semantics; keep `RayQuery` and RT stage intrinsics in thin frontends; schedule exactly one frontend per effect.
- Keep algorithm selections such as Reference/ReSTIR and GBuffer method independent from execution API. Rename UI, settings, captures, and diagnostics that conflate them; delete deep capability queries and unused `CanUseInlineRayQueryShadows`-style policy.
- Preserve each effect's existing outputs, accumulation/denoiser/history invalidation, scene/TLAS/material authority, supported alternate algorithms, and generation reload. Shadow visibility remains mandatory and has no no-ray alternate. Test mode transitions and reload across several effects sharing map/pipeline/table generations.
- Document honestly any retained single-mode effect and why; full-pipeline availability is not a requirement to migrate an effect with no demonstrated benefit.

#### Positive guardrails

- Selection is Renderer policy, resolved once, stable for the frame, and visible in capture/evidence metadata.
- Effects may share shader code, map records, pipelines, or table generations only through their owning immutable caches and complete keys.
- Supported-alternate and temporal behavior remain effect-owned rather than copied into the execution planner.

#### Negative guardrails

- No global mega-pipeline, vendor-ID heuristic without measured evidence, hidden per-pass substitution, fabricated product, partial strict frame, duplicated history, second execution settings tree, or claim that every shader can switch invocation APIs.
- No migration merely to achieve stage/API coverage; conformance and product value remain separate claims.

#### Acceptance criteria

- Every current ray-query effect has one explicit classification and owner; every migrated effect passes its paired D3D12/Vulkan correctness/quality/history/supported-alternate/reload gate.
- Strict modes preflight the whole selected frame and produce one actionable incompatibility result without scheduling; `Automatic` is inspectable and deterministic for identical inputs.
- Exactly one frontend is scheduled per selected effect, algorithm and execution axes are independent, and exact searches find no deep API selection, ambiguous old labels, duplicate plan/settings/history, fabricated product, or silent substitution.
- Shared pipeline/table generations remain lifetime-safe across multi-effect reload and several frames in flight.
- Fault-sensitive selection cases inject one incompatible effect, missing mandatory producer, stale generation, and unavailable supported alternate and prove strict planning rejects atomically while `Automatic` records one deterministic real implementation; removing or bypassing the central plan must make these checks fail.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; an unclassified effect, per-pass mode branch, copied history owner, or second settings/plan tree blocks completion.
- Focused whole-frame plan/effect/backend checks, scoped diff, stale-name audit, and `git diff --check` pass.

#### CL boundary

Suggested title: `Renderer: deliver whole-frame ray execution planning`.

### Phase 10 - Deliver Apply Changed and one shader-to-GPU provenance trace

#### Implementation prompt

> Implement Phase 10 as one Application/Editor shader-workflow and provenance CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including intent-to-owner trace proof, frontend implementation-detail audit, and the mandatory Code Review gate. Starting from the package-free vocabulary and complete raster/compute/RT runtime delivered by Phases 4-9, replace parallel recook/reload controls, artifact-directory scans, and the implementation-record table with one semantic `Apply Changed` workflow, immutable operation/catalog read models, automatic activation after renderer validation, contextual expert inspection, and one provenance trace from shader/effect identity through map, native pipeline/table generation, graph event, and capture. Delete manual normal-path reload, duplicate status formatting, and obsolete presentation fields. Do not add another panel, cache browser, log stream, permutation UI, backend-control surface, or executable-bypass path.

#### Phase-specific references

- [Editor and Tools intent-first workflows](../../Engineering/Standards/EditorAndTools.md#intent-first-frontend-workflows)
- [Validation logging and instrumentation](../../Engineering/Standards/ValidationPerformanceAndEvidence.md#logging)
- [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [PIX shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [Ray-tracing target diagnostics and selection](RayTracingPipelineImplementationPlan.md#effect-level-dual-execution-contract)

#### Required work

- Editor submits `Apply Changed`; Application snapshots changed virtual paths and routes one request; ShaderCompiler selects/cooks/publishes; Renderer validates/activates; the operation settles once.
- Keep expert `Rebuild All` and typed shader targeting in Advanced/CLI only. Remove manual normal-path reload; package targeting must already be absent at the Phase 4 floor.
- Replace implementation-oriented editor rows and artifact scans with shader type, stage, virtual source, active status, graph consumers, and for RT only the typed composition/effect/active-mode/readiness relation from immutable owner read models.
- Present one concise result. Failure leads with source root cause, next action, and confirmation that the previous generation remains active.
- Add one trace from shader type, effect, graph/capture label, code hash, or pipeline key through declaration, dependencies, compile job/input hash/result, map entry, code record, typed graphics/RT composition, runtime map/pipeline/table generation/materialization, execution plan, consumers, SBT logical record when applicable, and symbols/capture.
- Delete duplicated coordinator/console/panel lifecycle logs/status formatting; keep one bounded operation result through `EditorOperationService`.

#### Positive guardrails

- Application routes without reproducing compiler/runtime policy; ShaderCompiler owns dependency/cook/publication; Renderer owns validation/activation/generation/retirement; Editor owns presentation.
- Primary UI remains shader/source/task oriented; raw hashes/reflection/disassembly/requests remain contextual.
- RT native handles, identifiers, byte strides, and backend construction remain inaccessible to the frontend; it reads bounded semantic/provenance views only.

#### Negative guardrails

- No UI compiler sessions, cache directories, publication files, mutable renderer caches, RHI objects, task executor, artifact scans, per-job dialogs/toasts, readiness/precache controls, or second operation runtime.

#### Acceptance criteria

- Normal workflow exposes one dominant `Apply Changed` action and no package/layout/hash/backend/cache mechanics.
- The Phase 4 package-eradication floor remains clean, and exact searches return zero editor artifact-directory scans or obsolete parallel-workflow fields.
- Success activates one validated map/pipeline/table generation set; failure/cancellation settles once and preserves the previous generation without partially switching RT effects.
- Trace reads authoritative state and creates no duplicate registry/cache/log.
- A bidirectional workflow trace proves the visible intent reaches the existing Application, ShaderCompiler, Renderer, and Editor owners exactly once, while frontend model inspection contains semantic status/provenance only and no compiler session, file-layout, native handle, cache, publication, or activation policy.
- Fault-sensitive success, compile failure, validation failure, cancellation, and stale-result cases prove one terminal operation result and preservation of the accepted generation; the common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shader Tools: deliver Apply Changed and shader-to-GPU provenance`.

### Phase 11 - Eradicate legacy and compatibility surfaces

#### Implementation prompt

> Implement Phase 11 as one adversarial legacy-eradication and ownership-closure CL directly in the unstaged `master` worktree after Phases 0-10 are complete. Apply the [common phase delivery contract](#common-phase-delivery-contract), including repository-wide exact and semantic-equivalent searches, bidirectional owner traces, generated/build/frontend inspection, and the mandatory Code Review gate. Delete every residual legacy, compatibility, duplicate, bypass, fallback, eager-variant, diagnostic-scaffold, and renamed-equivalent surface from the shader, graphics-pipeline, ray-tracing, cook, runtime, graph, and Shader Tools migration. Fix each finding at its current owning responsibility and update all consumers in the same CL. Do not use this phase to defer deletions assigned to earlier phases, introduce a third design, or preserve an old path because final validation has not run; do not stage, commit, push, or submit.

#### Phase-specific references

- [Integration Style Guide clean-break policy](../../Engineering/Standards/IntegrationStyleGuide.md#current-clean-break-policy)
- [Code Review](../../Engineering/CodeReview.md)
- [Repository Structure and Ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)

#### Required work

- Re-run the Phase 0 rejected-name floor and add exact searches for every type, field, file, path, overload, macro, generated record, diagnostic label, model property, help spelling, and semantic equivalent removed by Phases 1-10. Search definitions, uses, includes, CMake/source groups, registrations, generated/cooked artifacts, tests-as-consumers, Application/Editor/CLI models, comments, and current documentation.
- Prove one authority for shader declaration/parameters, virtual source/dependency identity, compile request/job/input hash, catalog/map/code library, runtime generation, graphics state contributions/key/materialization, graph dispatch/draw/trace, RT composition/pipeline/table, scene-to-SBT mapping, effect execution plan, and Apply Changed state. Delete any mirror, forwarding owner, alternate generation counter, copied schema, or lookup route.
- Delete residual package/program/pass-wrapper vocabulary, `_NAMED` or dual-name binding compatibility, old physical source roots, texture/buffer `Read` aliases, raw TLAS-address/access-mode variants, no-query/fabricated-product fallbacks, ambiguous capability/mode fields, compiler-only RT records, native graph bypasses, and package/artifact frontend mechanics.
- Delete residual `GraphicsShaderPipelineState`-shaped bags, caller-authored attachment signatures, one-value vertex-layout selectors, shader-pair-only graphics keys, `RasterPassPipelineRuntime` base/wireframe/two-sided bundles, generic binding-time view/material policy, duplicate target bind/clear and topology routes, backend hard-coded state that overrides a neutral descriptor, and speculative PSO construction/precache/readiness scaffolding.
- Remove compatibility aliases, conversion constructors, fallback readers/writers, dual emission, feature flags selecting old/new architecture, deprecated overloads, test-only production registrations, migration counters/reports, verbose logging, per-item diagnostic spam, and comments/docs that describe rejected behavior as current.
- Inspect touched folders, owners, functions, and dependency directions for god units or generic buckets created during migration. Split only genuine mixed responsibilities through the existing target owners; do not perform unrelated subsystem reorganization.
- Reconcile source/build/document consumers and regenerate no artifacts in this phase. Phase 12 owns the single final regeneration and executable proof after the source floor is clean.

#### Positive guardrails

- Treat an old responsibility behind a new spelling as legacy; the search floor is semantic as well as textual.
- Every finding names its current owner, producer/consumer route, deletion patch, and claim-falsifying recheck.
- Preserve concise owner-local validation and durable diagnostics needed to explain real failures.
- Earlier phases still delete their assigned old paths atomically; this phase is an adversarial final floor, not a cleanup bucket.

#### Negative guardrails

- No alias, adapter, converter, deprecated overload, compatibility reader/writer, dual registry/map/cache, hidden feature flag, fallback producer, native bypass, or `Legacy`/`V2`/`New` namespace.
- No blanket removal of useful errors, external capture markers, or validation merely to satisfy a string search; relocate or narrow only when ownership is wrong or output is excessive.
- No speculative architecture, permutation, precache, preload, driver-cache, or reporting framework.
- No build, cook, launch, capture, performance run, or claim that final executable acceptance passed.

#### Acceptance criteria

- Repository-wide exact searches return zero definitions/uses/build entries/generated records/frontend fields/current-doc endorsements for every phase-owned rejected spelling, including all package/pass-wrapper/dual-name/source-path/AS-variant/fallback/ambiguous-capability/graphics-state/eager-variant/native-bypass spellings.
- Semantic searches and bidirectional traces prove no equivalent survives under a rename: each user-authored fact has one authority, each generated fact has one derivation, and each runtime product has one materialization/publication/retirement route.
- `RasterPassRenderState` remains narrow, attachments remain authoritative, prepared mesh work owns geometry/topology, the complete graphics descriptor is internal, and exact requested pipelines are the only materialized variants. No backend default silently changes declared semantics.
- Shader and RT routes have no package/program/duplicate parameter/parallel registry or generation path; all supported execution modes share the intended shader/map/scene concepts and schedule one real producer. Missing mandatory work fails before graph scheduling.
- Application/Editor/CLI expose semantic intent and bounded state only; package paths, artifact directories, native handles, cache controls, per-shader implementation tables, and duplicate operation truth are absent.
- Scoped structure review finds no new god owner/folder/function, forwarding-only helper, generic utility bucket, excessive diagnostic scaffolding, or duplicated validation/policy in the migration surface; any retained large unit has one cohesive documented responsibility.
- Includes/CMake/source groups/current docs reconcile, local links and no-write formatting pass, `architecture_boundary_check` passes, `git diff --check` passes, branch is `master`, and the staged diff is empty. No executable result is claimed.
- The Phase 11 Code Review report is `PASS` with no P0-P2 finding. Any unresolved legacy/equivalent owner blocks Phase 12 rather than being listed as later cleanup.

#### CL boundary

Suggested title: `Shaders: eradicate legacy shader and pipeline architecture`.

### Phase 12 - Regenerate, validate, and hand off the complete shader, graphics-pipeline, and ray-tracing candidate

#### Implementation prompt

> Implement Phase 12 directly in the unstaged `master` worktree against the complete Phase 0-11 candidate. Apply the [common phase delivery contract](#common-phase-delivery-contract) as the final whole-system gate, including rechecking the clean Phase 11 legacy floor and a mandatory Code Review `PASS`. Delete obsolete disposable shader and cooked output, regenerate the one current catalog/map/library once, then perform claim-driven formatting, architecture, compiler, cook, raster/compute/inline/RT-pipeline runtime, graphics-state, reload, lifetime, alternate-path/failure, capture, and performance validation. Fix failures at their owning responsibility without compatibility and return to Phase 11 if any old or duplicate path is exposed. Remove temporary harnesses, fault injectors, verbose logging, reports, and excessive diagnostics before handoff. Do not stage, commit, push, or submit.

#### Phase-specific references

- [Change Process review and acceptance](../../Engineering/Standards/ChangeProcess.md#review-and-acceptance)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary enforcement](../RendererRhiBoundary.md#enforcement)
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md)
- [Ray-tracing target completion contract](RayTracingPipelineImplementationPlan.md)
- [Microsoft DXR functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos Vulkan ray tracing](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)

#### Required work

- Re-run and prove the clean Phase 11 floor: zero definitions/uses of every phase-owned package/program/pass-wrapper/duplicate-parameter/old-prefix/old-path/compatibility symbol; caller-authored complete graphics-state bag, repeated attachment signature, eager variant bundle, incomplete graphics key, and recording-time pipeline creation; compiler-only RT package/rejection path; ambiguous RT capability/mode; device-address/descriptor/no-query shadow duplication; shader-visible TLAS address/access mode; graph/native bypass; duplicate map/pipeline/table/scene/effect-plan authority; and permutation/precache/preload scaffold.
- Regenerate the complete catalog, dependency records, global shader map, code library, provenance, and publication metadata once from final source.
- Run pinned no-write formatting where available, `git diff --check`, local-link validation, file/CMake/include inventory, and `architecture_boundary_check`.
- Build the smallest owning ShaderCompiler/Renderer targets, then the exact D3D12/Vulkan DevelopmentEditor product target required by the contract.
- Cook and inspect every supported raster/compute/RT shader for DXIL and SPIR-V; compare reflection/layout/type/code/export/group/payload/attribute/recursion identities and capability policy without silently skipped cells.
- Validate checkout-independent hashes, in-operation duplicate fan-out, repeated-operation recompilation, changed dependency closure, cancellation, failed-job replay, transactional publication, stale rejection, invalid replacement rollback, delayed GPU completion, and generation retirement.
- Run paired D3D12/Vulkan correctness and clean native-validation routes for raster graphics-state contribution/materialization, attachment compatibility/actions, real GBuffer material variants, compute/inline-ray-query, forced classic/partitioned TLAS through the same semantic AS parameter, all-six-stage RT conformance, opaque/alpha GBuffer dual execution, shadow ray type, procedural/callable fixtures, every migrated whole-frame effect, exposure, presentation, debug, strict/automatic selection, device-recreation/reload, explicit supported alternate algorithms, and mandatory shadow-production failure.
- Freeze hardware, adapter, driver, API, build, scene, camera, settings, warm-up, sample count, percentile, comparison tolerance, and failure protocol before collection. Force unsupported capability, missing target/export/group, pipeline creation, SBT allocation/alignment/index, stale generation, device loss/recreation, shader reload, unavailable supported alternatives, and missing mandatory producers.
- Capture identical inline/pipeline inputs in PIX where applicable, RenderDoc where supported, and Nsight/vendor tooling when it supplies causal evidence. Mark effect, active mode/reason, shader/code identity, native pipeline generation, table generation, logical record counts/bytes, and dispatch dimensions without per-ray logging.
- Measure compile queue/wall/CPU time, compiler-session memory, selected/compiled job counts, generated/cooked bytes, map/library open time, graphics/compute/RT pipeline creation, table build/update/bytes, TLAS/BLAS work, cold/warm frame impact, CPU/GPU effect time, p50/p95/p99 frame time, memory high-water, reload overlap, explicit alternate selections, mandatory-producer failures, and generation retirement. Do not add precache/readiness metrics for a system not implemented.
- Perform a final diagnostic/code-structure audit: no migration log stream, per-job spam, default report files, cache browser, submitted test scaffold, god orchestrator/folder/function, forwarding wrapper, or duplicated validation/policy remains.
- Recheck `master`, empty staged diff, unrelated dirty exclusions, generated/cooked source-control policy, and exact diff boundaries. Leave all work unstaged.

#### Positive guardrails

- Use the cheapest claim-falsifying check first and report exact commands/configurations/results/unavailable evidence.
- Temporary local harnesses are removed before handoff; no submitted test-only code without separate authorization.
- Preserve concise owner-local failures and external capture/profiler integration while deleting migration diagnostics.
- Report both wins and regressions per effect/device/API; separate compiler/cold-start cost from steady state and do not infer architecture causes from timing alone.

#### Negative guardrails

- No speculative broad build before focused owners, simulated backend/capture result, performance claim without complete provenance, nonmatching inline/pipeline frames, summed GPU queues, one-mean conclusion, retry loop, compatibility reader, old/new cook, fallback catalog, device-idle reload, or miscellaneous final-fix bucket.

#### Acceptance criteria

- Every final acceptance criterion below has exact evidence or is explicitly blocked; no unrun/static-only check is called passed.
- Shader class/catalog/job/map/library/runtime/graph/frontend, graphics state contributions/key/descriptor/materialization, and RT composition/pipeline/table/scene mapping/effect plan each have one authority and no legacy/compatibility/bypass path.
- Required generated artifacts match final source; no obsolete output, report, debug artifact, capture, log, or temporary proof file is unintentionally included.
- Diagnostics are bounded, orchestration reads as named stages, and no owner/folder/function mixes unrelated responsibilities.
- D3D12/Vulkan evidence proves attachment-derived graphics compatibility, granular pass state, exact-only pipeline variants, all six RT stages, GBuffer and shadow dual-mode parity, strict/automatic selection, explicit supported alternate algorithms, mandatory-product failure, table indexing/bounds, reload/device recreation, and submission-token retirement; every unsupported effect or unavailable claim is named precisely.
- Repository-wide exact and semantic searches plus bidirectional owner traces prove every rejected responsibility is absent from runtime, tools, build membership, generated/cooked artifacts, frontend models, and current documentation—not merely renamed—and that shader authoring, metadata, map/library lookup, generation, graphics-state contribution/materialization, RT composition, native pipeline/table, scene mapping, graph execution, effect planning, and frontend intent each have one non-overlapping authority.
- The final Code Review report classifies every touched site, records the complexity and performance result, contains no P0-P2 finding, and resolves every earlier `BLOCKED` claim with exact evidence or leaves the whole migration `BLOCKED`; partial acceptance is not allowed.
- Branch is `master`, staged diff is empty, scoped checks pass where available, and the user receives the unstaged changelist for manual review.

#### CL boundary

Suggested title: `Shaders: validate unified shader and pipeline architecture`.

Typed permutations and PSO precaching remain separate future proposals after this unified migration is accepted.

### Unified per-CL implementation record

Every Phase 0-12 CL description must contain the applicable subset of this record. Phase 0 records its documentation-only inventory and blocked executable claims; later phases update the applicable evidence. Keep the record in the CL description or this document; do not add a runtime report system.

```text
Unified phase and selected slice:
Intended production outcome and phase non-goals:
Selected standards, architecture routes, PGE/workload gates:
Current owner extended:
Authority replaced and exact deletion obligation:
Producer -> owned product -> consumer:
Mutable owner, lifetime owner, publication, retirement:
Definition -> all uses and representative use -> owner trace:
Exact rejected names and semantic-equivalent search set:
Shader classes/stages and nested parameters:
Map/library entries and compile targets:
Raster pass state, attachment signature, mesh/material facts, graphics key/descriptor (if applicable):
RT composition, payload/attribute/recursion/local data (if applicable):
Scene/TLAS/SBT logical mapping (if applicable):
Requested/active execution, supported alternate, and mandatory failure (if applicable):
Frame-graph resources, queue, and captured generations:
Build/CMake/include/generated-artifact reconciliation:
Copy budget, permanent concepts added, and complexity removed:
Performance classification and expected cost movement:

Positive and negative/corruption checks:
D3D12/Vulkan parity checks:
Reload/lifetime/failure checks:
AC -> claim-falsifying check -> exact result/evidence:
Exact commands, configurations, results, and evidence paths:
Measured overhead and limits:
Unavailable evidence and blocked claims:
Unrelated dirty path exclusions:
Legacy-eradication searches:
Code Review P0/P1/P2 findings and final PASS/BLOCKED verdict:
```

### Unified verification matrix

| Layer | Required verification |
| --- | --- |
| authoring and parameters | one typed class/schema authority; explicit SRV/UAV/AS/attachment vocabulary; direct compute/graphics use; typed RT exports/groups; duplicate/missing/illegal relationships; no forwarding schema |
| source and compilation | virtual source identity; dependency closure; portable input hash; compile-every-selected-input; in-operation fan-out; cancellation; DXIL/SPIR-V stage capability truth |
| map and code library | deterministic catalog/map/library; code/hash/layout/export/group integrity; transactional publication; zero package reader/writer/identity compatibility |
| neutral RHI | independent AS/inline/pipeline capabilities; one semantic AS binding; immutable descriptors; checked SBT arithmetic; queue/resource/state legality; opaque generations; no native leakage |
| backend GPU | exact classic/partitioned AS descriptor layout/write; D3D12 state objects/identifiers/tables/dispatch; Vulkan RT pipelines/group handles/device-address tables/dispatch; all stage sentinels; clean native validation |
| frame graph/runtime | typed compute/draw/trace; declared resources/transitions/dependencies/culling; pre-execute materialization; exact generation capture; stale rejection; submission-token retirement |
| renderer scene/SBT | one classic/partitioned logical contribution plan; instance/geometry/ray-type formula; bounds; dirty generation; table bytes/update; no material duplication |
| effect selection/parity | one immutable whole-frame plan; strict/automatic matrix; exactly one frontend; same-frame GBuffer/shadow/migrated-effect parity; algorithm/API separation; supported alternates/history; mandatory-product failure |
| tooling/provenance | one `Apply Changed` operation; source-located failure; shader/effect-to-code/map/pipeline/table/graph/capture trace; bounded semantic frontend; no artifact scan or backend control |
| lifetime/failure | invalid replacement rollback; several frames in flight; reload/device recreation; table/pipeline generation match; no device-idle shortcut; previous accepted generation preserved |
| performance/evidence | fixed provenance; p50/p95/p99 CPU/GPU; compiler/map/pipeline/table/TLAS/BLAS/memory/bytes; cold/warm separation; paired captures; both wins and regressions |

### Unified review checklist

- Does the change extend one existing owner and delete every replaced authority in the same CL?
- Is every direct-binding shader-visible field declared once on its dispatch shader and used by graph/binding from the same metadata? Does every local-record field have one distinct group/stage owner rather than a mirror?
- Does every texture/buffer shader binding say SRV or UAV explicitly, every scene AS use its semantic binding, and every raster/depth output remain an attachment rather than a pretend shader view?
- Is a multi-stage composition present only where graphics draw state or an RT export/hit-group set genuinely requires it?
- Do catalog, compile job, map, code library, runtime generation, native pipeline, and table each own a distinct responsibility?
- Are effect/algorithm selection and inline/pipeline execution selection independent and resolved before pass creation?
- Are shared semantics actually shared, with `RayQuery` and RT stage intrinsics confined to thin frontends?
- Are acceleration-structure, inline-query, and RT-pipeline capabilities independent and truthful?
- Does classic/partitioned provider selection preserve one shader/map/graph identity, with exact descriptor lowering confined to private RHI and no speculative mutable-descriptor machinery?
- Are every export, group, layout, payload, attribute, recursion, local record, table region, and index validated before unsafe execution?
- Are native identifiers/group handles backend-private and tied to the exact pipeline generation?
- Does one Renderer scene plan own classic/partitioned TLAS contribution and SBT record meaning without duplicating material/geometry data?
- Does the frame graph own every resource, transition, dependency, queue rule, and pass generation reference?
- Does reload publish map/library/pipeline/table state atomically and retire old state by all-queue submission tokens?
- Do strict requests name every incompatible selected effect and schedule no partial frame? Are automatic choices inspectable?
- Are explicit supported alternate algorithms, temporal histories, and unchanged downstream outputs still owned and tested where they belong? Does every mandatory product reject a missing producer before scheduling?
- Do D3D12 and Vulkan evidence cover every claimed stage/effect and use matching inputs/provenance?
- Are temporary logs, reports, fault injectors, fixtures not authorized for submission, god units, wrappers, and duplicate policy removed before handoff?

### Unified implementation reference map

Local authority and workloads:

- [Ray-tracing target architecture](RayTracingPipelineImplementationPlan.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md)
- [Strategy Requirements](../../Strategy/Requirements.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md)

Primary ray-tracing implementation references:

- [Microsoft DirectX Raytracing functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos Vulkan ray-tracing chapter](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)
- [NVIDIA RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT)
- [NVIDIA DXR shader binding table tutorial](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-2)
- [NVIDIA SBT data-layout optimization](https://developer.nvidia.com/blog/efficient-ray-tracing-with-nvidia-optix-shader-binding-table-optimization/)
- [AMD FidelityFX inline ray-tracing helper at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)
- [AMD Cauldron ray-tracing capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [Unreal Engine hardware ray tracing](https://dev.epicgames.com/documentation/unreal-engine/hardware-ray-tracing-in-unreal-engine)
- [Unreal Engine `RHISupportsInlineRayTracing`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/RHISupportsInlineRayTracing)
- [Unreal Engine `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Unreal Engine `FRayTracingShaderBindingTableInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRayTracingShaderBindingTableIni-)
- [Unreal Engine `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)

## Final Acceptance Criteria

The unified shader, graphics-pipeline, and ray-tracing migration is accepted only when:

- every Phase 0-12 implementation record maps each AC to a claim-falsifying check and exact evidence, every deferred executable claim is discharged by its assigned later phase, and the final scoped [Code Review](../../Engineering/CodeReview.md) verdict is `PASS` with no P0-P2 finding;
- a one-to-one compute author writes one `GlobalShader<Shader>` class with nested `Parameters`, one `IMPLEMENT_GLOBAL_SHADER` declaration, parameter assignments, and `Dispatch<Shader>`; there is no package, program alias, pass-registration macro, duplicate pass schema, forwarding pass class, layout string, or pipeline string;
- `AllocParameters<Shader>()` and shader reflection/binding consume the same `Shader::Parameters` metadata and every shader-visible field has one declaration;
- graph input/output access derives from typed parameter fields and pass recording sees only declared resources;
- texture/buffer shader views use only explicit `CreateSRV` / `CreateUAV`, scene AS uses only `CreateAccelerationStructureBinding`, raster/depth outputs use neutral attachment bindings, and no generic `Read`, neutral `CreateRTV`, or neutral `CreateDSV` authoring alias remains;
- graphics names concrete stage shader types, narrow granular pass render state, and real prepared draw work without a universal shader-program or caller-authored complete pipeline-state abstraction;
- graph attachments are the sole author-facing target/depth compatibility and action authority; prepared mesh/material work owns vertex input, topology, fill/cull, streams, and draw arguments; the existing runtime owner assembles one complete immutable graphics key/descriptor and materializes only exact requested variants before recording;
- shaderless and true multi-stage/graph-only operations use narrow envelopes without copying shader-visible fields;
- every registered source/include has a canonical virtual path and portable diagnostic identity; same-basename paths cannot collide or shadow silently;
- catalog freeze rejects duplicate/late declarations with both source locations;
- the catalog/map contains exactly one variant per `(ShaderTypeId, Target)` and no permutation/precache/preload scaffolding;
- classic/partitioned TLAS and native descriptor/address storage never multiply shader classes, HLSL roots, parameter schemas, map records, or graph call sites; one semantic AS parameter is lowered and validated by private RHI;
- `ShaderCompileInputHash` changes for every compiler-affecting input, survives checkout relocation, and excludes package/pass/presentation text;
- identical compile requests deduplicate only within one active operation, repeated cooks compile again, cancellation settles, and no partial publication appears;
- `GlobalShaderMap` is the sole typed logical lookup and every map entry references a validated `ShaderCodeHash` in `CookedShaderLibrary`;
- runtime lookup never derives source basenames, package IDs, or cooked paths and no `.sparkshader` reader/writer remains;
- `RenderPassRuntimeCache` is the sole active/replacement/retired generation and materialized layout/graphics/compute/RT-pipeline/shader-table owner; creation occurs before recording, not in Execute;
- changed includes select every dependent shader type and no unrelated shader when dependency data is valid;
- Shader Tools presents one `Apply Changed` intent, one operation state, automatic validated activation, source navigation, and contextual shader/effect-to-map/pipeline/table/capture details without artifact scans, package mechanics, or native backend controls;
- compile/validation failure reports one source-located root cause and preserves the previous accepted generation;
- every supported raster, compute, and RT shader cooks and validates for the required DXIL/SPIR-V targets; unsupported language/backend/target combinations remain honestly classified;
- all six RT shader stages traverse class, compile job, map, library, typed composition, native D3D12/Vulkan pipeline, shader table, typed graph trace, capture, reload, and retirement in focused evidence, and any temporary conformance harness is absent from the handoff diff;
- acceleration-structure, inline-query, and RT-pipeline capabilities are independent and full pipeline readiness becomes true only when the complete backend/graph/runtime path is ready;
- native identifiers and group handles remain backend-private and every table region/index/alignment/bounds check is tied to the exact pipeline generation;
- ray-traced GBuffer and shadow visibility pass same-frame inline/pipeline parity, alpha/material semantics, two-ray-type indexing, and classic/partitioned TLAS mapping on both APIs; rasterized GBuffer remains an explicit algorithm and missing shadow production fails before graph construction;
- every ray-query effect is classified, every migrated effect schedules exactly one frontend from one immutable whole-frame plan, strict requests preflight atomically, automatic choices are inspectable, and temporal/history ownership is not duplicated;
- D3D12/Vulkan runtime/capture evidence covers raster, compute/inline query, RT conformance, migrated effects, presentation/debug, explicit supported alternates, mandatory-product failures, reload/device recreation, and resolves a captured shader/code/pipeline/table identity to the exact class, source closure, compile request, map entry, code record, logical table record, and symbols;
- delayed GPU completion proves old map/library/layout/pipeline/table generations retire only after all queue submissions complete;
- the Phase 0 rejected-name and semantic-equivalent floor is clean across runtime, tools, build membership, generated/cooked artifacts, frontend models, diagnostics, and current documentation; no alias, adapter, compatibility overload/reader, dual writer, fallback to the replaced contract, copied schema, parallel registry/cache/generation, or renamed legacy owner remains;
- no migration logging, report generator, cache browser, submitted test scaffold, god owner/folder/function, one-method forwarding wrapper, duplicated policy, or excessive diagnostics remains;
- the [required evidence pack](#required-evidence-pack) is complete or each unavailable claim is explicitly blocked with provenance.

## Final Position

Sparkle should follow Unreal's lean global-shader center end to end: one direct-dispatch shader class owns its nested parameters and optional compile hooks; non-dispatch RT stages add only stage-specific compile policy when needed; one implementation declaration owns virtual source, entry, and stage; one frozen catalog drives reproducible compile-every-time jobs; one generated global shader map resolves typed shader references to code-library records; and the frame graph dispatches, draws, or traces those shader types through the same typed metadata. Graphics authors set granular pass intent while attachments and prepared mesh/material work supply their own facts; the runtime owner alone assembles the complete graphics key/descriptor and lazily materializes exact requests. A focused ray-tracing composition names only the exports, hit groups, and shared ABI that a native RT pipeline genuinely needs, deriving global bindings from the selected ray-generation shader. One semantic acceleration-structure parameter covers classic and partitioned providers while private RHI owns native descriptor/address representation. Render-graph labels remain diagnostic presentation, while map, code, native pipeline, table, and generation mechanics stay behind their owners.

The clean target is neither "the filename is everything" nor "copy every Unreal subsystem." It is "the author states only the shader class, parameters, source/entry/stage, narrow raster intent, the focused RT composition when several stages truly cooperate, and the actual draw/dispatch/trace; the engine derives and validates everything else." Permutations, universal program types, precaching, preload/streaming, and native driver caches stay out until a measured workload earns them.
