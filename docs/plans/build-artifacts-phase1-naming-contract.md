# Build Artifacts Phase 1 Naming Contract

Date: 2026-06-03

Status: Phase 1 naming and shared configuration contract. Existing output behavior is preserved. Final build/package validation was not run.

## Root Names

| Name | CMake variable | Current path | Meaning | Phase 1 behavior |
| --- | --- | --- | --- | --- |
| Repository root | `SPARKLE_REPOSITORY_ROOT` | `${CMAKE_SOURCE_DIR}` | Source checkout root containing `Engine/`, `Tools/`, `Projects/`, `CMake/`, and docs. | Naming only. |
| Build root | `SPARKLE_BUILD_ROOT` | `${CMAKE_BINARY_DIR}` | Active generated CMake build tree: generator files, intermediates, current `bin/`, `lib/`, `_deps/`, launcher state, and current cooked outputs. | Existing output behavior remains. |
| Artifact root | `SPARKLE_ARTIFACT_ROOT` | `${CMAKE_SOURCE_DIR}/artifacts` | Future development artifact root for product-aware launcher, tools, project binaries, cooked outputs, diagnostics, and symbols. | Declared and ignored, not used for outputs yet. |
| Distribution root | `SPARKLE_DIST_ROOT` | `${CMAKE_SOURCE_DIR}/dist` | Future assembled package root for versioned release zips/manifests. | Declared and ignored, no package assembly yet. |

## Dependency Categories

| Category | Launcher wording | Meaning | Owner |
| --- | --- | --- | --- |
| `host-prerequisite` | Host prerequisite | Machine-installed tools such as Visual Studio C++ tools, Qt MSVC kit, CMake, Git, Windows SDK, Vulkan SDK, and optional ClangCL/Rider/clang-format. | Launcher setup/toolchain detection. |
| `source-dependency-group` | Source dependency group | FetchContent-managed source groups such as Core Workspace, Content Pipeline, Shader Compiler, KTX Support, and backend memory allocators. | CMake dependency configuration plus launcher dependency UI. |
| `runtime-redistributable` | Runtime redistributable | Runtime files deployed beside apps/tools, such as Qt DLLs/plugins or `slang.dll`. | Package/deploy rules. |
| `build-output` | Build output | Executables, DLLs, libraries, import libs, symbols, generator outputs, and intermediate outputs produced by local builds. | CMake and build tools. |
| `cooked-output` | Cooked output | Generated runtime content: cooked scene assets, textures, shader packages, registries, and cook summaries. | Cook tools. |
| `project-selection` | Project selection | Selected `.sparkle-project` workspace and project-specific target names. | Launcher project model and project CMake. |

`Verify Host Environment` owns host prerequisites. `Sync Source Dependencies` owns source dependency groups. Runtime redistributables should be discussed only in package/deploy/build-output contexts, not as source dependency sync requirements.

## Product Visibility

| Visibility | Meaning | Examples |
| --- | --- | --- |
| `public` | User-facing or externally consumable product surface. | Launcher executable, sample editor/runtime, public engine runtime modules, cooked sample content. |
| `internal` | Shared implementation surface used across Sparkle components but not a standalone user product. | Launcher operation API, engine module internals, validation helpers. |
| `private` | Local-only, generated, or implementation detail. | CMake caches, `_deps`, launcher logs/state, project IDE state, object files. |

## Binary And Asset Types

| Type | Meaning | Examples |
| --- | --- | --- |
| `app` | Launchable user-facing executable. | `SparkleLauncher.exe`, `ShowcaseEditor.exe`, `ShowcaseRuntime.exe`. |
| `developer-tool` | Launchable development or CI executable. | `AssetCooker.exe`, `TextureCooker.exe`, `ShaderCompiler.exe`, `SparkleLauncherProbe.exe`. |
| `runtime-dll` | Runtime library required by an app or tool. | Qt DLLs/plugins, `slang.dll`, Sparkle DLLs when shared builds are enabled. |
| `plugin-dll` | Dynamically discovered extension/plugin library. | Reserved for future plugin systems. |
| `import-library` | Linker import library for a DLL. | MSVC `.lib` import libraries. |
| `static-library` | Static link library or private build library. | `SparkleLauncherCore`, cooker libraries, static engine modules. |
| `symbol-file` | Debug symbol output. | `.pdb` files. |
| `generated-asset` | Generated content or metadata consumed by runtime/editor/tools. | Cooked scene assets, `.stex` textures, cooked shader packages, manifests. |

