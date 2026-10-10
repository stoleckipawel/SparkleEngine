# Graphics Workload Study Delivery Plan

**Status:** remaining discovery, analysis and study work; no calendar or capacity estimates

**Prerequisites:** `REL-11` before new production tooling; correct workload/reference prerequisites for `M3/WL-04`; [WS-D0](README.md#discovery-gate-ws-d0) before implementation. Existing release/capture repairs retain their own authorization; this plan cannot bypass the parent diagnostics `P1-GATE` to add internal collectors.

**Authority:** [workload-study contract](README.md), [Graphics Workloads](../../../../Acceptance/GraphicsWorkloads.md), [Diagnostics Acceptance](../Acceptance.md) and [Python dossier](../../../Modules/Tools/PythonAutomationAndAnalysis.md). Stage results belong to existing candidate/workload completion reports.

## Stage Contract

Apply [Change Lifecycle](../../../../Engineering/Workflow/ChangeLifecycle.md), [Change Integration](../../../../Engineering/Workflow/ChangeIntegration.md) and the [Feature Delivery Package](../../../../Engineering/Workflow/Templates/FeatureDeliveryPackage.md). Freeze prerequisite revisions, file allowlist, consumer/deletion/hook/copy budget, AC/FM/CHK and numeric rules before results. Before every implementation handoff enforce [Code Style](../../../../Engineering/Foundations/CodeStyle.md), applicable formatting/logical-spacing checks, narrow owner validation and `git diff --check`; boundary checks only for an actual Renderer/RHI boundary change. Remove disposable probes.

## Remaining Stages

| Stage | Work and exit | Deliberate scope and stop |
| --- | --- | --- |
| `WS-S0` Freeze discovery | Close five `WS-D0` cells, actual producer sample, statistics/observer/size cards and API/hardware matrix. | Documentation/local probe only; stop on missing producer, unowned schema or unfrozen thresholds. |
| `WS-S1` One analysis consumer | Implement only the selected offline CLI over frozen workload records; publish per-run tables/plots and source joins. Pass `AC-WS-01`, input/failure/cleanup parts of `AC-WS-04`, `AC-PY-01..04`. | No collector/export/history or Python framework. Delete the replaced ad hoc supported analysis route; original evidence stays intact. |
| `WS-S2` Causal paired-API studies | Complete three questions on matched D3D12/Vulkan tuples, including rejected/inconclusive evidence. Pass `AC-WS-02` and relevant `WL-04/07`; explain actual queue/barrier/shader/CPU constraints. | No forced async/serial topology change to manufacture a win, no broad feature additions. Stop on quality mismatch, candidate drift or observer contamination. |
| `WS-S3` Incident, adoption and publication | Deliver one difficult incident record, regression route and independent study reproduction; pass `AC-WS-03/04`, route into canonical case studies. | No invented influence/adoption evidence or unsupported hardware pass. Delete probes/replaced mechanisms; retain useful captures and negative conclusions. |

## Reference-Guided Study Selection

Consult the [what/where/why cards](../../../../Strategy/Research/RenderingReferenceExamples.md) during `WS-S0`, not while interpreting favorable candidate results. They supply hypotheses, not performance thresholds or completed studies.

| Question family | Reference | Measurable output and boundary |
| --- | --- | --- |
| CPU recording/binding/submission | `NVR-01/07/16`: state/lifetime and historical/current CAD comparisons. | One existing consumer, exact thread/queue order, CPU and whole-frame distributions, resource retirement and equivalent output. The deprecated sample is historical; preserve current parallel topology, no DGC requirement. |
| Shader code size/live state/traffic | `NVR-18/19`: source correlation and shipped ray-tracing investigation. | Exact optimized source/bytecode/native identity, observed limiter, one change and quality/memory/cost controls. Missing counter/source support is explicit; SER is optional, GPU Trace is distinct from frame capture. |
| Sampling/reconstruction/residency | `NVR-02/03/09/17`: guide/history, estimator and budget obligations. | One equivalent raw/filtered product and uncertainty/cost comparison, or one residency-pressure question. No algorithm replacement without its owning gate; heap budget is not allocated high-water. |
| Analyzer and transfer | `NVR-12/14/20`: a focused image utility and teaching route. | One immutable-record consumer and non-author exercise with feedback. No embedded viewer, Adobe integration or general profiling framework. |

Choose exactly three distinct causal questions for `WS-S2`, retaining the existing rejected/inconclusive-result obligation; they need not use three different families. Record the selected source, local differences, rights and falsifier through the [reference-use record](../../../../Strategy/Research/RenderingReferenceExamples.md#delivery-reference-use-record). References do not change `WS-D0`, workload prerequisite, sample or observer authority.

## Ready-To-Use Prompts

```text
Execute WS-S0 only in Docs/Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md. Research/planning only. Inspect GraphicsWorkloads, diagnostics comparison/data-spine status, external capture/runbook, Python dossier, real product launch and current installed tool/hardware environment. Close WS-D01..05 against an exact source revision and real producer sample; freeze units/identity/schema, missing-data policy, numeric comparison/observer rules, bounds and consumer/hook/copy/deletion ledger. Keep REL-11 and existing parent gates authoritative. No production code or guessed exporter. Validate docs/style/traceability and hand off WS-D0 or exact unresolved owners.

Select three distinct causal questions using this plan's reference-guided table. For each selected NVR card record exact source, local assumption/differences, rights, measurable output and falsifier before candidate results. Do not mandate a vendor feature or copy a sample architecture.
```

```text
Execute WS-S1 only after accepted WS-D0 and production admission. Extend the existing Tools owner with one selected offline analysis CLI and real consumer. Reuse workload schemas; preserve run-level distributions and candidate/frame/settings/native-artifact joins. Reject partial/mismatched/missing/oversized inputs, preserve originals and clean interrupted publication. Run frozen AC-WS-01/04 and AC-PY controls, code style and narrow owner checks. Delete replaced supported analysis paths and remove probes. Stop on unowned schema, lost identity or duplicate collector/export. No runtime Python or public framework. Handoff exact commands/results/limitations.
```

```text
Execute WS-S2 only after the accepted input/analyzer and workload/reference prerequisites. Run the three predeclared causal studies on matched Windows D3D12/Vulkan configurations using frozen quality/sample/uncertainty/practical-effect and observer controls. Keep one change per hypothesis, preserve production parallel topology, retain native before/after identity and one rejected/inconclusive alternative. Repair only a demonstrated bounded defect under the existing owner, with code style and affected narrow checks. Stop on quality/settings drift, invalid reference or unfrozen threshold. Handoff AC-WS-02 and relevant workload cells, no invented performance win.
```

```text
Execute WS-S3 only against accepted study artifacts. Close the one difficult incident from earliest wrong product through competing hypotheses, minimal fix/deletion and controlled regression; preserve exact source/shader/native identity. Obtain independent reproduction/review, record feedback and changes, and route report/plots/captures into the existing CASE/WL/completion owners. Run AC-WS-03/04 and applicable style/docs checks; remove probes. Stop on unproved root cause or absent adopter; keep valid individual results. No author self-certification or unrelated feature expansion.
```

Every prompt inherits this non-negotiable exit: one producer/schema/identity authority; source and native artifact joins; frozen statistical and observer policy; quality-equivalent comparisons; honest missing/inconclusive results; no duplicate live mechanisms; bounded failures and code style. Quote each clause with retained proof or `BLOCKED`. Research and documentation do not count as executed studies.
