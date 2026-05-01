# Sparkle Shader System Design

Status: Draft for architecture review  
Date: 2026-05-01  
Scope: Global, render-pass, and future ray tracing shader infrastructure; offline cooking; runtime cooked package loading; editor recook; DX12/Vulkan readiness

## Executive Summary

Sparkle's shader architecture should stay offline-cooked, single-threaded, and compiler-free at runtime. The current system already has a strong foundation: typed global shader registrations, generated package layouts, backend-neutral cooked shader packages, DXC and Slang backends, DXIL and SPIR-V targets, runtime package validation, and build-time boundary checks.

The next phase should not add parallel compilation or a hidden runtime compiler fallback. It should instead make the system easier to review, harder to misuse, and ready for future API growth. The recommended direction is a narrow Renderer-private shader runtime facade that centralizes pass shader package loading, binding-layout compilation, PSO creation, and hot-reload orchestration while keeping RHI as the owner of cooked package contracts and API-specific translation.

The guiding architecture principle is a professional three-layer split:

1. Compiler tool: owns source compilation, compiler backends, reflection extraction, cache identity, debug artifacts, and cooked package emission.
2. Renderer shader orchestration: owns shader intent, pass/package selection, runtime lookup policy, reload behavior, diagnostics, and validated runtime assembly.
3. RHI backend realization: owns bytecode consumption, binding/root-signature translation, API-specific validation, and native graphics/compute pipeline creation.

The portfolio story should be simple:

1. Shader source and typed metadata are authored in engine code.
2. `ShaderCompiler.exe` compiles and verifies shaders offline through DXC or Slang.
3. Cooked packages carry backend-neutral metadata plus DXIL/SPIR-V bytecode records.
4. Runtime loads only validated cooked packages and never compiles source.
5. Renderer pass code declares intent; a shader runtime facade assembles validated runtime state.
6. Boundary validation prevents compiler implementation details from leaking into runtime modules.
7. Ray tracing shader libraries and inline ray-query shaders use the same offline-cooked discipline instead of adding a second compiler path.

The implementation should also stay intentionally small. Separation of concerns is valuable only when each file owns a real concept. The plan should replace old paths instead of layering new wrappers over stale ones, and every phase should include cleanup work that keeps the shader system easier to navigate than before.

## Goals

- Keep runtime startup deterministic: missing, stale, or incompatible shader packages fail loudly.
- Keep shader compiler implementation details inside `Tools/ShaderCompiler/Backends/*`.
- Support DX12 today and prepare cleanly for Vulkan through target-format selection and SPIR-V packages.
- Treat DXC and Slang as peer offline compiler backends.
- Add a small typed permutation model for global/render-pass shaders.
- Prepare shader compiler and package infrastructure for ray tracing shader libraries, exports, hit groups, and inline ray queries.
- Make package identity, layout identity, backend identity, target identity, and reflection visible to reviewers.
- Improve editor recook and hot reload without introducing runtime compilation.
- Keep the cook path single-threaded for now.
- Reduce duplicate shader-system concepts as the architecture moves forward; new files must retire or simplify old ones, not add a second active path.

## Non-Goals

- No multithreaded shader cook graph in this phase.
- No shader worker process pool or distributed shader compilation.
- No runtime source compilation fallback.
- No material shader compiler or full material shadermap system yet.
- No bindless material texture rewrite in this phase.
- No Vulkan RHI pipeline implementation in this document; the package/runtime contract should prepare for it.
- No full ray tracing acceleration structure, shader table, or RHI state object implementation in this document; the compiler/package contract should prepare for ray tracing now.
- No work graph or mesh shader implementation beyond schema readiness.
- No abstraction added only to make the folder tree look more architectural; each new type must remove complexity, enforce a boundary, or make diagnostics/review materially clearer.

## Current Architecture Snapshot

### Offline Compiler

The offline compiler lives under `Tools/ShaderCompiler/` and is invoked by scripts and editor recook workflows. Important current properties:

- `ShaderCompiler.exe` is a standalone tool.
- CLI commands include `cook`, `list-backends`, `list-targets`, `list-shaders`, `list-permutations`, `inspect-shader`, and `inspect-package`.
- `cook` supports `--target`, `--backend`, `--no-cache`, `--cache-dir`, `--debug-artifacts`, `--analysis`, and a verification self-test.
- The cook executor is serial through `SerialCookExecutor`.
- DXC is isolated under `Tools/ShaderCompiler/Backends/Dxc/`.
- Slang is isolated under `Tools/ShaderCompiler/Backends/Slang/`.
- Orchestration talks to concrete backends only through `IShaderBackend` and `ShaderBackendPool`.
- Typed shader packages are planned through `ShaderCookPlanner` using `GlobalShaderRegistry`.
- Parameter structs are verified against reflection in `ShaderParameterStructVerifier`.
- Cooked packages are emitted by `CookedPackageWriter` and indexed by `CookedRegistryWriter`.

### Runtime

Runtime shader consumption is split between RHI and Renderer:

- RHI owns the cooked package contract under `Engine/RHI/Public/Shaders/`.
- `CookedShaderPackageCache` loads, validates, caches, clears, and reloads packages.
- Validation checks package key, variant hash, source identity hash, binding layout hash, shader model, stage mask, binding records, bytecode hashes, and required binary format.
- D3D12 binding layout compilation consumes cooked binding and reflection records to build root signatures.
- Renderer pass traits currently orchestrate package layout building, package loading, binding layout creation, and PSO creation.
- `PipelineStateManager` owns runtime storage and package cache lifetime.
- `ShaderRecookCoordinator` launches the external compiler and reloads cooked shaders after the RHI is idle.

### Existing Boundary Enforcement

`CMake/Validation/ValidateShaderCompilerBoundary.cmake` already enforces important constraints:

- Runtime modules must not include DXC/tool-only compiler types.
- ShaderCompiler must not depend on renderer, application, editor, game framework, or runtime-private RHI implementation.
- DXC tokens are confined to `Backends/Dxc/`.
- Slang tokens are confined to `Backends/Slang/`.
- The Application shader recook path is an explicit exception for invoking `ShaderCompiler.exe`.

## Professional Baseline

This design is informed by public professional engine patterns without copying their implementation.

