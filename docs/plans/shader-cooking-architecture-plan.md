# Shader Cooking Architecture Plan

Date: 2026-05-19

## Goal

Turn the current shader compiler into a deliberate shader cooking pipeline with clear authoring contracts, durable cooked outputs, stable shader bytecode identity, pipeline layout artifacts and honest PSO readiness, robust dependency discovery, deterministic cook execution, explicit compiler-backend and binary-format availability, and a naming model that matches what the system actually does.

This plan addresses the current critique items in stages:

- Sparkle writes cooked shader packages, but compiled bytecode entries are not yet identified by a stable blob catalog.
- Ray tracing cooking metadata exists, but runtime ray tracing resources and pipelines are not complete across backends.
- Include handling is deterministic but regex-based in two places.
- The cook graph is still insertion-order traversal, not a dependency graph.
- Backend resolution constructs backend objects just to inspect support, and required compiler-backend plus binary-format availability is not represented as an explicit registry contract.
- The single-source fallback path conflicts with the typed shader registration direction.
- Authoring, cooked package contracts, RHI, and tool implementation are still too tightly named and placed together.
- Names such as compiler, target, backend, format, package, stage, shader, layout name, shader name, and PSO stats are overloaded.

## What Blob Means Here

Sparkle already produces binary cooked shader package payloads, such as `.sparkshader` files and a shader package registry. Those are real binary artifacts. The package magic can remain an internal implementation detail; the public cooked artifact name should follow the current path utility and registry behavior.

The missing NVIDIA Donut-style piece we still want is narrower than Donut's full ShaderMake system. Sparkle benefits from naming each compiled bytecode product explicitly. A blob in this plan means one compiled bytecode product for one package entry or ray tracing export, one compiler backend, one codegen target, and one binary format. Donut can also emit static C/C++ header style blobs for embedding or shipping without runtime source discovery; Sparkle can treat that as an optional export format later.

So the problem is not "Sparkle produces no binary output." The problem is:

- Sparkle does not expose a stable `ShaderBlobId` runtime selection and diagnostics contract.
- Sparkle packages stages, but it does not yet provide an explicit blob catalog that says which bytecode belongs to which entry/export, compiler backend, codegen target, and binary format.
- Sparkle has debug artifact bundles, but those are inspection outputs, not runtime blob catalogs.
- Sparkle does not have optional static embedded bytecode output for bootstrap tests, offline samples, or fallback-free minimal runtimes.

The target is not to copy Donut's names. The target is to adopt the useful shape: name each compiled bytecode product deterministically, package it, and make runtime selection explicit.

## Professional Output Contract

The shader cooker should follow the established shape used by production engines and reference frameworks: offline tools emit compiled bytecode plus reflection and metadata; runtime code translates those records into backend-native shader modules, root signatures, descriptor set layouts, pipeline layouts, shader tables, and PSOs. Sparkle should not invent a new kind of shader artifact when NVIDIA Donut/ShaderMake, AMD Cauldron-style sample infrastructure, and Unreal-style cooked shader pipelines all converge on this separation.

The expected outputs are:

| Output | Form | Owner | Purpose |
| --- | --- | --- | --- |
| Cooked shader package | Binary `.sparkshader` file under cooked shader package root | Shader cooker writes, runtime shader package cache reads | Shipping/runtime artifact containing package identity, bytecode records, reflection records, binding/layout metadata, ray tracing metadata, UTF-8 strings, and binary blob storage. |
| Shader package registry | `ShaderPackageRegistry.sreg` under cooked shader root | Shader cooker writes, runtime package discovery reads | Stable map from logical package identity to cooked package files and publication/generation data. |
| Shader blob catalog | Records inside the package or registry | Shader cooker writes, runtime and inspection tools read | Names each compiled bytecode product by package, entry/export, compiler backend, codegen target, and binary format. This is an index over bytecode already stored in the package, not a separate loose-file system. |
| Reflection and binding metadata | Fixed-size records plus string table entries in the package | Compiler backend extracts, cooker serializes, RHI/Renderer consume | Describes resources, constant buffers, push constants where relevant, input elements, shader visibility, and logical binding names. Keep native D3D12 registers/root signatures and Vulkan descriptor objects out of the neutral cooked records unless they live in a clearly backend-owned translated section. |
| Pipeline layout artifact | Cooked package section or derived runtime cache record | Cooker emits neutral layout intent; RHI translates | Bridges reflection/binding metadata to runtime root signature or descriptor set layout creation. It is not a full PSO unless render state and pass state are available. |
| Debug artifact bundle | Optional directory outside runtime packages | Shader cooker writes, developers inspect | Compile command, resolved source/dependencies, preprocessed/debug source, reflection dump, disassembly when available, parameter verification, blob id, backend/target/format, and diagnostics. Never required for runtime loading. |
| Analysis reports | Optional CSV/JSON files outside runtime packages | Shader cooker writes, developers/CI inspect | Stats and readiness data. These are not runtime shader assets. |
| Local compile cache | Tool-private cache directory | Shader cooker owns | Speeds up repeated cooking. It is not a shipping artifact and must be invalidated by source, include closure, backend identity, codegen target, binary format, compile defines, and package schema version. |
| Static bytecode export | Optional generated `.h`/`.cpp` or similar files | Shader cooker writes only when explicitly requested | Bootstrap/test/sample convenience output. It must not replace `.sparkshader` as the primary runtime artifact. |

