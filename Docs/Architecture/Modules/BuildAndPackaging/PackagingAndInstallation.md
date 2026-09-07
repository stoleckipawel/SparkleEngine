# Packaging And Installation

**Status:** target capability dossier; no current public package or installer was found

**Scope:** define deterministic staging, manifests, integrity/signing, packaging, installation, writable state, relocation, and clean-machine operation

**Owner:** repository build and release-delivery configuration; product contributors are Launcher, Showcase, Application, Tools, and the engine modules whose runtime bytes enter the package

**Snapshot:** 2026-09-07; root/module `CMakeLists.txt` and `CMake/*.cmake` were searched for `install(...)` and CPack ownership with no match; source evidence `S` only

**Strategy and acceptance sources:** [`PGE-01`, `PGE-13`, `PGE-14`, `PGE-15`](../../../Strategy/Requirements.md), [First Release](../../../Acceptance/FirstRelease.md), and `FCR-PROD-02`/`FCR-PROD-05` in [Feature Completion Reports](../../../Acceptance/FeatureCompletionReports.md)

**Current readiness:** **0/100** — target only; no public stage/sign/verify/package/install product path was found. See [Current Feature Readiness](../../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities).

## At A Glance

| Product boundary | Current state | Required result |
| --- | --- | --- |
| development artifacts | implemented workspace-oriented output/copy routes | remain inputs only; never advertised as a release package |
| immutable staging | Not found | declared membership and one manifest account for every byte, permission, hash, and license |
| trust/publication | Not found | SBOM/provenance, signing identity, integrity verification, and quarantined failure |
| installation and writable state | Not found | standard-user install/update/uninstall plus explicit per-user state outside immutable bytes |
| relocation and clean-machine use | Not found | offline operation without repository, build tree, SDK, or private path dependencies |

The target route is one directional transaction: Build -> Cook -> Stage -> Verify/Sign -> Package -> Install/Run. Each step consumes a declared predecessor product and must never scan unrelated development directories to infer missing membership.

## Capability Identity

| ID | Capability | Current state |
| --- | --- | --- |
| `PKG-01` | Deterministic immutable staging tree and manifest | Not found |
| `PKG-02` | License/SBOM, signature, integrity, and publication verification | Not found |
| `PKG-03` | Standard-user install/update/uninstall with separate writable state | Not found |
| `PKG-04` | Relocated, read-only, offline, clean-machine product operation | Not found |

## Capability Boundary

The current CMake graph produces development artifacts and copies selected runtime support beside executables. It does not currently own a formal immutable stage, archive or installer, manifest, signing flow, SBOM, installation/update contract, or clean-machine package verification. Those absences are part of this feature dossier rather than implied future behavior.

The target capability is one Build -> Cook -> Stage -> Sign -> Verify -> Package route. It must classify the runtime, editor, Launcher, host tools, symbols, source/debug material, licenses, vendor binaries, cooked content, configuration, and writable per-user state independently.

## Inputs, Outputs, Ownership, And Lifetime

- Inputs: frozen source revision, profile/toolchain/options, dependency identities, selected product, cooked-content manifest, redistribution policy, signing identity, and package version.
- Outputs: immutable staging tree, byte/permission/hash manifest, license/SBOM inventory, signatures, archive or installer, verification result, and clean-machine evidence reference.
- Build/release configuration owns membership and publication. Individual modules own declarations of their required runtime products; no consumer may scan development directories to infer membership.
- Package bytes are immutable. Logs, settings, caches, captures, saves, and crash data live in an explicit per-user writable root and survive or reset only by declared policy.

## Requested And Active State

Packaging is currently **absent**, not Experimental or Included. A development artifact directory is not an active package. Any future candidate must publish the selected product, profile, backend/provider support, content set, package identity, and signature status in both its manifest and completion report.

## Failure, Capacity, And Observability

Missing, extra, unsigned, hash-mismatched, license-incomplete, path-escaping, source-tree-dependent, or package-mutating bytes fail verification. Unsupported standard-user paths and unavailable optional providers must produce bounded actionable failures. The report records archive size, installed size, file count, peak stage/package memory, cold/warm duration, and deletion/update behavior.

## Design Decisions And Tradeoffs

| Decision | Benefit | Cost or constraint |
| --- | --- | --- |
| manifest-driven membership | reproducibility and review do not depend on directory scans | every contributing target must declare its runtime products |
| immutable installed bytes plus separate user state | verification, update, and uninstall behavior stay deterministic | paths/configuration must be designed for relocation and standard users |
| verify before candidate publication | corrupt, unsigned, or license-incomplete output cannot escape as a release | staging adds time, storage, signing, and provenance infrastructure |
| classify editor/runtime/tools/symbols independently | product contents match the intended audience and redistribution policy | multiple product profiles need explicit package matrices |

## Acceptance Handoff

- `AC-PKG-01`: one deterministic manifest accounts for every delivered byte and rejects undeclared or changed bytes.
- `AC-PKG-02`: runtime operation uses only package and documented OS/user-state locations; no repository, build-tree, SDK, or private-path dependency is observed.
- `AC-PKG-03`: standard-user install, first run, repeated run, update/replace, uninstall, and missing/corrupt-content cases have explicit outcomes.
- `AC-PKG-04`: licenses, redistribution decisions, SBOM/provenance, signatures, symbols, and excluded source/tool/editor material match the frozen release disposition.
- `FM-PKG-01`: stage membership is incomplete or contains forbidden material; verification fails before publication.
- `FM-PKG-02`: signature/hash/license verification fails; the package is quarantined and cannot be called a candidate.
- `FM-PKG-03`: runtime writes into immutable package state or depends on the source tree; clean-machine acceptance fails.
- `CHK-PKG-01`: generate and independently verify the stage manifest, hashes, signatures, licenses, and allowlist/denylist.
- `CHK-PKG-02`: install or unpack on a clean standard-user environment and execute the frozen first-run, repeat-run, failure, and uninstall matrix.

No acceptance row is satisfied by this documentation pass. [`BUILD-E03`](../../../Plans/CapabilityEvidence.md#product-workflow-and-delivery-evidence) is the smallest central proof destination; candidate-bound results belong in the release artifact path required by [First Release](../../../Acceptance/FirstRelease.md).
