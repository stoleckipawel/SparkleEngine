# Graphics Portfolio Delivery Plan

**Status:** planning scaffold; five evidence/audience decisions still need to be filled

**Scope:** assemble existing evidence into three case studies and an independent reproduction/review exercise

Start by choosing the cases and reader. Then select the original artifacts, verify sharing rights, and prepare the smallest useful reproduction. Final delivery waits for those cases' accepted evidence; earlier planning can prepare the route under [Roadmap M0–M6](../../../Strategy/Roadmap.md#retained-advanced-graphics-roadmap).

The [portfolio dossier](README.md) defines the result. The [catalog](../../../Strategy/FeatureDeliveryCatalog.md#three-headline-cases) gives examples; [Graphics Workloads](../../../Acceptance/GraphicsWorkloads.md) owns case/workload criteria and reports.

## Fill Before Execution

| Decision | What still needs to be chosen |
| --- | --- |
| **Cases and reader** | Portfolio author: exact case IDs, question, intended reviewer and your personal contribution. Use the three established narratives. |
| **Original evidence** | Evidence owner: candidate/artifact locations and hashes, validity, and the shortest retrieval route. Reuse existing release/study/model manifests. |
| **Shareable subset** | Release/content owner: cleared assets/artifacts and distribution channel; identify private symbols/dumps and other material to withhold. Publication needs its own authorization. |
| **Reproduction** | Author/reviewer: machine/tool/API prerequisites, frozen inputs, expected result, mismatch/absence controls and a bounded exercise. Reuse existing adoption criteria. |
| **Independent review** | Author: actual non-author participant, review question and how findings will be recorded. This remains a real external-evidence requirement. |

All five decisions are open. The first step closes them; plan presence does not close them.

## Remaining Stages

### 1. Freeze The Case And Evidence Route

**Start when:** the current owners and selected Roadmap objective are known.

Fill the decisions above, identify missing accepted artifacts and assign each gap to its owner. Freeze the case manifest and retrieval/reproduction failure checks. This step changes documentation only.

**Exit:** a reviewable local assembly plan with exact inputs, rights and check cards. Required unresolved decisions remain visible.

### 2. Assemble The Three Cases

**Start when:** the selected evidence and sharing rights are accepted.

For each case, explain the question, your change or derivation, the relevant rendering product, source/native/model lineage, measurements, negative result and limitations. Add exact reproduction commands. Validate links and hashes, reject mixed candidates and preserve the originals.

**Exit:** three inspectable case packages built from the accepted evidence, with missing cells explicitly identified.

### 3. Reproduce, Review And Revise

**Start when:** the case package is frozen and an independent participant is available.

Execute the existing workload/adoption checks (`WL-07/08`), collect findings and make the justified repairs. Record results in their original reports and update public claims to match that proof. Remove assembly probes and completed instructions.

**Exit:** retained independent reproduction/review and a disposition for every finding, under the existing proof owners.

## First Permitted Prompt

```text
Execute only step 1 of GraphicsPortfolio/Plan.md.
Use Roadmap to select the objective and inspect the existing case/workload and
release/study/neural reports. Fill all five decisions: cases/reader, original
evidence, shareable subset, reproduction and independent review. Link original
artifacts and assign gaps to their existing owners. Handoff a reviewable local
plan with exact inputs and checks. Do not fabricate results, create a new
collector/schema, publish material or contact anyone.
```

For any implementation repair, follow [Change Lifecycle](../../../Engineering/Workflow/ChangeLifecycle.md), [Change Integration](../../../Engineering/Workflow/ChangeIntegration.md) and [Code Style](../../../Engineering/Foundations/CodeStyle.md). Documentation-only assembly checks links/anchors, UTF-8, whitespace and `git diff --check`; broader builds need a changed executable claim.
