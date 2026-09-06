# Performance Dashboard And Efficiency Hub Idea

Status: research; target-product exploration only, not implementation authority or proof of current behavior

Date: 2026-08-17

Scope: the conditions under which historical performance automation should become a separate SparkleEngine tool

## Decision Question And Authority

Should historical performance comparison remain a command-line workflow, live inside the Editor, or become a separate application?

This document owns only that future product-boundary question and its promotion gates. It does not redefine:

- metric meaning, collection, identity, or live presentation from [Performance Diagnostics Architecture](../Architecture/CrossModule/PerformanceDiagnostics/README.md);
- delivery order or package state from the [Performance Diagnostics Delivery Plan](../Plans/CrossModule/PerformanceDiagnostics.md);
- Editor interaction from [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsVisualDesign.md);
- profiler operation from the [External Performance Profiler Runbook](../Engineering/Verification/ExternalProfiling.md);
- benchmark routes, samples, and acceptance evidence from [Bistro And San Miguel Acceptance Workloads](../Acceptance/GraphicsWorkloads.md);
- roadmap priority from the [Principal Graphics Roadmap](../Strategy/Roadmap.md).

Code, CMake, tests, generated artifacts, and observed captures remain the authority for implemented behavior.

## Current Recommendation

Do not build a separate graphical application yet. Mature one evidence contract through cheaper forms first:

```text
Sparkle runtime measurements and explicit exports
        |
        v
narrow external runner and analyzer
        |
        v
generated static history report
        |
        v
scheduled runs on a controlled machine
        |
        v
thin separate dashboard, only after promotion gates pass
```

The engine should own authoritative measurements and bounded live orientation. The external workflow should own repeatable execution, comparison, and report generation. A future dashboard may submit jobs and read immutable evidence; it must not become another collector, metric authority, benchmark protocol, capture implementation, or result schema.

This keeps the first useful product small: a reproducible run and an honest comparison are more valuable than an application shell.

## Responsibility Split

| Owner | Responsibilities | Must not own |
| --- | --- | --- |
| Sparkle runtime and Editor | Physical measurements at current owners, stable identities, validity/loss, bounded live views, explicit evidence export, profiler handoff. | Historical database, background benchmark service, anomaly verdicts, vendor-profiler replacement. |
| External runner and analyzer | Launch matrix, workload invocation, artifact validation, cohort matching, comparison, thresholds, generated reports, CI exit status. | Alternate metric semantics, hidden collection, UI-specific schema, implicit acceptance. |
| Future dashboard | Browse accepted runs, filter exact cohorts, display trends, link evidence, submit approved jobs. | Mutable copies of engine truth, its own capture path, arbitrary remote execution, automatic root-cause claims. |

The versioned evidence artifact is the only handoff. Runtime views, command-line reports, and any future dashboard are immutable readers of the same meanings.

## Maturity Stages

| Stage | Deliverable | Exit evidence |
| --- | --- | --- |
| 0. Evidence contract | Accepted identities, validity, cohort fields, artifact layout, and export semantics. | Two compatible runs can be compared without guessing units, provenance, or validity. |
| 1. Offline analyzer | One command validates artifacts and reports regressions against explicit workload thresholds. | Repeatable local use on accepted workloads with malformed and incomparable inputs rejected. |
| 2. Static history | Analyzer emits a self-contained report over immutable run artifacts. | The report answers a recurring review question more clearly than raw files. |
| 3. Controlled automation | Scheduled runner uses a named machine, fixed environment, bounded retention, and explicit ownership. | Repeated trustworthy datasets exist and failures distinguish infrastructure from product regressions. |
| 4. Separate dashboard | Thin reader/job frontend over the existing contract and runner. | Every promotion gate below is satisfied. |

Fleet scheduling, arbitrary agents, multi-tenant access, a trace warehouse, and generalized observability remain out of scope until a separately approved need owns them.

## Separate-Application Promotion Gates

Create a dashboard project only when all of the following are true:

1. Accepted runtime evidence and the external analyzer already work end to end.
2. Repeated comparable datasets exist for at least one stable workload and controlled environment.
3. At least two real consumers need history, or one consumer has a recurring question that static reports cannot answer efficiently.
4. The needed interaction is primarily browsing, filtering, comparing, and evidence navigation rather than new collection semantics.
5. Storage, retention, access, secrets, runner safety, schema migration, and maintenance have named owners.
6. A prototype proves the app can remain a thin reader over the existing artifact and job contracts.
7. The expected review value justifies the added build, dependency, deployment, and support surface.

If any gate fails, improve the runner, analyzer, or generated report instead.

## Comparison Contract

Comparison starts with exact cohort identity. At minimum, a cohort records commit, product, backend, build profile, workload and route, scene variant, resolution, presentation policy, adapter, driver, operating system, capture mode, and relevant experiment controls.

Rules:

- Preserve each run as the primary record; summaries never replace raw evidence.
- Compare only compatible cohorts unless the user explicitly requests a controlled cross-cohort experiment.
- Use workload-owned thresholds and sample policy; the dashboard does not invent universal budgets.
- Show unavailable, invalid, dropped, partial, and incomparable data instead of substituting zero or carrying values forward.
- Keep baselines immutable. Promoting a new baseline is an explicit reviewed action with provenance.
- Link a regression to the exact run, logs, captures, configuration, and source revision used to classify it.
- Treat anomaly output as a triage hint, never as a root-cause or acceptance verdict.

Memory reports must preserve distinct concepts such as process memory, committed GPU memory, resident GPU memory, budgets, pending upload bytes, and retirement backlog. Combining them into one “memory usage” score destroys diagnostic meaning.

## First Useful Report

The generated report should answer only the high-value questions:

- Did the run complete with valid, comparable evidence?
- Which exact cohort and baseline were used?
- Which workload-owned thresholds passed, regressed, improved, or could not be evaluated?
- How did CPU frame time, GPU queue timing, frame pacing, memory/residency, and compilation or streaming indicators change without collapsing their identities?
- Which artifacts should a reviewer open next in Sparkle, PIX, RenderDoc, Nsight, RGP, RMV, ETW, or a CPU profiler?

Default presentation should show the current result, baseline delta, distribution or high-water evidence where applicable, validity, and one contextual next action. Detailed histories, raw samples, and expert tool links remain progressive disclosure.

## Stop Rules

Reject or remove the dashboard direction if it produces any of these outcomes:

- a second metric definition, sample store, capture state machine, benchmark route, or export schema;
- an Editor panel and a separate app that answer the same question from independently retained state;
- always-on uploads, hidden background collection, or uncontrolled command execution;
- anomaly labels without exact cohort, validity, threshold, and source evidence;
- infrastructure work before accepted measurements and repeatable runs exist;
- a polished surface that cannot identify a concrete engineering decision it improves.

## Sequencing Constraint

The [Performance Diagnostics Delivery Plan](../Plans/CrossModule/PerformanceDiagnostics.md) remains the only diagnostics implementation sequence. This proposal cannot move a package to `Selected`, add a project, or change acceptance scope. Any future dashboard package must first update that plan and the repository ownership map, then satisfy the normal Engineering standards and change process.

Until the promotion gates pass, the selected product boundary is:

```text
authoritative engine evidence -> external analyzer -> generated report
```

That boundary is intentionally complete without a separate application.
