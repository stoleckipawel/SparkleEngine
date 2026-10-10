# First Release Scope Record

**Status:** revised `REL-00` proposal with owner-directed rendering obligations; remaining scope/rights decisions unclosed, no release acceptance claimed

**Identity:** `REL-00-v0.1.0-01`; inspected 2026-10-10 at `93e86b8113c3fcd0c4921729c3513538b95bc794`, initially clean worktree

**Responsibility:** select the finite first-release product, public surface, machines, maps and publication promises

**Authority:** [First Release Acceptance Contract](../FirstRelease.md#release-scope-freeze) owns the requirements and gate verdict. [Capability Dispositions](CapabilityDispositions.md) owns the row-level choices beneath this record. Architecture inventories describe implementation; neither a scope choice nor source presence accepts a feature.

## Read This Page

- **See the selected product:** [Decision summary](#decision-summary), [machine/API matrix](#product-and-supported-matrix) and [map proposal](#releasemapset).
- **Make the next decision:** [Approval and next work](#approval-and-next-work) separates approved choices from unresolved ones.
- **Inspect policy or provenance:** The compatibility, budgets and support sections define the promises; iteration records retain the decision history.

## Decision Summary

The current proposal makes `v0.1.0` a portable Windows x64 `ShippingGame` Showcase demonstration with source-build instructions. The owner selected the existing laptop and then explicitly required reliable progressive reference tracing, realtime path tracing at >=30 FPS, complete DLSS/Ray Reconstruction, finished exposure/tone mapping, basic chromatic aberration/vignette, backend parity, a complete frame graph and principal-level whole-frame review. These are now Included obligations in the [acceptance owner](../FirstRelease.md#required-rendering-closure). Other product/map/support choices remain proposals; feature correctness and release acceptance are still unproved.

| Decision | Selected scope | Approval/evidence boundary |
| --- | --- | --- |
| Consumer product | `ShowcaseRuntime`; archive, checksums, manifest, notices, instructions and support route | Reuses the [PD-0 output freeze](../FirstRelease.md#frozen-product-and-output-classification); no package exists by this decision |
| Source adopter | Public tag; one pinned Windows source toolchain; documented configure/build/cook/run journey | No private assistance, credentials or repository-local dependency assumptions in accepted reproduction |
| Developer archive | Excluded; Launcher, Editor, import/cook/compiler tools remain source-build products | Never delays runtime publication solely to create a developer archive |
| Performance machine | `MIN-01` and `REF-01` are the same existing laptop | Machine choice approved by user; performance and clean-machine adoption unrun |
| APIs | D3D12 default; Vulkan explicitly selectable with required feature/semantic parity | Separate native/performance/provider evidence; unavailable required cells block closure |
| Consumer modes | Lit, a verified realtime path-traced preset, and progressive Reference; controlled rendering settings | Reference and realtime tracing are Included, not experimental substitutes; diagnostic views remain source tools |
| Default examples | Cornell Box first run; Cesium Man second example | Proposed `ReleaseMapSet`; exact imported bytes, environment assets, notices and rights still require `REL-01` proof |
| Mandatory rendering closure | Reference and realtime tracing, DLSS SR/RR, exposure/tone mapping, complete frame graph and parity; decals/color grading/chromatic aberration/vignette/HDR10 | Concrete acceptance matrix in FirstRelease; no source-present or fallback acceptance |
| Experimental features | PTLAS and source-only external capture | Limited vendor/debug tuples; mandatory tracing and reconstruction have been removed from this category |

No feature is Removed: development experiments have owners and remain useful. Excluded means excluded from this product promise and package, not deleted from source. Source-only features require their own source-adopter proof if advertised.

## Product And Supported Matrix

| Field | Frozen proposal |
| --- | --- |
| Product identity | SparkleEngine `v0.1.0`; portable Windows x64 `ShippingGame`; sole consumer executable `bin/ShowcaseRuntime.exe` |
| Windows | Windows 11 x64 24H2/build 26100 or newer is the target prerequisite; observed machine is Windows 11 Home 25H2, build `26200.9457`. The earlier OS boundary still requires separate acceptance. |
| Named hardware | `MIN-01` / `REF-01`: AMD Ryzen 9 8940HX, 16 cores/32 logical processors; 64 GB installed class, 67,833,212,928 bytes OS-reported physical memory; NVIDIA GeForce RTX 5070 Ti Laptop GPU, 12,227 MiB driver-reported VRAM; driver `610.47` |
| Storage | Patriot P400L 2000GB NVMe SSD; NTFS; at least 16 GiB free for extract/stage/user data. Observed free space is not a reserved budget. |
| Hardware support claim | Initial validated target is this named machine, not every 8 GiB GPU or 16 GiB PC. The former generic floor is a future qualification target, not an accepted minimum. Different desktop/mobile GPUs, drivers and integrated adapters require their own evidence before advertisement. |
| D3D12 | Feature level 12_1, SM 6.6, DXR 1.1; reject missing required features before rendering; supported tuple is named machine plus recorded driver and candidate |
| Vulkan | Vulkan 1.3 plus the [frozen Vulkan feature requirements](../FirstRelease.md#frozen-platform-and-toolchain-prerequisites); same named machine, distinct native validation and performance results |
| Source toolchain | The exact [PD-0 source prerequisites](../FirstRelease.md#frozen-platform-and-toolchain-prerequisites): VS Community 2026 18.7.0/MSVC 14.51.36231, Windows SDK 10.0.26100.0, CMake 4.3.3, Git for Windows 2.54.0, Qt 6.11.1 `msvc2022_64`, Vulkan SDK 1.4.350.0/Slang 2026.8, DXC 1.9.0.5347 |
| Optional development environments | Debug/Development Editor/Game profiles and Launcher remain source products. Alternate compilers/toolchain versions are unqualified, not supported substitutes. |

Two identities on one machine do not produce independent-adopter evidence. `MIN-01` measures the native 1080p baseline; `REF-01` supplies comparison/capture evidence on the same hardware. HDR acceptance additionally requires a recorded HDR10-capable display and OS output configuration; the current display has not been qualified. No hardware purchase is authorized by this scope.

## Baseline And Selection

Release preset `REL-NATIVE-1080P` is 1920x1080 SDR, Lit, raster GBuffer, native render extent with Linear provider, RR Off, classic TLAS/refit On, PTLAS Off, auto batching On, Automatic Histogram exposure, ACES approximation and automatic SDR encoding. D3D12 is the first-run API; Vulkan must produce the same declared behavior. Direct/Indirect diffuse/specular, direct subsurface and shadows retain their declared default states. VSync defaults On for consumers and Off for performance controls. Grading is neutral, chromatic aberration/vignette disabled, no baseline decal actors; all remain Included selectable obligations. HDR10 is opt-in after qualification.

`REL-RT-1080P` is the mandatory realtime path-traced performance preset: 1920x1080 output, traced primary/secondary transport, classic TLAS, DLSS Quality requested and RR On, fixed SDK-derived render extent/topology frozen before measurement, Frame Generation and dynamic resolution Off. The existing Lit/direct/indirect route is the implementation starting point, not proof that this preset exists. Native tracing and RR-Off controls separately measure quality/cost; reconstructed FPS must be labelled with the actual render extent. Neither a Ray GBuffer alone nor a Linear/Off fallback can pass this preset. Freeze finite-depth/reuse/approximation and SDK-supported SR/RR composition in the owning feature discovery, preserving exactly one final image product. Do not silently add a new controller or copy reference-tracer state into the realtime path.

`REL-REFERENCE` exposes the progressive full admitted surface-transport reference in the consumer viewport, with sample/convergence/invalid state, explicit reset/cancel and reproducible raw-linear export. It does not use DLSS, RR, firefly clamping or artistic post effects to manufacture oracle output. Its [PTD-00 domain and two distinct products](../../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/TransportAndEstimator.md#claim-boundary) remain authoritative: full supported surface reflection is required; finite-depth output is diagnostic only. Freeze animated content to a named immutable pose, and reject/reset when contributing scene/camera/material/light generations change. Domain mismatches must close before a release map is used as a reference; “full” does not claim transmission, volume, spectral or physical subsurface transport currently excluded by that contract.

The baseline is a target, not the current repository preset. `Config/DefaultEngine.ini` currently requests VSync Off, ACES filmic, DLSS/NativeAA, ray reconstruction On and PTLAS preference On. `LevelRegistry::EnsureDefaultLevel` currently selects Empty. `PD-1` must replace these consumer defaults without changing development defaults by accident, and prove first-run and unavailable-provider behavior. No production settings were changed in Stage 0.

| Public route | Scope contract | Owning implementation |
| --- | --- | --- |
| API selection | `--graphics-api D3D12` or `--graphics-api Vulkan`; build default then `SPARKLE_RHI_BACKEND`, then CLI; invalid selection fails visibly | Application graphics launch; existing `--renderer` and `--rhi` spellings remain source-development conveniences, not new durable consumer promises |
| First run | No arguments opens Cornell Box at its authored camera with a visible control/help affordance | Application consumer host and GameFramework level session; never treat built-in Empty as package success |
| Map switching | In-product Examples menu lists exactly the two approved maps; switching has progress, failure and return to the previous valid level | Existing level request/session authority; no second map controller |
| Settings | Resolution/window mode, VSync, tracing mode/preset, Included reconstruction and render/post controls with requested/active status; Save, Reset and restart-required indication | Application persistence and Renderer feature authority; consumer presentation and safe unavailable handling remain delivery work |
| Help and quit | F1 help, Escape menu, explicit Quit and native close; input prompts are always discoverable | Application/Platform host |
| Camera | W/S or Up/Down forward/back; A/D or Left/Right strafe; E/Space up, Q/C down; Shift sprint, right mouse held for look, wheel for speed; reset to authored view is required | [Existing Application input collector](../../../Engine/Application/Private/Input/CameraInputIntentCollector.cpp) and GameFramework camera controller; help/sensitivity/reset presentation remains `PD-1` work. Escape releases mouse-look before opening the consumer menu. |
| Development controls | Generic CVar assignment, startup-level environment variable, capture flags, console, shader recook, debug views and authoring actions | Source-only; absent from consumer UI and optional Shipping payload; not a replacement for the public menu |

Consumer input is keyboard/mouse, English UI/help, arbitrary Unicode installation/user paths, windowed and borderless on one active display. Resolution choices are 1280x720, 1920x1080 and 2560x1440 when available; only 1920x1080 carries the performance promise. Native monitor refresh and VSync are reported honestly. DPI checks cover 100%, 150% and 200%. Controller, exclusive fullscreen, multi-window/multi-GPU, VR, localization, screen-reader certification and a broad accessibility certification are excluded. Keyboard navigation, readable labels, scalable text and visible errors remain required basics. HDR10 requires a qualified display/OS tuple; no silent claim from swapchain format vocabulary.

## ReleaseMapSet

Only these two existing level families are proposed for the consumer package. Catalog selection is a development acquisition setting, not package membership. Source-only maps remain usable through development workflows. Rights classification is not legal clearance.

| Level | Product classification | Fixed view/route | Input and rights decision |
| --- | --- | --- | --- |
| `CornellBox` | Included; first-run/default | Authored camera `(0, 1, 3.8)`, yaw pi, pitch 0, FOV 45; fixed camera, 300 warmup + 300 sampled frames, three repeats | Existing publisher archive revision 2011-07-27; catalog SHA-256 `f27eaf47afec74b5236ce3095ac02d4256e8ac8610bfdb5a53560c67c2e0d8f1`. Inspected local publisher notice states CC BY 3.0; original OBJ additionally states public domain. [Owned conversion/attribution record](../../../Projects/Showcase/Assets/Meshes/CornellBox/Conversion/README.md) names Guedis Cardenas/Morgan McGuire. Current GLB hash matches its documented `a2af112abeaa1b08087009afa3efef2c6b665c59bd7a6efe70d703e8f2349d91`. Retain complete redistribution/cook receipts in `REL-01`; rect-light behavior still needs its real feature proof. |
| `CesiumMan` | Included; animation example | Authored camera `(0, 1.5, -3)`, yaw/pitch 0, FOV 60; stationary camera, animation running; same sampling protocol | [Pinned upstream metadata](https://github.com/KhronosGroup/glTF-Sample-Assets/blob/edc7c9e67c639d230715049ee31f9a96a6babbbe/Models/CesiumMan/README.md) identifies CC BY 4.0 and separate logo/trademark notice. Local model JSON matches this revision after external URI relocation; both referenced buffer/texture bytes match exactly. Retain attribution and separate logo notice; no endorsement claim. Cooked animation/material fidelity and final notices remain packaging proof. |

The two proposed package routes are deterministic observations, not new camera-path infrastructure. Captures record level/cooked-generation identity, camera, animation time, resolution, settings, driver and backend. Reference comparison uses an exact frozen pose; realtime performance uses the running clip. Changes invalidate evidence. Small scenes alone cannot prove the requested whole-frame depth: existing [Bistro/San Miguel workloads](../GraphicsWorkloads.md) remain required flagship/cross-scene quality and frame-review inputs. `BistroExterior` is the additional source-workload performance target for `REL-RT-1080P`; freeze its existing authored camera and named motion/disocclusion cases through MAP-00 before runs. Its exclusion from the consumer archive is not exclusion from realtime/frame-review qualification, and shareable content/capture rights still apply.

Retained local source receipts are `build/validation/release-scope-20261010/content-receipts.json`: Cesium Man input identity passes; Cornell archive re-fetch returns HTTP 406 both with the default request and explicit browser User-Agent/Accept headers. Its local notice/original OBJ/MTL and matching documented GLB hash are retained, but a fresh archive-to-source match is unproved. `REL-01` content owner must supply/verify the pinned archive receipt. This failure does not establish that the asset is unsupported or change its proposed product classification.

| Other current level entries | Consumer decision and reason |
| --- | --- |
| `Empty` | Excluded; source smoke fixture, never an intentional consumer example or fallback success |
| `Sponza` | Excluded pending exact redistribution decision; [pinned current upstream metadata](https://github.com/KhronosGroup/glTF-Sample-Assets/blob/edc7c9e67c639d230715049ee31f9a96a6babbbe/Models/Sponza/README.md) references a CRYENGINE-specific agreement, not a generic CC license |
| `DamagedHelmet` | Excluded pending exact provenance/rights decision; [pinned current metadata](https://github.com/KhronosGroup/glTF-Sample-Assets/blob/edc7c9e67c639d230715049ee31f9a96a6babbbe/Models/DamagedHelmet/README.md) lists both attribution and an earlier noncommercial contribution |
| `ABeautifulGame`, `DiffuseTransmissionPlant` | Excluded from consumer package; do not advertise transmission/volume support through a model that depends on those semantics. A Beautiful Game's upstream CC BY 4.0 notice does not prove local material fidelity. |
| `ModernSponza`, `ModernSponzaCandles`, `ModernSponzaKnight`, `JungleRuins`, `BistroExterior`, `BistroInteriorWine`, `SanMiguelHigh`, `SanMiguelLow` | Excluded from the small consumer package; retained source/development workload families with acquisition, resource and rights obligations |
| `LPSHead`, `StanfordBunnySubsurface`, `StanfordDragonSubsurface`, `AnisotropyBarnLamp`, `SheenCloth` | Excluded from consumer package; dedicated source workloads, not accepted subsurface/anisotropy/sheen product demonstrations |

The live catalog has **20 `[Level]` records and 17 `[AssetPack]` records**. There is no claim that all are ready, cooked, legally cleared or runtime supported. Every record is accounted for above; future entries require a scope decision.

## Mutable Data And Compatibility

The [PD-0 path and budget freeze](../FirstRelease.md#frozen-paths-mutation-boundary-and-budgets) remains authoritative. Immutable package bytes may be extracted to a read-only standard-user location. User data belongs below `%LOCALAPPDATA%\SparkleEngine\Showcase\v0.1\`, with separate Settings, Logs, Captures, Crashes and Cache folders. No save-game promise. No repository, development SDK or writable extraction-root requirement.

| Boundary | Publication promise |
| --- | --- |
| Package/map identity | Published tag, manifest schema identity, archive/checksums and cooked-generation hashes are immutable per version. Public map IDs are exactly `CornellBox` and `CesiumMan` for `v0.1.x`; schema and bytes can change only with a new release identity. |
| User settings | Documented Included setting names remain interpretable within `v0.1.x`; invalid/out-of-range values yield a visible safe reset, not undefined behavior. New optional keys may be added. Development-only keys are unstable. No compatibility reader is introduced in this scope pass. |
| Cache | Disposable, generation/version-qualified; invalidate/rebuild rather than promise cache-format compatibility. Clearing Cache does not remove settings. |
| Saves | Excluded; no save-game migration, mod/save format or synchronized account state |
| CLI/environment | `--graphics-api` and `SPARKLE_RHI_BACKEND` values/precedence documented for `v0.1.x`; unknown backend rejects. Other development CLI/env contracts are version-specific and unstable; packaging must not depend on them. |
| Source API / binary ABI | Exact public tag builds on the pinned toolchain; unstable source API across `0.y.z`; no supported binary ABI/plugin SDK. Patch release does not rename documented consumer controls or silently alter accepted map semantics. |
| Upgrade/rollback | Manual download, verify, extract into a new version directory; never overwrite the immutable prior archive. Revert by launching the previous verified extraction. Back up Settings before a patch; restore that backup if rolling back. No automatic update or destructive migration. |
| Side-by-side | Separate extraction roots. `v0.1.x` shares the declared `v0.1` settings root; different minor versions use separate roots. Concurrent use of two versions writing the same settings is unsupported and must be stated. |
| Reset/removal | Reset settings through the product action or remove only its user settings file; clear each optional data folder separately. Delete extraction to remove the product; separately opt in to deleting its per-user data. |

## Budgets And Observation

The [PD-0 operation budgets](../FirstRelease.md#frozen-paths-mutation-boundary-and-budgets) are preserved: first interactive UI <=15 s, first example <=30 s, other release map <=60 s, quit <=5 s; archive <=4 GiB/extracted <=8 GiB; process peak commit <=12 GiB; tracked local GPU peak <=7 GiB; no unbounded growth. Source cold/warm build-cook and Launcher deadlines remain in that owner. These are operation limits, not calendar delivery targets.

Both `REL-NATIVE-1080P` and `REL-RT-1080P` must meet the [30 FPS contract](../FirstRelease.md#thirty-fps-performance-floor) on MIN-01: ShippingGame, VSync Off, no capture/validation/profiler, >=300 warmup and >=300 valid samples, three repeats per selected map **and per API**. The native baseline runs each consumer map; realtime also runs BistroExterior as above. AC power, fixed Windows/OEM performance mode and steady thermals are required. Presented/app p95 and separate CPU/GPU p95 must each be <=33.33 ms; report p99, 1% low, worst frames, memory and clocks. The native baseline uses no DLSS; realtime uses its declared fixed DLSS Quality/RR topology, with no generated frames, dynamic resolution, provider fallback or settings retuning after results. The progressive reference instead carries its own convergence/cancellation/resource budgets, not a 30 FPS requirement. The [principal review packet](../FirstRelease.md#principal-frame-review) consumes separate captures and observer-controlled timing records.

## Network, Security, Publication And Support

| Topic | Finite promise and owner |
| --- | --- |
| Runtime network | Offline runtime. No telemetry, account, analytics, remote asset acquisition, update polling or automatic crash upload. Support links open only on explicit user action. Application/package owner verifies actual traffic and imports. |
| Source acquisition | HTTPS access for public Git/dependency/content acquisition; pinned revisions/hashes, no private credentials; offline consumer package remains independent. Build/content owners retain receipts and SBOM. |
| Privacy | Local logs/captures/crashes may expose user paths, scene content and memory. Reports are voluntary; explain/redact before upload. Dumps and private shader/CPU symbols are retained locally or shared privately with explicit consent, never attached automatically to public issues. |
| Distribution | Proposed repository GitHub Releases channel, immutable `v0.1.0` tag and versioned archives; exact repository URL and publisher identity must be fixed by `REL-01`, not inferred from this local folder. |
| Trust | Trusted Authenticode for every shipped PE; SHA256SUMS, manifest, dependency/content notices, SBOM, build provenance and vulnerability/malware disposition. Separate symbols archive governed by consent/privacy policy. Keys/accounts remain outside source and archives. Unsigned builds remain developer prereleases. |
| Support | Release owner is the repository maintainer. Public issues for reproducible non-sensitive defects; private security intake with published contact is mandatory before publication. No commercial SLA or always-on support promise. |
| Supported version | Support the `v0.1.x` line until an accepted replacement is published and this line is explicitly retired; no calendar release target. Retain hashes/notices/known issues/advisories for published versions. |
| Response policy | Preserve PD-0 acknowledgement targets: public defect <=5 business days, private security <=3 business days; confirmed S0 containment/withdrawal <=1 business day. Acknowledgement is not a promised fix date. |
| Patch/withdrawal | Triage severity; fix within scope, cut a new immutable patch, rerun invalidated gates, and publish release notes/checksums. Unsafe payload is withdrawn with a visible advisory and verified rollback instructions. Never replace existing signed archive bytes silently. |
| Consumer limitations | Named hardware/API matrix only; no guaranteed NVIDIA/AMD/Intel-wide support, neural training/inference platform, multiplayer, networking, mod ecosystem, stable SDK, installer/updater, Linux/macOS, mobile or VR. No new scene families; display additions are the five admitted decals/grading/chromatic/vignette/HDR10 targets. AMD specialist profiler work remains out of scope. |

The support mailbox, publisher/signing credential, distribution URL and complete dependency/content rights are **unprovisioned obligations**, not working services. Current root `LICENSE.txt` uses MIT text with `[year] [fullname]` placeholders; `REL-01` must establish the actual copyright/publisher identity and third-party notices. It must also prove redistribution for the published source tree, not merely the two runtime models. This record chooses policy; it does not invent a contact address, copyright owner or signing certificate.

## Approval And Next Work

| Item | Decision state / next owner |
| --- | --- |
| Named minimum/reference laptop | **Approved by user** in this session; OS/storage/CPU/GPU inventory retained locally; no performance verdict |
| Rendering amendment | **Owner-directed requirements recorded**: full admitted progressive reference, realtime >=30 FPS, complete SR/RR, exposure/tone/lens features, parity, graph and principal review. Required feature cells are Included; no correctness/acceptance promotion. |
| Other scope, two-map set and support/compatibility proposals | Still proposed; the rendering amendment does not imply blanket approval of unrelated choices. |
| `REL-00` | **BLOCKED for remaining scope approval/rights/domain/preset receipts**. Requested rendering requirements have been incorporated; no longer waiting to learn what the owner wanted changed. No `R`, `N` or `P` evidence is added. |
| `REL-01` discovery | Ready to inspect the two selected assets and their dependency closure, repository license/publisher, exact acquisition pins, signing/distribution/security intake; retain receipts before any package claims |
| `PD-1` implementation | Waits for approved `REL-00` and applicable mother-plan prerequisites. Consumer first-run/menu, settings validation, Shipping console isolation and changed defaults remain concrete TODOs. |
| Later release gates | Build/cook/package, required missing rendering features, HDR display qualification, per-API performance, controlled failures, clean-machine adoption and publication remain unrun in Stage 0 |

## ITER-REL-00-01 Control Record

| Field | Record |
| --- | --- |
| Scope / owner | Stage 0 only; repository maintainer/release owner. Start revision above, clean tree. Documentation/source/host inspection, no production or build configuration changes. |
| North Star / persona | `NS-REAL`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY`; `PGE-01`, `PGE-05`–`10`, `PGE-13`, `PGE-15` advance scope clarity only; mathematical/graphics proof preserved, not increased. |
| Delivery mapping | `REL-00`, `FR-00`, `RISK-REL-01/05/09/11/12/13`; all inventory owners and `FCR-*` routes retained; `PTD-00` accepted development discovery preserved, not rerun or promoted to oracle proof. |
| Complexity / preservation | Two subject-local scope/ledger documents; navigation edits only. No new production API, data copy, dependency, implementation plan, test harness, legacy alias or runtime mechanism. Preserve PD-0 budgets and mandatory feature obligations. |
| Risk `RISK-SCOPE-01` | Historical inventory/count/default mistaken for live support -> overbroad product claim. High likelihood from dated snapshot; high impact. Detect via live catalog/settings/enums/target membership and exhaustive ID reconciliation; reduce/fix proposal and hold approval. Release owner retires with approved ledger and retained source inputs. |
| Risk `RISK-SCOPE-02` | Third-party sample availability mistaken for redistribution/material fidelity -> invalid public package. High likelihood from mixed license and extension metadata; critical impact. Detect through primary metadata, asset hashes/notices and material-consumer trace; exclude ambiguous content pending receipts. Content owner retires through `REL-01`, not this source audit. |
| `AC-SCOPE-01` | Every existing module inventory capability ID has exactly one disposition and module owner; all 20 level entries, 18 view enum values and 26 persisted selector names (25 common plus one conditional Editor setting) have an explicit distribution/classification decision. |
| `AC-SCOPE-02` | Product/audiences, named machines, baseline, maps, budgets, mutable roots, compatibility/security/support policies are concrete and trace to the acceptance contract. Mandatory Included targets remain Included; no executable acceptance is inferred. |
| `FM-SCOPE-01` | Missing/duplicate/unmatched row, new catalog entry or selector -> coverage check fails, retain source input, revise ledger before approval; an unexplained omission is a scope blocker. |
| `FM-SCOPE-02` | Unknown asset right or unqualified feature -> explicitly unresolved/excluded, never approved by source presence; retain primary metadata and request owner disposition before package work. |
| Checks | `CHK-SCOPE-01`: exact ID/set coverage, source-reference existence, catalog/view/settings reconciliation and mandatory-feature assertion. `CHK-SCOPE-02`: local Markdown links/anchors, UTF-8, stale names and whitespace. `CHK-SCOPE-03`: review against each `REL-00` clause, public-versus-source boundary and existing PD-0 constants. Negative controls reject missing/duplicate IDs and an invalid disposition using in-memory inputs; no permanent probes. |
| Result / invalidation | **SUPERSEDED for amended documents:** the original 511-ID static slice remains retained in `build/validation/release-scope-20261010/`; amended requirements/classifications are rechecked in ITER-REL-00-02 below. Rights receipts remain useful within their stated identities. No build/cook/runtime/native/performance/package/adopter verdict is added. |

## ITER-REL-00-02 Rendering Amendment

| Field | Record |
| --- | --- |
| Scope / start | Owner-requested revision of Stage 0 only, at `93e86b8113c3fcd0c4921729c3513538b95bc794`; the preceding eight-document scope draft is pre-existing owned work and preserved. No production/CMake/configuration changes. |
| Outcome / mappings | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`; `PGE-02/05/06/08/09/13/15` advance requirement precision only. `REL-00/04/06/07`, FCR-REN-02/06/07/08/09/10/14/15/25 and existing domain/feature checks. No score or native acceptance increase. |
| Structural budget | Amend existing scope/ledger/feature/plan/report owners; one small Vignette dossier and target inventory ID REN-POST-14. FCR-REN-25 owns two independently proved lens results; retain 49 families, remove weaker Experimental/source-only classifications for mandatory rendering, preserve PTD-00 and separate graph/feature/native authority. |
| `RISK-SCOPE-03` | Required reference/realtime/provider/parity work remains labelled experimental or source-only -> release escapes the requested quality target. High likelihood from previous proposal; critical impact. Detect via disposition/surface/mode assertions, numeric preset review and owner/phase crosswalk; fix docs and hold acceptance. Release owner retires through approved scope plus real feature/performance/review reports. |
| `AC-SCOPE-03` | Required tracer/reconstruction rows and Reference mode are Included/runtime; vignette has one owner/ID/dossier; two independent lens proofs preserve the 49-family registry. Existing exclusions remain explicit. |
| `AC-SCOPE-04` | Native/realtime/reference presets, >=30 FPS and reference truth have distinct proof contracts; parity requires both APIs; principal review requires retained independent identity/findings and full frame evidence. No fallback, finite diagnostic, generated frame, unrun reviewer or metadata can pass those gates. |
| `FM-SCOPE-03` | Missing vignette/duplicate ID or a mandatory row reverted to Experimental -> static coverage/required-row checks reject; revise before freeze, no production mutation. |
| `FM-SCOPE-04` | Source/SDK/example output treated as feature completeness -> retain the current D3D12-only initialization gap, PTD domain/uncertainty limits and unrun review/performance status; acceptance stays blocked. |
| Checks / artifacts | Exact inventory/ledger/class/mode/settings coverage, negative in-memory omission/duplicate/weakened-class controls, local links/anchors, UTF-8, whitespace, production-input hash preservation and `git diff --check`; retained commands/results in `build/validation/release-scope-amendment-20261010/`. Native builds/runs/review are not appropriate proof for this documentation-only amendment and remain unrun. |
| Static result | **PASS:** 138 checks, 22 changed/new documents, 622 local links, 152 anchors, 13 unchanged production inputs and three rejected in-memory negative controls at the revision above. Coverage is 512 capability IDs across 16 inventory owners; the existing 49 report families remain unchanged. This accepts documentation consistency only. |
| Disposition / next work | Rendering requirements incorporated; REL-00 remains BLOCKED for remaining scope/rights/domain/preset freeze. Execute dependency-ready source/SDK/domain discovery through existing owners; implementation requires its own authorized phase and checks. Preserve numerical budgets and do not retune them to candidate results. |
