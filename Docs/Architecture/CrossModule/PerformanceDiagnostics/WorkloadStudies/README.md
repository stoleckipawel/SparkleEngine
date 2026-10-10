# Graphics Workload Studies

**Status:** target workflow and discovery contract; no implemented analysis tool or measured study is claimed

**Scope:** the `PGD-03..05` [strategy outputs](../../../../Strategy/FeatureDeliveryCatalog.md): paired-API causal studies, one difficult incident and one useful offline analysis consumer. [Graphics Workloads](../../../../Acceptance/GraphicsWorkloads.md) owns `MAP/WL/CASE` definitions and benchmark schema/policy; [Diagnostics Acceptance](../Acceptance.md) owns comparison and observer semantics. This folder owns only the bounded study workflow and its [remaining plan](Plan.md).

## Intended Consumer And Boundary

A rendering engineer gives a narrow analysis CLI immutable workload records and referenced native captures, asks one predeclared comparison question, and receives inspectable per-run tables/plots plus a decision record. The CLI never collects live engine samples, owns capture-provider state, controls GPU timing or silently merges incompatible identities.

Application remains the live diagnostics/serialization owner under the [diagnostics plan](../Plan.md). Workloads own experiment intent, schema, sample policy and destinations. Tools owns the offline parser/analysis implementation under [Python Automation And Analysis](../../../Modules/Tools/PythonAutomationAndAnalysis.md). External tools remain under the [profiling runbook](../../../../Engineering/Verification/ExternalProfiling.md). Do not create another exporter, history ring, collector or universal capture command.

## Discovery Gate `WS-D0`

| Cell | Owner and closure evidence |
| --- | --- |
| `WS-D01` Actual consumer and producer | Workload/Application owners: inspect current launch/capture/export/request routes and select one comparison. Name available fields and missing ones; do not presume the internal data spine exists. |
| `WS-D02` Input identity and schema | Workload/Tools owners: freeze schema, units, run/frame/scene/settings/candidate join, native artifact namespaces, source/symbol provenance, null/partial/unsupported behavior and size bounds. Reuse the workload schema rather than copying it. |
| `WS-D03` Statistical and observer policy | Workload owner: inherit warm-up/sample/repeat rules; freeze practical-effect bands and correlation-aware uncertainty, independent run treatment, exclusion rules, quality equivalence and observation mode. A profiling capture is not an optimized baseline. |
| `WS-D04` Tool and publication | Tools owner: supported Python/dependencies, CLI/path/root/exit behavior, deterministic output identity, cancellation, partial cleanup, memory/input bounds and consumer acceptance. No runtime bindings or tool framework. |
| `WS-D05` Study and architecture budget | Renderer/Tools owners: predeclare three distinct causal questions, including rejected/inconclusive evidence, and one incident; freeze hook/copy/deletion ledger, negatives, supported tuples and independent review. |

`WS-D0` passes only with a reviewed source/dossier revision, a valid producer sample, numeric decision cards and the exact narrow consumer. Missing producer data is an explicit prerequisite with an owning plan stage. Use existing external-native data for an honest narrow study where sufficient; do not implement the internal diagnostics plan merely to fill a portfolio table.

## Local Acceptance And Failures

| ID | Observable criterion and check |
| --- | --- |
| `AC-WS-01` / `CHK-WS-01` | Valid frozen inputs reproduce per-run tables/plots and source joins on repeat; mismatched API/settings/schema/symbol or partial records visibly reject, never synthesize zero metrics. |
| `AC-WS-02` / `CHK-WS-02` | Three predeclared studies report baseline/candidate quality, practical effect, uncertainty, CPU/GPU/memory and whole-frame context; at least one rejected/inconclusive result is retained. Reanalyze a null/noise comparison and changed-setting negative. |
| `AC-WS-03` / `CHK-WS-03` | One incident has a reproducible symptom, earliest incorrect product, competing/refuted hypotheses, bounded repair and a defect-detecting negative/regression route. Successful launch or an attractive image alone fails the check. |
| `AC-WS-04` / `CHK-WS-04` | An independent user can follow the study instructions and trace headline results to original artifacts; interrupted/oversized/missing inputs produce bounded failure and cleanup. |

`FM-WS-01` identity/schema mismatch maps to `CHK-WS-01`; `FM-WS-02` changed settings, incomplete observer or invalid statistics maps to `CHK-WS-02`; `FM-WS-03` an unproved incident root cause maps to `CHK-WS-03`; `FM-WS-04` partial publication or missing adopter maps to `CHK-WS-04`. Failures produce an explicit rejection/inconclusive/unrun disposition and preserve original inputs. A missing counter cannot be recast as zero or supported.

Primary performance precedent is recorded in the [role study](../../../../Strategy/Research/PrincipalGraphicsRoles.md#technical-evidence-and-transfer-limits). All thresholds and actual results remain with their existing acceptance owners; this page does not pass `WL-04` or advance `PY-03`.