The goal is not to make Sparkle look like Unreal, Donut, or Cauldron line-for-line. The goal is to learn the engineering standards they demonstrate: explicit shader identity, typed metadata, deterministic offline compilation, strict runtime contracts, backend isolation, inspectable cooked artifacts, and small API surfaces between systems.

### Industry Standards We Should Follow

| Standard | Public reference pattern | Sparkle equivalent | Required Sparkle work |
| --- | --- | --- | --- |
| Typed shader identity | Unreal `FShaderType`, `FGlobalShaderType`, `IMPLEMENT_SHADER_TYPE` | `GlobalShaderRegistry`, `TGlobalShader`, `IMPLEMENT_GLOBAL_SHADER` | Keep typed registrations as the source of truth; improve `inspect-shader` output. |
| Typed permutations | Unreal `FShaderPermutationDomain`, bool/int dimensions, permutation IDs | `TShaderPermutationDomain`, `ShaderPermutationVector`, `ShaderPermutationKey` | Finish stable dimension names, vector enumeration, variant IDs, and compile defines. |
| Offline compile cache identity | Unreal shader compiler input hashes and shader cache/DDC versions | `ShaderCacheKey`, backend name/version, target, source/dependency/settings hashes | Make backend/target/cache fingerprints visible in diagnostics and package inspection. |
| Compile output versioning | Unreal shader compiler input/output versions and shader cache version GUID | `kCookedShaderPackageVersion`, registry version, backend version | Version package/registry only when serialized compatibility changes; document recook requirements. |
| Reflection/parameter validation | Unreal parameter map verification and shader binding layout metadata | `ShaderParameterStructVerifier`, cooked reflection records, `BindingLayoutHash` | Make reflected typed fields a hard cook contract, not a warning path. |
| Runtime shader maps/packages | Unreal global shader maps and serialized shader code libraries | `.sparkshader` cooked packages plus `ShaderPackageRegistry.sreg` | Move toward multi-format packages and reviewer-friendly package inspection. |
| Runtime bytecode resolution facade | NVIDIA Donut `ShaderFactory` and bytecode cache | Proposed Renderer-private `RenderPassShaderRuntime` | Centralize package lookup, binary-format selection, binding layout, and PSO assembly. |
| API-specific binary selection | Donut platform bytecode macros and auto shader creation | `CookedShaderBinaryFormat`, required runtime binary format | Let one logical package carry DXIL and SPIR-V records for DX12/Vulkan readiness. |
| Ray tracing shader libraries | Unreal ray tracing shader classes and pipeline state, DXR/Vulkan RT shader libraries, NVIDIA/AMD samples using raygen/miss/hit/callable exports and hit groups | Future ray tracing shader package kind, export records, hit-group records, and local parameter layouts | Add compiler/package metadata before RHI state object work so ray tracing does not become a parallel shader system. |
| Inline ray tracing / ray queries | DXR inline ray tracing through HLSL `RayQuery` in regular shaders; Vulkan ray query through SPIR-V ray query capabilities | Graphics/compute packages with ray-query feature requirements and acceleration-structure resource bindings | Track inline ray tracing as a shader feature flag, not as a separate RT library or hit-group path. |
| Binding and pipeline object separation | AMD Cauldron `RootSignatureDesc`, `RootSignature`, `PipelineDesc`, `PipelineObject`, `ParameterSet` | `PassParameterLayout`, `RenderBindingLayout`, `RenderPipelineState`, `PassParameterSet` | Keep RHI-native root signatures/PSOs behind RHI; keep pass parameter binding RHI-neutral. |
| Shader blob resource metadata | FidelityFX `FfxShaderBlob` resource counts and binding names | Cooked binding records plus reflection arrays | Preserve backend-neutral resource metadata in packages and expose it through inspection. |
| Tool/runtime boundary | Unreal editor/compiler separation, Donut factory boundary, Cauldron backend objects | `ValidateShaderCompilerBoundary.cmake` and backend directories | Expand validation as old paths are removed; keep compiler APIs out of runtime. |

### Standards We Should Not Copy Yet

| Deferred standard | Why it is deferred |
| --- | --- |
| Unreal shader worker process pool and distributed compilation | The current priority is correctness, boundary clarity, and portfolio reviewability; the user explicitly wants single-threaded cook for now. |
| Unreal material shader maps | Valuable long-term, but too large for this pass. Global/render-pass shaders should prove the architecture first. |
| Runtime on-demand shader compilation | Conflicts with Sparkle's strict cooked runtime requirement and makes boundaries harder to review. |
| Bindless material pipeline | Deferred by architecture preference; the near-term path should remain bindful and explicit. |
| Full Vulkan pipeline creation | The package contract should prepare SPIR-V and format selection now; Vulkan RHI implementation can follow later. |

### Epic/Unreal Pattern

Unreal's shader system is built around typed shader classes, shader type registration, permutation domains, shader maps, compiler jobs, derived data/cache keys, compiler output versioning, and parameter binding validation.

Sparkle should adapt:

- Typed shader metadata.
- Stable shader and permutation identity.
- Explicit compile input and output versioning.
- Cook-time binding validation.
- Runtime lookup through validated shader package identity.

Sparkle should not adopt yet:

- Worker/distributed compilation.
- Full material shader map complexity.
- On-demand runtime shader compilation.
- Large editor-only compilation manager infrastructure.

Concrete Sparkle lesson: use Unreal's discipline around identity and validation, not its full compilation scale. Sparkle should make every shader package answer these questions: which typed shader produced it, which permutation vector it represents, which backend and target produced each binary, which layout hash was validated, and why runtime accepted or rejected it.

### NVIDIA Donut Pattern

Donut keeps shader bytecode lookup behind a `ShaderFactory`-style abstraction and lets render passes request shaders without owning file-system or platform-bytecode details. It also supports API-specific bytecode selection and useful shader lookup/debug hooks.

Sparkle should adapt:

- A single facade for resolving runtime shader artifacts.
- API-specific binary selection hidden behind runtime shader infrastructure.
- Debuggability of bytecode/package lookup.

Sparkle should not adopt as the main path:

- Runtime source compilation.
- Static embedded shader blobs as the primary package model.
- Pass-local shader file probing.

Concrete Sparkle lesson: pass code should not know where bytecode lives or how to choose DXIL versus SPIR-V. A Renderer-private shader runtime facade should serve the same architectural role as Donut's shader factory boundary, but backed by Sparkle's cooked `.sparkshader` packages.

