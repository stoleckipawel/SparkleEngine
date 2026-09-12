# Indirect Lighting And ReSTIR GI User Experience Contract

**Status:** proposed experience; implementation and acceptance remain blocked by `IND-D0`

**Responsibility:** define how artists, rendering engineers, automation, and support select, understand, diagnose, capture, and recover real-time indirect lighting

**Authority boundary:** the [dossier](README.md) owns acceptance; [Transport And Estimator](TransportAndEstimator.md) owns path/result meaning; [Execution Architecture](ExecutionArchitecture.md) owns runtime truth; [Discovery](Discovery.md) ratifies public choices; [Plan](Plan.md) owns delivery order

## Experience Principles

1. Artists select a product promise—stable diffuse and rough-specular bounce lighting—not a paper acronym or a list of bounce/candidate constants.
2. The active path domain is always disclosed: maximum depth, supported roughness/lobes, environment/emission behavior, exclusions, traversal, reconstruction, and any degradation.
3. Raw indirect diffuse/specular, path/shift/reservoir state, and reconstructed output remain distinct. Denoised stability is never reported as estimator correctness.
4. Unsupported delta, transmission, caustic, animation, provider, or backend cells reject or display explicit exclusions; they never silently become diffuse approximations.
5. Quality settings and captures are equivalent in Editor, game, automation, and candidate evidence.

## Personas And Jobs

| Persona | Job | Required route |
| --- | --- | --- |
| environment/material artist | judge bounce fill, color bleeding, emissive/environment response, and glossy reflection support | Lit viewport plus compact indirect status |
| technical artist | select a stable profile and identify disocclusion, roughness, path-depth, or reconstruction limitations | Rendering settings and focused debug views |
| rendering engineer | falsify technique accounting, shift mapping, GRIS weights, correlation, identity, and backend behavior | raw capture, decoded path record, rejection counters, reference manifest |
| automation/release | configure one exact path domain and retain terminal candidate artifacts | structured settings/status/capture API equivalent to UI |
| support | distinguish unsupported transport from quality exhaustion, stale history, or provider failure | portable reason-coded support record |

## Current Experience Truth

The existing Lit route always produces indirect diffuse/specular through a seed-replay prototype and exposes low-level bounce/distance CVars plus a generic optional DLSS Ray Reconstruction selector. No accepted ReSTIR GI mode, path-domain disclosure, portable reconstruction baseline, raw path-capture workflow, or conformance result is currently public. The journeys below are gated target behavior.

## First-Use Journey

1. Open a supported Lit view with the recommended quality profile.
2. The Indirect Lighting row reports `Requested -> Active`, active path domain, traversal, reconstruction, render scale, history state, and exclusions.
3. Edit a light, emissive surface, material roughness, or sky. The viewport identifies whether a new current generation is active or history is restarting.
4. Switch among only accepted profiles. The row explains quality/cost intent and any reduced path/lobe domain; it does not expose raw reservoir constants as the primary control.
5. If a result looks wrong, compare `Raw diffuse`, `Raw specular`, `Reconstructed`, and `Reference difference` where an accepted reference artifact exists.
6. Capture a frame or sequence. The manifest binds path domain, technique accounting, seeds, history, backend, content, and reference identity.

The clean transcript covers a diffuse box, rough-glossy receiver, environment rotation, emissive toggle, camera motion/disocclusion, save/reload, and an explicitly unsupported path.

## State And Domain Disclosure

| State | Required disclosure | Safe behavior |
| --- | --- | --- |
| `Unavailable` | missing backend/traversal/reconstruction/build/content capability | reject before partial graph/history publication |
| `Resolving` | requested profile and generation being validated | retain last complete result but label it stale/not current |
| `Active` | exact estimator revision, path/lobe domain, depth, traversal, reconstruction, and generation | only completed current outputs carry active status |
| `Degraded` | sample/path budget pressure, excluded lobe reached, reconstruction bypass, or quality fallback accepted by policy | display actual route and visible limitation |
| `Failed` | non-finite transport, invalid shift/identity, provider/device/shader/publication failure | invalidate affected sample/history, preserve prior valid output separately, offer explicit recovery |

The UI never labels the current seed replay as ReSTIR GI until `IND-D0` and conformance checks pass. `Automatic` chooses only an accepted domain; strict modes do not substitute another GI architecture.

## Settings Contract

