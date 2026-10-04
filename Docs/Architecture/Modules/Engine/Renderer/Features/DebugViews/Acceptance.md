# Debug View Presentation - Acceptance

**Status:** feature-local acceptance contract; not a candidate result

**Responsibility:** own binary completion criteria, controlled failures, checks, oracles, and evidence boundaries for Debug Views.

**Current readiness:** **Not applicable** to this contract; current progress remains owned by the [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Verified baseline:** 2026-10-04 at revision `26803f97` with an inspected dirty working tree; candidate evidence must name its own exact revision and state.

**Control-contract revision:** 2026-10-04 at `bbb9f7ed`; lighting controls now require feature execution removal, not per-view composite suppression. No candidate result is added.

**Architecture:** [Viewport Rendering Controls](Controls/README.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Delivery:** [Plan](Plan.md)

Candidate results belong in `FCR-REN-11`. Source inspection does not prove build, runtime, pixel, backend, or release behavior.

## Completion Criteria

- `AC-DVP-01` - every non-sentinel `RenderViewMode` has one contract row and a real production consumer; numeric C++ and HLSL values match exactly, `ReferencePathTracer` is `1`, and values are contiguous through `Count = 18`;
- `AC-DVP-02` - the selected mode has one representation on `ViewportRenderRequest` and immutable `RenderView`; two viewport requests may hold different modes without global cross-talk;
- `AC-DVP-03` - no Editor mirror enum, preset translator, visualization target, mode-shaped show flag, selector CVar, command bridge, settings copy, compatibility alias, or RHI field competes with `RenderViewMode`;
- `AC-DVP-04` - Editor owns labels, icons, grouping, shortcuts, and interaction while Game/runtime may submit the same rendering semantic without an Editor dependency;
- `AC-DVP-05` - the Reference mode schedules only the private Reference middle and Lit schedules only GBuffer/ReSTIR/reconstruction inside the same frame shell;
- `AC-DVP-06` - Wireframe affects only raster fill; buffer/lobe/instance modes select their focused debug resolve, while World Tangent additionally requires its named raster and ray-traced GBuffer producers without replacing the lighting normal;
- `AC-DVP-07` - feature mechanism remains enclosed; outside-feature edits are only accepted integration hooks, build/generated membership, or documentation/evidence;
- `AC-DVP-08` - scene-referred HDR modes receive exposure and the tone curve exactly once, with no producer-local display curve;
- `AC-DVP-09` - display-linear exact modes bypass exposure and the tone curve while retaining one output encoding;
- `AC-DVP-10` - exposure history remains based on the Lit scene and returning to Lit does not introduce an adaptation reset caused only by the diagnostic mode;
- `AC-DVP-11` - render/output extent mismatch follows the declared sampling rule with no out-of-bounds read;
- `AC-DVP-12` - unavailable products are not presented as a valid mode result;
- `AC-DVP-13` - any future independent per-view control has an orthogonal meaning, current consumer, deterministic disabled behavior, and no overlap with `RenderViewMode`;
- `AC-DVP-14` - advertised D3D12/Vulkan and output-encoding rows meet their declared tolerances;
- `AC-DVP-15` - exact commands, configurations, observations, and artifacts are retained, and unrun checks are reported as unrun.
- `AC-DVP-16` - GBuffer, lighting, and GPU-scene visualization each privately own one explicit activation predicate and early return, focused pass parameter surface, and focused shader; scene-level orchestration contains no family activation branch, every debug mode belongs to exactly one family, and no ordinal comparison or catch-all visualization shader determines ownership.
- `AC-DVP-17` - every implemented Show leaf edits one feature-named CVar with a real non-zero lobe or controlled shadow consumer under [Lighting Show Menu And Feature Execution Controls](Controls/ShowFlags.md); no Renderer flag enum/set or Indirect Subsurface placeholder exists.
- `AC-DVP-18` - Editor and console use one sequenced CVar mutation/query route; admission, graph identity, shader parameters, and invalidation for a frame observe the same accepted values. Feature-local IsEnabled reads intent; IsActive adds path support, active consumers, and real prerequisites. No Show/CVar value is transported through viewport request/View or Application/RHI.
- `AC-DVP-19` - Direct Lighting, Indirect Lighting, and Shadows parents derive checked/mixed/unchecked state from implemented child CVars. Parent/reset actions apply as one ordered batch before frame admission, with no stored parent gate or intermediate mixed-policy frame.
- `AC-DVP-20` - all-on results match the pre-change pre-presentation reference within predeclared decoded-format tolerance; any exact-equality claim must also retain unchanged estimator/sample ordering. All-on execution retains every required producer.
- `AC-DVP-21` - disabling one lobe removes its exclusive evaluation/publication and its contribution while preserving valid active-lobe transport within the declared oracle. Group-off omits the exclusive family chain and preserves separately owned emissive, sky, and presentation.
- `AC-DVP-22` - menu and console mutate/query the same process-global feature intent; edits affect all applicable viewports. Console changes refresh menu checks; no per-viewport mirror or AND gate survives. Requested versus applied state is truthful if control delivery is delayed.
- `AC-DVP-23` - mode switches retain CVar intent without rewriting it; inapplicable modes expose their limitation. CVar actions do not use viewport-request generation as transport, and bulk actions do not produce partial-frame intent.
- `AC-DVP-24` - disabled lobe/shadow diagnostics explicitly report unavailability rather than keeping producers alive, silently reenabling them, displaying stale output, or presenting fabricated zero as a valid raw measurement. Disabled products have no invalid downstream reads.
- `AC-DVP-25` - CVars and IsEnabled/IsActive helpers remain feature-owned; optional Add...Passes entry points own admission before exclusive allocation. Frame/host orchestration has no per-leaf activation chain, generic feature manager, Show state, settings bag, parent CVar, or feature leakage into Application/RHI.
- `AC-DVP-26` - Indirect Subsurface requires the owning transport decision, real producer, activation/disabled-work route, reconstruction/history contract, non-zero independent oracle, active composite binding, feature CVar, and Editor leaf in one coherent change.
- `AC-DVP-27` - Direct Shadows off bypasses only primary direct visibility and omits exclusive signal work; Indirect Shadows off bypasses only secondary-hit direct-light visibility traces, preserves continuation intersections and Reference policy, and resets every affected temporal state. Shadow intent remains retained when its lighting family is inactive.
- `AC-DVP-28` - each supported leaf/path has execution evidence for omitted exclusive passes, shader evaluations/writes, or visibility traces, plus declared remaining shared and initialization cost. Cached topology changes retire safely; re-enable rejects incompatible histories. Saved-GPU-time claims additionally require repeatable candidate-bound timing/capture evidence, not pixel suppression alone.

## Failure Modes

| ID | Challenge | Required result | Check |
| --- | --- | --- | --- |
| `FM-DVP-01` | Select modes rapidly in two viewports. | Each viewport follows its own request generation; no process-global cross-talk or stale mode. | `CHK-DVP-02` |
| `FM-DVP-02` | Toggle Lit/Reference/Lit. | Graph topology retires safely and never schedules both middles. | `CHK-DVP-03` |
| `FM-DVP-03` | Vary exposure/tone mapping across HDR and exact modes. | HDR responds once; exact decoded values remain invariant apart from output encoding. | `CHK-DVP-04` |
| `FM-DVP-04` | Remove a required debug product or select an unavailable mode. | The route is explicitly unavailable; it never reuses unrelated/stale output as success. | `CHK-DVP-05` |
| `FM-DVP-05` | Exercise advertised backends, extents, and encodings. | Results remain within predeclared tolerance and native diagnostics have no uncategorized issue. | `CHK-DVP-06` |
| `FM-DVP-06` | Insert or reorder a view-mode enumerator and inspect each visualization family. | No family activates until it explicitly names the mode, and no family binds resources owned only by another family. | `CHK-DVP-07` |
| `FM-DVP-07` | Console/UI mutation or query races Renderer, or parent edits straddle frame admission. | Block until one sequenced route and accepted batch is proved; no request/View/settings workaround. | `CHK-DVP-08`, `CHK-DVP-09` |
| `FM-DVP-08` | Click checked/mixed/unchecked parents and reset while multiple viewports render. | All implemented child CVars change in one ordered batch, globally for applicable paths; no parent authority or partial-frame state. | `CHK-DVP-09` |
| `FM-DVP-09` | Alternate console and menu edits, then change rendering modes. | Both reflect the same retained feature intent; active/inapplicable/unavailable states are distinguished without mode-driven CVar mutation. | `CHK-DVP-09`, `CHK-DVP-10` |
| `FM-DVP-10` | Disable a lobe and select its raw diagnostic or re-enable after many frames. | Diagnostic is unavailable while disabled; no hidden producer or stale read. Re-enable resets incompatible history and publishes fresh products. | `CHK-DVP-10`, `CHK-DVP-12` |
| `FM-DVP-11` | Attempt to add Indirect Subsurface before its transport/product gate passes. | No CVar, UI row, resource, fabricated zero, or completion claim is admitted. | `CHK-DVP-08` |
| `FM-DVP-12` | Toggle Shadows with temporal reuse active; inspect trace categories and Reference. | Exclusive visibility work is absent, affected histories reset, continuation intersections and Reference policy preserved. | `CHK-DVP-10`, `CHK-DVP-11`, `CHK-DVP-12` |
| `FM-DVP-13` | Disable all direct/indirect lobes or remove an enabled feature's prerequisite. | All-off removes exclusive chains without missing/stale reads; enabled missing work reports a real failure, never zero-as-success or provider substitution. | `CHK-DVP-10`, `CHK-DVP-11` |
| `FM-DVP-14` | Toggle a leaf in a cached graph or prune shared estimator lobes. | Scheduled work matches accepted policy; old graphs retire safely and active estimator PDFs/targets/guides remain valid. | `CHK-DVP-08`, `CHK-DVP-10`, `CHK-DVP-12` |

## Checks

| ID | Oracle | Coverage |
| --- | --- | --- |
| `CHK-DVP-01` | Parse C++/HLSL values; trace request/View/consumer uses; search for all rejected parallel authorities and RHI leakage; retain the integration-hook ledger. | `AC-DVP-01`, `03`, `06`, `07`, `13` |
| `CHK-DVP-02` | Exercise two independent viewport requests and rapid mode changes. | `AC-DVP-02`, `04`; `FM-DVP-01` |
| `CHK-DVP-03` | Trace and execute Lit/Reference/Lit across Scene and Game View kinds; inspect mutually exclusive pass/resource sets. | `AC-DVP-05`; `FM-DVP-02` |
| `CHK-DVP-04` | Compare fixed numeric inputs for every HDR/exact mode across exposure, tone mapper, and output encoding combinations. | `AC-DVP-08`-`11`; `FM-DVP-03` |
| `CHK-DVP-05` | Inject unavailable products/capabilities and inspect the published result. | `AC-DVP-12`; `FM-DVP-04` |
| `CHK-DVP-06` | Run selected shader cook and focused D3D12/Vulkan viewport workloads, retaining native validation and decoded pixel comparisons. | `AC-DVP-14`, `15`; `FM-DVP-05` |
| `CHK-DVP-07` | Parse the family predicates, pass parameters, shader registrations, and shader inputs; prove every debug enumerator has exactly one family owner and no ordinal activation or retired catch-all shader remains. | `AC-DVP-06`, `16`; `FM-DVP-06` |
| `CHK-DVP-08` | Trace exact-candidate Editor/console mutation and safe query through serial/threaded control admission, feature-local CVar/IsEnabled/IsActive, graph identity/retirement, shader preparation, and history invalidation. Enumerate holders/hooks; reject Renderer Show types/request/View fields, r.ShowFlags, parent gates, private-header coupling, settings mirrors, frame/host admission sprawl, and masking-only paths. Prove enabled-missing versus disabled/inapplicable behavior with a temporary local negative probe; remove the probe. | `AC-DVP-13`, `AC-DVP-17`, `AC-DVP-18`, `AC-DVP-25`, `AC-DVP-26`, `AC-DVP-28`; `FM-DVP-07`, `FM-DVP-11`, `FM-DVP-14` |
| `CHK-DVP-09` | Exercise every leaf, derived parent, and reset through menu and console in serial/threaded mode with multiple viewports. Record requested/applied CVar values, batch/frame ordering, and mode transitions. Require one global authority, no partial bulk-edit frame, no local selection mirror, and honest inapplicability/delayed application. | `AC-DVP-19`, `AC-DVP-22`, `AC-DVP-23`; `FM-DVP-07`, `FM-DVP-08`, `FM-DVP-09` |
| `CHK-DVP-10` | Use independently non-zero lobe fixtures and primary/secondary occluders for all-on, each-off, each-group-off, all-off, disable/re-enable, diagnostic, and missing-prerequisite cases. Freeze all-on tolerance and active-lobe oracle before execution changes; retain sampling/PDF/target review for shared estimators. Require no absent/stale reads, correct intentional disabled-output semantics where justified, valid guides, fully visible named shadow evaluation, affected-history resets, continuation identity and unchanged Reference policy. Negative controls must detect omitted admission/bypass/reset and zero-as-success failures. | `AC-DVP-20`, `AC-DVP-21`, `AC-DVP-24`, `AC-DVP-26`, `AC-DVP-27`, `AC-DVP-28`; `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-12`, `FM-DVP-13`, `FM-DVP-14` |
| `CHK-DVP-11` | Cook affected direct/indirect/shadow/composite shader variants and run the focused CHK-DVP-10 matrix on each advertised D3D12/Vulkan path, including selected reconstruction providers and all-off guide contracts. Retain candidate/configuration, hardware/driver, commands, decoded products, topology/retirement and invalidation observations, native validation, and cleanup. Never infer an unrun backend/provider row. | `AC-DVP-14`, `AC-DVP-15`, `AC-DVP-20`, `AC-DVP-21`, `AC-DVP-24`, `AC-DVP-27`, `AC-DVP-28`; `FM-DVP-10`, `FM-DVP-12`, `FM-DVP-13` |
| `CHK-DVP-12` | Capture the same scene/path/backend/resolution/provider settings for all-on, each-off and group-off, after declared warm-up and outside graph-rebuild transients. Inspect pass/dispatch lists, selected shader branch/variant, exclusive lobe writes, and visibility versus continuation traces. Disclose shared/initialization work and reset/rebuild costs. A negative control that computes the disabled effect and masks its result must fail. Repeat GPU timings with declared measurement method/sample count/variation before claiming saved time; record execution omission independently of timing magnitude. | `AC-DVP-21`, `AC-DVP-27`, `AC-DVP-28`; `FM-DVP-10`, `FM-DVP-12`, `FM-DVP-14` |

Manual, build, shader-cook, runtime, GPU, and paired-backend checks may be deferred, but they are never recorded as passed merely because the source shape is coherent.
