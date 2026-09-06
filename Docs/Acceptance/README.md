# Acceptance

Status: acceptance navigation index

Acceptance orchestrates progress across features, workloads, and releases. It owns shared completion vocabulary, candidate-report structure, workload gates, release gates, and high-level status; it does not maintain a second feature-documentation tree.

The owning [Architecture](../Architecture/README.md) feature dossier defines each feature together with its local criteria, controlled failure modes, checks, and definition of done. Acceptance links to those definitions and records what a release candidate actually proved. It does not set priority, prescribe implementation design, or duplicate feature matrices.

## Orchestration And Progress Documents

| Contract | Responsibility |
| --- | --- |
| [First Release Acceptance](FirstRelease.md) | whole-release scope, consumer/source-adopter, packaging, security, failure, clean-machine, publication, and stabilization gates |
| [Feature Completion Reports](FeatureCompletionReports.md) | per-feature polish, complete-path explanation, evidence record, and approval schema |
| [Graphics Workloads](GraphicsWorkloads.md) | Bistro, San Miguel, scene-quality, performance, and evidence workloads |
| [Renderer Acceptance Progress](Renderer/README.md) | high-level Renderer feature-to-dossier/report routing and current acceptance disposition |

## Responsibility Split

| Question | Owning location |
| --- | --- |
| What is the feature, how does it work, and what exactly must it prove? | the feature dossier under [Architecture](../Architecture/README.md) |
| What shared words, evidence grades, and candidate-report fields apply? | [Feature Completion Reports](FeatureCompletionReports.md) and [Validation And Evidence](../Engineering/Verification/ValidationAndEvidence.md) |
| Which scenes and workload thresholds exercise several features together? | [Graphics Workloads](GraphicsWorkloads.md) |
| Is the complete release acceptable? | [First Release Acceptance](FirstRelease.md) |
| What did this exact candidate prove, fail, exclude, or leave blocked? | its `FCR-*` entry/report and retained evidence |

Do not add a feature-specific contract under `Docs/Acceptance`. Add it to the owning Architecture feature dossier, then link its stable IDs from the relevant report registry, workload, and release gate. Acceptance may summarize a disposition, but the summary must link to the feature owner and must not restate the detailed criteria or failure matrix.

Source presence, a successful build, a responsive process, and an uninspected capture are different evidence states. Use the exact completion vocabulary and retain commands, configuration, environment, artifacts, and limitations with every claim.

Before opening a candidate report, use [Product Workflow Coverage](../Architecture/CrossModule/ProductWorkflowCoverage.md) for actor-visible journeys, [Product Execution Traces](../Architecture/CrossModule/ProductExecutionTraces.md) for their ownership boundaries, and the [Capability Documentation Review](../Engineering/Workflow/CapabilityReview.md) to expose unanswered dossier fields. Those are source maps and review procedure, not acceptance evidence.

For every material iteration, start with the [Change Lifecycle control record](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record), select the persona `NS-*`/`PGE-*` targets and roadmap gate/risk, then use the owning feature dossier's binary criteria and controlled failures with checks designed under [Validation And Evidence](../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). Acceptance remains `Blocked` when any required link or artifact is missing.
