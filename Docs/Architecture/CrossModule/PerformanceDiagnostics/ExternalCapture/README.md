# External GPU Capture Integration

**Status:** feature dossier; source-backed target design and conditional implementation package

**Scope:** make native capture tools usable through Sparkle's existing launch and viewport routes in priority order **PIX -> Nsight -> RenderDoc -> specialist tools**.

**Verified baseline:** 2026-10-06, revision `82528cbe5edbcf729464fbefe6998435730920eb`; clean before this documentation change.

**Authority boundary:** this dossier owns scope, support disposition and feature-local acceptance. It refines external capture within [Performance Diagnostics](../README.md); it does not own internal stats, GPU timestamps, benchmark export, or release results.

**Current readiness:** **0/100 — target only** for launch-to-native-artifact capture. Existing markers are separate foundations; this package adds no credit to the parent readiness projection.

Start with [Plan Stage 0](Plan.md#stage-0--freeze-the-pix-discovery-cell). The design is documented and the prompts are ready to execute conditionally; no installed SDK/tool matrix or capture has passed yet. Existing code emits D3D12 PIX events and Vulkan debug labels but has no external capture provider, provider launch selector, or viewport capture action in the audited baseline.

## Reader Route

| Need | Owner |
| --- | --- |
| Current source and Unreal/Unity/Godot/vendor precedent | [Research](Research.md) |
| Decisions, missing environment proof and risk owners | [Discovery](Discovery.md) |
| Request/result, target identity and terminal-state rules | [Semantics](Semantics.md) |
| Module placement, feature homes, lifetimes, integration budget | [Execution Architecture](ExecutionArchitecture.md) |
| First use, launch, capture, failure/recovery and automation | [User Experience](UserExperience.md) |
| Ordered implementation slices and copy-ready prompts | [Plan](Plan.md) |
| Version-sensitive profiler operations | [External Profiling runbook](../../../../Engineering/Verification/ExternalProfiling.md) |

Acceptance is kept below because its binary capture criteria share this dossier's scope and lifecycle. No eighth companion is needed.

## Product And Support Disposition

The user chooses a tool at launch, sees honest readiness beside the intended scene viewport, requests a capture, and receives the native artifact or confirmed native-tool handoff with exact correlation and limitations. No-provider behavior remains the ordinary engine path. The feature does not replace the native tools' viewers, hardware counters, shader debuggers, memory analyzers, or crash inspectors.

| Priority / identity | Activity and declared matrix | Disposition / delivery |
| --- | --- | --- |
| 1 / `EXT-01` | PIX GPU frame capture, Windows D3D12, Debug/Development Editor and Game | Selected target; minimal PIX vertical slice first, then lifecycle/adoption. Vulkan is excluded for this activity. |
| 2 / `EXT-04`, `EXT-05` | Nsight Graphics Capture, supported NVIDIA Windows D3D12 and Vulkan, Debug/Development | Selected experimental target, SDK/driver/activity gates per backend. Accepted workflow does not remove SDK beta warning. |
| 3 / `EXT-02`, `EXT-03` | RenderDoc frame debugging, supported Windows D3D12 and Vulkan, Debug/Development | Selected target after Nsight cells; negotiated API and current graphics-feature gates. Unsupported NVAPI/PTLAS/other vendor feature combinations reject explicitly; no silent renderer fallback. |
| 4 / specialist route | PIX Timing; Nsight GPU Trace/Systems; AMD RGP/RMV/RRA/RGA/uProf; WPR/WPA; PresentMon | Tool-managed handoff selected; bounded automatic integration only with a named consumer and a separate frozen activity contract. |
| 4 / crash route | Nsight Aftermath and Radeon GPU Detective | Runbook/provenance handoff selected. SDK crash-monitor installation, crash-handler ownership, privacy and retention are separate discovery, not frame-capture adapter work. |
| Every provider / ShippingEditor and ShippingGame | Optional diagnostics | Excluded at compile/link/package time; no capture parser or runtime enabling path. |
| macOS/Linux/mobile/console; Intel-specific SDK integration; embedded counters/viewers | New platform/hardware or product authorities | Outside this package. Revisit with an explicit consumer and capability study; no implied portability. |

`EXT-00` is the bounded neutral spine delivered with real PIX, not a standalone framework. Stable package identities survive the priority change. Native artifact locations follow typed user-state roots; candidate reports link retained artifacts rather than committing large captures.

## Acceptance And Check Contract

Checks use the [claim-driven verification standard](../../../../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). Every execution card supplies exact candidate, configuration, commands/workflow, oracle, repetitions, artifact/hash, cleanup, observer settings and escalation. No new test-only files/classes/executables are submitted; disposable probes remain local and are removed before handoff.

| Criterion | Binary pass condition | Checks |
| --- | --- | --- |
| `AC-EC-01` | Valid GUI/CLI launch intent matches; invalid/duplicate/unsupported intent rejects; no-provider ordinary run does not search/load/write capture resources. | `CHK-EC-LAUNCH`, `CHK-EC-PACKAGE` |
| `AC-EC-02` | Capturer/activity is initialized at the verified early boundary; marker-only and UI-connected facts cannot set Ready; missing installation/compiled adapter is explicit. | `CHK-EC-BOOT` |
| `AC-EC-03` | Native capture contains the requested scene contribution and declared target/delimiter interval; confirmed identity matches markers; stale/wrong window cannot return success. | `CHK-EC-TARGET`, `CHK-EC-NATIVE` |
| `AC-EC-04` | One active lease, exactly one terminal result per accepted request; Busy/conflict/cancel/timeout/drain/shutdown/late-result behavior conforms to EC-S07–09. | `CHK-EC-LIFE`, `CHK-EC-COMBINATION` |
| `AC-EC-05` | Finalized native artifact or confirmed native handoff opens in the named tool, exposes pass/resource/pipeline identity, and preserves shader/source identity or explicit unavailability. No schedule-call/file-existence surrogate is used. | `CHK-EC-NATIVE`, `CHK-EC-SYMBOL` |
| `AC-EC-06` | Presenter uses the same authoritative operation as automation; hidden/no-provider, unavailable/experimental/busy/finalizing/error/recovery and accessible actions are truthful. | `CHK-EC-UX` |
| `AC-EC-07` | Every accepted backend/profile/API-feature/tool-version cell and requested pair/triple has a retained supported/unsupported/blocked disposition; missing tools do not manufacture feature rejection. | `CHK-EC-MATRIX`, `CHK-EC-COMBINATION` |
| `AC-EC-08` | Shipping builds/packages without external profiler SDKs and contains no optional capture code/state/parser/UI/markers/imports/dependencies; eligible no-provider behavior is preserved. | `CHK-EC-PACKAGE`, `CHK-EC-OBSERVER` |
| `AC-EC-09` | Frozen capacity/deadline/idle-cost controls pass, native capture observer cost is reported, parallel recording/queue topology is preserved or explicitly classified nonrepresentative. | `CHK-EC-LIFE`, `CHK-EC-OBSERVER` |
| `AC-EC-10` | Every implementation stage fits the homes/hook allowlist, has one authority, deletes replaced touched paths, and passes dependency/bounded-removal checks. | `CHK-EC-ARCH` |
| `AC-EC-11` | A second engineer or clean-environment adopter reproduces launch -> capture -> inspect -> recover using retained configuration and documentation; specialist routes name real distinct activities/artifacts. | `CHK-EC-ADOPT` |

| Failure / cause | Detection and safe state / visible recovery | Severity / check / risk |
| --- | --- | --- |
| `FM-EC-01`: missing SDK/tool, marker-only runtime, early-load failure | Capability unavailable with one setup reason; continue only with proven rollback, otherwise fail launch. Relaunch after setup. | Workflow-blocking; BOOT/LAUNCH; RISK-EC-01/05. |
| `FM-EC-02`: stale/destroyed/minimized/zero-size target, alternate window presents | No wrong-frame success; reject or bounded Armed timeout, keep original target identity. Restore target and retry. | Evidence-corrupting; TARGET/LIFE; RISK-EC-02. |
| `FM-EC-03`: conflicting injected layers, native hotkey capture, second request | Busy or conflict before unsafe operation; retain state, clean relaunch for unsafe attachment. No hook unload workaround. | Process/evidence risk; COMBINATION; RISK-EC-03. |
| `FM-EC-04`: timeout, device loss, shutdown, late callback | Terminal once, then quarantine until native quiescence; UI detached safely, next request blocked where drain is uncertain. Relaunch when necessary. | Lifetime-critical; LIFE; RISK-EC-04. |
| `FM-EC-05`: unwritable/full disk, long/Unicode path, file removed, native open failure | No false finalized artifact; report path/error and preserve last unrelated result. Retry with existing capture root or open manually when artifact itself is valid. | Workflow/evidence risk; NATIVE/UX; RISK-EC-04/06. |
| `FM-EC-06`: wrong shader/binary/tool identity, missing symbols or replay feature | Reject correlation claim or mark unavailable; keep native artifact and exact provenance, rerun supported cell without silent setting changes. | Diagnosis-corrupting; SYMBOL/MATRIX; RISK-EC-07. |
| `FM-EC-07`: capacity exceeded, incompatible observer defaults, Shipping dependency | Reject bounded operation, expose observer/profile mismatch; remove leaked dependency or expensive idle behavior before acceptance. | Product/performance risk; PACKAGE/OBSERVER/LIFE; RISK-EC-06. |

Check names in the failure table abbreviate the matching `CHK-EC-*` identity below.

### Executable Check Cards To Freeze Before Implementation

| Check | Initial state / action and independent oracle | Matrix, threshold, artifacts, cleanup, escalation |
| --- | --- | --- |
| `CHK-EC-LAUNCH` | Compare normalized Launcher/direct CLI intent; unknown/duplicate ID and no-provider controls. Oracle is exact expected set/rejection and process load trace. | Each ID, pair/triple and eligible profile; zero silent provider/backend changes. Retain argv/intent/error/load trace; no default settings persisted. Bootstrap if load differs. |
| `CHK-EC-BOOT` | Cold start attached, explicitly requested, missing tool and event-runtime-only. Trace first graphics/interposer call and loaded module/activity. | Each provider/backend; Ready iff verified capability. Retain tool/header/module hashes and order; fresh process between cases. Probe or stop if native API order ambiguous. |
| `CHK-EC-TARGET` | Empty then ready Sponza; alternate containing window/target, resize/destroy/reuse generation while Armed. Oracle is native event interval and requested scene's marker/output. | Three captures per supported target shape; zero wrong-target successes. If live multi-window topology is absent, local target seam probe plus explicit unsupported live cell; never claim exercised topology. Retain marker map/result; remove injector. |
| `CHK-EC-LIFE` | Inject deterministic delayed/duplicate completion, failed start, deadline, unsafe cancel, loss, destroy and shutdown using local-only narrow harness plus native controllable cases. | Exact terminal count = 1; new captures forbidden until quiescent; caps/deadlines from D04. Retain transition/native traces; remove probe; escalate unresolved SDK lifetime to blocked cell. |
| `CHK-EC-NATIVE` | Capture known Sponza pass/output, poll real finalization, open actual file in native tool; inject inaccessible/full output and removed artifact. | Three successful captures per accepted cell; identify pass, output resource and pipeline. Zero completion from schedule/file existence. Retain native capture/hash and inspection notes; captures remain in user-state root. Replay incompatibility blocks that setting cell. |
| `CHK-EC-SYMBOL` | Join captured shader to exact cooked bytecode/binary and debug data; deliberately offer mismatched symbols. | At least one representative raster/compute shader and RT shader if present/accepted; wrong identity never accepted. Retain hashes/source match and unavailable cases; no all-content cook unless exact symbols require it. |
| `CHK-EC-COMBINATION` | No provider, each alone, three pairs and triple; explicit Streamline/interposer and validation settings, native hotkey then UI click. | Each accepted backend/version tuple; untested combinations reject before load; no overlap. Retain disposition/load/capture evidence; clean process per cell. Single-provider success cannot escalate tuple to supported. |
| `CHK-EC-UX` | Clean UI/CLI first use, disabled reasons, narrow layout, keyboard, scale, tooltip and capture-open failure. Compare emitted request to same controller operation. | Zero provider-state copies in UI; accessible name/text/focus at supported scales; no-provider group absent. Retain transcript/screenshots as UX evidence only; no GPU proof inferred. |
| `CHK-EC-MATRIX` | Empty/Sponza shakedown then existing runtime-supported-map roster per provider's declared backend; inspect current catalog and feature settings. | Every included provider/profile/native-feature cell classified; expensive retained captures may be linked. All-map final sweep is justified by parent P1 gate, not every small edit. Retain roster/readiness/limitations; unsupported tool feature cannot disable engine feature silently. |
| `CHK-EC-PACKAGE` | Smallest affected targets first; final ShippingEditor/Game configure/build with capture SDKs absent, import/link-map/string/disassembly/staging audit and Empty/Sponza smoke. | Zero optional capture payload/callsite/dependency; tool-free normal run. Retain commands/artifact hashes; reuse canonical build, no root build-* directory. Broad product builds only for this erasure claim. |
| `CHK-EC-OBSERVER` | Identical optimized Empty/Sponza controls: no provider, attached idle, active capture; record native-validation and symbol modes separately. | D04 idle thresholds/noise floor fixed first; at least three runs of 300 warm valid frames for numerical comparisons. Capture disturbance reported, not promised negligible. Retain raw samples/topology/memory; no perf claim from replay. |
| `CHK-EC-ARCH` | Exact outside-home file/symbol ledger, public/dependency/copy audit, repeated provider-switch search, bounded-removal review, responsibility pass. | Zero unexplained hook/duplicate state; `architecture_boundary_check` and `git diff --check`. Retain scoped diff/commands; no source-removal experiment on user work. New hook triggers D07 review. |
| `CHK-EC-ADOPT` | Non-author/clean-environment transcript of documented launch/capture/inspect/recovery and chosen specialist route. | No private repair; retained settings suffice. Retain transcript/native artifacts/limits; missing second reviewer/adopter remains blocked evidence, not self-certified adoption. |

## Completion And Result Ownership

Each provider can be delivered when its own relevant criteria, failures and checks pass; PIX does not wait for unavailable NVIDIA hardware. Complete parent Phase 1 only when `EXT-00`–`EXT-05` have accepted or evidence-backed unsupported dispositions and the parent map/Shipping gates close. An unavailable installation, unrun probe, or temporarily missing hardware is **BLOCKED**, not permission to reject/delete an intended feature.

Specialist Stage 7 closes its named tool-managed lanes separately and does not hold accepted PIX hostage. Full package closure additionally requires all applicable `AC-EC-*`, controlled `FM-EC-*`, independent review/adoption, clean-break cleanup and final report through [Acceptance](../../../../Acceptance/README.md). Candidate `PASS`, `BLOCKED`, `EXCLUDED`, or `SUPERSEDED` results belong in that report, not in these target pages.
