# Adoption, Support, And Incident Response

**Status:** target product capability dossier; current developer routes exist, but the public adoption/support operation is incomplete

**Scope:** define public onboarding, failure recovery, support/security intake, incident response, patch/withdrawal, and independent-reproduction responsibilities

**Owner:** release/product delivery; Launcher, Application, Core diagnostics, Docs, and project products provide owned parts of the route

**Snapshot:** 2026-09-07; current product/workflow inventories, repository-root entry points, and First Release failure/support surfaces reconciled; source evidence `S` only

**Strategy and acceptance sources:** `NS-ADOPTION`, `NS-EVIDENCE`, and `NS-OWNERSHIP` in the [Engineer Persona](../../../Strategy/EngineerPersona.md); `FCR-PROD-01`, `FCR-PROD-02`, `FCR-PROD-06`; [First Release](../../../Acceptance/FirstRelease.md)

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

## Acceptance Handoff

- `AC-ADOPT-01`: a non-author can identify the product promise, supported matrix, prerequisites, first action, expected output, controls, reset, and exit without private guidance.
- `AC-ADOPT-02`: missing content/toolchain/provider/hardware, corrupt configuration, startup failure, crash, hang, and unsupported environment produce distinct recovery or escalation paths.
- `AC-ADOPT-03`: support and security intake, privacy/consent, retention, severity, acknowledgement, patch/advisory/withdrawal, and version applicability are explicit.
- `AC-ADOPT-04`: an independent clean-user attempt retains enough evidence to reproduce both success and one controlled failure.
- `FM-ADOPT-01`: onboarding reaches a dead end or reports false readiness; adoption acceptance fails.
- `FM-ADOPT-02`: a support bundle leaks secrets/private paths or lacks product identity; it is rejected before submission.
- `FM-ADOPT-03`: a severe defect has no owned response/distribution route; affected release classification is withdrawn or blocked.
- `CHK-ADOPT-01`: conduct a scripted non-author source and packaged-product journey with no undocumented intervention.
- `CHK-ADOPT-02`: exercise the frozen failure/support/security matrix and inspect bundle contents, routing, acknowledgement, and resolution state.

No public support service or release response time is promised by this dossier; those values require a frozen release decision and candidate evidence through [`BUILD-E05`](../../../Plans/CapabilityEvidence.md#product-workflow-and-delivery-evidence).
