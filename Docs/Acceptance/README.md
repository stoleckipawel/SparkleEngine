# Acceptance

Status: acceptance navigation index

Acceptance defines what must be demonstrated before a feature, workload, or release claim is accepted. Contracts are grouped by their primary architecture scope; this area does not set priority or prescribe implementation design.

## Release-Wide Contracts

| Contract | Responsibility |
| --- | --- |
| [First Release Acceptance](FirstRelease.md) | whole-release scope, consumer/source-adopter, packaging, security, failure, clean-machine, publication, and stabilization gates |
| [Feature Completion Reports](FeatureCompletionReports.md) | per-feature polish, complete-path explanation, evidence record, and approval schema |
| [Graphics Workloads](GraphicsWorkloads.md) | Bistro, San Miguel, scene-quality, performance, and evidence workloads |

## Feature Contract Groups

| Scope | Open it when... |
| --- | --- |
| [Renderer](Renderer/README.md) | the accepted behavior is primarily owned by Renderer |
| [CrossModule](CrossModule/README.md) | the accepted behavior must be proven across several module owners |

Source presence, a successful build, a responsive process, and an uninspected capture are different evidence states. Use the exact completion vocabulary and retain commands, configuration, environment, artifacts, and limitations with every claim.

For every material iteration, start with the [Change Lifecycle control record](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record), select the persona `NS-*`/`PGE-*` targets and roadmap gate/risk, then bind binary acceptance criteria and controlled failure modes to checks designed under [Validation And Evidence](../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). Acceptance remains `Blocked` when any required link or artifact is missing.
