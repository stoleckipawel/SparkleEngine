# First Release Acceptance Contract

Status: binding acceptance contract for the first public engine release; requirements are not completion claims

Release target: SparkleEngine `v0.1.0`, Windows x64

Roadmap and sequence: [F. Release-First Principal Graphics Roadmap](../Strategy/Roadmap.md)

Detailed graphics workloads: [I. Bistro and San Miguel Acceptance Workloads](BistroAndSanMiguelWorkloads.md)

Validation policy: [Validation, Performance, and Evidence](Standards/ValidationPerformanceAndEvidence.md)

## Purpose And Authority

This document defines the proof required to call the first SparkleEngine release shippable. It does not prove that any requirement currently passes, prescribe subsystem architecture, or authorize implementation by itself. Code and executable build configuration prove what exists; a release evidence package proves what was exercised.

The first release is a product-delivery milestone, not a promise that every source-present experiment is production-ready. Every user-visible capability MUST be classified before the release candidate is cut:

- **Included** — reachable in the shipped product and required to pass every applicable acceptance gate below;
- **Experimental** — intentionally exposed, unmistakably labelled, bounded by documented limitations, and still required to fail safely;
- **Excluded** — not selectable, packaged, advertised, or used by the release path;
- **Removed** — deleted when it has no current owner or valid release purpose.

An unclassified or silently broken option blocks release. Excluding an unfinished feature is acceptable; shipping it as if it were complete is not.

## What The First Release Delivers

The default binary delivery is a portable Windows x64 archive. An installer is out of scope unless a later gate proves it is necessary. The release has four independently identifiable outputs:

1. an annotated release tag and immutable source snapshot, signed when the approved signing policy requires it;
2. `SparkleEngine-v0.1.0-windows-x64.zip`, containing the `ShippingGame` Showcase runtime, redistributable dependencies, cleared sample content, notices, and user documentation;
3. `SparkleEngine-v0.1.0-symbols.zip`, retained or published separately according to the support policy;
4. `SHA256SUMS.txt`, release notes, known issues, and exact build provenance. The runtime archive contains the authoritative `manifests/sparkle-package-manifest.json` used by package discovery.

The launcher, editor, cooker, shader compiler, and other authoring tools are developer-path products. They MAY be added to a separately named developer archive only after their own package and clean-machine gates pass. They do not delay the runtime release merely because they exist in source, provided the public documentation clearly separates build-from-source tooling from the redistributable runtime.

## Release Audiences

Calling this an engine release creates two required acceptance paths. Passing only one does not make SparkleEngine shippable.

| Audience | Required promise | Required shortest journey |
| --- | --- | --- |
| Runtime consumer | The packaged Showcase experience runs safely and as advertised without source, build tools, environment variables, administrator rights, or network access. | Download -> verify -> extract -> launch -> select/run examples -> inspect controls/settings -> exit -> remove. |
| Source adopter | The tagged source can produce the documented engine, tools, cooked content, and Showcase runtime using only public prerequisites and instructions. | Clone/download -> verify dependencies -> configure -> build -> cook -> run one example -> locate architecture/API stability/support policy. |
| Contributor | Contribution workflow is discoverable and failures are actionable. | Optional for `v0.1.0`; if advertised, it receives its own accepted workflow. |
| Binary SDK/plugin consumer | Stable installed headers, libraries, ABI, samples, and compatibility rules. | Excluded from `v0.1.0` unless explicitly added and proven; exported C++ symbols alone are not an SDK. |

The source API may remain unstable during `0.y.z`, but the exact contents of a published version are immutable. Public documentation MUST distinguish source availability from a supported binary SDK and name what compatibility, if any, a patch or later minor release preserves.

## Completion Vocabulary

Use these states in release trackers and feature records. Do not use “done” or “feature complete” as substitutes.

| State | Meaning | Permitted public claim |
| --- | --- | --- |
| `Source present` | Relevant code or configuration exists and was statically inspected. | None about runtime correctness. |
| `Integrated` | The owned producer/consumer path builds or runs in a focused check. | The named integration path exists. |
| `Verified` | Required correctness, quality, performance, and failure evidence passes on the declared matrix. | The exact verified configuration and limitation. |
| `Release candidate` | The frozen commit and staged package pass all pre-publication gates. | Candidate for the named release, not yet shipped. |
| `Shippable` | Independent package acceptance passed and the approval record is complete. | Shippable on the named platform and matrix. |
| `Published` | Immutable artifacts were uploaded and post-publish retrieval was verified. | Released at the exact version and artifact hashes. |
| `Blocked` | A required result is missing, failed, unavailable, or inconclusive. | The blocker only. |