| Surface | Primary control | Expert/detail disclosure |
| --- | --- | --- |
| mode/profile | discovery-ratified recommended profile plus accepted strict profiles | estimator revision, proposal set, shift family, GRIS/bias mode |
| transport domain | product labels such as diffuse/rough-specular and declared depth | exact roughness threshold, terminal techniques, roulette and excluded delta/transmission/caustic cells |
| reconstruction | accepted portable baseline; optional DLSS RR when supported | provider, render/output extent, guide/history generations, bypass/reset reason |
| quality | named equal-purpose quality/cost profiles | candidates, reused candidates, rays/path, history cap, record/history memory |
| diagnostics | off by default, one focused product or comparison at a time | raw lobes, selected path, mapping/rejection, weights/`M`, correlation, confidence, reference error |

Changing domain, estimator, proposal, shift, traversal, reconstruction, render extent, or relevant shader/content generation invalidates affected history and marks captures incomparable. Public persistence stores semantic intent, not packed path records or SDK structures.

## Diagnostic Views And Captures

| Product | Question | Required metadata |
| --- | --- | --- |
| raw diffuse/specular | is transport/lobe accounting correct before reconstruction? | pre-exposure, depth domain, sample/ray budget, generation |
| path class/terminal technique | what contribution was selected? | vertex count, primary lobe, NEE/emission/environment/termination class |
| shift/rejection | why did a source candidate map or fail? | source/destination receiver, mapping family, support/Jacobian/visibility reason |
| reservoir health | are generalized weight, target, sum, `M`, and validity bounded? | formula revision, clamps, duplication/correlation mode, counters |
| motion/history | is reuse compatible? | receiver/path motion, object/material/light/sky/provider generations, disocclusion |
| reconstruction | what is filtering doing? | confidence, hit distance, guides, history length/reset, raw/reconstructed toggle |
| reference difference | how does the raw estimator compare at a frozen budget/window? | reference independence, spp/window, exposure, metric, validity mask |

The capture manifest contains candidate/dirty/build/shader/backend/device/driver identity, scene/camera/settings hashes, path domain, estimator/formula revision, random-sequence identity, current/previous generations, raw products, decoded bounded samples, rejection/finite counters, reference identity, warmup and capture windows. A screenshot alone is not a capture.

## Progress, Failure, And Recovery

Interactive rendering does not expose a fabricated completion percentage. It reports current-frame generation, history age, active sample/path budget, and whether reference capture is accumulating. Reference progress states completed samples/tiles and authoritative output only when publication finishes.

Every failure reports category, view/candidate, requested/active domain, first failing equation/invariant/capability/provider, rejected/retained state, cleanup, and next action. Recovery creates a new generation after correcting content/settings or selecting an accepted provider/profile. Cancellation or device loss publishes no partial current history and cannot overwrite the last successful artifact.

## Automation, Accessibility, And Support

Automation can apply every supported semantic selection, query requested/active/domain/exclusion/failure state, wait on a frame or capture generation, request identical raw products, and receive the same reason codes. Console variables may serve engineering experiments but cannot be the only public or evidence route.

Status and debug products use text plus shape/pattern, not color alone. Controls are searchable, keyboard reachable, scalable, and ordered consistently. Temporal diagnostics can pause and frame-step. Numeric legends state units/ranges and provide high-contrast invalid/unsupported markers.

The support record is machine-readable and privacy-minimal: it includes hashes, logical IDs, profiles, generations, counters, and capability/provider identity, while excluding user paths and asset payloads unless explicitly attached.

## Experience Acceptance

- `UX-IND-01` — a first-time artist reaches the recommended active profile, edits light/material/environment inputs, and understands the supported bounce/lobe domain without reservoir knowledge.
- `UX-IND-02` — excluded path, invalid shift, stale history, unsupported provider/backend, reconstruction bypass, and non-finite transport produce distinct safe states and actions.
- `UX-IND-03` — raw/reconstructed/reference views cannot be confused, and the current seed-replay route cannot appear as accepted ReSTIR GI.
- `UX-IND-04` — UI and automation create semantically identical settings and capture manifests for one candidate/view/generation.
- `UX-IND-05` — keyboard, text, contrast, scaling, pause/step, and non-color checks pass on the supported editor route.

These criteria refine `AC-IND-08`, `AC-IND-10`, `AC-IND-11`, and `AC-IND-13`; `FCR-REN-07` owns observed results.
