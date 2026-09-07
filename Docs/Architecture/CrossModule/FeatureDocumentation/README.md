# Feature Documentation Coverage

**Status:** traceability index; this folder does not define feature behavior, sequencing, or release approval

**Owner:** Architecture documentation routing

This route answers whether a feature, target, workload, report family, or planned slice mentioned by the strategy/acceptance/planning corpus has an explicit Architecture owner. It prevents a strategy statement or acceptance row from becoming the only description of a feature.

| Audit | Question answered |
| --- | --- |
| [Source Document Coverage](SourceDocumentCoverage.md) | For every source document named in the refinement request, which feature topics occur and where are they documented? |
| [Stable Identifier Coverage](StableIdentifierCoverage.md) | Where does every `NS-*`, `PGE-*`, current `FCR-*`, `REL-*`, `RISK-REL-*`, `FM-REL-*`, `MAP-*`, `CASE-*`, and `WL-*` identifier route? |

## Rules

1. The owning Architecture dossier defines current capability, explicit absence, target boundary, ownership/lifetime/failure, and local acceptance.
2. Strategy owns why and priority; Plans own delivery sequence; Acceptance owns high-level progress and candidate results; Research owns source studies and alternatives.
3. This folder owns links only. It must not copy thresholds, results, or feature contracts from their authoritative documents.
4. “Covered” means an explicit owner exists. It does not mean implemented, verified, included, or accepted.
5. A newly mentioned public feature or independently selectable mode must enter both ledgers or receive an explicit not-applicable/excluded owner before the source document is complete.