Do not add multi-output expansion, runtime alternative selection, or a list command for generated alternatives in this plan. If a shader entry needs compile defines, treat them as explicit package or entry compile options and include them in the blob identity/cache key. The professional baseline here is a stable one-output-per-entry/export package model.

Runtime-created objects are deliberately not primary cook outputs. D3D12 shader modules, root signatures, PSOs, Vulkan shader modules, descriptor set layouts, pipeline layouts, pipeline caches, and ray tracing shader tables are created or cached by RHI/Renderer from cooked records. A later PSO cache can be added when render pass, vertex input, raster/depth/blend state, and backend compatibility policy are concrete.

## Output Layout And Naming Contract

Use names that a graphics programmer would recognize immediately. Folder names should identify whether the contents are runtime artifacts, debug artifacts, analysis reports, or tool caches. File names should use stable machine identifiers where runtime lookup requires them, and readable logical names where humans inspect them.

Canonical runtime layout:

```text
Cooked/<ProjectName>/Shaders/
	ShaderPackageRegistry.sreg
	Packages/
		<ShaderPackageKeyHex>.sparkshader
```

Rules:

- Keep `ShaderPackageRegistry.sreg` at the cooked shader root. It is the runtime discovery index.
- Keep shipping package files under `Packages/`. The current file name is `<ShaderPackageKeyHex>.sparkshader`; the registry and inspection output must expose the readable `packageId` so humans do not have to decode file names.
- Use `.sparkshader` for package files and reserve package magic/version fields for binary validation inside the file.
- Use `ShaderPackageKey`, `ShaderBlobId`, `PackageId`, `EntryPoint`, `ExportName`, `CompilerBackend`, `CodegenTarget`, and `BinaryFormat` as the canonical field names in inspection output, debug manifests, CSV/JSON reports, and logs.
- Prefer PascalCase for serialized field names that mirror C++ records and kebab-case for command-line switches. Examples: `ShaderBlobId` in manifests, `--debug-artifacts` on the CLI.
- Keep logical names readable and stable: package ids such as `ForwardOpaque`, entries such as `VSMain`, exports such as `RayGen`, backend names such as `dxc` or `slang`, targets such as `DxilSm66` or `SpirV16`, and formats such as `Dxil` or `SpirV`.
- Do not put source paths, compiler command lines, debug dumps, disassembly, or analysis CSVs inside shipping package directories unless they are part of the binary package itself.

Recommended side-output layout:

```text
<DebugArtifactRoot>/
	<PackageId>/
		<EntryOrExportName>/
			<CompilerBackend>-<CodegenTarget>-<BinaryFormat>/
				compile-command.json
				compile-defines.json
				dependencies.json
				preprocessed-source.hlsl
				reflection.json
				parameter-struct-verification.json
				disassembly.txt
				diagnostics.txt

<AnalysisRoot>/
	CookedShaderStats.csv
	PipelineStateStats.csv

<StaticExportRoot>/
	<PackageId>_<EntryOrExportName>_<CompilerBackend>_<CodegenTarget>_<BinaryFormat>.h
```

The exact debug root can remain user-selected via `--debug-artifacts <dir>`, and analysis can continue to default to the tool cache until a dedicated `--analysis-dir <dir>` exists. The names inside those roots should still follow this contract so artifacts are easy to browse and compare.

CLI and log output should use the same vocabulary as the files. Prefer structured fields such as `packageId`, `packageKey`, `packagePath`, `blobId`, `entryPoint`, `exportName`, `compilerBackend`, `codegenTarget`, `binaryFormat`, `debugArtifactRoot`, `analysisOutput`, and `cacheDirectory`. Avoid ambiguous fields such as `shader`, `target`, `backend`, `blob`, or `path` when a more specific name exists.

## Current State

- `Tools/ShaderCompiler` owns offline shader cooking, package writing, registry writing, debug artifacts, package inspection, and analysis commands.
- DXC and Slang backends exist behind `IShaderBackend`; backend auto-selection is currently source-extension based.
- `ShaderCompileOptions` carries raw defines, source path, entry point, stage, backend target, package kind, and ray tracing export metadata.
- Typed shader registration lives in RHI authoring headers. Cook planning reads `GlobalShaderRegistry` and builds cooked package descriptions from registered shaders.
- Cooked packages contain stage bytecode, reflection-derived binding records, package layout metadata, and early ray tracing export/hit-group/local-parameter metadata.
- `PsoStatsPass` writes package-level CSV stats, not true pipeline state object stats.
- Include preprocessing and include closure hashing each scan source text with their own regex.
- `DependencyGraph` currently stores nodes and returns insertion order.
- Vulkan RHI ray tracing methods for AS prebuild info, scratch buffer creation, acceleration structure buffers, and instance buffers are placeholders or fail as not implemented.

## Target Architecture Rules