### AMD FidelityFX/Cauldron Pattern

Cauldron separates root signature descriptions, pipeline descriptions, pipeline objects, parameter sets, and shader blobs. FidelityFX shader blobs carry resource metadata used to build backend pipelines.

Sparkle should adapt:

- Explicit separation between package metadata, binding layout, pipeline object, and per-pass parameter binding.
- Backend-neutral shader resource metadata in cooked artifacts.
- Runtime objects named and inspectable for debugging.

Sparkle should not adopt:

- Sample-level render module ownership of compiler details.
- Per-pass ad hoc compiler use.

Concrete Sparkle lesson: keep resource binding, root signature, pipeline state, and parameter sets as separate reviewable concepts. Sparkle already has these pieces; the cleanup is to make the ownership obvious and prevent render passes from becoming miniature pipeline compilers.

### Sparkle Design Rules From The Baseline

These rules are the practical industry-standard checklist for future changes:

1. A shader must have stable typed identity before it can be cooked.
2. A permutation must have a deterministic key, visible dimensions, and stable compile defines.
3. A cooked package must contain enough metadata to diagnose backend, target, layout, reflection, and bytecode identity.
4. Runtime must validate package compatibility before creating binding layouts or PSOs.
5. Backend-specific APIs must remain behind backend modules or RHI implementation files.
6. Renderer passes should declare shader intent, not perform bytecode discovery or compiler work.
7. Editor hot reload may invoke tools, but runtime reload consumes only validated cooked artifacts.
8. Ray tracing shader support must extend the same package/registry system with libraries, exports, hit groups, and inline ray-query feature metadata; it must not create a second ad hoc compiler path.
9. CI must prove both positive and negative shader paths: successful cook, boundary cleanliness, and intentional metadata/reflection failure.

## Complexity Budget And Cleanup Discipline

The shader system should not become impressive by file count. Professional architecture is not maximum decomposition; it is clear ownership, fewer duplicate concepts, and predictable places to look. New files are acceptable when they create a durable boundary or make behavior easier to verify. They are harmful when they only rename an existing concept, preserve an obsolete path, or split a simple operation into many tiny hops.

### When A New File Or Type Is Justified

- It owns a real lifecycle, such as package loading, backend compilation, runtime facade assembly, or editor recook coordination.
- It isolates a dependency boundary, such as DXC, Slang, RHI backend code, Renderer policy, or Application/editor tool launching.
- It turns implicit policy into a reviewable contract, such as package validation, cache keys, permutation identity, or ray tracing export metadata.
- It reduces repeated code across multiple call sites without hiding important behavior.
- It gives diagnostics or inspection one obvious home.

### When To Avoid A New File Or Type

- The proposed type only forwards calls to one other object.
- The behavior is used once and is clearer inside the owning class or command.
- The split would require readers to jump through several files to understand one small operation.
- The old implementation would remain active beside the new one.
- The name describes a vague layer, such as manager, service, helper, or context, without a concrete resource or policy it owns.

### Migration Rules

1. Prefer replacement over compatibility shims once the new architecture is selected.
2. Every new shader-system path must identify the old path it makes obsolete.
3. Do not keep two active authorities for package layout, shader identity, cache identity, reflection validation, or runtime package loading.
4. If a transition temporarily needs both paths, document the removal condition in the same phase that introduces the new path.
5. Keep public headers smaller than private implementation surfaces; prefer Renderer-private and RHI-private files unless a type is truly part of a module contract.
6. Periodically search for stale terms, duplicate concepts, and unused helpers before adding the next feature phase.
7. Ray tracing compiler support should extend the existing package and registry model; do not create a disconnected RT-only shader pipeline unless a real API requirement forces it.

### Review Checklist For Complexity

- Can a reviewer state which of the three layers owns this file?
- Does this file have one reason to change?
- Does it replace an old concept or merely sit beside it?
- Is the public API smaller after the change?
- Are failure diagnostics easier to trace from user action to owning system?
- Would merging this file into its owner make the code easier to read?
- Did the change remove old comments, docs, validation exceptions, or helper paths that no longer describe reality?

## Ray Tracing Compiler Infrastructure Readiness

Ray tracing is close enough that the shader compiler infrastructure should reserve the correct concepts now. The RHI ray tracing implementation can remain deferred, but the compiler/package design should stop assuming every shader package is only ordinary graphics or compute.

Sparkle should plan for both ray tracing paths:

1. Ray tracing shader libraries: DXR/Vulkan RT libraries with ray generation, miss, closest-hit, any-hit, intersection, and callable exports. These feed hit groups, shader tables, and future RT pipeline/state object creation.
2. Inline ray tracing shaders: regular graphics or compute shaders that use ray queries, such as HLSL `RayQuery` with `RaytracingAccelerationStructure` resources or Vulkan ray query SPIR-V capabilities. These do not use raygen/miss/hit groups or shader tables, but they still require compiler target support, feature requirements, acceleration-structure bindings, and runtime capability validation.

### Current Gap

The current shader stage surface is intentionally small: vertex, pixel, geometry, hull, domain, and compute. `ShaderCompileProfile` builds DXC profiles from stage prefixes like `vs_6_0` and `cs_6_0`, while the Slang backend currently maps only vertex, pixel, and compute stages. That is good for the current renderer, but it is not enough for DXR/Vulkan RT libraries or inline ray queries.

Classic ray tracing needs shader libraries and exports, not just one entry point per graphics or compute stage. A package may contain ray generation, miss, closest-hit, any-hit, intersection, and callable exports. It may also contain hit groups that combine closest-hit, any-hit, and optional intersection exports. Runtime eventually needs shader table records and a ray tracing pipeline/state object, but the compiler package can prepare the metadata earlier.

Inline ray tracing should stay attached to normal graphics/compute packages. A pixel or compute shader that uses ray queries is still a pixel or compute shader from the pipeline's point of view. The package should record that the shader requires inline ray query support, an acceleration structure resource binding, and a minimum backend/API feature level. It should not create fake hit groups or a separate RT pipeline package.

### Required Compiler/Package Concepts

