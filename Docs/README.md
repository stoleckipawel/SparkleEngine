# Sparkle Documentation

This folder keeps only active implementation guidance. Old review notes, superseded staged plans, and standalone validation notes are intentionally removed unless they directly support the current PBR/provider work.

## PBR Rendering

- `Rendering/PBR/05-Renderer-Reference-Quality-Gap-Audit.md`: current implementation-prompt source, focused first on editor-visible SIGMA and DLRR integration. Each staged prompt includes code denoising, reuse/DRY, and NVIDIA/AMD reference-pattern requirements.

## Review Background

- `Architecture/00-Review/A_PrincipalRoleRequirements.md`: retained only as background for principal-level rendering expectations.

Keep new docs only when they describe an active implementation prompt, a source-backed contract, or a reference-backed rendering decision that the source cannot make obvious by itself.
