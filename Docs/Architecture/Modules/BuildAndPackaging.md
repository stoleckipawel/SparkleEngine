# Build And Packaging Capability Inventory

Status: capability snapshot; current build-system inventory, not a successful-build or release-package record

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; root/module CMake, profile/artifact contracts, dependency fetches, project discovery, tool/product membership, and install/test/CI searches inspected; evidence `S` only

Scope: build profiles, toolchains, options, dependency acquisition, targets, artifact layout, runtime staging, project discovery, checks, automation, tests, installation, and packaging

Owner: root/module `CMakeLists.txt` and `CMake/`; Launcher is the user-facing workspace orchestrator

Evidence and disposition: [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../Acceptance/FirstRelease.md)

## Build Contract

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `BUILD-001` | CMake baseline | Implemented path | CMake 3.20+, C/C++, C++20 required, compile commands exported. Current owned platform/backend sources make Windows the effective target. | `S` |
| `BUILD-002` | Six profiles | Implemented path | DebugEditor, DebugGame, DevelopmentEditor, DevelopmentGame, ShippingEditor, ShippingGame; DevelopmentEditor is default. State and target macros are emitted globally. | `S` |
| `BUILD-003` | Profile optimization/debug policy | Implemented path | MSVC Debug `/Od /Ob0 /Zi /RTC1`; Development `/O2 /Ob2 /Zi /DNDEBUG` plus full PDB/link optimization; Shipping `/O2 /Ob2 /DNDEBUG`. Clang-family Debug `-O0 -g`, Development `-O2 -g`, Shipping `-O3`. | `S` |
| `BUILD-004` | Compiler routes | Partial | CMake has MSVC and Clang/GNU flag branches; Launcher explicitly models MSVC and clang-cl on Windows. No current Linux/macOS platform product path exists. | `S` |
| `BUILD-005` | Static/shared engine modules | Implemented path | `SPARKLE_BUILD_SHARED` selects static default or engine DLLs; product targets copy declared runtime DLL owners beside executables in shared mode. | `S` |
| `BUILD-006` | Optional tool/features | Implemented path | Content pipeline ON, shader compiler ON, KTX support OFF, NVIDIA Streamline ON by default; strict warnings and sanitizer instrumentation are opt-in. RHI owns D3D12/Vulkan/NVAPI switches. | `S` |
| `BUILD-007` | Project discovery | Implemented path | Direct children of `Projects` with `.sparkle-project` are added; TemplateProject is skipped; editor/runtime target convention is supported. Showcase is the only current marked product. | `S` |
| `BUILD-008` | Target layering | Implemented path | Separate Core, Tasks, Platform, RHI common/backends/facade, Renderer, GameFramework, Editor, runtime/editor Application, Launcher core/GUI, import/cook/shader tools, and Showcase products. | `S` |
| `BUILD-009` | Host-tool exclusion | Implemented path | Launcher and cook/shader tool targets are excluded from default Game-profile builds; runtime Application source membership excludes editor/import/cook paths. | `S` |
| `BUILD-010` | Dependency acquisition | Capability-gated | FetchContent owns pinned/tagged ImGui, spdlog, Font Awesome, NVAPI/Streamline, cgltf, MikkTSpace, stb, tinyexr, zlib, Assimp, Compressonator, optional KTX, and SPIRV-Reflect; DXC/Slang/Vulkan/Qt discovery has separate host/SDK routes. | `S` |
| `BUILD-011` | Selective dependency sync | Implemented path | `SPARKLE_SYNC_SOURCE_DEPENDENCY` configures one cache and returns before workspace generation, enabling Launcher dependency repair. | `S` |

## Artifact And Delivery Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `BUILD-012` | Development artifact contract | Implemented path | Runnable outputs go to `artifacts/dev`: launcher, tools, projects, runtime-support, libraries; diagnostics and symbols have separate roots; optional validated artifact variant namespaces alternate build trees. | `S` |
| `BUILD-013` | Product layout | Implemented path | Showcase products emit to `artifacts/dev/projects/Showcase/editor/<Profile>` or `artifacts/dev/projects/Showcase/runtime/<Profile>`; Windows manifest is attached; project working directory is set for VS debugging. | `S` |
| `BUILD-014` | Runtime support staging | Implemented path | Shared Sparkle DLL owners and enabled NVIDIA Streamline DLLs copy beside project products; Launcher runs `windeployqt` and copies visual resources plus repository-root marker. | `S` |
| `BUILD-015` | Tool bundles | Implemented path | Tool executables/libraries/symbols have target-owned locations and declared runtime DLL ownership; Launcher preflights required support files/directories before cooking. | `S` |
| `BUILD-016` | Architecture check | Implemented path | `architecture_boundary_check` runs the repository CMake boundary script; required after Renderer/RHI boundary changes. | `S` |
| `BUILD-017` | Code-style targets | Implemented path | `code_style_check` and `code_style_format` route through PowerShell and require clang-format/clang-tidy 22.1.3 policy. | `S` |
| `BUILD-018` | Install/stage/package | Not found | No CMake `install(...)`, CPack, package manifest generator, archive/installer, signing, or clean-machine staged product target was found. Development artifact copying is not release packaging. | `S` |
| `BUILD-019` | Automated tests | Not found | No `enable_testing()` or `add_test()` occurs in current CMake. Existing validation is custom checks/manual evidence, not CTest coverage. | `S` |
| `BUILD-020` | CI | Not found | No tracked `.github` workflow or other inspected CI configuration exists. | `S` |
| `BUILD-021` | Root onboarding | Partial | Root has `LICENSE.txt`, AGENTS routing, and deep Docs, but no root `README.md` in this snapshot. Clean-user entry remains incomplete. | `S` |

## Vertical Build-To-Product Trace

CMake selects profile/options -> dependency owner resolves/fetches host and source requirements -> engine/tool/project targets are added -> profile target builds into private build tree while declared artifacts publish to `artifacts/dev` -> runtime DLL/vendor/Qt support is copied to target-owned directories -> Launcher checks freshness/readiness and launches from the project workspace. The trace currently stops before a formal Stage/Package/Install product.

## Explicit Non-Capabilities And Risks

- There is no current release package, installer, signed manifest, binary provenance/SBOM, CI gate, or automated test runner.
- FetchContent refs are not uniformly immutable commit hashes (some use tags/branches such as `master`); “pinned” must be verified per dependency before reproducibility claims.
- Sanitizer options exist but the current Windows/MSVC product route does not make all sanitizer modes usable.
- Development artifact staging can still rely on repository/project roots and does not prove redistributable clean-machine execution.
- This inventory did not configure or build any target; current build health remains unknown in this pass.
