# Reference Path Tracer Remaining Recovery And Validation Plan

**Status:** source stages delivered; recovery, deferred GPU validation, artifact integrity and final adoption remain open

**Reconciled:** 2026-10-10 against source `34b6d8a7a34283fb4bf568be13307c741ca1bedd`. This cleanup reuses the existing source-delivery records and deferred-evidence disposition; it does not claim a fresh cook, launch, pixel, estimator or reference-authority result. The black-lighting observation remains dated 2026-09-26 until a new candidate disproves it.

**Authority boundary:** [Transport And Estimator](TransportAndEstimator.md), [Execution Architecture](ExecutionArchitecture.md) and [User Experience](UserExperience.md) retain design; [Discovery](Discovery.md) retains `PTD-00-R1`; the [dossier](README.md) owns `RPT-FS-*`, `AC-RPT-*`, `FM-RPT-*` and `CHK-RPT-*`. This page orders unfinished work. Completed Stage 0–9 implementation prompts, migration steps and execution diaries are removed; necessary source evidence remains under `Evidence`.

## Remaining Work At A Glance

**Research use for the remaining recovery/proof:** the [reference cards](../../../../../../../Strategy/Research/RenderingReferenceExamples.md) pin Falcor `NVR-04`, Quake II RTX `NVR-05`, RTXPT `NVR-15` and production sampling `NVR-17`. Consult their raw-versus-reconstructed products, material/transport domains and integration failures to form a discriminating hypothesis for the earliest wrong Sparkle output. These related renderers are not automatically independent oracles; shared estimator ancestry can conceal a shared defect. Record exact assumption, local differences, rights and falsifier in the current recovery card. Preserve accepted `PTD-00-R1` semantics and unfinished analytic/native/adoption checks; do not replay delivered stages or admit vendor caches/SDKs.

Start with the recovery route below. The ordinary per-view mode, alternate frame middle, session/accumulation, shared transport, automatic traversal, UI and manual publication route are source-delivered; do not restart their implementation. Close identified invalid-result/preflight defects and execute the deferred analytic, lifecycle, backend, UX and publication checks against one real candidate. Source delivery remains distinct from executable acceptance.