- Use "cook" for the offline process that turns authored shader declarations into cooked runtime artifacts.
- Use "compile" only for backend code generation from source text to bytecode.
- Treat DXC, Slang, and future compilers as compiler backends, not as the shader cooking system.
- Treat DXIL, SPIR-V, and future outputs as cooked binary formats.
- Treat shader model or SPIR-V environment as codegen targets.
- Make compiled bytecode blobs addressable by stable package, entry/export, compiler backend, codegen target, and binary format identity.
- Make pipeline layout artifacts first-class cook outputs, and define PSO artifact inputs clearly before adding any PSO cook output.
- Make include dependency discovery single-sourced and language-aware enough for conditionals and comments.
- Make backend availability a registry fact, not a side effect of constructing expensive backend instances during name resolution.
- Keep runtime cooked-only. Do not add runtime shader source parsing or runtime compiler fallback.
- Keep the professional output contract explicit: shipping runtime loads package/registry artifacts; debug, analysis, local cache, and static exports are optional side outputs.
- Keep output names recognizable and consistent across folders, package inspection, debug manifests, analysis reports, CLI help, and logs.
- Keep RHI backend-neutral authoring and cooked contracts separate from D3D12, Vulkan, DXC, Slang, SPIRV-Reflect, and editor tooling.
- Prefer clean replacement over compatibility shims when a new contract is chosen.
- Do not keep legacy command names, CLI aliases, file names, manifest names, serialized fields, validation targets, or runtime fallback paths after their replacement phase lands. Staged implementation can temporarily use current names only until the phase that replaces them.

## Offline Tool And Runtime Hard Line

The shader cooker and the runtime engine must have a hard responsibility split. The offline tool may depend on compiler SDKs, source files, include resolution, reflection extraction, debug artifacts, analysis reports, local caches, backend registries, codegen-target descriptors, and binary-format descriptors. Runtime engine code must consume only cooked runtime artifacts and shared neutral contracts.

Shader cooker responsibilities:

- Read authored shader registrations, source files, includes, compile options, and package descriptions.
- Own DXC, Slang, SPIRV-Reflect, compiler-backend descriptors, codegen-target descriptors, and binary-format descriptors.
- Compile source to bytecode, extract reflection, verify parameter layouts, build blob ids, write `.sparkshader` packages, write `ShaderPackageRegistry.sreg`, emit debug artifacts, emit analysis reports, and manage the local compile cache.
- Provide tool commands such as `cook`, `inspect-package`, `list-backends`, and future explicit static export commands.

Runtime engine responsibilities:

- Load `ShaderPackageRegistry.sreg` and `.sparkshader` packages from cooked output only.
- Validate package version, package key, shader blob id, codegen target, binary format, reflection records, binding metadata, pipeline layout intent, and ray tracing metadata.
- Translate cooked records into backend-native shader modules, root signatures, descriptor set layouts, pipeline layouts, shader tables, runtime pipeline objects, and runtime caches.
- Reject unsupported devices, missing cooked metadata, or backend/format mismatches through runtime capability diagnostics before render work begins.

Forbidden runtime responsibilities:

- Do not include or link DXC, Slang, SPIRV-Reflect, shader compiler backend headers, or tool-private cooking headers from runtime modules.
- Do not parse shader source files, scan includes, run source preprocessing, invoke compiler backends, inspect debug artifact directories, read analysis reports, or use local compile caches at runtime.
- Do not call `list-backends` or depend on offline backend registry data from runtime code. Runtime capability checks are RHI/device facts; offline backend availability is a cooker/tool fact.
- Do not keep runtime shader source fallback paths. If cooked data is missing or incompatible, runtime must fail clearly instead of compiling.

Shared contract responsibilities:

- Shared code may contain neutral authoring declarations, cooked package structs, package utilities, registry records, stable enum/string conversions, and validation helpers needed by both the cooker and runtime.
- Shared contracts must not include compiler SDK types, tool-private orchestration types, renderer-private implementation types, or backend-native graphics handles unless those handles are inside clearly backend-owned runtime code.
- Boundary validation should enforce this split: compiler SDK tokens stay in approved tool/backend paths, tool-private cooking tokens stay out of runtime, and runtime-private renderer/RHI implementation stays out of the cooker except through shared cooked contracts.

## Clean Replacement Policy

This plan does not preserve backward compatibility for the old shader cooking surface. When a phase replaces a contract, remove the old contract in the same phase instead of adding aliases or deprecation layers.

Examples:

- When Phase 1 replaces overloaded CLI inputs, remove the ambiguous production `--shader` behavior instead of keeping it as an alias for `--package`, `--shader-id`, or `--source-file`.
- When Phase 1 removes the implicit raw-source production fallback, do not keep a compatibility path that silently creates `Empty` layout packages.
- When Phase 2 adds `ShaderBlobId`, make package inspection, debug artifacts, logs, and cache keys use the new identity directly instead of printing both old and new identity names.
- When Phase 3 corrects PSO stats naming, remove the old `PsoStatsPass`/`pso-stats.csv` production names instead of writing duplicate reports.
- When Phase 8 moves shader authoring and cooked contracts, update includes and validation gates to the new owner instead of forwarding through RHI-owned compatibility headers.
- When Phase 9 renames `ShaderCompiler` to `ShaderCooker`, update scripts, CMake targets, editor launch code, validation, docs, command help, and logs in one clean cut. Leave old names only in historical notes, not executable paths or production includes.

## Goal Delivery Map

These phases are not an open-ended backlog. They are the delivery path for the goal above. If a phase is skipped, the corresponding goal remains incomplete.

