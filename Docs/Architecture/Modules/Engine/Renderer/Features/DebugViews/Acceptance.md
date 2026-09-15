# Debug View Presentation - Acceptance

**Status:** feature-local acceptance contract; not a candidate result

**Architecture:** [Render View Modes](ViewModes.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Delivery:** [Plan](Plan.md)

Candidate results belong in `FCR-REN-11`. Source inspection does not prove build, runtime, pixel, backend, or release behavior.

## Completion Criteria

- `AC-DVP-01` - every non-sentinel `RenderViewMode` has one contract row and a real production consumer; numeric C++ and HLSL values match exactly, `ReferencePathTracer` is `1`, and values are contiguous through `Count = 17`;
- `AC-DVP-02` - the selected mode has one representation on `ViewportRenderRequest` and immutable `RenderView`; two viewport requests may hold different modes without global cross-talk;
- `AC-DVP-03` - no Editor mirror enum, preset translator, visualization target, mode-shaped show flag, selector CVar, command bridge, settings copy, compatibility alias, or RHI field competes with `RenderViewMode`;
- `AC-DVP-04` - Editor owns labels, icons, grouping, shortcuts, and interaction while Game/runtime may submit the same rendering semantic without an Editor dependency;
- `AC-DVP-05` - the Reference mode schedules only the private Reference middle and Lit schedules only GBuffer/ReSTIR/reconstruction inside the same frame shell;
- `AC-DVP-06` - Wireframe affects only raster fill and buffer/lobe/instance modes affect only the debug resolve or named instance-color consumer;
- `AC-DVP-07` - feature mechanism remains enclosed; outside-feature edits are only accepted integration hooks, build/generated membership, or documentation/evidence;
- `AC-DVP-08` - scene-referred HDR modes receive exposure and the tone curve exactly once, with no producer-local display curve;
- `AC-DVP-09` - display-linear exact modes bypass exposure and the tone curve while retaining one output encoding;
- `AC-DVP-10` - exposure history remains based on the Lit scene and returning to Lit does not introduce an adaptation reset caused only by the diagnostic mode;
- `AC-DVP-11` - render/output extent mismatch follows the declared sampling rule with no out-of-bounds read;
- `AC-DVP-12` - unavailable products are not presented as a valid mode result;
- `AC-DVP-13` - any future independent per-view control has an orthogonal meaning, current consumer, deterministic disabled behavior, and no overlap with `RenderViewMode`;
- `AC-DVP-14` - advertised D3D12/Vulkan and output-encoding rows meet their declared tolerances;
- `AC-DVP-15` - exact commands, configurations, observations, and artifacts are retained, and unrun checks are reported as unrun.

## Failure Modes

| ID | Challenge | Required result | Check |
| --- | --- | --- | --- |
| `FM-DVP-01` | Select modes rapidly in two viewports. | Each viewport follows its own request generation; no process-global cross-talk or stale mode. | `CHK-DVP-02` |
| `FM-DVP-02` | Toggle Lit/Reference/Lit. | Graph topology retires safely and never schedules both middles. | `CHK-DVP-03` |
| `FM-DVP-03` | Vary exposure/tone mapping across HDR and exact modes. | HDR responds once; exact decoded values remain invariant apart from output encoding. | `CHK-DVP-04` |
| `FM-DVP-04` | Remove a required debug product or select an unavailable mode. | The route is explicitly unavailable; it never reuses unrelated/stale output as success. | `CHK-DVP-05` |
| `FM-DVP-05` | Exercise advertised backends, extents, and encodings. | Results remain within predeclared tolerance and native diagnostics have no uncategorized issue. | `CHK-DVP-06` |

## Checks

| ID | Oracle | Coverage |
| --- | --- | --- |
| `CHK-DVP-01` | Parse C++/HLSL values; trace request/View/consumer uses; search for all rejected parallel authorities and RHI leakage; retain the integration-hook ledger. | `AC-DVP-01`, `03`, `06`, `07`, `13` |
| `CHK-DVP-02` | Exercise two independent viewport requests and rapid mode changes. | `AC-DVP-02`, `04`; `FM-DVP-01` |
| `CHK-DVP-03` | Trace and execute Lit/Reference/Lit across Scene and Game View kinds; inspect mutually exclusive pass/resource sets. | `AC-DVP-05`; `FM-DVP-02` |
| `CHK-DVP-04` | Compare fixed numeric inputs for every HDR/exact mode across exposure, tone mapper, and output encoding combinations. | `AC-DVP-08`-`11`; `FM-DVP-03` |
| `CHK-DVP-05` | Inject unavailable products/capabilities and inspect the published result. | `AC-DVP-12`; `FM-DVP-04` |
| `CHK-DVP-06` | Run selected shader cook and focused D3D12/Vulkan viewport workloads, retaining native validation and decoded pixel comparisons. | `AC-DVP-14`, `15`; `FM-DVP-05` |

Manual, build, shader-cook, runtime, GPU, and paired-backend checks may be deferred, but they are never recorded as passed merely because the source shape is coherent.
