# Shader Cooking Phase 0 Architecture Checklist

Date: 2026-05-19

This checklist records the current shader cooking surfaces and the terms that later shader cooking phases must preserve. Phase 0 is intentionally validation and documentation only; it does not move production code or change runtime behavior.

## Clean Replacement Rule

The shader cooking cleanup is a clean replacement, not a backward-compatible migration. A phase may start from the current name or path, but when that phase introduces the replacement it must remove the old production surface in the same change. Do not keep permanent CLI aliases, wrapper executables, forwarding scripts, compatibility include headers, duplicate serialized fields, duplicate log names, or runtime fallback paths for replaced shader cooking contracts.

Allowed temporary state: current names such as `ShaderCompiler` may remain only until their scheduled replacement phase lands.

Not allowed after the owning phase lands: old command names, old file names, ambiguous `--shader` production behavior, implicit raw-source package fallback, legacy manifest commands, compatibility target aliases, or old include paths that forward to the new owner.

## Current Inventory

| Surface | Current owner | Phase 0 note |
| --- | --- | --- |
| Tool target | `Tools/ShaderCompiler`, executable target `ShaderCompiler` | The tool currently owns offline cooking, package writing, registry writing, inspection, debug artifacts, and analysis commands. Final user-facing rename waits for Phase 9. |
| Command entrypoints | `cook`, `inspect-package`, `inspect-shader`, `list-backends`, `list-shaders`, `list-targets` | Commands still use current target names until the later CLI cleanup and rename phases. |
| Cooked runtime outputs | `Cooked/<ProjectName>/Shaders/ShaderPackageRegistry.sreg` and `Cooked/<ProjectName>/Shaders/Packages/<ShaderPackageKeyHex>.sparkshader` | These are the shipping shader runtime artifacts. |
| Debug artifact outputs | User-selected debug artifact directory via shader cooking settings/CLI | Debug artifacts are developer inspection side outputs and must not be required for runtime loading. |
| Analysis outputs | `PsoStatsPass` under `Tools/ShaderCompiler/Private/Analysis` | Current stats describe cooked shader packages, not full pipeline state objects. Phase 3 corrects the naming. |
| Local compile cache | `LocalDiskShaderArtifactStore`, `ShaderCacheKey`, `ShaderCompileOptionsHasher`, `IncludeClosureHasher` | Cache data is tool-private and not a shipping artifact. |
| Typed shader registration inputs | `Engine/RHI/Public/Shaders/Authoring` and `GlobalShaderRegistry` | Registration currently lives under RHI authoring headers; Phase 8 moves ownership after contracts stabilize. |
| Cook planning | `ShaderCookPlanner` reads `GlobalShaderRegistry` and builds package descriptions | Phase 1 removes accidental raw-source fallback behavior from production cooking. |
| Compiler backend registration | `BuiltinBackends`, `IShaderBackend`, backend registrations under `Backends/Dxc` and `Backends/Slang` | DXC and Slang are compiler backends, not the shader cooking system. Phase 6 replaces construction-based probing with descriptors. |
| Include dependency discovery | `ShaderSourcePreprocessor` and `IncludeClosureHasher` | Include scanning is currently duplicated and regex-oriented. Phase 4 creates one dependency scanner owner. |
| Cook graph execution | `DependencyGraph`, `CookNode`, `SerialCookExecutor`, `ShaderCookGraphBuilder`, `ShaderCookGraphExecutor` | The graph currently behaves as insertion-order work. Phase 5 turns it into a validated DAG. |
| Cooked package contracts | `Engine/RHI/Public/Shaders/CookedShaderPackage.h`, `CookedShaderPackageCache`, package writer/reader paths | Runtime consumes cooked package and registry contracts only; compiler backend details stay tool-private. |
| Runtime ray tracing placeholders | RHI ray tracing descriptors plus D3D12/Vulkan backend implementations | Cooked ray tracing metadata exists before full runtime backend completion. Phase 7 completes consumption and backend support. |

## Approved Terms

Use these terms in validation output, docs, command help, logs, package inspection, debug manifests, and future serialized reports:

- cook
- compiler backend
- codegen target
- binary format
- shader blob
- package
- registry
- reflection metadata
- binding metadata
- pipeline layout artifact
- debug artifact bundle
- analysis report
- local compile cache
- static export
- entry
- export
- binding name
- reflected name

## Output Categories

| Category | Runtime required | Expected location/form |
| --- | --- | --- |
| Cooked shader package | Yes | `.sparkshader` files under the cooked shader package root. |
| Shader package registry | Yes | `ShaderPackageRegistry.sreg` under the cooked shader root. |
| Shader blob catalog | Yes after Phase 2 | Records inside the package or registry; not a loose-file runtime contract. |
| Reflection metadata | Yes | Fixed package records plus string table entries. |
| Binding metadata | Yes | Fixed package records plus string table entries. |
| Pipeline layout artifact | Yes after Phase 3 | Cooked neutral layout intent or explicit translated backend layout records. |
| Debug artifact bundle | No | Optional developer output outside runtime package directories. |
| Analysis report | No | Optional CSV/JSON output outside runtime package directories. |
| Local compile cache | No | Tool-private cache storage. |
| Static export | No | Optional generated source files only when explicitly requested. |

Runtime-created native graphics objects are not primary cook outputs. D3D12 shader modules, root signatures, PSOs, Vulkan shader modules, descriptor set layouts, pipeline layouts, pipeline caches, and ray tracing shader tables are created or cached by RHI/Renderer from cooked records.

## Canonical Runtime Layout

```text
Cooked/<ProjectName>/Shaders/
	ShaderPackageRegistry.sreg
	Packages/
		<ShaderPackageKeyHex>.sparkshader
```

## Canonical Side-Output Layout

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

## Canonical Inspection And Log Fields

Use specific field names rather than overloaded labels:

- `PackageId`
- `ShaderPackageKey`
- `ShaderBlobId`
- `EntryPoint`
- `ExportName`
- `CompilerBackend`
- `CodegenTarget`
- `BinaryFormat`
- `debugArtifactRoot`
- `analysisOutput`
- `cacheDirectory`

Avoid ambiguous fields such as `shader`, `target`, `backend`, `blob`, or `path` when one of the names above is more precise.

## Boundary Gates

Phase 0 is guarded by these validation targets:

- `shader_compiler_boundary_check` keeps runtime modules from including or linking tool-only shader compiler APIs, DXC, Slang, SPIRV-Reflect, tool-private cooking headers, or shader compile option/result types. It also keeps `Tools/ShaderCompiler` free of renderer-private and high-level runtime implementation dependencies, and confines DXC/Slang/SPIRV-Reflect implementation tokens to backend-owned paths.
- `rhi_backend_boundary_check` keeps Renderer and high-level modules backend-neutral and routes graphics API work through public RHI contracts.
- `shader_package_parity_check` keeps cooked shader package selection consistent across runtime binary formats.

Later phases should extend these gates when a replacement lands so stale production aliases fail validation instead of lingering quietly.

Expected validation command:

```powershell
cmake --build build --config DevelopmentEditor --target shader_compiler_boundary_check rhi_backend_boundary_check shader_package_parity_check -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
```