| Goal area | Delivering phases | Done when |
| --- | --- | --- |
| Professional output contract and recognizable folders/names | Phase 0, Phase 2, Phase 3, Phase 9 | Runtime artifacts, debug artifacts, analysis reports, local caches, and optional static exports have distinct locations and names; package inspection, CLI output, logs, and serialized manifests use the same vocabulary. |
| Explicit cook inputs and no legacy source fallback | Phase 1 | Production cooking uses package/shader/source-file intent explicitly, and raw source cooking cannot silently create an `Empty` layout package. |
| Durable cooked outputs and stable bytecode identity | Phase 2 | Every compiled bytecode product has a stable `ShaderBlobId`, and runtime package/registry data can select bytecode without source paths. |
| Reflection, binding metadata, and pipeline layout artifacts | Phase 3 | Cooked packages expose bytecode, reflection, binding metadata, and pipeline layout intent as separate inspectable sections that D3D12 and Vulkan can translate consistently. |
| Honest PSO readiness | Phase 3 | The plan defines the exact PSO input model and stats naming, but does not claim to cook PSOs until render pass, vertex input, raster/depth/blend state, shader blob ids, and backend compatibility policy are all available. |
| Robust dependency discovery and cache correctness | Phase 4 | One dependency scanner owns include/dependency discovery for preprocessing, debug artifacts, cache keys, and missing/recursive include diagnostics. |
| Real cook graph and deterministic serial execution | Phase 5 | Cook work is represented as a validated DAG; serial execution produces stable aggregate outputs. |
| Compiler-backend and binary-format registry clarity | Phase 6, Phase 9 | Backend listing, auto-selection, configure errors, and cooked-package diagnostics use precise terminology; DXC and Slang are required compiler backends, while DXIL and SPIR-V are first-class binary formats/codegen outputs with explicit descriptor and version reporting. |
| Hard offline-tool/runtime-engine boundary | Phase 0, Phase 3, Phase 6, Phase 8, Phase 9 | Compiler SDKs, source preprocessing, reflection extraction, debug artifacts, analysis reports, local caches, and backend registries remain offline-tool concerns; runtime consumes only cooked artifacts and shared neutral contracts. |
| Ray tracing capability and cooked metadata readiness | Phase 7 | Cooked ray tracing package metadata is validated against RHI capability data, and unsupported devices fail through diagnostics before any acceleration-structure, shader-table, pipeline, dispatch, or render work is attempted. |
| Clear authoring/cooked contract ownership | Phase 8 | Shader authoring declarations and cooked package structs live in a shared shader contract owner, while RHI consumes them without owning offline authoring concepts. |
| Final naming model across code and outputs | Phase 9 | Production code, scripts, validation, docs, command help, logs, folder names, and artifact names use the final terms consistently. |

Full delivery requires Phases 0 through 9 plus their validation. Phase 3 intentionally delivers pipeline layout artifacts and PSO readiness, not fake PSO files. A future PSO cache phase should only be added after the renderer exposes the missing render-state inputs listed above.

## Naming Decisions

Use these as the final names unless an implementation phase finds a concrete conflict.

| Current name | Target name | Reason |
| --- | --- | --- |
| `ShaderCompiler` executable | `ShaderCooker` | The tool cooks packages, registries, layouts, analysis outputs, and debug bundles. Compilation is one step. |
| `Engine::ShaderCompiler` namespace | `Engine::ShaderCooking` or plain tool-local owners | Keeps offline cooking distinct from backend compilation. |
| `IShaderBackend` | `IShaderCompilerBackend` | Makes DXC/Slang role explicit. |
| `ShaderTarget` | `ShaderCodegenTarget` | Avoids confusion with runtime platforms and shader stages. |
| `CookedShaderBinaryFormat` | Keep | This already names DXIL/SPIR-V correctly. |
| `backendName` | `compilerBackendName` | Names the selected compiler backend, not the renderer backend. |
| `ShaderCookStageDesc` | `ShaderEntryCookDesc` for graphics/compute, `ShaderExportCookDesc` for ray tracing | A ray tracing export is not a graphics stage. |
| `ShaderCookPackageDesc::stages` | `entries` or `exports` depending on package kind | Avoids pretending every package member is a graphics stage. |
| `LayoutName` | `BindingName` | This is engine binding identity. |
| `ShaderName` in parameter fields | `ReflectedName` | This is the HLSL/Slang symbol name. |
| generic `Name` in parameter fields | `FieldName` or remove where redundant | Avoids three competing names. |
| `PsoStatsPass` | `CookedShaderStatsPass` now, `PipelineStateStatsPass` later | Current CSV reports cooked shader package stats, not PSO stats. |
| `--shader` CLI option | `--package`, `--shader-id`, or `--source-file` | One option should not mean package id, shader id, source path, and fallback source cook. |
| `BuildSingleShaderPackage` | remove, or replace with explicit `cook-source` dev command | Hardcoded `VSMain`/`PSMain` conflicts with typed registrations. |

Do not rename everything in one blind sweep. Each phase below gives the contract boundary where the rename becomes useful and testable.

## Phased Implementation

Each phase is written as a copy-paste implementation prompt. The fenced prompt should give a future implementation turn enough context to start locally, make scoped edits, preserve the intended boundaries, and validate the touched slice. After each code phase, run the narrow source validation for the touched slice before moving on. Build commands should use current target names until the rename phase lands.

### Phase 0: Architecture Inventory And Gates

Goal: Deliver the validation and terminology foundation that every later phase must obey.

Implementation prompt:

```text
Implement Phase 0 of the shader cooking architecture plan: audit shader cooking ownership and add source validation gates for the cleanup work.

Start from the current ShaderCompiler target, existing CMake validation scripts, `Tools/ShaderCompiler`, `Engine/RHI/Public/Shaders`, and the runtime ray tracing placeholders. Record the current tool entrypoints, cooked package outputs, typed shader registration inputs, backend registrations, include dependency scanners, graph executor path, analysis passes, and runtime ray tracing placeholders.

Add or update validation scripts so runtime sources cannot include DXC, Slang, SPIRV-Reflect, or tool-private cooking headers, and so tool code cannot reach renderer-private runtime implementation. Keep this phase documentation/validation focused; do not move production code or change runtime behavior.

Add a small architecture checklist under docs or CMake validation output that names the approved final terms and output categories: cook, compiler backend, codegen target, binary format, shader blob, package, registry, reflection metadata, binding metadata, pipeline layout artifact, debug artifact bundle, analysis report, local compile cache, static export, entry, export, binding name, and reflected name. Include the canonical runtime layout and the canonical inspection/log field names from the output layout contract. Include the clean replacement rule: each later phase removes the old command/name/path in the same phase that introduces the replacement, with no permanent aliases or backward-compatibility shims.

Record the hard offline-tool/runtime-engine boundary in the checklist. The shader cooker may own compiler SDKs, source processing, backend registries, caches, debug artifacts, and analysis outputs. Runtime code may only consume cooked packages, cooked registries, shared neutral contracts, and RHI/device capability data.
```

Acceptance criteria:

- Boundary validation still passes for the current layout.
- The validation scripts fail if DXC/Slang tokens appear outside approved backend/tool paths.
- The doc and validation wording use the new terms and output categories consistently.
- Shipping runtime outputs are clearly separated from debug artifacts, analysis reports, local caches, static exports, and runtime-created native graphics objects.
- Folder names, file names, inspection fields, and log fields follow the output layout and naming contract.
- The checklist records that replaced shader cooking contracts are removed cleanly, not preserved through aliases or compatibility shims.
- The checklist records that runtime modules must not include compiler SDKs, tool-private cooking headers, source dependency scanners, backend registries, debug artifact readers, analysis reports, or local compile caches.
- No runtime behavior changes.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target shader_compiler_boundary_check rhi_backend_boundary_check shader_package_parity_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```

### Phase 1: CLI And Naming Contract Cleanup

Goal: Deliver explicit cook command inputs and remove the legacy raw-source fallback from production cooking.

Implementation prompt:

```text
Implement Phase 1 of the shader cooking architecture plan: replace overloaded shader cooking command names with explicit cook inputs.

Start from `CookShadersCommand`, `ShaderCookPlanner`, cook scripts, and editor recook command construction. Split the current `--shader` behavior into explicit options. Use `--package <package-id>` for registered package cooking, `--shader-id <registered-shader-name>` for a registered shader selection if that remains useful, and `--source-file <path>` only for an explicit development command that is not part of production package cooking.

Remove the implicit `BuildSingleShaderPackage` fallback from the production `cook` path. If ad hoc source cooking is still needed for development, make it a separate command such as `compile-source` or `cook-source-dev` with explicit `--entry`, `--stage`, `--binding-layout`, and `--package-kind` arguments. Do not silently assume `VSMain`, `PSMain`, or the `Empty` layout.

Rename user-facing help text from compiler language to cooking language where the command writes packages or registries. Keep backend compilation names only inside backend code and diagnostics. Do not keep deprecated aliases for the old ambiguous production `--shader` behavior, legacy manifest commands, or implicit raw-source package construction. Finish by updating scripts and tests that still pass the old ambiguous option.
```

Acceptance criteria:

- Production package cooking fails clearly when a requested package or shader id is unknown.
- No production cook path creates an `Empty` layout package from a raw source path by accident.
- CLI help distinguishes package cooking from source compilation.
- Old ambiguous CLI names and source fallback paths are removed, not kept as compatibility aliases.
- Existing cook scripts use the explicit package/global cook path.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler shader_compiler_boundary_check shader_package_parity_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
build\bin\DevelopmentEditor\ShaderCompiler.exe cook --help
```

### Phase 2: Stable Shader Blob Catalog

Goal: Deliver durable package/registry/blob identity for the current one-output-per-entry model.

Implementation prompt:

```text
Implement Phase 2 of the shader cooking architecture plan: add a cooked shader blob catalog.

Start from cooked package writing, cooked registry writing, package inspection, debug artifact emission, cache key generation, and runtime cooked package loading. Keep the current one-output-per-entry shader model. If a shader needs compile defines, keep them explicit compile options owned by the package or entry. Do not add multi-output expansion, runtime alternative selection, or a list command for generated alternatives.

Add a stable `ShaderBlobId` for every compiled bytecode output. It should include package id, entry/export identity, compiler backend name, codegen target, and binary format. Store the blob catalog inside cooked packages or the package registry so runtime can select a bytecode blob by package and entry/export without knowing source paths.

Keep `.sparkshader` as the primary runtime package artifact. Keep package files under `Cooked/<ProjectName>/Shaders/Packages/` and keep `ShaderPackageRegistry.sreg` at `Cooked/<ProjectName>/Shaders/`. Optionally add `--emit-static-blobs <dir>` later for generated `.h` or `.cpp` bytecode arrays used by bootstrap tests or standalone samples. Static blobs are an export format, not the primary runtime contract. Finish by making package inspection print readable package/blob fields and by ensuring debug artifacts identify the same blob ids as the cooked package.
```