Evidence expires when relevant code, content, configuration, toolchain, dependency, driver, or package bytes change. The affected gate returns to `Blocked` until rerun.

## Current Release Audit

Audit date: 2026-09-06

Audit basis: static source/build/document inspection of `master` at `f61cabc4` with 331 changed worktree paths. No build, cook, package, clean-machine run, graphics capture, or benchmark was performed for this audit.

| Release area | Current source-inspected state | Release status and consequence |
| --- | --- | --- |
| Product identity | The CMake project has no declared version, the root license retains `[year] [fullname]`, no root README exists, and no Windows version-resource file was found. | `Blocked`: freeze name, semantic version, publisher, license, support promise, and executable metadata. |
| Build profiles | `DevelopmentEditor`, `DevelopmentGame`, `DebugEditor`, `DebugGame`, `ShippingEditor`, and `ShippingGame` profiles exist. Showcase editor/runtime and launcher executable targets exist. | `Source present`: a profile name is not a reproducible release build. |
| Package discovery | Runtime source recognizes `manifests/sparkle-package-manifest.json`. | `Source present`: no owned CMake `install()` rules, CPack configuration, staging contract, or produced release archive was found. |
| Package writable state | Package-mode paths currently place `build`, `logs`, captures, and persisted `Config/DefaultEngine.ini` beneath the discovered package/workspace root. | `Blocked`: a read-only extraction or protected installation directory is not proven. Define one per-user mutable root or explicitly constrain and prove portable writable mode; shipped configuration remains immutable. |
| Consumer first run | The runtime defaults to `Empty`; startup-map selection is environment-driven, the launcher owns discoverable per-level run actions, and the runtime console option defaults enabled. | `Blocked`: freeze one consumer-safe first-run, example-selection, controls/help, settings/reset, and quit path. Do not expose development console/tool operations accidentally. |
| Missing-content behavior | Catalog/load failures can fall back to the built-in `Empty` level. | `Blocked`: a required packaged map, catalog, shader, or content failure must not masquerade as a successful empty scene. Reserve fallback only for an explicitly requested recovery path. |
| Core/platform/tasks | Platform, application, task, filesystem, logging, input, and lifecycle owners exist. | `Source present`: standard-user paths, shutdown, cancellation, resize, and repeated-run behavior remain unproven as release claims. |
| RHI | D3D12 and Vulkan implementations, native validation switches, timing, capture, and ray-tracing paths exist. | `Source present`: native validation cleanliness, capability failure, resource lifetime, and device-removal behavior require evidence. |
| Renderer | Raster, deferred/PBR, exposure, tone mapping, debug views, upscaling, ray/path, ReSTIR, scene/view/frame, and GPU-scene paths are present. | `Source present`: each included mode needs a frozen feature row, map evidence, performance result, and backend disposition. |
| World/content | World, scene, component, map catalog, import, cook, shader cook, and runtime asset paths exist. | `Source present`: deterministic clean cook, package-relative load, malformed-input handling, and bounded memory need proof. |
| Showcase content | The catalog contains 16 level records; 13 are described by the current workload audit as runtime-supported and three as source-readiness-only. External packs have mixed licenses and support states. | `Blocked`: freeze a redistributable `ReleaseMapSet`; runtime support does not establish redistribution rights or visual acceptance. |
| Evidence harness | Level-selected launch and manual viewport capture exist; backend timestamp infrastructure exists. | `Blocked`: `MAP-00` still owns fixed resolution, readiness/settled identity, named capture sidecars, timing export, and a unified manifest. |
| Automated validation | No `enable_testing()` or `add_test()` registration was found; `ShaderCompilerCliValidation` is a focused custom target. | `Blocked` for regression confidence. The release owner must explicitly authorize any permanent submitted test code required by the binding test policy; until then, use approved executable checks and retained evidence without claiming automated coverage. |
| Dependency integrity | Most dependencies name tags/commits or archive hashes, but `stb` uses `master` and the sparse Compressonator clone follows its default branch. | `Blocked`: a clean build can resolve different source over time. Pin every release input to an immutable commit/archive hash and record license/provenance. |
| Optional provider payload | NVIDIA Streamline is enabled by default and the generic product staging list includes release and `_d` debug-suffixed DLLs. | `Blocked`: classify the provider, redistribution terms, hardware fallback, signing, and exact Shipping DLL closure; debug/development payload cannot enter by generic copy. |
| Crash/support path | D3D12 DRED collection exists, but the focused scan found no first-party process-level minidump/unhandled-exception owner or public security/support route. | `Blocked`: choose an OS WER/local-dump or owned crash policy, consent/privacy boundary, support bundle, symbol retention, and hang/device-loss instructions. |
| Installed-consumer policy | The clean-break standard currently assumes no active users, shipped compatibility contract, persisted user data, public SDK ABI, or installed consumer. | `Blocked before publication`: `REL-10` would invalidate that premise. Freeze the external compatibility/reset/update contract and update the standard before shipping. |
| Source adoption | Public DLL headers and source-build machinery exist, but no root quick start, supported source toolchain matrix, installed SDK contract, or independent source-adopter record exists. | `Blocked`: prove the tagged clone/configure/build/cook/run journey and explicitly exclude binary ABI/plugin support if it remains unsupported. |
| Distribution and support | Development artifact staging exists, but no release staging/package owner, dependency manifest, checksum set, clean-machine record, or support/crash policy was found. | `Blocked`: implement and prove the complete distribution path. |