## Release And Package Identity

| Name | CMake variable | Default | Meaning |
| --- | --- | --- | --- |
| Release channel | `SPARKLE_RELEASE_CHANNEL` | `dev` | One of `dev`, `preview`, `rc`, or `release`. |
| Package version | `SPARKLE_PACKAGE_VERSION` | `0.0.0-dev` | Semver-style package version for future manifests and package names. |
| Package platform | `SPARKLE_PACKAGE_PLATFORM` | `windows-x64` | Platform token used in package names and manifests. |

Reserved package ids:

- `sparkle-launcher`
- `sparkle-runtime`
- `sparkle-editor`
- `sparkle-dev-tools`
- `sparkle-symbols`
- `sparkle-dependencies`

Recommended future package filename pattern:

```text
<package-id>-<version>-<channel>-<platform>.zip
```

Example:

```text
sparkle-launcher-0.3.0-preview-windows-x64.zip
```

## Repository Navigation Contract

| Repository path | Meaning |
| --- | --- |
| `Engine/` | Runtime/editor engine source modules and shipped engine surfaces. |
| `Tools/Launcher/` | Launcher app, launcher core workflow API, GUI, shell, and probes. |
| `Tools/Cooking/` | Cook tool source and private cook libraries. |
| `Tools/Shaders/` | Offline shader compiler source and shader cook implementation. |
| `Tools/Import/` | Source import adapters and import diagnostics. |
| `Tools/Conversion/` | Legacy/conversion tool source until folded into modern cook workflows. |
| `Projects/` | Sparkle projects discovered by `.sparkle-project`. |
| `CMake/` | Shared CMake contracts, build profiles, dependency configuration, and validation targets. |
| `docs/` | Architecture plans, implementation handoffs, and supporting documentation. |
| `build*/` | Ignored generated build roots. |
| `artifacts/` | Ignored future development artifact root. |
| `dist/` | Ignored future package assembly root. |

## Package Navigation Contract

Future package roots should be navigable by product role, not by accidental build folder:

```text
<package-root>/
  Launcher/
  Runtime/
  Editor/
  Tools/
  Content/
  Redist/
  Symbols/
  Manifests/
  Docs/
```

Meaning:

- `Launcher/` contains the ready-to-run launcher and its runtime redistributables.
- `Runtime/` contains runtime apps/modules that can run without local rebuild.
- `Editor/` contains editor apps/modules that can run without local rebuild.
- `Tools/` contains developer tools used for optional rebuild/recook workflows.
- `Content/` contains cooked outputs bundled for first-run use.
- `Redist/` contains redistributable runtime components owned by package rules.
- `Symbols/` contains symbol files and debug metadata packages.
- `Manifests/` contains package, dependency, build, release, and file manifests.
- `Docs/` contains release notes and package usage notes.

## Phase 1 Implementation Notes

- CMake now includes `CMake/SparkleArtifactContract.cmake`.
- `.gitignore` now covers `/artifacts/` and `/dist/`.
- Launcher public naming constants are available in `SparkleLauncher/ArtifactNaming.h`.
- Launcher labels now use `Verify Host Environment`, `Sync Source Dependencies`, `Generate Project Files`, and `Open Workspace`.
- The scene-asset cook action now uses `Cook Scene Assets` because it prepares scene, mesh, and material assets.
- No output roots were moved.
- No final package assembly was created.
- Final build/package validation was not run.
