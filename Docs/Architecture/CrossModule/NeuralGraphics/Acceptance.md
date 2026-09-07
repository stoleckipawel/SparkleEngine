# Neural Graphics Acceptance

**Status:** feature-local target acceptance contract; no criterion has a candidate result

**Scope:** define the binary criteria, controlled failures, checks, and completion boundary for a future owned neural-graphics feature

**Owner:** Neural Graphics feature family; [First Release](../../../Acceptance/FirstRelease.md) owns release disposition and [Feature Completion Reports](../../../Acceptance/FeatureCompletionReports.md) owns candidate results

## Binary Criteria

| ID | Criterion |
| --- | --- |
| `AC-NG-01` | One bounded user-visible graphics problem, tensor/data contract, output domain, supported matrix, classical fallback, and non-goals are frozen. |
| `AC-NG-02` | Licensed/provenanced train/validation/test data, leakage controls, deterministic generation/preprocessing, and immutable test identity are complete. |
| `AC-NG-03` | Owned model/operator, training recipe, seeds/configuration, checkpoints, metrics, and independent reference comparisons reproduce within declared tolerance. |
| `AC-NG-04` | Model export/lowering, operator/kernel decisions, precision/layout, binding ABI, artifact identity, and validation are inspectable and deterministic. |
| `AC-NG-05` | Renderer/RHI execution declares every resource, dependency, synchronization, capability, generation, history, fallback, and retirement boundary. |
| `AC-NG-06` | Frozen Bistro/San Miguel or approved replacement workloads report quality, temporal behavior, CPU/GPU p50/p95/p99, memory/workspace, artifact size, and classical comparison. |
| `AC-NG-07` | Missing/corrupt/incompatible artifact, unsupported hardware/backend/operator, allocation/compile/dispatch failure, non-finite output, reload, and shutdown remain honest and bounded. |
| `AC-NG-08` | Package manifests account for model/kernel/runtime bytes, licenses, hashes, compatibility, and source/debug erasure; a non-author can reproduce the supported result. |

## Controlled Failures

| ID | Fault and required outcome |
| --- | --- |
| `FM-NG-01` | Dataset provenance/license/split integrity fails -> training/evaluation publication is blocked. |
| `FM-NG-02` | Model schema/operator/shape/precision is unsupported -> lowering fails with exact identity and no stale replacement. |
| `FM-NG-03` | Kernel compilation, validation, capability, allocation, or dispatch fails -> requested/active state reports failure or declared classical fallback. |
| `FM-NG-04` | Output is non-finite, outside semantic bounds, temporally unstable, or below the frozen quality threshold -> candidate fails regardless of speed. |
| `FM-NG-05` | Runtime/model/kernel generations mismatch or retire early -> execution is rejected and the previous accepted generation remains valid. |
| `FM-NG-06` | Required artifact/license/hash is absent from the package -> verification and startup fail before presenting false support. |

## Checks

| ID | Covers | Smallest required check |
| --- | --- | --- |
| `CHK-NG-01` | `AC-NG-01..03`, `FM-NG-01` | Rebuild a bounded dataset/model twice from frozen inputs; compare splits, artifacts, metrics, provenance, and reference outputs. |
| `CHK-NG-02` | `AC-NG-04..05`, `FM-NG-02..03`, `FM-NG-05` | Reverse-trace one model to kernels and GPU commands, validate numerical equivalence, then inject schema/capability/compile/allocation/generation failures. |
| `CHK-NG-03` | `AC-NG-06..07`, `FM-NG-04` | Run the frozen quality/temporal/performance/memory matrix against the classical fallback and perturb fault-sensitive inputs. |
| `CHK-NG-04` | `AC-NG-08`, `FM-NG-06` | Verify package manifest/licenses/hashes and repeat the supported and missing-artifact journeys from clean bytes. |

Documentation and source inspection establish scope only. No `AC-NG-*`, `FM-NG-*`, or `CHK-NG-*` result is passed here. The deferred [`NG-E01..04`](../../../Plans/CapabilityEvidence.md#neural-graphics-evidence) sequence owns the central proof destinations after roadmap admission.
