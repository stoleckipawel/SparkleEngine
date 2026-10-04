# Debug View Presentation - Acceptance

**Status:** feature-local acceptance contract; not a candidate result

**Responsibility:** own binary completion criteria, controlled failures, checks, oracles, and evidence boundaries for Debug Views.

**Current readiness:** **Not applicable** to this contract; current progress remains owned by the [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Verified baseline:** 2026-10-04 at revision `26803f97` with an inspected dirty working tree; candidate evidence must name its own exact revision and state.

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
- `AC-DVP-17` - every implemented lighting show flag has one fixed enum/set bit, one Editor leaf, one feature-named global CVar gate, and one real narrow consumer according to [Renderer Show Flags](Controls/ShowFlags.md); contribution leaves have non-zero products, shadow leaves have controlled visibility consumers, and `IndirectSubsurface` remains absent until its owning transport/product gate passes;
- `AC-DVP-18` - the viewport-local set travels only through `ViewportRenderRequest` and immutable `RenderView`, while global CVar gates reach Renderer through a proved sequenced owner-thread boundary and are read only at the corresponding lighting-composite or shadow-evaluation parameter-preparation owner; no CVar name begins with `r.ShowFlags`;
- `AC-DVP-19` - Direct Lighting, Indirect Lighting, and Shadows parent rows are derived checked/mixed/unchecked bulk actions over available children, never stored flags or CVars, and one parent action publishes one request-generation change;
- `AC-DVP-20` - with every per-view bit and global gate enabled, the pre-presentation lighting-composite output is identical to the pre-show-flag candidate for the same inputs;
- `AC-DVP-21` - disabling one contribution leaf removes only its named non-zero product from Lit composition within the predeclared decoded-format tolerance; disabling every available contribution child retains separately owned emissive, sky, and presentation behavior;
- `AC-DVP-22` - a false global CVar gate disables its named contribution or shadow behavior in every viewport, while a true gate defers to each viewport-local bit; Editor actions never mutate the global gates and console actions never rewrite viewport selection;
- `AC-DVP-23` - two viewports retain independent leaf/parent/reset state across mode changes; each semantic edit advances only its owning request generation and advances it exactly once;
- `AC-DVP-24` - hidden Lit contributions continue to be produced for raw lighting diagnostics, reconstruction, and history continuity; the direct shadow signal also remains produced and numerically unchanged when Direct Shadows is disabled; diagnostic modes retain the selection and apply only the explicitly documented contribution-versus-shadow behavior;
- `AC-DVP-25` - the implementation retains one public semantic type, one request field, one immutable View copy, the smallest named lighting consumers, and the smallest Editor presentation hook, with no Application/RHI state, CVar copies, resolved-mask holder, registry, settings bag, compatibility alias, producer-pruning branch, or `r.ShowFlags` namespace;
- `AC-DVP-26` - Indirect Subsurface is enabled only after the Indirect Lighting owner admits the lobe and supplies its real producer, reconstruction/diagnostic contract, non-zero independent oracle, composite input, flag, CVar, and Editor leaf in one coherent change;
- `AC-DVP-27` - Direct Shadows substitutes fully visible evaluation only for primary-surface direct lighting; Indirect Shadows substitutes fully visible direct-light evaluation only at secondary hits in the Lit indirect estimator, preserves continuation intersections and Reference behavior, and resets every dependent indirect temporal state when its effective value changes.

## Failure Modes

| ID | Challenge | Required result | Check |
| --- | --- | --- | --- |
| `FM-DVP-01` | Select modes rapidly in two viewports. | Each viewport follows its own request generation; no process-global cross-talk or stale mode. | `CHK-DVP-02` |
| `FM-DVP-02` | Toggle Lit/Reference/Lit. | Graph topology retires safely and never schedules both middles. | `CHK-DVP-03` |
| `FM-DVP-03` | Vary exposure/tone mapping across HDR and exact modes. | HDR responds once; exact decoded values remain invariant apart from output encoding. | `CHK-DVP-04` |
| `FM-DVP-04` | Remove a required debug product or select an unavailable mode. | The route is explicitly unavailable; it never reuses unrelated/stale output as success. | `CHK-DVP-05` |
| `FM-DVP-05` | Exercise advertised backends, extents, and encodings. | Results remain within predeclared tolerance and native diagnostics have no uncategorized issue. | `CHK-DVP-06` |
| `FM-DVP-06` | Insert or reorder a view-mode enumerator and inspect each visualization family. | No family activates until it explicitly names the mode, and no family binds resources owned only by another family. | `CHK-DVP-07` |
| `FM-DVP-07` | A live console write can race a Renderer-thread CVar read. | DVP-4A stops until mutation uses a proved sequenced owner-thread boundary; no request/View/settings copy is accepted as a workaround. | `CHK-DVP-08` |
| `FM-DVP-08` | Click a checked or mixed parent while two viewport requests are active. | All available children change only in the owning viewport and its request generation advances once. | `CHK-DVP-09` |
| `FM-DVP-09` | Combine a false global gate with local on/off values, then restore the gate. | False disables the named contribution or shadow behavior globally; true restores each viewport's retained local choice without stale or cross-viewport mutation. | `CHK-DVP-09`, `CHK-DVP-10` |
| `FM-DVP-10` | Hide a Lit contribution and select its raw lighting diagnostic. | The raw non-zero product remains available and numerically unchanged; only Lit composition is suppressed. | `CHK-DVP-10` |
| `FM-DVP-11` | Attempt to add Indirect Subsurface before its transport/product gate passes. | No enum value, CVar, UI row, resource, fabricated zero, or completion claim is admitted. | `CHK-DVP-08` |
| `FM-DVP-12` | Toggle Shadows with temporal indirect reuse active, then inspect continuation hits and Reference output. | No stale shadowed/unshadowed indirect history survives; continuation-hit identity and Reference pixels remain invariant; only the named visibility evaluations change. | `CHK-DVP-10`, `CHK-DVP-11` |

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
| `CHK-DVP-08` | At the exact candidate revision, trace the flag definition through request/View and every focused lighting consumer, plus console mutation through its owner-thread boundary; enumerate every public type, state holder, feature CVar/read, Editor hook, Application/RHI reference, rejected name, and DVP-4 integration hook. Search for and reject `r.ShowFlags`, parent CVars, and copied CVar carriers. Deliberately add one forbidden duplicate in a temporary local probe or demonstrate an equivalent negative search fixture, then prove the check rejects it and remove the probe. | `AC-DVP-13`, `17`, `18`, `25`, `26`, `27`; `FM-DVP-07`, `FM-DVP-11` |
| `CHK-DVP-09` | In two viewport sessions, exercise every leaf plus checked/mixed/unchecked Direct Lighting, Indirect Lighting, and Shadows parents and reset actions across applicable and inapplicable modes. Record before/after sets and generations; the oracle requires one owning-viewport mutation and exactly one generation advance per semantic action. Exercise global-gate changes separately and prove viewport selections remain unchanged. | `AC-DVP-19`, `22`, `23`; `FM-DVP-08`, `FM-DVP-09` |
| `CHK-DVP-10` | Use analytic fixtures with each lobe independently non-zero plus controlled primary/secondary occluders. Compare raw products and the pre-presentation composite for all-on, each contribution off, each group off, all contributions off, each shadow leaf off, Shadows off, global-gate off/on restoration, and diagnostic selection. All-on must match the pre-change candidate exactly. Contribution isolation must match the decoded-format reference within one declared storage-format ULP per channel. Direct Shadows off must preserve the raw shadow signal while matching fully visible primary direct evaluation. Indirect Shadows off must match fully visible secondary-hit direct-light evaluation, trigger the declared temporal reset, retain continuation-hit identity, and leave Reference output invariant. Each oracle must fail when its targeted suppression/bypass/reset is intentionally omitted. | `AC-DVP-20`-`24`, `27`; `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-12` |
| `CHK-DVP-11` | Cook the focused lighting-composite, direct-lighting, and Lit indirect-lighting shader set and run the `CHK-DVP-10` matrix on each advertised D3D12/Vulkan path with native validation enabled. Retain candidate/configuration, hardware/driver, commands, decoded pixels, raw lobe/shadow artifacts, invalidation observations, diagnostics, and cleanup; do not infer the unrun backend from the other. | `AC-DVP-14`, `15`, `20`-`24`, `27`; `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-12` |

Manual, build, shader-cook, runtime, GPU, and paired-backend checks may be deferred, but they are never recorded as passed merely because the source shape is coherent.
