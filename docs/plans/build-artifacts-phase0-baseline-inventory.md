# Build Artifacts Phase 0 Baseline Inventory

Date: 2026-06-02

Status: inventory-only baseline for Phase 0 of `docs/plans/build-artifacts-release-architecture-roadmap.md`.

This pass documents current assumptions before changing output layout, packaging, or launcher behavior. No files were moved, no build behavior was changed, and final build/package validation was not run.

## Executive Summary

- Current build outputs are centered on the selected CMake binary tree: `build*/bin`, `build*/lib`, `build*/_deps`, `build*/Launcher`, `build*/Cooked`, and CMake generator files.
- The launcher discovers a build tree from `SPARKLE_BUILD_DIR`, `SPARKLE_BUILD_DIRECTORY`, or a fallback list: `build-ninja-msvc-qt`, `build`, `build-ninja-msvc`, `build-msvc`.
- Cooked content is split across two assumptions: launcher paths use the selected build tree, while `AssetCookerDiscovery` hardcodes root `build/Cooked` and `build/Cook`.
- `Check Dependencies` is host prerequisite inspection. `Sync Third Parties` is CMake configure/fetch. They are related but not duplicates.
- Dependency groups exist in CMake and launcher UI, but CMake still fetches through configure rather than a separate group-specific source sync workflow.
- There is no formal install/package/dist pipeline yet. Deployment today is build-tree output plus `windeployqt` for `SparkleLauncher` and `slang.dll` copy for `ShaderCompiler`.
- CI currently references `Scripts/CI/*.ps1`, but the scanned repo snapshot does not contain a `Scripts` directory.

## Freeze Rules For Migration

- Do not add new direct `build/bin`, `build/lib`, `build/_deps`, `build/Cooked`, or `build/Cook` assumptions outside existing path helpers while the migration is active.
- Do not treat a build output directory as a release package.
- Do not add package copy/deploy rules that bypass the future artifact manifest and product boundary model.
- Keep host prerequisites separate from source dependency groups and runtime redistributables in launcher terminology.
- Keep rebuild/recook actions optional wherever a ready-to-use binary or cooked output can be supplied later.

## Generated Local Directories And Ignore Coverage

| Path | Owner | Visibility | Category | Current ignore coverage | Likely future destination | Risk if unchanged |
| --- | --- | --- | --- | --- | --- | --- |
| `build/` | CMake/launcher | private local | build tree, dependency cache, launcher state | ignored by `/build/` | `artifacts/dev/build/<preset>` plus cache/state roots | default fallback can mix binaries, deps, cooked outputs, and launcher logs |
| `build-lite/` | CMake | private local | build tree | ignored by `/build-*/` | same preset-scoped build root model | easy to mistake as committable generated content |
| `build-msvc/` | CMake/launcher | private local | build tree | ignored by `/build-*/` | same preset-scoped build root model | stale cache can block regenerate when generator/platform changes |
| `build-ninja-msvc/` | CMake/launcher | private local | build tree | ignored by `/build-*/` | same preset-scoped build root model | launcher fallback order can select unexpected tree |
| `build-ninja-msvc-qt/` | CMake/launcher | private local | preferred build tree | ignored by `/build-*/` | same preset-scoped build root model | currently preferred by launcher whether or not it is the intended active tree |
| `build*/_deps/` | CMake FetchContent | private local | synced source dependency cache | covered by build ignore | source dependency cache, possibly group-scoped | cleaning/reconfiguring can redownload everything |
| `build*/Launcher/` | launcher | private local | settings, logs, freshness state | covered by build ignore | launcher local state root | launcher state disappears with full build-tree clean |
| `build*/Cooked/` | cooker/launcher/runtime | generated content | cooked output | covered by build ignore | `artifacts/dev/cooked/<project>` or package content root | launch readiness tied to build tree instead of product/runtime artifact |
| `build/Cook/Plans`, `build/Cook/Summaries` | AssetCooker | private generated | cook diagnostics | covered by `/build/` | cook report root under artifacts/logs/reports | hardcoded root `build` ignores selected build directory |
| `build*/Cache/Shaders` | shader cooker/launcher | private generated | shader cache/debug/transient outputs | covered by build ignore | cache root under artifacts/cache or local app data | cache clean and cook behavior are coupled to build tree |
| `.vs/`, `.vscode/` | IDEs | private local | IDE workspace state | ignored | local workspace state | clean scope can delete developer state |
| `logs/`, `Projects/**/logs/` | runtime/tools/launcher | private local | structured logs | ignored | artifacts/logs or local state | logs are not product-owned or package-owned |
| `Projects/**/build/` | project-local tools | private local | project build tree | ignored | avoid or move to unified artifacts | project-local build assumptions can bypass root artifact model |
| `imgui.ini`, `Projects/**/imgui.ini` | ImGui apps | private local | UI state | ignored only through maintenance scope, not explicit `.gitignore` rule observed | local app state | can appear as untracked unless ignored elsewhere |
| `artifacts/`, `dist/` | planned packaging | generated/release | future artifact and distribution roots | not currently ignored in `.gitignore` | canonical output/package roots | future phases should add ignore/ownership rules before generating them |

