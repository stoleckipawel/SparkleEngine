# Capability Evidence Plan

Status: implementation plan; not proof, release classification, or release-wide sequencing authority

Snapshot: expanded 2026-09-06 from the repository-wide source-only capability inventory at committed `master` revision `8414b5dc`

Responsibility: release-surface reconciliation, inventory refresh, and the claim-falsifying checks required to promote individual source-present capability claims

Inventory authority: [Current Capability Inventory](../Architecture/Modules/README.md)

Release gates and evidence meaning: [First Release Acceptance Contract](../Acceptance/FirstRelease.md) and [Validation, Performance, and Evidence](../Engineering/Verification/ValidationAndEvidence.md)

Candidate-bound report authority: [First Release Feature Completion Reports](../Acceptance/FeatureCompletionReports.md)

Release-wide sequencing authority: [Release-First Principal Graphics Roadmap](../Strategy/Roadmap.md)

## Responsibility

This file is the queue of unanswered capability questions. It gives each claim a stable evidence ID, the smallest useful proof, and an escalation trigger. Evidence IDs use `<module>-ENN` and are deliberately distinct from Architecture capability IDs such as `RHI-BIND-05`. It does not decide when work starts, whether a feature ships, whether a gate passes, or where candidate evidence is retained. When an item is executed, attach it to the applicable `FCR-*` completion report rather than creating a competing evidence record here.

Before executing an item, its iteration record MUST map the plan ID to applicable `NS-*`, `PGE-*`, `REL-*`/`MAP-*`, `RISK-*`, `FCR-*`, binary `AC-*`, controlled `FM-*`, and one key `CHK-*` under [Change Lifecycle](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record). Keep the row `Open` when any required mapping, oracle, artifact, or independent review is missing.

A completed evidence item must record:

- exact revision and dirty-tree state;
- build profile, backend, feature/mode, executable, map/workload, machine, GPU, driver, OS/SDK/tool versions;
- exact commands or UI route and whether the result came from workspace or packaged bytes;
- expected observation, actual observation, warnings/errors, duration, and artifact paths/hashes;
- owner/reviewer and the affected inventory row;
- resulting release disposition decision, if one was approved.

If the smallest check falsifies the claim, stop and record the defect. Do not continue into broad builds, cooks, captures, or performance runs that cannot rescue the failed prerequisite.