| Concept | Why it matters | Likely Sparkle shape |
| --- | --- | --- |
| Shader package kind | Graphics, compute, and ray tracing packages have different validation and runtime assembly rules. | Add a package kind such as `Graphics`, `Compute`, `RayTracingLibrary`. |
| Ray tracing shader stages | RT stages are not covered by graphics/compute stage masks. | Extend stage identity with `RayGeneration`, `Miss`, `ClosestHit`, `AnyHit`, `Intersection`, and `Callable`, or introduce a separate RT export kind if that keeps graphics masks cleaner. |
| Shader library/profile handling | DXR commonly uses library profiles such as `lib_6_3+`; Vulkan RT uses SPIR-V ray tracing stages. | Teach profile construction to handle RT libraries/exports instead of only prefix profiles. |
| Multi-source RT library packages | A reviewable RT library should not require one giant shader file; raygen, miss, hit, intersection, and callable shaders can be authored separately and packaged together. | Allow one ray tracing package to collect exports from multiple source files under one package id. |
| Export records | A ray tracing library is selected by named exports, not only stage bytecode. | Add cooked export records with export name, stage/export kind, entry point, binary record, and optional debug artifact. |
| Hit-group records | Closest-hit, any-hit, and intersection shaders are grouped for the RT pipeline. | Add backend-neutral hit-group metadata: group name, type, closest-hit export, any-hit export, intersection export. |
| Local parameter layout | DXR local root signatures and Vulkan shader binding table data need local shader parameters. | Keep global/pass parameter layout separate from future local RT parameter layouts. |
| Payload/attribute metadata | RT pipelines need payload and attribute size limits for validation and diagnostics. | Store declared payload size, attribute size, and recursion-related metadata where the package can inspect it. |
| Inline ray-query feature metadata | Inline RT is not a separate shader stage; it is a feature used by graphics/compute bytecode. | Add package or binary feature flags such as `UsesInlineRayQuery`, required shader model/capability, and acceleration-structure binding requirements. |
| Acceleration-structure resource binding | Both RT pipelines and inline ray queries need acceleration structure resources represented in binding metadata. | Add an acceleration-structure resource domain/semantic that can validate typed parameters and reflection without leaking D3D12/Vulkan names upward. |
| Inspector output | RT packages are hard to review without export and hit-group visibility. | Extend `inspect-package` to print package kind, exports, hit groups, local parameters, backend, target, and hashes. |

### Boundary Rules For Ray Tracing

- `Tools/ShaderCompiler` owns RT source compilation, export discovery, library metadata, hit-group metadata, reflection extraction, and package emission.
- `Tools/ShaderCompiler` also owns inline ray-query detection or declaration, required feature metadata, target/profile selection, and reflection validation for acceleration-structure bindings.
- Renderer shader orchestration owns RT feature intent: which ray tracing pipeline is requested, which exports/hit groups are used, and when packages reload.
- Renderer shader orchestration owns inline RT usage intent for regular passes, such as a shadow, lighting, reflection, or probe pass choosing a shader variant that uses ray queries.
- RHI backend realization owns DXR/Vulkan RT pipeline/state object creation, shader table allocation, acceleration structure binding, and native validation.
- RHI backend realization also owns inline ray-query capability validation for graphics/compute PSOs and native acceleration-structure binding translation.
- The first RT compiler work should not add runtime compilation, editor-only fallback compiles, or pass-local file probing.
- The package format should be extended once, with clear versioning, instead of creating separate `.rtshader` artifacts disconnected from `.sparkshader` and the shader registry.

### Near-Term RT Readiness Order

1. Extend shader identity and package metadata with package kind and RT export/hit-group concepts.
2. Add inline ray-query metadata for ordinary graphics/compute packages: feature flags, acceleration-structure bindings, minimum shader model/capability, and backend target requirements.
3. Allow a ray tracing library package to collect exports from multiple source files under one package id.
4. Teach `list-shaders`, `list-permutations`, and `inspect-package` to display RT library metadata and inline ray-query metadata even before runtime consumes it.
5. Add backend capability reporting for RT library support and inline ray-query support per target/backend.
6. Add DXC/Slang profile selection for DXIL ray tracing libraries, SPIR-V ray tracing targets, and regular graphics/compute shaders that require inline ray queries.
7. Add package validation for RT libraries: required exports, hit-group references, payload/attribute metadata, backend identity, target format, and layout hashes.
8. Add package validation for inline RT: required ray-query capability, acceleration-structure binding metadata, backend identity, target format, and layout hashes.
9. Only after those contracts are inspectable, add RHI DXR/Vulkan RT state object, shader table, acceleration structure, and inline ray-query PSO validation work.

## HelloWorld Shader Showcase Plan

The shader system should include tiny example shaders that prove each capability without requiring a full gameplay or material feature. These examples are for documentation, inspection, CI, and portfolio review. They should live under the existing shader asset tree, starting from `Engine/Assets/Shaders/HelloWorld/`, and should use the same typed registration/package path as real engine shaders.

| Example package | Shader file | What it proves | Runtime requirement |
| --- | --- | --- | --- |
| `HelloTriangle` | `HelloWorld/HelloTriangle.hlsl` | Basic graphics package with vertex/pixel stages and simple package inspection. | Can be runtime-visible or tool-only. |
| `HelloPermutation` | `HelloWorld/HelloPermutation.hlsl` | Small typed permutation domain, stable variant IDs, compile defines, and `list-permutations` output. | Tool-only is enough until pass integration exists. |
| `HelloCompute` | `HelloWorld/HelloComputeCS.hlsl` | Compute package, UAV binding, reflection validation, and DXIL/SPIR-V multi-format output. | Can reuse compute runtime patterns later. |
| `HelloInlineRayQuery` | `HelloWorld/HelloInlineRayQueryCS.hlsl` | Ordinary compute package with inline ray-query feature metadata and acceleration-structure binding metadata. | Tool-only until RHI inline RT validation exists. |
| `HelloRayTracingLibrary` | `HelloWorld/RayTracing/HelloRayGen.hlsl`, `HelloMiss.hlsl`, `HelloClosestHit.hlsl`, `HelloAnyHit.hlsl`, `HelloIntersection.hlsl`, `HelloCallable.hlsl` | RT library package assembled from one file per RT shader type, with exports, hit groups, payload/attribute metadata, and package inspection. | Tool-only until DXR/Vulkan RT state object support exists. |