## CMake And Path Assumptions

| Finding | Owner | Dependency category | Visibility | Likely shipping destination | Risk if unchanged | References |
| --- | --- | --- | --- | --- | --- | --- |
| Global executable and DLL outputs use `${CMAKE_BINARY_DIR}/bin/<profile>` for multi-config and single-config builds. | CMake root | build output | private/dev, later product runtime | dev artifacts or product runtime package by component | every product shares one binary root; release packaging cannot tell tools from runtime/editor apps | `CMakeLists.txt:96-110` |
| Global static/import library outputs use `${CMAKE_BINARY_DIR}/lib/<profile>`. | CMake root | build output | private/dev | SDK/dev package, not runtime package | import/static libraries are mixed with local build internals | `CMakeLists.txt:96-110` |
| Many targets override output dirs back to `${CMAKE_BINARY_DIR}/bin` or `${CMAKE_BINARY_DIR}/lib`. | CMake modules/tools | build output | private/dev | target-owned artifact root | overrides can bypass future profile/product hierarchy | `Engine/*/CMakeLists.txt`, `Tools/*/CMakeLists.txt` |
| Build profiles are CMake configurations: `DevelopmentEditor`, `DevelopmentGame`, `DebugEditor`, `DebugGame`, `ShippingEditor`, `ShippingGame`. | CMake profiles | build output/profile | public build contract | manifest profile metadata | good foundation; must be carried into package/version manifests | `CMake/SparkleBuildProfiles.cmake` |
| Shared library toggle is `SPARKLE_BUILD_SHARED`, default OFF. | Engine CMake | build output ABI | private/dev, product runtime when ON | runtime/editor SDK split | DLL/public/private boundaries are not package-owned yet | `Engine/CMakeLists.txt:42` |
| Launcher build directory is selected from env vars or fallback dirs. | launcher core | build output locator | private/dev | local workspace config | unexpected active tree causes generator/platform mismatch and stale output use | `Tools/Launcher/SparkleLauncher/Private/Core/LauncherPaths.cpp` |
| Launcher binary lookup is `<build>/bin/<profile>/<tool>.exe`. | launcher core | build output | private/dev | product executable or bundled component root | launch/build readiness cannot find future precompiled fallback without new resolution layer | `Tools/Launcher/SparkleLauncher/Private/Core/LauncherPaths.cpp`, `ToolResolver.cpp` |
| Launcher state lives under `<build>/Launcher`. | launcher core | launcher local state | private local | local state root | state/logs are destroyed by full build clean and tied to one build tree | `LauncherPaths.cpp` |
| Launcher cooked output uses `<selected build>/Cooked/<project>`. | launcher core | cooked output | generated content | product/project cooked content package | mismatch with AssetCooker hardcoded `build/Cooked` | `LauncherPaths.cpp` |
| AssetCooker hardcodes `repositoryRoot/build/Cooked/<project>`. | AssetCooker | cooked output | generated content | product/project cooked content package | selecting `build-ninja-msvc-qt` still cooks into root `build`, causing launcher readiness failure | `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.cpp` |
| AssetCooker hardcodes `repositoryRoot/build/Cook/Plans` and `repositoryRoot/build/Cook/Summaries`. | AssetCooker | generated reports | private generated | cook report/log root | cook diagnostics are not tied to selected build/artifact root | `AssetCookerDiscovery.cpp` |
| ShaderCompiler requires `VULKAN_SDK` for DXC and Slang headers/libs/runtime. | ShaderCompiler CMake | host prerequisite/runtime redistributable | host + deployed DLL | tool runtime package | hidden prerequisite for shader workflows; `slang.dll` copied, DXC runtime handling is not package-modeled | `Tools/Shaders/ShaderCompiler/CMakeLists.txt` |
| SparkleLauncher requires Qt 6.8 Widgets and deploys Qt runtime with `windeployqt`. | Launcher CMake | host prerequisite/runtime redistributable | host + deployed runtime files | launcher package | deploy happens in build-tree output, not package manifest | `Tools/Launcher/SparkleLauncher/CMakeLists.txt` |
| CI invokes scripts under `Scripts/CI`. | CI | validation script | public CI contract | CI infra | `Scripts` directory was absent in scanned snapshot, so CI cannot run from a fresh sync | `.github/workflows/shader-cook.yml` |

