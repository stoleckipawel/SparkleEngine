# Shader And Cook Artifact Validation Matrix

Status: Stage 31 artifact compatibility contract
Date: 2026-06-13
Last synchronized: 2026-06-13

## Purpose

This matrix treats shader packages, cooked assets, cook plans, and inspection reports as contracts. A renderer/RHI, tooling, or GameFramework change is not accepted just because it builds; the produced artifact must name its producer, schema owner, runtime consumer, inspection path, diagnostics, and sample load evidence.

Related contracts:

- [tooling-pipeline-contract.md](tooling-pipeline-contract.md)
- [game-framework-contract.md](game-framework-contract.md)
- [pass-authoring-contract.md](pass-authoring-contract.md)
- [pipeline-runtime-contract.md](pipeline-runtime-contract.md)
- [after/repository-threading-readiness.md](after/repository-threading-readiness.md)

## Artifact Matrix

| Artifact | Path pattern | Producer | Schema/version owner | Runtime consumer | Inspection or validation command | Failure diagnostics required | Stage 31 evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Shader package | `artifacts/dev/projects/<Project>/cooked/Shaders/Packages/*.sparkshader` | `ShaderCompiler cook` from `ShaderContracts` catalog and renderer shader registrations. | [CookedShaderPackage.h](../../Engine/RHI/Public/Shaders/CookedShaderPackage.h), version `2`, plus `Tools/Shaders/ShaderContracts` catalog records. | `CookedShaderPackageCache`, `PipelineRuntimeLibrary`, `RenderPassShaderRuntime`, RHI pipeline services. | `ShaderCompiler list-shaders --validate`, `ShaderCompiler cook --package Sky --target DxilSm66`, `ShaderCompiler inspect-package <package>`. | Package id, package key, backend format, shader stage, entry point, binding layout hash, compiler backend, feature mismatch, reason. | `list-shaders --validate` reported `17` typed registrations and `10` valid packages; Sky package inspection reported DXIL compute bytecode, reflection, and one pipeline layout. |
| Shader package registry | `artifacts/dev/projects/<Project>/cooked/Shaders/ShaderPackageRegistry.sreg` | `ShaderCompiler cook`. | Shader package registry writer and package manifest contract. | Runtime package lookup and shader reload/package cache paths. | `ShaderCompiler list-shaders --validate`; targeted package cook writes registry beside packages. | Registry path, package id, package key, missing package, duplicate package, backend format, reason. | Sky cook wrote `artifacts/dev/projects/Shared/cooked/Shaders/ShaderPackageRegistry.sreg`. |
| Cooked texture | `artifacts/dev/projects/<Project>/cooked/Textures/**/*.stex` | `TextureCooker cook-request-file`; requests usually produced by `MaterialCooker` or `AssetCooker collect-texture-requests`. | Texture cooker output contract plus [CookedTextureReference.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedTextureReference.h), texture reference version `2`. | Renderer texture manager and material texture-reference loading path. | `TextureCooker inspect-request-file <request-file>`, `TextureCooker cook-request-file <request-file> --summary <summary-json>`, runtime smoke that loads a materialized scene. | Asset id, source path, output path, texture group, color space, mip policy/filter, dimension, channel mask, reason. | DamagedHelmet request inspection found `6` requests; cook wrote `6/6` textures and summary `artifacts/diagnostics/cook/Summaries/stage31-damagedhelmet-texturecook-summary.json`. Showcase cooked output has `163` `.stex` files. |
| Cooked material | `artifacts/dev/projects/<Project>/cooked/Materials/*.smat` | `MaterialCooker`, usually dispatched through `AssetCooker cook-assets`. | [CookedMaterialAsset.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedMaterialAsset.h), version `3`; [CookedTextureReference.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedTextureReference.h), version `2`. | `MaterialAssetLoader`, scene material payload appenders, renderer material/texture systems. | `AssetCooker cook-assets Showcase DevelopmentEditor`; texture request inspection validates referenced texture handoff. | Asset id, path, schema version, material record kind, texture reference, expected feature, reason. | Showcase cook produced material outputs and runtime smoke loaded Sponza with `25` materials; cooked output has `51` `.smat` files. |
| Cooked mesh | `artifacts/dev/projects/<Project>/cooked/Meshes/*.smsh` | `MeshCooker`, usually dispatched through `AssetCooker cook-assets`. | [CookedMeshAsset.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedMeshAsset.h), version `5`. | `MeshAssetLoader`, scene mesh payload appenders, renderer mesh scene data builders, renderer GPU mesh cache. | `AssetCooker cook-assets Showcase DevelopmentEditor`; runtime smoke that loads cooked scene payloads. | Asset id, path, schema version, mesh record kind, vertex/index layout expectation, feature, reason. | Showcase cook produced Sponza with `103` mesh asset refs; runtime smoke loaded `103` meshes and renderer prepared `103` renderable instances. Cooked output has `136` `.smsh` files. |
| Cooked scene manifest | `artifacts/dev/projects/<Project>/cooked/SceneManifests/**/*.sscn` | `SceneCooker`, usually dispatched through `AssetCooker cook-assets`. | [CookedSceneManifest.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedSceneManifest.h), version `9`. | `SceneManifestLoader`, `SceneManifestValidator`, `SceneAssetPayloadLoader`, GameFramework level/scene systems, renderer scene snapshots. | `AssetCooker cook-assets Showcase DevelopmentEditor`; runtime smoke that loads a cooked startup level. | Asset id, path, schema version, manifest section, referenced mesh/material/skeleton/animation id, missing registry entry, reason. | Showcase cook wrote `9` scene manifests during dispatch; runtime smoke loaded `Sponza/Sponza` from the cooked registry. Cooked output has `12` `.sscn` files. |
| Cooked animation | `artifacts/dev/projects/<Project>/cooked/Animations/*.sanim` | `SceneCooker` animation writer, usually through `AssetCooker cook-assets`. | [CookedAnimationAsset.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h), version `1`. | `AnimationAssetLoader`, animation payload appenders, runtime animation systems, future renderer skinning handoff. | `AssetCooker cook-assets Showcase DevelopmentEditor`; sample animated scene cook/load evidence. | Asset id, path, schema version, clip/channel/sampler record, skeleton reference when present, reason. | AssetCooker import summaries reported animations imported for `CesiumMan` and `DiffuseTransmissionPlant`; cooked output has `3` `.sanim` files. |
| Cooked skeleton | `artifacts/dev/projects/<Project>/cooked/Skeletons/*.sskel` | `SceneCooker` skeleton writer, usually through `AssetCooker cook-assets`. | [CookedSkeletonAsset.h](../../Engine/GameFramework/Public/Assets/Cooked/CookedSkeletonAsset.h), version `1`. | `SkeletonAssetLoader`, skeleton payload appenders, runtime skeleton systems, future renderer skinning handoff. | `AssetCooker cook-assets Showcase DevelopmentEditor`; sample skeletal scene cook/load evidence. | Asset id, path, schema version, joint hierarchy record, inverse-bind data, referenced animation, reason. | AssetCooker cooked skeletal sample content; cooked output has `2` `.sskel` files. |
| Scene asset registry | `artifacts/dev/projects/<Project>/cooked/SceneAssetRegistry.sreg` | `AssetCooker` / scene asset registry writer during scene cook dispatch. | Scene asset registry writer and cooked scene manifest identity contract. | GameFramework level startup and `SceneAssetPayloadLoader`. | `AssetCooker cook-assets Showcase DevelopmentEditor`; runtime smoke startup load. | Registry path, scene asset id, manifest path, duplicate id, missing id, reason. | Runtime smoke loaded startup scene asset `Sponza/Sponza` through the registry. Stage 32 removed the broken Bistro level descriptor that referenced a missing registry entry. |
| Project cook plan | `artifacts/diagnostics/cook/Plans/<Project>.assetcookplan.txt` | `AssetCooker` discovery and planning. | AssetCooker plan schema `asset-cooker-plan-v1`. | LauncherCore, CI/local validation, cook evidence readers. | `AssetCooker cook-assets Showcase DevelopmentEditor`. | Project, configuration, tool configuration, stage list, scene count, engine/project scene count, overrides, reason. | Showcase dispatch wrote `artifacts/diagnostics/cook/Plans/Showcase.assetcookplan.txt` with `9` scenes, `4` engine scenes, `5` project scenes. |
| AssetCooker summary | `artifacts/diagnostics/cook/Summaries/*-assetcook-summary.json` | `AssetCooker` dispatcher. | AssetCooker summary schema `asset-cooker-summary-v1`. | LauncherCore, CI/local validation, evidence docs. | `AssetCooker cook-assets Showcase DevelopmentEditor`. | Project, configuration, stage, status, elapsed time, output path, failure reason. | Showcase dispatch wrote `artifacts/diagnostics/cook/Summaries/Showcase-DevelopmentEditor-assetcook-summary.json`. |
| TextureCooker summary | `artifacts/diagnostics/cook/Summaries/*texturecook-summary.json` | `TextureCooker cook-request-file`. | TextureCooker timing/report schema. | AssetCooker/Launcher/CI evidence readers and texture pipeline diagnosis. | `TextureCooker cook-request-file <request-file> --summary <summary-json>`. | Request file, asset id, source path, output path, texture group, status, elapsed time, reason. | DamagedHelmet texture cook wrote `artifacts/diagnostics/cook/Summaries/stage31-damagedhelmet-texturecook-summary.json` with `6/6` cooked textures. |

