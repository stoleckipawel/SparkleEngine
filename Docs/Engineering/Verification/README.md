# Engineering Verification

**Status:** verification index

Use these documents when selecting checks, interpreting results, measuring performance, or retaining evidence.

## Quick Choice

| Claim | First route |
| --- | --- |
| source/ownership/link/build membership | focused static or configuration check from Validation And Evidence |
| build or tool integration | smallest target/profile that exercises the changed owner |
| runtime behavior or failure/recovery | narrow executable path with a predeclared oracle and cleanup |
| GPU/API correctness | applicable backend run plus native validation and attributable capture |
| performance cause | reproducible workload, bounded observations, then external profiling |
| release/package behavior | frozen candidate matrix and retained acceptance artifacts |

Always name what the check can falsify. A wider command is not stronger evidence when its oracle does not observe the changed claim.

## Verification Documents

| Document | Read it when... |
| --- | --- |
| [Validation And Evidence](ValidationAndEvidence.md) | mapping a claim or failure mode to the cheapest check that can falsify it, then reporting the exact result |
| [External Profiling](ExternalProfiling.md) | preparing and running an external GPU/performance capture |

Feature-local pass criteria belong with the owning [Architecture](../../Architecture/README.md) feature dossier; candidate results and workload/release gates belong in [Acceptance](../../Acceptance/README.md). This folder owns how evidence is designed and gathered.