Rules for examples:

- Keep each example intentionally small and named around the capability it proves.
- Prefer the smallest file set that teaches the concept clearly; ray tracing library examples should use separate files per shader type because exports and hit groups are the lesson.
- Register examples through the same typed shader registration mechanism as production shaders.
- Mark examples as tool-only until the Renderer/RHI runtime can honestly execute them.
- Include each example in `list-shaders`, `inspect-shader`, `list-permutations`, and `inspect-package` acceptance checks when the underlying feature lands.
- Do not add bespoke example-only compiler paths, package formats, runtime fallbacks, or hand-authored package layouts.

## Target Architecture

The target architecture keeps the runtime compiler-free and moves orchestration into a narrow Renderer-private shader runtime facade.

```text
Shader source + typed metadata
    |
    v
GlobalShaderRegistry
    |
    v
ShaderCompiler.exe cook
    |
    |-- ShaderCookPlanner builds packages and permutations
    |-- ShaderCookGraphExecutor runs serial compile nodes
    |-- IShaderBackend compiles through DXC or Slang
    |-- Reflection verifies typed parameter structs
    v
Cooked shader packages + registry
    |
    v
Runtime startup / reload
    |
    |-- Renderer shader runtime facade receives pass declarations
    |-- Renderer facade resolves package request and reload policy
    |-- RHI package reader/validator checks package contract
    |-- RHI backend compiles binding layout/root signature
    |-- RHI creates PSO
    v
RenderPassRuntimeStorage
```

### Proposed Ownership

| Area | Owner | Notes |
| --- | --- | --- |
| Typed shader metadata | Shared shader authoring contract, currently RHI-public shader authoring headers | Stage, package id, parameter struct, and permutation metadata are allowed here only while they remain renderer-agnostic. |
| Renderer pass shader declarations | Renderer-private pass code | Passes declare package id, variant id, expected stages, and PSO intent. They should not load files or know compiler details. |
| Offline compilation | `Tools/ShaderCompiler` | Concrete compiler APIs stay backend-local. |
| Cooked package schema | RHI public shader contract | Shared by tool and runtime. Must stay backend-neutral and free of pass semantics. |
| Cooked package binary reader and validator | RHI | Bytecode format, stage records, reflection records, and binding layout compatibility feed backend pipeline creation. |
| Package locating, reload policy, and pass diagnostics | Renderer-private shader runtime facade | The facade knows startup/reload policy, pass names, fallback behavior, and editor handoff. |
| API-specific root signatures and PSOs | RHI backend | D3D12/Vulkan decisions stay behind RHI. |
| Pass shader runtime orchestration | Renderer-private shader runtime facade | Renderer declares pass needs; facade assembles runtime storage. |
| Editor recook | Application/editor bridge | May launch the external tool, then ask Renderer to reload after idle. |

## RHI vs Renderer Shader Boundary

The user's instinct is mostly right: a shader compiler is not a hardware interface, and a full shader system is not pure RHI. Professional engines usually split the problem into three layers rather than choosing only Renderer or only RHI.

1. Tool/compiler layer: owns source compilation, compiler backends, reflection extraction, cache keys, debug artifacts, and package emission.
2. Render-system layer: owns shader identity, pass/material intent, shader maps or factories, runtime lookup policy, reload behavior, and diagnostics.
3. RHI/backend layer: owns shader bytecode handles, binding/root-signature translation, pipeline state creation, and API-specific validation.

Sparkle should follow that split.

### Real Engine Reference Points

| Engine/sample family | Where the boundary sits | Sparkle lesson |
| --- | --- | --- |
| Unreal Engine | RHI exposes low-level shader and pipeline state concepts. RenderCore owns most typed shader metadata, shader maps, parameter binding machinery, and runtime shader lookup. Renderer consumes those abstractions in passes and materials. Shader compilation lives in developer/tool modules, not inside RHI. | Do not put the whole shader system in RHI. Keep RHI focused on bytecode, binding, and PSO creation; put shader identity/orchestration above it. |
| NVIDIA Donut | Render passes request shaders through a `ShaderFactory`/device-facing abstraction instead of probing files or compiling locally. API-specific bytecode selection is centralized. | Sparkle's Renderer-private `RenderPassShaderRuntime` should play the factory/orchestration role, while RHI creates backend objects from selected bytecode. |
| AMD FidelityFX/Cauldron | Cauldron separates shader blobs/resource metadata from `RootSignature`, `PipelineDesc`, `PipelineObject`, and `ParameterSet`. Render modules describe pass needs; backend/RHI objects realize them. | Keep package metadata, parameter layouts, root signatures, PSOs, and pass parameter binding as separate objects with clear owners. |

### Why Not Put All Shader Runtime In RHI?

Pros:

- Bytecode format selection is naturally close to the active graphics API.
- Root signature and pipeline state creation are backend responsibilities.
- Validation can live close to the types that consume bytecode and reflection.

Cons:

- RHI would start knowing about render-pass semantics, package ids, hot reload policy, project cooked paths, and diagnostics phrased around passes.
- It would become closer to Unreal's RenderCore/Renderer layer than to Unreal's RHI layer.
- Adding material shaders later would pressure RHI to understand high-level material and scene policy.
- It makes RHI harder to keep as a small hardware/API interface.

Conclusion: RHI should own the low-level shader contract and backend translation, not the render-system policy.

### Why Not Put All Shader Runtime In Renderer?

Pros:

- Shader use is a renderer feature, and pass intent is easiest to understand near the pass.
- Startup, reload, diagnostics, and fallback policy are renderer-facing workflows.
- It keeps RHI smaller from the application point of view.

Cons:

- Renderer would need to understand DXIL/SPIR-V selection, reflection records, root signature requirements, and backend binding rules.
- D3D12/Vulkan details could leak upward through package validation and PSO assembly.
- Multiple backend implementations would become harder to keep consistent.
- It weakens the boundary that prevents compiler/backend implementation details from leaking into high-level code.

Conclusion: Renderer should own orchestration and policy, but not API-specific bytecode translation or native pipeline creation.

### Recommended Sparkle Split