## Schema Change Rule

Any change to an artifact schema must update the entire row, not only the local reader or writer.

| Changed artifact | Required paired updates |
| --- | --- |
| `.sparkshader` or shader registry | `ShaderContracts`, renderer shader registrations/catalog, `ShaderCompiler` list/cook/inspect, `CookedShaderPackageCache`, `PipelineRuntimeLibrary`, runtime smoke package-load evidence. |
| `.stex` or texture request data | `TextureCooker`, material texture reference writer, renderer texture manager/upload path, request inspection/cook command, runtime smoke that reaches textured materials. |
| `.smat` | `MaterialCooker`, texture reference generation, `MaterialAssetLoader`, scene material payload appender, renderer material/texture consumers, Showcase cook/load evidence. |
| `.smsh` | `MeshCooker`, `SceneCooker` mesh references, `MeshAssetLoader`, scene mesh payload appender, renderer mesh scene builders/cache, Showcase cook/load evidence. |
| `.sscn` or scene registry | `SceneCooker`, scene registry writer, `SceneManifestLoader`, level startup/switching, renderer scene snapshot builders, sample smoke/load evidence. |
| `.sanim` | `SceneCooker` animation writer, `AnimationAssetLoader`, animation payload appender, runtime animation owner, animated sample load evidence. |
| `.sskel` | `SceneCooker` skeleton writer, `SkeletonAssetLoader`, skeleton payload appender, runtime skeleton owner, skeletal sample load evidence. |
| Cook plan or summary | `AssetCooker` planner/dispatcher, LauncherCore operation evidence, CI/local validation docs, failure report fields. |