## Launcher Action Inventory

| Operation id | Current label | Group | Real prerequisite categories | Output/effect category | Risk if unchanged | Recommended owner/name direction |
| --- | --- | --- | --- | --- | --- | --- |
| `toolchain.check` | Check Dependencies | Setup | host prerequisites | inspection only | name overlaps with dependency sync and can imply it downloads/fixes dependencies | Launcher setup: `Verify Host Environment` |
| `workspace.setup` | Sync Third Parties | Setup | host prerequisites, CMake configure, source dependency groups | CMake configure, FetchContent cache, build files when stale | name hides that it runs configure and may regenerate build files | Launcher setup/CMake: `Sync Source Dependencies` or `Configure Workspace` depending final behavior |
| `workspace.generate-solution` | Regenerate Solution | Setup | host prerequisites, selected build directory | generated build/IDE files | can fail on stale generator/platform cache; circular prompts have appeared in UX | Launcher setup/CMake: `Generate Project Files` |
| `workspace.open-solution` | Open IDE | Run | host IDE, current build files | launches VS/Rider | currently grouped as `Run` in operation definition but appears in workflow setup contexts | Launcher setup/development: `Open Workspace` |
| `workspace.build-all` | Build All | Build | host prerequisites, current build files, project selection, enabled dependency groups | launcher, project editor/runtime, enabled cook tools | monolithic action can obscure optional dependency groups | Launcher build: keep but show exact targets and optional groups |
| `launcher.build.self` | Build Launcher | Build | host prerequisites, current build files, Qt runtime redistributable | launcher executable and deployed Qt runtime in build tree | rebuild is not needed for users with precompiled launcher | Launcher build: optional refresh/update workflow |
| `project.build.editor` | Build Editor | Build | host prerequisites, current build files, project selection | editor target executable/libraries | no fallback precompiled lookup yet | Launcher build: editor component build |
| `project.build.runtime` | Build Runtime | Build | host prerequisites, current build files, project selection | runtime target executable/libraries | no fallback precompiled lookup yet | Launcher build: runtime component build |
| `cook.tools.prepare` | Build Cook Tools | Build | host prerequisites, current build files, enabled cook dependency groups | AssetCooker, TextureCooker, ShaderCompiler | disabled groups are blockers but should be capability unlocks | Launcher build/cook: `Build Cook Toolchain` |
| `cook.project` | Cook All | Cook | host prerequisites, current build files, project selection, cook tools, content/shader groups | cooked meshes/materials/textures/shaders | cooked root mismatch can break launch readiness | Launcher cook: project content preparation |
| `cook.shaders` | Cook Shaders | Cook | shader compiler group, ShaderCompiler executable, current build files, project selection | cooked shader packages, shader cache/debug artifacts | VULKAN_SDK/DXC/Slang prerequisite is not obvious enough in action terms | Launcher cook: shader content preparation |
| `cook.textures` | Cook Textures | Cook | content pipeline group, AssetCooker/TextureCooker, current build files, project selection | cooked texture assets | KTX is optional but currently only surfaced as setup dependency group | Launcher cook: texture content preparation |
| `cook.assets` | Cook Meshes | Cook | content pipeline group, AssetCooker, current build files, project selection | scenes, meshes, materials | label says meshes while action cooks scene/material assets too | Launcher cook: `Cook Scene Assets` |
| `project.open.editor` | Open Editor | Launch | editor executable, project marker, cooked meshes/textures/shaders | launches editor | local rebuild is required today even if future ready binary exists | Launcher run: prefer bundled editor when available |
| `project.open.runtime` | Open Runtime | Launch | runtime executable, project marker, cooked meshes/textures/shaders | launches runtime | local rebuild/recook required today | Launcher run: prefer bundled runtime and cooked outputs |
| `project.run.smoke` | Run Smoke Tests | Launch | executable, project marker, cooked outputs, smoke env/options | launches validation run | validation/runtime dependency distinction is not visible enough | Launcher validation/run |
| `project.run` | Run Project | Launch | executable, project marker, cooked outputs | launches selected target | duplicates editor/runtime actions unless kept as advanced/custom | Launcher run: advanced/custom launch |
| `quality.format` | Format Code | Maintenance | clang-format host tool | source formatting/check | scans Engine and Projects only; tool absence should be local prerequisite only | Launcher maintenance |
| `workspace.clean` | Clean Workspace | Maintenance | confirmation, generated output selection | deletes generated outputs | scopes are useful but can be too prominent for first-run users | Launcher maintenance/details |