| Responsibility | Recommended owner | Reason |
| --- | --- | --- |
| DXC/Slang invocation, reflection extraction, package writing | `Tools/ShaderCompiler` | This is offline tooling, not runtime. |
| Typed shader declarations while renderer-agnostic | Shared shader authoring contract, currently under RHI public shader headers | These declarations describe shader interface facts needed by both cook and runtime. They must not include pass execution policy. |
| Render pass package selection and PSO intent | Renderer | This is rendering feature logic. |
| Package path locating and reload policy | Renderer-private shader runtime facade | Cooked asset location, old-package retention, pass diagnostics, and editor reload handoff are renderer/application policy. |
| Package file format, bytecode records, reflection records, layout hash validation | RHI shader contract | These records are consumed by backend binding layout and PSO creation and must stay backend-neutral. |
| DXIL/SPIR-V selection for active API | RHI query plus Renderer facade coordination | RHI knows the required binary format; Renderer facade asks for it and requests the right package record. |
| Root signature/binding layout compilation | RHI backend | This is API-specific hardware binding work. |
| Graphics/compute PSO creation | RHI backend | Native pipeline objects are RHI objects. |

Short-term, `CookedShaderPackageCache` can remain in RHI because it already owns strict validation and loaded bytecode records. The review-grade target should split its mixed responsibilities more clearly: keep the binary reader/validator in RHI, and let the Renderer-private facade own package request policy, path locating, reload sequencing, and pass-facing diagnostics. That preserves Sparkle's current safety while aligning the architecture more closely with Unreal/Donut/Cauldron boundaries.

## Boundary Decision

### Add Renderer-Private Shader Runtime Facade

Create a small Renderer-private owner for package request policy, package locating, generated layout lookup, binding layout compile requests, PSO creation requests, and reload coordination.

Reasons:

- Keeps RHI/package boundaries intact.
- Reduces `RenderPassPipelineTraits` to declarations and pass-specific PSO state.
- Keeps pass names, cooked asset paths, reload behavior, and diagnostics out of RHI over time.
- Centralizes runtime diagnostics for startup and reload failures.
- Makes future variant and binary-format selection easier to review.
- Matches Donut's shader-factory lesson without moving compiler work into runtime.
- Matches Cauldron's separation of parameter/pipeline objects while staying in Sparkle's module model.

Constraints:

- Adds one new abstraction.
- Requires careful naming so it does not become a generic rendering god object.
- Needs a focused migration to avoid compatibility shims lingering.

Decision: implement `RenderPassShaderRuntime` as the narrow Renderer-private facade. Do not keep the current pass-trait orchestration as a parallel active architecture once the facade is in place.

## Key Design Decisions

### Decision 1: Runtime Is Strictly Cooked

Runtime startup and reload consume `.sparkshader` packages only. Runtime must not include or link DXC, Slang, SPIRV-Reflect, `ShaderCompileOptions`, or `ShaderCompileResult`.

Rationale:

- Predictable runtime behavior.
- Clean portfolio boundary.
- Faster failure on bad cook state.
- Easier CI validation.

### Decision 2: Editor Recook Is External Tooling

The editor may launch `ShaderCompiler.exe`, but only the Application/editor recook bridge can know about the tool executable. Renderer and RHI reload validated cooked packages only.

Rationale:

- Supports development hot reload without contaminating runtime modules.
- Keeps production runtime behavior the same as cooked startup.

### Decision 3: Keep Cook Execution Single-Threaded

The cook graph remains serial for now.

Rationale:

- The current priority is correctness, reviewability, and boundaries.
- Parallelism adds scheduling and cache complexity that does not improve the architecture story yet.

### Decision 4: Treat DXC and Slang as Peer Backends

Both backends can produce DXIL and SPIR-V. Backend identity and backend version are part of cache and package metadata.

Rationale:

- Future-proofs shader language and target choices.
- Keeps compiler implementation replaceable.
- Makes backend differences inspectable.

### Decision 5: Binding Layout Hash Remains the Compatibility Authority

The runtime already validates `BindingLayoutHash` against the generated layout. Do not add a duplicate layout version field unless serialized compatibility genuinely changes.

Rationale:

- Avoids redundant identity systems.
- Keeps package validation grounded in actual layout content.

### Decision 6: Small Typed Permutations First

Implement a compact UE-like permutation domain for global/render-pass shaders only. Defer material shader permutations.

Rationale:

- Gives Sparkle the architectural shape needed for growth.
- Avoids material-system scope explosion.

### Decision 7: Use Readable Cooked Shader Package Extensions

Cooked shader package files should use `.sparkshader` instead of `.sshd`.

Rationale:

- `.sparkshader` is self-describing in logs, package inspection, and portfolio review.
- `.sshd` looks like an SSH daemon file and does not communicate shader-package intent.
- The filename extension is a tooling and UX concern; the binary magic/version remains the authoritative compatibility check inside the package.
- The implementation touchpoint should be centralized in `Paths::CookedShaderPackage`, not repeated across compiler and runtime code.

## Data Flow

### Cook Flow

1. `GlobalShaderRegistry` exposes typed shader registrations.
2. `ShaderCookPlanner` groups registrations by package.
3. `ShaderPackageLayoutBuilder` builds a deterministic `PassParameterLayout`.
4. Future permutation support enumerates stable variant IDs.
5. `ShaderCookGraphBuilder` creates one node per package/stage/target/backend work item.
6. `ShaderCookGraphExecutor` executes nodes serially.
7. `ShaderBackendPool` resolves `dxc`, `slang`, or automatic backend selection.
8. Backend compiles to DXIL or SPIR-V.
9. Backend extracts backend-neutral reflection.
10. `ShaderParameterStructVerifier` validates reflected fields.
11. `CookedPackageWriter` emits records, bytecode, reflection, hashes, backend identity, and target format.
12. `CookedRegistryWriter` emits package registry metadata.

### Runtime Flow

1. Renderer pass declares package ID, variant ID, binding layout ID, expected stages, and PSO state.
2. Renderer shader runtime facade builds the expected package layout from typed metadata.
3. Facade resolves the cooked package request and reload policy for the pass.
4. Facade asks RHI package code to read and validate a package with the required binary format for the active RHI.
5. RHI package code validates package identity, layout hash, source identity, stage mask, bytecode hashes, and required format.
6. RHI backend compiles the native binding layout/root signature from cooked binding and reflection records.
7. RHI creates graphics or compute PSO from the loaded bytecode.
8. Facade returns pass runtime storage to `PipelineStateManager`.
9. Pass execution binds parameters through existing pass binder paths.