Nothing in this table is accepted merely because a source route exists. The worktree must be deliberately reconciled into a clean candidate commit before reproducibility evidence begins.

### Current Feature-Inventory Seed

The static scan found the following closure families. This is a seed for `REL-00`, not a substitute for tracing the actual UI, configuration, producers, consumers, and package reachability.

| Family | Source-present surfaces to classify |
| --- | --- |
| Build and products | Six build profiles; shared engine DLLs; Showcase editor/runtime; launcher; import, cook, shader, and support tools; dependency/runtime staging. |
| Core, platform, and tasks | Math/memory/events/logging; filesystem/project discovery; Win32 window/input/focus; task graph, scopes, events, parallel-for, workers, cancellation, and profiling hooks. |
| World and scene | Entity/component/world ownership; transforms; cameras; directional/point/spot/rect lights; sky; static/skeletal meshes; materials; animation/skinning/morph data; level parsing/loading/session and cooked registries. |
| RHI and presentation | D3D12 and Vulkan devices, queues, resources, descriptors, bindings, command lists, swapchain/presentation, frame latency, timestamps, captures, classic/partitioned TLAS, inline ray queries, and native ray-tracing pipelines. |
| Renderer | Scene/view/frame submission; persistent GPU scene and caches; scene depth/GBuffer; deferred PBR; sky; direct lighting/shadows; lighting composite; exposure; tone mapping; output encoding; buffer visualization; linear upscaling; ray-traced GBuffer; path-traced direct/indirect; reference accumulation; direct/indirect ReSTIR routes. |
| Editor and application | Editor viewport/session/camera; rendering/upscaling settings; scene selection/transactions; viewport capture; UI frame; runtime/editor hosts; shader source-change and recook coordination. |
| Import and cooking | Source importers; scene, mesh, material, texture, and asset cookers; texture decoding/shape/mips/channels/BC compression; cooked scene/mesh/material/texture/animation/skeleton/shader products. |
| Launcher | Toolchain/source discovery; configure/build/cook; level catalog and asset-pack sync; editor/runtime launch; progress/cancel/failure; maintenance and clean operations. |
| Project and content | Showcase configurations, 16 catalog level records, built-in assets, external pack recipes, level thumbnails/descriptions, and startup-level selection. |
| Documentation and delivery | Architecture/standards/workload routes; package discovery manifest; user quick start, requirements, notices, package/readme, support, crash handling, checksums, and release publication. |

Every item begins as `Source present`. `REL-00` splits compound rows into independently selectable features and finds any missing families before scope approval.

## Release Scope Freeze

`REL-00` produces one version-controlled release-scope record. It MUST name:

- public product name, version, platform, architecture, minimum Windows version, and supported GPU/API matrix;
- minimum and reference CPU, memory, GPU, driver, and storage configurations;
- the exact runtime executable and default graphics configuration;
- required audiences, supported source toolchain, public API/ABI stability, persisted-data compatibility, reset, update, downgrade, and side-by-side-install policy;
- every selectable render mode, debug view, upscaler, lighting path, and backend as `Included`, `Experimental`, or `Excluded`;
- the `ReleaseMapSet`, meaningful first-run/default map, consumer map-selection path, fixed cameras/routes, and redistribution disposition;
- source-only versus packaged developer tools;
- network requirements, prerequisite installer policy, immutable installation root, per-user save/cache/config/log/capture/crash locations, reset, and uninstall instructions;
- first-interactive-frame, map-load, shutdown, package-size, installed-size, disk-space, CPU-memory, and GPU-memory budgets in addition to the fixed 30 FPS floor;
- SDR/HDR, resolution/refresh, DPI, monitor, keyboard/mouse/controller, language/locale, and accessibility scope;
- telemetry/network behavior, privacy/consent, security-reporting, dependency/SBOM, vulnerability, malware-scan, and release-provenance policy;
- support duration, issue-reporting route, crash-data policy, known limitations, and explicit non-goals;
- distribution channel and executable/archive/tag signing policy. Direct public Windows downloads require trusted Authenticode signatures for every shipped PE file; an unsigned build is a developer/pre-release artifact, not this consumer release.

