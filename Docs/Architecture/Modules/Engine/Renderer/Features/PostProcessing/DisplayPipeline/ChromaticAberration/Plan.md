# Chromatic Aberration Delivery Plan

**Status:** conditional implementation plan; Stage 0 is permitted, Stages 1-4 are blocked until `CHRD-00 PASS` and `DSP-6` prerequisites

**Responsibility:** order discovery, inactive state, semantic pass, editor/debug/capture integration, and candidate evidence for `FCR-REN-25`

**Authority boundary:** [Discovery](Discovery.md) freezes decisions; [README](README.md) owns acceptance; [Display And Reconstruction](../../../../FirstRelease/DisplayAndReconstruction.md#dsp-6--chromatic-aberration) owns release sequence

**Current readiness:** **0/100** — target only.

## Execution Contract

Each stage maps the touched `AC-CHR-*`, `FM-CHR-*`, `RISK-CHR-*`, and `CHK-CHR-*`; starts from current status/revision/dirty state; uses the smallest defect-detecting check; extends current settings/View/graph/shader/editor/capture/package owners; removes replaced names; and stops on an unresolved domain, unit, channel, edge, debug, or budget decision. No compatibility path, spectral-LUT framework, history, quality tiers, permanent test code, or silent fallback is allowed.

## Stages

### Stage 0 — Close `CHRD-00`

Run `CHR-EXP-01` through `06`, disposition `CHRD-01` through `10`, freeze model/controls/domains/units/bounds/state/debug/budgets/evidence, and accept exact document revisions. No production code changes.

**Exit:** the semantics can be independently implemented without choosing a constant or coordinate rule, and every risk/criterion/failure has a defect-detecting check.

```text
Execute only Chromatic Aberration Stage 0. Inspect current post/display/settings/View/shader/debug/capture/UI/package paths, compare simple RGB and spectral precedent, and freeze every CHRD decision, model constant, control range, domain, unit, edge rule, matrix, budget, fixture, tolerance, and invalidation trigger. Update only the owning documents. Stop instead of choosing from visual preference alone.
```

### Stage 1 — Add State And A Neutral Seam

Extend public display settings, persistence, per-view overrides, immutable View resolution, graph vocabulary, shader registration, and requested/resolved/active diagnostics. Keep strength neutral and omit all GPU work.

**Exit:** neutral and invalid states are truthful; two views isolate; settings round-trip; output extent is the only geometry input; no second state owner or persistent resource appears.

```text
Execute only Chromatic Aberration Stage 1 from accepted CHRD-00 revisions. Add the smallest typed settings/View/graph/shader-catalog state and requested/resolved/active result. Keep output unchanged and omit the pass. Exercise neutral, invalid/non-finite/range values, persistence, per-view override, two views, resize/minimize, missing program membership, and package surface. Do not implement sampling yet.
```

### Stage 2 — Implement And Prove The Lens Pass

Implement `CHR-MATH-*` in one shader with an independent CPU coordinate/filter oracle. Insert the distinct target-linear pass, preserve alpha/product identity, clamp all reads, omit zero, and use no history.

**Exit:** `AC-CHR-01` through `04` semantic portions pass; swapped channel, wrong scale/aspect/stage, wrap, alpha, and zero-work defects are detected; D3D12/Vulkan shaders compile through owned routes and cost meets the frozen budget.

```text
Execute only Chromatic Aberration Stage 2. Implement the accepted three-channel model at the accepted target-linear boundary with output-extent pixel units, bilinear clamp, alpha preservation, and exact zero omission. Compare an independent double-precision CPU evaluator over impulse/grid/edge/alpha patterns, all admitted aspects/resolutions/centers/offsets/strengths, and seeded semantic defects. Inspect typed bindings, native validation, raw pre/post products, and pass cost on both backends. Stop on any unexplained mismatch.
```

### Stage 3 — Complete Experience And Integration

Add the narrow DevelopmentEditor controls to the existing rendering settings surface, neutral reset, per-view override, active/error state, capture metadata, exact-debug bypass, UI exclusion, and automation fields. Exercise rapid edits, resize, view switch, invalid state, missing pipeline, capture, and package operation.

**Exit:** `AC-CHR-05/06/08` and all controlled failure paths pass; a developer can identify whether the intentional effect is active; ordinary artifacts cannot be mislabeled.

```text
Execute only Chromatic Aberration Stage 3. Wire accepted controls and status through the existing settings/editor/View owners, classify every debug mode, keep UI downstream, and publish bounded capture/automation metadata. Exercise first use, reset, invalid values, two views, rapid edits, resize/DPI/minimize, mode/debug changes, missing pipeline, capture lineage, and packaged settings. Do not add a player-facing menu or generic post-process framework.
```

### Stage 4 — Candidate Evidence And Closure

Run `CHK-CHR-01` through `05` for one immutable candidate, including analytic defect controls, matched resolutions, temporal camera motion, SDR/HDR cells, D3D12/Vulkan raw comparison, native validation, package, cost/memory, excluded-surface audit, and artifact classification. Remove temporary probes and submit `FCR-REN-25`.

**Exit:** all `AC-CHR-01` through `08` pass conjunctively, each failure/risk has detecting evidence, and the acceptance owner records the verdict.

```text
Execute only Chromatic Aberration Stage 4 for one frozen candidate. Run the complete predeclared CHK-CHR matrix and seeded defects across domains, resolutions, aspects, views, debug/UI/capture, backends, package, and cost. Remove temporary test-only code; reconcile source/header/shader/generated/CMake/settings/package/docs membership; and file FCR-REN-25 with exact evidence and limitations. Do not retune the model or thresholds after seeing candidate output.
```

## Completion Rule

A working shader is not completion. The plan closes only when the authored units remain stable across the admitted matrix, the effect is distinguishable from accidental fringing, zero is exact no-work identity, and `FCR-REN-25` records an accepted candidate result.

