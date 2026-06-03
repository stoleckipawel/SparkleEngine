# Phase 5 Release Assembly And Manifests Handoff

Date: 2026-06-03

Scope:

- add a reviewable release assembly model under `dist/releases/<version>`
- consume generated `artifacts/` outputs without treating them as publish-ready releases
- stage bundled runtime components for launcher, sample editor/runtime, and cooked sample assets
- generate package manifests, bundled runtime component manifests, checksums, and release notes
- keep final build/package validation deferred to Phase 6

Implemented Release Roots:

| Domain | Root |
| --- | --- |
| Release root | `dist/releases/<version>/` |
| Runtime package | `dist/releases/<version>/sparkle-runtime-<version>-<channel>-<platform>/` |
| Symbols package | `dist/releases/<version>/sparkle-symbols-<version>-<channel>-<platform>/` |
| Symbols archive | `dist/releases/<version>/sparkle-symbols-<version>-<channel>-<platform>.zip` |

Runtime Package Navigation:

| Package area | Purpose |
| --- | --- |
| `SparkleLauncher.exe` | package-root launcher contract |
| `Apps/ShowcaseEditor/` | bundled sample editor/runtime app surface |
| `Apps/ShowcaseRuntime/` | bundled sample standalone runtime surface |
| `Projects/Showcase/Cooked/` | pre-cooked sample project assets |
| `Projects/Shared/Cooked/` | pre-cooked shared assets |
| `redist/` | runtime redistributable staging area |
| `manifests/` | release, build, dependency, component, package, file, and checksum manifests |
| `RELEASE_NOTES.md` | release notes template |
| `licenses/` | license files |

Assembly Target:

- `sparkle_release_assembly` is a review-only CMake target.
- The target runs `CMake/SparkleReleaseAssembly.cmake`.
- The script consumes `SPARKLE_ARTIFACT_ROOT` and writes `SPARKLE_DIST_ROOT`.
- Package assembly is explicit and does not run as part of normal build, cook, or launcher actions.
- The Sparkle Launcher Package workflow now describes the CMake target but keeps one-click package execution disabled until Phase 6.

Manifest Outputs:

| Manifest | Purpose |
| --- | --- |
| `manifests/sparkle-release-manifest.json` | version, channel, platform, package identity, source root, publish readiness flag |
| `manifests/sparkle-build-manifest.json` | build configuration plus placeholders for Phase 6 commit, toolchain, and Qt kit capture |
| `manifests/sparkle-dependency-manifest.json` | dependency pack naming, dependency groups, and runtime redistributable expectations |
| `manifests/sparkle-bundled-runtime-components.json` | staged component status, source, destination, package kind, visibility, binary type, producer, regeneration path |
| `manifests/sparkle-package-manifest.json` | package kind, visibility, package-root launcher contract, navigation contract, inclusion rules |
| `manifests/sparkle-package-files.json` | runtime package file list with sizes and SHA-256 hashes |
| `manifests/SHA256SUMS.txt` | runtime package checksum list |
| `sparkle-symbols-files.json` | symbols package file list with sizes and SHA-256 hashes |
| `SHA256SUMS.txt` | symbols package checksum list |

Bundled Runtime Components:

| Component | Artifact source | Package destination | Regeneration |
| --- | --- | --- | --- |
| Launcher | `artifacts/dev/launcher/<Config>/` | package root | `Build > Build Launcher` |
| Showcase editor | `artifacts/dev/projects/Showcase/editor/<Config>/` | `Apps/ShowcaseEditor/` | `Build > Build Editor` |
| Showcase runtime | `artifacts/dev/projects/Showcase/runtime/<Config>/` | `Apps/ShowcaseRuntime/` | `Build > Build Runtime` |
| Showcase cooked assets | `artifacts/dev/projects/Showcase/cooked/` | `Projects/Showcase/Cooked/` | `Cook > Cook All` |
| Shared cooked assets | `artifacts/dev/projects/Shared/cooked/` | `Projects/Shared/Cooked/` | `Cook > Cook All` |

Package Inclusion Rules:

- Runtime packages include public apps, runtime support files, public cooked assets, manifests, docs, and licenses.
- Development packages are planned to include developer tools, import/static libraries, development headers, diagnostics needed for local work, and optional source dependency pack references.
- Symbols packages are separate from user-facing runtime packages and contain symbol/debug artifacts only.
- Internal and private surfaces must not be included in public runtime packages unless a future manifest rule explicitly promotes them.
- Inclusion should be driven by product ownership, visibility, binary type, package kind, and declared dependencies, not by broad folder scanning.

Dependency Pack Naming:

- Initial dependency pack identity is `sparkle-dependencies-<version>-<channel>-<platform>`.
- Dependency packs should record source dependency groups separately from host prerequisites and runtime redistributables.
- Package manifests should remain inspectable even when a dependency pack is not bundled.

Validation:

- Final build validation was not run.
- Final package validation was not run.
- Publish readiness is explicitly recorded as `false` in the Phase 5 release manifest.