### Editor Recook Flow

1. Editor or source change tracker requests recook.
2. `ShaderRecookCoordinator` launches `ShaderCompiler.exe` externally.
3. Tool writes cooked packages and a recook completion signal.
4. Coordinator verifies successful process completion and signal freshness.
5. Renderer waits for RHI idle.
6. Runtime clears/reloads cooked package state.
7. If validation fails, old packages remain active and error diagnostics are shown.

## Required Changes

### Phase 1: Architecture Document and Invariants

- Land this design document.
- Add a short link from `Scripts/README.md` or future top-level docs index if a docs tree becomes the standard location.
- Keep the no-runtime-compiler invariant explicit in boundary validation comments.

Acceptance:

- A reviewer can understand current state, target state, non-goals, and tradeoffs from one document.

### Phase 2: Boundary Validation Audit

- Re-audit `ValidateShaderCompilerBoundary.cmake` against current backend files.
- Keep `ShaderCompiler.exe` exception limited to Application shader recook files.
- Ensure Slang and DXC tokens remain backend-local.
- Consider adding stale manual-layout tokens to validation after migration.

Acceptance:

- Build fails if compiler implementation details leak into runtime or orchestration.

### Phase 3: Reflection as a Required Contract

- Make typed reflected fields fail cook when backend reflection is unavailable or incomplete.
- Keep `Reflected=false` as the path for pass IO and layout-only abstractions.
- Improve diagnostics with package, variant, stage, backend, target, field, reflected kind, and declared kind.

Acceptance:

- A shader with missing reflected metadata fails before package emission.
- `cook --verification-self-test parameter-mismatch` remains a reliable negative test.

### Phase 4: Include and Cache Correctness

- Make include closure failures explicit.
- Add an inspect or debug output mode that reports dependency hash, settings hash, backend identity, and target.
- Ensure cache keys include all compile-affecting backend and target inputs.

Acceptance:

- A missing include cannot silently produce a stale cache hit.
- Debug artifacts explain why a shader did or did not recook.

### Phase 5: Multi-Format Package Strategy

- Rename cooked shader package files from `.sshd` to `.sparkshader` through the centralized `Paths::CookedShaderPackage` helper, then recook packages.
- Allow one logical package to contain DXIL and SPIR-V records for the same package/stage/variant.
- Extend cook settings from a single target to a target set when needed.
- Keep runtime selection through `CookedShaderBinaryFormat requiredBinaryFormat`.
- Keep the package model flexible enough for future ray tracing library records, not only graphics/compute stage records.
- Bump package and registry versions only if serialized compatibility changes.

Acceptance:

- Cooked shader package filenames are readable in cooked output directories, logs, and package inspection paths.
- A package can satisfy DX12 with DXIL and future Vulkan with SPIR-V without recooking over the same logical package.

### Phase 6: Ray Tracing Shader And Inline Ray Query Readiness

- Add a package kind concept so graphics, compute, and ray tracing library packages can validate differently.
- Extend shader identity with RT export kinds or RT stages: ray generation, miss, closest-hit, any-hit, intersection, and callable.
- Add inline ray-query feature metadata for ordinary graphics/compute shaders that use `RayQuery` or Vulkan ray query capabilities.
- Add an acceleration-structure binding semantic/domain that can be used by both RT libraries and inline ray-query shaders.
- Add backend capability reporting for ray tracing library support and inline ray-query support per target/backend.
- Teach DXC/Slang profile selection to handle DXIL RT libraries, SPIR-V ray tracing targets, and regular graphics/compute profiles that require inline ray queries.
- Add cooked export records and hit-group records with stable names and hashes.
- Allow one RT package to gather exports from separate ray generation, miss, closest-hit, any-hit, intersection, and callable source files.
- Keep global/pass parameter layouts separate from future local RT parameter layouts.
- Extend `inspect-package` to print RT package kind, exports, hit groups, local parameter layout metadata, payload/attribute metadata, inline ray-query feature flags, acceleration-structure bindings, backend, target, and hashes.

Acceptance:

- The tool can describe a ray tracing shader library package with ray generation, miss, closest-hit, any-hit, intersection, and callable exports without adding runtime compilation or pass-local file probing.
- The `HelloRayTracingLibrary` example can keep each RT shader type in a separate file while producing one inspected package with coherent export and hit-group metadata.
- The tool can describe a normal graphics/compute package that uses inline ray queries without pretending it is a ray tracing library package.
- A reviewer can inspect the package metadata needed for future DXR/Vulkan RT state object creation.
- A reviewer can inspect inline ray-query feature requirements and acceleration-structure bindings before runtime support lands.
- The RHI ray tracing implementation can consume the package contract later without inventing a second shader artifact format.

### Phase 7: Small Typed Permutation Model

- Give permutation dimensions stable define names and values.
- Generate stable variant IDs from permutation vectors.
- Add compile defines from the selected vector.
- Make `list-permutations` display actual vectors and keys.
- Make `inspect-shader` show the domain in reviewer-friendly terms.

Acceptance:

- A global/render-pass shader can declare a small permutation domain.
- The cook can enumerate and emit stable variants.
- Runtime can request a typed `TShaderRef` with a permutation key.

### Phase 8: Renderer-Private Shader Runtime Facade

- Extract runtime orchestration out of `RenderPassPipelineTraits`.
- Suggested name: `RenderPassShaderRuntime`.
- Inputs: pass name, package definition, expected stages, pipeline kind, PSO state hooks.
- Outputs: binding layout, pipeline state, loaded package pointer/reference, diagnostics.
- Move or inject package path locating so pass/reload policy lives in the Renderer facade instead of being hardwired into RHI package cache internals.
- Keep RHI package binary parsing/validation and backend root signature creation in RHI.

Acceptance:

- Pass traits declare shader package intent and pass-specific PSO state only.
- Startup and reload use the same facade path.
- RHI does not need pass names, cooked asset path policy, editor reload policy, or old-package retention decisions.
- Diagnostics have one place to explain package/runtime assembly failures.

### Phase 9: Editor Recook and Hot Reload Hardening

