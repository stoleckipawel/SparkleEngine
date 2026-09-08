# Packaging And Installation

**Status:** target capability dossier; no current public package or installer was found

**Scope:** define deterministic staging, manifests, integrity/signing, packaging, installation, writable state, relocation, and clean-machine operation

**Owner:** repository build and release-delivery configuration; product contributors are Launcher, Showcase, Application, Tools, and the engine modules whose runtime bytes enter the package

**Snapshot:** 2026-09-08 at committed `master` revision `ffe60e3a`; root/module `CMakeLists.txt`, artifact/path discovery, product membership, and `CMake/*.cmake` were inspected; no `install(...)`, CPack, stage, manifest generation, or package owner was found; source evidence `S` only

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

## External Precedent And Transfer Boundary

The external reference is **Unreal Engine 5.8 documentation**, retrieved 2026-09-08: [Packaging Unreal Engine Projects](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project?application_version=5.8) and [BuildGraph](https://dev.epicgames.com/documentation/unreal-engine/buildgraph-for-unreal-engine?lang=en-US&application_version=5.8), including its [script-elements reference](https://dev.epicgames.com/documentation/unreal-engine/buildgraph-script-elements-reference-for-unreal-engine?lang=en-US&application_version=5.8). No Unreal source checkout, code commit, AutomationTool implementation, or Epic service was inspected or adopted.

Only these invariants transfer into Sparkle's local contract:

- Build, Cook, Stage, Verify/Sign, and Package are distinct owned products; Stage is a fresh declared tree outside development outputs, not a rename of `artifacts/dev`.
- A requested terminal product executes only the dependency nodes it requires; every node has explicit inputs, required predecessors, produced outputs, result, and retained artifact identity.
- Shipping membership deliberately differs from development membership, including removal of developer console/debug/authoring surfaces.
- cancellation/failure remains visible at the owning operation, and a failed prerequisite or node prevents dependent publication.

Sparkle does **not** adopt UAT, BuildGraph XML, nodes/agents/triggers as public vocabulary, Unreal's directory/Pak layout, Editor-driven packaging, or a Launcher/package dependency. Local CMake/tool owners and the frozen Sparkle manifest remain authoritative.

## `FCR-PROD-05` Package Contract

| ID | Binary acceptance criterion |
| --- | --- |
| `AC-PKG-01` | One command consumes a frozen revision/configuration/content set and produces Build -> Cook -> fresh Stage -> Sign -> Verify -> Package results with explicit predecessor/output identities; failure at any node prevents archive publication. |
| `AC-PKG-02` | `manifests/sparkle-package-manifest.json` accounts for every staged path, byte size, hash, permission/role, owner, origin, license, signature expectation, and component; independent verification rejects missing, extra, changed, absolute, escaping, or forbidden entries. |
| `AC-PKG-03` | The runtime archive matches the frozen product/layout classification exactly; symbols are a separate correlated archive and Installer/SDK/Launcher/Editor/authoring/source/debug products are absent. |
| `AC-PKG-04` | Extracted immutable bytes run read-only, offline, as a standard user from Unicode/spaced/relocated paths with no repository/build/SDK/environment/admin dependency; writes occur only in the frozen per-user root. |
| `AC-PKG-05` | Normal extract/first/repeat run, side-by-side replacement policy, archive removal, retained-user-data policy, and missing/corrupt dependency/content paths are documented and stay within frozen size/time/memory budgets. |
| `AC-PKG-06` | SBOM/provenance, notices/redistribution decisions, trusted PE signatures, checksums, archive/source/candidate identity, and native/shader symbol correlation all refer to the same immutable candidate. |

| ID | Cause/injection, safe result, and affected criterion |
| --- | --- |
| `FM-PKG-01` | Seed stale/extra/source/editor/tool/debug files or omit a declared runtime file. Fresh staging plus manifest verification rejects the tree before signing/publication (`AC-PKG-01`–`03`). |
| `FM-PKG-02` | Alter a copied PE/archive/manifest/license/signature after staging. Verification quarantines the output and no candidate/archive success is emitted (`AC-PKG-02`, `AC-PKG-06`). |
| `FM-PKG-03` | Make package bytes read-only, remove overrides, relocate, and block network. Any package/repository write or external-path/SDK dependency fails `AC-PKG-04`. |
| `FM-PKG-04` | Deny/fill/corrupt the per-user root. The runtime names the path/space, preserves immutable bytes and last accepted state, and exits or disables only the optional operation (`AC-PKG-04`, `AC-PKG-05`). |
| `FM-PKG-05` | Remove each allowlisted DLL/content file from isolated archive copies. Startup identifies the missing component and exits without partial success or fallback content (`AC-PKG-03`–`05`). |
| `FM-PKG-06` | Search output names/docs/files for installer or SDK semantics. Any bootstrapper/registry/install API/binary ABI implication without an admitted owner fails `AC-PKG-03` and blocks publication. |

| Check | Claims falsified | Smallest route and fixed oracle |
| --- | --- | --- |
| `CHK-PKG-01` | `AC-PKG-01`–`03`, `AC-PKG-06`; `FM-PKG-01`, `FM-PKG-02`, `FM-PKG-06` | Generate from a clean destination, retain per-node results, independently enumerate/verify allowlist, denylist, hashes, signatures, SBOM/notices/symbol identity, then inject stale/extra/missing/tampered inputs. No archive may exist after a failed node. |
| `CHK-PKG-02` | `AC-PKG-04`, `AC-PKG-05`; `FM-PKG-03`–`05` | On a clean standard-user minimum machine, extract to Unicode/spaced and read-only locations, block network/remove overrides, inventory all file/network/process activity, run consumer first/repeat/fault cases, remove archive, and compare budgets/state hashes. |

No acceptance row is satisfied by this documentation pass. [`BUILD-E03`](../../../Plans/CapabilityEvidence.md#product-workflow-and-delivery-evidence) is the smallest central proof destination; candidate-bound results belong in the release artifact path required by [First Release](../../../Acceptance/FirstRelease.md).
