# Documentation Organization

Status: binding documentation organization and maintenance standard

Applies to: authored Markdown and documentation-owned images under `Docs`

## One Knowledge Owner

Every rule, decision, plan item, evidence gate, current-state fact, and historical claim MUST have one owning document. Other documents link to the owner and state only the context needed by their own responsibility. Repeated tables, phase lists, status summaries, and definitions are duplicate authority unless they are generated projections or clearly labeled snapshots.

Code and executable build configuration own implemented behavior. A document MUST NOT upgrade source presence into build, runtime, visual, performance, or release proof.

## Placement By Knowledge Type

| Area | Place here | Do not place here |
| --- | --- | --- |
| `Strategy` | desired capabilities, priority, roadmap, operating model, dated executive assessments | implementation rules or system design |
| `Architecture` | current maps, capability snapshots, accepted decisions, current/target system shape | phase execution, runbooks, or external studies |
| `Engineering` | binding standards, change/review procedure, accepted technical decision records, operational runbooks | product scope or acceptance gates |
| `Acceptance` | completion vocabulary, feature/release/workload gates, report schemas | architecture or priority |
| `Plans` | ordered delivery slices, dependencies, stop rules, migration and validation sequence | enduring decisions or completion claims |
| `Research` | external precedent, option studies, visual exploration, dated migration baselines | binding local policy or current-state authority |

When one subject needs independently maintained knowledge types, split it and cross-link the owners. Small rationale, delivery notes, or validation checklists may remain with an owning contract when they exist only to explain or verify that contract and have no independent lifecycle. Do not hide a standalone plan, runbook, or research report in an Architecture folder because it shares a subject with an architecture document.

## Placement By Module Ownership

Architecture MUST make the repository's durable module boundaries visible in its physical hierarchy:

- current module maps, capability snapshots, catalogs, and module-owned designs live under `Architecture/Modules/<repository-boundary>/<module>`;
- `Architecture/Modules/Engine`, `Tools`, and `Projects` mirror their repository boundaries; repository-wide build and packaging knowledge remains directly under `Architecture/Modules`;
- a document that primarily belongs to one module stays with that module even when it calls, configures, or consumes another module; use links to explain those relationships;
- `Architecture/CrossModule` is reserved for a system with several durable owners and no coherent primary module owner;
- a cross-module document MUST name the participating module owners and link to their module routes.

Do not create a topic folder that mixes Renderer, RHI, GameFramework, and tool documents merely because they participate in one feature. Prefer one primary owner. Use CrossModule only when the lifecycle and authority are genuinely shared.

Engineering uses a different axis because it owns implementation guidance rather than module maps:

- `Engineering/Workflow` owns how changes, reviews, and documentation work proceed;
- `Engineering/Foundations` owns conditional rules shared across modules;
- `Engineering/Modules` owns implementation rules for named module domains;
- `Engineering/Verification` owns check selection, evidence semantics, and operational validation;
- `Engineering/Decisions` preserves rationale and does not replace current rules.

The Engineering index MUST route by reader task and say why each document applies. A generic, flat `Standards` collection is not an acceptable substitute for this routing.

## Required Document Header

Every non-index document MUST make these facts obvious before its first substantive section:

- title;
- primary status/type;
- one-sentence responsibility or scope;
- authority boundary when a neighboring document could be mistaken for the owner;
- verification date and revision for current-state maps or snapshots;
- explicit non-claims when source inspection could be mistaken for executable evidence.

Use one primary type: **canonical decision**, **standard**, **current map**, **target architecture**, **capability snapshot**, **plan**, **acceptance contract**, **runbook**, **research**, **dated assessment**, or **archive**. “Summary” and “index” are navigation roles, not second authorities.

## Granularity

Split a document when sections have different owners, audiences, lifecycles, authority types, or independent reasons to change. Keep sections together when they enforce one invariant and normally change in the same review.

Length alone is not a split rule. A short file without an independent responsibility is fragmentation; a long file containing architecture, phase history, research, and acceptance is mixed authority. Prefer one cohesive owner over both extremes.

A subfolder MUST represent a durable subject, repository module, or document class. A module folder MAY contain only its `README.md` when it mirrors a durable source/build module and that page owns the module inventory; this consistency is intentional, not fragmentation. Other categorization folders need at least two owned documents. Every multi-document folder needs a short `README.md` if its contents are not completely routed by the parent index.

## Naming

- Use descriptive PascalCase filenames consistent with repository code and existing stable documents. A module inventory uses `README.md` inside its exact module folder so the path carries the module identity.
- Name the subject, not the author, date, state of mind, or editing action.
- Put delivery documents under `Plans`; do not use `ImplementationPlan` in an Architecture filename.
- Put historical dates and revisions in document metadata, not filenames, unless multiple retained snapshots require date identity.
- Avoid catch-all names such as `Misc`, `Notes`, `Ideas`, `New`, or `Final`. A deliberately exploratory document may use “Idea” only when its research status and promotion gate are explicit.
- Preserve stable A-L identifiers in titles where cross-document traceability still uses them; the filename should remain descriptive.

## Navigation And Links

The required route is `Docs/README.md` -> area index -> module or concern index -> owning document. Every non-index document MUST be reachable from its nearest index, and every index entry MUST state why a reader would open it. A reader MUST be able to distinguish module-owned and cross-module knowledge from the path alone.

Use relative Markdown links. Link directly to the owning section where practical. When a file moves or splits, update every producer and consumer in the same change and remove the old path; do not retain alias files, duplicate copies, or compatibility indexes.

An index routes. It SHOULD NOT reproduce large status tables, requirements, phase lists, or architecture descriptions from the documents it lists.

## Lifecycle

- Reconcile current maps and capability snapshots whenever their named owners, consumers, build membership, or public selection surface changes.
- Update architecture and its directly affected plans, acceptance gates, and indexes in one change when a contract moves.
- Delete superseded content when traceability has no active consumer.
- Move content to an archive only when provenance, legal traceability, or an active migration audit requires retention; archives are never default reviewer paths.
- Record completion in evidence or a current-state owner. Do not turn a plan into a permanent mixed plan/status diary.

## Review Checklist

Before handoff, verify:

1. one owner exists for every changed claim;
2. placement and primary status agree;
3. headings and filename describe the same responsibility;
4. the nearest index routes the document;
5. no old path, duplicate authority, or stale paraphrase remains;
6. all local links and linked anchors resolve;
7. current-state claims name their snapshot and evidence boundary;
8. UTF-8, whitespace, and `git diff --check` pass.
