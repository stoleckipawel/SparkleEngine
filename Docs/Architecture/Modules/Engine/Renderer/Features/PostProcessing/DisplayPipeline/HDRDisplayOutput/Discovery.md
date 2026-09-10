# HDR Display Output Discovery

**Status:** acceptance contract; `HDRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the Windows HDR product, scene/display semantics, target and SDR-white policy, D3D12/Vulkan presentation contract, transitions, UX, budgets, and evidence before `DSP-7`

**Authority boundary:** [Research](Research.md) owns external findings; [Semantics](Semantics.md), [Execution Architecture](ExecutionArchitecture.md), and [User Experience](UserExperience.md) are conditional candidates; [Plan](Plan.md) orders delivery; [README](README.md) owns `AC-HDR-*`/`FM-HDR-*`; RHI owns native mechanics, not display policy

**Verified:** 2026-09-10 against committed revision `ca55e7d8`; Renderer/RHI/settings/window/swapchain/UI/capture/package source inspected; no executable HDR hardware evidence was run; concurrent user-owned dirty paths remain outside this gate evidence

**Current readiness:** **0/100** — discovery and research add no readiness credit. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

HDR is not established by a 10-bit texture, a PQ function, a metadata call, or a monitor badge. The accepted route must join one color/luminance contract to an actually activated output tuple and remain truthful through window/display/device transitions and every fallback.

## Why The Existing Target Needs Discovery

The current first-release contract selected a fixed 1000-nit HDR10 target, 200-nit SDR UI white, a 10-bit swapchain, and static metadata. Current Microsoft guidance adds material constraints: FP16/scRGB is the general Advanced Color route, the UINT10/PQ route is suitable only for narrower cases; applications should use the system SDR white level when composing their own UI; and applications should not rely on swapchain metadata being honored. Khronos likewise states that metadata does not choose color space and its use is outside Vulkan's control.

Those findings do not cancel the admitted HDR10 goal. They make fixed target/white, UINT10 eligibility, and metadata role explicit `HDRD-*` decisions instead of settled implementation details.

## Iteration Control Record

| Field | `HDRD-00-R1` requirement |
| --- | --- |
| gate identity | `HDRD-00-R1`; replaces a prose-only gate shape, not an accepted production design |
| claimant | Renderer display owner plus RHI/platform owners and independent color, UX, transition, and evidence reviewers |
| source baseline | revision `ca55e7d8`, exact dirty-state boundary, inspected commands/files, negative-capability record |
| external baseline | exact standard editions, documentation access dates, repository commits, Windows/runtime/driver/display identities |
| accepted package | exact revisions/hashes of README, Discovery, Research, Semantics, Execution Architecture, User Experience, and Plan |
| permitted work | read-only source trace; offline CPU/reference tooling; native capability probes on named hardware; workflow/evidence dry run; docs |
| prohibited work | production Renderer/RHI/window/UI/package changes, selector advertisement, readiness credit, or release support claim |
| verdict | `PASS` only when every discovery criterion passes; otherwise `Blocked` with owner, missing artifact, next probe, and invalidated dependents |
| invalidation | source/platform/profile/hardware matrix drift; standard/guidance change; semantic/tuple/state/UI/metadata/budget/oracle change; release-scope change |

## Discovery Scope

Included are the Windows release profile; D3D12/Vulkan/window/interposer cells; scene-to-target/gamut/tone/PQ or explicitly admitted scRGB semantics; output association and native tuples; current display luminance and SDR-white facts; UI/debug/capture composition; requested/support/eligibility/activation/fallback state; swapchain/display/OS/window/device lifecycle; metadata disposition; SDR preservation; package reachability; performance/memory/latency; physical and non-physical evidence interpretation.

Excluded are non-Windows platforms, HLG, Dolby Vision, dynamic metadata, automatic calibration, ICC/profile management, multiple creative mastering profiles, HDR offscreen/export guarantees, universal screenshot fidelity, player-facing calibration UI, and claims about display behavior outside the named evidence matrix. A discovery probe may observe an excluded route only to justify the boundary; it does not admit it.

## Decision Recording Contract

Each `HDRD-*` disposition records the selected profile/cell/policy, rejected alternatives, normative/platform/source/probe basis, exact types/constants/ranges/events, affected target sections, owner/reviewer/date, evidence consequences, and invalidation triggers. A monitor badge, API success code, sample default, vendor screenshot, or implementation literal is never a decision record.

## Current Source Truth

At `ca55e7d8`:

- [`PixelFormat.h`](../../../../../../../../../Engine/RHI/Public/Formats/PixelFormat.h) has no 10-bit packed RGB format.
- [`RhiPresentationDefaults.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationDefaults.h) admits four 8-bit SDR back-buffer formats only.
- [`RhiPresentationService.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationService.h) exposes current present format but no output capability, color space, luminance, metadata, or requested/active/fallback result.
- [`D3D12SwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/D3D12/SwapChain/D3D12SwapChain.cpp) creates/resizes/presents a format-selected flip swapchain without output/color-space/HDR policy.
- [`VulkanSwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/Vulkan/SwapChain/VulkanSwapChain.cpp) accepts only the requested format paired with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`.
- [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) tone maps, encodes, copies, and publishes an SDR-oriented product; UI is composed through a separate RHI overlay route.

This is a verified source-backed absence. It does not establish which Advanced Color route is correct for Sparkle or that the host/UI composition can use a specific swapchain tuple.

## Blocking Decisions

| ID | Decision | Required disposition |
| --- | --- | --- |
| `HDRD-01` | product and profile | freeze supported Windows versions, window/fullscreen behavior, DevelopmentEditor and packaged Runtime scope, D3D12/Vulkan cells, and explicit scRGB/HLG/dynamic-metadata exclusions |
| `HDRD-02` | scene-working contract | name Renderer scene primaries/white/units, exposure relation, reference diffuse white, legal range, and conversion into the HDR output transform |
| `HDRD-03` | output approach | freeze the packed 10-bit Rec.2020/PQ route required to satisfy the admitted HDR10 result; decide whether any DevelopmentEditor/UI profile additionally uses FP16/scRGB or is explicitly ineligible, and state fallback per profile/backend without calling the two signal encodings equivalent |
| `HDRD-04` | target luminance | ratify fixed 1000-nit mastering, adapt tone mapping to queried output luminance, or define a bounded hybrid; freeze black/peak/FALL policy and user control |
| `HDRD-05` | SDR/UI white | replace or ratify fixed 200 nits against the current system/user SDR white query; define 80-nit reference conversion, update events, bounds, and fallback |
| `HDRD-06` | gamut/tone/PQ semantics | freeze scene-to-Rec.2020 transform, gamut mapping, tone curve, clipping, PQ constants/precision, alpha, dithering, and exactly-once ownership |
| `HDRD-07` | D3D12 activation | freeze output association/capability query, format, color-space support/set, swapchain interface/interposer compatibility, metadata policy, result reporting, and recreation order |
| `HDRD-08` | Vulkan activation | freeze required instance/device extensions, surface format/color-space selection, metadata availability/policy, Win32 presentation behavior, result reporting, and recreation order |
| `HDRD-09` | metadata role | decide whether static metadata is omitted, diagnostic/best-effort, or required; it may never define pixel interpretation or serve as active-HDR proof |
| `HDRD-10` | state and transitions | freeze requested/supported/eligible/activating/active/fallback/error states and display/OS/window/resize/fullscreen/suspend/device/interposer invalidation events |
| `HDRD-11` | UI/capture/debug | freeze SDR UI mapping/composition, debug-view behavior, raw/display/capture identity, screenshot limitations, and observable support output |
| `HDRD-12` | evidence and budgets | freeze HDR/SDR display matrix, measurement/reference patterns, backend rules, transition/fault cases, black/peak/color tolerances, performance/memory/latency budgets, artifacts, and escalation |

## Required Discovery Experiments

| Experiment | Question | Required retained result |
| --- | --- | --- |
| `HDR-EXP-01` current domain/owner trace | where do scene units become display nits and who owns each transform/API state? | one Renderer -> RHI -> OS/display diagram with format/domain/state at every edge |
| `HDR-EXP-02` Windows route probe | which current Sparkle window/UI/profile cells are eligible for UINT10/PQ versus FP16/scRGB? | D3D12 capability/output/alpha/composition/interposer matrix on named Windows versions |
| `HDR-EXP-03` Vulkan route probe | are HDR10 surface color space and metadata extension available and behaviorally usable on the release machine? | extension/surface-format/present/monitor matrix with exact driver/OS/runtime identity |
| `HDR-EXP-04` luminance policy study | how do fixed 1000/200 targets compare with output-reported peak and current system SDR white? | measured/query table, candidate tone/UI mappings, decision rationale, and failure bounds |
| `HDR-EXP-05` semantic oracle | do PQ/gamut/tone/UI transforms match independent high-precision reference values and expose double/omitted transforms? | frozen ramps, patches, matrices, tolerances, and seeded defects |
| `HDR-EXP-06` transition model | can every monitor/OS/window/resize/fullscreen/suspend/device/interposer event reach valid active or SDR fallback state? | state machine with injected query/set/recreate/metadata failures and cleanup |
| `HDR-EXP-07` experience walkthrough | can a person identify active versus requested HDR and recover without a black/washed output? | first-use, unsupported, monitor move, OS toggle, fallback, support, and package journeys |
| `HDR-EXP-08` evidence dry run | can the protocol distinguish correct signal values, OS/display processing, screenshot capture, and visual appearance? | artifact lineage and minimum hardware/measurement matrix with invalid/ambiguous verdict rules |

## Risk Register

| ID | Cause, event, consequence | Prevention/detection | Contingency, owner, retirement |
| --- | --- | --- | --- |
| `RISK-HDR-01` | unnamed scene primaries/units make a numerically valid PQ result colorimetrically wrong | `HDRD-02/06`, independent matrices/ramps | block transform; Renderer display owner retires after semantic oracle passes |
| `RISK-HDR-02` | format is created but color space is unsupported/not set, producing false active state or washed output | tuple-based activation and native inspection | recreate known-good SDR and report reason; RHI owner retires after fault/transition checks |
| `RISK-HDR-03` | fixed 200-nit UI ignores the user's current SDR white, making UI too dim/bright | `HDRD-05`, system query and measured UI patch | use accepted bounded fallback and remain explicit; platform/display owner retires after update/failure proof |
| `RISK-HDR-04` | metadata is trusted as signal interpretation or physical-display proof | `HDRD-09`; pixel/color-space/measurement evidence separate | treat metadata as optional state; acceptance owner retires after checks demonstrate independence |
| `RISK-HDR-05` | display/OS/window/device changes leave stale active state or black output | explicit state machine and event invalidation; fault injection | atomically return to SDR or no-frame safe state; RHI/window owner retires after matrix |
| `RISK-HDR-06` | D3D12 interposer or Vulkan extension path diverges from the native owner contract | explicit interface/capability cells and architecture boundary check | block affected backend/profile; owner retires after native and paired semantic evidence |
| `RISK-HDR-07` | screenshots/captures are interpreted as display evidence despite compositor/display transforms | raw encoded values, native state, external measurement, artifact labels | verdict `Inconclusive`; capture/acceptance owner retires after lineage review |
| `RISK-HDR-08` | HDR expansion destabilizes proven SDR output | stage-by-stage SDR preservation and automatic fallback checks | keep/default to known SDR tuple; display owner retires after complete transition and package matrix |
| `RISK-HDR-09` | Renderer chooses PQ from request while RHI still presents SDR, or RHI activates HDR while Renderer emits SDR | generation-qualified two-party commit and mixed-generation fault injection | withhold publication or use known-good SDR; Renderer/RHI owners retire after transaction proof |
| `RISK-HDR-10` | stale `GetContainingOutput`/display facts survive a display-change or monitor move | event-driven output association with platform generation and forced re-query | invalidate active state and revalidate/fallback; platform owner retires after move/hotplug/straddle evidence |
| `RISK-HDR-11` | adding FP16/scRGB as a convenience silently doubles or confuses signal/UI/capture contracts | separate profile IDs, semantics, tuples, states, artifacts, and acceptance cells; explicit exclusion if unaffordable | keep HDR10 cell only or re-scope; product owner retires after profile matrix review |
| `RISK-HDR-12` | packing/quantization/dither errors create banding, clipping, or nondeterministic reference output | high-precision oracle, code-value hand cases, deterministic dither policy and raw captures | block transform/format; Renderer owner retires after error/quality thresholds pass |
| `RISK-HDR-13` | queried peak/min/full-frame values are invalid, stale, or mistaken for creative mastering policy | validity/provenance fields and bounded product policy separate from capability facts | use documented fallback or reject HDR; display policy owner retires after query-fault evidence |
| `RISK-HDR-14` | fallback itself fails or loops through repeated recreation, leaving indefinite black output | bounded transition attempts, monotonic generations, known-good SDR tuple, black-frame/latency watchdog evidence | enter explicit no-valid-presentation error and stop retry storm; presentation owner retires after fault matrix |

## Discovery Acceptance

| ID | Pass criterion | Evidence |
| --- | --- | --- |
| `AC-HDRD-01` | `HDRD-01` through `12` have reviewed dispositions with no implementation-shaping unknown. | decision table and invalidation triggers |
| `AC-HDRD-02` | every Renderer/RHI/OS/display edge has one owner, domain, format, identity, lifetime, failure, and observable state. | `HDR-EXP-01`, architecture and state-machine revisions |
| `AC-HDRD-03` | D3D12 and Vulkan have truthful eligible/unsupported cells on named release hardware/software. | `HDR-EXP-02/03` raw queries and limitations |
| `AC-HDRD-04` | tone/peak/black/gamut/PQ/UI-white/metadata policies are numerically defined and consistent with current platform guidance or explicitly justified divergence. | `HDR-EXP-04/05`, accepted semantics |
| `AC-HDRD-05` | transition, fallback, UI, debug, capture, and support journeys are reviewable without interpreting intent. | `HDR-EXP-06/07`, accepted UX |
| `AC-HDRD-06` | every `AC-HDR-*`, `FM-HDR-*`, and material risk maps to a defect-detecting check with fixed matrix, threshold, artifact, cleanup, and budget. | `HDR-EXP-08` and no-orphan map |
| `AC-HDRD-07` | full feature/profile/backend/window/display/UI/package/exclusion matrix has one disposition and no reachable undeclared cell | reconciled `HDR-FS-*` ledger, support/reachability matrices, selector/source/package audit |
| `AC-HDRD-08` | resource/performance/transition budgets bound query rate, swapchain memory, transform/UI work, recreation, black frames, retries, diagnostics, captures, and evidence workload | numeric budget sheet and boundary/fault observations |
| `AC-HDRD-09` | every plan stage has prerequisites, estimates, owned result, non-goals, deletions, stop rules, and a prompt that cannot choose policy | accepted Plan and stage dry-run record |
| `AC-HDRD-10` | independent reviewers can reproduce semantic/native/state/fallback/artifact cases and distinguish signal/native/display claims | exact-revision color, D3D12, Vulkan, architecture, UX, and evidence reviews |

## Discovery Failure Modes

| ID | Controlled failure | Required safe result | Detecting checks |
| --- | --- | --- | --- |
| `FM-HDRD-01` | native/color/profile/UI/metadata/budget decision is deferred into code or a stage prompt | production remains unauthorized; move decision to Discovery and invalidate dependent text | `CHK-HDRD-01/08` |
| `FM-HDRD-02` | backend is called active without current output association and complete compatible tuple | no active claim; return explicit ineligible/fallback/error and preserve SDR if possible | `CHK-HDRD-04/05/07` |
| `FM-HDRD-03` | metadata, screenshot, monitor badge, or visual plausibility is the sole oracle | evidence is `Inconclusive`; add raw math/native/measurement lineage | `CHK-HDRD-03/06` |
| `FM-HDRD-04` | fallback/transition can leave no usable output or retry indefinitely | gate remains blocked; bound transaction/retry/black-frame result and explicit error | `CHK-HDRD-07` |
| `FM-HDRD-05` | UINT10/PQ and FP16/scRGB share one ambiguous profile/result/artifact label | split exact contracts or exclude one cell; no generic `HDR active` claim | `CHK-HDRD-02/05/06` |
| `FM-HDRD-06` | Renderer and RHI generations can publish a mixed transform/tuple frame | withhold HDR publication or commit coherent SDR; redesign join before Stage 1 | `CHK-HDRD-05/07` |
| `FM-HDRD-07` | system SDR white/output facts have no validity, event, failure, or fallback rule | UI/HDR profile remains ineligible; no fixed constant is presented as queried fact | `CHK-HDRD-03/04/07` |
| `FM-HDRD-08` | an admitted product/backend/transition/exclusion lacks owner, failure, stage, and proof | no-orphan gate fails and the cell remains unreachable | `CHK-HDRD-02/09/10` |

## Check Design Ledger

Each Stage-0 check declares initial state, action/fault, independent oracle, exact OS/runtime/driver/adapter/display/window profile, thresholds, artifacts, maximum work, cleanup, and escalation.

| ID | Smallest falsifier | Oracle/artifact | Fails when |
| --- | --- | --- | --- |
| `CHK-HDRD-01` | scan package/prompts for unresolved policy leakage | complete `HDRD-*` decision and reference map | code/prompt must invent profile, constant, owner, state, or fallback |
| `CHK-HDRD-02` | enumerate features/profiles/backends/windows/displays/UI/captures/packages/exclusions | no-orphan support/reachability ledger | any reachable/admitted cell lacks exact contract/proof or profiles are conflated |
| `CHK-HDRD-03` | evaluate scene/target/gamut/tone/PQ/UI-white/packing hand cases | independent high-precision tables/images and seeded mutations | a semantic alternative remains ambiguous or wrong transform can pass |
| `CHK-HDRD-04` | probe current output, Advanced Color/luminance/SDR-white facts and all validity/failure paths | raw platform queries tied to window/output generation | stale/invalid facts can establish eligibility or fixed fallback masquerades as query |
| `CHK-HDRD-05` | enumerate/activate candidate D3D12 and Vulkan tuples without Renderer HDR pixels | native format/color-space/output/extension/interface/result traces | incomplete/wrong/current-generation tuple can report active |
| `CHK-HDRD-06` | compare raw target/PQ, native state, compositor capture, external capture, and measurement interpretations | artifact-lineage matrix and deliberately misleading controls | metadata/screenshot/badge/brightness can substitute for its missing evidence class |
| `CHK-HDRD-07` | inject query/create/resize/set/metadata/present/output/device/interposer faults across transitions | state/generation/latency/black-frame/resource trace | mixed generation, stale active, retry loop, leak, or unusable fallback occurs |
| `CHK-HDRD-08` | dry-run every plan prompt with missing decisions/hardware | reviewer transcript and stop/escalation map | executor chooses policy, crosses stages, or cannot name safe rollback |
| `CHK-HDRD-09` | map every criterion/failure/risk/event/matrix cell to checks/artifacts | zero-orphan reviewed mapping | any material claim lacks controlled positive/negative proof and cleanup |
| `CHK-HDRD-10` | independent color, Windows/D3D12, Vulkan, architecture, UX, acceptance review | named corrections and exact-revision disposition | claim depends on private explanation or hardware/source ambiguity |

## Required `HDRD-00-R1` Evidence Package

The gate report retains:

1. exact source revision/dirty boundary, commands/files, negative capability trace, and current owner/domain diagram;
2. exact standard/document/source revisions and named OS/runtime/driver/adapter/display/window/interposer probe matrix;
3. all `HDRD-01` through `12` decisions with alternatives, rationale, owners/reviewers, and invalidation triggers;
4. accepted feature/profile/support/reachability/exclusion matrices and exact companion-document revisions;
5. scene/target/tone/gamut/PQ/UI-white/packing hand cases, tolerances, raw fixtures, and seeded defect controls;
6. D3D12/Vulkan/platform query/tuple/metadata facts, output association, profile eligibility, and limitations;
7. owner/type/state/generation/lifetime/transition/rollback/failure/deletion ledger with mixed-generation and fallback fault results;
8. first-use, monitor/OS/window/device transition, recovery, UI, debug/capture, accessibility, automation, package dry runs;
9. performance/memory/query/recreation/black-frame/retry/capture/evidence budgets and boundary observations;
10. no-orphan mapping, rights/provenance, exact checks run/unavailable, limitations, independent review, and `PASS` or `Blocked` verdict.

## Gate Decision

`HDRD-00` is **Blocked**. A future `PASS` must name the accepted dossier/research/semantics/architecture/experience/plan revisions and the exact release hardware/software cells. It authorizes only Stage 1 of [Plan](Plan.md); `FCR-REN-26` remains blocked until all feature evidence passes.
