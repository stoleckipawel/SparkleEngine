# Shader System Delivery Plan

Status: implementation plan; includes the migration ledger but is not architecture authority or proof of completion

Responsibility: own the ordered cross-module shader, graphics-pipeline, and ray-tracing migration and its clean-break validation sequence

Architecture authority: [Shader System Architecture](../../Architecture/CrossModule/ShaderSystem.md)

Migration provenance: [Shader System Migration Baseline](../../Research/ShaderSystemMigrationBaseline.md)

## Purpose And Authority

This document owns the ordered shader, graphics-pipeline, and ray-tracing migration phases, their clean-break boundaries, review gates, and final validation sequence. It does not redefine the enduring architecture or turn a source-consistency checkpoint into executable evidence.

## Implementation Contract

The unified shader and ray-tracing migration is an ordered clean break, not a menu. Each phase is one manually reviewed changelist-sized checkpoint on `master`; none may leave two authoring, parameter, lookup, cook, runtime, capability, pipeline, table, graph, or effect-selection authorities active together. A difficult consumer blocks its owning phase rather than justifying an alias, adapter, wrapper, disabled placeholder, or cleanup ticket.

### Common phase delivery contract

Every implementation prompt and every acceptance-criteria list below inherits this contract. Phase-specific references are additive; they never replace the repository process or review authorities.

Mandatory references for every phase:

- [Documentation authority](../../README.md)
- [Change Integration](../../Engineering/Workflow/ChangeIntegration.md)
- [Change Lifecycle](../../Engineering/Workflow/ChangeLifecycle.md)
- [SparkleEngine Code Review](../../Engineering/Workflow/CodeReview.md)
- [Coding Style](../../Engineering/Foundations/CodeStyle.md)
- [Repository Structure and Ownership](../../Engineering/Foundations/ModuleOwnership.md)
- [Data-Oriented Design](../../Engineering/Foundations/DataAndMemory.md)
- [Naming and Vocabulary](../../Engineering/Foundations/Naming.md)
- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [Whole Repository Architecture Map](../../Architecture/WholeRepositoryMap.md)
- [Ray-tracing target architecture](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md)
- [External Renderer Repository Comparison](../../Research/RendererRepositories.md)

Before editing, the implementer must record the phase outcome, current authority being replaced or extended, mutable and lifetime owners, producer-to-product-to-consumer route, build/generated-artifact membership, copy and complexity budget, performance classification, selected standards/workload gates, exact rejected-name search set, semantic-equivalent search set, and unrelated dirty-path exclusions. The inventory must walk definitions to all uses and representative uses back to their owner; a name-only list is insufficient.

During implementation, complete the real production route before calling the target present. Update every owned producer, consumer, constructor, reset/reload/retirement path, include, filename, build entry, generated schema/artifact, diagnostic, tool/frontend model, and current document in the phase that replaces the contract. Inspect the scoped diff after each coherent batch. Any old-to-new converter, legacy overload, alias, fallback reader, dual writer, feature flag, parallel registry/cache/generation, copied schema, forwarding facade, or renamed equivalent is a failed clean break, not a temporary convenience.

Every phase closes with one evidence table mapping each AC to its cheapest claim-falsifying check, exact command or inspection route, result, and any unavailable evidence. Acceptance requires all of the following:

- prove the target is reachable through the intended production owner and is consumed by the real downstream path; a new definition, isolated fixture, dead registration, or test-only route does not count;
- prove the replaced path cannot still produce, load, publish, select, execute, or present a result, using exact rejected-name searches plus semantic searches for equivalent fields, adapters, aliases, fallbacks, duplicated layouts, alternate generated formats, parallel directories, and stale build/tool/document consumers;
- classify every touched site as authority, composition, producer, consumer, or duplicate, and leave one mutable authority, one lifetime/generation authority, and one production path for each responsibility;
- account for every permanent type, wrapper, field, log, diagnostic, setting, and file added; delete temporary instrumentation, fault injection, local harnesses, reports, and unauthorized test scaffolding before handoff;
- run pinned no-write formatting where applicable, `git diff --check`, local-link and file/include/CMake inventory checks, and `architecture_boundary_check` whenever the Renderer/RHI boundary changes;
- apply the [Code Review](../../Engineering/Workflow/CodeReview.md) procedure to the final scoped diff. A phase is `PASS` only when it has no P0-P2 finding and all evidence authorized for that phase is present; otherwise report `BLOCKED` and do not describe the phase as complete.

Phases 0-11 must not claim compile, runtime, backend, capture, or performance success. Phase 12 must include negative and corruption cases that would fail if the new authority, validation, lifetime, or selection route were bypassed; a happy-path launch alone is not proof.

### Common rules for every phase

