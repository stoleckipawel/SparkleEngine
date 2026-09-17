# Stage 9 Source Evidence

**Status:** **IMPLEMENTED / VALIDATION DEFERRED** against source input `e783f2cfe2e6be205eeedd24ca7caf2206bc9047` plus the scoped Stage-9 working tree on 2026-09-17. This is a source handoff, not an evidence-candidate `PASS`, accepted Reference authority, build proof, GPU proof, artifact proof, backend parity, or Stage-10 authorization.

## Delivered Source Route

The original frame remains the only producer of radiance. The Reference middle publishes its committed mean and M2 as ordinary private frame products; `ViewportCaptureService` resolves both through the existing generic asynchronous RHI texture-readback service. The public capture result snapshots only generic immutable-product provenance: SHA-256 session identity, committed prefix, and target. No Reference-specific RHI method, capture pass, AOV, counter stream, event stream, or detached renderer was added.

`ApplicationEditor` owns the rest of the workflow under `Private/Editor/ReferencePathTracer`:

- `ReferencePathTracerArtifactCoordinator` binds complete and save-when-complete actions to one session identity, turns partial/checkpoint actions into pause-and-settle intents, and starts generic readback only after the existing session reports a stable paused or completed prefix;
- `ReferencePathTracerArtifactEncoding` owns readback decoding plus the FLOAT RGB ZIP EXR, frozen little-endian mean/M2/count checkpoint, and canonical manifest schemas; `ReferencePathTracerArtifactWriter` owns only budget/free-space policy, verified byte persistence, manifest-last ordering, cleanup, and the same-volume atomic publication transaction;
- output stays under an explicit writable root, never overwrites an existing publication, refuses insufficient disk/budget, rejects a readback whose exact provenance changed after intent capture, rejects cross-digest checkpoint planes, removes only staging created by the current transaction, responds to task cancellation between encoding/publication stages, and leaves the live GPU prefix plus earlier or foreign publications untouched;
- the Editor's collapsed `Evidence / Output` surface issues artifact intent only; it does not own Renderer state or filesystem/codec policy;
- `ShowcaseEditor.exe --reference-path-tracer-request <request.json>` parses one typed flat request exactly once, resolves project/level/output policy before runtime allocation, creates a hidden Game-kind ordinary viewport request, enters the same `FramePipeline` and Reference middle, emits bounded JSON Lines progress plus exactly one terminal record, and uses the same capture/coordinator/writer route.

The refinement audit removed host/UI identity from the transport digest: the View component now contains the transport-relevant render extent, while `viewportId` remains solely in the session's ownership/action routing. Editor `Scene` and offscreen/runtime `Game` producers therefore do not fork digest or sample identity merely because their host or viewport owner differs. Shared SHA-256 hex formatting and root-containment policy remain in Core; no feature-prefixed copy of either mechanism remains.

The noninteractive source currently accepts the canonical implemented product cell: `SurfaceTransportReference`, `ActiveGameCamera`, Box filter, full extent, target `4096` SPP, seed/replicate `0`, the selected process backend, and optional timeout checkpointing. Other frozen request-schema cells remain explicit `InvalidRequest` until their owning production inputs exist; they are not silently approximated.

## Enclosure And Integration-Hook Audit

