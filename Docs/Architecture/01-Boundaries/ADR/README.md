# Architecture Decision Records

This directory contains Architecture Decision Records for SparkleEngine boundaries, exceptions, and important architectural tradeoffs that need durable explanation.

## Purpose

Use an ADR when:

- a boundary exception is intentionally allowed
- a module ownership decision needs durable justification
- an integration seam carries risk and needs a retirement path
- future contributors might otherwise misread a temporary exception as architecture policy

Do not use an ADR for routine code cleanup, naming tweaks, or implementation detail churn that does not affect architecture.

## Required ADR Sections

Every ADR should include:

- Context
- Decision
- Affected files or scope
- Why the decision is allowed
- Risk
- Retirement path or long-term posture
- Acceptance criteria for removal, tightening, or validation

## Naming

Use the format:

- `0001-short-kebab-case-title.md`
- `0002-short-kebab-case-title.md`

Keep the title specific to the boundary or decision.

## Current ADRs

- [0001-renderer-native-api-provider-exceptions.md](./0001-renderer-native-api-provider-exceptions.md)

## How To Add A Future ADR

1. Pick the next numeric prefix.
2. Use a narrow, specific title.
3. Link the ADR from any human-facing rule document that depends on it.
4. If the ADR documents a counted exception, keep the implementation rule narrow and the baseline frozen.
5. If the decision is temporary, say what completion condition retires it.
6. Do not broaden rule scope without updating both the ADR and the executable boundary rule.

