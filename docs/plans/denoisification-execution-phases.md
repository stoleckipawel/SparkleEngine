# Denoisification Execution Phases

## Purpose

This document turns `denoisification-and-exchange-surfaces.md` into concrete implementation phases with action points and copy-ready prompts.

Use the source report for the analysis and rationale. Use this file when starting work.

## Global Rules For Every Phase

Apply these checks before changing code in any phase:

1. Keep public contracts focused on intent, not implementation details.
2. Keep source/display metadata in Tools or Editor, not in runtime RHI, Renderer, or GameFramework diagnostics.
3. Prefer grouped records over same-index vectors and multi-out-parameter APIs.
4. Keep RHI public contracts backend-neutral unless the file is explicitly backend-specific.
5. Keep `D3D12*` names backend-scoped and preferably private.
6. Use `Cooked*` for serialized artifact records, not generic live runtime views.
7. Reserve `Context` for scoped services and `Data` for payloads.
8. Prefer `Registry`, `Loader`, `Cache`, `Manager`, `Builder`, and `Factory` only when the type really has that role.
9. Use the shared shape where useful: `identity -> input -> policy -> output -> diagnostics`.
10. Use function verbs consistently: `Get` for cheap access, `Find` for optional lookup, `Resolve` for id/path translation, `Ensure` for lazy mutation, `Build` for in-memory products, `Create` for owned runtime/backend objects, `Load` for artifact-to-memory work, `Capture` for snapshots, `Apply` for descriptor/payload mutation, `Submit` for handoff, `Process` for draining queued work, and `Release` for explicit native cleanup.
11. Rename files when their primary type is renamed, and use contract-style file names only when the file intentionally groups several small records.
12. When a new shape replaces an old path, remove the old path in the same phase unless the phase explicitly marks it as temporary migration work.
13. Do not add compatibility shims, dual APIs, or fallback code just to keep legacy structure alive.
14. Reduce the mental load of the touched context: fewer concepts to track, fewer duplicated rules, fewer same-index relationships, fewer ambiguous owners, and fewer invalid states.
15. Make the local change surface easier and safer: one owner for each decision, one source of truth for each rule, narrow includes, clear invariants, and call sites that read in domain terms.
16. Do not run build or cook validation in Phases 0-9. Use static validation there, and reserve build/cook validation for Phase 10 after implementation phases are complete.

## Refactor Quality Bar

Every implementation phase must improve the structure of the touched context, not just rename it.

Use this bar while editing:

1. Remove replaced legacy paths instead of leaving old and new flows side by side.
2. Collapse duplicated policy, comparison, conversion, and dispatch rules into one owner.
3. Prefer one grouped record over multiple values that must be remembered together.
4. Prefer one explicit lifecycle role over broad names that require reading several files to understand.
5. Keep migration scaffolding temporary, named, and tracked in the phase if it cannot be removed immediately.
6. End each phase with a short local reasoning check: what is now easier to understand, what legacy path disappeared, and what owner now makes the decision.

## Implementation Prompt Contract

Every phase prompt below must be treated as carrying this contract. Do not run an implementation prompt without it.

1. Apply the Global Rules For Every Phase.
2. Apply the Refactor Quality Bar.
3. Check the Coverage Audit Against The Source Report before editing.
4. Stay inside the phase scope and named files unless a caller forces a directly related include, call site, or rename update.
5. Prefer replacing legacy paths over keeping compatibility shims, dual APIs, or fallback flows.
6. Keep the touched context easier to change safely: one owner per decision, one source of truth per rule, clear invariants, and domain-readable call sites.
7. Use Phase 9 static consistency checks before considering the phase complete.
8. Do not build or cook during Phases 0-9; record final build/cook needs for Phase 10.
9. End the implementation response with this output:
	- changed files
	- legacy paths removed or temporary migration paths explicitly tracked
	- local reasoning win
	- static validation performed
	- Phase 10 build/cook validation notes, if any
	- ready-to-use CL description for the completed phase, including title, summary, validation, and known risks or follow-ups