## Dependency Groups And Feature Flags

| Group | Feature flag | Default | Dependencies | Unlocks | Category | Owner | Risk if unchanged |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Core Workspace | always enabled | ON | Dear ImGui v1.92.5, spdlog v1.14.1, Font Awesome Free Solid v6.7.1 | launcher, engine/editor/runtime builds, baseline configure | synced source dependency | CMake dependencies, launcher GUI | good grouping, but currently sync occurs through configure rather than explicit group selection |
| Content Pipeline | `SPARKLE_ENABLE_CONTENT_PIPELINE` | ON | cgltf v1.15, stb master, tinyexr v1.0.7, zlib v1.3.1, Assimp v5.4.3, Compressonator sparse clone | AssetCooker, TextureCooker, scene/mesh/material/texture cook | optional synced source dependency | CMake tools/dependencies, launcher GUI | fresh users pay for content tooling even if they only run prebuilt outputs |
| Shader Compiler | `SPARKLE_ENABLE_SHADER_COMPILER` | ON | SPIRV-Reflect vulkan-sdk-1.3.290.0 plus VULKAN_SDK-provided DXC/Slang | ShaderCompiler, shader cook | optional synced source dependency plus host SDK | CMake tools/dependencies, launcher GUI | host SDK/runtime redistributables are not separated from source dependency group |
| KTX Support | `SPARKLE_ENABLE_KTX_SUPPORT` | OFF | KTX-Software v4.3.2 | KTX2 texture container support | optional synced source dependency | CMake dependencies, launcher GUI | appears only as setup-level capability; per-texture workflow relationship is not strong yet |
| RHI D3D12 | `SPARKLE_RHI_WITH_D3D12` | ON | Windows SDK D3D12 libs, D3D12MemoryAllocator v3.1.0 | D3D12 runtime backend | host/system SDK plus source dependency | Engine RHI | D3D12MA fetched separately from global dependency groups |
| RHI Vulkan | `SPARKLE_RHI_WITH_VULKAN` | auto ON when Vulkan found | Vulkan SDK, VulkanMemoryAllocator v3.1.0 | Vulkan runtime backend | optional host SDK plus source dependency | Engine RHI | backend capability is not visible as a launcher dependency group yet |

## Current Apps, Tools, Libraries, DLLs, Symbols, And Generated Assets

