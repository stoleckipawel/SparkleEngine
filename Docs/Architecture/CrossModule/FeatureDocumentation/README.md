# Feature Documentation Coverage

**Status:** traceability index; this folder does not define feature behavior, sequencing, or release approval

**Owner:** Architecture documentation routing

This route answers whether a feature, target, workload, report family, or planned slice mentioned by the strategy/acceptance/planning corpus has an explicit Architecture owner. It prevents a strategy statement or acceptance row from becoming the only description of a feature.

## Coverage Versus Quality

| Question | This index can answer | Owning feature must answer |
| --- | --- | --- |
| Is the topic routed? | yes: source mention or stable identifier reaches one Architecture owner | why it exists and what observable result it promises |
| Is current absence visible? | yes: a negative/target dossier can own the gap | which adjacent behavior is not a substitute and what ownership would be required |
| Is the feature understandable? | no: a link alone is only coverage | request/inputs -> owner/lifecycle -> result, matrices, tradeoffs, failures, and evidence boundary |
| Is it implemented or accepted? | no | code/build membership and candidate-bound executable evidence |

Coverage is necessary but not sufficient. A routed feature still fails the documentation-quality bar when its owning page is only an ID list or source dump.

| Audit | Question answered |
| --- | --- |
| [Source Document Coverage](SourceDocumentCoverage.md) | For every source document named in the refinement request, which feature topics occur and where are they documented? |
| [Stable Identifier Coverage](StableIdentifierCoverage.md) | Where does every `NS-*`, `PGE-*`, current `FCR-*`, `REL-*`, `RISK-REL-*`, `FM-REL-*`, `MAP-*`, `CASE-*`, and `WL-*` identifier route? |
| [Documentation Presentation Research](PresentationResearch.md) | Which external information-architecture and presentation precedents informed the reader-first documentation structure? |
| [Renderer And RHI Coverage Research](GraphicsCoverageResearch.md) | What must Renderer/RHI documentation expose before a graphics change can be understood or reviewed? |

## Rules

1. The owning Architecture dossier defines current capability, explicit absence, target boundary, ownership/lifetime/failure, and local acceptance.
2. Strategy owns why and priority; the plan colocated with an Architecture owner owns delivery sequence; Acceptance owns high-level progress and candidate results; colocated research owns source studies and alternatives.
3. This folder owns links only. It must not copy thresholds, results, or feature contracts from their authoritative documents.
4. “Covered” means an explicit owner exists. It does not mean implemented, verified, included, or accepted.
5. A newly mentioned public feature or independently selectable mode must enter both ledgers or receive an explicit not-applicable/excluded owner before the source document is complete.