Any later scope addition returns `REL-00` to `Blocked`. Defect fixes within frozen scope do not.

## Release Roles

One person may hold several roles, but the independent acceptance run MUST be performed by someone who did not prepare the candidate on that machine.

| Role | Decision owned |
| --- | --- |
| Release owner | Scope, supported claims, severity/waiver decisions, candidate go/no-go, publication, and stabilization window. |
| Build/package owner | Reproducible build, stage/install tree, dependencies, manifest, hashes, archive, and symbol retention. |
| Feature owners | Included/experimental/excluded classification, focused correctness/failure evidence, defects, and invalidated-gate reruns. |
| Content/provenance owner | `ReleaseMapSet`, source identity, license/attribution/redistribution record, cook completeness, and PBR/reference review. |
| Evidence reviewer | Measurement contract, raw-artifact integrity, native validation disposition, comparison thresholds, and honest limitations. |
| Independent acceptor | Clean-machine public-instructions run and reproduction report; does not repair the candidate during acceptance. |

The release owner resolves disagreement by reducing scope or holding the release, never by relabelling missing evidence as a pass.

## Consumer Journey Acceptance

The runtime-consumer journey is accepted only from the candidate archive and public instructions:

- the download page states version, archive size, installed size, free-space requirement, system requirements, supported GPUs/drivers/APIs, known issues, checksum, signer/publisher, and support link before download;
- every shipped `.exe` and `.dll` has an expected trusted Authenticode signature and publisher; signature verification and current Windows Smart App Control/SmartScreen behavior are recorded on a clean machine;
- extraction and first launch require no administrator rights, registry edit, environment variable, developer SDK, compiler, source checkout, network connection, or writable installation directory;
- first launch reaches an intentional accepted example or consumer menu within the frozen startup budget. `Empty`, a developer console, or a fallback scene is not the public first impression unless deliberately selected and explained;
- one discoverable product-owned path selects every shipped example and advertised backend/mode. A developer-only environment variable is evidence plumbing, not consumer UX;
- controls/help identify movement, look, speed, map/mode selection, settings/reset, capture if advertised, and a reliable quit path. Unsupported controller, HDR, accessibility, language, or display behavior is explicit;
- settings persist under the declared per-user root, survive a normal restart, reject or reset corruption safely, and can be reset without deleting the installation;
- the runtime makes no undeclared outbound connection and does not collect or upload logs, dumps, hardware identity, or usage data without explicit consent;
- normal removal deletes only installed files. Per-user data retention/deletion is documented and never removes unrelated data.

The first-run record includes screen capture, elapsed startup/map-load/exit times, process exit status, created/modified filesystem paths, network observation, signature results, and the exact package hash.

## Source-Adopter Acceptance

The engine-adopter journey is accepted separately from the binary package:

- a fresh tagged clone or source archive verifies the release identity and contains no generated build products, private credentials, local machine paths, or unlicensed payload;
- every fetched dependency is pinned to an immutable commit or archive hash, appears in the dependency/provenance inventory, and has a resolved license/notice disposition;
- the documented Windows, compiler, SDK, CMake, generator, Git, Qt, Vulkan, and optional-vendor prerequisites distinguish required, optional, and auto-fetched inputs;
- an empty dependency/build cache can perform the documented sync, configure, Shipping build, shader/content cook, and Showcase run without private knowledge. A second run proves the warm/offline-cache path or clearly states that network is required;
- one minimal command path builds the runtime; optional editor, launcher, import/cook, ray-tracing, and vendor-provider paths are separately selectable and fail at configure time when prerequisites are absent;
- `Showcase` is the supported sample project. The instructions identify where to add a project, where generated/cooked data lives, how to clean it, and which interfaces are stable or deliberately unstable;
- warnings, failed downloads, missing tools, unsupported CPUs/GPUs, bad cached dependencies, and cook errors stop with actionable ownership and recovery instructions;
- a non-author completes the journey and reports elapsed time, download/build/disk cost, commands, failures, and one technical criticism.

No binary SDK, plugin ABI, installed headers/libraries, or compatibility promise may be inferred from source-export macros. If those are excluded, the release page says so plainly.

## Compatibility Boundary Created By Shipping