## Stage 31 Validation Evidence

Commands run on 2026-06-13:

| Command | Result |
| --- | --- |
| `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1` | Passed. |
| `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target TextureCooker -- /nologo /v:minimal /m:1` | Passed. |
| `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target SparkleGameFramework -- /nologo /v:minimal /m:1` | Passed. |
| `ShaderCompiler.exe list-shaders --validate` | Passed; `17` typed registrations and `10` valid packages. |
| `ShaderCompiler.exe cook --package Sky --target DxilSm66` | Passed; wrote Sky package and shader package registry under `artifacts/dev/projects/Shared/cooked/Shaders`. |
| `ShaderCompiler.exe inspect-package artifacts/dev/projects/Shared/cooked/Shaders/Packages/82428E195CE838B6.sparkshader` | Passed; package has one DXIL compute blob, reflection, and pipeline layout. |
| `TextureCooker.exe inspect-request-file artifacts/diagnostics/cook/Temp/stage30-damagedhelmet-texture-requests.txt` | Passed; request file has `6` texture requests. |
| `TextureCooker.exe cook-request-file artifacts/diagnostics/cook/Temp/stage30-damagedhelmet-texture-requests.txt --summary artifacts/diagnostics/cook/Summaries/stage31-damagedhelmet-texturecook-summary.json` | Passed; cooked `6/6` textures. |
| `AssetCooker.exe cook-assets Showcase DevelopmentEditor` | Passed; planned and dispatched `9` scenes, wrote cook plan and summary. |
| `ShowcaseRuntime.exe` with `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_RHI_BACKEND=D3D12`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, and `SPARKLE_SMOKE_FRAME_LIMIT=60` from `Projects/Showcase` | Passed; loaded cooked `Sponza/Sponza`, prepared `103` renderable mesh instances, loaded `25` materials, reported active shader runtimes, and reported `frameGraphUnresolvedBarrierWarnings=0`. |

Runtime smoke note: Stage 31 found `Bistro/BistroExterior` missing from `SceneAssetRegistry.sreg`. Stage 32 resolved this by removing the broken `Bistro.level` runtime descriptor until a real Bistro scene source and cook/load evidence exist. The Stage 32 launcher-shaped D3D12 runtime smoke with level switching enabled completed all `5/5` valid switch targets.

## Acceptance

Stage 31 is accepted when:

- Every artifact row names producer, schema owner, runtime consumer, validation command, and evidence.
- Shader package enumeration validates renderer packages without building the editor.
- Texture, material, mesh, scene, animation, skeleton, and cook-plan outputs are tied to GameFramework/Renderer runtime expectations.
- Schema changes are rejected unless producer, loader, runtime consumer, inspector/report, and sample evidence move together.
