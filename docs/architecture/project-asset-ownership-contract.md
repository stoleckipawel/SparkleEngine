# Project And Engine Asset Ownership Contract

Status: Stage 32 project and engine asset contract
Date: 2026-06-13
Last synchronized: 2026-06-13

## Purpose

This contract makes engine assets, project content, generated outputs, and validation evidence explicit source-root responsibilities. Project samples should behave like executable evidence for cook, load, render, and launcher workflows. Engine assets should be small built-in runtime inputs with owners and validation, not a catch-all for sample data, renderer shaders, or temporary files.

Related contracts:

- [repository-system-map.md](repository-system-map.md)
- [after/repository-target-folder-architecture.md](after/repository-target-folder-architecture.md)
- [tooling-pipeline-contract.md](tooling-pipeline-contract.md)
- [validation-workflow-contract.md](validation-workflow-contract.md)
- [artifact-validation-matrix.md](artifact-validation-matrix.md)
- [after/repository-threading-readiness.md](after/repository-threading-readiness.md)

## Root Ownership

| Root | Current role | Accepted policy | Must not contain |
| --- | --- | --- | --- |
| `Engine/Assets/Meshes` | Small engine-owned mesh fixtures such as cube and instancing samples used by cook/load validation. | Engine built-in source assets. Every file here must be small enough and stable enough to serve engine validation, not project decoration. | Project sample scenes, generated cooked outputs, or temporary validation captures. |
| `Engine/Assets/Textures/Defaults` | Engine-owned fallback textures. | Built-in defaults used by renderer/runtime fallback paths. | Project textures or captured output. |
| `Engine/Assets/Textures/Sky` | Engine-owned sky/environment candidates. | Built-in rendering inputs until a dedicated environment asset owner exists. | Project-only skyboxes or validation captures. |
| `Engine/Assets/Shaders` | Current engine shader source root for RHI fixtures, shared includes, and renderer pass shaders. | Transitional shader source root. New ownership should prefer `Engine/RHI/Shaders` for generic RHI fixtures and `Engine/Renderer/Shaders` for renderer pass shaders when those moves are done deliberately with ShaderCompiler/package validation. | Project shader overrides, generated shader packages, local shader cache, or temporary shader experiments. |
| `Projects/Showcase/Assets` | Showcase source content for cook/load/render evidence. | Project-owned source content used by AssetCooker, TextureCooker, GameFramework, Renderer, and launcher-shaped smoke. | Generated cooked outputs, logs, validation captures, tool internals, or backend-private data. |
| `Projects/Showcase/Levels` | Runnable level descriptors. | Only levels whose scene asset references resolve through cooked scene registry or intentionally empty levels with no missing asset references. | Descriptors for source content that is not cookable today. |
| `Projects/Showcase/Src` | Project runtime/editor entry points. | Public-engine API usage only. | Engine private headers, tool-private headers, generated code, or validation artifacts. |
| `Projects/Showcase/logs`, `Projects/Showcase/StreamlineLogs`, `Projects/Showcase/imgui.ini` | Local generated state. | Ignored/local-only output, not architecture or source. | Durable source, project configuration, or evidence that should live under `artifacts/validation`. |
| `artifacts/validation` | Generated validation evidence. | Logs, captures, reports, and index files from validation runs. | Hand-authored source assets or architecture decisions. |

## Showcase Evidence Set

Showcase is accepted as the reusable evidence project for current cook/load/render workflows when these source assets have matching cooked/runtime proof:

| Evidence level/source | Source path | Runtime asset id | Stage 32 policy |
| --- | --- | --- | --- |
| Sponza | `Projects/Showcase/Assets/Meshes/Sponza/Sponza.gltf` | `Sponza/Sponza` | Primary large static mesh/material/runtime smoke evidence. |
| ABeautifulGame | `Projects/Showcase/Assets/Meshes/ABeautifulGame/ABeautifulGame.gltf` | `ABeautifulGame/ABeautifulGame` | Project multi-material cook/load evidence. |
| DamagedHelmet | `Projects/Showcase/Assets/Meshes/DamagedHelmet/DamagedHelmet.gltf` | `DamagedHelmet/DamagedHelmet` | Texture request and material/texture validation evidence. |
| CesiumMan | `Projects/Showcase/Assets/Meshes/CesiumMan/CesiumMan.gltf` | `CesiumMan/CesiumMan` | Animation/skeleton source evidence. |
| DiffuseTransmissionPlant | `Projects/Showcase/Assets/Meshes/DiffuseTransmissionPlant/DiffuseTransmissionPlant.gltf` | `DiffuseTransmissionPlant/DiffuseTransmissionPlant` | Camera/light/animation cook evidence. |
| Empty | `Projects/Showcase/Levels/Empty.level` | none | Empty runtime host/control evidence. |

The former `Projects/Showcase/Levels/Bistro.level` was removed because it referenced `Bistro/BistroExterior` without a cookable source scene. The raw Bistro folder remains non-runnable source/reference content until a real scene source is added and paired with cook/load evidence.

## Data Transfer Contracts

| Boundary | Transfer shape | Required evidence |
| --- | --- | --- |
| Project source content to tools | Project cook plan, focused cooker job requests, source paths, importer id, and diagnostics. | `AssetCooker cook-assets Showcase DevelopmentEditor` plan and summary. |
| Engine built-in source assets to tools | Engine scene entries in the cook plan with origin `Engine`. | Cook plan engine scene count and cooked scene manifests. |
| Cooked outputs to runtime | Versioned cooked artifacts and registries under `artifacts/dev/projects/<Project>/cooked`. | [artifact-validation-matrix.md](artifact-validation-matrix.md) rows and runtime smoke logs. |
| Runtime validation to evidence | Logs, captures, summaries, and reports under `artifacts/validation` or `artifacts/diagnostics`. | Launcher-shaped smoke command details in [validation-workflow-contract.md](validation-workflow-contract.md). |

## Guardrails

Positive guardrails:

- Treat `Projects/Showcase` as executable evidence, not a file dump.
- Keep source assets in durable project/engine roots and generated artifacts under `artifacts`.
- Keep project source code using public engine APIs only.
- Move shader/data roots only with paired tool/runtime validation.

Negative guardrails:

- Do not register a `.level` file unless its scene asset references are cookable and loadable.
- Do not use `Engine/Assets` for project samples, generated shader packages, local logs, or validation captures.
- Do not commit generated `logs`, `StreamlineLogs`, `imgui.ini`, cooked outputs, or smoke captures as source.
- Do not make Projects include tool-private or backend-private implementation details.

## Stage 32 Evidence

Stage 32 inventory found:

- `Engine/Assets/Meshes`: engine built-in source mesh fixtures.
- `Engine/Assets/Textures`: default and sky/environment textures.
- `Engine/Assets/Shaders`: current transitional shader source root.
- `Projects/Showcase/Assets`: project source scenes/textures.
- `Projects/Showcase/Levels`: runnable level descriptors.
- `Projects/Showcase/logs`, `Projects/Showcase/StreamlineLogs`, and `Projects/Showcase/imgui.ini`: local generated state.

Stage 32 cleanup:

- Removed `Projects/Showcase/Levels/Bistro.level` because it referenced `Bistro/BistroExterior` without a source scene for AssetCooker.
- Removed Bistro from the launcher startup-level choices.
- Kept raw Bistro source/reference files under project assets until a real scene source and evidence path exist.

Stage 32 validation:

- `SparkleLauncher` built after startup-level option cleanup.
- `AssetCooker cook-assets Showcase DevelopmentEditor` passed and produced the same `9` cookable source scenes.
- Launcher-shaped D3D12 `ShowcaseRuntime` smoke from `Projects/Showcase` with level switching enabled completed all `5/5` valid switch targets and exited `0`.