Acceptance criteria:

- Every compiled bytecode output has a stable blob id.
- Cache keys include the normalized blob identity inputs.
- Debug artifacts include package id, entry/export identity, compiler backend name, codegen target, binary format, compile defines, and blob id.
- Cooked packages or registries expose enough metadata to select the right bytecode blob at runtime.
- Existing shaders continue to cook as one bytecode output per registered entry/export and codegen target.
- The package/registry/debug-artifact split matches the professional output contract: runtime loads package and registry data; debug artifacts, reports, local caches, and static exports stay optional side outputs.
- Package inspection, debug manifests, and logs use canonical names such as `PackageId`, `ShaderPackageKey`, `ShaderBlobId`, `EntryPoint`, `ExportName`, `CompilerBackend`, `CodegenTarget`, and `BinaryFormat`.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler shader_package_parity_check sparkle_validation_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
build\bin\DevelopmentEditor\ShaderCompiler.exe cook --debug-artifacts build\ShaderDebugArtifacts
build\bin\DevelopmentEditor\ShaderCompiler.exe inspect-package --package ForwardOpaque
```

### Phase 3: Pipeline Layout And PSO Artifact Cooking

Goal: Deliver pipeline layout artifacts and an explicit PSO readiness contract without inventing fake PSO outputs.

Implementation prompt:

```text
Implement Phase 3 of the shader cooking architecture plan: add cooked pipeline layout artifacts and prepare PSO cooking.

Start from shader reflection extraction, `ShaderPackageLayoutBuilder`, cooked package records, D3D12/Vulkan runtime layout creation, and `PsoStatsPass`. Use shader reflection plus typed parameter layout data to emit a stable pipeline layout artifact per package and codegen target. For D3D12 this maps toward root signature intent. For Vulkan this maps toward descriptor set layout, push constant, and pipeline layout intent. Keep the artifact backend-neutral where possible and backend-specific only in clearly owned backend translation records.

Rename `PsoStatsPass` to `CookedShaderStatsPass` until real PSO artifacts exist, and write `CookedShaderStats.csv` instead of `pso-stats.csv`. Add a new `PipelineStateStatsPass`/`PipelineStateStats.csv` only when the cooker has render-state and pipeline identity data.

Define the future PSO cook input model: graphics shader package, render pass or dynamic rendering contract, vertex input, raster/depth/blend state, shader blob ids, and codegen target. Do not fake a PSO if those inputs are not available yet. Native root signatures, descriptor set layouts, pipeline layouts, shader modules, and PSOs remain runtime-created or runtime-cached objects unless a later backend-owned cache contract is explicitly added. Finish by making inspection distinguish bytecode, reflection, pipeline layout, and future PSO sections.
```

Acceptance criteria:

- Cooked package inspection shows pipeline layout metadata separately from raw binding reflection.
- D3D12 and Vulkan runtime layout creation can consume the same cooked layout contract or an explicit translated backend layout record.
- Native backend graphics objects are not written as primary cook outputs by this phase.
- The old PSO stats name is removed or corrected.
- No code path claims to cook PSOs until real PSO input state exists.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler ShowcaseRuntime rhi_backend_parity_check shader_package_parity_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```

### Phase 4: Include Dependency Scanner Replacement

Goal: Deliver one dependency discovery path for preprocessing, cache correctness, debug artifacts, and include diagnostics.

Implementation prompt:

```text
Implement Phase 4 of the shader cooking architecture plan: replace regex include scanning in preprocessing and include-closure hashing with a shared dependency scanner.

Start from `ShaderSourcePreprocessor`, `IncludeClosureHasher`, `ShaderIncludeResolver`, cache key generation, and debug artifact source emission. Introduce `ShaderSourceDependencyScanner` as the single owner of source dependency discovery. It should return resolved include paths, source hashes, diagnostics, and enough line/source information for debug artifact emission. Prefer compiler/backend dependency output when available, such as DXC dependency output or Slang module dependency APIs. Where backend dependency output is unavailable, use a small lexer that understands comments, string literals, preprocessor lines, and inactive conditional blocks well enough for deterministic cache dependencies.

Update `ShaderSourcePreprocessor` and `IncludeClosureHasher` to consume the shared scanner results instead of each owning regexes. Keep `#line` emission for debug artifacts, but do not let debug preprocessing define cache correctness differently from the dependency scanner. Finish by adding small source cases for comments, strings, missing includes, recursive includes, and active included-file hash changes.
```

Acceptance criteria:

- There is one include/dependency scanner owner.
- Includes inside comments and ordinary strings are ignored.
- Includes behind inactive simple conditionals do not poison cache dependencies when backend dependency output can answer that accurately.
- Recursive include and missing include diagnostics still name the includer path.
- Include closure hash changes when an active included file changes.
- DXC and Slang paths either use backend dependency data or share the same fallback scanner with clear limitations documented.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler shader_compiler_boundary_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
build\bin\DevelopmentEditor\ShaderCompiler.exe cook --debug-artifacts build\ShaderDebugArtifacts
```

### Phase 5: Real Cook Graph And Serial Executor

Goal: Deliver a real deterministic cook DAG and a serial execution contract.

Implementation prompt:

