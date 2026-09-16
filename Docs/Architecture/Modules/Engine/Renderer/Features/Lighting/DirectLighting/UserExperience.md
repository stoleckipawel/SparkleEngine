# Direct Lighting User Experience Contract

**Status:** proposed experience; implementation and acceptance remain blocked by `DIR-D0`

**Responsibility:** define how artists, rendering engineers, automation, and support select, understand, diagnose, capture, and recover the Direct Lighting feature

**Authority boundary:** the [dossier](README.md) owns feature acceptance; [Sampling And Shading](SamplingAndShading.md) owns result meaning; [Execution Architecture](ExecutionArchitecture.md) owns runtime state and data truth; [Discovery](Discovery.md) ratifies public choices; [Plan](Plan.md) owns delivery order

## Experience Principles

1. The normal path is scene-first: an artist authors physically meaningful lights and sees the Lit view respond without knowing reservoir terminology.
2. Requested mode, resolved algorithm, active visibility provider, reconstruction provider, quality profile, and any degradation are different facts and remain visible.
3. Quality profiles express intended quality/cost behavior. Candidate counts, reuse radii, history caps, ray offsets, and bias modes remain expert diagnostics unless `DIR-D0` explicitly promotes one.
4. `Automatic` may choose only among accepted profiles. A strict selection rejects unsupported capability before graph scheduling; it never clears or copies a plausible image and calls that success.
5. Raw scene-linear products remain capturable even when reconstructed output is displayed. Presentation transforms and denoising never become correctness evidence.

## Personas And Jobs

| Persona | Primary job | Required route |
| --- | --- | --- |
| environment/lighting artist | place many lights, judge balance, and find lights that are missing, noisy, or shadow-wrong | Scene light inspector plus Lit viewport status |
| technical artist | choose a supported quality profile and diagnose overlap, alpha, disocclusion, or reconstruction limits | Rendering settings plus focused lighting debug views |
| rendering engineer | reproduce estimator, identity, visibility, and backend defects from raw products | capture manifest, stable IDs, reservoir decode, counters, and native validation |
| automated test/release | apply the same semantic selection and retain candidate-bound results without UI-only state | structured settings, status, capture request, and terminal result |
| support/triage | distinguish unsupported hardware from quality exhaustion or invariant failure | portable support record with exact requested/active state and reason codes |

## Current Experience Truth

The current source route schedules direct ReSTIR lighting in Lit rendering and exposes the generic ray-reconstruction selector. It does not expose an accepted Direct Lighting algorithm/profile selector, overlap-quality state, reservoir debug workflow, or durable direct-lighting result record. Therefore every journey below is a target contract, not a claim that the current editor provides it.

## First-Use Journey

1. Open a supported Lit viewport or game camera and select a scene light.
2. Author light type, photometric value, color, range/cone/shape, enabled state, and shadow intent through the existing scene transaction route.
3. Open the Direct Lighting section of Rendering settings. The default is the discovery-ratified recommended profile, not a bag of algorithm constants.
4. Read one compact state line: `Requested -> Active`, visibility provider, reconstruction provider, render scale, and status.
5. If active, the viewport updates and the status identifies any bounded quality pressure. If unavailable, the prior valid image remains and the reason plus next action is shown.
6. For a defect, enable one focused debug product or create a capture. The resulting manifest is the authority; the preview is diagnostic only.

The clean first-use transcript must work with defaults, one directional light, one point light, one spot light, one rectangular light, and then a dense-overlap scene. It must also prove save/reload and Editor/Game camera agreement.

## Selection And State Model

`DIR-D0-12` must ratify the labels and availability matrix, but the state machine is fixed:

```text
Unavailable --supported request--> Resolving --valid--> Active
     ^                                  |                    |
     |                                  +--failure---------->|
     +--unsupported/invalid-------------+              Degraded
                                                             |
                                     reset/reconfigure ------+
```

| State | User-visible meaning | Required behavior |
| --- | --- | --- |
| `Unavailable` | requested mode/profile cannot run on this candidate | no partial dispatch/publication; identify capability, provider, build, or content blocker |
| `Resolving` | settings/provider/profile change is being validated | preserve the last complete image and never imply the new request is active |
| `Active` | declared algorithm/provider/profile produced the current completed frame | show requested and active identity plus frame/capture generation |
| `Degraded` | active bounded-work estimator exceeded a quality/capacity assumption or optional provider fell back by accepted policy | keep the actual active route visible and explain likely artifact and remedy |
| `Failed` | invariant, device, shader, publication, or provider failure invalidated the frame | publish no partial history; retain prior valid result where possible and provide recovery action |