| Class | Current items | Owner | Visibility | Dependency category | Likely shipping destination | Risk if unchanged |
| --- | --- | --- | --- | --- | --- | --- |
| Launcher app | `SparkleLauncher.exe` | Tools/Launcher | public user tool | build output + Qt runtime redistributable | launcher package/bootstrap component | only resolved from local build output today |
| Launcher internal library | `SparkleLauncherCore` | Tools/Launcher | private dev library | static library | not shipped separately unless SDK/internal tooling package | public headers expose useful operation API but package boundary is undefined |
| Launcher probe | `SparkleLauncherProbe.exe` | Tools/Launcher | private validation/dev tool | build output | CI/dev tools package | not separated from user launcher package |
| Showcase apps | `ShowcaseEditor.exe`, `ShowcaseRuntime.exe` | Projects/Showcase | public sample/editor/runtime | build output | editor/runtime product components | generated target names inferred by project/profile; no precompiled fallback resolver |
| Cook executables | `AssetCooker.exe`, `TextureCooker.exe`, `ShaderCompiler.exe`, historical `AssetConverter.exe` | Tools/Cooking, Tools/Shaders, Tools/Conversion | developer tools | build output, optional dependency groups | dev tools package, not runtime package | optional tools are built into same `bin` root as runtime/editor |
| Cook static libraries | `CookCommon`, `AssetCookerCore`, `TextureCookShared`, `TextureCookerMiniz`, `MeshCooker`, `MaterialCooker`, `SceneCooker`, `SourceImportAdapters` | Tools | private dev libs | static libraries | dev SDK/internal only | not product-boundary labeled in artifacts |
| Engine modules | `SparkleCore`, `SparklePlatform`, `SparkleRHI`, `SparkleRenderer`, `SparkleGameFramework`, `SparkleApplication`, `SparkleApplicationEditor`, `SparkleEditor` | Engine | public engine modules | static by default, DLLs when `SPARKLE_BUILD_SHARED=ON` | runtime/editor package plus SDK symbols/import libs when shared | CMake comments describe DLLs, but default is static; package role must be explicit |
| RHI backend libraries | `SparkleRHI_D3D12`, `SparkleRHI_Vulkan`, `SparkleD3D12MA`, `SparkleVMA` | Engine/RHI | private backend/dev libraries | static/interface libs | runtime component internals | backend dependency ownership not visible in launcher/package manifests |
| Third-party libraries | ImGui, spdlog, Assimp, zlib, Compressonator CMP_Core, KTX, SPIRV-Reflect, D3D12MA, VMA | CMake FetchContent | private/source dependency | synced source dependency | source dependency package/cache, not runtime package except redistributable DLLs where applicable | no manifest records source commit/version/group per output |
| Runtime redistributables | Qt DLLs/plugins via `windeployqt`, `slang.dll` copied next to `ShaderCompiler`, possible Vulkan/DXC runtime expectations | CMake deploy commands/toolchain | runtime files | runtime redistributable | launcher/tool runtime packages | deploy/copy occurs without package manifest or ownership table |
| Symbols | MSVC `.pdb`, static/import `.lib`, generated debug info | compiler/linker | private/dev, symbol package candidate | build output | symbols/debug package | `.pdb` ignored but not collected into symbol artifacts |
| Cooked scene assets | `.sscn` manifests/registries, cooked mesh/material outputs | AssetCooker, MeshCooker, SceneCooker, MaterialCooker | generated content | cooked output | project cooked content package | cooked root mismatch and no versioned package manifest |
| Cooked textures | `.stex` texture assets | TextureCooker/RHI | generated content | cooked output | project cooked content package | optional KTX/texture dependency relationship not fully encoded in package metadata |
| Cooked shader packages | cooked shader package files/registries with DXIL/SPIR-V records | ShaderCompiler/RHI | generated content | cooked output | project cooked shader package | host SDK/dependency group and backend variant requirements need manifest ownership |
| Shader cache/debug artifacts | shader cache, recook signals, debug artifacts/stats | ShaderCompiler/Application | private generated | cache/debug output | local cache or diagnostics artifact | clean scopes refer to build-tree cache, not artifact/cache root |
| Launcher logs/state | `Activity.json`, `ActionHistory.tsv`, `Settings.json`, operation `Latest.txt` logs | launcher | private local | local state/log output | local state, diagnostics package only on demand | tied to build directory; full clean removes UX history |

## Package, Deploy, Copy, And Validation Commands