## Inventory Expansion

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `INV-001` | Every user-reachable feature/backend/mode/tool is represented. | Enumerate public settings, editor controls, console commands/CVars, launcher actions, executable arguments, map catalog, and package manifest; diff against inventory IDs. | Any unmatched selector creates a new row before `REL-00` can close. | Open |
| `INV-002` | Every row has one implementation owner and all producers/consumers are known. | For each detailed row, trace declaration, configuration producer, runtime producer, consumer, failure path, and CMake target. | A duplicated owner or alternate representation triggers a boundary review. | Open |
| `INV-003` | Release vocabulary is finite. | Assign Included, Experimental, Excluded, or Removed to every user-reachable row; record the public promise and unsupported combinations. | Any Pending/reachable row keeps `REL-00` open. | Open |
| `INV-004` | Core and Tasks source capabilities are understood. | Reconcile [Core](../Architecture/Modules/Engine/Core/README.md) and [Tasks](../Architecture/Modules/Engine/Tasks/README.md) against live public headers/CMake after relevant changes. | Concurrency or lifetime claims trigger focused stress evidence. | Source audit complete; executable evidence open below. |
| `INV-005` | Platform and Application source capabilities are understood. | Reconcile [Platform](../Architecture/Modules/Engine/Platform/README.md) and [Application](../Architecture/Modules/Engine/Application/README.md) against live owners after relevant changes. | Package or privilege-sensitive paths move to clean-machine/package gates. | Source audit complete; executable evidence open below. |
| `INV-006` | GameFramework/world and Editor source capabilities are understood. | Reconcile [GameFramework](../Architecture/Modules/Engine/GameFramework/README.md) and [Editor](../Architecture/Modules/Engine/Editor/README.md) after world/editor contract changes. | Any data-owner ambiguity triggers architecture reconciliation before evidence. | Source audit complete; executable evidence open below. |
| `INV-007` | Assets, import, and cooking source capabilities are understood. | Reconcile [Engine Assets](../Architecture/Modules/Engine/Assets/README.md), [Importers](../Architecture/Modules/Tools/SourceImporters/README.md), and [Cooking](../Architecture/Modules/Tools/Cooking/README.md) after format/product changes. | Each advertised format/content family receives a clean import/cook/runtime fixture. | Source audit complete; executable evidence open below. |
| `INV-008` | Launcher, Showcase, build, and package source capabilities are understood. | Reconcile [Launcher](../Architecture/Modules/Tools/Launcher/README.md), [Showcase](../Architecture/Modules/Projects/Showcase/README.md), and [Build](../Architecture/Modules/BuildAndPackaging.md) after workflow/product changes. | Any route dependent on workspace/private state moves to package correction before release proof. | Source audit complete; formal package path absent and evidence open below. |
| `INV-009` | Every advertised capability has a complete reviewer dossier. | Apply the [Capability Documentation Review dossier contract](../Engineering/Workflow/CapabilityReview.md#capability-dossier-contract) to every independently selectable capability; mark Answered, Partial, Unknown, or Not applicable and link unresolved dimensions. | A blank owner, selector, input/output, lifetime, limit, backend, failure, diagnostic, evidence, or invalidation field blocks inventory closure. | Open |
| `INV-010` | Every vital developer/user journey is represented horizontally. | Reconcile all executables, Launcher actions, editor workflows, runtime controls, importer/cooker commands, package/support operations, and catalog selections against [Product Workflow Coverage](../Architecture/CrossModule/ProductWorkflowCoverage.md). | An unmatched journey or materially different actor outcome creates a new `WF-*` row and corresponding `FCR-*` coverage. | Open |
| `INV-011` | Every vital journey has a complete vertical owner-to-outcome trace. | Apply the vertical template to build/cook/run, source-to-render, editor transaction, settings activation, shader reload, capture, cancellation/shutdown, and any newly admitted journey in [Product Execution Traces](../Architecture/CrossModule/ProductExecutionTraces.md). | An unowned representation, silent fallback, undefined safe state, or unobservable final outcome blocks the journey. | Open |
| `INV-012` | Capability, evidence, acceptance, and release records have no orphans. | Map every capability/`WF-*` row to its smallest evidence item, `FCR-*`, applicable `AC-*`/`FM-*`/`CHK-*`, release gate, owner, and invalidation trigger; run the reverse check from every report/plan row. | Any row with no proof destination, or any evidence/report with no current promise, blocks disposition. | Open |

## Foundation And Host Evidence

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `CORE-E01` | Diagnostics and console expose only intended Debug/Development/Shipping behavior. | Build one focused consumer per state; exercise logger level, fatal/verify capture, command scope, CVar parse failure, history and output bounds. | Missing/misleading diagnostic or Shipping leak triggers owner correction and package inspection. | Open |
| `CORE-E02` | File-set publication is generation-safe. | Publish a two-file fixture successfully, then inject failure/lock/cancellation at each finalization boundary and verify the prior complete set remains readable. | Mixed generation or orphaned temporary file blocks every cooker relying on the helper. | Open |
| `CORE-E03` | Workspace/project/cooked paths work from development and packaged locations. | Resolve and open the same fixture from repository, development artifact, relocated staged directory, and read-only install root. | Any repository-current-directory dependency moves directly to package correction. | Open |
| `CORE-E04` | Child-process launch/cancel/log/environment behavior is trustworthy. | Run a fixture that emits both streams, reads an override, exits nonzero, hangs until cancellation, and spawns a child; record process-tree and log outcome. | Orphan, lost output, false success, or quoting failure blocks Launcher/editor tool claims. | Open |
| `TASK-E01` | Serial and worker execution preserve graph semantics. | Execute one DAG covering dependency, `WhenAll`, nested completion, cleanup, failure, and per-node results with zero workers and representative three-lane workers. | Result/order/settlement divergence triggers scheduler diagnosis. | Open |
| `TASK-E02` | Invalid graphs and hard/default capacities reject deterministically. | Exercise every `TaskGraphErrorCode`, default edges/tasks, and one hard-limit boundary without broad runtime launch. | Acceptance of invalid/stale/foreign handles blocks Tasks consumers. | Open |
| `TASK-E03` | Cancellation and shutdown are bounded and deadlock-free. | Cancel queued/running cooperative work; destroy nested scopes; test Drain/Cancel and prohibited worker waits/shutdown with timeouts. | Hang, cleanup omission, or post-shutdown body start expands to stress/race instrumentation. | Open |
| `TASK-E04` | Lane scheduling and ETW data support performance diagnosis. | Run imbalanced frame/background/IO work, inspect worker/steal/trace events, and compare throughput without claiming deterministic completion order. | Starvation, missing trace identity, or unbounded queueing blocks performance claims. | Open |
| `PLAT-E01` | Window lifecycle is stable across all public state transitions. | Repeat create, resize, DPI move, minimize/wait/restore, maximize, borderless fullscreen, alt-tab, close on minimum/reference displays. | Hang, invalid extent, lost state, or DPI drift expands to native message trace. | Open |
| `PLAT-E02` | Layered input sends each event to exactly the intended consumer. | Controlled keyboard/mouse/wheel fixture across Gameplay/UI/System, deferred/immediate, text input, disabled interaction, overlapping regions, capture/focus loss. | Double delivery, stuck key/button, or capture leak blocks editor/runtime input. | Open |
| `APP-E01` | Runtime serial/threaded modes and pipeline depths preserve lifecycle and output. | Run Empty plus one scene with serial and threaded depth 0/1/2, minimize/restore, level switch, and repeated exit. | World/render divergence, deadlock, or invalid slot ownership blocks affected mode. | Open |
| `APP-E02` | Invalid process configuration fails visibly. | Supply valid, unknown, malformed, out-of-range, quoted, and duplicate CVar assignments; compare reported/effective state. | Silently ignored release-relevant typo requires diagnostic/UX correction. | Open |
| `APP-E03` | Runtime product is editor/tool-free. | Inspect link/import/file manifest for ShippingGame and run with Editor, source assets, cooker, and shader compiler trees unavailable. | Hidden dependency or missing cooked-only failure blocks runtime release. | Open |

## World, Content, And Tool Evidence

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `GF-E01` | Level activation/cancellation/reload publishes one coherent generation. | Rapidly request two levels, cancel during read/decode, reload active level, and inspect events, generations, diagnostics, world and Renderer identity. | Stale commit, partial world, wrong event, or retained resource blocks scene switching. | Open |
| `GF-E02` | Cooked loaders reject malformed, stale, truncated, and cross-referenced data. | Mutate one fixture per cooked family at magic, counts, byte ranges, and reference indices; verify explicit failure and no partial activation. | Plausible load of incompatible bytes requires schema/version correction. | Open |
| `GF-E03` | The 11-stage world graph is numerically stable across worker counts. | Run a fixed camera/animation/transform/morph/skin/extraction fixture serially and with selected worker counts; compare published arrays/identities. | Divergence or race expands to per-system trace. | Open |
| `GF-E04` | World edits, read views, journal, and undo/redo remain generation-safe. | Apply every edit payload, stale request, undo/redo, level replacement, cursor read/ack, and held snapshot. | Accepted stale edit, invalid inverse, or unbounded retention blocks editor claim. | Open |
| `GF-E05` | Static, instanced, skeletal, and morph data survive source-to-render handoff. | Use one compact fixture containing all four, verify resource tables, deltas, transforms, joint/morph ranges, and Renderer result. | Lost identity/binding or raster/ray divergence blocks affected asset kind. | Open |
| `IMP-E01` | glTF and GLB advertised semantics import without silent loss. | Fixture matrix for geometry/accessors/tangents/instances/GPU instancing/material slots/variants/cameras/lights/skin/morph/all interpolation modes. | Any unreported loss narrows support or requires implementation. | Open |
| `IMP-E02` | FBX advertised semantics import without silent loss. | Fixture matrix for units/axes, static/instanced geometry, supported materials/textures, perspective camera, four lights, skin and skeleton animation. | Any unreported loss or Assimp-version drift narrows support. | Open |
| `IMP-E03` | Unsupported format/extension behavior is explicit. | Request OBJ/USD/Alembic/OpenVDB, FBX morph, glTF embedded/BasisU/WebP/multi-UV/transform and advanced material lobes. | Partial-looking success blocks format advertisement. | Open |
| `IMP-E04` | Coordinate conversion occurs exactly once. | Import asymmetric transforms, winding, camera/light directions, skeleton bind pose, animation and morph deltas from both formats; compare engine-space oracle. | Axis/handedness/unit mismatch blocks the format. | Open |
| `COOK-E01` | Full project cook is deterministic and dependency-complete. | Cook isolated identical Showcase inputs twice for both shader targets; compare registry/products/hashes/order and enumerate every runtime-opened file. | Nondeterminism or missing dependency blocks package work. | Open |
| `COOK-E02` | Texture source/filter/compression matrix is correct. | Small numeric fixtures cover every source decoder, color/channel/dimension/mip policy/filter, selected BC format, alpha and error path. | Decoder/filter/format mismatch blocks that advertised combination. | Open |
| `COOK-E03` | Texture parallelism and 1 GiB lease policy remain bounded. | Cook more than four mixed-size textures, exact/over-budget cases, failure and retry while recording leases and peak process memory. | RSS far beyond declared bound, hang, or staged leak triggers memory-owner correction. | Open |
| `COOK-E04` | Multi-file scene publication is atomic and stale products are handled. | Cook multi-scene generation, then inject one asset failure/lock/interruption and remove a source item; inspect old/new registry and files. | Mixed generation or reachable stale asset blocks cooking. | Open |
| `EASSET-E01` | Every included Engine asset is cooked, licensed, and packaged intentionally. | Generate source-to-product-to-package manifest for registered shaders, eight defaults, selected skies, and fixtures; verify hashes/notices. | Unowned/missing/unlicensed asset blocks package acceptance. | Open |
| `TOOL-E01` | Shared tool output remains readable, unambiguous, and correctly isolated. | Drive all three severities, raw/quoted/path fields, progress, summary, and list output through direct and Launcher-captured streams using Unicode, whitespace, quote, newline, and long-path inputs; inspect game imports/files. | Ambiguous output, lost/reordered diagnostics, or game-product linkage requires contract correction. | Open |

## Product, Workflow, And Delivery Evidence

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `ED-E01` | Every visible editor action has a real result and visible failure. | Exercise open/save, seven edit payloads, undo/redo, all render settings/view modes, capture, restart, and all utility-panel actions. | No-op, silent failure, stale UI, or unsupported reachable selector blocks the action. | Open |
| `ED-E02` | Viewport input/camera/presentation remain isolated from world state. | Interleave UI text entry, viewport navigation, selection/edit, exposure/view mode, resize and level switch; inspect world camera and render camera separately. | Cross-talk or stale product triggers ownership correction. | Open |
| `ED-E03` | Shader/mesh/texture diagnostic panels report truthful products. | Compare displayed rows/bytes/formats/artifacts/previews with cooked files, scene inventory, and one native capture. | Incorrect diagnostic data is a release defect. | Open |
| `LAUNCH-E01` | Quick Start resolves the next real prerequisite to a live product. | On disposable clean workspace, request one level and record every resolved operation through final child activation log, not merely Launcher exit/PID. | Loop, false readiness, or lost child blocks adoption path. | Open |
| `LAUNCH-E02` | Toolchain/dependency/freshness diagnosis is accurate. | Remove or mismatch one prerequisite/stamp/source input at a time; compare status, planned repair, and post-repair state for MSVC and clang-cl routes. | False current/ready or destructive repair blocks workflow. | Open |
| `LAUNCH-E03` | Build/cook/run operations map exact inputs to exact products. | Dry-run and execute each operation ID with representative scopes/profiles/API, verifying commands, logs, outputs, exit status and invalidation. | Plan/execution mismatch blocks the operation. | Open |
| `LAUNCH-E04` | Cancellation stops Launcher work and descendant processes cleanly. | Cancel dependency sync, configure/build, cook, and run handoff at controlled points; inspect scopes/process tree/artifacts/activity. | Orphan or ambiguous success blocks cancellation claim. | Open |
| `LAUNCH-E05` | Sync validates large external content safely. | Exercise existing, absent, hash-failed, interrupted, disk-full/proxy-restricted representative pack on disposable storage. | Corrupt extraction or lost user files blocks sync. | Open |
| `LAUNCH-E06` | Clean scopes delete only previewed contained targets. | On disposable workspace populate all scopes and preserved paths; compare preview inventory with post-clean tree for each confirmation route. | Escaped path, unpreviewed deletion, or preserved loss blocks clean. | Open |
| `SHOW-E01` | Both Showcase products start and exit in every proposed profile/backend. | Build/run frozen profile/backend matrix first with Empty, recording real process and log activation/exit. | Any profile/backend failure narrows release matrix before scene work. | Open |
| `SHOW-E02` | Every Included/Experimental level has complete content and honest startup. | Sync/cook/run each approved catalog level; record required packs, activation, screenshot/capture, diagnostics and unsupported-feature notes. | Fallback to Empty or misleading title/description blocks level inclusion. | Open |
| `SHOW-E03` | Repeat level switching does not leak or retain stale generations. | Cycle compact/large/animated/Empty levels with cancellation and resize; record CPU/GPU memory, IDs, load times and exit. | Monotonic growth or ghost data triggers lifetime capture. | Open |
| `BUILD-E01` | Supported configure/build matrix is reproducible. | From clean source/cache states configure and build only the frozen profile/compiler/backend/product matrix; retain dependency refs and logs. | Host-specific/manual state narrows matrix or blocks release. | Open |
| `BUILD-E02` | Shared/static and editor/game membership match claims. | Build selected static/shared editor/game targets and inspect linked modules/imports; run architecture boundary check. | Editor/tool/backend leakage blocks the configuration. | Open |
| `BUILD-E03` | A formal staged package is dependency-complete. | First implement the owned Stage/Package manifest path; then inspect allowlisted files/imports and run relocated/read-only/outside-repository. | This item is blocked until packaging capability exists. | Open; implementation gap. |
| `BUILD-E04` | CI and automated regression guard the release contract. | First add an approved minimal configure/build/static-check/test workflow; then demonstrate clean failure/pass artifacts. | This item is blocked until CI/test capability exists. | Open; implementation gap. |

## RHI Evidence

### RHI capability-to-evidence map

| Evidence item | Direct RHI capability scope |
| --- | --- |
| `RHI-E01` | `RHI-BACK-01` through `RHI-BACK-06` |
| `RHI-E02` | `RHI-BACK-03`, `RHI-BACK-04`, `RHI-DEV-01`, `RHI-DEV-02`, `RHI-DEV-09` |
| `RHI-E03` | `RHI-DEV-03` through `RHI-DEV-05`, `RHI-PIPE-08`, `RHI-CMD-01` through `RHI-CMD-06` |
| `RHI-E04` | `RHI-RES-01` through `RHI-RES-03`, `RHI-FMT-01` through `RHI-FMT-06` |
| `RHI-E05` | `RHI-RES-07`, `RHI-CMD-06` |
| `RHI-E06` | `RHI-DEV-06`, `RHI-BIND-02` through `RHI-BIND-05` |
| `RHI-E07` | `RHI-RES-09`, `RHI-BIND-01`, `RHI-BIND-05`, `RHI-BIND-06`, `RHI-PIPE-01` through `RHI-PIPE-07`, `RHI-RTC-01` |
| `RHI-E08` | `RHI-RT-01`, `RHI-RT-03` |
| `RHI-E09` | `RHI-RT-04`, `RHI-RT-05`, `RHI-RTC-01`, `RHI-RTC-02` |
| `RHI-E10` | `RHI-RT-06`, `RHI-RT-07`, `RHI-RTC-01`, `RHI-RTC-02` |
| `RHI-E11` | `RHI-RT-08` |
| `RHI-E12` | `RHI-PRES-01` through `RHI-PRES-04`, `RHI-DIAG-08` |
| `RHI-E13` | `RHI-RES-05`, `RHI-CMD-07`, `RHI-DIAG-01` through `RHI-DIAG-06` |
| `RHI-E14` | `RHI-RES-04`, `RHI-RES-06` through `RHI-RES-08` |
| `RHI-E15` | `RHI-DEV-07`, `RHI-DEV-08`, `RHI-BIND-07`, `RHI-PIPE-02`, `RHI-PIPE-03`, `RHI-PIPE-07`, `RHI-PIPE-09`, `RHI-PIPE-10`, `RHI-RT-02`, `RHI-RTC-03`, `RHI-RTC-04`, `RHI-PRES-05`, `RHI-DIAG-07` |
| `RHI-E16` | `RHI-LIFE-01` through `RHI-LIFE-06` |

This map names the primary RHI proof destination, not the whole release chain. Each executed item still requires the cross-module `WF-*`, `FCR-*`, `AC-*`, `FM-*`, `CHK-*`, risk, environment, and invalidation mappings required above.

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `RHI-E01` | Each advertised backend configures and builds with only its owned source membership. | Configure/build the smallest D3D12-only and Vulkan-only RHI/consumer target selected by the build guide; run `architecture_boundary_check`. | Source leakage, unavailable SDK behavior, or link failure blocks backend advertisement. | Open |
| `RHI-E02` | Adapter/device creation enforces and reports the real minimum. | Launch one minimal Renderer consumer per backend on the proposed minimum and reference adapters; capture capability dump and failure on one unsupported configuration. | Observed requirements change the published matrix and relevant feature dispositions. | Open |
| `RHI-E03` | Graphics, compute, and copy queues submit/wait/retire correctly. | Exercise one frame containing all used queue transitions with validation enabled and record tokens/timestamps. | Validation error, hang, or absent overlap triggers a focused synchronization trace; performance claims wait. | Open |
| `RHI-E04` | Required formats support their actual usages. | Generate a table from `RhiFormatSupport` for release adapters and compare against frame-graph attachment/sample/storage/copy requirements. | Any missing usage bit blocks the consuming feature or requires an explicit fallback. | Open |
| `RHI-E05` | Transient aliasing is correct. | Run the smallest graph with a known alias transition under native validation and inspect one capture/resource timeline. | Any lifetime/barrier error expands to representative feature graphs before inclusion. | Open |
| `RHI-E06` | Descriptor-array material sampling works at boundaries. | Render known indices 0, last populated, and capacity edge through an affected ray path on both backends with validation. | Descriptor-indexing warning or wrong sample blocks the bindless claim; add capacity/overflow failure proof. | Open |
| `RHI-E07` | Raster bindful materials and ray fixed-table materials agree. | Render one material exercising all eight texture roles through raster and ray GBuffer, then compare decoded buffers. | Semantic mismatch triggers shader/descriptor contract reconciliation before broader visual tests. | Open |
| `RHI-E08` | Classic TLAS build/refit produces correct results. | Toggle the refit setting on a rigid-motion scene, validate structure updates, and compare visibility/identity. | Any stale geometry or identity mismatch disables refit or blocks the feature. | Open |
| `RHI-E09` | Inline and native-pipeline effects are semantically equivalent. | On one D3D12 and one Vulkan RT-capable adapter, compare ray GBuffer and direct shadows in strict Inline versus Pipeline modes using captures. | Mismatch creates effect-specific defects; Automatic mode cannot be accepted as parity evidence. | Open |
| `RHI-E10` | Shader-table records map scene instances and ray types correctly. | Use a scene with opaque and alpha-tested instances, both ray types, repeated geometry/materials, and inspect SBT/index diagnostics plus output. | Any wrong hit group/record or out-of-range index blocks native pipeline release. | Open |
| `RHI-E11` | Partitioned TLAS works only in the advertised subset. | Run the selected provider with current one-operation/no-update/no-translation strategy and record capability/fallback behavior. | Unsupported vendor/driver or incorrect movement requires Experimental/Excluded disposition or narrower matrix. | Open |
| `RHI-E12` | Resize, minimize, restore, frame pacing, and presentation are stable. | Repeat resize/minimize/restore/VSync/back-buffer/frame-count combinations per backend with validation and clean exit. | Hang, device loss, stale viewport, or pacing fault expands to a long stability loop. | Open |
| `RHI-E13` | Capture and diagnostic paths are trustworthy. | Capture a known-color/depth-compatible product, verify dimensions/channel encoding, and provoke one validation/DRED-friendly failure in a development build. | Misleading/empty diagnostics block public diagnostic claims even if rendering continues. | Open |
| `RHI-E14` | Resource budgets and delayed retirement remain bounded. | Record RHI/renderer allocation and retirement over repeat load/switch/resize for a representative scene. | Monotonic growth or budget oversubscription triggers allocation-lifetime capture before performance work. | Open |
| `RHI-E15` | Unsupported RHI vocabulary, partial modes, and backend-asymmetric paths remain unreachable or visibly bounded. | Enumerate public selectors/capability reports, request unsupported mesh/task/stage/topology/blend/bindless/HDR/refit/native-traversal/provider combinations, and verify rejection or explicit requested-versus-active reporting. | Any selectable path that silently substitutes, appears supported, or enters an unimplemented contract must be removed, disabled, or assigned a narrower release disposition. | Open |
| `RHI-E16` | Device-service creation, publication, resize recovery, settlement, destruction, and terminal device-loss behavior are complete and honestly distinguished. | Execute `CHK-RHI-LIFE-*`: inject every partial-create stage, wrong-thread/settlement misuse, resize/out-of-date churn, wait failure, D3D12 removal, and Vulkan device loss; inspect cleanup, generations, diagnostics, and every post-loss caller. | Partial publication, use after settlement/loss, stale swapchain identity, false completion, leak, hang, or any undocumented recovery claim blocks the backend lifecycle. | Open |

## Renderer Evidence

### Renderer capability-to-evidence map

| Evidence item | Direct Renderer capability scope |
| --- | --- |
| `REN-E01` | `REN-OWN-01`, `REN-OWN-03` through `REN-OWN-06` |
| `REN-E02` | `REN-OWN-02` through `REN-OWN-04`, `REN-FG-08`, `REN-SCENE-01`, `REN-SCENE-02` |
| `REN-E03` | `REN-MAT-01` through `REN-MAT-10`, `REN-GBUF-01` through `REN-GBUF-08`, `REN-FRONT-01` |
| `REN-E04` | `REN-GBUF-01` through `REN-GBUF-08`, `REN-FRONT-02` through `REN-FRONT-04` |
| `REN-E05` | `REN-FRONT-05` |
| `REN-E06` | `REN-PBR-01` through `REN-PBR-05` |
| `REN-E07` | `REN-PBR-06` through `REN-PBR-10` |
| `REN-E08` | `REN-LGT-01`, `REN-LGT-02` |
| `REN-E09` | `REN-LGT-03`, `REN-LGT-05` through `REN-LGT-07` |
| `REN-E10` | `REN-LGT-04`, `REN-LGT-05` |
| `REN-E11` | `REN-SCENE-03` through `REN-SCENE-07` |
| `REN-E12` | `REN-RT-01` through `REN-RT-05` |
| `REN-E13` | `REN-POST-01` through `REN-POST-03` |
| `REN-E14` | `REN-POST-04` |
| `REN-E15` | `REN-POST-05` |
| `REN-E16` | `REN-POST-06` |
| `REN-E17` | `REN-POST-07` through `REN-POST-10` |
| `REN-E18` | `REN-FRONT-06`, `REN-DBG-01` through `REN-DBG-04`, `REN-POST-10` |
| `REN-E19` | `REN-FG-01` through `REN-FG-08`, `REN-PIPE-01` through `REN-PIPE-05`, `REN-RT-06`, `REN-DIAG-08` |
| `REN-E20` | `REN-SCENE-08` through `REN-SCENE-10`, `REN-DIAG-02` through `REN-DIAG-04` |
| `REN-E21` | `REN-OWN-01`, `REN-UI-01` through `REN-UI-04`, `REN-DIAG-01` through `REN-DIAG-07` |
| `REN-E22` | All Renderer capability IDs admitted to the frozen release profile; this is a terminal aggregate gate, not a substitute for the narrower items above. |
| `REN-E23` | `REN-SET-01` through `REN-SET-05`, `REN-FRONT-07`, plus every Renderer CVar, public settings field, viewport request, editor control, and provider selector discovered by the selector audit. |
| `REN-E24` | `REN-VOL-01` through `REN-VOL-03` |
| `REN-E25` | `REN-DECAL-01` through `REN-DECAL-03` |
| `REN-E26` | `REN-POST-11` |
| `REN-E27` | `REN-POST-12` |
| `REN-E28` | `REN-POST-13` |
| `REN-E29` | `REN-TEMP-01` through `REN-TEMP-05` |
| `REN-E30` | `REN-LAT-01` through `REN-LAT-05` |
| `REN-E31` | `REN-PIPE-01` through `REN-PIPE-05`, `REN-DIAG-08` |
| `REN-E32` | `REN-VIS-01` through `REN-VIS-09` |
| `REN-E33` | `REN-RESO-01` through `REN-RESO-07`, with shared `REN-TEMP-01` through `REN-TEMP-05` and `REN-POST-04` through `REN-POST-06` joins |

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `REN-E01` | Serial and render-thread execution preserve output and lifecycle. | Run the same short deterministic camera path in both execution modes; compare logs/frame identities/captures and repeat exit. | Mismatch, deadlock, dropped control, or stale generation triggers focused concurrency tracing. | Open |
| `REN-E02` | Scene deltas, reset, and history invalidation are complete. | Add/move/change/remove one mesh/material/light/camera, switch/reload scene, and inspect first stable frames and history resets. | Ghosting/stale resources/missing invalidation blocks affected mode. | Open |
| `REN-E03` | Raster GBuffer encodes its declared material contract. | Render a controlled matrix covering every component, alpha mask, double-sided normal, skeletal motion, morph motion, and depth; inspect raw attachments. | Incorrect channel/space/default blocks downstream lighting evidence. | Open |
| `REN-E04` | Ray GBuffer matches raster semantics where both apply. | Compare raw GBuffer attachments for the same controlled matrix in Raster, strict Inline, and strict Pipeline modes. | Differences must be classified as intended or fixed before frontend inclusion. | Open |
| `REN-E05` | Unsupported transparency is not exposed as completed PBR. | Audit importer/material/editor/runtime selectors and render one blend-mode material. | Reachable blend/transmission that renders as opaque requires exclusion/UX correction or implementation. | Open |
| `REN-E06` | The active compiled BRDF defaults are physically and numerically sane, and inactive source alternatives are not advertised. | Confirm cooked shader provenance uses GGX/Smith-correlated/Schlick, Burley, wrap subsurface, Jimenez multibounce AO, and no specular occlusion; run canonical roughness/metallic/F0/light-angle sweeps. | NaN, instability, implausible energy, or reachable undocumented macro variants block the affected material/lighting claim. | Open |
| `REN-E07` | Every light type and hard capacity behaves honestly. | Render directional/point/spot/rect fixtures, shadows on/off, zero/range/cone/size edges, and exact/over-limit counts. | Silent truncation or corrupt overflow requires visible policy and release classification. | Open |
| `REN-E08` | ReSTIR direct reuse and shadow visibility are stable. | Capture reservoir/lighting outputs over static and moving camera/light/geometry cases with temporal/spatial stages isolated. | Bias, disocclusion trails, invalid samples, or pipeline/inline mismatch blocks inclusion. | Open |
| `REN-E09` | ReSTIR indirect and bounce controls behave as documented. | Exercise 0/1/multi/max bounce, static accumulation, disocclusion, and scene reset on both advertised backends. | Unbounded noise/ghosting/stall or backend failure narrows or excludes the mode. | Open |
| `REN-E10` | The offline path-traced mode is a valid comparison path only within an accepted transport and dependency domain. | First pass [`PTD-00`](../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/Discovery.md); after the authorized plan is implemented, run its analytic/minimal/independent oracle ladder, controlled replicate convergence, deterministic sample/resume/reset, raw finite-value/precision diagnostics, direct/indirect/emissive accounting, and paired-backend checks. | A shared, mismatched, biased/mislabeled, stale, correlated, unconverged, post-processed, or operationally ambiguous output blocks its use as quality reference. | Open; discovery first |
| `REN-E11` | Deforming raster/ray geometry remains spatially identical. | Animate eight-influence skinning and morph targets while comparing raster position/motion against ray hits/shadows. | Divergence blocks ray-traced animation; measure CPU and BLAS rebuild costs before inclusion. | Open |
| `REN-E12` | Classic and partitioned TLAS preserve scene identity. | Render the same motion/visibility/alpha fixture through each available TLAS route and inspect instance/SBT mapping. | Identity or visibility divergence blocks the optional TLAS path. | Open |
| `REN-E13` | Exposure modes and history are stable. | Use controlled luminance steps, camera cuts, resize, viewport override, min/max, and adaptation-speed cases; record exposure values. | Flicker, lag outside contract, or invalid reset blocks automatic exposure choices. | Open |
| `REN-E14` | Linear upscale has defined quality and cost. | Compare native versus representative scale factors on UI edges, geometry, motion, and lighting; record time and output extent. | Artifact or cost outside release budget narrows allowed scale factors. | Open |
| `REN-E15` | DLSS Super Resolution has correct integration and fallback. | On one supported and one unsupported configuration, exercise every reachable quality mode, motion/depth/exposure inputs, resize, reload, and provider failure. | Crash, stale provider, wrong input, or dishonest fallback blocks DLSS inclusion. | Open |
| `REN-E16` | DLSS Ray Reconstruction has correct ReSTIR integration and fallback. | Validate all declared inputs, history resets, unsupported mode combinations, provider loss, and quality/performance on supported hardware. | Any silent substitution or severe reconstruction artifact blocks inclusion. | Open |
| `REN-E17` | Tone mapping/output encoding are correct and finite. | Use known linear ramps and HDR values for all mapper/encoding combinations; inspect output numerically and in capture. | Double encoding, clipping, NaN, or wrong automatic choice blocks affected option. | Open |
| `REN-E18` | Every debug view presents the intended quantity. | Capture all 16 modes on a controlled fixture and both frontends/backends where applicable; document unavailable combinations. | Tone/exposure corruption or stale/missing target blocks the view or requires Experimental classification. | Open |
| `REN-E19` | Graph rebuild, shader reload, and provider retirement are safe. | Change extent/mode/provider/shader generation while frames are in flight; run validation and repeat. | Use-after-free, old-generation use, deadlock, or partial reload blocks hot reload/provider switching. | Open |
| `REN-E20` | Mesh/texture concurrency and total residency stay bounded. | Stream a content set larger than 16 items, record active jobs, decoded memory, GPU memory, stalls, cancellation, unload, and repeat. | Concurrency limit without total-memory control or monotonic retention blocks heavy-content release. | Open |
| `REN-E21` | Diagnostics, UI composition, preview, and capture products are accurate, attributable, bounded, and usable. | Compare reported counts/timings/memory, host/editor UI composition, texture-handle lifetime, and preview/capture output against one native capture and known scene inventory; verify requested/resolved product, viewport request, frame, scene, shader, provider, graph-topology, format, and encoding provenance. | Incorrect values, stale/wrong UI texture, unbounded registry growth, or provenance that cannot distinguish stale/wrong/color-misinterpreted output require contract correction; they are not harmless diagnostic omissions. | Open |
| `REN-E22` | The required release profile meets quality, frame-time, memory, and stability gates. | Only after feature correctness closes, run the acceptance workloads on minimum/reference machines using packaged bytes. | Any earlier correctness/native-validation failure stops this broad performance run. | Open |
| `REN-E23` | Every Renderer selector has a real owned effect or explicit rejection and reports requested versus active state. | Enumerate every Renderer CVar/public setting/editor control/viewport field/provider choice; for each, trace registration -> persisted/UI producer -> runtime consumer -> topology/history effect -> diagnostic, and exercise valid/invalid values. Begin with the currently unconsumed `r.Material.BindingMode`. | Any registered but ineffective, silently clamped/substituted, unreachable, or falsely active selector must be removed, disabled, wired to a complete feature contract, or classified explicitly before scope freeze. | Open |
| `REN-E24` | Volumetric-lighting absence is explicit and no neighboring feature is advertised as participating-media support. | Audit source/build membership, shader registrations, settings/CVars, editor controls, import/cook material data, scene/GPU payloads, frame-graph products, debug views, and public documentation for fog, medium, volume, transmittance, scattering, atmosphere, and aerial-perspective routes. | Any reachable authored value, selector, product, or claim requires an owned capability row and complete feature contract; sky, alpha masking, or wrap subsurface may not silently stand in for volume transport. | Open; negative capability audit |
| `REN-E25` | Deferred decals remain visibly absent until their separately governed future feature is implemented and proved. | Audit authoring/import/cook, scene/GPU data, selectors, frame-graph passes, shader registrations, GBuffer mutation, secondary-ray material evaluation, editor UX, and release claims; verify no current route presents the target architecture as implemented. | Any reachable decal path must enter the owned delivery and acceptance route and update all frame/inventory/catalog consumers; a partial or silent fallback blocks advertisement. | Open; future feature excluded |
| `REN-E26` | Color grading remains explicit absence rather than an effect inferred from tone mapping or output encoding. | Audit display/view settings, CVars, editor controls, grading parameters, 1D/3D LUT asset/import/cook types, color-space transforms, shader registrations, pass construction, graph products, captures, and public claims. | Any reachable setting, data, pass, product, or claim requires an owned grading contract; a tone-mapper curve may not silently stand in for grading. | Open; negative capability audit |
| `REN-E27` | Chromatic aberration remains explicit absence rather than an effect inferred from reconstruction or filtering artifacts. | Audit view/lens settings, CVars, editor controls, channel/radial models, shader registrations, pass construction, guard-band and viewport behavior, graph products, captures, and public claims. | Any reachable setting, pass, product, or claim requires an owned lens-effect contract; accidental color fringing may not be advertised as the feature. | Open; negative capability audit |
| `REN-E28` | Frame generation remains explicit absence and Reflex/PCL or temporal reconstruction is never presented as generated-frame support. | Audit Streamline feature registration/evaluation, provider factories, RHI interop, motion/depth/optical-flow resources, frame tokens, UI composition, pacing, swapchain/present, capture/provenance, settings, packaging, and public claims for a generated-frame route. | Any synthesis/provider route requires rendered-versus-generated identity and complete latency/presentation/failure contracts; SDK or Reflex presence alone may not produce a claim. | Open; negative capability audit |
| `REN-E29` | Per-view temporal sampling, previous-camera publication, history invalidation, motion, reprojection, and provider constants share one exact convention. | Run the `CHK-TMP-*` sequence: exact 16-frame samples, dual-view continuity, every invalidation cause, static/camera/rigid/skin/morph/sky motion, ReSTIR reprojection, and reachable provider constants on both native backends. | Wrong sign/unit, stale history, cross-view state, consumer divergence, or an exposed undocumented jitter pattern blocks the affected temporal consumer. | Open |
| `REN-E30` | Optional latency markers are correctly attributed and bounded without becoming a false Reflex, backend-parity, frame-generation, or latency-benefit claim. | Run the `CHK-LAT-*` matrix for host marker order/identity, Streamline on/off, D3D12/Vulkan, PCL/Reflex support/readiness, provider failures/shutdown, and 32-bit token-wrap boundary; measure benefit only after correctness. | Wrong/missing marker identity, silent active claim, wrap collision, provider failure/deadlock, or unqualified latency marketing blocks the capability. | Open |
| `REN-E31` | Registered/cooked shader identity, typed bindings, complete pipeline keys, backend materialization, build membership, and whole-generation replacement agree. | Exercise the `CHK-PIP-*` matrix: registration/metadata enumeration, one-field key mutations, every binding domain and failure, in-flight reload, smallest shared/static/contract-only build consumers, and native validation. | Missing/duplicate registration, ABI drift, incompatible cache reuse, partial generation activation, early retirement, or backend semantic mismatch blocks the affected pipeline family. | Open |
| `REN-E32` | Per-view visibility, candidate validation, authored grouping, deterministic sorting/batching, workload facts, and negative advanced-draw boundaries are correct. | Execute `CHK-VIS-*`: analytic frustum/bounds cases, exact synthetic candidate/batch ledgers, batching on/off raw-product equivalence, task boundary/failure injection, and selector/source reachability for occlusion/LOD/indirect/stereo/multiview. | Missing/duplicate/wrongly ordered identity, partial failure publication, visual mismatch, misleading diagnostics, or reachable unowned draw mode blocks the affected path. | Open |
| `REN-E33` | Output/render extents, provider resolution, temporal sampling, active attachment sample count, resize invalidation, and absent AA/dynamic-resolution claims agree end to end. | Execute `CHK-RESO-*`: dimension ledger over unity/odd/minimum/provider ratios, requested/active provider matrix, resize/provider churn, attachment/pipeline sample-count enumeration and negative selector/source audit. | Mixed extents, stale history, wrong dispatch/product metadata, implicit resolve, false MSAA/post-AA/dynamic-resolution claim, or dishonest provider mode blocks the affected route. | Open |

## Shader Compilation And Delivery Evidence

### ShaderCompiler capability-to-evidence map

| Evidence item | Direct ShaderCompiler capability scope |
| --- | --- |
| `SHD-E01` | `SHD-CLI-04`, `SHD-CAT-01` through `SHD-CAT-12`, `SHD-REG-01` through `SHD-REG-05` |
| `SHD-E02` | `SHD-BUILD-01`, `SHD-BUILD-03`, `SHD-BUILD-05`, `SHD-CLI-02`, `SHD-CLI-03`, `SHD-TGT-01`, `SHD-TGT-02` |
| `SHD-E03` | `SHD-OPT-02` through `SHD-OPT-06`, `SHD-TGT-01`, `SHD-TGT-02`, `SHD-CAT-01` through `SHD-CAT-07`, `SHD-ABI-01` through `SHD-ABI-10`, `SHD-PUB-01` through `SHD-PUB-03`, `SHD-DIAG-01` through `SHD-DIAG-03` |
| `SHD-E04` | `SHD-OPT-03`, `SHD-PLAN-03`, `SHD-PLAN-07`, `SHD-PUB-06` |
| `SHD-E05` | `SHD-CLI-01`, `SHD-OPT-01`, `SHD-PLAN-01` through `SHD-PLAN-05`, `SHD-PLAN-07`, `SHD-PUB-07` |
| `SHD-E06` | `SHD-OPT-07`, `SHD-PUB-03` through `SHD-PUB-05` |
| `SHD-E07` | `SHD-REG-03` through `SHD-REG-05`, `SHD-ABI-01` through `SHD-ABI-10` |
| `SHD-E08` | `SHD-BUILD-02`, `SHD-BUILD-04`, `SHD-CLI-05`, `SHD-REG-05`, `SHD-PUB-01` through `SHD-PUB-05`, `SHD-DIAG-04` |
| `SHD-E09` | `SHD-PUB-03` through `SHD-PUB-07`, `SHD-DIAG-05` through `SHD-DIAG-07` |
| `SHD-E10` | `SHD-OPT-07`, `SHD-DIAG-05` |
| `SHD-E11` | `SHD-BUILD-04`, `SHD-DIAG-08` |
| `SHD-E12` | `SHD-CLI-02`, `SHD-CLI-03`, `SHD-TGT-02`, `SHD-CAT-08` through `SHD-CAT-12`, `SHD-PLAN-06` |

| ID | Claim to establish | Smallest next check | Escalation trigger | State |
| --- | --- | --- | --- | --- |
| `SHD-E01` | Registration inventory and contracts are internally valid. | Build the smallest tool/registration targets and run `ShaderCompiler list-shaders --validate`. | Contract error blocks all cook/runtime evidence. | Open |
| `SHD-E02` | Tool dependencies are reproducibly discovered. | Configure/build ShaderCompiler from a recorded clean dependency/toolchain state; run `list-backends` and `list-targets`. | Host-specific SDK/DLL/module dependency triggers build/staging correction. | Open |
| `SHD-E03` | The two runtime targets cook completely. | Run full cook for `DxilSm66` and `SpirV16`; retain logs, map/library hashes, dependencies, stats, and debug artifacts for one representative program. | Any compiler/reflection/ABI failure blocks the affected backend/feature. | Open |
| `SHD-E04` | Serial and parallel cooks publish identical products. | Cook the same input with parallelism 1 and 4 into isolated destinations and compare product identities/content. | Nondeterminism triggers indexed-integration/input-identity diagnosis before incremental evidence. | Open |
| `SHD-E05` | Changed-source cooking is dependency-correct. | Change one leaf, one shared include, and one removed registration in controlled workspaces; compare planned and published entry sets. | Missed dependent or stale entry blocks editor incremental recook. | Open |
| `SHD-E06` | Publication is atomic and prior products survive failure. | Execute existing cancellation, traversal, and missing-ID checks; add locked-file/partial-publication exercise only if the existing route does not cover the observed risk. | Any corrupt/mixed map-library generation blocks cooking and hot reload. | Open |
| `SHD-E07` | Reflection matches RHI pipeline layouts on both targets. | Inspect representative compute, raster, raygen, miss, closest-hit, and any-hit entries; create their runtime pipelines with native validation. | Descriptor/push/local-record mismatch blocks the program family. | Open |
| `SHD-E08` | Runtime loads only exact compatible publications. | Launch with valid, missing, mismatched-hash, missing-target, stale-registration, and parameter-signature-corrupt products; verify explicit failure/fallback policy. | Plausible output from incompatible artifacts blocks release. | Open |
| `SHD-E09` | Hot reload swaps whole generations safely. | Recook all, selected, and changed shaders while rendering; verify generation change only after complete validation and retirement after queue completion. | Partial swap, stale pipeline, deadlock, or crash blocks public hot reload. | Open |
| `SHD-E10` | Editor cancellation and process handoff are correct. | Cancel before compile, during compile, and before publish; close/reopen editor; inspect child status, artifacts, and user diagnostics. | Orphaned process or ambiguous success blocks editor workflow. | Open |
| `SHD-E11` | Shipping contains no shader compiler/source/private diagnostics. | Inspect final package manifest/imports/files and run it outside the repository with source/tool trees unavailable. | Any hidden source/compiler dependency blocks package acceptance. | Open |
| `SHD-E12` | Unsupported compiler/stage claims remain unreachable. | Audit UI/CLI/docs/package for non-runtime targets and Slang ray-stage claims; request unsupported combinations and verify rejection. | Reachable unsupported option must be removed, disabled, or classified honestly. | Open |

## Evidence Handoff

The [inventory evidence marks](../Architecture/Modules/README.md#evidence-state) describe the source snapshot; the [acceptance contract](../Acceptance/FirstRelease.md) owns release evidence meaning and approval. Executed results belong in the applicable `FCR-*` candidate report with its iteration traceability, risk/AC/FM/check ledgers, and artifacts. This plan only identifies the next claim-falsifying check.

Evidence may close one row without promoting neighboring rows. For example, a successful D3D12 inline ray-GBuffer run does not prove Vulkan, native pipeline, direct shadows, reference lighting, ReSTIR indirect, partitioned TLAS, or performance.
