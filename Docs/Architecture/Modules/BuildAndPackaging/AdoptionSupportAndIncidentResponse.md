# Adoption, Support, And Incident Response

**Status:** target product capability dossier; current developer routes exist, but the public adoption/support operation is incomplete

**Scope:** define public onboarding, failure recovery, support/security intake, incident response, patch/withdrawal, and independent-reproduction responsibilities

**Owner:** release/product delivery; Launcher, Application, Core diagnostics, Docs, and project products provide owned parts of the route

**Snapshot:** 2026-09-08 at committed `master` revision `ffe60e3a`; current product/workflow inventories, repository/public entry points, Core/RHI diagnostics, symbols, and First Release failure/support surfaces reconciled; source evidence `S` only

**Strategy and acceptance sources:** `NS-ADOPTION`, `NS-EVIDENCE`, and `NS-OWNERSHIP` in the [Engineer Persona](../../../Strategy/EngineerPersona.md); `FCR-PROD-01`, `FCR-PROD-02`, `FCR-PROD-06`; [First Release](../../../Acceptance/FirstRelease.md)

**Current readiness:** **15/100** — developer diagnostics and partial onboarding exist; public adoption, support/security intake, incident, patch, and withdrawal operation do not. See [Current Feature Readiness](../../../Acceptance/CurrentReadiness.md#product-build-and-delivery).

## At A Glance

| User journey | Current state | Missing closure |
| --- | --- | --- |
| discover prerequisites and first-use route | partial Launcher/Docs developer path | public product identity, supported environment, controls, reset, and expected result |
| recover from startup/content/backend failure | partial local diagnostics | stable public error categories, recovery steps, and verified first-user comprehension |
| report a crash, hang, defect, or security issue | Not found | privacy/consent policy, support bundle, redaction, intake channels, and ownership |
| receive response or product correction | Not found | severity model, response targets, advisory/patch/withdrawal and supported-version policy |
| reproduce independently | Not found | clean public bytes, package identity, evidence bundle, and non-author result |

The capability begins before launch and continues after failure. A polished internal tool is only one component; adoption closes when a non-author can reach the result or produce an actionable privacy-safe report and understand what happens next.

## Capability Identity

| ID | Capability | Current state |
| --- | --- | --- |
| `ADOPT-01` | Public product identity, prerequisites, first-use, controls, reset, and exit | Partial developer route; public route incomplete |
| `ADOPT-02` | Actionable unsupported/missing/corrupt/startup failure and recovery | Partial local diagnostics; public recovery route incomplete |
| `ADOPT-03` | Privacy-safe crash/hang support bundle and issue/security intake | Not found |
| `ADOPT-04` | Severity, response, patch/advisory/withdrawal, and supported-version operation | Not found |

## Capability Boundary

This feature begins when a non-author encounters Sparkle and ends when they can either reach the promised result or produce a privacy-safe, actionable failure report. It includes product identity, prerequisites, first-run selection, expected output, controls, troubleshooting, support/security intake, crash/hang collection, severity/response policy, patch or withdrawal, and independent reproduction. It does not make every internal tool a supported public product.

Current Launcher readiness and Docs routes are partial building blocks. There is no root onboarding document, frozen public support channel, consented crash-report product, security intake, response clock, update/patch delivery, or independent first-user result in the inspected snapshot.

## Contract

- Inputs: package/source-adopter classification, supported platform/backend/provider/content matrix, prerequisites, product/version identity, consent and privacy policy, diagnostic identity, and issue severity.
- Outputs: successful first-use result or stable actionable failure; redacted support bundle; issue/security acknowledgement; reproducible case; resolution, advisory, patch, or withdrawal decision.
- Runtime and tools own precise diagnostics. Delivery owns the user journey, support-bundle schema, consent, retention, routing, service expectation, and published support decision.
- Logs/captures/crash data are bounded, inspectable before submission, opt-in where required, free of credentials/private paths, and associated with an exact product/revision/configuration identity.

## `FCR-PROD-06` Support And Incident Contract

The support/stabilization owner owns root `SUPPORT.md` and `SECURITY.md`, the public defect and private security channels they name, privacy/retention/redaction policy, candidate-to-symbol lookup, severity clocks, advisory/patch/withdraw decisions, and supported-version data. Core/RHI/Application own diagnostic production, not intake or response policy. The [frozen product contract](../../../Acceptance/FirstRelease.md#frozen-paths-mutation-boundary-and-budgets) owns paths and clocks.

| ID | Binary acceptance criterion |
| --- | --- |
| `AC-ADOPT-01` | A runtime consumer and source adopter can reach the correct public onboarding/support route, identify product/version/package/source/configuration, expected result, prerequisites, reset/exit, known issues, and whether to use public defect or private security intake without private guidance. |
| `AC-ADOPT-02` | A user can inspect and explicitly export a bounded redacted bundle for startup failure, process crash, hang/timeout, and GPU/device failure; no automatic upload/telemetry occurs and absence of a diagnostic is reported unavailable rather than successful. |
| `AC-ADOPT-03` | Native/shader symbols, logs, dumps/captures when consented, and manifest/provenance map to the exact immutable candidate; a mismatched or insufficient bundle is rejected before submission with recovery. |
| `AC-ADOPT-04` | Public defect and private security intake acknowledge within the frozen clocks, assign an owner/severity, preserve confidentiality/retention, and produce a reproducible resolution, known issue, advisory, patch, or withdrawal decision with version applicability. |
| `AC-ADOPT-05` | A patch never changes bytes under an existing version; verify/apply/rollback or replace-side-by-side policy, evidence invalidation, user-state preservation/reset, and withdrawal are explicit and exercised for one isolated candidate copy. |

| ID | Cause/injection, safe result, and affected criterion |
| --- | --- |
| `FM-ADOPT-01` | Remove `SUPPORT.md`/`SECURITY.md`, break an intake link, or omit a prerequisite. The journey is `Blocked`; docs/search/Launcher may not report readiness (`AC-ADOPT-01`, `AC-ADOPT-04`). |
| `FM-ADOPT-02` | Inject username/private paths, tokens, arbitrary content, oversized logs, or wrong candidate identity into a bundle. Preview/redaction/validation rejects it before submission and leaves originals local (`AC-ADOPT-02`, `AC-ADOPT-03`). |
| `FM-ADOPT-03` | Disable DRED/Vulkan/crash/hang collection or omit symbols. The route reports the unavailable item and alternative manual evidence; zero/empty output is never success (`AC-ADOPT-02`, `AC-ADOPT-03`). |
| `FM-ADOPT-04` | File a controlled `S0`/security report with the operating owner unavailable or clock exceeded. Publication/stabilization blocks and affected bytes are contained/withdrawn; silence is not a response (`AC-ADOPT-04`). |
| `FM-ADOPT-05` | Present a patch for the wrong hash/version or interrupt replacement. It is rejected before mutation; original bytes/user state remain, and no existing version is overwritten (`AC-ADOPT-05`). |

| Check | Claims falsified | Smallest route and fixed oracle |
| --- | --- | --- |
| `CHK-ADOPT-01` | `AC-ADOPT-01`; `FM-ADOPT-01` | Script one non-author runtime and source journey from public entry through success and one prerequisite failure. Record every intervention; any private repair, dead link, false readiness, or missing distinction fails. |
| `CHK-ADOPT-02` | `AC-ADOPT-02`, `AC-ADOPT-03`; `FM-ADOPT-02`, `FM-ADOPT-03` | On isolated user data, trigger startup/process/hang/GPU cases where supported, export with consent, inject private/mismatched/oversized/unavailable data, and inspect bundle schema, redaction, bounds, identity, symbols, and network activity. |
| `CHK-ADOPT-03` | `AC-ADOPT-04`; `FM-ADOPT-04` | Submit controlled public and private fixtures through the named channels, retain acknowledgement/owner/severity/timestamps, and rehearse known-issue/advisory/withdraw decisions against the frozen clocks. |
| `CHK-ADOPT-04` | `AC-ADOPT-05`; `FM-ADOPT-05` | Apply and interrupt a versioned replacement against copies with representative user state; verify hash applicability, atomicity, side-by-side/reset policy, evidence invalidation, rollback/withdraw result, and unchanged published bytes. |

Every criterion/failure maps to a check. At this revision root `SUPPORT.md`, root `SECURITY.md`, a consented bundle/exporter, public crash/hang operation, assigned operating maintainer, and patch/withdraw route are absent, so `FCR-PROD-06` is `Blocked`; the frozen clocks are not claimed as an operating service.
