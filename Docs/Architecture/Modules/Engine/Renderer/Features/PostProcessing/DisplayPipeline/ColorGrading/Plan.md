# Color Grading Delivery Plan

**Status:** conditional implementation plan; Stage 0 is permitted, production Stages 1-6 are blocked until `CGRD-00 PASS` and `DSP-5` prerequisites

**Responsibility:** order discovery, semantics, ownership, parametric grade, LUT workflow, editor/runtime integration, evidence, packaging, and closure for `FCR-REN-24`

**Authority boundary:** [Discovery](Discovery.md) authorizes the accepted design revision; [README](README.md) owns feature acceptance; [Display And Reconstruction](../../../../FirstRelease/DisplayAndReconstruction.md#dsp-5--color-grading) owns release-wide order; this plan owns only feature-local delivery

**Current readiness:** **0/100** — plan presence is not implementation or evidence.

## Universal Execution Contract

Every stage starts from current status/revision/dirty state, maps its touched `AC-CGR-*`, `FM-CGR-*`, `RISK-CGR-*`, and `CHK-CGR-*`, and names the smallest claim-falsifying check. Extend existing settings, View, frame-graph, shader, asset/cook, residency, editor, capture, and package owners. Use one scene-grade semantic contract on D3D12 and Vulkan. Delete replaced names/paths in the same stage; add no compatibility reader, alternate settings authority, generic color framework, permanent test-only code, or silent fallback.

Every stage stops when an accepted discovery decision is missing or contradicted; working space/order is ambiguous; requested and active state cannot be distinguished; source/cooked/runtime identity forks; a stale/partial generation can publish; an excluded feature becomes reachable; or the selected check cannot detect its seeded defect.

## Stage Map

| Stage | Result | Prerequisites |
| --- | --- | --- |
| 0 | `CGRD-00` decision package | source/research access |
| 1 | narrow types, identity, and inactive vertical seam | Stage 0 `PASS`, `DSP-0/4` domain prerequisites |
| 2 | parameter-only scene-referred grade | Stage 1 |
| 3 | typed `.cube` source-to-cooked contract | accepted LUT semantics and asset-owner decision |
| 4 | residency join and LUT-applied grade | Stages 2-3 |
| 5 | editor, capture, two-view, and automation experience | Stage 4 |
| 6 | backend/package/performance evidence and FCR handoff | Stage 5 and release candidate identity |

## Stage 0 — Close Discovery

**Work:** execute `CGR-EXP-01` through `07`; disposition `CGRD-01` through `12`; freeze semantic, architecture, experience, budget, evidence, rights, and deletion revisions. Reconcile target wording when decisions change.

**Exit:** all `AC-CGRD-*` pass, no implementation-shaping unknown remains, and the accepted document revisions are named. Otherwise record `Blocked` with the exact owner and next experiment.

```text
Execute only Color Grading Stage 0. Do not change production code. Inspect current Renderer/display/settings/shader/asset/cook/residency/editor/capture/package owners; run the seven discovery experiments; freeze every CGRD decision, risk, fixture, tolerance, budget, and invalidation trigger. Update only the owning feature documents. Stop rather than choosing working space, negative math, LUT grammar/layout/interpolation, ownership, UX, or evidence thresholds by convenience.
```

## Stage 1 — Establish Types, Identity, And The Inactive Seam

**Work:** add the narrow public/internal parameter, asset-handle, semantic revision, digest, and requested/resolved/active result types; extend settings persistence and View preparation; add target resource/stage vocabulary and shader registration/build membership without applying a non-identity grade. Prove neutral omission, serialization, two-view isolation, and invalid-state refusal.

**Exit:** neutral state adds no pass/resource; identical intent hashes identically; distinct view/asset/semantic generations do not alias; invalid requests never report active; no second settings or residency owner exists.

```text
Execute only Color Grading Stage 1 from the accepted CGRD-00 revisions. Extend existing display settings, View preparation, frame-graph resource vocabulary, shader catalog, and build membership with the smallest grade state/identity seam. Keep runtime output exactly neutral and omit work. Prove settings round trip, digest stability, two-view isolation, requested/resolved/active truth, invalid enum/value rejection, and clean package/build membership. Do not add parsing, LUT upload, non-neutral math, or a manager singleton.
```

## Stage 2 — Deliver The Parameter-Only Grade

**Work:** implement accepted SOP and saturation semantics in one shared HLSL owner plus independent CPU oracle; insert the distinct scene-referred pass at the accepted boundary; preserve alpha/product identity and exact debug policy. Exercise identities, edge values, order mutations, non-finite inputs, backend shader compilation, and output-resolution extents.

**Exit:** `AC-CGR-01/02/05` parameter portions and `FM-CGR-01/03` pass; seeded wrong order, weights, negative rule, channel, alpha, and double-application defects are detected; cost remains within the Stage-0 budget.

```text
Execute only Color Grading Stage 2. Implement the accepted CGR-MATH parameter transform once, after the accepted reconstruction/exposure boundary and before target tone mapping. Use typed binding and an independent double-precision oracle. Keep LUT absent. Exercise neutral omission, analytic colors/ramps/negative/HDR/non-finite values, alpha sentinel, wrong-order/weight/rule defects, debug classifications, output extents, D3D12/Vulkan shader paths, native validation, and pass cost. Stop on any semantic or stage ambiguity.
```

## Stage 3 — Add The Typed LUT Source And Cooked Contract

**Work:** extend the owning asset/cook route with the accepted bounded `.cube` subset, canonical representation, source hash, schema/semantic identity, transactional write, deterministic recook, errors, and package dependency discovery. Use parser/math probes locally; do not add submitted test files unless separately authorized.

**Exit:** valid fixtures produce deterministic bytes; malformed/hostile/unsupported cases fail before publication/allocation; dimension/domain/count/axis data are unambiguous; package planning sees the logical dependency; old schema paths do not survive.

```text
Execute only Color Grading Stage 3. Add the accepted .cube source parser and canonical cooked grade-LUT format at the existing asset/cook owner. Enforce checked counts and byte/dimension limits, finite/domain/directive rules, deterministic bytes, provenance, transactional publication, and clear errors. Exercise identity/asymmetric known-cell plus malformed/truncated/extra/non-finite/overflow/unsupported fixtures. Do not upload, render, embed OCIO, or accept extra formats.
```

## Stage 4 — Join Residency And Apply The LUT

**Work:** load/validate the cooked contract, reuse the existing residency/upload/retirement path, bind the immutable LUT generation, and implement accepted domain/axis/texel/interpolation/composition semantics. Exercise stale completion, upload/capacity/device failure, repeated replacement, two views, and CPU/GPU known-cell parity.

**Exit:** `AC-CGR-03/04/06` runtime portions pass; no partial/stale generation activates; requested/active identities stay truthful; LUT application detects axis, half-texel, interpolation, domain, order, and stale-generation defects.

```text
Execute only Color Grading Stage 4. Reuse existing residency to publish a checked cooked LUT generation and bind it to the one grading pass. Implement the accepted LUT mapping/composition exactly. Exercise known asymmetric cells and interpolation points, boundary/out-of-domain values, missing/corrupt cooked data, upload/OOM/capacity/device loss, randomized replacement completion, two views, and delayed GPU retirement. Preserve the explicit last-good/default state and never claim the pending request active.
```

## Stage 5 — Complete The Development Experience

**Work:** implement the [User Experience](UserExperience.md) through the current rendering settings/editor surface, source asset selection, state/error presentation, reset, live edit/cancel, capture metadata, and noninteractive manifest. No player-facing menu or file browser is admitted.

**Exit:** first use, all control states, keyboard/text access, rapid edit, failure/recovery, reset, two-view isolation, capture lineage, and editor/manifest equivalence pass; no console-only step or silent substitute remains.

```text
Execute only Color Grading Stage 5. Add the accepted DevelopmentEditor controls and automation fields over the existing settings and active-state owners. Show semantic domain, neutral values, LUT metadata/identity, requested versus active/pending/unavailable reason, and reset. Exercise first use, invalid source, recook/upload failure, rapid edits/cancel, two views, capture metadata, package-missing state, accessibility, and manifest equivalence. Do not create a color-suite, volume system, runtime player UI, or alternate state cache.
```

## Stage 6 — Evidence, Package, And Close

**Work:** run all predeclared `CHK-CGR-*` against the exact candidate; compare D3D12/Vulkan raw stage products, SDR/HDR joins, package operation, memory/cost, clean machine, and excluded-surface audit. Remove local probes and superseded paths; audit CMake, generated shaders, assets, package allowlists, rights, docs, and dirty work. Submit `FCR-REN-24`.

**Exit:** all applicable `AC-CGR-01` through `08` pass conjunctively, every failure/risk has detecting evidence, all excluded surfaces remain absent, and the acceptance owner records the real verdict. Documentation, a successful build, or one attractive screenshot cannot close the feature.

```text
Execute only Color Grading Stage 6 for one frozen candidate. Run CHK-CGR-01 through 06 with seeded defect controls, raw pre/post-grade products, backend comparisons, two-view and replacement stress, package/clean-machine route, cost/memory, capture lineage, and excluded-feature audit. Remove temporary test-only code and reconcile source/header/shader/generated/CMake/asset/package/docs membership. File FCR-REN-24 with exact PASS/BLOCKED/EXCLUDED evidence and limitations; do not tune thresholds after candidate output.
```

## Completion Rule

This plan completes only when `FCR-REN-24` is decided against one immutable candidate and evidence set. A parameter-only pass is not LUT completion; a valid LUT parser is not runtime activation; a visually pleasing image is not colorimetric proof; and an identity fallback is never the requested grade.