| Touched owner | Classification and retained reason |
| --- | --- |
| Renderer `Passes/Lighting/ReferencePathTracer` | Feature home: publish committed mean/M2 handles and immutable transport provenance; host and filesystem policy remain absent. |
| Renderer viewport contracts/publication/capture | New justified shared hook: generic raw products and immutable provenance travel through the already owned product/readback boundary. No Reference type, branch, codec, or RHI API was added there. |
| `FramePipeline` and frame product roots | Existing composition hook: project the feature's two raw resources and their generic provenance; no artifact workflow or alternate frame owner enters the pipeline. |
| ApplicationEditor `Private/Editor/ReferencePathTracer` | Artifact/offscreen feature home: request parsing, prefix coordination, encoding schemas, publication transaction, and offscreen loop are private and independently named by responsibility. |
| `EditorOperationService`, `EditorApplication`, and existing capture coordinator | Existing composition/lifetime hooks: launch one blocking-I/O operation, route generic completed readbacks to their owning coordinator, and invoke the feature owner. They do not implement EXR/checkpoint/path policy. |
| Editor Reference overlay/output plus `ViewportPanel`/`UI` | Feature UI home plus the minimum existing intent-forwarding hooks. Renderer truth is read-only and filesystem policy is absent. |
| Core SHA-256 and path containment | Shared capability repair with current non-Reference consumers: byte-digest formatting no longer has three feature-local copies, and component-aware containment replaces the ambiguous string-prefix implementation already used by project/cooking code. |
| Platform/Runtime window options | New justified generic hook with a current offscreen consumer: explicit initial extent/visibility only; no Reference semantic enters Platform. |
| `Showcase` entry, Application CMake, dependency CMake | One host entry/composition hook and build/dependency membership. TinyEXR/zlib remain private to non-Shipping ApplicationEditor linkage. |
| Feature documentation and this report | Documentation/evidence only; no source-presence row is treated as runtime or release proof. |

## TinyEXR Decision

| Concern | Reviewed result |
| --- | --- |
| Identity | Existing FetchContent dependency `v1.0.7`, now pinned to release commit `6e8cac308cdf4d717078f3f37c4aa39bf3b356b4`. The upstream release labels this revision a security fix. |
| Required API | Upstream v1 documents `SaveEXRImageToMemory`, multi-channel FLOAT output, ZIP compression, caller-owned error release, and one-translation-unit `TINYEXR_IMPLEMENTATION`. Sparkle encodes to memory and writes with `std::filesystem`, preserving Unicode path handling rather than using TinyEXR's narrow filename API. |
| Compression dependency | `TINYEXR_USE_MINIZ=0`; the existing pinned zlib target supplies the documented zlib-compatible API. No second miniz implementation enters `SparkleApplicationEditor`. |
| Rights | TinyEXR is BSD-3-Clause. Sparkle copies no source or asset into the repository; fetched-source license handling remains a build/package responsibility. |
| Security boundary | The writer consumes engine-owned typed GPU bytes only; it does not decode untrusted EXR. Dimensions, row pitch, byte extent, output containment, budget, free space, staging, persisted checkpoint hash, and publication collision are checked at the ApplicationEditor boundary. |
| Build/package | TinyEXR and zlib are private `SparkleApplicationEditor` links only in non-Shipping configurations. Renderer and RHI do not link the codec. Shipping compilation retains only disabled writer/entry code; final compiled-out/package proof remains Stage 10. |

