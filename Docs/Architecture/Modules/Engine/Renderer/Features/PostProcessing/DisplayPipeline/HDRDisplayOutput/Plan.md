# HDR Display Output Delivery Plan

**Status:** conditional implementation plan; Stage 0 is permitted, Stages 1-7 are blocked until `HDRD-00 PASS` and `DSP-7` prerequisites

**Responsibility:** order discovery, neutral presentation contracts, D3D12/Vulkan activation, Renderer color transform, UX/transitions, evidence, packaging, and `FCR-REN-26` handoff

**Authority boundary:** [Discovery](Discovery.md) authorizes the accepted design; [README](README.md) owns feature acceptance; [Display And Reconstruction](../../../../FirstRelease/DisplayAndReconstruction.md#dsp-7--hdr10-display-output) owns release-wide order; Renderer/RHI module owners retain their boundaries

**Current readiness:** **0/100** — plan and source precedent are not implementation/evidence.

## Universal Execution Contract

Every stage records current revision/dirty state, owner path, touched `AC-HDR-*`, `FM-HDR-*`, `RISK-HDR-*`, and `CHK-HDR-*`, and the cheapest defect-detecting check. Preserve the working SDR route continuously. Renderer owns color/target/UI policy; RHI owns output/native presentation facts. Requested state never implies active state. Metadata never substitutes for color-space/pixel/measurement proof. Use no Renderer native API calls, backend-specific artistic transforms, compatibility paths, second HDR state store, permanent test-only code, or silent fallback.

Stop when an accepted decision is missing/contradicted; active tuple is incomplete/stale; SDR cannot be restored atomically; a frame can combine HDR pixels with SDR presentation or the reverse; UI/capture domains are ambiguous; required hardware/native validation is unavailable; or the selected check cannot detect its seeded defect.

## Stage Map

| Stage | Result | Prerequisites |
| --- | --- | --- |
| 0 | `HDRD-00` accepted package | research and named probe hardware |
| 1 | neutral RHI request/capability/result and SDR-preserving state machine | Stage 0 |
| 2 | D3D12 eligible tuple activation and fallback | Stage 1 |
| 3 | Vulkan eligible tuple activation and fallback | Stage 1 |
| 4 | independent Renderer HDR target/PQ transform | Stages 1 and accepted semantics; may use offscreen/native-neutral fixture |
| 5 | joined active-result, UI, debug, capture, and settings experience | Stages 2-4 |
| 6 | transition/recovery/backend/package hardening | Stage 5 |
| 7 | candidate colorimetric/physical/performance evidence and FCR handoff | Stage 6, release candidate/hardware |

## Stage 0 — Close Discovery

Run `HDR-EXP-01` through `08`; disposition `HDRD-01` through `12`; freeze exact standards/platform versions, output routes, target/white/metadata policy, semantic constants, ownership, state machine, hardware matrix, budgets, evidence, rights, and document revisions. No production code.

**Exit:** all `AC-HDRD-*` pass; fixed 1000/200, UINT10/FP16, and metadata choices are explicit; no implementation prompt has to choose platform or color policy.

```text
Execute only HDR Display Output Stage 0. Do not change production code. Inspect current Renderer/RHI/window/swapchain/interposer/settings/UI/capture/package paths; run all HDR discovery probes on named Windows/display/backend cells; freeze scene and target color semantics, UINT10 versus FP16 eligibility, peak/black and SDR-white policy, metadata role, native contracts, state transitions, budgets, fixtures, tolerances, and evidence lineage. Reconcile owning docs and stop on any unresolved active/fallback or color-domain choice.
```

## Stage 1 — Add Neutral Presentation State Without HDR Activation

Extend RHI presentation types/services/capabilities with the accepted neutral request/capability/result/output-generation model; add platform output association and bounded query facts; extend Renderer/settings/UI consumers to display truthful SDR-active/requested-HDR-unavailable state. Do not add HDR format, transform, or native color-space calls yet.

**Exit:** the existing SDR route is byte/behavior preserved; stale output generations reject; requested/support/eligible/active/fallback states are distinct; both backends and interposer expose coherent neutral facts; failure queries retain usable SDR.

```text
Execute only HDR Stage 1 from accepted HDRD-00 revisions. Add one backend-neutral presentation request/capability/result and output-generation contract at RHI, with platform output/Advanced Color/SDR-white facts and typed failures. Surface it through existing settings/Renderer/UI without activating HDR. Exercise SDR preservation, invalid/stale output identities, query failure, display change, two windows if admitted, D3D12/Vulkan/interposer cells, and bounded diagnostics. Do not add PQ, 10-bit/FP16 formats, metadata, or alternate state stores.
```

## Stage 2 — Activate And Recover The D3D12 Tuple

Add the accepted PixelFormat/DXGI tuple, swapchain interface checks, output/color-space capability, create/recreate/set order, optional metadata policy, active-result publication, and atomic SDR fallback. Reconcile the interposer/native swapchain interface path.

**Exit:** D3D12 never reports active without the complete current tuple; unsupported/set/recreate/metadata/output-change faults yield the accepted result and usable SDR; resize/display changes reapply state; native diagnostics are clean.

```text
Execute only HDR Stage 2 in RHI D3D12 presentation owners. Implement the accepted packed-10 or FP16 tuple, current-output association, CheckColorSpaceSupport/SetColorSpace1 lifecycle, accepted metadata policy, generation publication, interposer interface contract, and atomic SDR fallback. Inject unsupported display, stale output, interface, create/resize, color-space, metadata, minimize/restore, monitor-change, and device failures. Inspect native state/validation. Do not add Renderer tone/PQ logic or call DXGI outside RHI.
```

## Stage 3 — Activate And Recover The Vulkan Tuple

Enable the accepted instance/device extensions, enumerate/select the exact surface format/color-space pair, implement create/recreate/present and optional metadata policy, publish neutral active state, and preserve SDR fallback.

**Exit:** Vulkan never substitutes sRGB for requested HDR while reporting active; extension/tuple/call/recreate/present faults produce explicit fallback; current output generation and Windows display facts remain coherent; validation is clean.

```text
Execute only HDR Stage 3 in RHI Vulkan and shared Windows presentation owners. Implement the accepted extension gates, HDR surface format/color-space tuple, swapchain lifecycle, optional vkSetHdrMetadataEXT policy, result generation, and SDR fallback. Exercise missing extension/tuple, query/create/recreate/metadata/present failure, resize/minimize, monitor/OS change, suspend/resume, and device recovery with Vulkan validation. Keep color policy out of RHI and never report HDR active on an SRGB_NONLINEAR tuple.
```

## Stage 4 — Implement The Renderer HDR Transform

Implement accepted scene-to-target/gamut/tone/Rec.2020/PQ semantics and SDR UI mapping through existing display, frame-graph, shader-runtime, and UI packet owners. Provide independent CPU reference products and explicit raw stage identities. The transform may execute only for an immutable active HDR result in joined runs.

**Exit:** `HDR-MATH-*` matches high-precision ramps/patches/matrices; wrong constants/scale/gamut/double transform/UI white/alpha defects are detected; SDR path remains unchanged; D3D12/Vulkan shader outputs agree before native presentation.

```text
Execute only HDR Stage 4 from accepted Semantics. Add one Renderer-owned HDR target/gamut/PQ output path and accepted SDR-UI white mapping through existing settings/View/frame graph/shader/UI/capture owners. Compare an independent high-precision oracle over ramps, wedges, peaks, black, diffuse/SDR white, alpha, invalid values, and seeded transform defects. Keep native display APIs in RHI and preserve current SDR tone/encoding exactly. Stop on unnamed scene units or output domain.
```

## Stage 5 — Join Active State And Complete The Experience

Make Renderer choose SDR/HDR transform from the current immutable RHI active result; implement [User Experience](UserExperience.md), debug classification, capture labels, support record, settings persistence, and automation. Ensure each frame's output transform and swapchain tuple share one generation.

**Exit:** first use and every state are truthful; no mixed-generation frame publishes; UI is readable at accepted white; captures cannot be mistaken across domains; automation returns failure when required HDR does not activate.

```text
Execute only HDR Stage 5. Join the accepted RHI active-result generation to Renderer transform selection, then implement the existing-settings UI, state/reason vocabulary, debug/capture classification, bounded support record, persistence, and automation. Exercise enable/disable, unsupported/ineligible, activation pending/failure, two output generations, UI white, exact debug modes, every capture boundary, and require-active manifests. Do not let request alone select PQ or duplicate native state in Renderer.
```

## Stage 6 — Harden Transitions, Recovery, Backend Parity, And Package

Exercise and repair monitor move/straddle policy, OS HDR toggle, resize/fullscreen/windowed, minimize/restore, suspend/resume, hotplug/display change, device loss, interposer availability, backend switch/restart, package dependencies, and clean-machine first use. Measure transition latency/black frames, memory, and steady-state cost.

**Exit:** every transition reaches current HDR active or usable SDR fallback within frozen bounds; D3D12/Vulkan state semantics agree; no stale tuple, indefinite black output, leak, or package-only failure remains.

```text
Execute only HDR Stage 6 on the frozen transition/backend/package matrix. Inject every accepted output/OS/window/swapchain/metadata/device/interposer failure and transition, verify generation ordering and atomic SDR recovery, and measure black-frame/latency/memory/steady-state budgets. Run D3D12 and Vulkan native validation plus clean packaged first-use. Fix defects at their owner; do not hide a backend or transition with silent disable.
```

## Stage 7 — Earn Candidate Evidence And Close

Run the complete `CHK-HDR-*` protocol for one immutable candidate: raw numeric products, native tuple/state, output queries, metadata consistency if used, external HDR display measurements/images under frozen interpretation, SDR/HDR displays, both backends, UI, transitions, package, performance, capture lineage, excluded routes, and seeded defects. Remove temporary probes and submit `FCR-REN-26`.

**Exit:** all `AC-HDR-01` through `08` pass conjunctively; every `FM-HDR-*`/risk has detecting evidence; SDR remains proven; external/display limitations are explicit; acceptance owner records the verdict.

```text
Execute only HDR Stage 7 for one frozen candidate and hardware/software matrix. Run all CHK-HDR checks with seeded color/native/state/fallback/capture defects; retain raw scene/target/PQ values, native tuple/query results, transition artifacts, external display measurements, UI patches, backend/package/performance evidence, hashes, and limitations. Remove temporary test-only code, reconcile source/header/shader/generated/CMake/settings/package/docs, and file FCR-REN-26. Metadata success, screenshots, responsive presentation, or one backend cannot substitute for the full oracle.
```

## Completion Rule

The plan closes only when `FCR-REN-26` is accepted for one immutable candidate. A correct PQ shader without native activation is incomplete; a correct native tuple with the wrong Renderer transform is incomplete; metadata is never completion; and HDR failure without a usable truthful SDR fallback is a failed feature.

