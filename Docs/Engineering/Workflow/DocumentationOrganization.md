# Documentation Organization

**Status:** binding documentation organization and maintenance standard

**Applies to:** authored Markdown and documentation-owned images under `Docs`

## Reader-First Page Contract

Documentation MUST first help a reader form a useful mental model. Authority, evidence, and exact ledgers remain essential, but they MUST NOT bury the answer to “what is this, what do we have, what is missing, and why was it designed this way?”

A feature, system, or module overview follows this reading order unless a shorter page does not need every section:

1. **Plain-language purpose** — one or two sentences naming the result and intended reader.
2. **At a glance** — current state, projected 0–100 readiness when the page owns or describes a feature, supported path, important missing capability, and evidence boundary visible without scrolling through a ledger.
3. **How it fits** — a small diagram showing owners, data flow, or execution order when three or more relationships matter.
4. **How to use or select it** — the public executable, API, setting, CVar, editor control, or explicit statement that no user route exists.
5. **How it works** — the shortest end-to-end explanation before class, method, and capability-ID detail.
6. **Supported and unsupported matrix** — compare modes, backends, content types, or product profiles when behavior differs.
7. **Design decisions and tradeoffs** — state the chosen approach, why it exists, its benefit, and its real cost or rejected alternative.
8. **Limitations, failures, and troubleshooting** — expected gaps and diagnostic/recovery routes in reader language.
9. **Evidence and reference** — exact capability/acceptance/check IDs, source paths, snapshots, and neighboring authorities.

This is progressive disclosure: the first screen provides orientation, middle sections explain the system, and bottom sections serve implementation and review. Do not begin a reader-facing page with a multi-page inventory, source-directory audit, acceptance ledger, or history unless that exact ledger is the document’s sole purpose.

### Feature Depth Test

Section names alone do not satisfy the page contract. A feature page is deep enough only when a reader can follow one concrete request or input to its observable result and answer:

- who owns the request, mutable state, produced data, and final decision;
- which selector or caller makes the route reachable and how requested state becomes active state;
- what crosses each module, thread, frame, process, CPU/GPU, or source/runtime boundary;
- what identity, generation, completion, reset, cancellation, capacity, and retirement mean;
- how modes, backends, content classes, and unavailable combinations differ;
- which design choice was made, what it buys, and what cost or alternative it accepts;
- what failure looks like, what remains valid, and whether recovery, retry, fallback, refusal, or restart occurs;
- what source presence proves and which build, runtime, visual, performance, package, adoption, or release claims remain unproved.

Do not fill missing depth with generic headings or repeat the same overview in every child. The family page owns shared flow and invariants; a leaf explains the behavior and proof boundary that is unique to that capability. A long ledger without an end-to-end mental model is incomplete, as is a polished overview without exact limitations and evidence.

Use the [Documentation Page Template](Templates/Page.md) when creating or materially restructuring one page. Use the [Feature Delivery Documentation Package](Templates/FeatureDeliveryPackage.md) when a substantial feature needs independently maintained research, discovery, semantic, architecture, experience, acceptance, and staged-delivery authorities.

## Language And Presentation