## Coverage Audit Against The Source Report

Use this as a quick guard so proposed improvements from `denoisification-and-exchange-surfaces.md` do not get lost while phases are implemented.

| Source Report Area | Covered By | Must Not Miss |
| --- | --- | --- |
| Boundary rules | Global rules and Phase 9 | Intent-based public contracts, narrow shared helpers, backend-neutral RHI, renderer products/diagnostics instead of backend lifetime details. |
| Shared process shape | Phase 0 and Phase 9 | Apply `identity -> input -> policy -> output -> diagnostics` where ownership/lifecycle match. |
| Similar systems | Phase 9 | Cook requests/outputs, import/load handoffs, diagnostics snapshots, registries/managers/caches, binding/layout records, viewport products, path policies, descriptor-dispatch-result flows, and build-frame products. |
| Naming standards | Global rules, Phase 0, Phase 6 | File names, suffix roles, function verbs, prefixes/layers, and NVIDIA/AMD-aligned rendering terms. |
| Renderer naming findings | Phases 5 and 6 | `RenderViewContext`, `MaterialCacheManager`, `SceneRenderStateCoordinator`, `RuntimeManager` member naming, and preserving good pass/framegraph names. |
| RHI naming findings | Phase 6 | `Cooked*` artifact vs runtime names, public D3D12 texture contracts, shader reflection names if promoted to runtime metadata, and RHI facets. |
| GameFramework naming findings | Phase 4 | Runtime-only payloads, `SceneAssetManager` role check, `LevelManager` split only when responsibilities split, and `ApplyFromDesc` consistency. |
| Concrete blocks 1-11 | Phases 1-8 | Every source block has an implementation phase and done criteria. |
| Static validation and non-goals | Phase 9 | No global `Result<T>`, no texture request move to `CookCommon`, no editor metadata catalog merge, no cooked format churn, and no broad RHI/editor rewrite first. |
| Build and cook validation | Phase 10 | Defer builds, cooks, and recooks until the final validation phase. |
| Structural denoising | Global rules, Phase 0, Phase 9 | Remove replaced legacy paths, avoid compatibility shims, reduce local reasoning cost, and make each touched context easier to change safely. |

## Phase 0: Baseline Guardrails

**Scope:** Boundary rules, shared process shape, naming rules, NVIDIA/AMD terminology alignment.

**Why first:** This prevents the refactor from becoming a pile of unrelated renames. Every later phase should carry these standards locally.

**Action points:**

1. Open the touched files and classify every public type as one of: `Desc`, `Request`, `Result`, `Snapshot`, `Registry`, `Loader`, `Cache`, `Manager`, `Builder`, `Factory`, `Context`, `Data`, or `Runtime`.
2. For each touched boundary, identify the five slots: identity, input, policy, output, diagnostics.
3. Before adding helpers, choose the narrowest owning module.
4. Check whether any new include pulls Tools, Editor, Win32, or D3D12 details into runtime-facing headers.
5. Carry NVIDIA/AMD terminology where it matches the concept: `Device`, `CommandList`, `SwapChain`, `BackBuffer`, `RenderTarget`, `Present`, `ResourceView`, `RenderPass`, `FrameGraph`, `ShaderReflection`, `ShaderPackage`.
6. Check function verbs against the source report before renaming or adding methods.
7. Preserve good established names unless the touched code proves the role has changed.
8. Identify any legacy path that the phase should delete once the replacement is in place.
9. State the local reasoning win expected from the phase before editing.

**Prompt:**

