# First Release Capability Dispositions

**Status:** proposed scope ledger for release-owner approval; no capability receives acceptance evidence here

**Responsibility:** give every existing module inventory row one release classification and an owning module, then split compound user surfaces explicitly

**Parent decisions:** [Release Scope Record](README.md). **Requirement authority:** [First Release](../FirstRelease.md). **Source meaning:** [Module inventory](../../Architecture/Modules/README.md).

## Interpretation

Included is an obligation to deliver and accept the exact bounded path, not a statement that it works. Experimental still needs correctness within its declared limits and safe failure. Excluded has no first-release promise. No row is Removed. The Exposure column restricts distribution: Source only does not enter the ShippingGame UI/package; Runtime/source is only the subset consumed by the declared product. Internal mechanisms do not become standalone public features or a stable SDK.

The capability name and source link retain the existing inventory owner instead of copying its implementation narrative. A section names exactly one module owner; cross-module feature/report ownership remains in the existing FCR registry. Compound rows are split in the public-surface tables below. Old inventory evidence remains dated; a classification does not refresh all of its source assertions.

No fixed function, data format or hidden low-level flag is a new public selector. Generic development CVar/CLI access is source-only; the consumer settings allowlist below is exhaustive. A new advertised/reachable consumer choice requires a row before scope approval.

## Module Coverage

| Module / disposition owner | Rows | Inventory |
| --- | ---: | --- |
| Application / Application | 16 | [Inventory](../../Architecture/Modules/Engine/Application/README.md) |
| Build and packaging / Build/package | 21 | [Inventory](../../Architecture/Modules/BuildAndPackaging/README.md) |
| Core / Core | 18 | [Inventory](../../Architecture/Modules/Engine/Core/README.md) |
| Editor / Editor | 24 | [Inventory](../../Architecture/Modules/Engine/Editor/README.md) |
| Engine assets / Assets/content | 5 | [Inventory](../../Architecture/Modules/Engine/Assets/README.md) |
| GameFramework / GameFramework | 26 | [Inventory](../../Architecture/Modules/Engine/GameFramework/README.md) |
| Launcher / Launcher | 22 | [Inventory](../../Architecture/Modules/Tools/Launcher/README.md) |
| Cooking / Cooking | 22 | [Inventory](../../Architecture/Modules/Tools/Cooking/README.md) |
| Platform / Platform | 13 | [Inventory](../../Architecture/Modules/Engine/Platform/README.md) |
| RHI / RHI | 85 | [Inventory](../../Architecture/Modules/Engine/RHI/CapabilityInventory.md) |
| Renderer / Renderer | 144 | [Inventory](../../Architecture/Modules/Engine/Renderer/CapabilityInventory.md) |
| ShaderCompiler / ShaderCompiler | 69 | [Inventory](../../Architecture/Modules/Tools/ShaderCompiler/README.md) |
| Showcase / Showcase | 7 | [Inventory](../../Architecture/Modules/Projects/Showcase/README.md) |
| SourceImporters / SourceImporters | 19 | [Inventory](../../Architecture/Modules/Tools/SourceImporters/README.md) |
| Tasks / Tasks | 14 | [Inventory](../../Architecture/Modules/Engine/Tasks/README.md) |
| ToolSupport / ToolSupport | 7 | [Inventory](../../Architecture/Modules/Tools/ToolSupport/README.md) |

## Application