| Command/rule | Owner | Category | Output | Risk |
| --- | --- | --- | --- | --- |
| Global CMake output directory rules | root CMake | build output routing | `bin`, `lib` under build tree | not package-aware |
| `windeployqt` post-build for `SparkleLauncher` | launcher CMake | runtime redistributable deploy | Qt runtime files in launcher target directory | deployed files are not manifest-owned |
| `cmake -E copy_if_different` for `slang.dll` | ShaderCompiler CMake | runtime redistributable deploy | `slang.dll` next to `ShaderCompiler` | Slang runtime is copied, DXC/runtime SDK handling remains implicit |
| FetchContent downloads/clones | CMake dependencies/RHI | synced source dependencies | `build*/_deps` | configure performs sync; group-specific selection is only via feature flags |
| `SparkleCookTools` custom target | Tools CMake | build aggregation | depends on enabled cook tools | good aggregator, but package ownership is undefined |
| CMake validation custom targets | CMake validation | build validation | validation target outputs/logs | useful boundaries, but not final package validation |
| CI `RunShaderCompilerCookCheck.ps1`, `RunShaderCacheCheck.ps1`, `RunShaderEditorPipelineCheck.ps1` | CI | validation scripts | CI build/cook output | scripts missing in scanned snapshot |
| Formal `install()`, CPack, artifact manifest, release zip assembly | none observed | package/release | none | release architecture does not exist yet |

## Owner And Consumer Map

| Output/component | Primary owner | Consumers | Visibility | Shipping destination |
| --- | --- | --- | --- | --- |
| Launcher executable and Qt runtime | Tools/Launcher | new users, developers | public | launcher bootstrap/user tools package |
| LauncherCore operation API | Tools/Launcher | GUI, shell, probe, tests | private/internal public headers | internal tooling SDK if needed |
| Build files and IDE workspace | CMake/launcher setup | developers, CI | private generated | never shipped |
| Engine runtime modules | Engine | editor/runtime apps | public runtime/dev | runtime package plus SDK/symbol package as needed |
| Editor modules | Engine/Editor/ApplicationEditor | editor app | public editor/dev | editor package |
| Showcase runtime/editor apps | Projects/Showcase | users, validation, portfolio demos | public sample product | sample app package |
| Cook tools | Tools/Cooking, Tools/Shaders | developers, CI, recook workflows | developer tool | dev tools package |
| Source dependency cache | CMake Dependencies | configure/build | private generated | optional source dependency bundle/cache |
| Cooked project output | AssetCooker/ShaderCompiler/TextureCooker | editor/runtime launch | public generated content | project content package |
| Logs/reports | launcher/tools/runtime | developers/support | private diagnostics | diagnostics bundle on demand |
| Symbols/import libs | compiler/linker | developers/debugging | private/dev | symbols/SDK package |

## Known Break Points When Output Roots Move

- `AssetCookerDiscovery` must stop hardcoding root `build/Cooked` and `build/Cook`.
- `LauncherPaths` must gain separate concepts for build tree, local launcher state, cooked output root, dependency cache root, and packaged runtime root.
- `ResolveSparkleToolPath` must support precompiled/bundled tools and apps before falling back to local build outputs.
- `LaunchOperations` must check bundled runtime/editor/cooked outputs before demanding local rebuild/recook.
- `MaintenanceOperations` clean scopes must target new artifact roots without deleting source dependency caches unless explicitly requested.
- `CMakeWorkflowProcessRequests` must avoid stale generator/platform caches by selecting a clean preset-specific binary directory or detecting mismatches before configure.
- `windeployqt` and `slang.dll` copy rules must become package-manifest entries or package assembly steps.
- CI must either restore `Scripts/CI` or update workflow references to real validation entry points.

## Phase 0 Completion Checklist

- Hardcoded output path assumptions were identified across CMake, launcher core/GUI, tools, docs, and CI.
- Launcher actions were categorized by host prerequisite, synced source dependency, runtime redistributable, build output, cooked output, and project selection.
- Dependency groups and feature flags were inventoried.
- Current package/deploy/copy commands were inventoried.
- Generated local directories and ignore coverage were inventoried.
- Apps, development tools, libraries, DLL/runtime redistributables, symbols, and generated assets were inventoried.
- Initial owner/consumer map was captured.
- No output roots were moved.
- No packaging redesign was implemented.
- Final build/package validation was not run.