```text
Analyze the next denoisification slice before editing. Follow the Implementation Prompt Contract from docs/plans/denoisification-execution-phases.md. Use docs/plans/denoisification-and-exchange-surfaces.md and docs/plans/denoisification-execution-phases.md as constraints. For the files in this slice, classify the public contracts by suffix role, identify identity/input/policy/output/diagnostics fields, list boundary leaks or misleading names, identify legacy paths to remove, and state how the touched context should become easier to reason about. Do not build, cook, or edit yet; return the smallest safe implementation plan. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. The phase-specific files have a short pre-edit analysis.
2. Any rename is tied to ownership/lifecycle clarity, not churn.
3. The planned surface does not expand Core, RHI, Renderer, or GameFramework unnecessarily.
4. The plan names the replaced legacy path, or explicitly says there is none.
5. The plan states the local mental-load reduction expected from the refactor.

## Phase 1: Texture Cook Request Contract

**Source block:** Block 1.

**Scope:** `TextureCookRequest`, texture cook policy fields, request equality, dedup, and conflict diagnostics.

**Primary files:**

- `Tools/TextureCooker/Public/TextureCookRequestList.h`
- `Tools/TextureCooker/Private/Requests/TextureCookRequestList.cpp`
- `Tools/MaterialCooker/Private/MaterialCooker.cpp`
- `Tools/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`

**Action points:**

1. Group texture processing knobs into `TextureCookPolicy` or an equivalent policy record.
2. Keep request identity, input path, output path, and policy visibly separate.
3. Add shared equality/conflict helpers or a small `TextureCookRequestSet` in the narrow texture-cook shared area.
4. Replace `AssetCooker` local full-field request comparisons with the shared helper.
5. Replace `MaterialCooker` by-id-only dedup with the same shared path.
6. Keep path normalization policy local to the cook tools; do not expand Core path APIs for this slice.

**Prompt:**

```text
Implement Phase 1 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Refactor texture cook requests so identity, input/output paths, and processing policy are grouped clearly. Add shared request equality/dedup/conflict handling in the narrow texture cooker ownership area, then update AssetCooker and MaterialCooker to use it. Remove replaced local compare/dedup legacy paths instead of leaving dual APIs, and report the local reasoning win at the end. Keep behavior and cooked formats unchanged. Do not build or cook in this phase; use static validation with git diff --check and targeted searches for old local dedup helpers. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. `TextureCookPolicy` or equivalent exists.
2. No local `AssetCooker` full-field texture request comparison remains.
3. `MaterialCooker` no longer dedups texture requests by id alone.
4. Request summaries and output path behavior remain unchanged.

## Phase 2: Cooker Output Records And Cooked Scene Build

**Source blocks:** Blocks 2 and 3.

**Scope:** Mesh/material cooker output APIs and `CookedSceneBuild` grouping.

**Primary files:**

- `Tools/MeshCooker/Public/MeshCooker.h`
- `Tools/MaterialCooker/Public/MaterialCooker.h`
- `Tools/MeshCooker/Private/MeshCooker.cpp`
- `Tools/MaterialCooker/Private/MaterialCooker.cpp`
- `Tools/SceneCooker/Public/CookedSceneBuild.h`
- `Tools/SceneCooker/Private/SceneCooker.cpp`
- `Tools/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/AssetConverter/Private/Cli/AssetConverterCommands.cpp`

**Action points:**

1. Replace paired mesh output vectors with a grouped `MeshCookOutput` record.
2. Replace paired material output vectors with a grouped `MaterialCookOutput` record.
3. Update callers so same-index relationships are no longer implicit.
4. Split `CookedSceneBuild` into identity, manifest build data, asset outputs, and status/result state.
5. Keep manifest write behavior, scene registry paths, instance counts, and reference counts unchanged.
6. Do not change cooked binary formats as a side effect.

**Prompt:**