Reuse valid prior proof and re-run invalidated cells. `REL-03`, release maps, support identities, all included GPU validation and independent adoption remain mandatory before release/package/oracle claims. The [readiness dashboard](../../../../../../../Acceptance/CurrentReadiness.md#renderer) remains the score owner.



## Current Recovery And Completion Route - 2026-09-26

The source stages do not need to be repeated wholesale. The current candidate must first clear the earliest observed functional failure, then execute the retained evidence ladder in dependency order. This section orders remaining work; it does not replace the dossier's acceptance criteria or convert a manual observation into retained evidence.

**Host prerequisite observed 2026-10-03:** Windows Smart App Control blocked the unsigned `TextureCooker.exe` during `cook.all` and `ShowcaseEditor.exe` during `levels.run`. The shader generation portion completed, but texture cooking and Editor launch did not. A subsequent read of the local Smart App Control state returned `Off`; neither tool was retried after that change. Before step 2, prove the same cooker and Editor now start. If either remains blocked, follow the [Launcher trust-failure route](../../../../../../../Architecture/Modules/Tools/Launcher/README.md#when-windows-blocks-a-development-tool). This host episode is separate from the black-lighting defect; do not claim a current-candidate GPU result until the executable and cooked inputs actually run.

### Observed candidate state

| Observation | Consequence |
| --- | --- |
| Selecting Reference with a configured DLSS provider previously failed during graph construction because Reference lacked temporal guides. | The source now keeps Lit's configured provider untouched, renders Reference at output extent, and uses a color-only Linear resolve. This fix requires D3D12/Vulkan regression proof. |
| D3D12 then failed to resolve the Reference sky sampler because the backend catalog assumed one address mode for all axes. | The D3D12 library now has one explicit `LinearNoMipWrapClampClamp` slot and lowers U/V/W independently. This remains backend mechanism, not Reference policy, and requires paired regression proof. |
| After those fixes, Reference accumulated and displayed sky but scene geometry remained effectively black. Lit was also effectively black while GBuffer diffuse remained populated. | Inspect the first incorrect product in each route. Shared inputs or presentation are hypotheses, not established causes; two independent failures are also possible. The symptom blocks Stage-9 evidence-candidate and usable-viewport claims. |
| Correcting the exposure reduction dispatch did not make scene lighting visible in the observed run. | Exposure under-dispatch was a real source defect but is not accepted as the root cause of the current black result. Investigation must continue from measured product boundaries. |

### Smallest discriminating review of the black result

Keep one camera, scene generation, cooked-shader candidate, backend, and presentation configuration fixed while comparing products. Use the existing visualization modes and generic radiance capture/readback where available; do not add permanent diagnostic passes or alter reference energy to make a screenshot brighter. The [live source route](ExecutionArchitecture.md#live-source-route-and-product-boundaries) names the owners.

| Observation or next probe | What it establishes | Next owner/question; do not infer |
| --- | --- | --- |
| GBuffer Diffuse is populated but GBuffer World Normal/depth or material attributes are wrong. | Some geometry/color data arrived; the full surface contract did not. | Inspect canonical GBuffer/ray-material/scene preparation. Diffuse alone does not validate the normals used by Lit or Reference. |
| GBuffer attributes are plausible, but direct and indirect lighting visualizations are zero or non-finite. | The Lit failure precedes its final composite/display product. | Check published lights, scene bindings, shadow visibility, ReSTIR target writes and clearing, then composite inputs. It says nothing conclusive about Reference's separate integrator. |
| Lighting lobe products contain nonzero finite radiance but Lit `SceneColor` or `FinalColorLdr` is dark. | The failure lies after those lobe outputs. | Inspect `LightingComposite`, exposure input/reduction/adaptation, visualization and upscaler/presentation lineage. Do not rescale the path estimator. |
| Reference `CommittedMean`/generic `Radiance` is nonzero and finite but its viewport image is dark. | Transport produced signal; the Reference display derivative or common presentation is suspect. | Inspect `ReferencePathTracerDisplay`, exposure, Linear resolve, and final encoding. Progress alone cannot establish this; inspect raw float values. |
| Reference raw radiance is zero/non-finite while Lit has nonzero raw radiance. | The Reference route or its ray-visible prepared inputs require isolation. | Check primary hit/material/emission, environment and light bindings, PDFs/visibility, invalid-sample behavior. Never seed Reference from Lit's GBuffer to pass this probe. |
| Both raw radiance products are zero/non-finite. | A shared input defect is plausible, but not proved. | Compare scene/light/environment publication and resource bindings, then test each independent middle. Do not declare a single root cause from matching final pixels. |

For each probe, retain the product name, pixel/region values and format, frame/scene/sample identity, backend, and the smallest source/asset revision that changes the observation. A successful `FinalColorLdr` screenshot cannot replace raw radiance or a GPU-known-value fixture.

### Source-contract discrepancies to close before oracle evidence

The [mathematical audit](TransportAndEstimator.md#current-invalid-sample-and-claim-gap) identifies a concrete invalid-result gap: a NaN transport sample currently enters mean/M2, while the session can still advance its committed prefix on submission completion; manual encoding may reject the non-finite value only later. Close this as a production result/state rule, with a controlled invalid-hit and safety-ceiling falsifier. Do not add a permanent telemetry framework, silently drop the sample, clamp it, or call a corrupt prefix complete. Separately, `ReferencePathTracerSession::ResolveAvailability` currently tests perspective/extent, accumulation/frontend/material-texture capability, blend alpha, and nonzero subsurface strength; that source inspection does not prove every [frozen content exclusion](ExecutionArchitecture.md#product-and-mathematical-claim) is rejected before sample zero. Reconcile the accepted Scene/material reachability matrix against preflight and test each reachable excluded case. These are identified source-to-contract gaps, not explanations of the black image.

### Required execution order

1. **Restore correct Lit and Reference surface radiance.** Use the decision table above to locate the first incorrect product on each route, test whether an input or presentation owner is genuinely shared, and repair the proven owner(s). Do not add Reference-only intensity, fallback lighting, GBuffer transport, fabricated guides, per-scene constants, or diagnostic infrastructure to make the image look plausible.
2. **Rebuild and cook the exact candidate.** First confirm that Windows permits the built cooker and Editor to start. Then use the ordinary Launcher Quick Start, retaining the exact source revision, configuration, shader compiler/settings, shader hashes, backend, adapter/driver, scene, camera, and content generation. A successful build or shader-generation message is not a completed texture cook, and a stale cooked shader invalidates the render observation.
3. **Clear the D3D12 viewport gate.** Exercise Lit, GBuffer controls, lighting-lobe views, Reference selection, finite nontrivial scene radiance, progressive sample commits, camera-motion reset/refinement, pause/resume/restart, Lit/Reference/Lit retention, resize, close, and second-view capacity. Retain `CHK-RPT-02`, `03`, `05`, `09`, `15`, and `18` evidence rather than a success screenshot.
4. **Clear the Vulkan and frontend gate.** Repeat the same semantic inputs under Vulkan and every automatically selected complete Inline/Pipeline route with native validation. Then execute the raw paired comparison required by `CHK-RPT-12`; a successful launch alone is insufficient.
5. **Prove raw-product and artifact integrity.** Exercise exact committed-prefix capture, FLOAT EXR decode/round trip, checkpoint mean/M2/count restoration, hashes, provenance mutation, cancellation, access/disk failure, interruption, and completion-file-last publication under `CHK-RPT-09`, `10`, and `13`.
6. **Close the source-contract discrepancies and execute the estimator evidence ladder.** Prove unsupported reachable content is rejected before sample zero and invalid samples cannot advance a valid/completed prefix; then run `CHK-RPT-03` through `11` with the frozen analytic/metamorphic scenes, hand cases, injected PDF/MIS/emission/roulette/sample/endpoint/accumulation defects, independent replicates, and semantically matched external renderers. Do not tune thresholds or scope after observing output.
7. **Finish product and release proof.** Complete Editor and Game/runtime first use, accessibility, response/resource budgets, unsupported routes, Shipping exclusion, clean package operation, release maps, and every remaining `CHK-RPT-14` through `20` cell. Only then may Stage 10 submit `FCR-REN-08`.

### Recovery gate

Stage 9 remains source-delivered, but it cannot produce an evidence candidate until steps 1-6 pass for one immutable candidate. Stage 10 remains unauthorized until that evidence candidate also satisfies the exact release inputs named by Stage 10. Manual work may be deferred without erasing source progress, but an observed black frame is an active failed product criterion rather than merely an unrun check.

## Universal Execution Contract

Every implementation prompt below inherits these rules. The executing agent must:

1. start at the repository root; read `AGENTS.md`, `Docs/README.md`, the selected Engineering task routes, [Transport And Estimator](TransportAndEstimator.md), [Execution Architecture](ExecutionArchitecture.md), [User Experience](UserExperience.md), [feature acceptance](README.md), [Discovery](Discovery.md), and this plan in full;
2. inspect `git status --short`, preserve unrelated dirty work, and inspect live owners/producers/consumers/lifetime/build membership with `rg` before editing;
3. confirm all named design/scope prerequisites and prior-stage source handoffs. A deferred owner-operated build, shader cook, GPU run, or interactive workflow follows [Development Continuation And Deferred Validation](#remaining-work-at-a-glance) and does not by itself block the next implementation slice. Stop only when a required design/input is absent or stale, source contradicts the contract, an executed check finds a defect, or implementation cannot proceed without inventing policy;
4. implement only the selected stage and defects required for its exit criteria. Do not begin later UI, general framework, performance, denoising, neural, material-system, or compatibility work;
5. preserve Scene-owned scene data, View-owned view identity/camera data, one private Reference Path Tracer product/policy capsule, the existing small shared path-tracing/RayTracing/Lighting/Common shader owners, thin RHI traversal adapters, and the single-truth/copy budget. After preparation, pass the canonical `RenderFrame` through execution boundaries instead of unpacking parallel identity/time/Scene/View/ray-binding arguments or carrying the complete mutable viewport request beside its accepted View; pass only a focused one-shot control edge when the feature consumes one. Preserve the original Sparkle frame shell: `FramePipeline::BuildRenderFrameGraph` calls ordinary nested `Add...Passes` functions, while `AddSceneRenderingPasses` selects the accepted request's `RenderViewMode::ReferencePathTracer` and invokes either the Lit or Reference pass function directly before declaring shared exposure, optional scene denoising, and presentation-upscaling passes once. Never add a class-shaped graph stage, recipe/base interface, graph factory, dependency bag, global feature manager, Editor mirror enum, target/show-flag translation, graph-settings copy, process-global selector, RHI mode, second renderer/frame loop/submission path, or both estimator middles together;
6. apply the [GPU-Only Transport Boundary](#remaining-work-at-a-glance) and [Shared Path-Tracing Family Boundary](#remaining-work-at-a-glance) before adding or naming any type, function, resource, or file. Keep only Reference target/claim, selected strategy set and admissibility, sample-stream identity/dimensions, finite/reference termination, per-view session/invalidation/progress, raw-result authority, evidence, and failure policy in the feature capsule. Extend the established `Common`, `Lighting`, `RayTracingHit*`, `PathSurface`, `RayTracingPathSample`, `PathSampling`, `PathLighting`, and `PathTracer` shader owners for camera/RNG mechanics, trace/hit/material/surface conversion, environment, path state/events, BSDF/light sample-evaluate-PDF, visibility, MIS/roulette arithmetic, robust rays, and accumulation arithmetic whose semantics are reusable by unbiased, optimized/biased, cache-backed, or denoised GPU path tracers. Search current Reference, `Path*`, ReSTIR, and cache-facing shader code before adding code. No generic mechanism may retain a `ReferencePathTracer` name; no Reference-only or biased/cache/denoiser policy may enter a shared owner; no second shared vocabulary may compete with an established one; and only the production GPU shader route may produce transport results;
7. perform clean breaks for Sparkle-owned contracts: update all producers/consumers/build/docs together and delete the replaced path. Do not add legacy readers, aliases, version bridges, fallback selectors, or dual representations;
8. keep current code/behavior labels honest. Never use “unbiased,” “ground truth,” “converged,” or “accepted reference” beyond the exact passed scope;
9. design each check with initial state, action/injection, oracle, matrix, artifact, maximum duration/resources, cleanup, and escalation. Use the cheapest claim-falsifying check first;
10. do not add submitted test-only classes, fixtures, executables, files, CMake targets, feature-specific diagnostic passes/readbacks, debug panels, dashboards, or parallel inspection APIs. Use the real production route and existing generic capture/validation surfaces. A temporary focused probe is allowed only when a named criterion cannot be falsified more cheaply, and every probe must be removed before handoff;
11. execute `CHK-RPT-17` for the current stage. Retain `ARCH-RPT-<stage>` listing every touched production file outside the frozen feature and shader homes, its accepted hook category and necessity, all feature-named references outside the capsule, new public/shared types and fields, repeated selector switches, dependency direction, and bounded-removal proof. Any unlisted file or unjustified hook blocks the stage;
12. from Stage 2 onward, execute `CHK-RPT-18` and retain `FRAME-RPT-<stage>` proving the sole recipe key/decision, mutually exclusive pass/resource provenance, shared frame shell, raw/display lineage, safe Lit/Reference topology transition, and absence of a second renderer or submission path;
13. from Stage 3 onward, execute the source/ownership portion of `CHK-RPT-19` and retain `CORE-RPT-<stage>` classifying every changed GPU path-tracing operation as shared invariant, Reference policy, or optimized/cache/denoiser policy; list the owning existing file, every current consumer, likely ReSTIR PT/cache/denoiser boundary challenge, semantic-duplicate search, dependency direction, and bounded removal. A new Reference-prefixed mechanism requires a written explanation of why another GPU path tracer cannot use the same semantics. Compile affected Reference and optimized consumers before executable acceptance, or place that compilation in the explicit validation backlog when owner-operated validation is deferred;
14. execute `CHK-RPT-20` for the stage's affected concerns: recheck the pinned NVIDIA revisions and current official Epic Path Tracer documentation, update the alignment row with its local owner/stage/evidence status, and retain every intentional difference or exclusion. Then run immediately available static checks, `architecture_boundary_check` whenever Renderer/RHI boundaries change, focused build/shader/GPU/interactive checks when selected for the current execution, and `git diff --check`. Record every unrun owner-operated check as deferred with its command/workflow and oracle. Do not claim unrun checks passed and do not convert deferral alone into `BLOCKED`;
15. finish the stage with the binding [responsibility refinement](../../../../../../../Engineering/Workflow/ChangeLifecycle.md#finish-with-responsibility-refinement). Re-read the complete changed feature path and record one responsibility sentence for every substantive changed file, class, and function; split independently changing identity, lifecycle, GPU-resource, graph-pass, estimator, presentation, or artifact concerns, while keeping resources owned by the lifecycle that allocates, retains, releases, binds, and retires them rather than passing a sibling resource owner through state transitions; remove dead scaffolding, duplicate state/policy, ceremonial wrappers/helpers/namespaces, needless validation/diagnostics, and leaked implementation detail. This gate is decided by cohesion and knowledge removal, not line count or class count, and fails on either a god unit or fragmentation that merely relocates the same knowledge;
16. leave one iteration record containing gate revision, decisions, changed files grouped by responsibility, deletions, the refinement result, `ARCH-RPT-<stage>`, `FRAME-RPT-<stage>` when applicable, `CORE-RPT-<stage>` when applicable, checks actually run, exact outputs/artifact links, deferred-validation backlog, known limitations, confirmed blockers, and the next permitted implementation stage.

Every prompt's `NON-NEGOTIABLE` paragraph is a contract, not motivational prose. A source-delivery handoff must classify each item as statically established, executable evidence passed, validation deferred, or falsified. Use **IMPLEMENTED / VALIDATION DEFERRED** when the implementation is complete but owner-operated evidence remains; use `BLOCKED` only for a confirmed defect, contradiction, missing required design/input, or implementation dependency that prevents meaningful progress. “Implemented,” a clean build, a plausible image, or a manual click-through cannot substitute for final acceptance evidence.

If source reality proves a plan instruction wrong, correct the owning architecture/plan document in the same stage and explain the divergence. Do not preserve a bad plan through code contortions.

## Stage 3 - Validate The Semantic Integrator

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- Black, constant environment, emissive hit, Lambertian normalization/energy, one- and two-segment hand cases pass within frozen tolerance.
- Canonical View center/edge/corner/subpixel rays and Philox known vectors/dimensions match accepted `MATH-01` and `MATH-09`; no camera or sample identity depends on frame timing or TAA jitter.
- Intentional PDF, cosine, emission, sample-selection, and invalid-value faults are detected by the named checks.
- No GBuffer, real-time lighting, post-process, temporal history, API-specific estimator fork, hidden limit, biased approximation, cache, or denoiser policy enters the shared path-tracing core.
- Generic traversal, trace-result, hit-data/reconstruction, path-surface/emission, sky evaluation, camera-ray, RNG permutation/conversion, path-state, radiance/throughput, BSDF-sample vocabulary, and numeric code has one focused shared semantic owner; the feature capsule contains no copied `OpaqueTraceResult`, `TraceOpaque`, `ReferencePathTracerSemantic`, surface/BSDF/radiance/environment wrapper, hit decoder, generic math implementation, or feature-prefixed replacement. Shared changes have current Reference and optimized consumers where semantics truly match and retain no Reference-only policy.
- The minimal Reference radiance is produced by the alternate middle of the ordinary Sparkle frame; it does not use a second renderer/executable/submission path and is not composed on top of Lit output.
- Scene-kind and Game-kind Views execute the same camera/transport/sample implementation; View kind never selects or forks the estimator.
- Focused `CHK-RPT-03`, `04`, `05`, `18`, and `19` cover the minimal portions of `AC-RPT-05` through `10`, retained `AC-RPT-22`/`23`, `FM-RPT-02` through `06`, and `FM-RPT-21`/`22`.


## Stage 4 - Validate Surface Transport

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- Every included BSDF/light strategy passes normalization, energy/white-furnace or applicable limit, unit/PDF/Jacobian, delta/zero, emission/environment, and NEE/MIS hand cases.
- Independent replicates show the expected mean on analytic scenes; injected missing/duplicate probabilities, MIS weights, emission, and roulette compensation fail.
- `SurfaceTransportReference` contains no accepted deterministic cutoff, clamp, filter, cache, or biased environment MIP.
- `CORE-RPT-4` proves one shared surface/event/BSDF/light/PDF/visibility/MIS/roulette vocabulary is consumed by Reference and affected optimized paths while Reference strategy composition remains enclosed.
- `CHK-RPT-04`, `05`, early `11`, `18`, and `19` cover `AC-RPT-06` through `10`, `15`, retained `AC-RPT-22`/`23`, `FM-RPT-05`, `06`, `11`, `16`, `21`, and `22`.


## Stage 5 - Validate Content And Numeric Robustness

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- Every included content row has analytic/metamorphic GPU evidence and a feature-support rejection for excluded cases.
- Robustness fixtures pass without per-scene epsilon or distance tuning; both API representations are considered even if Stage 8 retains final parity.
- All invalid values and rejected events are accounted for; no visible defect is “fixed” by a contribution clamp.
- `CORE-RPT-5` proves that material decoding, hit/path-surface reconstruction, alpha/sidedness, deformation snapshots, normal handling, and robust-ray math have one shared owner with affected non-Reference consumers updated.
- `CHK-RPT-03`, `04`, `08`, `18`, and `19` cover `AC-RPT-05` through `08`, `12`, `15`, retained `AC-RPT-22`/`23`, `FM-RPT-04`, `08`, `16`, `19`, `21`, and `22`.


## Stage 6 - Validate Per-View Accumulation And Mode Authority

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- Accumulation matches the higher-precision oracle at all frozen prefix/count extremes and state transitions.
- Every mutable contributing input is present in one canonical digest or explicitly excluded; same semantic Scene/View input reproduces the digest, while every admitted radiance/camera mutation changes its named component.
- Every Editor/Game camera and scene change resets before mixing; target-SPP, presentation, scheduling, and Lit comparison changes preserve or revalidate exactly as specified.
- Sample ranges neither overlap nor skip; stale or out-of-order completion cannot advance the committed prefix.
- Submitted GPU work meets the frozen responsiveness/watchdog quantum contract without importing wavefront, compaction, SER, or vendor-specific policy absent measured need.
- During camera movement, no stale prefix commits and the latest accepted camera identity reaches the display within the frozen responsiveness budget; after motion stops, accumulation proceeds automatically from ordinal zero.
- Pause/restart/Lit-suspend/resume, eviction, timeout, cancellation, and view destruction produce the accepted prefix/state/resource result.
- Raw accumulation and its display derivative are mechanically distinguishable; every forbidden presentation/bias switch is absent or rejected.
- `CORE-RPT-6` proves accumulation/merge/variance arithmetic is reusable and free of Reference lifecycle/product policy, while no generic session/job/history framework or second accumulator authority was introduced.
- `CHK-RPT-09`, focused `CHK-RPT-15`, `CHK-RPT-18`, and `CHK-RPT-19` cover accumulation/lifecycle portions of `AC-RPT-13`, `15`, `18`, retained `AC-RPT-22`/`23`, and `FM-RPT-06`, `09`, `10`, `14`, `15`, `21`, `22` without requiring disk output.


## Stage 7 - Validate Live Viewport And Lit Comparison

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- A first-time Editor user finds Reference Path Tracer immediately after Lit, while an approved Game/runtime host can select the same mode through its ordinary development view selector. Either route sees the latest composition accumulate, moves/rotates its canonical camera with responsive feedback and exact reset, stops and sees automatic refinement, completes, switches Lit/back with correct resume/reset, and pauses/restarts without private Renderer knowledge, saving, or a wizard.
- Every hard invalidation resets before mixing; every presentation/scheduling-only change preserves the prefix; continuous animation never produces accepted streaked history.
- The overlay remains truthful through validation, rapid camera motion, reset, accumulation, completion, pause, Lit suspension, eviction, unavailable, and failure states; target progress is never labeled convergence.
- Closing details/viewport/application, invalid inputs, second-view capacity, eviction, time/resource limits, accessibility, and user-facing error/support behavior match the frozen contract.
- `CORE-RPT-7` confirms the UI/session slice added no host-specific estimator path, Reference-prefixed reusable mechanism, or optimized-path duplicate.
- Focused `CHK-RPT-02`, `09`, `15`, `16`, `18`, `19`, and `20` close the primary viewport portions of `AC-RPT-02`, `04`, `13`, `18`, `20`, retained `AC-RPT-22`/`23`, and `FM-RPT-09`, `14`, `17`, `18`, `21`, `22`.


## Stage 8 - Prove Traversal And Backend Parity

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- One focused shared path/RayTracing family and one enclosed Reference policy are visible in the source/dependency audit; ray/result/payload/hit/visibility mechanics are reusable and non-Reference-named, feature entry shaders contain composition only, and optimized consumers do not fork identical operations.
- All four required strict route combinations pass capability, native validation, robustness, deterministic/statistical parity, live camera reset/refinement, completion, and unsupported-route behavior, or `PTD-00` is formally narrowed before closure.
- No fallback, backend drift, payload/SBT mismatch, viewport-state divergence, or unexplained native validation message remains.
- `CHK-RPT-07`, `08`, `12`, focused `16`, `18`, and `19` cover `AC-RPT-11`, `12`, `17`, `20`, retained `AC-RPT-22`/`23`, and `FM-RPT-07`, `08`, `13`, `18`, `21`, `22`.


## Stage 9 - Prove Manual Publication And Evidence-Candidate Integrity

**Remaining work:** exercise the existing production route against the following retained exit requirements; repair only demonstrated failures. This is deferred validation, not another implementation of the delivered feature.

- `CHK-RPT-03` through `13` all pass for every included matrix cell before Stage 9 may claim an evidence candidate. If owner-operated execution is deferred, Stage-9 implementation may close as **IMPLEMENTED / VALIDATION DEFERRED**, the exact matrix remains in the backlog, and no oracle/readiness credit is granted.
- The protocol detects seeded bias, correlation, shared-dependency, local-image, invalid-value, truncation, accumulation, backend, and artifact-publication defects.
- Raw save uses the exact session extent and precision, never screenshot/UI/preview resolution; checkpoint/partial/staging/completed states are mechanically distinct, and publication failure preserves the live viewport prefix and prior valid result.
- Editor and approved runtime views preserve the same canonical request/digest/sample stream; adding artifact work does not regress the Stage-7 live-navigation and Lit-comparison gates.
- `CHK-RPT-18` still proves that Editor and runtime execution enter the same original frame and mutually exclusive Reference middle recipe; artifact publication has not introduced a detached renderer, second submission route, or Lit-product dependency.
- `CORE-RPT-9` confirms evidence/export work consumes the same shared path semantics and immutable raw prefix without adding a Reference-specific capture/readback codec or forking common path-tracing operations.
- External comparisons retain exact source revisions/configurations/licenses and semantic equivalence; agreement is supporting evidence, not derivation proof. `CHK-RPT-20` confirms every NVIDIA/Epic lesson remains implemented, stage-owned, deliberately different, or excluded rather than silently omitted.
- No unexplained native validation, crash/hang, invalid-result state, statistical disagreement, or plausible partial result remains.


## Stage 10 - Adopt References, Verify Packaging, Remove Superseded Paths, And Close

### Objective

Finish the product boundary, produce release-map references inside the accepted domain, delete old authority, and submit the candidate `FCR-REN-08` report.

### Work

1. Freeze each applicable release-map scene/camera/configuration and verify it lies wholly inside the accepted transport domain.
2. Produce raw high-sample references, independent replicates, uncertainty/convergence, full frames/crops, artifact checklist, input/output hashes, and shared-dependency oracle statements for `PTD-03`/`MAP-A` through `MAP-H`.
3. Compare real-time PBR and lighting subjects only against applicable raw reference quantities. Keep display/preview comparisons separately labeled.
4. Validate the exact DevelopmentEditor/package workflow on clean supported machines, read-only install, writable output root, spaces/non-ASCII, both backends, first use, support output, resource bounds, and controlled failures. Both `ShippingEditor` and `ShippingGame` remain free of every producer/session factory and user/export/package route unless admitted.
5. Complete the deletion ledger: old reference producer/history/settings/shaders/selectors/docs/artifacts are removed or retained under an unambiguous non-authoritative role. Remove temporary probes and generated local evidence not permitted for submission.
6. Verify source/header/shader/generated/CMake/package/SBOM/license/docs membership and dependency direction. Re-run `CHK-RPT-20` against the then-current pinned NVIDIA revisions and official Epic Path Tracer documentation; any newly relevant capability is explicitly adopted, assigned, deliberately different, or excluded before the completion report.
7. Run the required focused final checks, architecture boundary check, `git diff --check`, and dirty-work audit. Escalate breadth only for claims actually affected.
8. File `FCR-REN-08` with exact results, exclusions, failures, limits, evidence links, and invalidation triggers. The acceptance owner, not the implementer or this plan, decides `PASS`, `BLOCKED`, or `EXCLUDED`.

### Exit gate

- Every applicable `AC-RPT-01` through `23` passes conjunctively; every `FM-RPT-*` has exercised detecting evidence; all `RPT-FS-*` rows match public/release reachability.
- `CHK-RPT-14`, `15`, `16`, `17`, `18`, `19`, and `20` pass in addition to retained Stage 9 evidence.
- `SurfaceTransportReference` is the single reference authority; no GBuffer-seeded or compatibility reference path competes with it.
- Package and release-map use remains inside the accepted domain and preserves raw/preview/provenance separation.
- `FCR-REN-08` records the real verdict. Only an accepted result authorizes downstream `PTD-03` ground-truth use.

### Ready-to-use prompt

```text
Execute only Stage 10 of Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md after Stage 9 evidence is complete and the exact `ReleaseMapSet`, support-machine identities, and `REL-03 PASS` package revision are accepted. Apply the Universal Execution Contract.

Freeze every applicable release-map camera/configuration within the accepted transport domain and produce PTD-03 raw references with independent replicates, convergence/uncertainty, complete provenance, hashes, full frames/crops, artifact review, and shared-dependency oracle statements. Exercise the exact clean-machine DevelopmentEditor/package workflow, writable-root/read-only-install and spaces/non-ASCII paths, both backends, first use, user-facing error/support output, budgets, and controlled failures. Keep `ShippingEditor` and `ShippingGame` free of every producer/session factory and user/export/package route unless release scope explicitly admits them.

NON-NEGOTIABLE: ship only the scope proved by the accepted TransportAndEstimator.md revision and only the workflow proved by UserExperience.md. Every view-mode label/order, preset, overlay, reset reason, Editor/Game/runtime result, manifest, artifact action, map comparison, and support message must preserve the bounded claim, host-agnostic Renderer semantic, and raw/preview distinction. A host/View-kind estimator fork, map outside the domain, unexplained invalid-result state, unresolved MATH-* defect, inaccessible first-use step, stale/mixed prefix, silent capability substitution, plausible partial artifact, or surviving competing selector/estimator authority blocks FCR-REN-08.

Complete the clean break: remove or unambiguously relabel all old GBuffer-seeded reference producer/history/settings/shaders/selectors/docs and remove temporary probes; audit source/header/shader/generated/CMake/package/SBOM/license/docs membership and unrelated dirty work. Retain `FRAME-RPT-10` and `CORE-RPT-10`; run CHK-RPT-01, 14, 15, 16, 17, 18, 19, 20 plus every invalidated prior check, architecture_boundary_check, focused builds/runs required by the claims, and git diff --check. File FCR-REN-08 with exact PASS/BLOCKED/EXCLUDED evidence and limitations. Do not call the feature accepted unless every applicable AC-RPT-01 through 23 passes conjunctively and the acceptance owner approves the report.
```

## Remaining Execution Prompt

```text
Execute the earliest unfinished recovery/validation slice in Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md. Apply its Universal Execution Contract and current design owners. Do not repeat delivered Stage 0–9 implementation. Revalidate current code, cooked inputs and candidate evidence, locate the first incorrect product, and repair only the identified defect. Retain the existing frozen tolerances, capacities, matrix, source/bytecode identity and failure oracles; use the smallest required checks. Source presence or deferred execution is not PASS. Remove probes, pass the Stage Source-Style Gate for source changes, and remove completed tasks from this plan at handoff.
```


## Stop And Escalation Rules

Stop the active stage and report `BLOCKED` only when source inspection or an executed check confirms one of the following, or when a required design/input/dependency is genuinely unavailable. An unrun build, shader cook, GPU run, backend exercise, or interactive Editor workflow is recorded as deferred validation and is not itself a blocker:

- an unresolved decision can change estimator expectation, supported domain, ownership, artifact identity, backend claim, or evidence threshold;
- a release map or public selector reaches an excluded semantic;
- implementation requires a second scene/material/session/accumulation/selector authority or compatibility layer;
- the Reference route requires a second renderer/frame loop/submission path, runs Lit and Reference estimator middles together, consumes Lit estimator products as raw reference input, duplicates recipe resolution below the topology boundary, or adds a concrete feature dependency to generic graph construction;
- Reference Path Tracer cannot live immediately after Lit, cannot preserve Lit settings, or needs a mandatory setup wizard/console route for its accepted default;
- the selected mode cannot stay responsive while navigating, cannot present the newest accepted camera identity within the frozen budget, or needs manual restart after movement stops;
- a canonical Editor/Game camera or radiance-affecting change can mix prefixes, or a presentation/scheduling-only change needlessly resets;
- a shared dependency has no independent defect-detecting oracle;
- a reusable camera/RNG/trace/hit/material/surface/environment/BSDF/light/path/ray/accumulation operation is introduced or retained under a `ReferencePathTracer` name, duplicates an established shared owner, or a shared owner absorbs Reference/optimized/cache/denoiser product policy;
- an invalid/capped/rejected contribution can disappear without a retained signal;
- artifact/checkpoint work is being used to delay or substitute for the Stage-7 viewport milestone, or a checkpoint/partial output can be mistaken for complete evidence;
- automatic capability resolution selects an incomplete route, native validation remains unexplained, or backend/frontend outputs diverge beyond the frozen rule;
- an external comparison lacks semantic equivalence or rights/provenance;
- a stage would need broad framework, wavefront, denoising, neural, spectral, media, transmission, or unrelated feature work not admitted by `PTD-00`.

Escalation changes the smallest falsified surface first. It does not begin with a full engine build, full cook, all maps, or arbitrary SPP increase. The full package/map matrix belongs only to Stages 9-10 after lower-level claims pass.

## Completion Rule

This plan is complete only when the acceptance owner records `FCR-REN-08 PASS` against one immutable evidence set and every applicable `AC-RPT-01` through `23` passes. If scope, math, sampler, material/light semantics, compiler/shader identity, backend/frontend, accumulation, artifact schema, architecture hooks, frame-recipe topology, shared path-tracing core, external precedent, or release content changes later, the completion report names the invalidated evidence and reruns the smallest affected checks, including `CHK-RPT-17` for any source-shape change, `CHK-RPT-18` for any recipe/resource-lineage change, `CHK-RPT-19` for any path-family semantic or ownership change, and `CHK-RPT-20` for any NVIDIA/Epic source or correspondence change.

An implemented tracer with incomplete evidence is **not complete**. A fully evidenced finite-path diagnostic is **not the full surface reference**. A package workflow with a shared or post-processed oracle is **not trustworthy**. The plan is designed to make those substitutions impossible to hide.