- Work directly in the unstaged `master` worktree. Do not create or switch branches and do not stage, commit, push, or submit. The user owns every source-control action.
- Apply the [common phase delivery contract](#common-phase-delivery-contract), [Change Integration](../../Engineering/Workflow/ChangeIntegration.md), [Change Lifecycle](../../Engineering/Workflow/ChangeLifecycle.md), and applicable Engineering guidance. This document controls shader-specific vocabulary and ordering.
- Phase 0 is documentation-only. Phases 1 through 11 use static/source-consistency checks and do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. They update existing validation consumers and record exact deferred oracles without executing them. Phase 12 performs the single final regeneration and all focused-to-broad executable validation for the complete candidate.
- Preserve one `SparkleTasks` runtime, one out-of-process cooker, one transactional publication route, one active renderer shader generation, and all-queue `RhiSubmissionToken` retirement.
- Preserve `PassCommandContext` as command/declared-resource/diagnostic infrastructure only. Pass recording performs no file I/O, compilation, shader-map/library lookup, layout creation, pipeline creation, or hidden resource discovery.
- Do not add permutations, `ShouldPrecachePermutation`, pipeline precaching/prewarming, preload/readiness/streaming controls, native driver caches, or a universal authored shader-program layer. Full RT execution is delivered only through the focused composition, RHI, backend, graph, scene, and effect owners frozen here.
- Do not encode classic/partitioned or descriptor/device-address selection in shader class names, HLSL root filenames, authored defines, effect uniforms, or graph call-site mode parameters. One semantic AS parameter is lowered by private RHI.
- Treat every consumed render product as mandatory unless the owning architecture names a real alternate algorithm. Never add a clear/copy/no-op/dummy pass merely to satisfy graph production or make missing work look successful. Shadow visibility selects exactly one real inline-query or pipeline producer before graph construction and fails when neither is available; frame orchestration does not duplicate shader/runtime/RHI mechanism checks.
- Do not add one-field carriers, broad context/service/resource bags, a second catalog/map/runtime-generation owner, permanent migration diagnostics, per-job logging, a compiler-result browser, report generators, feature flags, compatibility formats, or submitted test scaffolding.
- Update definitions, consumers, filenames, includes, CMake/source groups, CLI/help/autocomplete, editor models, diagnostics, and current documentation in the phase that owns their replacement. Record obsolete disposable generated/cooked outputs immediately; Phase 12 is the sole phase that deletes and regenerates them after the source floor is clean.
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
| RT stage composition | `RayTracingPipelineComposition` with typed shader refs, hit groups, ray-generation-derived shared ABI, and optional bounded local data | Renderer semantic effect/shader owner; never used for one-shader compute or ordinary graphics |
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
- [Change Integration clean-break policy](../../Engineering/Workflow/ChangeIntegration.md#current-clean-break-policy)
- [Coding Style one-field types](../../Engineering/Foundations/CodeStyle.md#one-field-types)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [Ray-tracing target architecture](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md)
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
- Freeze `RayTracingGBuffer` as the first parity effect; define the effect-level portability boundary, shared scene/TLAS/material/output authority, payload/attribute/miss/ray-flag contract, requested-versus-active mode semantics, explicit raster alternative, and the instance/geometry/ray-type SBT formula. Do not treat an arbitrary compute entry point as interchangeable with an RT stage.
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

- [Repository Structure and Ownership](../../Engineering/Foundations/ModuleOwnership.md)
- [Data-Oriented Design identity rules](../../Engineering/Foundations/DataAndMemory.md#identity-and-references)
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
- Keep PTLAS unavailable unless its complete descriptor capability, layout, write, resource resolution, and ray-query chain is valid. Phase 1 removes the dead address variant without claiming PTLAS runtime proof; Phase 6 owns complete source delivery and Phase 12 owns paired executable backend validation before the provider can be accepted.
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

- [Coding Style](../../Engineering/Foundations/CodeStyle.md)
- [Naming and Vocabulary](../../Engineering/Foundations/Naming.md)
- [Data-Oriented Design single truth](../../Engineering/Foundations/DataAndMemory.md#single-truth-and-copy-budget)
- [Ray-tracing target shader and composition contract](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#shader-authoring-and-pipeline-composition)
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

This table records the Phase 2 source-consistency checkpoint now present in committed `master`. It does not claim compilation, shader recording, backend execution, cook, runtime, capture, or performance evidence.

| AC / claim | Cheapest claim-falsifying check or inspection route | Result |
| --- | --- | --- |
| Direct compute authoring is class + nested `Parameters` + implementation + typed dispatch, with no authored package/program/pass metadata at the graph call | Trace `DirectLightingCS` from `DirectLighting.h` through `DirectLightingShaders.cpp` to `AddDirectLightingPass`; exact searches for `DirectLightingProgram`, `TShaderProgram`, and `SPARKLE_RENDER_PASS` | `PASS`: the graph caller allocates and dispatches `DirectLightingCS` directly; rejected program/pass spellings have zero runtime/build hits. The package macro remains confined to the registration implementation until Phase 4. |
| Every authored shader closes definition -> parameters -> registration -> real graph consumer | Extract `class X final : public GlobalShader<X>`, `IMPLEMENT_GLOBAL_SHADER*`, `Dispatch*<X>`, and `Draw<VS, PS>` sets from Renderer headers/sources and compare both directions | `PASS`: 27 shader classes, 27 nested `Parameters` schemas, 27 registrations, and 27 consumed shader types close exactly; 25 are compute dispatch shaders and two are the GBuffer vertex/pixel pair. |
| Replaced frontend, definition bags, count-only acceptance, duplicate schemas, and forwarding pass owners are absent | Exact runtime/build `rg` floor for `TGlobalShader`, `TShaderRef`, nested `FParameters`, unused `FGlobalShader`/`FComputeShader`/`FRay*Shader`, `RenderPassDefinition*`, `RenderPassShaderRuntimeDesc`, `ComputePassOperations`, `RasterPassOperations`, `GetDefinition`, `GetParameterMetadata`, and `*PassParameters`; inspect `PassParameterLayout::Matches` and `PassBinder` | `PASS`: zero rejected-name definitions or uses; layout compatibility compares the complete structural signature rather than parameter count; phase-owned one-shader pass files are deleted. |
| Shader-visible fields have one schema and the graph and binder consume it | Trace macro registration -> `ShaderParameterStructBuilder` metadata -> `TypedPassParameterInstance` -> `SetupShaderParameters` -> `PassBinder`; inspect GBuffer composition and per-draw overrides | `PASS`: compute shaders use their nested schema directly. GBuffer composes `GBufferVS::Parameters` and `GBufferPS::Parameters`; its seven attachment fields are graph-only, while the two mesh-cache SRVs remain declared once in `GBufferVS::Parameters` and are supplied by the retained mesh collaborator. |
| Every parameter has one C++/metadata/HLSL binding identity | Exact runtime/tool searches for `SHADER_PARAMETER_*_NAMED`, `SPARKLE_REGISTER_NAMED_GRAPH_SHADER_PARAMETER`, parameter-field `LayoutName`/`ShaderName`, `GetLayoutName`, parameter `GetShaderName`, `SetShaderName`, and dual-name reflection fallback; compare representative cbuffer, SRV, and UAV declarations with HLSL | `PASS`: 103 authored named overrides and their three macro definitions are deleted. Parameter descriptors and pass layouts carry only `Name`; graph registration, package layout, cook verification, reflection validation, cooked validation, and runtime binding consume it directly. |
| Resource and attachment vocabulary has one canonical author route | Exact searches for `builder.Read`, typed `FrameGraphBuilder`/`FrameGraph` texture/buffer `Read` overloads, `CreateRTV`, and `CreateDSV`; count canonical calls beneath `Renderer/Private/Passes` | `PASS`: rejected author-facing spellings are zero. Current pass sources contain 178 `CreateSRV`, 49 `CreateUAV`, seven `CreateAccelerationStructureBinding`, six `CreateRenderTarget`, and two `CreateDepthTarget` calls. Generic `PassResourceBuilder::Read(handle, usage, label)` remains private graph-compiler declaration infrastructure, not an author-facing view alias. |
| Graphics setup names concrete stages and retains only real draw behavior | Trace `Draw<GBufferVS, GBufferPS>` through `FrameGraphBuilder`, graph attachment derivation, `GBufferMeshPass` / `GBufferMeshBatchDrawer`, `GpuMesh`, `RenderPassRuntimeCache`, and paired RHI consumers | `PASS` as a Phase 5 source claim: authors provide narrow raster intent; graph attachments, prepared mesh/material work, typed shader resolution, and the generation owner each contribute their facts once. The complete descriptor is private to materialization/RHI and recording performs lookup/bind/draw only. Phase 12 owns compilation and paired runtime proof. |
| Recording context remains semantic-free and package/runtime mechanics stay behind graph/runtime owners | Inspect `PassCommandContext`, `FrameGraphBuilder`, and `RenderPassRuntimeCache`; search pass sources for package/runtime lookup | `PASS`: the command context contains only commands, diagnostics, and declared-resource resolution. Typed graph helpers own current package-backed materialization; feature callers do not select packages, layouts, or pipelines. |
| Source/build/docs hygiene and mandatory source-only review | Recursive Renderer CMake source-glob inspection; pinned `clang-format 22.1.3 --dry-run --Werror --style=file` over changed C/C++; local Markdown link resolution; `cmake -DSPARKLE_REPO_ROOT=... -P CMake/ArchitectureBoundaryCheck.cmake`; `git diff --check`; Code Review procedure | `PASS`: recursive build membership covers the moved/deleted files, pinned no-write formatting, local links, the architecture boundary check, and whitespace validation pass. Final source-only review reports no P0-P2 finding. No executable check is claimed. |

#### CL boundary

Suggested title: `Renderer: make shader classes drive typed graph dispatch`.

### Phase 3 - Establish reproducible compile jobs and changed dependencies

#### Implementation prompt

> Implement Phase 3 as one compiler-job, dependency-selection, and replay-diagnostics CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including producer/consumer closure, semantic legacy eradication, and the mandatory Code Review gate. Replace package-shaped cook nodes with immutable one-variant shader compile requests/jobs/results keyed by compiler-affecting input, persist virtual dependencies, make Changed select the exact reverse closure, and delete old node identities and full-catalog Changed behavior. Preserve one cooker, `SparkleTasks`, and the compile-every-selected-input rule. Do not add permutations, persistent compiler-result storage, workers, or executable checks.

#### Phase-specific references

- [Tasks Engineering](../../Engineering/Modules/Tasks.md)
- [Editor Engineering](../../Engineering/Modules/Editor.md) and [Tools Engineering](../../Engineering/Modules/Tools.md)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
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
- Source-level routes and existing validation consumers are updated for duplicate fan-out, repeated-operation recompilation, cancellation settlement, and exact changed-dependency selection; Phase 12 owns their executable proof.
- Static production traces connect changed virtual paths through reverse-dependency selection, immutable requests/jobs, `SparkleTasks`, result fan-out, and transactional publication; no old cook node, full-catalog fallback, cache-like store, alternate worker path, or presentation-owned compile policy remains reachable.
- The common phase evidence table and source-only Code Review gate report `PASS` with no P0-P2 finding; the behavioral claims above remain Phase 12 executable obligations rather than being inferred from source shape.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### Phase 3 source-consistency evidence

This table records the Phase 3 source-consistency checkpoint now present in committed `master`. It does not claim compilation, cooking, cancellation execution, backend output, runtime activation, capture, or performance evidence. Phase 12 retains the executable obligations named below.

| AC / claim | Cheapest claim-falsifying check or inspection route | Result |
| --- | --- | --- |
| Old compile-node and compile-option authorities are absent | Exact runtime/build search for `CookNode`, `ShaderCookNode*`, `ShaderCompileOptions*`, `ShaderContractJobIdentity`, `ShaderCookPlanExecutor`, `FormatNodeContext`, `BuildCompileOptions`, `optionsHash`, and old node member access | `PASS`: the rejected definitions, includes, builders, executor, member accesses, and semantic option-hash residue have zero runtime/build hits; the defining files are deleted. Historical names remain only in this plan's comparison, ledger, prompt, and search floor. |
| One typed compile identity covers every compiler-affecting input without package or checkout identity | Trace `ShaderContractCatalogBuilder` -> `ShaderCompileJobBuilder` -> `ShaderSourcePreprocessor` / `IncludeClosureHasher` / `ShaderCompileRequestHasher` -> `ShaderCompileInputHash`; inspect every appended field and every excluded field | `PASS`: stable `ShaderTypeId` plus `Target` identify the logical result; the request captures the exact preprocessed compiler source after verifying a stable virtual dependency closure. That source hash, length-delimited virtual identity/closure, entry, stage, unit kind, target/profile, required features, compile policy, ABI remaps, backend identity/version, and unambiguous hash domains feed the input hash. Backends and replay consume the captured bytes. Physical roots, package/program/pass labels, basenames, presentation text, and analysis-output policy do not. Hash collisions between shader type names fail during catalog construction. |
| Changed intent selects the verified virtual reverse closure rather than the full catalog | Trace `ShaderSourceChangeTracker::CollectChangedVirtualPaths` -> `ShaderRecookRequest::ChangedVirtualPaths` -> repeated `--changed` arguments -> `ShaderDependencyManifest::ReadRequired` -> `SelectAffectedShaderTypes` -> `ShaderCookPlanner`; inspect missing/corrupt/incomplete paths | `PASS`: additions, modifications, and deletions produce sorted virtual paths; persisted forward and reverse records must agree; valid metadata selects only dependent shader types plus the still-atomic multi-stage physical publication group. Missing, corrupt, or partial metadata fails loudly with an explicit global-rebuild instruction; there is no valid-metadata full-catalog fallback. |
| Every selected input compiles; exact duplicates fan out only within the active operation | Trace `ShaderCookPlanBuilder` -> `ShaderCompileBatch::SelectProducers` / `CompileProducers` / `FanOutResults` / `FinalizeResults` -> package consumers | `PASS`: one logical job exists per selected shader type and target. Exact same-input jobs share one producer inside the local batch only; results are copied back in deterministic job order, and every logical result independently runs parameter-ABI verification and opt-in artifact publication. No result survives the operation. |
| Failure/cancellation cannot publish a partial generation | Trace the Application stop token and private cancellation signal through the child process; trace batch task failure through `GlobalShaderCooker` to `ShaderArtifactPublication`; inspect staged validation and `TryPublishFileSet` membership | `PASS` as a current source-consistency claim: cancellation is observed before publication and every compile/ABI/map validation exception returns before the transactional switch. Map, library, dependency manifest, and recook signal are staged and published as one file set only after every logical result finalizes; cancellation after commit begins loses to that coherent publication rather than interrupting it. Phase 12 executes cancellation and failed-job nonpublication oracles. |
| Replay diagnostics are bounded and successful diagnostics stay opt-in | Inspect `ShaderCompileFailureReplay`, `ShaderCompileBatch`, `ShaderCompileJobExecutor`, and `ShaderDebugArtifactWriter`; search for per-job log/store/retry surfaces | `PASS`: the deterministic first failed producer or first logical ABI failure overwrites one `LastShaderCompileFailure.json`; diagnostic and preprocessed-source payloads are capped, while identity, policy, hashes, dependencies, defines, and binding remaps remain reproducible. Successful bundles require the existing explicit debug-artifact directory. No retry, accumulated failure directory, or per-job log stream exists. |
| Existing validation consumers encode the deferred behavioral oracles | Inspect `ValidateShaderCompilerCli.cmake` and Phase 12 executable requirements | `PASS`: the focused validation route describes a full dependency-producing cook followed by changed-path selection and two identical shader-target operations before artifact inspection. Phase 12 owns any temporary local harness needed to prove producer fan-out, actual repeated compiler invocation, exact closure, cancellation, failed publication, and portable hashes; none is inferred as executed here, and no test-only scaffold is submitted in Phase 3. |
| Source/build/docs hygiene and mandatory source-only review | Recursive ShaderCompiler/Application CMake-glob inspection; pinned `clang-format 22.1.3 --dry-run --Werror --style=file` over changed C/C++; local Markdown link resolution; exact include/file inventory; `git diff --check`; Code Review procedure | `PASS`: recursive build membership covers every added/deleted source and header, explicit CLI validation inputs and help are reconciled, local links and whitespace checks pass, and the final source-only review reports no P0-P2 finding. `architecture_boundary_check` is not selected because this phase changes no Renderer/RHI dependency boundary. No executable check is claimed. |

#### CL boundary

Suggested title: `Shaders: establish compile jobs and dependency-directed cooking`.

### Phase 4 - Replace cooked packages with the global shader map

#### Implementation prompt

> Implement Phase 4 as one cooked-map, code-library, typed-lookup, and generation-lifetime CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including corruption-sensitive authority/lifetime checks and the mandatory Code Review gate. Replace per-package cooked authority with `GlobalShaderMap` and content-addressed `CookedShaderLibrary` records, make `ShaderRef<Shader>` resolve through the active map, preserve `RenderPassRuntimeCache` as the sole generation/materialization owner, and delete every old package identity/file/reader/writer/cache/path/schema dispatch and package-generation spelling. Do not keep an adapter. Update the focused ShaderCompiler/map/library validation consumers and record the exact Phase 12 oracles, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence.

#### Phase-specific references

- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [Epic `FGlobalShaderMap`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FGlobalShaderMap)
- [Epic `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)

#### Required work

- Generate deterministic `GlobalShaderMap` entries for `(ShaderTypeId, Target)` containing `ShaderCodeHash`, parameter signature, stage/feature/runtime metadata, and provenance joins.
- Generate `CookedShaderLibrary` records keyed by exact `ShaderCodeHash`; deduplicate validated identical code bytes without conflating incompatible metadata.
- Open/validate map and library once per replacement generation. `ShaderRef<Shader>` resolves the active target entry; graph/runtime never computes a file path or package ID.
- Refactor `RenderPassRuntimeCache::ShaderRuntimeGeneration` to own the opened map/library and generation-bound materialized layouts/pipelines. Keep lazy graph-construction materialization and all-queue retirement.
- Delete `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `BuildShaderPackageIdFromSourcePath`, `CookedShaderPackage*`, `LoadedShaderPackage`, package cache/identity/path/schema-version dispatch, `.sparkshader` readers/writers, and package generations. Record obsolete ignored generated outputs for Phase 12 deletion and sole regeneration; no source or runtime reader may retain them as compatibility input.
- Delete the old package-shaped RT export/hit-group/local-parameter/payload/attribute/recursion records, package inspection, and deliberate package-runtime rejection rather than copying them into an unconsumed map schema. Phase 6 owns the final RT records and their complete execution consumer.
- Update Application/Editor/CLI package targets, publication readers/payloads, model fields, help/autocomplete, diagnostics, and artifact-path consumers to typed shader/map/library vocabulary in the same CL. Phase 10 may simplify their workflow but cannot finish package removal.
- Keep neutral code/map validation records in RHI public, generation policy in Renderer, generation output in ShaderCompiler, and backend object creation in backend-private RHI.
- Preserve complete replacement validation, rollback on failure, renderer shader-generation capture in `RenderFrameIdentity`, view-history invalidation, and submission-token retirement.
- Update validation consumers for the deferred Phase 1-3 claims: canonical virtual resolution, same-basename distinction, collision/traversal/case rejection, checkout-independent dependency identity, representative compute/graphics nested-parameter metadata, identical in-operation fan-out, repeated-operation recompilation, exact changed-dependency closure, cancellation settlement, failed-job nonpublication, and portable input hashes. Phase 12 executes these oracles; this phase proves only their source reachability and fixes owning code rather than weakening an oracle or adding a fallback.

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
- Static production traces connect canonical virtual resolution and rejection rules, relocation-stable dependency identity, and representative compute/graphics nested parameter metadata to the final map/library signature; no physical-path fallback or duplicate parameter authority satisfies the check. Phase 12 owns executable proof.
- Existing validation consumers encode the deferred Phase 3 fan-out, recompilation, dependency, cancellation, failure-publication, and checkout-independent hash oracles against the new request/job/map production route rather than an isolated helper. Phase 12 owns execution.
- Invalid replacement preserves the current generation; retired map/library/layout/pipeline state remains until all recorded submissions complete.
- RHI remains Renderer-independent; map/library/runtime owners have no duplicated identity or lifetime state.
- Source inspection proves missing/incorrect code hash, duplicate logical key, incompatible parameter signature, truncated code record, stale generation, and invalid replacement are rejected at their narrow owners and preserve replacement-before-switch ordering. Phase 12 executes corruption and lifetime cases that would fail if lookup or validation bypassed the new map/library route.
- Bidirectional production traces and semantic searches prove map/library lookup is the only runtime route and package behavior has not survived behind renamed records, compatibility readers, generated extensions, editor fields, or publication payloads.
- The common phase evidence table and source-only Code Review gate report `PASS` with no P0-P2 finding; no executable evidence is claimed.
- Includes/CMake/generated policy/docs reconcile and `git diff --check` passes.
- Exact searches prove the old RT package metadata/rejection path is gone; RT runtime support remains honestly absent until Phase 6 delivers the whole replacement.

#### Phase 4 source-consistency evidence

This table records the Phase 4 source-consistency checkpoint now present in committed `master`. Per the plan-wide delivery rule, it does not claim configuration, compilation, shader compilation, cooking, launch, test, capture, runtime, or performance success. Phase 12 owns every executable and generated-artifact oracle.

| AC / claim | Cheapest claim-falsifying inspection route | Result |
| --- | --- | --- |
| Package authority is unreachable | Exact case-insensitive runtime/tool/build/Application/Editor search for the Phase 4 package type, file, request, CLI, extension, cache, identity, generation, and path floor; file/include inventory for deleted definitions | `PASS` as a source claim: the rejected floor has zero non-document, non-generated hits; old defining files and registration constants are deleted. Historical names remain only in explicitly historical inventory, rationale, prompts, and search floors. |
| One generated map and code-library production route exists | Trace `GlobalShaderCooker` -> compile results -> `ShaderArtifactPublication` -> staged map/library validation -> one `TryPublishFileSet`; inspect logical-key ordering, exact code-hash deduplication, catalog reconciliation, empty-code rejection, and temporary-file cleanup | `PASS` as a source claim: `(ShaderTypeId, Target)` entries are deterministically ordered, identical bytecode shares one exact hash record, collisions fail, and map/library share one publication hash. Dependency metadata must cover exactly the validated current catalog and has no persisted partial/complete mode. A selected cook carries forward only entries still present in that catalog; removed registrations are pruned from dependency metadata, the map, and therefore the code library, including a deletion-only cook with no compile jobs. A partial file-set publication is unreachable. Phase 12 cooks, deletes a registration in an isolated fixture, and corrupts artifacts. |
| Every published entry comes from one validated catalog/job contract | Trace full-catalog validation in `ShaderCookPlanBuilder`, immutable job/result identity checks in `GlobalShaderCooker`, product/catalog checks in `ShaderArtifactAssembly::FromProduct`, staged `GlobalShaderMap::Open`, and runtime registration validation | `PASS` as a source claim: missing/duplicate catalog identities fail before job construction; result type, target, and input hash must match the immutable job; source, entry, stage, target format, features, parameter signature, backend provenance, and exact bytecode hash must match before serialization. The loader rejects unknown feature bits, empty required metadata, invalid target/stage/key/signature/hash, missing code, and truncated ranges before a generation can switch. |
| Typed lookup has no file-path or package bypass | Trace concrete `GlobalShader` registration -> `ShaderTypeId` -> `ShaderRef<Shader>::Resolve` -> `ResolvedShader` -> binding layout and D3D12/Vulkan pipeline consumers; reverse-search artifact opens and runtime path helpers | `PASS` as a source claim: graph/runtime callers hold typed references, RHI consumes validated resolved records, and only the Renderer generation owner opens active paths. Inspection tooling and the cooker are the other intentional artifact readers. |
| Renderer owns one live generation and retirement route | Trace `RenderPassRuntimeCache::OpenGeneration`, lazy runtime holders, replacement construction, switch ordering, `CaptureLastSubmittedState`, and `PollRetiredGenerations` across every queue | `PASS` as a source claim: the replacement map/library and all already-materialized layouts/pipelines validate before the active pointer switches; failure leaves it unchanged; retired generation-owned objects remain until every captured submission completes. Phase 12 executes invalid replacement and delayed-completion cases. |
| Frontend and tool vocabulary is package-free | Inspect Application publication/request handling, Editor model/table/action, CLI parser/help/inspection, launcher/AssetCooker messages, recook signal, and current architecture routes | `PASS` as a source claim: semantic shader targeting and map/library publication replace package selection and inspection; no package field or compatibility conversion remains. Phase 10 still owns the broader `Apply Changed` workflow simplification. |
| Existing validation consumer encodes the deferred source/cook oracles | Inspect `ValidateShaderCompilerCli.cmake` against the Phase 1-4 acceptance floor without executing it | `PASS` as a source claim: the existing validation target now asserts a non-empty full map/library, typed DXIL/SPIR-V lookup with code/input/parameter hashes, exact representative Changed selection, two independently scheduled repeated operations, byte-identical deterministic republication, cancellation settlement, traversal rejection, unknown-shader rejection, and unchanged artifacts after every rejected operation. Phase 12 executes it and supplies the documented isolated temporary fixtures needed for same-basename/collision/case/relocation, true producer fan-out, compiler failure, corruption, stale-registration deletion, and delayed GPU retirement; no permanent test executable or runtime instrumentation is added here. |
| One-field and bloat audit | Search newly retained records and changed one-field structs; inspect diagnostics and registration/build glue | `PASS`: the one-field `ShaderContractCatalog` wrapper was replaced by a direct vector alias; no one-property validity carrier, package adapter, cache browser, migration logger, or permanent instrumentation was introduced. Type-only shader DSL tags remain intentional compile-time vocabulary rather than data carriers. |
| Generated outputs and executable proof | Read-only ignored-artifact inventory; Phase 12 delivery contract | `DEFERRED`: 56 old `.sparkshader` files and two old registries remain disposable ignored outputs. No production reader can consume them. Phase 12 deletes them before the one current map/library regeneration and runs every compiler/cook/load/corruption/lifetime oracle. |
| Source/build/docs hygiene and review | Recursive source-membership inspection, exact stale include/file searches, pinned `clang-format 22.1.3 --dry-run --Werror --style=file`, local Markdown-link resolution, `architecture_boundary_check`, `git diff --check`, branch/staging inspection, and Code Review procedure | `PASS` as a source-only review: recursive membership covers new/deleted files, explicit CLI validation paths use map/library vocabulary, current architecture routes are reconciled, pinned no-write formatting passes, local links resolve, the static architecture boundary check reports no new violation, the worktree is `master`, the index is empty, whitespace validation passes, and no P0-P2 source finding remains. No compilation, cook, test, or runtime check is claimed. |

#### CL boundary

Suggested title: `Shaders: replace cooked packages with the global shader map`.

### Phase 5 - Split raster intent, attachment compatibility, and graphics pipeline materialization

#### Implementation prompt

> Implement Phase 5 as one graphics-state ownership and paired-backend vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including bidirectional state-authority traces, corruption-sensitive key inspection, paired D3D12/Vulkan consumer closure, and the mandatory source-only Code Review gate. Replace the caller-authored `GraphicsShaderPipelineState` and eager variant bundle with narrow granular `RasterPassRenderState`, graph-derived attachment compatibility, mesh/material-owned geometry/raster facts, and one complete internal immutable `GraphicsPipelineKey`/`GraphicsPipelineDesc` materialization path. Update the GBuffer draw route end to end and delete every old state field, variant pointer, duplicate target/topology operation, and compatibility overload in this CL. Update the Phase 12 validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Do not add PSO precaching/prewarming, a renamed aggregate, Unreal-scale mesh-command caching, or a second pipeline system; do not stage, commit, push, or submit.

#### Phase-specific references

- [Graphics Pipeline Policy](../../Engineering/Modules/Renderer.md#graphics-pipeline-policy)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [Data-Oriented Design single truth](../../Engineering/Foundations/DataAndMemory.md#single-truth-and-copy-budget)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
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
- Static producer/consumer traces prove the base GBuffer requests only its exact key and that real wireframe/two-sided variants can be requested only through their owning policy; Phase 12 measures exact materialization and reuse. No result is described as precaching.
- D3D12 and Vulkan source consumers account for every neutral attachment, blend/color-write, depth/stencil, cull/fill, topology, vertex-input, and sample field or reject it explicitly. Phase 12 owns paired runs/captures and native-object provenance.
- Graph execution begins/ends the render pass and applies attachment actions exactly once; GBuffer code contains no manual bind/clear duplication, and command recording creates no pipeline.
- Bidirectional owner traces prove feature code cannot reach the complete descriptor, backends cannot invent Renderer semantic policy, and no old aggregate/variant path survives under an alias, overload, default, diagnostic helper, test consumer, or generated/build entry.
- Static architecture-boundary inspection, no-write formatting, includes/CMake/docs, `git diff --check`, and the source-only Code Review gate pass with no P0-P2 finding. No executable or precache/performance claim is made.

#### Phase 5 source-consistency evidence

This table records the Phase 5 source-consistency checkpoint now present in committed `master`. It does not claim configuration, compilation, shader compilation, cooking, launch, test, capture, backend runtime, or performance success. Phase 12 owns those executable obligations.

| Former fact | One current owner | Disposition |
| --- | --- | --- |
| shader stages, code generation, binding layout | typed `ShaderRef<GBufferVS>` / `ShaderRef<GBufferPS>` resolved by the active runtime generation | code hashes, combined parameter signature, and generation contribute to `GraphicsPipelineKey`; labels do not |
| blend and depth/stencil policy | granular `RasterPassRenderState` operations in raster pass setup | copied once into the internal request/key/descriptor; no attachment, mesh, shader, graph, or backend field is expressible |
| color/depth handles, format/count/sample compatibility, load/store/clear/access | typed frame-graph attachment bindings plus registered `FrameGraphTextureDesc` | compatibility alone enters the key; actions/access remain graph execution and validation facts |
| vertex declaration, front-face convention, depth clipping, topology, streams, index format, and draw counts | `GpuMesh` selected into `GBufferMeshBatchDrawer::PreparedDraw` | setup derives the complete request once, resolves it to generation-owned runtime references, and stores only those references plus material bindings, mesh reference, and draw arguments until recording consumes and clears the current list |
| cull and fill policy | prepared material double-sided state plus the explicit GBuffer wireframe view policy | exact solid/back, solid/two-sided, or wireframe/no-cull requests are collected; no speculative combination is created |
| viewport and scissor | current `RenderView`, copied once into the GBuffer frame input for the parallel-recording boundary | consumed by `GBufferMeshPass::PrepareRasterPass` as dynamic command state; absent from key and descriptor identity |
| complete graphics identity and materialized pipeline lifetime | `RenderPassRuntimeCache` active/replacement/retired generation | one immutable key, one exact pipeline map, replacement-before-switch recreation, and all-queue retirement |
| neutral descriptor validity and native state lowering | public RHI `PixelFormat` traits plus `ValidateGraphicsPipelineDesc` / `ValidateComputePipelineDesc`; D3D12/Vulkan private consumers | format classification, required stages/layouts, attachment/count/sample, vertex-input, depth/stencil, and color-write invariants are defined once; both backends call that authority before retaining only native translation, API/device capability, native construction, and native failure handling |

| AC / claim | Cheapest claim-falsifying inspection route | Result |
| --- | --- | --- |
| Old aggregate and eager variants are unreachable | Exact runtime/build search for `GraphicsShaderPipelineState`, `RasterPassPipelineRuntime`, `RhiVertexLayoutKind`, the eager wireframe/two-sided members, old depth/stencil records, old target fields, and deleted backend static-layout files | `PASS` as a source claim: definitions, consumers, includes, and files are removed rather than aliased or wrapped. |
| Graph attachments are the sole target/action authority | Trace the seven GBuffer attachment assignments through `ShaderRenderTarget` / `ShaderDepthTarget`, `PassParameterSet`, graph usage declaration, `FrameGraph::BuildRasterPass`, and `BeginRasterPass` / `EndRasterPass`; reverse-search GBuffer bind/clear calls | `PASS` as a source claim: formats and samples derive from registered texture descriptions, access controls graph usage, incompatible samples/access/store policy fail before recording, and clear/bind/end happen once in graph execution. |
| Prepared mesh/material work owns geometry and raster variants | Trace `GpuMesh` and material/view filtering into `GBufferMeshBatchDrawer::PreparedDraw`, then reverse-trace the recording loop | `PASS` as a source claim: setup performs filtering and raster-policy selection once, materializes each exact request, and stores borrowed runtime/mesh references plus the bounded material binding and draw values needed across the recording boundary. Recording iterates and then clears the current list; it cannot enumerate view batches or select material/fill/cull policy. One mesh topology-setting call remains and no retained mesh-command cache was added. |
| Complete key and exact lazy materialization have one generation owner | Inspect every `GraphicsPipelineKey` field and explicit hash contribution, `FrameGraph::PreparePasses` ordering after current scene/view application and graph compilation but before parallel recording, live-pass filtering, exact cache lookup while publishing prepared draws, direct prepared-runtime consumption during drawing, reload replacement, and retired generation lifetime | `PASS` as a source claim: generation, shader code, binding signature, blend, raster, depth/stencil, topology, active vertex-input entries, and active attachment formats/count/depth/sample affect equality/hash. Unused fixed-capacity entries, labels, viewport/scissor, streams, and draw arguments do not. Culled passes request nothing; missing runtimes/keys and distinct-key hash collisions fail loudly; lookup and materialization are absent from recording callbacks. Phase 12 executes perturbation, collision, stale-generation, exact-count, reuse, and retirement oracles. |
| D3D12/Vulkan lower the same complete neutral descriptor without copied policy | Trace `BuildGraphicsPipelineDesc` through the one public-RHI descriptor validator into both backend constructors; search both private trees for copied neutral format case lists, attachment/count/sample predicates, missing-binding validation, and hard-coded opaque/sample-one/static-mesh substitution | `PASS` as a source claim: public RHI owns pixel-format traits and graphics/compute descriptor validity once. Both backends consume the declared color writes/blend operations, fill/cull/front face/depth clip, depth/stencil, topology, vertex declaration, attachment formats/count, and sample count, while retaining only native mapping/capability/construction failures. The copied backend-local format and descriptor-validation blocks are deleted; paired execution remains Phase 12. |
| Copy, complexity, and performance budget | Inspect retained per-frame products, cache ownership, materialization timing, speculative variant reachability, and recording work | `PASS` as a source-shape claim: one current-frame prepared-draw vector carries borrowed mesh/runtime references plus bounded material and draw values; the generation map is the sole persistent key/pipeline store; culled passes and unrequested raster variants create no pipeline; recording performs no state discovery or pipeline creation. Runtime cost, first-use behavior, exact reuse, memory, and frame-time effects remain unmeasured Phase 12 obligations. |
| Source/build/docs hygiene and review | Recursive CMake membership inspection, exact stale-name and semantic-equivalent searches, pinned no-write formatting, local Markdown-link validation, `architecture_boundary_check`, `git diff --check`, branch/staging inspection, and Code Review procedure | `PASS` as a source-only review: all Phase 5 rejected runtime/build names have zero hits; recursive CMake globs cover the added/deleted files; `clang-format 22.1.3 --dry-run --Werror --style=file` reports 56 checked and zero failures; local links resolve; the architecture boundary check reports no new violation; `git diff --check` exits zero; the branch is `master`; the index is empty; and the final review has no P0-P2 finding. No executable or performance result is inferred. |

#### CL boundary

Suggested title: `Renderer: split raster intent from graphics pipeline materialization`.

### Phase 6 - Deliver the complete paired ray-tracing runtime foundation

#### Implementation prompt

> Implement Phase 6 as one intentionally atomic shader-map-to-GPU ray-tracing vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including complete source-to-GPU reachability, paired negative/corruption oracle coverage, and the mandatory source-only Code Review gate. Extend the lean shader frontend and final `GlobalShaderMap`/`CookedShaderLibrary` representation to all six RT stages, add the focused typed pipeline composition, complete DXIL/SPIR-V compilation and validation paths, clean-break the RHI capability contract, materialize native D3D12/Vulkan pipelines and shader tables, and add typed frame-graph trace execution and runtime-generation lifetime. Update the Phase 12 end-to-end validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Delete the old runtime rejection and ambiguous capability only when the complete replacement source route is present. Do not split this into compiler-only, backend-only, disabled-public-API, or graph-bypass checkpoints; do not stage, commit, push, or submit.

#### Phase-specific references

- [Ray-tracing target architecture](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md)
- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
- [Microsoft DXR functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos Vulkan ray-tracing chapter](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Unreal Engine hardware ray tracing](https://dev.epicgames.com/documentation/unreal-engine/hardware-ray-tracing-in-unreal-engine)
- [Unreal Engine `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Unreal Engine `FRayTracingShaderBindingTableInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRayTracingShaderBindingTableIni-)
- [Unreal Engine `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)

#### Required work

- Add concrete ray-generation, miss, closest-hit, any-hit, intersection, and callable shader classes through the same `GlobalShader`, virtual-source, implementation-registration, catalog, compile-job, map, code-library, provenance, and typed `ShaderRef` path as raster/compute. Ray-generation owns nested global `Parameters`; other stages declare no root-parameter struct or empty parameter carrier. Optional local data uses its focused hit-group/stage record schema rather than a second root contract. No second RT registry or package identity exists.
- Add one focused `RayTracingPipelineComposition` that names typed shader refs, triangle/procedural hit groups, and optional bounded local POD. Derive the global binding layout and shared payload/attribute/recursion ABI from the selected ray-generation shader; local record schemas have one group/stage owner and are absent when unused. The composition is not used for compute or ordinary graphics and does not copy shader declarations, registration strings, stages, or ABI metadata.
- Extend final map/library records and ShaderCompiler validation/inspection for exports, groups, layout hashes, payload/attribute limits, recursion, local records, target/backend/source identity, and deterministic ordering. Prove DXC DXIL and SPIR-V RT-library output before advertising either target; keep Slang RT capability false until its own equivalent conformance passes.
- Update the existing Phase 12 validation route to exercise all six stage kinds, one triangle hit group, one procedural hit group, miss, callable indexing, global bindings, bounded local-record validation, and exact sentinel outputs. Its invalid cases cover duplicate/missing exports, illegal group composition, layout mismatch, malformed local data, payload/attribute/recursion limits, and target capability failure. Do not add or run a temporary harness in this phase.
- Replace ambiguous `SupportsRayTracing` authority with independent acceleration-structure, inline-ray-query, and RT-pipeline readiness across every RHI/Renderer producer and consumer; delete the old field. Readiness requires the complete extension/feature/property/function/backend chain, not an extension bit.
- Complete and prove the one semantic acceleration-structure binding retained by Phase 1. Force classic and partitioned TLAS providers on D3D12/Vulkan where supported; verify the same shader type, source, parameter signature, map entry, and graph call bind the exact backend-native descriptor representation. A provider remains unavailable if its descriptor layout/write/resource-resolution chain is incomplete. Do not revive raw-address shader uniforms, access-mode enums, duplicate shaders, alternate code records, or mutable-descriptor machinery without a separately demonstrated need.
- Add immutable backend-neutral RT pipeline composition descriptors, opaque `RayTracingPipeline` and `RayTracingShaderTable` products with generation identity, a logical table materialization request containing names and bounded POD rather than native identifiers, and `TraceRaysDesc` with raygen/miss/hit/callable regions and dimensions.
- Extend existing RHI pipeline/device/command owners. Define global/local binding semantics, resource states, tracking, legal queues, checked 64-bit size/index arithmetic, region bounds, failure reporting, and `TraceRays`; expose no D3D12/Vulkan handle, identifier bytes, or native table layout to Renderer.
- D3D12 builds a validated state object, exports/groups/config/associations, queries identifiers from that exact generation, supplies native identifier size/alignment/stride limits to the common checked table packer, allocates the packed raygen/miss/hit/callable bytes, binds with `SetPipelineState1`, and dispatches through the neutral command path.
- Vulkan validates every dependent extension/feature/property/function pointer, builds stages/groups/pipeline/layout, queries group handles from that exact generation, supplies device identifier size/alignment/stride limits to the same checked table packer, allocates the device-addressable result, and dispatches through `vkCmdTraceRaysKHR` via the neutral command path.
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

- Static bidirectional traces connect all six RT stage kinds through compile, validation, final map/library publication, typed resolution, legal composition, materialization, graph execution, reload, and retirement owners. Phase 12 owns paired D3D12/Vulkan execution and capture evidence.
- Exact searches return zero old package RT records/readers/rejection, ambiguous `SupportsRayTracing`, duplicate RT registry, native trace call outside backend-private RHI, graph bypass, disabled public RT facade, and stale-generation acceptance.
- Invalid export/group/layout/local-record/payload/attribute/recursion/capability/table arithmetic/queue/generation inputs fail before unsafe native execution with one bounded owner-local diagnostic.
- D3D12/Vulkan native validation is clean; table addresses, region sizes, strides, alignments, counts, and logical indices agree with the neutral oracle; sentinels prove raygen, miss, triangle hit, procedural intersection/hit, any-hit, and callable execution.
- Forced classic/partitioned provider evidence shows one shader/map/graph identity and the correct native AS descriptor type/write on each supported backend; exact searches keep every Phase 1 duplicate/access-mode spelling at zero.
- Runtime support becomes true only through complete readiness; map/library/pipeline/table generations publish and retire atomically through the existing submission-token owner.
- A source-to-GPU trace for every stage and a reverse trace from each native dispatch/capture event resolve to one shader type, map entry, code record, composition, pipeline generation, table generation, and graph pass; no test-only, direct-native, or disabled facade is the sole consumer of the new contract.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; any missing backend cell, retained bypass, temporary harness artifact, or unproven readiness transition makes the phase `BLOCKED`.
- Scoped no-write formatting, architecture-boundary inspection, build membership, `git diff --check`, and exact deferred Phase 12 evidence paths are reported without an executable claim.

#### Phase 6 source-consistency evidence

This table records the Phase 6 source-consistency checkpoint now present in committed `master`. No configuration, compilation, shader compilation, cook, test, launch, capture, native validation, or performance command was run for that checkpoint.

| AC / claim | Cheapest claim-falsifying inspection route | Result |
| --- | --- | --- |
| One generic six-stage authoring/cook/map route | Trace `ShaderStage`, optional nested `Parameters`, registration metadata, compile-unit selection, DXC library arguments/reflection, final map publication/open, and typed `ShaderRef` resolution | `PASS` as source shape: all six stage kinds share the global-shader catalog/job/map/library path; ray generation alone requires the root parameter schema; RT metadata and local-record contracts publish in the final map. DXIL/SPIR-V output remains unexecuted Phase 12 evidence. |
| One neutral pipeline/table/trace contract with private lowering | Trace `RayTracingPipelineComposition` through `RayTracingPipelineRuntime`, public RHI descriptors/validation, the common-RHI checked table packer, D3D12 state object/identifier/table/dispatch, Vulkan pipeline/group-handle/table/dispatch, and reverse-search native identifiers/calls | `PASS` for the zero-local-data source route: Renderer retains logical typed selections, groups, and generations; D3D12/Vulkan retain native identifier acquisition, alignment limits, allocation/upload, and dispatch calls; one common-RHI owner derives checked record strides, region layout, and byte placement from those backend facts. Native validity remains unexecuted Phase 12 evidence. |
| Portable local-record consumption | Trace a nonzero local schema from shader declaration/reflection through composition, native pipeline association, table bytes, and shader-visible consumption on both APIs | `BLOCKED`: size/signature validation and common-RHI byte packing exist, but D3D12 has no local-root-signature association and Vulkan has no shader-record storage contract. Nonzero local data would be packed but not exposed coherently to shader code. The phase must either implement one referenced cross-backend schema end to end or delete the unsupported nonzero-local-data surface; packing alone is not acceptance. |
| Independent readiness and one semantic AS binding | Exact-search the removed whole-word `SupportsRayTracing` field; trace Vulkan extension/feature/function/property readiness and D3D12 tier readiness; reverse-trace Renderer AS binding and the overall product-support authority | `BLOCKED`: the three RHI mechanism fields are independent and no provider choice or address/access variant enters shader identity, but no Renderer authority combines native mechanism readiness with compiler, final map/library, typed graph, and concrete-effect readiness. A hardware `SupportsRayTracingPipeline` value can become true before this phase's required source-to-typed-graph contract exists. Forced provider and descriptor-write proof also remains Phase 12 work. |
| Typed graph, exact runtime composition, reload, and retirement | Trace `FrameGraphBuilder::TraceRays`, table resource declaration/state, exact-composition cache entries, pre-execute materialization, bound pipeline/table generation validation, replacement generation recreation, and all-queue retirement | `PASS` as source shape: graph execution has one RT pass kind and neutral dispatch; exact compositions coexist under the existing runtime cache; no disk lookup or pipeline/table creation occurs in the recording callback. Execution and delayed-completion proof remains Phase 12 work. |
| Concrete all-stage reachable consumer and reverse trace | Search renderer registrations, HLSL entries, composition construction, and `FrameGraphBuilder::TraceRays` call sites for ray-generation, miss, closest-hit, any-hit, intersection, and callable stages | `BLOCKED`: no existing product/tool route owns all six concrete stages, and this phase forbids both a permanent test-only consumer and Phase 7 product-effect migration. Adding unused fixture classes would leave the disabled-facade checkpoint that this phase explicitly rejects. Phase 6 cannot be called complete until the delivery contract names or authorizes a real consumer boundary. |
| Source/build/docs hygiene and review | Recursive CMake membership, exact rejected-name/native-leak searches, pinned no-write formatting, architecture boundary check, `git diff --check`, branch/index inspection, and Code Review | `BLOCKED` overall because the portable-local-record, concrete-consumer, and overall-readiness findings are P1 source blockers. Recursive globs cover the new files; the pinned formatter is currently blocked by Windows Application Control and no current no-write formatting pass is claimed; the architecture boundary check reports no new violation; `git diff --check` exits zero; the branch is `master`; and the index is empty. No executable result is inferred. |

#### CL boundary

Suggested title: `Shaders: deliver paired ray-tracing pipeline foundation`.

This phase is intentionally larger than an ordinary subsystem CL. Splitting its compiler, public contract, backend, graph, or lifetime portions would create the misleading placeholder states this unified plan forbids.

### Phase 7 - Deliver dual-execution ray-traced GBuffer parity

#### Implementation prompt

> Implement Phase 7 as one product-effect vertical-slice CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including shared-semantics ownership proof, paired parity/alternate-path oracle coverage, and the mandatory source-only Code Review gate. Refactor the existing inline ray-traced GBuffer into shared semantic HLSL plus thin inline and pipeline frontends, add one immutable effect execution plan and strict requested/active mode semantics, and schedule exactly one typed frontend. Update the Phase 12 same-frame parity and explicit-raster validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Use the Phase 6 map, pipeline, table, graph, and lifetime owners directly; add no second scene/material/output or execution system.

#### Phase-specific references

- [Ray-tracing target dual-execution contract](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#effect-level-dual-execution-contract)
- [Ray-tracing target shader/SBT contract](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#pipeline-abi-and-shader-table-contract)
- [Frame graph typed resource precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [NVIDIA NVRHI tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)
- [AMD FidelityFX inline ray-tracing helper at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)

#### Required work

- Freeze field-specific Phase 12 output comparison for base color, normal, material, emissive, subsurface, device Z, motion vector, miss values, face/culling, near/TMin, transforms, indexed/deformed geometry, and resolution bounds using identical immutable inputs in one frame.
- Split current GBuffer HLSL into one owner for ray setup inputs, hit reconstruction, material lookup, barycentrics, transforms, motion/depth conventions, miss encoding, and output stores; keep `RayQuery` candidate traversal and RT stage intrinsics in thin sibling frontends.
- Keep the current compute shader as the inline frontend. Add raygen, miss, and opaque triangle closest-hit shader classes and one focused `RayTracingPipelineComposition`; use one ray type, recursion depth one, global resources, zero local SBT data, and intentionally all-zero SBT contributions.
- Until Phase 8 adds any-hit, derive masked-geometry presence inside `RenderScene` from its authoritative primitives, instance groups, and material table. Refresh the bounded readiness bit only on the exact structural/material events that can change it; do not rescan the scene per frame or let another owner mirror it. Strict Pipeline is unavailable for such a scene and fails at GBuffer graph construction before a frontend is scheduled; Automatic records that reason and selects Inline when it is ready. Never run the opaque pipeline against masked geometry.
- Add `RayTracingExecutionMode::{Automatic,Inline,Pipeline}` in the existing renderer settings vocabulary and one immutable graph-construction effect plan containing only the derived active frontend and reason. The GBuffer owner queries algorithm and execution-mode CVars directly rather than receiving copied settings parameters; capability readiness remains with its capability owners; shader generation remains with frame identity/runtime materialization. `FramePipeline` may retain only the topology it actually built for rebuild detection. Keep the plan separate from `GBufferAlgorithm` selection and delete ambiguous `Raytraced` execution-API wording rather than aliasing it.
- Make strict `Inline`/`Pipeline` requests fail before graph construction when unavailable. `Automatic` chooses an available frontend through one inspectable Renderer policy. Schedule exactly one frontend pass and keep target creation, prepared scene, TLAS, material/geometry data, view, downstream consumers, histories, and the explicit rasterized-GBuffer algorithm unchanged.
- Add bounded active-mode/reason capture markers and provenance joins without copied readiness flags, shader-generation labels, or per-ray/per-record logging.

#### Positive guardrails

- Both frontends consume the same prepared scene/view and write the same authoritative GBuffer attachments in the same frame.
- Exact integer/identity fields compare exactly; floating-point tolerances are field-specific and frozen before results are inspected.
- Rasterized GBuffer remains an explicit supported algorithm, not a substitute that fabricates a ray-traced result; algorithm choice stays separate from execution API.

#### Negative guardrails

- No execution-mode branch inside a low-level pass Execute callback, duplicate target set, second TLAS/material table, duplicated hit reconstruction/output encoding, compatibility enum value, any-hit/procedural/callable/multi-ray-type complexity, or new temporal owner.
- No consecutive-moving-frame comparison, screenshot-only acceptance, hidden fallback for an explicit request, or product capability inferred from the conformance fixture alone.

#### Acceptance criteria

- Existing validation consumers encode the frozen same-frame inline/pipeline GBuffer oracle and paired D3D12/Vulkan hit/miss and geometry/view cases; Phase 12 owns execution and capture.
- Requested intent resolves deterministically to one visible active frontend and reason; strict unsupported requests schedule no partial frontend; `Automatic` records its reason; the explicit raster algorithm remains functional.
- One shared semantic implementation owns hit/material/motion/depth/output work and exact searches find no ambiguous old mode name, duplicate scene/output authority, or two scheduled frontends.
- Both frontend call graphs reach the same semantic hit/material/motion/depth/output functions, and adapter inspection shows only traversal/stage mechanics; a focused semantic perturbation or equivalent fault-sensitive oracle detects either frontend bypassing shared logic before that temporary perturbation is removed.
- Static lifetime traces prove the effect captures the exact map/pipeline/table generation through Phase 6 owners; Phase 12 exercises reload and several frames in flight.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; duplicate outputs, scene/material tables, execution plans, or hidden strict-mode fallback block completion.
- Source-only shader/effect/backend consumer inspection, scoped diff, stale-name searches, and `git diff --check` pass; executable evidence is deferred to Phase 12.

#### CL boundary

Suggested title: `Renderer: deliver dual-execution ray-traced GBuffer`.

#### Phase 7 source-consistency evidence

This table records the Phase 7 source-consistency checkpoint now present in committed `master`. No configuration, build, compiler invocation, shader compilation, cook, test, launch, capture, native validation, or performance command was run for that checkpoint.

| AC / claim | Cheapest claim-falsifying inspection route | Result |
| --- | --- | --- |
| One scene/output/semantic authority and two thin frontends | Reverse-search every GBuffer UAV store, hit reconstruction, material lookup, ray setup, miss encode, `RayQuery`, and `TraceRay` call in the three `RayTracingGBuffer*` shader files | `PASS` as source shape: all fourteen hit/miss attachment stores and the shared reconstruction call live in `RayTracingGBufferCommon.hlsli`; each frontend calls the same `StoreTraceResult`; the adapters contain only bounds plus inline traversal or RT-stage/payload mechanics and contain zero output stores. |
| One typed frontend is scheduled | Exact-search `Dispatch<RayTracingGBufferInlineCS>`, `TraceRays<RayTracingGBufferRGS>`, the execution-plan definition, and GBuffer target creation | `PASS` as source shape: there is one typed call site for each frontend, one defined `RayTracingGBufferExecutionPlan`, one target factory, and one graph-setup switch. There is no execution-mode branch in a recording callback. |
| Strict/Automatic selection is honest | Trace direct GBuffer-owner CVar queries through the plan resolver, scene-owned capability report, RHI AS/descriptor-indexing/Inline/Pipeline capabilities, scene masked-material policy, GBuffer graph-construction failure, and the built-topology rebuild observation | `PASS` as source shape: no GBuffer algorithm, execution mode, or derived plan is forwarded through frame-graph settings or feature-construction parameters; the plan stores only active frontend and reason; strict unsupported requests fail at the GBuffer owner before a frontend is scheduled; Automatic records why it selected one ready frontend; the opaque Pipeline route is rejected for masked geometry until Phase 8 any-hit; raster selection remains independent. Shader generation is not copied into policy or diagnostic labels. Native creation failure remains Phase 12 evidence and fails rather than falling back. |
| One pipeline composition and zero local/index complexity | Inspect the typed composition, shader metadata, HLSL payload, `TraceRay` arguments, table records, and all `TraceRays` consumers | `PASS` as source shape: raygen/miss/opaque closest hit form one triangle group, the payload is 24 bytes, recursion is one, local records are absent, and ray contribution, geometry multiplier, and miss index are all intentionally zero. The composition stores only typed membership and hit-group policy; materialization derives dispatch-wide ABI from the ray-generation registration, and neutral export/group descriptors derive local-record contracts from their resolved shader entries instead of copying those fields. |
| Generation lifetime and bounded capture join | Trace plan generation from `RenderPassRuntimeCache` through graph-settings equality, diagnostic label, exact-composition runtime, native pipeline/table generations, callback capture, reload replacement, and submission-token retirement | `PASS` as source shape: active frontend/reason plus shader generation form one bounded marker; the same generation creates the native pipeline and table; graph replacement follows settings-generation change. Reload/several-frames-in-flight execution remains Phase 12 evidence. |
| Frozen paired parity and alternate oracle | Inspect the Phase 12 same-frame table, cases, tolerances, fault-sensitive perturbations, and explicit Rasterized route | `PASS` as a deferred executable contract: identity/miss/UNorm/normal/emissive/depth/motion tolerances and opaque hit/miss/cull/TMin/transform/deformation/extent cases are fixed before collection. No execution result is claimed. |
| Clean break, membership, links, boundaries, whitespace, and review | Exact runtime/build old-name searches; recursive Renderer/RHI CMake globs; local Markdown-link validation; `cmake -DSPARKLE_REPO_ROOT=... -P CMake/ArchitectureBoundaryCheck.cmake`; `git diff --check`; branch/index inspection; Code Review procedure | `BLOCKED` only on the pinned formatting command: runtime/build searches have zero Phase 7 old-name hits; the typed frontend counts are one each; recursive globs cover the new/deleted files; local links pass; the architecture check reports no new violation; `git diff --check` exits zero; branch is `master`; and the index is empty. The corrective review removed the same-scope runtime redeclaration, moved nontrivial public-header bodies to common RHI source, centralized neutral checked table packing, consolidated Vulkan stage binding state/mechanics, and reduced the allocator diff to its intended capability rename; bounded re-review found no remaining Phase 7 P0-P2 issue. `clang-format 22.1.3` was absent; an isolated exact-version binary was obtained, but Windows Application Control blocked execution before it could inspect or modify any file. |

The Phase 7 source slice is implemented, but its CL gate remains `BLOCKED` until the pinned no-write formatter runs successfully. The whole migration candidate also retains the separately recorded Phase 6 portable-local-record and all-six-stage conformance obligations; Phase 7's deliberately zero-local, three-stage product route neither hides nor claims those later proofs.

### Phase 8 - Add production hit semantics, shadow rays, and scene-to-SBT indexing

#### Implementation prompt

> Implement Phase 8 as one production hit-semantics and nontrivial table-mapping CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including formula-corruption oracle coverage, scene/SBT authority closure, and the mandatory source-only Code Review gate. Add alpha-tested any-hit parity, a second shadow-visibility ray type and dual frontend, and one authoritative instance/geometry/ray-type contribution plan shared by classic and partitioned TLAS construction. Update the Phase 12 validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Preserve the Phase 6 runtime and Phase 7 effect contracts; do not put material data or transient addresses in the SBT.

#### Phase-specific references

- [Ray-tracing target SBT index formula](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#sbt-organization-and-index-formula)
- [Ray-tracing target scene/effect ownership](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#target-ownership)
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
- Expose bounded owner-local table bytes, record counts, build/update time, invalidation reason, and TLAS/BLAS measurements required by Phase 12 without adding permanent per-record logs.

#### Positive guardrails

- Classic and partitioned TLAS, inline traversal, and pipeline trace use one scene/material identity and one logical mapping.
- Corrupt each formula term and table order independently; bounds failures are caught before dispatch or produce the expected sentinel in focused negative validation.
- Preserve GBuffer parity, the explicit raster GBuffer algorithm, mandatory shadow production, and all generation/lifetime invariants.

#### Negative guardrails

- No pointer identity, transient descriptor address, duplicated material payload, per-frame unconditional table/TLAS rebuild, second scene slot allocator, silent alpha-policy drift, or hidden ray-type order.
- No global recursion/stack increase, procedural geometry, callable product dependency, or migration of unrelated ray-query effects.

#### Acceptance criteria

- Existing validation consumers encode paired inline/pipeline alpha-tested GBuffer and shadow-visibility oracles, the explicit raster route, and pre-graph failure when no real shadow producer is available; Phase 12 owns D3D12/Vulkan execution.
- Static checked-arithmetic inspection covers the full index formula, and Phase 12 owns multi-instance, multi-geometry, multi-material, two-ray-type maximum-valid/first-invalid and independent corruption cases.
- Classic/partitioned builders publish one plan; exact searches return zero constant-zero renderer contributions where mapping is required, duplicate table authority, or large/transient local record data.
- Dirty-generation source routes avoid unrelated BLAS/TLAS/table rebuilds and expose bounded bytes/time/reasons for Phase 12 measurement.
- Bidirectional traces from scene primitive/material/ray-type changes to table invalidation and from native table records back to the one Renderer plan prove no effect-local mapping, second slot allocator, or backend-specific logical order can produce a record.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; any corruption case that reaches unsafe dispatch or any unexplained AS/table rebuild blocks completion.
- Source-only effect/scene/backend/lifetime inspection, architecture boundaries, scoped diff, and `git diff --check` pass; executable evidence is deferred to Phase 12.

#### CL boundary

Suggested title: `Renderer: unify production ray hit semantics and SBT indexing`.

#### Phase 8 source-consistency evidence

The 2026-08-28 reconciliation confirms that the Phase 8 source slice is present in the implementation state at `99af6d5b` and remains unchanged in committed `master` at `20814381`. This is a static source/build-membership statement only: this documentation update adds no configure, build, shader compile, cook, launch, test, capture, native validation, parity, reload, or performance result.

| Claim | Current source evidence | Remaining proof boundary |
| --- | --- | --- |
| Product RT stages are reachable from typed registrations. | `RayTracingGBufferShaders.cpp` and `DirectShadowSignalShader.cpp` register the GBuffer and shadow ray-generation, miss, hit, and alpha any-hit stages used by their pipeline definitions. | Phase 12 must cook, materialize, and dispatch both products on D3D12 and Vulkan. |
| Inline and pipeline frontends share effect semantics. | `RayTracingGBufferCommon.hlsli` / `RayTracingGBufferPipeline.hlsl` and `DirectShadowSignalCommon.hlsli` / `DirectShadowSignalPipeline.hlsl` keep alpha/material or visibility decisions below thin traversal adapters. | Phase 12 must demonstrate output parity, edge cases, and mandatory-producer failure. |
| One Renderer plan owns logical scene-to-SBT indexing. | `RayTracingShaderTablePlan` owns the checked `rayContribution + geometryMultiplier * geometryIndex + instanceContribution` mapping and publishes `Surface` and `ShadowVisibility` ray types to both classic and partitioned scene builders. | Phase 12 must exercise bounds, corruption, multi-instance/geometry/material indexing, and both backend table layouts. |
| Table generations participate in graph lifetime. | Frame-graph RT runtimes capture pipeline and scene-table generations; graph rebuild decisions include the table-plan generation and retired graph resources follow submitted GPU completion. | Phase 12 must prove reload, device recreation, delayed completion, and no stale table/pipeline pairing. |

### Phase 9 - Migrate eligible effects and deliver one whole-frame execution plan

#### Implementation prompt

> Implement Phase 9 as one whole-frame ray-tracing selection and eligible-effect migration CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including whole-frame selection reachability, strict-mode negative-oracle coverage, and the mandatory source-only Code Review gate. Classify every current ray-query effect, migrate only effects with a coherent pipeline design and accepted parity/quality oracle, resolve one immutable strict/automatic execution plan before graph construction, preserve all temporal and supported-alternate ownership, and delete deep feature-specific API selection. Update the Phase 12 validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Do not require a mega-pipeline or force unsuitable effects into pipeline mode.

#### Phase-specific references

- [Ray-tracing target selection semantics](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#selection-semantics)
- [Ray-tracing target shared HLSL boundary](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#shared-hlsl-boundary)
- [NVIDIA RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT)
- [AMD Cauldron ray-tracing capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [Debug View Presentation Contract](../../Architecture/Modules/Engine/Renderer/DebugViewPresentation.md)

#### Required work

- Inventory every selected ray-query GBuffer, direct/indirect/reference/ReSTIR/shadow effect and classify it `Dual`, `InlineOnly`, `PipelineOnly`, or `SupportedAlternate` with owner, reason, shared semantics, output/history contract, readiness, and accepted oracle. `SupportedAlternate` must be a real algorithm with its own contract and evidence; a dummy product is unclassifiable.
- Resolve global requested `Inline`, `Pipeline`, or `Automatic` once before frame-graph construction into an immutable per-effect plan. Strict modes list every incompatible selected effect and schedule no partial frame. `Automatic` may mix modes but records capability, availability, measured policy input, active choice, and reason.
- Migrate only effects with a useful typed composition and accepted parity/quality route. Share ray setup, hit/material/light/BSDF/output semantics; keep `RayQuery` and RT stage intrinsics in thin frontends; schedule exactly one frontend per effect.
- Keep algorithm selections such as Reference/ReSTIR and GBuffer method independent from execution API. Rename UI, settings, captures, and diagnostics that conflate them; keep the removed `CanUseInlineRayQueryShadows` spelling and equivalent deep capability-query policy at zero.
- Preserve each effect's existing outputs, accumulation/denoiser/history invalidation, scene/TLAS/material authority, supported alternate algorithms, and generation reload. Shadow visibility remains mandatory and has no no-ray alternate. Encode Phase 12 mode-transition and reload cases across several effects sharing map/pipeline/table generations.
- Document honestly any retained single-mode effect and why; full-pipeline availability is not a requirement to migrate an effect with no demonstrated benefit.

#### Positive guardrails

- Selection is Renderer policy, resolved once, stable for the frame, and visible in capture/evidence metadata.
- Effects may share shader code, map records, pipelines, or table generations only through their owning immutable caches and complete keys.
- Supported-alternate and temporal behavior remain effect-owned rather than copied into the execution planner.

#### Negative guardrails

- No global mega-pipeline, vendor-ID heuristic without measured evidence, hidden per-pass substitution, fabricated product, partial strict frame, duplicated history, second execution settings tree, or claim that every shader can switch invocation APIs.
- No migration merely to achieve stage/API coverage; conformance and product value remain separate claims.

#### Acceptance criteria

- Every current ray-query effect has one explicit classification and owner; every migrated effect has one paired D3D12/Vulkan correctness/quality/history/supported-alternate/reload oracle assigned to Phase 12.
- Strict modes preflight the whole selected frame and produce one actionable incompatibility result without scheduling; `Automatic` is inspectable and deterministic for identical inputs.
- Exactly one frontend is scheduled per selected effect, algorithm and execution axes are independent, and exact searches find no deep API selection, ambiguous old labels, duplicate plan/settings/history, fabricated product, or silent substitution.
- Static ownership and capture traces preserve shared pipeline/table generation lifetime across multi-effect reload and several frames in flight; Phase 12 executes the lifetime case.
- Phase 12 validation consumers inject one incompatible effect, missing mandatory producer, stale generation, and unavailable supported alternate to prove strict planning rejects atomically while `Automatic` records one deterministic real implementation; removing or bypassing the central plan must make these checks fail.
- The common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding; an unclassified effect, per-pass mode branch, copied history owner, or second settings/plan tree blocks completion.
- Source-only whole-frame plan/effect/backend inspection, scoped diff, stale-name audit, and `git diff --check` pass; executable evidence is deferred to Phase 12.

#### CL boundary

Suggested title: `Renderer: deliver whole-frame ray execution planning`.

### Phase 10 - Deliver Apply Changed and one shader-to-GPU provenance trace

#### Implementation prompt

> Implement Phase 10 as one Application/Editor shader-workflow and provenance CL directly in the unstaged `master` worktree. Apply the [common phase delivery contract](#common-phase-delivery-contract), including intent-to-owner trace proof, frontend implementation-detail audit, and the mandatory source-only Code Review gate. Starting from the package-free vocabulary and complete raster/compute/RT runtime delivered by Phases 4-9, replace parallel recook/reload controls, artifact-directory scans, and the implementation-record table with one semantic `Apply Changed` workflow, immutable operation/catalog read models, automatic activation after renderer validation, contextual expert inspection, and one provenance trace from shader/effect identity through map, native pipeline/table generation, graph event, and capture. Delete manual normal-path reload, duplicate status formatting, and obsolete presentation fields. Update the Phase 12 workflow validation route, but do not configure, build, compile shaders, cook, launch, run tests, capture, or collect performance evidence. Do not add another panel, cache browser, log stream, permutation UI, backend-control surface, or executable-bypass path.

#### Phase-specific references

- [Editor intent-first workflows](../../Engineering/Modules/Editor.md#intent-first-frontend-workflows)
- [Validation logging and instrumentation](../../Engineering/Verification/ValidationAndEvidence.md#logging)
- [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [PIX shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [Ray-tracing target diagnostics and selection](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md#effect-level-dual-execution-contract)

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
- Existing Phase 12 validation consumers encode success, compile failure, validation failure, cancellation, and stale-result cases that prove one terminal operation result and preservation of the accepted generation; the source-only common phase evidence table and Code Review gate report `PASS` with no P0-P2 finding.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shader Tools: deliver Apply Changed and shader-to-GPU provenance`.

### Phase 11 - Eradicate legacy and compatibility surfaces

#### Implementation prompt

> Implement Phase 11 as one adversarial legacy-eradication and ownership-closure CL directly in the unstaged `master` worktree after Phases 0-10 are complete. Apply the [common phase delivery contract](#common-phase-delivery-contract), including repository-wide exact and semantic-equivalent searches, bidirectional owner traces, generated/build/frontend inspection, and the mandatory Code Review gate. Delete every residual legacy, compatibility, duplicate, bypass, fallback, eager-variant, diagnostic-scaffold, and renamed-equivalent surface from the shader, graphics-pipeline, ray-tracing, cook, runtime, graph, and Shader Tools migration. Fix each finding at its current owning responsibility and update all consumers in the same CL. Do not use this phase to defer deletions assigned to earlier phases, introduce a third design, or preserve an old path because final validation has not run; do not stage, commit, push, or submit.

#### Phase-specific references

- [Change Integration clean-break policy](../../Engineering/Workflow/ChangeIntegration.md#current-clean-break-policy)
- [Code Review](../../Engineering/Workflow/CodeReview.md)
- [Repository Structure and Ownership](../../Engineering/Foundations/ModuleOwnership.md)
- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)

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

- [Change Lifecycle review and acceptance](../../Engineering/Workflow/ChangeLifecycle.md#review-and-acceptance)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Renderer/RHI boundary enforcement](../../Architecture/Decisions/RendererRhiBoundary.md#enforcement)
- [Bistro and San Miguel workloads](../../Acceptance/GraphicsWorkloads.md)
- [Ray-tracing target completion contract](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md)
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
- Execute the frozen Phase 7 oracle below by constructing the Inline and Pipeline graphs from one immutable prepared scene/view/frame identity, retaining both output sets for comparison in that same frame, and then running the explicit Rasterized algorithm as a separate supported-route check. The validation owner may retain comparison resources locally; production graph construction must still schedule exactly one selected ray-tracing frontend and must not gain a dual-output mode.
- The all-six-stage RT conformance route must use one registered ray-generation class with global parameters, at least one miss and callable record, one triangle group with closest-hit and any-hit, and one procedural group with closest-hit, any-hit, and intersection. The same logical composition, record indices, bounded local POD, dispatch dimensions, and exact sentinels run on D3D12 and Vulkan. Independent invalid cases must reject duplicate/missing/wrong-stage exports, illegal triangle/procedural groups, root-layout mismatch, local size/signature mismatch, malformed record bytes, payload/attribute/recursion excess, unsupported compiler/backend target, overflow/misalignment/out-of-range table regions, illegal queue, stale pipeline/table generation, and a dispatch whose exact pipeline was not bound. Each oracle must demonstrably reach its intended owner rather than failing earlier for an unrelated reason.
- Run graphics-key perturbation and corruption checks that independently vary generation, each shader code hash, binding-layout signature, every blend/color-write, raster, depth/stencil, topology, vertex-input, color/depth format, attachment count, and sample fact; prove each pipeline-affecting mutation changes equality/hash and exact materialization, while labels, viewport/scissor, attachment actions/access, streams, and draw arguments do not. Force a hash collision, missing pre-materialized key, stale generation, incompatible attachment access/sample, unsupported backend mapping, solid/two-sided/wireframe request set, repeated compatible draws, and reload with delayed queue completion.
- Freeze hardware, adapter, driver, API, build, scene, camera, settings, warm-up, sample count, percentile, comparison tolerance, and failure protocol before collection. Force unsupported capability, missing target/export/group, pipeline creation, SBT allocation/alignment/index, stale generation, device loss/recreation, shader reload, unavailable supported alternatives, and missing mandatory producers.
- Capture identical inline/pipeline inputs in PIX where applicable, RenderDoc where supported, and Nsight/vendor tooling when it supplies causal evidence. Mark effect, active mode/reason, shader/code identity, native pipeline generation, table generation, logical record counts/bytes, and dispatch dimensions without per-ray logging.
- Measure compile queue/wall/CPU time, compiler-session memory, selected/compiled job counts, generated/cooked bytes, map/library open time, graphics/compute/RT pipeline creation, table build/update/bytes, TLAS/BLAS work, cold/warm frame impact, CPU/GPU effect time, p50/p95/p99 frame time, memory high-water, reload overlap, explicit alternate selections, mandatory-producer failures, and generation retirement. Do not add precache/readiness metrics for a system not implemented.
- Perform a final diagnostic/code-structure audit: no migration log stream, per-job spam, default report files, cache browser, submitted test scaffold, god orchestrator/folder/function, forwarding wrapper, or duplicated validation/policy remains.
- Recheck `master`, empty staged diff, unrelated dirty exclusions, generated/cooked source-control policy, and exact diff boundaries. Leave all work unstaged.

#### Frozen Phase 7 same-frame GBuffer oracle

The oracle is fixed before images are collected. It runs on D3D12 and Vulkan with identical source revision, cooked generation, scene, camera matrices/jitter/history, render extent, TLAS, instance/geometry/material buffers, texture table, and output formats. It includes sky-only miss pixels, front-facing and back-facing opaque triangles, near-plane/TMin boundary cases on either side of `0.001`, non-square and non-multiple-of-eight extents, indexed geometry, rigid motion, skinned deformation, and morph deformation. The Phase 8 extension below adds the alpha/cutout, shadow, and two-ray-type cases without weakening these comparisons.

| Compared field | Frozen comparison |
| --- | --- |
| hit/miss, instance id, primitive id, face/cull result, resolution bounds and untouched guard texels | exact integer/identity equality; any out-of-bounds write fails |
| miss BaseColor, Normal, Material, Emissive, Subsurface, DeviceZ, MotionVector | exact stored-value equality to the shared `GBufferPacking::PackSky*` values and zero motion |
| BaseColor, Material, Subsurface (`R8G8B8A8_UNorm`) | each stored channel differs by at most one 8-bit code value (`1/255`) |
| Normal (`R16G16B16A16_Float`) | decoded unit-vector angular error at most `0.1` degree; alpha is exactly zero |
| Emissive (`R16G16B16A16_Float`) | per-channel absolute error at most `0.002 * max(1, abs(reference))`; alpha is exactly zero |
| DeviceZ (`R32_Float`) | absolute error at most `1e-5`; sky is exactly zero |
| MotionVector (`R16G16_Float`) | per-component absolute error at most `0.02` viewport pixel; invalid-history and miss cases are exactly zero |

A fault-sensitive check temporarily perturbs one shared hit-store result and proves both frontends fail the oracle in the same field; it then restores the source and reruns. Independent temporary perturbations bypass the shared store from each frontend and must be detected, proving the oracle does not pass merely because both paths miss or compare an untouched target. Phase 12 removes every perturbation and temporary comparison resource before handoff.

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

- [Ray-tracing target architecture](../../Architecture/Modules/Engine/Renderer/RayTracingExecution.md)
- [Renderer/RHI boundary](../../Architecture/Decisions/RendererRhiBoundary.md)
- [External Renderer Repository Comparison](../../Research/RendererRepositories.md)
- [Strategy Requirements](../../Strategy/Requirements.md)
- [Renderer Engineering](../../Engineering/Modules/Renderer.md) and [RHI Engineering](../../Engineering/Modules/RHI.md)
- [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)
- [Bistro and San Miguel workloads](../../Acceptance/GraphicsWorkloads.md)

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

- every Phase 0-12 implementation record maps each AC to a claim-falsifying check and exact evidence, every deferred executable claim is discharged by its assigned later phase, and the final scoped [Code Review](../../Engineering/Workflow/CodeReview.md) verdict is `PASS` with no P0-P2 finding;
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
- the [required evidence pack](../../Architecture/CrossModule/ShaderSystem.md#required-evidence-pack) is complete or each unavailable claim is explicitly blocked with provenance.

## Final Position

Sparkle should follow Unreal's lean global-shader center end to end: one direct-dispatch shader class owns its nested parameters and optional compile hooks; the ray-generation shader additionally owns its dispatch-wide payload, attribute, and recursion compile contract; non-dispatch RT stages add only stage-specific local policy when needed; one implementation declaration owns virtual source, entry, and stage; one frozen catalog drives reproducible compile-every-time jobs; one generated global shader map resolves typed shader references to code-library records; and the frame graph dispatches, draws, or traces those shader types through the same typed metadata. Graphics authors set granular pass intent while attachments and prepared mesh/material work supply their own facts; the runtime owner alone assembles the complete graphics key/descriptor and lazily materializes exact requests. A focused ray-tracing composition names only typed stage membership, hit groups, and bounded local data, deriving shared ABI and global bindings from the selected ray-generation shader. One semantic acceleration-structure parameter covers classic and partitioned providers while private RHI owns native descriptor/address representation. Render-graph labels remain diagnostic presentation, while map, code, native pipeline, table, and generation mechanics stay behind their owners.

The clean target is neither "the filename is everything" nor "copy every Unreal subsystem." It is "the author states only the shader class, parameters, source/entry/stage, narrow raster intent, the focused RT composition when several stages truly cooperate, and the actual draw/dispatch/trace; the engine derives and validates everything else." Permutations, universal program types, precaching, preload/streaming, and native driver caches stay out until a measured workload earns them.