```text
Implement Phase 2 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Replace mesh/material cooker paired output vectors with grouped output records, then reshape CookedSceneBuild so identity, manifest data, generated asset outputs, and status are separate concepts. Remove replaced out-vector and flat scene-build legacy paths instead of leaving dual APIs, and report the local reasoning win at the end. Preserve cooked file formats, registry paths, and caller behavior. Do not build or cook in this phase; use static validation with git diff --check and stale-symbol searches for the old out-vector API names. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. No `outMeshAssets/outMeshAssetReferences` API pair remains.
2. No `outMaterialAssets/outMaterialAssetReferences` API pair remains.
3. `CookedSceneBuild` is no longer a flat mix of identity, manifest, outputs, and error state.
4. AssetCooker and AssetConverter call sites read as grouped outputs.

## Phase 3: Source Import Contract

**Source block:** Block 4.

**Scope:** Tool-side import handoff shape and importer output ownership.

**Primary files:**

- `Tools/SourceImportAdapters/Public/SourceImportResult.h`
- `Tools/SourceImportAdapters/Private/Fbx/*`
- `Tools/SourceImportAdapters/Private/Gltf/*`
- `Engine/GameFramework/Public/Scene/Materials/MaterialDesc.h`
- `Engine/GameFramework/Public/Scene/Meshes/MeshData.h`

**Action points:**

1. Introduce tool-owned import records such as `ImportedMesh`, `ImportedMaterial`, and `ImportedScene` beside the existing contract first.
2. Move source path, importer type, display/source names, node transforms, and texture source bindings into tool-side records.
3. Update glTF and FBX importers to produce tool import records.
4. Update cookers to translate import records into cooked/runtime-facing records.
5. Remove broad GameFramework runtime includes from SourceImportAdapters public headers once migration is complete.
6. Keep runtime structs out of the permanent source-import exchange model.

**Prompt:**

```text
Implement Phase 3 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Introduce a tool-owned source import contract beside SourceImportResult, migrate glTF/FBX importers and cookers toward ImportedMesh/ImportedMaterial/ImportedScene style records, and remove runtime GameFramework scene/material/mesh dependencies from public SourceImportAdapters headers where possible. Remove replaced runtime-flavored import handoff paths when migration is complete for the touched slice, and report the local reasoning win at the end. Preserve import behavior and readable display/source names. Do not build or cook in this phase; use static validation with git diff --check and include/dependency searches. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Source import records are visibly tool/cook domain records.
2. SourceImportAdapters public headers no longer depend on broad runtime scene/material/mesh headers unless still temporarily required during migration.
3. Cookers own the translation from imported data to cooked/runtime data.

## Phase 4: GameFramework Runtime Scene Data

**Source block:** Block 5.

**Scope:** Runtime payloads, material descriptions, mesh data, scene asset loading.

**Primary files:**

- `Engine/GameFramework/Public/Scene/RuntimeScenePayload.h`
- `Engine/GameFramework/Public/Scene/Materials/MaterialDesc.h`
- `Engine/GameFramework/Public/Scene/Meshes/MeshData.h`
- `Engine/GameFramework/Private/Assets/SceneAssetManager.cpp`
- `Engine/GameFramework/Private/Scene/GameScene.cpp`

**Action points:**

1. Split source/import material fields away from runtime material state.
2. Ensure runtime material data carries cooked texture references or handles, not source texture paths.
3. Review whether `MeshData` is still a gameplay-facing contract or should become cooked/load-time geometry.
4. Keep `RuntimeScenePayload` runtime-only: instances, handles, cooked refs, and runtime material facts.
5. Preserve `GameScene`, `Scene*`, `*Snapshot`, `Level*`, and `RuntimeScenePayload` naming where it remains accurate.
6. Standardize descriptor application verbs when touched, preferably around `ApplyFromDesc`.
7. Review `SceneAssetManager`: prefer `SceneAssetLoader` if it remains loading-only, and keep `Manager` only if it owns loaded asset lifetime.
8. Leave `LevelManager` broad-but-acceptable until responsibilities are actually split; do not rename it as a churn-only step.

**Prompt:**

```text
Implement Phase 4 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Clean GameFramework runtime scene data after the import contract exists: remove source/import material path dependencies from runtime payload flow, make runtime material data use cooked refs or handles, and review MeshData ownership without changing gameplay behavior. Remove replaced source/import runtime paths instead of preserving compatibility shims, and report the local reasoning win at the end. Keep scene subsystem and snapshot names stable. Do not build or cook in this phase; use static validation with git diff --check and grep for source path metadata leaking into runtime scene payloads or renderer-facing diagnostics. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Runtime material loading does not depend on source texture paths.
2. Renderer receives material facts and texture handles/refs, not import paths.
3. `RuntimeScenePayload` does not become a source/import metadata carrier again.

## Phase 5: Renderer Shader Parameter Binding

**Source block:** Block 6.

**Scope:** Pass parameter binding storage and binding-value shape.

**Primary files:**

- `Engine/Renderer/Public/ShaderParameters/PassParameterSet.h`
- `Engine/Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h`
- `Engine/Renderer/Private/FrameGraph/Builder/PassBuilder.cpp`
- `Engine/Renderer/Private/Pipeline/PassBinder.cpp`
- `Engine/Renderer/Private/Passes/ShaderPass.h`

**Action points:**

1. Replace all-fields-at-once binding payloads with typed binding value records.
2. Keep binding kind validation centralized.
3. Preserve missing-binding diagnostics and pass binder behavior.
4. Keep serialized shader reflection/layout records separate from runtime binding values.
5. Carry naming rules: use `Data` for payload records and `Context` only for scoped services.
6. Preserve established render pass/framegraph names, and prefer `Execute` consistently for pass runtime work when touching pass lifecycle methods.

**Prompt:**

```text
Implement Phase 5 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Refactor PassParameterBinding so each binding stores one active typed value shape instead of every possible payload field. Remove replaced all-fields-at-once binding access paths instead of leaving parallel storage models, and report the local reasoning win at the end. Keep public behavior, missing-binding diagnostics, FrameGraph resource declarations, and PassBinder behavior unchanged. Do not build or cook in this phase; use static validation with git diff --check and targeted searches for stale direct field access. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Binding values are typed and small.
2. Setters no longer duplicate broad reset logic.
3. PassBinder code reads binding intent rather than filtering invalid fields.

## Phase 6: Renderer/RHI Naming And Facet Preparation

**Source blocks:** Block 8 plus NVIDIA/AMD benchmark findings.

**Scope:** RHI interface shape, D3D12 public leakage, resource view/descriptors, presentation terminology, high-priority naming fixes when touched.

**Primary files:**

- `Engine/RHI/Public/Interop/RenderHardwareInterface.h`
- `Engine/RHI/Public/Interop/RendererBackendServices.h`
- `Engine/RHI/Public/Resources/TextureTypes.h`
- `Engine/RHI/Public/D3D12/Textures/TextureLoadResult.h`
- `Engine/RHI/Public/D3D12/Textures/CookedTextureAsset.h`
- `Engine/Renderer/Private/Frame/RenderViewContext.h`
- `Engine/Renderer/Private/SceneData/Caching/MaterialCacheManager.h`
- `Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.h`
- `Engine/Renderer/Private/FrameGraph/ResourceRegistry.h`

**Action points:**

1. Do not expand `RenderHardwareInterface` with new unrelated methods.
2. Introduce or prepare narrower facets only when a touched feature needs them: resource creation/upload, descriptors/resource views, shader layout services, diagnostics, presentation.
3. Keep D3D12 descriptor heap/allocator names backend-private.
4. Expose resource-view intent at renderer/RHI seams where descriptor plumbing is currently too low-level.
5. Use standard presentation terms at presentation seams: `SwapChain`, `BackBuffer`, `RenderTarget`, `Present`.
6. Rename `RenderViewContext` toward `RenderViewData` or `PerViewRenderData` when touching frame builders.
7. Rename `MaterialCacheManager` toward `MaterialResourceCache` if the type remains a keyed residency cache.
8. Rename `SceneRenderStateCoordinator` toward `RenderSceneStateSynchronizer` or `SceneRenderStateLifecycle` when touching level-change renderer state.
9. Review `ResourceRegistry`: use `Registry` for identity catalogs and `Cache` for owned reusable resources.
10. Keep `Cooked*` shader/texture names for artifact records; prefer runtime names like `LoadedShaderPackage` or `ShaderPackageCache` for runtime views.
11. Keep `PipelineStateManager` unless it becomes a pure PSO cache; only then would `PipelineStateCache` fit.
12. Rename the `RuntimeManager` member inside `RenderPassContext` toward `PipelineStates` or `PipelineStateManager` when that file is touched.
13. Keep `RenderPassContext` as a service context, or rename to `RenderPassExecutionContext` only if context names keep colliding.
14. If cooked shader reflection metadata is promoted to runtime metadata, consider dropping `Cooked*` from names such as `CookedShaderResourceKind`, `CookedShaderScalarType`, and `CookedShaderResourceBindingRecord`.
15. For public D3D12 texture contracts, move `TextureLoadResult` private or rename it to `D3D12TextureLoadResult` if it must stay public; review `CookedTextureAsset` with the texture format abstraction.
16. Preserve clear backend-neutral RHI names that already fit, such as `RendererBackendServices`, `RenderBindingLayout`, and `RenderPipelineState`.

**Prompt:**

```text
Implement Phase 6 from docs/plans/denoisification-execution-phases.md for one narrow Renderer/RHI slice. Follow the Implementation Prompt Contract in that document. Do not perform a broad RHI rewrite. Keep RenderHardwareInterface from growing, move new work toward recognizable Device/CommandList/SwapChain/ResourceView/diagnostics facets where the touched code justifies it, and apply local naming fixes such as RenderViewData, MaterialResourceCache, or RenderSceneStateSynchronizer only when those files are already being refactored. Remove replaced backend-leaking or misleading local paths where the slice provides a clearer owner, and report the local reasoning win at the end. Preserve backend behavior and keep D3D12 details backend-scoped. Do not build or cook in this phase; use static validation with git diff --check and public-header dependency searches. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. The touched RHI/Renderer surface is narrower or clearer than before.
2. No new concrete D3D12 object leaks into renderer-facing public headers.
3. `RenderHardwareInterface` has not absorbed unrelated responsibilities.
4. Any renamed type has a file-name and include update to match.

## Phase 7: Renderer Viewport Products And Editor UI Surface

**Source blocks:** Blocks 7 and 10.

**Scope:** Viewport request/product flow, editor public UI surface, platform/backend leakage.

**Primary files:**

- `Engine/Renderer/Public/Viewport/ViewportContracts.h`
- `Engine/Application/Public/ProjectApp.h`
- `Engine/Application/Public/EditorApp.h`
- `Engine/Editor/Public/UI.h`

**Action points:**

1. Keep the existing `request -> render -> products -> UI` flow recognizable.
2. Add helper accessors for `ViewportRenderProducts` if flags and fixed fields start drifting.
3. Keep request flags and available-output flags separate concepts.
4. Introduce a smaller editor host interface when touching editor boundaries.
5. Replace public raw Win32 message exposure with engine/platform event contracts.
6. Hide native command-list render entrypoints in private UI implementation.
7. Group diagnostics providers instead of exposing loose callback/function types.

**Prompt:**

```text
Implement Phase 7 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Clean one viewport/editor boundary slice while preserving the request -> render -> products -> UI flow. If ViewportRenderProducts needs help, add accessors that keep available flags and fields consistent. If Editor UI public headers are touched, move away from raw Win32 and native command-list exposure toward editor host/platform event contracts. Remove replaced platform/backend-shaped public UI paths where the slice provides a cleaner facade, and report the local reasoning win at the end. Do not build or cook in this phase; use static validation with git diff --check and searches for Windows.h or native RHI handles in public Editor UI headers. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Editor public headers no longer gain platform/backend details.
2. Viewport product availability is harder to misuse.
3. Renderer contracts remain about products and diagnostics, not editor implementation details.

## Phase 8: Core Utility Ownership And AssetCooker Bridge API

**Source blocks:** Blocks 9 and 11.

**Scope:** Core path/math/string utility boundaries and AssetCooker external bridge isolation.

**Primary files:**

- `Engine/Core/Public/Paths/DirectoryPaths.h`
- `Engine/Core/Public/FileSystemUtils.h`
- `Engine/Core/Public/Paths/PathUtils.h`
- `Engine/Core/Public/Math/MathUtils.h`
- `Engine/Core/Public/Strings/StringUtils.h`
- `Tools/AssetCooker/Public/AssetCookRequest.h`
- `Tools/AssetCooker/Public/AssetCookResult.h`
- `Tools/AssetCooker/Public/AssetCookerTypes.h`
- `Tools/AssetCooker/Private/Api/AssetCookerService.*`
- `Tools/AssetCooker/Private/Cli/AssetCookerCli.cpp`

**Action points:**

1. Keep Core path helpers generic: normalization, simple filesystem helpers, and broadly reusable utilities.
2. Move cook output layout, shader symbol layout, and asset artifact policy into owning tool/runtime modules when touched.
3. Avoid adding DirectX-flavored convenience APIs to Core unless they are truly engine-wide math utilities.
4. Keep AssetCooker C-style pointer structs at the external/API edge.
5. Add owning internal C++ request/result records for cooker internals after tool exchange cleanup.
6. Convert between bridge structs and owning records only in `Private/Api` or CLI boundary code.

**Prompt:**

```text
Implement Phase 8 from docs/plans/denoisification-execution-phases.md for one narrow Core or AssetCooker bridge slice. Follow the Implementation Prompt Contract in that document. Do not expand Core with subsystem policy. Move cook/tool layout policy into the owning module when touched, and keep AssetCooker C-style public bridge structs isolated to API/CLI boundaries by introducing owning internal C++ request/result records where useful. Remove replaced subsystem-policy or bridge-as-domain-model paths where the slice creates the owning model, and report the local reasoning win at the end. Do not build or cook in this phase; use static validation with git diff --check and dependency searches for tool-only path policy entering runtime modules. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Core remains generic and does not become the owner of tool/runtime layout policy.
2. Internal AssetCooker code no longer treats C bridge structs as the main domain model.
3. Public bridge structs remain small, documented, and edge-only.

## Phase 9: Cross-System Consistency Pass

**Scope:** Similar process families, naming actions, validation checklist, and non-goals.

**Run this after each major phase, and once again after all phases.**

**Action points:**

1. Compare the changed system against related families: cook requests/outputs (`TextureCookRequest`, `AssetCookRequest`, `AssetCookResult`, shader package cook outputs), import/load handoffs (`SourceImportResult`, `GameSceneLoadResult`, `SceneAssetLoadResult`, `TextureLoadResult`, shader reload results), diagnostics snapshots (`TextureDiagnosticsRow/Snapshot`, `MeshDiagnosticsRow/Snapshot`, `AssetCookerDiagnosticRecord`, `ConsoleCommandResult`, RHI diagnostic messages), registries/managers/caches, binding/layout records, viewport products, path policies, descriptor-dispatch-result flows, and build-frame products (`FrameGraphBuildResult`, `FrameBuildResult`, render scene snapshots, cooked scene build records).
2. Confirm related systems look familiar only when lifecycle and ownership match.
3. Search for stale symbols, old paired-vector APIs, source metadata in runtime diagnostics, backend types in public Renderer/GameFramework headers, and `Context` names that are really data.
4. Rename files when primary type names changed.
5. Confirm replaced legacy paths, local shims, and duplicate APIs are removed, or explicitly marked as temporary migration work with an owner and removal phase.
6. Confirm the touched context is easier to reason about: fewer concepts, one owner per decision, one source of truth per rule, and fewer invalid states.
7. For diagnostics, keep `row + snapshot` for inspectable runtime state and `severity + message + owner id` for event/status records.
8. For descriptor-dispatch-result flows, keep the shape `descriptor/request -> dispatcher -> result/status` where systems execute named or routed work.
9. For build-frame products, keep build records temporary and group produced resources by meaning instead of turning them into long-lived service state.
10. Keep non-goals intact: no global `Result<T>` conversion, no move of texture request contracts into `CookCommon`, no editor metadata catalog merge, no cooked binary format churn, and no broad RHI/editor rewrite as a first pass.

**Prompt:**

```text
Run Phase 9 consistency validation from docs/plans/denoisification-execution-phases.md for the changes just made. Follow the Implementation Prompt Contract in that document. Compare the touched code against the similar-system families in docs/plans/denoisification-and-exchange-surfaces.md, verify naming suffixes and vendor-aligned terms, search for stale symbols and boundary leaks, confirm replaced legacy paths were removed or explicitly tracked as temporary migration work, and report the local reasoning win. Do not build or cook in this phase; record any required build/cook checks for Phase 10 and start with git diff --check, targeted grep searches, and public include review. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. `git diff --check` passes.
2. Targeted stale-symbol searches are clean or documented.
3. New public includes are reviewed.
4. Boundary greps show no new source/editor/backend metadata leaks.
5. Replaced legacy paths are removed or explicitly tracked as temporary migration work.
6. The local reasoning win is documented in the phase summary.
7. Any deferred issue is recorded as a later phase action, not hidden inside the current change.

## Phase 10: Final Build And Cook Validation

**Scope:** Final validation after implementation phases and static consistency checks are complete.

**Run this only after the relevant implementation phases have landed and Phase 9 static checks are clean.**

**Action points:**

1. Review the implemented phases and list which modules changed: Tools, SourceImportAdapters, GameFramework, Renderer, RHI, Core, Editor, or Application.
2. Choose the smallest build validation that covers the changed modules.
3. Choose cook or recook validation only when the completed phases touched cook behavior, cooked asset contracts, asset request/output flow, or source import translation.
4. Use existing project scripts and generated build files; do not invent a new validation path.
5. Run `git diff --check` before build/cook commands so whitespace failures are caught cheaply.
6. If validation fails, report the failing command and root cause, then fix only issues caused by the completed denoisification phases.
7. If validation is intentionally skipped, record the reason and the exact command that should be run later.

**Prompt:**

```text
Run Phase 10 final validation from docs/plans/denoisification-execution-phases.md. First review the implemented denoisification phases and identify the smallest build and cook validation that covers the changed modules. Start with git diff --check, then use the existing Sparkle build/cook scripts or generated build files for targeted validation. Run cook/recook validation only if the completed phases touched cook behavior, cooked asset contracts, asset request/output flow, or source import translation. If a validation command fails, summarize the failing command and fix only regressions caused by the completed denoisification work. At the end, include a ready-to-use CL description for this phase following the contract.
```

**Done when:**

1. Static validation has passed.
2. The selected build validation has passed or a blocker is documented.
3. The selected cook/recook validation has passed when relevant, or a blocker is documented.
4. Any remaining risk is tied to a concrete follow-up command or phase.

## Recommended Implementation Order

1. Phase 0: Baseline Guardrails.
2. Phase 1: Texture Cook Request Contract.
3. Phase 2: Cooker Output Records And Cooked Scene Build.
4. Phase 3: Source Import Contract.
5. Phase 4: GameFramework Runtime Scene Data.
6. Phase 5: Renderer Shader Parameter Binding.
7. Phase 6: Renderer/RHI Naming And Facet Preparation.
8. Phase 7: Renderer Viewport Products And Editor UI Surface.
9. Phase 8: Core Utility Ownership And AssetCooker Bridge API.
10. Phase 9: Cross-System Consistency Pass after each phase and at the end.
11. Phase 10: Final Build And Cook Validation.

## First Prompt To Use

Start with this prompt when ready to implement:

```text
Start Phase 1 from docs/plans/denoisification-execution-phases.md. Follow the Implementation Prompt Contract in that document. Keep the work limited to the texture cook request contract. Analyze the existing request, dedup, and conflict paths first, then implement TextureCookPolicy or an equivalent grouped policy and shared dedup/conflict handling. Update AssetCooker and MaterialCooker to use the shared path. Remove replaced local compare/dedup legacy paths instead of leaving dual APIs, and report the local reasoning win at the end. Preserve behavior and cooked formats. Do not build or cook in this phase; use static validation with git diff --check and targeted searches. At the end, include a ready-to-use CL description for this phase following the contract.
```