- Lead with the observable result, then name the implementation owner. Prefer “The Renderer turns an immutable world snapshot into a presented frame” over “Renderer owns frame policy.”
- Use present tense and active voice for current behavior. Use “target,” “planned,” or “not implemented” explicitly for future behavior.
- Keep paragraphs focused on one idea. Split dense sentences that combine ownership, lifetime, failure, evidence, and tradeoffs.
- Expand an acronym on first use unless the landing page defines it. Use exact code names only after explaining their role in ordinary language.
- State costs honestly. Every meaningful design choice SHOULD pair its benefit with a drawback, constraint, or rejected alternative.
- Use `Implemented path`, `Partial`, `Capability-gated`, `Not found`, and `Unproved` consistently. Do not use vague states such as “supported” or “complete” without the matrix and evidence scope.
- Use the [Current Feature Readiness scoring model](../../Acceptance/CurrentReadiness.md#scoring-model) for numeric progress. A percentage MUST expose its `I/R/V/D` components and snapshot, MUST link to the central owning row, and MUST NOT be described as an acceptance percentage.
- Put long source lists, capability IDs, failure/check matrices, and audit tables under clearly labeled reference sections near the end.
- Render header metadata as short bold labels separated by blank lines. Put the current state, main limitation, and evidence boundary in one callout when those facts are easy to confuse.

## Visual Elements

Use visuals when they reduce the work required to understand relationships:

- Mermaid `flowchart` for ownership, execution, or data flow;
- Mermaid `sequenceDiagram` for cross-thread or asynchronous lifetime;
- tables for feature/backend/support comparisons and design tradeoffs;
- checked-in images for visual-quality differences, UI instructions, or diagnostic examples when a real image communicates more than a diagram.

Every visual MUST have a sentence explaining what the reader should notice. Keep diagrams bounded to one question; split a diagram that needs unrelated branches or more than roughly a dozen nodes. Use semantic labels rather than source filenames inside the visual, then link source details below it. A diagram is navigation and explanation, not implementation or runtime evidence.

## One Knowledge Owner

Every rule, decision, plan item, evidence gate, current-state fact, and historical claim MUST have one owning document. Other documents link to the owner and state only the context needed by their own responsibility. Repeated tables, phase lists, status summaries, and definitions are duplicate authority unless they are generated projections or clearly labeled snapshots.

[Current Feature Readiness](../../Acceptance/CurrentReadiness.md) owns readiness percentages and their component breakdown. A feature dossier may project its current number near the top for reader orientation, but the projection names the same snapshot and links back to that dashboard. Plans and research link to the owning feature score; writing a plan or study does not increase implementation readiness.

Code and executable build configuration own implemented behavior. A document MUST NOT upgrade source presence into build, runtime, visual, performance, or release proof.

## Placement By Owning Subject And Knowledge Role

| Location | Place here | Do not place here |
| --- | --- | --- |
| `Strategy` | desired capabilities, priority, roadmap, operating model, dated executive assessments | implementation rules or system design |
| `Architecture/Modules/<boundary>/<module>` | module-owned current maps, capability snapshots, decisions, current/target system shape, delivery plans, research, and feature-local proof contracts | another module's private mechanics or release-result ledgers |
| `Architecture/CrossModule/<subject>` | the same knowledge roles for a system with several durable owners and no coherent primary module owner | a relationship that still has one clear module owner |
| `Engineering` | binding standards, change/review procedure, accepted technical decision records, operational runbooks | product scope or acceptance gates |
| `Acceptance` | shared completion vocabulary, cross-feature workload and release gates, report schemas, candidate reports, and high-level progress | architecture, priority, or duplicate feature-local proof contracts |

Within an Architecture subject folder, keep knowledge roles explicit:

| Role | Conventional file | Owns | Does not own |
| --- | --- | --- | --- |
| dossier/current/target design | `README.md`, `Capability.md`, or a descriptive architecture page | behavior, ownership, current state, decisions, and feature-local contract routing | delivery order, external precedent, or candidate results |
| plan | `Plan.md` or a scoped `*Plan.md` | ordered delivery slices, dependencies, stop rules, migration and validation sequence | enduring decisions or completion claims |
| research | `Research.md` or a descriptive `*Research.md` | external precedent, option studies, visual exploration, dated migration baselines | binding local policy, implementation state, or evidence grades |
| feature-local acceptance | `Acceptance.md` when too large for the dossier | criteria, controlled failures, checks, and definition of done | candidate or release verdicts |

Feature definition, its plan, research, and feature-local acceptance form one navigational unit while retaining separate authority. Keep independently maintained roles as separate, cross-linked files beside the owning dossier. `Docs/Acceptance` may index local contracts and record high-level candidate progress, but MUST NOT reproduce their detailed matrices. Actual candidate results belong in the release-level completion report and retained evidence.

Small rationale, delivery notes, or validation checklists may remain with an owning contract when they exist only to explain or verify that contract and have no independent lifecycle. Standalone plans and research MUST be colocated under their module or cross-module subject; runbooks remain in Engineering and release-result ledgers remain in Acceptance.

## Placement By Module Ownership

Architecture MUST make the repository's durable module boundaries visible in its physical hierarchy:

- current module maps, capability snapshots, catalogs, module-owned designs, plans, research, and feature-local acceptance contracts live under `Architecture/Modules/<repository-boundary>/<module>`;
- `Architecture/Modules/Engine`, `Tools`, and `Projects` mirror their repository boundaries; repository-wide build and packaging knowledge remains directly under `Architecture/Modules`;
- a document that primarily belongs to one module stays with that module even when it calls, configures, or consumes another module; use links to explain those relationships;
- `Architecture/CrossModule` is reserved for a system with several durable owners and no coherent primary module owner;
- a cross-module subject MUST name the participating module owners and link to their module routes; its plan and research stay inside that subject folder.

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
- a projected `Current readiness` value and plain-language state for feature, module, and current-capability pages; target-only pages state `0/100 — target only` when no implementation exists, while non-feature standards and pure research use `Not applicable` and link to the feature owner where needed.

Render these facts as short bold metadata labels with one fact per paragraph. Do not compress several metadata fields into a dense prose sentence.

Use one primary type: **strategy contract**, **roadmap**, **operating model**, **orientation**, **canonical decision**, **standard**, **current map**, **target architecture**, **capability snapshot**, **feature dossier**, **plan**, **acceptance contract**, **runbook**, **research**, **dated assessment**, or **archive**. A feature dossier may route separate current-state and target-design pages while owning or colocating its feature-local acceptance contract. “Summary” and “index” are navigation roles, not second authorities.

## Granularity

Split a document when sections have different owners, audiences, lifecycles, authority types, or independent reasons to change. Keep sections together when they enforce one invariant and normally change in the same review. Feature-local acceptance is not a reason to create a parallel `Acceptance` taxonomy: keep it in the feature dossier, or use an adjacent `Acceptance.md` only when size makes the dossier materially harder to navigate.

Length alone is not a split rule. A short file without an independent responsibility is fragmentation; a long file containing architecture, phase history, research, and acceptance is mixed authority. Prefer one cohesive owner over both extremes. Delivery plans call their intermediate gates **phase exit criteria**; they must not label those gates feature acceptance or retain a second final definition of done.

Direct sibling count is a navigation signal even though it is not an authority boundary. An active, non-archive folder MUST NOT retain more than seven directly owned Markdown files. Before adding an eighth, form durable subject or document-class subfolders and give each multi-document folder a local `README.md`. The subdivision groups documents; it MUST NOT merge independently maintained contracts merely to reduce the visible count. A closed generated catalog or legally required archive may exceed the budget only when its index states why semantic subdivision would damage its responsibility.

A subfolder MUST represent a durable subject, repository module, or document class. A module folder MAY contain only its `README.md` when it mirrors a durable source/build module and that page owns the module inventory; this consistency is intentional, not fragmentation. `Acceptance` MAY likewise use a single-`README.md` module progress folder when it mirrors a durable module and contains only high-level dossier/report/disposition routing. Other categorization folders need at least two owned documents. When a durable feature has two or more independently maintained files in one area, group them under `<Feature>/`: use `README.md` as the feature dossier and role names such as `Capability.md` or `Acceptance.md` for companions instead of repeating the feature name across flat siblings. Every multi-document folder needs a `README.md`; it may own the dossier when the folder path names the subject, otherwise it is a short route whose contents are not already completely routed by the parent index.

A feature family with child capabilities that have different inputs/results, selectors, capability states, failure modes, or proof obligations SHOULD use one named folder. Its `README.md` owns shared ordering and invariants; each independently reviewable child owns a descriptive document. Do not represent one family partly as loose peer files and partly as a same-level folder. Expected but absent capabilities receive explicit negative dossiers when omission could be mistaken for support from an adjacent feature.

## Naming

- Use descriptive PascalCase filenames consistent with repository code and existing stable documents. A module inventory uses `README.md` inside its exact module folder so the path carries the module identity.
- Name the subject, not the author, date, state of mind, or editing action.
- Use `Plan.md` when a subject folder has one delivery authority. Use a narrow name such as `FirstReleasePlan.md` when a module contains several independently maintained plans.
- Use `Research.md` when a subject has one study, or a descriptive `*Research.md`, `*Precedent.md`, or `*Baseline.md` when several studies coexist.
- Do not create parallel top-level `Plans` or `Research` taxonomies. The owning module/feature path supplies subject context; the filename and status header supply knowledge type.
- Put historical dates and revisions in document metadata, not filenames, unless multiple retained snapshots require date identity.
- Avoid catch-all names such as `Misc`, `Notes`, `Ideas`, `New`, or `Final`. A deliberately exploratory document may use “Idea” only when its research status and promotion gate are explicit.
- Preserve stable A-L identifiers in titles where cross-document traceability still uses them; the filename should remain descriptive.

## Navigation And Links

The required route is `Docs/README.md` -> Architecture/Engineering/Strategy/Acceptance index -> module or cross-module subject index -> owning document. Every non-index document MUST be reachable from its nearest index, and every index entry MUST state why a reader would open it. A reader MUST be able to distinguish module-owned and cross-module knowledge from the path, and architecture/plan/research/acceptance roles from the filename and status header.

Use relative Markdown links. Link directly to the owning section where practical. When a file moves or splits, update every producer and consumer in the same change and remove the old path; do not retain alias files, duplicate copies, or compatibility indexes.

An index routes. It SHOULD NOT reproduce large status tables, requirements, phase lists, or architecture descriptions from the documents it lists.

## Lifecycle

- Reconcile current maps and capability snapshots whenever their named owners, consumers, build membership, or public selection surface changes.
- Reconcile the central readiness row and every local projection whenever implementation, reachability, candidate evidence, or delivery/adoption state changes.
- Update a feature's architecture, local acceptance contract, directly affected plans, release/workload tracking, and indexes in one change when its contract moves.
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
8. every materially changed feature passes the feature depth test rather than merely matching the template;
9. UTF-8, whitespace, and `git diff --check` pass.