```text
Implement Phase 5 of the shader cooking architecture plan: turn the shader cook graph into a real DAG.

Start from `DependencyGraph`, `CookNode`, `SerialCookExecutor`, `ShaderCookGraphBuilder`, and `ShaderCookGraphExecutor`. Model nodes for dependency scan, preprocess/debug source emission, backend compile, reflection extraction, parameter verification, blob catalog write, package write, registry write, static blob export, and analysis output. Add edges that express required ordering. Implement topological sort with cycle diagnostics. Route execution through the serial executor so each cook step follows the validated graph order.

Keep output determinism: registry order, package order, blob catalog order, diagnostics order, and analysis output should remain stable for identical inputs. Use stable sorting before writing aggregate artifacts. Finish by adding graph validation coverage for ordering and cycles.
```

Acceptance criteria:

- `DependencyGraph` stores edges and validates cycles.
- Serial execution order comes from topological sort, not insertion order.
- A failed node reports the package, entry/export, compiler backend, codegen target, and binary format that failed.
- Aggregate outputs remain byte-stable for identical inputs.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler sparkle_validation_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
build\bin\DevelopmentEditor\ShaderCompiler.exe cook --debug-artifacts build\ShaderDebugArtifacts
```

### Phase 6: Backend And Binary Format Registry

Goal: Deliver explicit compiler-backend and binary-format availability while keeping DXC, Slang, DXIL, and SPIR-V equally visible in the shader cooker contract.

Implementation prompt:

```text
Implement Phase 6 of the shader cooking architecture plan: replace backend probing by construction with registry descriptor models for compiler backends, codegen targets, and cooked binary formats.

Start from `ShaderBackendFactory`, `BuiltinBackends`, backend CMake wiring, backend listing, backend pool creation, target parsing, format selection, and `Tools/ShaderCompiler/CMakeLists.txt`. Extend compiler backend registrations with static descriptors: name, required dependency status, supported source extensions, supported codegen targets, supported binary formats, ray tracing library support, inline ray query support, version probe, and availability probe. Add or expose format/target descriptors so DXIL and SPIR-V are reported as first-class cooked binary formats rather than incidental backend details. `ResolveShaderBackendName` should select from descriptors and availability probes without constructing a full backend compiler instance unless compilation is about to run.

Keep DXC and Slang mandatory in CMake. If the DXC dependency or the Slang SDK include directory, import library, or runtime DLL is missing, configure should fail clearly with the missing path or package. The `dxc` and `slang` compiler backends must not be hidden behind enable flags, soft fallbacks, or single-backend compatibility modes. Finish by making `list-backends` report required backend status, version, capabilities, resolved runtime location, supported codegen targets, and supported binary formats without constructing a compile backend.

Keep the registry tool-only. Runtime must not include the backend registry, call `list-backends`, or use offline backend availability to decide device capability. Runtime only sees cooked package fields such as `CompilerBackend`, `CodegenTarget`, and `BinaryFormat`, then maps those against RHI/device support.
```

Acceptance criteria:

- Listing backends reports required DXC and Slang compiler backends with versions, capabilities, resolved dependency locations, supported codegen targets, and supported binary formats.
- DXIL and SPIR-V are listed or reported as first-class cooked binary formats/codegen outputs, not hidden as incidental backend implementation details.
- Auto-selection considers source extension, codegen target, binary format, and required backend availability.
- Configure fails clearly when the DXC dependency or the Slang SDK include directory, import library, or runtime DLL is missing.
- Successful builds copy `slang.dll` beside the tool.
- Backend capability checks do not construct heavyweight compiler objects during simple name resolution.
- Backend registry descriptors remain inside the offline tool/backend layer and are not consumed by runtime engine modules.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
build\bin\DevelopmentEditor\ShaderCompiler.exe list-backends
```

### Phase 7: Ray Tracing Capability And Metadata Readiness

Goal: Deliver runtime ray tracing capability reporting and cooked metadata validation without enabling real ray tracing rendering or requiring ray tracing-capable hardware.

Implementation prompt:

```text
Implement Phase 7 of the shader cooking architecture plan: complete the runtime ray tracing capability and metadata readiness path that matches cooked ray tracing shader package metadata, without creating production ray tracing render usage.

Start from `RenderHardwareInterface`, `RhiRayTracingDesc`, D3D12 and Vulkan RHI implementations, cooked ray tracing package records, and renderer-side package validation. Implement or refine RHI-neutral descriptors for ray tracing capability reporting, BLAS/TLAS prebuild info descriptions, scratch buffer requirements, acceleration structure buffer requirements, instance buffer layout, ray tracing pipeline metadata, shader table metadata, and dispatch requirements. Keep these as capability/description contracts for now. Do not wire real render passes, scene ray tracing effects, dispatch rays, or production acceleration-structure build usage in this phase.

Connect cooked ray tracing library packages to runtime validation only. Use cooked export records, hit group records, local parameter records, payload size, attribute size, max recursion depth, shader blob ids, codegen target, and binary format to validate that a future runtime pipeline could be built. Runtime should reject unsupported backends or missing cooked metadata with a clear diagnostic before any ray tracing work could be scheduled. Finish with source/build validation and non-GPU metadata/capability checks only; do not require a smoke test that builds acceleration structures, creates ray tracing pipelines, creates shader tables, dispatches rays, or uses a ray tracing-capable graphics card.
```

Acceptance criteria:

- D3D12 and Vulkan expose equivalent RHI-level ray tracing capability fields and limits where the backend/device can report them.
- Unsupported devices report capability failure before any acceleration-structure, shader-table, pipeline, or dispatch work is attempted.
- Cooked ray tracing library metadata is consumed by runtime validation and diagnostics, not by a real render path yet.
- Runtime has a non-GPU validation path that checks cooked ray tracing metadata against RHI capability data and reports missing exports, hit groups, local parameters, payload size, attribute size, recursion depth, shader blob ids, codegen target, and binary format issues.
- No render pass, scene feature, frame graph pass, acceleration-structure build, ray tracing pipeline creation, shader table creation, or dispatch rays path is enabled by this phase.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShowcaseRuntime rhi_backend_parity_check rhi_backend_boundary_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```

### Phase 8: Authoring And Cook Contract Split

Goal: Deliver clear ownership for shader authoring declarations and cooked shader contracts outside backend runtime implementation.

Implementation prompt:

```text
Implement Phase 8 of the shader cooking architecture plan: split shader authoring and cooked-shader contracts from backend runtime implementation.

Start from `Engine/RHI/Public/Shaders`, tool includes, runtime package loading, and boundary validation scripts. Create a small shared contract owner for shader authoring declarations and cooked package structs. Candidate shape: `Engine/Shader/Public/Authoring` for registration macros and parameter structs, and `Engine/Shader/Public/Cooked` for package file structs, package utils, and registry records. RHI should consume cooked shader contracts and parameter layouts, but it should not be the conceptual owner of offline authoring macros.

Move code in small vertical slices: parameter struct descriptors, global shader registration, package layout builder, cooked package records, and package cache. Update boundary validation so tools can depend on the shared shader contract but not renderer/RHI private implementation. Finish by updating include paths and proving existing shader registrations still build from the new contract owner.
```

Acceptance criteria:

- Shader authoring macros no longer imply ownership by the RHI backend layer.
- ShaderCooker depends on the shared shader contract and backend compiler libraries, not renderer-private runtime code.
- RHI consumes cooked shader packages through the shared contract.
- Boundary checks enforce the new ownership split.
- Existing shader registrations still compile after include path updates.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCompiler ShowcaseRuntime ShowcaseEditor shader_compiler_boundary_check rhi_backend_boundary_check tools_architecture_boundary_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```

### Phase 9: Final Rename Sweep

Goal: Deliver the final naming model across code, scripts, validation, docs, logs, CLI help, and artifact output.

Implementation prompt:

```text
Implement Phase 9 of the shader cooking architecture plan: apply the shader cooking naming cleanup across code, scripts, docs, validation, and logs.

Start from CMake targets, scripts, editor recook launch code, validation scripts, docs, logs, and public command help. Rename the executable target from `ShaderCompiler` to `ShaderCooker` if the command is primarily package cooking. Rename namespaces and files that describe offline cooking accordingly. Rename backend-only interfaces to compiler-backend terms. Rename codegen target variables and CLI flags. Rename parameter names from layout/shader/name to binding/reflected/field where appropriate. Rename stats passes according to whether they report cooked shader packages or real pipeline state objects.

Update scripts, CMake targets, validation gates, documentation, editor recook command construction, and logs in the same phase. Make a clean cut: do not keep wrapper executables, target aliases, forwarding scripts, compatibility include headers, duplicated log fields, or deprecated command names once the tree builds. Finish by searching for stale production uses of the old names and leaving only intentional references in migration notes or historical docs.
```

Acceptance criteria:

- There is no broad production code path where "compiler" means package cooker.
- Backend compilation files can still use compile/compiler terms.
- User-facing help, logs, docs, and script names use the final terms.
- Editor recook still launches the renamed tool successfully.
- Boundary and freshness checks know the new target and file names.
- Old executable names, target aliases, forwarding scripts, and compatibility includes do not remain in production paths.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target ShaderCooker ShowcaseRuntime ShowcaseEditor sparkle_validation_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```

## Recommended Implementation Order

Do not start with the final rename. The least risky order is:

1. Phase 1, because it removes accidental raw-source fallback behavior before more features depend on it.
2. Phase 2, because stable blob identity makes later package, debug, graph, and PSO work clearer.
3. Phase 4, because dependency correctness protects cache and blob identity work.
4. Phase 5, because a real graph is useful once blob, reflection, layout, package, registry, and analysis nodes are explicit.
5. Phase 6, because required DXC/Slang backend descriptors and DXIL/SPIR-V format descriptors should be stable before future compiler backends or binary formats expand.
6. Phase 3, because pipeline layout artifacts need the blob identity to be final.
7. Phase 7, because runtime ray tracing should consume the cooked metadata contract after package identity is stable.
8. Phase 8, because ownership splitting is easier when contracts are known.
9. Phase 9, because the final names should follow the final architecture.

Phase 0 can run before or alongside Phase 1 as validation prep.

## Residual Risks

- Static embedded blobs can become a distraction. Keep them optional unless a bootstrap/runtime requirement appears, and never let them replace `.sparkshader` as the primary artifact.
- Full PSO cooking may need render pass, vertex input, blend/depth/raster, and material state decisions that are outside the shader cooker alone.
- Conditional include accuracy may depend on backend dependency output. A fallback lexer should be honest about limitations.
- Full ray tracing runtime completion crosses RHI, renderer, shader packages, frame graph scheduling, and backend capabilities. Phase 7 intentionally stops at capability and metadata readiness until ray tracing hardware and a product render path are available.
- The authoring contract split can touch many includes. Do it after package and naming contracts stop moving.