**Single row owner:** Application. [Existing source scope and limits](../../Architecture/Modules/Engine/Application/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `APP-001` | Shared application lifecycle | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-002` | Cooked runtime host | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-003` | Editor host | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-004` | Build-time editor erasure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-005` | Frame pump | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-006` | Task ownership | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-007` | Renderer execution selection | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-008` | Camera input bridge | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-009` | Viewport request/products | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-010` | Ordered shutdown | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `APP-011` | Command-line CVar assignment | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-012` | Runtime console | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-013` | Editor shader recook | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-014` | Editor operation service | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-015` | Viewport capture | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `APP-016` | Rendering-settings persistence | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## Build and packaging

**Single row owner:** Build/package. [Existing source scope and limits](../../Architecture/Modules/BuildAndPackaging/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `BUILD-001` | CMake baseline | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-002` | Six profiles | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-003` | Profile optimization/debug policy | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-004` | Compiler routes | Included | Runtime/source | Only the pinned MSVC toolchain is promised; alternate compiler routes remain unqualified. |
| `BUILD-005` | Static/shared engine modules | Included | Runtime/source | Use the selected product linkage; no binary SDK or stable exported ABI. |
| `BUILD-006` | Optional tool/features | Included | Runtime/source | Only classified options and exact eligible profiles; new options reopen scope. |
| `BUILD-007` | Project discovery | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-008` | Target layering | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-009` | Host-tool exclusion | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-010` | Dependency acquisition | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-011` | Selective dependency sync | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-012` | Development artifact contract | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-013` | Product layout | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-014` | Runtime support staging | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-015` | Tool bundles | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-016` | Architecture check | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-017` | Code-style targets | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `BUILD-018` | Install/stage/package | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `BUILD-019` | Automated tests | Excluded | No release promise | No new permanent test framework in this scope; existing/future claim-specific checks do not inherit this exclusion. |
| `BUILD-020` | CI | Excluded | No release promise | No hosted CI service promise; reproducible build/verification remains required. |
| `BUILD-021` | Root onboarding | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |

## Core

**Single row owner:** Core. [Existing source scope and limits](../../Architecture/Modules/Engine/Core/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `CORE-001` | Named diagnostics | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-002` | Console-variable registry | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-003` | Console command/session | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `CORE-004` | Event dispatch | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-005` | Whole-file I/O | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-006` | Bounded binary decode/encode | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-007` | Lightweight JSON helpers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-008` | Workspace/project path model | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-009` | Typed asset path resolution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-010` | Child-process execution | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `CORE-011` | Environment and command-line utilities | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-012` | Project level catalog | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-013` | Input vocabulary/state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-014` | Math and coordinate helpers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-015` | Pixel conversions | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-016` | Timer | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-017` | Thread ownership assertions | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `CORE-018` | Stable hashing/string tables | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## Editor

**Single row owner:** Editor. [Existing source scope and limits](../../Architecture/Modules/Engine/Editor/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `ED-001` | Fixed workspace shell | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-002` | Native window controls | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-003` | Level open/save | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-004` | Level progress/failure gating | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-005` | Scene outliner | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-006` | Camera inspector | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-007` | Light inspector | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-008` | Mesh inspector | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-009` | Sky inspector | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-010` | Material variant selector | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-011` | Undo/redo | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-012` | Stale edit protection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-013` | Editor viewport | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-014` | View modes | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-015` | Exposure overrides | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-016` | Rendering settings | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-017` | Restart service | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-018` | Viewport capture | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-019` | Editor console | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-020` | Shader tools | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-021` | Mesh diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-022` | Texture diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-023` | Memory/renderer diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `ED-024` | Icon/theme assets | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## Engine assets

**Single row owner:** Assets/content. [Existing source scope and limits](../../Architecture/Modules/Engine/Assets/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `EASSET-001` | Shader entry sources | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `EASSET-002` | Shader includes/contracts | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `EASSET-003` | Default textures | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `EASSET-004` | Sky environments | Included | Runtime/source | Only a rights-cleared dependency of ReleaseMapSet may enter the runtime package; do not ship the entire sky corpus. |
| `EASSET-005` | Cube fixtures | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## GameFramework

**Single row owner:** GameFramework. [Existing source scope and limits](../../Architecture/Modules/Engine/GameFramework/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `GF-001` | Level documents | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-002` | Level registry/session | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-003` | Asynchronous scene load | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-004` | Cooked-only runtime | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-005` | Cooked format identification | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-006` | Scene registry | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-007` | File lifetime during load | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-008` | Manifest validation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-009` | Entity identity/lifetime | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-010` | Fixed component schema | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-011` | Typed queries and frozen structure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-012` | Compiled system graph | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-013` | Parallel world evaluation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-014` | Transform hierarchy evaluation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-015` | Camera simulation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-016` | Lighting model | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-017` | Material model and variants | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-018` | Static/instanced meshes | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-019` | Skeletal animation | Included | Runtime/source | Cesium Man skeletal animation is an Included example; exact clip and pose require cook/runtime proof. |
| `GF-020` | Morph animation | Included | Runtime/source | Existing source morph capability; no consumer morph demonstration or arbitrary animation-format promise. |
| `GF-021` | World edits | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `GF-022` | Read snapshots/change journal | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-023` | Structural publication | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-024` | Dynamic publication | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-025` | View publication | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `GF-026` | Stable render identity | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## Launcher

**Single row owner:** Launcher. [Existing source scope and limits](../../Architecture/Modules/Tools/Launcher/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `LAUNCH-001` | Qt GUI | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-002` | Shell route | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-003` | Immutable operation plan | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-004` | Quick Start dependency graph | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-005` | Background operation service | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-006` | Repository/content discovery | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-007` | Toolchain detection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-008` | Source dependency sync | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-009` | Build-file generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-010` | Freshness diagnosis | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-011` | Workspace build | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-012` | Launcher self-build | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-013` | Host-tool install | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-014` | Level sync | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-015` | Cook workspace | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-016` | Focused/full cooks | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-017` | Force recook safety | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-018` | Run level | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-019` | Build profiles | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-020` | Graphics API choice | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-021` | Clean workspace | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `LAUNCH-022` | Logs/recovery | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## Cooking

**Single row owner:** Cooking. [Existing source scope and limits](../../Architecture/Modules/Tools/Cooking/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `COOK-001` | Project cook CLI | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-002` | Project discovery | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-003` | Capability preflight | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-004` | Shader stage delegation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-005` | Texture request planning | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-006` | Scene generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-007` | Tool/output reporting | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-008` | Source texture formats | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-009` | Texture dimensions | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-010` | Mip policy | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-011` | Channel extraction | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-012` | Semantic format policy | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-013` | Parallel batch | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-014` | Memory bound | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-015` | Generation publication | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-016` | Request inspection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-017` | Mesh | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-018` | Material | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-019` | Scene manifest | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-020` | Skeleton | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-021` | Animation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `COOK-022` | Registry | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## Platform

**Single row owner:** Platform. [Existing source scope and limits](../../Architecture/Modules/Engine/Platform/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `PLAT-001` | Native Win32 window | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-002` | Message pump | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-003` | Size/state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-004` | Window controls | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-005` | DPI awareness/events | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-006` | Window event contracts | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-007` | Win32 keyboard/mouse backend | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-008` | Per-frame input state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-009` | Immediate/deferred dispatch | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-010` | Layer routing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-011` | UI capture integration | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-012` | Mouse/cursor control | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `PLAT-013` | Owner-thread enforcement | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## RHI

**Single row owner:** RHI. [Existing source scope and limits](../../Architecture/Modules/Engine/RHI/CapabilityInventory.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `RHI-BACK-01` | Common public RHI | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BACK-02` | Diagnostics implementation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BACK-03` | D3D12 backend | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BACK-04` | Vulkan backend | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BACK-05` | Backend selection | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BACK-06` | Backend parity | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-01` | API/version identity | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-02` | Shader binary | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-03` | Queue kinds | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-04` | Synchronization | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-05` | Dynamic rendering | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-06` | Descriptor indexing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-DEV-07` | Mesh shaders | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-DEV-08` | Task shaders | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-DEV-09` | Adapter preference | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-01` | Aggregate service ownership | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-02` | Atomic creation/publication | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-03` | Ordered settlement and destruction | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-04` | Swapchain recovery | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-05` | Device-loss diagnostics | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-LIFE-06` | In-process device recreation | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-RES-01` | 2D textures | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-02` | Buffers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-03` | Resource views | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-04` | Upload | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-05` | Readback | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-06` | Persistent allocation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-07` | Transient allocation and aliasing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-08` | Residency pressure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RES-09` | Samplers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-01` | Float/color | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-02` | 8-bit color | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-03` | Depth/stencil | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-04` | BC color | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-05` | BC scalar/vector | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-FMT-06` | BC HDR | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-01` | Bindful descriptor sets/tables | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-02` | Fixed descriptor arrays | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-03` | Non-uniform indexing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-04` | Partially-bound arrays | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-05` | Renderer material texture table | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-06` | Raster-material coverage | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-BIND-07` | Runtime-sized bindless | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-PIPE-01` | Graphics pipeline | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-02` | Primitive topology | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-03` | Vertex inputs | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-04` | Index formats | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-05` | Raster state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-06` | Depth/stencil | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-07` | Blending | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-08` | Compute pipeline | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PIPE-09` | Geometry/hull/domain stages | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-PIPE-10` | Mesh/task shaders | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `RHI-CMD-01` | Recording leases | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-02` | Raster commands | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-03` | Compute commands | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-04` | Ray commands | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-05` | Copy commands | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-06` | Explicit state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-CMD-07` | Diagnostic scopes | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-01` | Bottom-level AS | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-02` | BLAS update/refit | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-03` | Classic TLAS | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-04` | Inline ray query | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-05` | Native RT pipeline | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-06` | Shader table | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RT-07` | Triangle hit groups | Included | Runtime/source | Triangle geometry only; procedural/intersection/callable generality is excluded. |
| `RHI-RT-08` | Partitioned TLAS | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `RHI-RTC-01` | Ray-traced GBuffer | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RTC-02` | Direct-shadow visibility | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-RTC-03` | Reference path-traced direct/indirect | Included | Runtime/source | Required native lowering for full admitted reference transport; no finite-depth diagnostic substituted as oracle. |
| `RHI-RTC-04` | ReSTIR indirect temporal/spatial/resolve | Included | Runtime/source | Required native execution of accepted realtime indirect paths/reuse; paired backend correctness and bounded costs. |
| `RHI-PRES-01` | Swapchain | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PRES-02` | VSync | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PRES-03` | Frame pacing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PRES-04` | Back-buffer commands | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `RHI-PRES-05` | HDR presentation | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `RHI-DIAG-01` | Object naming and GPU events | Included | Runtime/source | Native names/markers retained as mechanics; capture SDK/event-runtime package inclusion is separately allowlisted. |
| `RHI-DIAG-02` | Timestamp queries | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `RHI-DIAG-03` | Validation messages | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `RHI-DIAG-04` | D3D12 crash diagnostics | Included | Runtime/source | Local actionable device-loss diagnostics; no automatic upload or crash SDK. |
| `RHI-DIAG-05` | Live-object reporting | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `RHI-DIAG-06` | Texture capture | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `RHI-DIAG-07` | External provider handles | Experimental | Runtime/source | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `RHI-DIAG-08` | ImGui backend | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## Renderer

**Single row owner:** Renderer. [Existing source scope and limits](../../Architecture/Modules/Engine/Renderer/CapabilityInventory.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `REN-OWN-01` | Public renderer facade | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-OWN-02` | Scene ownership | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-OWN-03` | View ownership | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-OWN-04` | Frame ownership | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-OWN-05` | Threaded or serial execution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-OWN-06` | Renderer/RHI boundary | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-01` | Typed graph authoring | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-02` | Compiled pass kinds | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-03` | Resource model | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-04` | Dependency compilation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-05` | Transient aliasing | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-06` | Multi-queue submission | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-07` | Parallel command recording | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FG-08` | Graph rebuild and retirement | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PIPE-01` | Typed pass parameter contract | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PIPE-02` | Runtime shader and binding-layout validation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PIPE-03` | Graphics and compute materialization | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PIPE-04` | Native ray-pipeline and table materialization | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PIPE-05` | Atomic shader-generation replacement | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-08` | Shader hot reload | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-SCENE-01` | Persistent GPU scene | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-02` | Parallel scene preparation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-03` | Static meshes | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-04` | Skeletal meshes | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-05` | Morph targets | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-06` | Ray-traced deforming geometry | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-07` | Static BLAS reuse | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-08` | Mesh residency | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-09` | Texture residency | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SCENE-10` | Automatic mesh batching | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-01` | Per-view frustum visibility | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-02` | Material visibility classification | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-03` | Raster candidate validation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-04` | Authored/preserved groups | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-05` | Opaque sorting and auto batching | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-06` | Transparent draw preparation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-07` | Visibility task/failure bounds | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VIS-08` | Visibility/batch workload facts | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-VIS-09` | Advanced visibility/draw generation | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-TEMP-01` | Per-view previous-camera state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-TEMP-02` | Active temporal jitter | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-TEMP-03` | Common history invalidation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-TEMP-04` | Motion and reprojection convention | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-TEMP-05` | Provider temporal constants | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RESO-01` | Output extent resolution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RESO-02` | Render extent resolution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RESO-03` | Extent-driven topology and history | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RESO-04` | Active raster sample count | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RESO-05` | Renderer MSAA | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-RESO-06` | Standalone post-process AA | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-RESO-07` | Dynamic resolution | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-MAT-01` | Base color | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-02` | Tangent-space normal | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-03` | Roughness | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-04` | Metallic | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-05` | Ambient occlusion | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-06` | Dielectric F0 | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-07` | Emissive | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-08` | Subsurface | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-09` | Alpha mask | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-MAT-10` | Double-sided | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-01` | Base color | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-02` | Normal | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-03` | Material | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-04` | Emissive | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-05` | Subsurface | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-06` | Motion vector | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-07` | Device depth | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-GBUF-08` | Scene depth | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FRONT-01` | Raster GBuffer | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FRONT-02` | Ray GBuffer: inline | Included | Runtime/source | Required traced surface frontend for realtime/native parity; complete owned hit/material/traversal contract. |
| `REN-FRONT-03` | Ray GBuffer: native pipeline | Included | Runtime/source | Required traced surface frontend for realtime/native parity; complete owned hit/material/traversal contract. |
| `REN-FRONT-04` | Automatic ray execution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-FRONT-05` | Transparent blending | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-FRONT-06` | Wireframe | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-FRONT-07` | Material binding mode selector | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-PBR-01` | Active direct specular BRDF | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-02` | Active direct diffuse BRDF | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-03` | Active subsurface approximation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-04` | Alternate BRDF implementations | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-PBR-05` | Indirect approximation helpers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-06` | Directional lights | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-07` | Point lights | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-08` | Spot lights | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-09` | Rect lights | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-PBR-10` | Non-ray lighting fallback | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-LGT-01` | Source-named ReSTIR lighting route | Included | Runtime/source | Required direct/composite and realtime transport; actual estimator correctness must pass its owner contract, not ReSTIR naming. |
| `REN-LGT-04` | Reference Path Tracer | Included | Runtime/source | Required consumer progressive SurfaceTransportReference and raw export; accepted domain, independent oracle, uncertainty and invalid-sample proof. |
| `REN-LGT-05` | Accumulation invalidation | Included | Runtime/source | Required progressive/realtime reset and accumulation identity; no stale generations or invalid-sample advancement. |
| `REN-LGT-06` | Lighting composite | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-LGT-02` | Direct shadow visibility | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-LGT-03` | Source-named ReSTIR indirect prototype | Included | Runtime/source | Required realtime transport/reuse closure under the 30 FPS preset; current seed-replay prototype is not accepted path tracing. |
| `REN-LGT-07` | Sky | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-VOL-01` | Participating media and fog volumes | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-VOL-02` | Volumetric light transport | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-VOL-03` | Atmosphere and aerial perspective | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-DECAL-01` | Authored and scene decal data | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-DECAL-02` | Primary deferred GBuffer composition | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-DECAL-03` | Secondary-ray decal evaluation | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-RT-01` | Classic TLAS | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RT-02` | Partitioned TLAS | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `REN-RT-03` | Shared scene identity | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RT-04` | Shader-table plan | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RT-05` | Hit-group coverage | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-RT-06` | Plan invalidation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-01` | Manual exposure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-02` | Automatic exposure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-03` | Async exposure | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-04` | Linear upscaler | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-05` | NVIDIA DLSS Super Resolution | Included | Runtime/source | Required complete DLSS SR quality/lifecycle/package/input contract on both D3D12 and Vulkan; fallback cannot pass the provider cell. |
| `REN-POST-06` | NVIDIA DLSS Ray Reconstruction | Included | Runtime/source | Required complete RR guide/extent/frame/reset/quality contract on both APIs; SDK-supported composition must be frozen before performance runs. |
| `REN-POST-07` | Tone mapping | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-11` | Color grading | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-POST-12` | Chromatic aberration | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-POST-14` | Basic vignette | Included | Runtime/source | Required basic lens example; independent AC-VIG contract under FCR-REN-25, neutral-work erasure, both-backend formula/order/package proof. |
| `REN-POST-13` | Frame generation | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `REN-POST-08` | Output encoding | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-POST-09` | HDR display output | Included | Runtime/source | Required delivery obligation; source absence/partial path cannot accept this row. |
| `REN-POST-10` | Exact debug presentation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DBG-01` | Final/material | Included | Source only | Lit is consumer Included; Wireframe source-only. This compound row is split in the view-mode table. |
| `REN-DBG-02` | GBuffer | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DBG-03` | Lighting | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DBG-04` | Scene diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-UI-01` | UI render packets | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-UI-02` | Host overlay | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-UI-03` | Editor viewport presentation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-UI-04` | Editor texture handles | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-LAT-01` | Logical frame latency markers | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-LAT-02` | Streamline PCL marker route | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `REN-LAT-03` | Reflex simulation sleep | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `REN-LAT-04` | Provider call/shutdown lifetime | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `REN-LAT-05` | Frame-token identity | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `REN-DIAG-01` | Frame/pass diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-02` | Mesh diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-03` | Texture diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-04` | Memory diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-05` | Viewport products | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-DIAG-06` | Async captures | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-DIAG-07` | Mesh preview | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `REN-SET-01` | Aggregate public settings state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SET-02` | Application-owned rendering-settings persistence | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `REN-SET-03` | Startup and editor commit | Included | Runtime/source | Runtime startup Included; Editor commit/source producer retained, absent from consumer binary. |
| `REN-SET-04` | Serial/threaded settings handoff | Included | Runtime/source | Existing serial/threaded handoff retained; no public topology or concurrency tuning promise. |
| `REN-SET-05` | Live versus restart-active state | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |

## ShaderCompiler

**Single row owner:** ShaderCompiler. [Existing source scope and limits](../../Architecture/Modules/Tools/ShaderCompiler/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `SHD-BUILD-01` | Offline compiler executable | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-BUILD-02` | Shared shader contracts | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-BUILD-03` | Contract-only registrations | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-BUILD-04` | Runtime shader source dependency | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-BUILD-05` | Tool dependencies | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CLI-01` | `cook` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CLI-02` | `list-backends` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CLI-03` | `list-targets` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CLI-04` | `list-shaders` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CLI-05` | `inspect-shader` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-01` | Selection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-02` | Targets | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-03` | Backend | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-04` | Parallelism | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-05` | Diagnostics | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-06` | Compiler policy | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-OPT-07` | Cancellation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-TGT-01` | DXC | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-TGT-02` | Slang | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-01` | Compute | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-02` | Vertex | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-03` | Pixel | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-04` | Ray generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-05` | Miss | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-06` | Closest hit | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-07` | Any hit | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-CAT-08` | Geometry | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-CAT-09` | Hull | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-CAT-10` | Domain | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-CAT-11` | Intersection | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-CAT-12` | Callable | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-REG-01` | Typed global registration | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-REG-02` | Registry validation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-REG-03` | Parameter-structure contract | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-REG-04` | Ray contract | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-REG-05` | Runtime registration closure | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-01` | Virtual shader roots | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-02` | Include preprocessing | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-03` | Immutable jobs | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-04` | Changed-source planning | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-05` | In-operation deduplication | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PLAN-06` | Persistent result cache | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `SHD-PLAN-07` | Parallel compilation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-01` | DXIL generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-02` | SPIR-V generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-03` | DXIL reflection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-04` | SPIR-V reflection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-05` | Resource reflection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-06` | Constant data | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-07` | Stage inputs | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-08` | Specialization constants | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-09` | C++ parameter ABI validation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-ABI-10` | Vulkan descriptor normalization | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-01` | `GlobalShaderMap.smap` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-02` | `CookedShaderLibrary.slib` | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-03` | Shared publication hash | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-04` | Staged validation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-05` | Atomic file-set publish | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-06` | Deterministic identity | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-PUB-07` | Incremental preservation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-01` | DXC debug artifacts | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-02` | Slang debug artifacts | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-03` | Cook analysis | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-04` | Cooked inspection | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-05` | Out-of-process editor recook | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-06` | Used Shaders UI | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-07` | Runtime reload | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHD-DIAG-08` | Shipping tool erasure | Included | Source only | Included source workflow obligation: prove all compiler/editor payload absent from Shipping. |
| `SHD-DIAG-09` | Cook progress | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## Showcase

**Single row owner:** Showcase. [Existing source scope and limits](../../Architecture/Modules/Projects/Showcase/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `SHOW-001` | Project discovery | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `SHOW-002` | Editor product | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHOW-003` | Runtime product | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `SHOW-004` | Six-profile build naming | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `SHOW-005` | Startup level selection | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `SHOW-006` | Runtime level switching | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `SHOW-007` | Authored level save | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## SourceImporters

**Single row owner:** SourceImporters. [Existing source scope and limits](../../Architecture/Modules/Tools/SourceImporters/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `IMP-001` | glTF 2.0 JSON | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-002` | GLB | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-003` | FBX | Experimental | Source only | Capability/limitation-gated; explicit requested/active truth and safe rejection; no unproved vendor/backend/estimator claim. |
| `IMP-004` | OBJ/USD/Alembic/OpenVDB | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |
| `IMP-005` | Triangle geometry | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-006` | Tangent generation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-007` | Coordinate normalization | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-008` | Mesh instances | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-009` | GPU-authored instancing | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-010` | Metallic-roughness material | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-011` | Material textures | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-012` | Alpha | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-013` | Material variants | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-014` | Cameras | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-015` | Lights | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-016` | Skeleton/skin | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-017` | Morph targets | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-018` | Animation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `IMP-019` | Embedded FBX textures | Excluded | No release promise | Outside first-release scope; do not expose or advertise this capability in the consumer product. |

## Tasks

**Single row owner:** Tasks. [Existing source scope and limits](../../Architecture/Modules/Engine/Tasks/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `TASK-001` | Three-lane execution | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-002` | Deterministic serial reference path | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-003` | Compiled DAG | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-004` | Lane dependency safety | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-005` | Parallel ranges | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-006` | Logical nested completion | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-007` | Failure propagation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-008` | Cooperative cancellation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-009` | Structured scopes | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-010` | Execution observation | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-011` | Blocking event | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-012` | Drain/cancel shutdown | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-013` | Capacity policy | Included | Runtime/source | Required bounded product mechanic; accept through existing owner/FCR and exact candidate, not source presence. |
| `TASK-014` | Windows task tracing | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## ToolSupport

**Single row owner:** ToolSupport. [Existing source scope and limits](../../Architecture/Modules/Tools/ToolSupport/README.md); the release owner approves these proposals.

| ID | Capability | Classification | Exposure | Decision / required closure |
| --- | --- | --- | --- | --- |
| `TOOL-001` | Severity-prefixed messages | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-002` | Named fields | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-003` | Progress records | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-004` | Summaries and lists | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-005` | Path display helpers | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-006` | Current consumers | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |
| `TOOL-007` | Game-profile isolation | Included | Source only | Retain existing developer/source route; no packaged developer archive or consumer control. |

## Consumer And Development Surface Splits

This table takes precedence over a compound inventory title. It does not replace feature semantics or create a second setting/state authority.

| Surface | Classification / exposure | Owner and bounds |
| --- | --- | --- |
| D3D12 / Vulkan | Included / runtime | RHI native mechanics; Application launch precedence; separate matrix evidence on MIN-01 |
| Raster GBuffer | Included / runtime, default | Renderer surface-production owner; opaque/alpha-mask, single-sample |
| Ray GBuffer, inline/native-pipeline/automatic traversal | Included / runtime | Shared Renderer/RHI ray owner; exact supported lowering/target; do not advertise procedural geometry |
| Direct diffuse/specular/subsurface and direct shadows | Included / runtime | Direct-lighting owner; local physical/approximation boundaries, no unproved ReSTIR DI conformance |
| Indirect diffuse/specular, indirect shadows, temporal/spatial reservoir reuse | Included / runtime | Indirect-lighting owner; exact estimator limits; no ReSTIR GI/GRIS equivalence or unbiasedness claim |
| Directional, point, spot, rect lights; sky | Included / runtime | Light/scene owners; only supported units/geometry and rights-cleared scene inputs |
| Volumetric Lighting / Frame Generation | Excluded | Existing release contract; no setting, dependency or advertised output |
| Deferred decals including secondary-ray semantics | Included / runtime target | Existing mandatory FCR-REN-23 obligation; initial maps have no decal actors; no silent omission of required ray semantics |
| Progressive Reference Path Tracer | Included / runtime | Reliable full admitted SurfaceTransportReference, consumer mode/raw export, independent convergence/oracle/lifetime proof. Preserve PTD-00 and finish recovery/Stage 10; no finite diagnostic or denoised image substituted. |
| Manual/Automatic exposure; Histogram/DownsamplePyramid | Included / runtime | Exposure owner; all current enumerators retained with validated values/history behavior |
| Reinhard / AcesApprox / AcesFilmic | Included / runtime | Tone-mapping owner; AcesApprox release default, exactly one transform |
| Automatic/Linear/Srgb encoding | Included / runtime | Presentation owner; validated surface combination, no implication that Linear means HDR |
| Linear provider / native extent | Included / runtime, default | Reconstruction owner; this is not TAA or a new native AA implementation |
| DLSS / NativeAA, Quality, Balanced, Performance, UltraPerformance | Included / runtime | Vendor-compatible tuples only; all enum quality choices require supported-provider/extent proof. Missing provider is visible unavailable, not a hidden baseline switch. |
| DLSS Ray Reconstruction | Included / runtime | Required complete compatible lighting/guide/provider cells on D3D12 and Vulkan; Off native control, On realtime preset; current Vulkan initialization gap cannot be marked accepted |
| Classic TLAS / refit | Included / runtime | Shared AS identity/lifetime; no new consumer acceleration-structure editor |
| PTLAS and partition controls | Experimental / source only | Narrow installed capability and build/update semantics; Off consumer baseline; no performance savings claim |
| Mesh batching / single-sample raster / fixed Halton jitter | Included / runtime | Existing mechanics; advanced occlusion/mesh shaders, MSAA, standalone TAA/FXAA and dynamic resolution excluded |
| Color Grading / Chromatic Aberration / Vignette | Included / runtime targets | FCR-REN-24/25; neutral/Off baseline; each feature needs independent bounded controls, formula/order/failure and both-backend proof |
| SDR / HDR10 | Included / runtime | SDR default; HDR10 only with qualified display/OS/swapchain tuple and mandatory SDR fallback; FCR-REN-26 remains open |
| PIX / Nsight Graphics / RenderDoc | Experimental / Debug and Development source products only | Existing external-capture authority, one selected provider None -> Nsight -> PIX -> RenderDoc; exact supported native cells only. No optional Shipping capture payload. |
| PIX Timing / Nsight GPU Trace and Systems / WPR-WPA / PresentMon | Experimental / source runbooks only | Distinct tool-managed activities/artifacts; not universal frame-provider APIs; native evidence remains in owning external-capture report |
| AMD specialist profilers, embedded crash SDK/handler, internal profiler expansion | Excluded | User exclusion and separate discovery/privacy owners; no dormant adapter or new SDK in this scope |
| Reflex/PCL and developer concurrency/diagnostic tuning | Experimental / source only | Existing latency/startup owners; no consumer latency savings or exposed topology control promise |

The [owner-directed rendering closure](../FirstRelease.md#required-rendering-closure) additionally requires the realtime >=30 FPS preset, complete frame graph, full Included parity and independent principal-level frame review. These are release obligations, not source acceptance. See the scope record for native/realtime/reference presets. Vignette adds one target inventory row; no new enum or CVar was implemented.

## Every View Mode

**Owner:** Renderer semantic, Application runtime composition, Editor source presentation. [Actual enum](../../../Engine/Renderer/Public/Viewport/RenderViewMode.h) has 18 values; Count is not a mode.

| Mode | Classification | Exposure |
| --- | --- | --- |
| `Lit = 0` | Included | Runtime/source |
| `ReferencePathTracer = 1` | Included | Runtime/source |
| `Wireframe = 2` | Included | Source only |
| `GBufferDiffuse = 3` | Included | Source only |
| `GBufferWorldNormal = 4` | Included | Source only |
| `GBufferWorldTangent = 5` | Included | Source only |
| `GBufferRoughness = 6` | Included | Source only |
| `GBufferMetallic = 7` | Included | Source only |
| `GBufferEmissive = 8` | Included | Source only |
| `GBufferAmbientOcclusion = 9` | Included | Source only |
| `GBufferSubsurfaceColor = 10` | Included | Source only |
| `GBufferSubsurfaceStrength = 11` | Included | Source only |
| `DirectDiffuse = 12` | Included | Source only |
| `DirectSpecular = 13` | Included | Source only |
| `DirectSubsurface = 14` | Included | Source only |
| `IndirectDiffuse = 15` | Included | Source only |
| `IndirectSpecular = 16` | Included | Source only |
| `GpuSceneInstances = 17` | Included | Source only |

## Every Persisted Setting

**Single persistence owner:** Application. [Actual allowlist/writer](../../../Engine/Application/Private/RenderingSettings/EngineRenderingSettingsPersistence.cpp) contains 25 common names plus the conditional Editor startup-provider name. Renderer owns values/effects, not persistence or consumer UI. Current INI values below are observations, not the proposed release baseline. Public controls must use validated owning domains; no unrestricted numeric/CVar console in Shipping.

| Setting | Current immutable INI | Classification / exposure | Release decision |
| --- | --- | --- | --- |
| `r.ExternalCapture.StartupProvider` | `absent; typed default None` | Experimental / source only | Editor Debug/Development only; None/Nsight/PIX/RenderDoc; one provider; no Shipping option. |
| `r.VSync` | `0` | Included / runtime | Consumer default On; benchmark explicitly Off; current repository INI Off remains source observation. |
| `r.BackBufferFormat` | `28` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.PreferHighPerformanceAdapter` | `1` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.ToneMapper` | `2` | Included / runtime | Change consumer baseline from AcesFilmic (2) to AcesApprox (1) in PD-1; all three modes Included. |
| `r.Exposure.Mode` | `1` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.MeteringMethod` | `0` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.OutputColorEncoding` | `0` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.Manual` | `0.800000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.Compensation` | `0.000000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.TargetLuminance` | `0.180000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.Min` | `0.000001` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.Max` | `65536.000000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.AdaptationSpeedUp` | `2.000000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Exposure.AdaptationSpeedDown` | `1.000000` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.MeshAutoBatching` | `1` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.Upscaler.Provider` | `1` | Included / runtime | Linear/native and RR-Off are native controls; Included DLSS quality/RR cells must pass on both APIs. Unsupported requests are visible failures, not accepted substitutes. |
| `r.Upscaler.QualityMode` | `0` | Included / runtime | Linear/native and RR-Off are native controls; Included DLSS quality/RR cells must pass on both APIs. Unsupported requests are visible failures, not accepted substitutes. |
| `r.RayReconstruction.Mode` | `1` | Included / runtime | Linear/native and RR-Off are native controls; Included DLSS quality/RR cells must pass on both APIs. Unsupported requests are visible failures, not accepted substitutes. |
| `r.GBuffer.Algorithm` | `0` | Included / runtime | Included Rasterized default; RayTracing Included; realtime preset requires complete traced primary/secondary transport; shared automatic ray lowering. |
| `r.RayTracing.Tlas.Refit` | `1` | Included / runtime | Retain typed validated control; value/range/domain from owning settings contract. |
| `r.RayTracing.PreferPartitionedTlas` | `1` | Experimental / source only | Off consumer baseline; no exposed consumer partition planner controls. |
| `r.RayTracing.Ptlas.PartitionsPerAxis` | `8` | Experimental / source only | Off consumer baseline; no exposed consumer partition planner controls. |
| `r.RayTracing.Ptlas.PartitionUpdateMode` | `0` | Experimental / source only | Off consumer baseline; no exposed consumer partition planner controls. |
| `r.RayTracing.Ptlas.MarkAllDynamicInPartition` | `0` | Experimental / source only | Off consumer baseline; no exposed consumer partition planner controls. |
| `r.RayTracing.Ptlas.ModeChangeDistance` | `100.000000` | Experimental / source only | Off consumer baseline; no exposed consumer partition planner controls. |

## Coverage And Remaining Evidence

This revised proposal assigns **512 capability IDs** across **16 owners**: 475 Included, 8 Experimental, 29 Excluded, 0 Removed. The original 511 IDs are retained; REN-POST-14 is the newly admitted vignette target. Every capability ID is present once. This is coverage of the existing detailed inventory, not a claim that every implementation fact in its historical snapshot was freshly audited.

Live supplemental closure covers the product CMake targets, graphics-launch parser, package/path contracts, 20 actual catalog levels, all 18 view modes, every persisted setting and the current external-capture path. All other registered developer CVar/CLI settings are source-only, unstable, and absent from the consumer control promise. Future source changes or an unmatched consumer option invalidate the scope check.

Primary-source rights metadata does not prove the identity of locally acquired/cooked bytes. REL-01 must close that identity, credits, environment/texture/dependency closure, root license, publisher and redistribution receipts. Feature-local native proof, observer comparisons, failures and Shipping erasure remain in their existing acceptance owners. Approval adds A only for the disposition; never B/R/N/P.