No `Success` state exists independently of a completed active frame. A quiet log, responsive viewport, denoised image, or fallback output cannot manufacture success.

## Settings Contract

| Surface | Recommended behavior | Expert disclosure |
| --- | --- | --- |
| algorithm mode | `Automatic` plus only independently accepted explicit modes | resolved estimator revision and bias mode |
| quality profile | named profiles whose budgets are frozen in `DIR-D0-11` | initial candidates, temporal/spatial candidates, shadow rays, history cap, memory estimate |
| visibility | engine-wide automatic frontend resolution | read-only Inline/Pipeline active state, alpha mode, failure reason |
| reconstruction | portable baseline by default once accepted; optional DLSS RR where supported | active provider, input extent, guide generations, reset/bypass reason |
| diagnostics | `Off` by default; one product at a time | raw direct lobes, selected light ID/type, target/weight/`M`, visibility, compatibility, confidence, rejection reason |

Changing a correctness-defining expert control invalidates the corresponding history and marks prior evidence non-comparable. Settings serialization stores semantic choices, never transient resource handles or vendor-specific implementation structures.

## Diagnostic Views And Captures

| Product | Question answered | Mandatory legend/metadata |
| --- | --- | --- |
| raw diffuse/specular/subsurface | is the estimator producing the expected lobe energy before reconstruction? | scene-linear range, exposure state, sample budget, frame ID |
| selected light | which logical light reached each receiver? | stable logical ID, type, current index/generation, invalid marker |
| reservoir health | are target, weight sum, `M`, validity, and rejection bounded? | scale, clamps, bias mode, invalid/zero/non-finite counters |
| visibility | is failure sampling or shadow execution? | segment class, provider, hit/miss/alpha/backface/self-hit reason |
| reuse compatibility | why was history accepted or rejected? | motion, depth, normal, material/object/light generation, disocclusion bits |
| confidence/reconstruction | is filtering hiding or extending stale evidence? | raw confidence, responsive/reset mask, provider/history generation |
| overlap pressure | is the fixed budget under-resolving important lights? | affecting/eligible lights, candidates/rays, selected diversity, saturation status |

A capture contains candidate revision, dirty-boundary statement, build/shader/backend/device/driver identity, scene and settings hashes, camera/view/generation, random-sequence identity, raw products, counters, and the exact capture window. It never stores only a tone-mapped screenshot.

## Failure, Recovery, And Support

Every failure reports terminal category, affected view/candidate, requested and active configuration, first failing invariant/provider/capability, safe state, retained/invalidated histories, cleanup result, and one actionable next step. Recovery is explicit: correct content/settings, restore provider/capability, retry a new generation, or choose an accepted alternate profile. Automatic retry is bounded and cannot loop across frames while presenting stale output as current.

The support record is copyable and machine-readable. It excludes asset payloads and user filesystem details by default, but includes hashes and logical identifiers sufficient to reproduce the route. Redaction must not remove the fact needed to classify a failure.

## Automation And Accessibility

Automation can set every public selection, query requested/active/degraded/failed state, wait for a terminal frame/capture generation, request the same raw products, and receive the same reason codes as the UI. There is no UI-only quality mode or console-only success path.

Status cannot rely on color alone. Text labels, stable ordering, keyboard navigation, searchable setting names, scalable legends, numeric ranges, and high-contrast invalid markers are required. Rapid debug flicker and unbounded flashing are prohibited; temporal diagnostic sequences support pause and frame stepping.

## Experience Acceptance

- `UX-DIR-01` — a first-time artist reaches a correctly identified active profile from a default Lit viewport, edits all four admitted analytic lights, saves/reloads, and never handles reservoir vocabulary.
- `UX-DIR-02` — strict unsupported provider, invalid light, overlap exhaustion, shader/provider failure, and reconstruction-guide failure each produce distinct safe states and actionable reason codes.
- `UX-DIR-03` — engineer and automation produce equivalent raw captures with the same generation, settings, counters, and manifest identity.
- `UX-DIR-04` — requested, active, degraded, failed, stale, raw, reconstructed, and presented results cannot be confused in UI, logs, captures, or reports.
- `UX-DIR-05` — keyboard, text, contrast, scaling, pause/step, and non-color status checks pass on the supported editor route.

These criteria refine `AC-DIR-07`, `AC-DIR-09`, `AC-DIR-11`, and `AC-DIR-12`; `FCR-REN-06` owns their candidate results.