Publishing creates the first installed consumers and persisted user state. Before `REL-10`, update the [current clean-break policy](Standards/IntegrationStyleGuide.md#current-clean-break-policy) so it no longer claims those consumers do not exist and so future changes know which external boundaries are durable.

The decision MUST classify:

- release/tag/package identity and the immutable bytes of every published version;
- command-line options, package manifest, map IDs, user-facing settings, file locations, and support-data schema;
- persisted configuration/cache/capture/crash data as preserved, resettable, disposable, or unsupported across patch/minor releases;
- source API stability during `0.y.z` and the explicit absence or presence of a binary ABI/plugin/SDK contract;
- patch, minor, rollback, downgrade, and side-by-side behavior, including how a bad configuration or cache is recovered;
- support duration and how security/correctness fixes produce new versioned artifacts instead of mutating an old release.

`v0.1.0` may declare its source API unstable, but that does not permit replacing published `v0.1.0` bytes, corrupting consumer data, or silently changing its documented behavior. This planning document does not authorize compatibility machinery before the owning policy is approved.

## Supply-Chain, Security, And Privacy Acceptance

- Freeze every compiler, SDK, dependency, optional provider, model/data artifact, external map archive, license, and build-script input by immutable identity. Moving branches such as `master`/`main` block release reproducibility.
- Produce an SBOM or equivalent machine-readable component inventory for the runtime and a broader source-build dependency inventory. Include name, version/commit, source, hash, license, linkage/distribution role, shipped files, and owner.
- Review known vulnerabilities for shipped/reachable components. Known exploited or critical issues block; a high-severity waiver requires reachability analysis, mitigation, owner, expiry, and public impact statement.
- Scan source and staged output for credentials/private keys, unexpected executables, malware, debug/provider DLL contamination, absolute developer paths, and files absent from the allowlisted stage manifest.
- Sign first-party Windows PE files with the approved trusted identity, timestamp where applicable, then verify every first- and third-party PE after staging. Signing occurs after final binary mutation; the verified hashes become candidate identity.
- Publish cryptographic hashes and verifiable build provenance. When GitHub Actions produces the release, publish and independently verify an artifact attestation; an attestation proves provenance, not software safety.
- Runtime networking is deny-by-default for `v0.1.0`. Any required endpoint, payload, authentication, retention, retry, offline behavior, and privacy notice must be explicitly approved and tested.
- Publish `SECURITY.md` or an equivalent private reporting route, response owner, supported-version window, and advisory/patch procedure before release.
- Retain release artifacts, symbols, provenance, SBOM, notices, validation summaries, and signing records so a later vulnerability or crash can be reproduced without altering the public release.

## Release Map Set

The shipped map set is a curated product surface, not every catalog entry. Each member MUST be runnable offline from package-relative files and have a verified redistribution/license record. Large third-party workloads may remain reproducible downloads and acceptance inputs without being redistributed in the archive.

The minimum release set covers these roles:

| Role | Current candidate | What it must expose |
| --- | --- | --- |
| Empty/control | `Empty` | Startup, clear/output encoding, camera/input, shutdown, and no-content behavior. |
| Compact metallic-roughness PBR | `DamagedHelmet` | Base color, normal, metallic, roughness, occlusion, image-based lighting, and framing. |
| Skinning/animation | `CesiumMan` | Skeleton/skin data, animation, transforms, temporal stability, and authored materials. |
| Alpha/two-sided/transmission | `DiffuseTransmissionPlant` | Alpha handling, double-sided policy, normals, shadows, and layered foliage response. |
| Lighting/reference | `CornellBox` or `Sponza` | Direct/indirect light, shadowing, exposure, tone mapping, and reference comparison. |
| Representative scene | one legally cleared broader scene | Geometry, material variety, residency, traversal, load time, and stable 30 FPS delivery. |

These are candidates, not approved members. `REL-01` selects the exact set after provenance, embedded license, attribution, modification, and redistribution review. Bistro, San Miguel, Modern Sponza, LPS Head, and other downloaded packs MUST NOT be bundled merely because the catalog can acquire or run them. If rights are incomplete, publish an acquisition recipe and hash instead of the asset bytes.

## Per-Map Acceptance Criteria

Every `ReleaseMapSet` member completes the canonical `MAP-A` through `MAP-H` flow in [Bistro and San Miguel Acceptance Workloads](BistroAndSanMiguelWorkloads.md#per-level-checkpoint-stages), then meets the additional release criteria below from the staged candidate package.

### Content And End-To-End

- A clean cook produces every declared scene, mesh, material, texture, animation, and shader product with zero uncategorized errors.
- The staged archive launches offline as a standard user from a path containing spaces and from a non-repository directory.
- The requested map becomes active, reaches the authoritative settled state, renders the frozen camera/route, switches away and back where supported, and exits normally.
- Logs contain no missing asset, missing shader, failed pipeline, device removal, fatal diagnostic, or silent fallback. Every allowed warning is listed by stable signature and justification.
- Package files, runtime-generated cache, settings, captures, and logs land only in their documented locations.

### Artifact-Free Visual Result

An “artifact” is a visible or numerical defect not present in the accepted reference or intentionally documented presentation. Examples include missing or corrupt geometry, texture fallback, UV/tangent seams, inverted normals, NaN/Inf pixels, fireflies beyond the declared reference policy, shadow acne/light leaks, alpha sorting/cutout errors, skinning explosions, stale history, ghosting, disocclusion trails, flicker, banding, unintended clipping, double encoding, unstable exposure, or backend-specific divergence.

For every frozen view and included backend:

- zero unresolved severity `S0` or `S1` visual defects are permitted;
- geometry, transforms, normal/tangent orientation, UVs, material bindings, alpha/double-sided state, animation, lights, shadows, exposure, tone mapping, and output encoding have explicit `Pass`/`Fail` observations;
- base color, normals, metallic, roughness, ambient occlusion, emissive, alpha, and transmission fields are inventoried where applicable and compared with authored intent;
- albedo/base-color, world-normal, roughness, metallic, depth, motion, direct-light, indirect-light, and final-output debug views are captured where applicable;
- the lit result is compared with a declared high-sample renderer path or a named external/reference image using identical camera, exposure, output transform, and crop assumptions;
- each map freezes objective full-frame and region-of-interest image-error metrics and thresholds before candidate measurement; the record names the metric/tool version, reference/candidate inputs, masks, and excluded pixels;
- temporal inspection covers camera motion, animation, occlusion/disocclusion, scene switch, and at least 300 settled frames;
- all comparisons preserve original captures, thresholds, excluded pixels, and reviewer disposition. A pleasing screenshot is not proof.

Defect severity is release impact:

| Severity | Definition | Release treatment |
| --- | --- | --- |
| `S0` | Crash, hang, data loss, security issue, device removal without controlled failure, or package cannot start/load/exit. | Always blocks. |
| `S1` | Wrong image/content, common-path corruption, major PBR error, recurrent severe hitch, unsupported silent fallback, or broken primary workflow. | Always blocks. |
| `S2` | Bounded defect with a practical workaround that does not invalidate the advertised path. | Fix or explicitly waive with owner, evidence, user impact, and target release. |
| `S3` | Cosmetic or documentation issue that does not misrepresent behavior. | May ship when recorded. |

### Thirty-FPS Performance Floor

The first release floor is 30 FPS for every shipped example map on the named minimum machine. Because FPS alone hides stalls and CPU/GPU attribution, acceptance uses frame times:

- `ShippingGame`, 1920x1080, fixed native or explicitly named reconstruction resolution, VSync off, fixed quality preset, fixed camera/route, and validation disabled for the performance run;
- at least 300 post-settle warm-up frames followed by at least 300 valid measured frames, repeated three times per map/backend;
- presented/application frame-time p95 is at or below 33.33 ms; CPU and GPU p95 are recorded separately and neither may exceed 33.33 ms;
- p99, worst frame, one-percent-low FPS, load time, shader/pipeline warm-up, process memory, tracked GPU memory, local/non-local budget, and excluded samples are reported rather than hidden;
- no recurring hitch cluster, memory growth across repeat loops, thermal throttling, dynamic-quality change, or background compilation may make the pass misleading;
- D3D12 and Vulkan are measured separately. An API may be excluded at scope freeze, but an advertised API may not borrow the other API's result;
- a lower-spec system is reported as informational until it is added to the supported matrix.

Raster/hybrid and ray/path research claims retain the stricter mode-specific resolutions, timing distributions, and capture contract in the [canonical workload](BistroAndSanMiguelWorkloads.md#performance-contract). Reference rendering is evaluated for deterministic convergence and quality, not forced into the real-time floor unless it is selectable as an advertised real-time mode.

## Subsystem Feature-Closure Matrix

`REL-04` creates one row for every current user-visible capability and subsystem. A subsystem may be marked `Verified` only when every applicable item below links to evidence; “compiled” alone means `Integrated`.

| Owner/surface | Required closure evidence |
| --- | --- |
| Build and dependencies | Versioned Shipping configuration; compiler/SDK/dependency lock; clean configure/build; warnings policy; dependency licenses; logical package reproducibility; no undeclared runtime binary. |
| Core and platform | Supported OS/CPU detection; standard-user startup; Unicode and spaced paths; filesystem/config/log locations; clock/input/window behavior; clean exit; actionable unsupported-platform failure. |
| Tasks and lifecycle | Startup/shutdown ownership; cancellation and destruction with work in flight; 1/2/N worker behavior where applicable; bounded queues/memory; no thread, handle, or process leak over repeat runs. |
| Application and world | Default project/map; deterministic level activation; scene switch/reload; component/system lifetime; resize, minimize, restore, alt-tab, focus, and input behavior; repeat launch/load/exit. |
| Projects and content | Default project/map; catalog truth; packaged-versus-download-only content; redistribution record; startup-level selection; no release option that targets absent content. |
| Source import | Clean supported-format import; provenance and hashes; deterministic semantic inventory; transforms/materials/animation fidelity; malformed/missing input rejection. |
| Cooking | Clean scene/mesh/material/texture/asset cook; deterministic products or explained nondeterminism; bounded work/memory; stale and incompatible product rejection; no repository-relative dependency in the package. |
| Shader delivery | Clean shader cook for the release matrix; program/library/ABI identity; missing/incompatible artifact rejection; cold and warm pipeline behavior; no runtime dependency on source shader files unless declared. |
| D3D12 RHI | Debug layer and GPU-based validation on focused workloads; descriptor/resource/state/queue lifetime; timestamp/capture path; resize; capability rejection; DRED evidence for device-removal incidents. |
| Vulkan RHI | Core, synchronization, GPU-assisted, and best-practices validation where supported; queue/resource/swapchain lifetime; timestamp/capture path; resize; capability rejection; zero uncategorized validation findings. |
| Renderer | Each included view mode, lighting path, ray/raster route, upscaler, exposure/tone/output path, and debug view has a feature row, compatible-map result, backend disposition, quality evidence, and performance cost. |
| Editor | If distributed: clean-machine start; viewport/camera/settings/edit/capture workflows; save/generated-content ownership; user-visible support matches actual behavior. |
| Launcher | If distributed: source/toolchain discovery; configure/build/cook/run; asset-pack sync; progress/cancel/failure; selectable scope matches support; no repository-only path assumptions; child-process handoff and shutdown. |
| Runtime package | Dependency closure; package-relative discovery; offline start; clean-machine map loop; documented prerequisites; no authoring/training/source assets; integrity manifest; archive extraction and uninstall/delete behavior. |
| Documentation/support | Root quick start, system requirements, controls, map/mode matrix, third-party notices, release notes, known issues, troubleshooting, crash-report instructions, privacy statement where applicable, and exact support boundary. |

The feature-closure record MUST include owner, public surface, classification, supported configurations, positive case, negative/failure case, evidence link, open defects, last evidence revision, and approval. If a current feature cannot meet this bar, exclude it from the release surface before proceeding.

## Release Gates

Gates are strictly ordered. A later gate may be prepared, but it cannot be accepted while an earlier gate is red.

| Gate | Exit criteria | Required artifact |
| --- | --- | --- |
| `REL-00 Scope and freeze` | Product/platform/support matrix, feature classifications, non-goals, and approval are frozen. | `release-scope.md` and feature inventory. |
| `REL-01 Identity, rights, and map set` | Version/publisher/license are real; third-party notices and redistribution decisions are reviewed; exact `ReleaseMapSet` is frozen. | License/notices audit and map manifest. |
| `REL-02 Clean reproducible baseline` | Clean checkout configures and builds the named Shipping target; required static/architecture/style checks pass; validation policy and any explicit test authorization are recorded. | Environment manifest and raw build/check logs. |
| `REL-03 Package spine` | One command performs Build, Cook, Stage, and Package; the staged tree has a manifest, dependency closure, hashes, and no repository path dependency. | First non-candidate archive and package inventory. |
| `REL-04 Current feature closure` | Every current surface is `Included`, `Experimental`, `Excluded`, or `Removed`; every included/experimental row passes its subsystem criteria and failure behavior. | Approved feature-closure matrix. |
| `REL-05 Map correctness and PBR` | Every release map passes end-to-end, artifact, PBR, debug-view, reference, and temporal review on each included backend. | Per-map evidence packages and defect disposition. |
| `REL-06 Performance and stability` | Every release map meets the 30 FPS floor; load/soak/repeat/resize/switch/shutdown and memory gates pass on the supported matrix. | Raw timings, captures, memory records, and stability log. |
| `REL-07 Native backend diagnostics` | D3D12 and Vulkan validation passes are clean or every finding is resolved/classified; crash/device-loss diagnostics are actionable. | API validation logs, captures, driver matrix, and incident records. |
| `REL-08 Clean-machine candidate` | A frozen archive is installed/extracted, run offline, exercised, and removed on a clean minimum machine by a standard user without source/build tools. | Candidate hashes, clean-machine script/record, screenshots, and logs. |
| `REL-09 Independent release approval` | A non-author reproduces install, launch, map loop, backend switch, capture, and exit from published instructions; zero open `S0`/`S1`; waivers and known issues are approved. | Signed release checklist and reproduction report. |
| `REL-10 Publish and verify` | Tag and artifact bytes are immutable; uploaded hashes match; a fresh download passes the shortest smoke; release page, notes, notices, and support route work. | Published URL, tag, checksums, retrieval log, and post-release owner. |

## Candidate Stability Matrix

The candidate package, not a development-tree executable, is the subject of final acceptance.

- Cold launch, default-map settle, each release-map load, and clean exit: 10 consecutive loops per included backend.
- Map switch/reload loop covering every release map: 30 minutes without unbounded memory growth, stale content, crash, hang, or device removal.
- Fixed representative traversal: 30-minute soak per included backend while collecting frame time and memory high-water.
- Resize, minimize/restore, alt-tab, focus/input recovery, and display-mode changes supported by the product.
- Offline use, blocked network, standard-user account, non-system drive where available, Unicode/spaced extraction path, read-only installation directory, and missing/corrupt required-file failure.
- First run with empty caches and warm repeat run; pipeline/shader compilation behavior is visible and bounded.
- Crash or device-removal reproduction captures build ID, backend, adapter/driver, active map/mode, logs, and diagnostic breadcrumbs without collecting undeclared private data.

A failed loop invalidates the affected candidate. Fixes produce new candidate bytes and require rerunning every gate touched by the change.

## Release Evidence Package

Retain raw evidence under:

```text
artifacts/validation/releases/<version>/<candidate-id>/
    manifest.json
    release-scope.md
    feature-closure.csv
    environment/
    build-cook-package/
    maps/<level-id>/<backend>/
    performance/<level-id>/<backend>/
    native-validation/<backend>/
    stability/
    clean-machine/
    legal/
    known-issues.md
    approval.md
```

`manifest.json` binds the commit, dirty state, configuration, toolchain, dependency revisions, content/artifact hashes, feature/map matrix, hardware/driver matrix, command lines, start/end time, operator, and result. Large disposable captures remain outside version control; intentional summaries and small reviewer artifacts may be promoted to documentation. The release archive itself contains user-facing notices and instructions, not internal raw validation data.

## Approval Rules

- No release evidence is collected from a dirty tree unless the manifest names and hashes every deliberate patch; the final candidate MUST come from a clean tagged commit.
- Zero open `S0` and `S1` defects are permitted. An `S2` waiver names owner, user impact, workaround, evidence, and target release.
- “Works on my machine,” responsive process state, one screenshot, average FPS, a successful compile, or a historical run never closes a gate.
- A warning is not waived by repetition. It has a stable identity, owner, cause, impact, and disposition.
- Every pass names the exact platform, backend, build, content, map, camera/route, and candidate hash. Unsupported or unavailable configurations are reported, not simulated.
- Scope may shrink to reach a trustworthy release. Acceptance thresholds may not be weakened after observing a failure without recording a reviewed contract change and rationale.
- The release owner gives the final go/no-go only after an independent reviewer completes `REL-09`.

## Shipping Precedent And Local Adoption

This contract adopts established delivery structure without copying another engine's product scope:

- Unreal Engine documents packaging as Build, Cook, Stage, and Package, followed where needed by Deploy and Run. Sparkle adopts those named product boundaries and proves the resulting staged package rather than treating compilation as delivery. Source: [Packaging Your Project](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project).
- CMake defines installation rules as the source for an installed tree, while CPack generates packages from those rules. Sparkle therefore requires one owned staging/install contract before selecting an archive generator. Sources: [CMake `install()`](https://cmake.org/cmake/help/latest/command/install.html) and [CPack](https://cmake.org/cmake/help/latest/module/CPack.html).
- Microsoft recommends a D3D12 debug-layer-clean application and provides GPU-based validation for descriptor, resource-state, and related GPU-timeline errors; DRED provides breadcrumbs and page-fault data for device removal. Sparkle uses focused native-validation and incident gates, not validation-enabled performance numbers. Sources: [Direct3D 12 programming environment setup](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up), [GPU-based validation](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation), and [DRED](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_device_removed_extended_data).
- Khronos treats validation as a development requirement and notes that validation layers are not intended to ship with the application. Sparkle separates Vulkan validation runs from the final package and performance profile. Sources: [Vulkan validation overview](https://docs.vulkan.org/guide/latest/validation_overview.html) and [Vulkan development tools](https://docs.vulkan.org/guide/latest/development_tools.html).
- GitHub releases bind a tag, release notes, and binary assets. Sparkle additionally requires checksums, a manifest, immutable candidate identity, and a post-upload retrieval smoke. Source: [Managing releases in a repository](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository).

These sources are industry precedent. This document and the repository's executable configuration remain the local authority.