Primary sources: [TinyEXR v1.0.7 release](https://github.com/syoyo/tinyexr/releases/tag/v1.0.7), [TinyEXR v1 API and license](https://github.com/syoyo/tinyexr/blob/6e8cac308cdf4d717078f3f37c4aa39bf3b356b4/README.md), and [`SaveEXRImageToMemory` declaration](https://github.com/syoyo/tinyexr/blob/6e8cac308cdf4d717078f3f37c4aa39bf3b356b4/tinyexr.h).

## Checks Actually Run

| Check | Result |
| --- | --- |
| Exact Stage-9 prompt versus live `Plan.md` | Matched before edits. |
| `git ls-remote https://github.com/syoyo/tinyexr.git refs/tags/v1.0.7^{}` | Resolved the release tag to exact commit `6e8cac308cdf4d717078f3f37c4aa39bf3b356b4`, now used by FetchContent. |
| Scoped source/ownership inspection | Completed for frame products, Reference committed resources, generic Renderer/RHI capture, ApplicationEditor operation ownership, Editor UI, Showcase launch, build membership, hashes, and dependency pin. The refinement pass separated manifest interpretation from execution and encoding/schema from filesystem publication, reused shared Core path/hash operations, and found no new RHI or detached-renderer owner. |
| Digest/provenance race inspection | Corrected the View digest component to exclude viewport/host identity, retained viewport identity only for session ownership/actions, and bound every accepted readback to the exact requested digest/prefix/target before publication. This is source inspection, not viewport/offscreen runtime equivalence evidence. |
| Transaction failure inspection | Corrected foreign-staging cleanup ownership, added cooperative cancellation checkpoints, and enforced predicted plus exact staging/final/10% free-space checks before publication. Disk-full, cancellation, and corruption execution remain deferred. |
| Changed-file clang-format 22.1.3 write plus dry run | Passed for all 44 changed and new C/C++ files in the scoped working tree. |
| Focused semantic style scan | Passed for the same 44 files: no anonymous namespace or namespace-end-comment violation. |
| Repository-wide `CMake/CodeStyle.ps1 -Mode Check -SourceFamily Cpp` | **Not passed:** the check found existing formatting drift in unrelated tracked Renderer and tool files. No reported violation named a Stage-9 changed or new file; the focused changed-file check above passed. |
| `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` | Passed after the final ownership/refinement edits with no new violation. |
| `git diff --check` | Passed; Git reported only line-ending conversion warnings for existing CMake/document files. |
| Strict UTF-8 and local Markdown link-target check over changed feature documents | Passed. |

## Deferred Evidence And Decision

No C++ build, shader cook, Editor launch, GPU capture, EXR decoder round trip, corrupt-checkpoint injection, disk/access failure, D3D12/Vulkan run, native validation, frontend comparison, camera navigation, external renderer comparison, statistical study, or `CHK-RPT-03` through `CHK-RPT-13` matrix executed. The user-owned no-build/manual-run policy is retained.

| Stage-9 trace group | Owning checks | Exact disposition |
| --- | --- | --- |
| Camera, hit, material, BSDF, light, estimator, roulette, sampler, robustness, and accumulation claims: `AC-RPT-03`, `05` through `13`; `FM-RPT-02` through `12` | `CHK-RPT-03` through `09`, `11` | **VALIDATION DEFERRED**; no analytic, injected-defect, external, replicate, or GPU result exists. |
| Raw lineage, invalid-result, and atomic-publication claims: `AC-RPT-14`, `15`; `FM-RPT-03`, `06`, `10`, `15` | `CHK-RPT-09`, `10` | Source route implemented; **VALIDATION DEFERRED** for exact bytes, EXR decode, provenance mutation, forbidden-input rejection, and interrupted publication. |
| Independent-oracle and statistical claims: `AC-RPT-16`; `FM-RPT-02`, `11`, `12` | `CHK-RPT-06`, `07`, `11` | **VALIDATION DEFERRED**; no interchange renderer or statistical run occurred. |
| Backend/frontend claims: `AC-RPT-17`; `FM-RPT-07`, `13` | `CHK-RPT-07`, `12` | Source route unchanged/reused; **VALIDATION DEFERRED** for native validation and paired results. |
| Failure/resource/workflow claims: `AC-RPT-02`, `18`; `FM-RPT-14`, `15`, `17` | `CHK-RPT-13`, `15` | Bounded source categories and staging policy implemented; **VALIDATION DEFERRED** for real recovery, accessibility, clean-machine, and support behavior. |
| Scope, ownership, enclosure, frame-route, and shared-core claims: `AC-RPT-01`, `04`, `20` through `23`; `FM-RPT-01`, `03`, `10`, `18`, `20` through `22` | `CHK-RPT-01`, `02`, `16` through `20` | Focused source and architecture-boundary inspection passed only for the Stage-9 diff; executable topology, host equivalence, package, compiled-consumer, and pinned-precedent evidence remains deferred. |
| Release-map truth: `AC-RPT-19`; `FM-RPT-16`, `19` | `CHK-RPT-14` | **STAGE 10 / NOT RUN**; Stage 9 makes no release-map claim. |

Therefore Stage 9 is **IMPLEMENTED / VALIDATION DEFERRED**, but it has **not** produced the evidence candidate required by its acceptance gate. All `CHK-RPT-03` through `CHK-RPT-13`, `CHK-RPT-18` through `CHK-RPT-20`, workflow failure/recovery cells, external comparisons, and paired backend/frontend results remain backlog with zero readiness credit. Stage 10 is **not authorized** until those exact matrices execute and pass and any source-limited request cells required by the accepted candidate are closed.
