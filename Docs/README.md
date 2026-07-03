# Sparkle Documentation

This folder keeps only active implementation guidance. Old review notes, superseded staged plans, and standalone validation notes are intentionally removed unless they directly support the current PBR/provider work.

## Renderer Release Readiness

- `Architecture/00-Review/B_RendererReleaseReadinessMap.md`: current renderer map and release-readiness gap audit against AMD/NVIDIA reference repository shapes. It is focused on cleanup, feature completion, and reducing scaffolding before product release.
- `Architecture/00-Review/C_ValidationDiagnosticsCleanupMap.md`: source-backed staged cleanup map for validation, diagnostics, smoke flows, debug artifacts, and wrapper-only code that can be reviewed for deletion.

## Review Background

- `Architecture/00-Review/A_PrincipalRoleRequirements.md`: retained only as background for principal-level rendering expectations.

Keep new docs only when they describe an active implementation prompt, a source-backed contract, or a reference-backed rendering decision that the source cannot make obvious by itself.