- Replace or harden `recook.signal` with atomic publication and stale-signal detection.
- Keep previous valid packages active when recook fails.
- Reload only after process success, signal freshness, and RHI idle.

Acceptance:

- Editor recook cannot accidentally reload half-written packages.
- Failure messages explain whether the compiler failed, signal was stale, or runtime validation rejected the package.

### Phase 10: Reviewer Diagnostics and CI

- Extend `inspect-package` output with backend, target/format, reflection counts, layout hash, source identity hash, and variant hash.
- Include ray tracing package kind, exports, hit groups, payload/attribute metadata, inline ray-query feature flags, and acceleration-structure bindings once Phase 6 lands.
- Extend `inspect-shader` and `list-permutations` for typed permutation visibility.
- Extend CI with cold-cache cook, backend/target smoke checks, parameter mismatch negative test, and package inspection proof.

Acceptance:

- CI proves shader cook health and boundary health.
- A reviewer can inspect one package and understand why runtime accepts it.

### Phase 11: Cleanup

- Run a duplicate-concept audit for shader identity, package layout, package loading, reflection validation, backend selection, debug artifact writing, and runtime reload policy.
- Merge or delete tiny forwarding files that do not own lifecycle, policy, diagnostics, or a dependency boundary.
- Remove stale manifest terminology from docs and comments where typed registrations are now authoritative.
- Remove manual layout helper paths once generated layouts fully own cook and runtime.
- Remove temporary compatibility wrappers after migration.
- Keep each public shader header justified as a module contract; move implementation-only types private when possible.
- Keep deferred Vulkan/material/full-RHI-ray-tracing notes labeled as deferred work.

Acceptance:

- No old architecture path appears as an active implementation path.
- Search results do not imply runtime shader compilation or manual package layouts are still supported.
- A reviewer can navigate the active shader path without finding duplicate owners for the same responsibility.
- Any remaining helper/manager/service type has a concrete ownership reason documented by its name, API, or nearby architecture notes.

## Cleanup Inventory

### Cleanup Required Soon

- Tighten reflection failure behavior in backend/cook flow.
- Harden include closure diagnostics.
- Improve `list-permutations` beyond dimension counts.
- Improve package inspection output.
- Consolidate runtime package assembly behind the new facade.
- Harden recook signal handling.
- Identify duplicate or obsolete shader package layout/loading paths before adding RT package records.

### Cleanup After Migration

- Remove manual built-in package layout helper files and includes if any remain.
- Remove stale manifest/cook wording from comments and scripts.
- Add validation tokens to prevent old manual helpers returning.
- Ensure docs describe implemented state, not aspirational state.
- Collapse files that became one-line wrappers after the facade/package split.
- Move implementation-only shader runtime types out of public headers where no external module consumes them.

### Deferred Cleanup

- Material shader architecture.
- Vulkan RHI pipeline implementation.
- Ray tracing/work graph package schema expansion.
- Advanced shader cache eviction policy.
- Parallel cook execution.

## Risks and Mitigations

| Risk | Mitigation |
| --- | --- |
| Shader runtime facade becomes too broad | Keep it Renderer-private and focused on package-to-runtime assembly only. |
| File count grows faster than clarity | Require every new file to own lifecycle, policy, diagnostics, or a dependency boundary; delete or merge old paths in the same phase. |
| Multi-format packages force schema churn | Version package and registry together only when serialization changes. |
| Permutations expand package count too quickly | Start with explicit small domains and reviewer-visible enumeration. |
| Reflection differs between DXC and Slang | Make backend/target part of diagnostics and CI smoke tests. |
| Editor recook reloads stale or partial output | Use atomic publication, signal freshness, process result checks, and RHI idle before reload. |
| Runtime accepts stale layouts | Keep `BindingLayoutHash` validation mandatory. |

## Validation Plan

Use these commands as the eventual acceptance surface. Paths assume a built workspace from the repository root unless noted.

```powershell
Scripts\BuildProject.bat Showcase Editor Debug
```

```powershell
build\bin\Debug\ShaderCompiler.exe list-backends
build\bin\Debug\ShaderCompiler.exe list-targets
```

```powershell
Push-Location Projects\Showcase
..\..\build\bin\Debug\ShaderCompiler.exe list-shaders --validate
..\..\build\bin\Debug\ShaderCompiler.exe list-permutations ForwardOpaque
..\..\build\bin\Debug\ShaderCompiler.exe cook --no-cache --backend dxc --target DxilSm66
..\..\build\bin\Debug\ShaderCompiler.exe cook --no-cache --backend dxc --target SpirV16
..\..\build\bin\Debug\ShaderCompiler.exe cook --no-cache --backend slang --target DxilSm66
..\..\build\bin\Debug\ShaderCompiler.exe cook --no-cache --backend slang --target SpirV16
..\..\build\bin\Debug\ShaderCompiler.exe cook --verification-self-test parameter-mismatch
Pop-Location
```

```powershell
Scripts\CI\RunShaderCompilerCookCheck.ps1
```

Expected results after the planned work:

- Boundary validators pass.
- `list-backends` reports DXC and Slang clearly.
- `list-targets` reports DXIL and SPIR-V targets.
- `list-shaders --validate` reports typed registrations.
- `list-permutations` reports actual stable vectors.
- Cold-cache cooks succeed for available backend/target pairs.
- Parameter mismatch self-test fails with a clear diagnostic.
- Package inspection shows package identity, backend identity, binary format, and reflection counts.
- Editor startup rejects missing/stale packages loudly.
- Editor recook reloads only validated cooked packages after RHI idle.

## Open Questions

1. Facade name: `RenderPassShaderRuntime`, `RenderShaderRuntime`, or `ShaderPipelineRuntime`?
2. Multi-format package UX: should `cook --target` accept repeated values, comma lists, or a named profile such as `--target-profile dx12-vulkan`?
3. Permutation naming: should variant IDs be human-readable, hash-based, or both?
4. Recook signal shape: atomic JSON result file, monotonic stamp file, or process-result-only reload?
5. Documentation placement: keep this under `docs/architecture/`, or create a broader docs index as the repo documentation surface grows?

## Recommended Next Step

Start with Phase 2 and Phase 3: boundary validation audit plus reflection-as-contract hardening. Those are the highest confidence changes because they preserve the current architecture, improve correctness, and create immediate reviewer value without expanding runtime scope.