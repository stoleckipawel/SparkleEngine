# FBX Importer Implementation Plan

## Goal

Add FBX scene import to Sparkle in a way that scales beyond a one-off Bistro bring-up.

The target is not "parse FBX directly into renderer data." The target is:

- format-specific import front-end
- engine-common scene import result
- existing `GameScene` integration path
- renderer continuing to consume scene snapshots instead of importer-owned data

This follows the same broad shape used in NVIDIA and AMD sample engines:

- NVIDIA-style pattern: importer builds engine scene data first, renderer consumes engine scene state later
- AMD-style pattern: format loader builds an API-agnostic scene representation first, GPU upload and pass-specific rendering happen after that

For Sparkle, the AMD `GLTFCommon -> GPU upload` split and the NVIDIA `scene/import -> runtime scene ownership -> renderer extraction` split are the right ideas to copy. The exact implementation should stay Sparkle-native.

## Recommendation

Do not add a raw FBX runtime path directly into `GameScene`.

Implement FBX in three layers:

1. `SceneImportResult` as a format-agnostic import result
2. `FbxImporter` as one front-end that fills that result
3. `GameScene` integration that consumes the shared result exactly once

Use Assimp first, not Autodesk FBX SDK.

Reasoning:

- lower integration friction in CMake and CI
- easier redistribution story for an open repo
- allows FBX now, and gives OBJ and other fallback formats later if needed
- keeps Sparkle from binding its asset pipeline to Autodesk-specific SDK behavior too early

If Bistro import becomes a product requirement and Assimp proves insufficient, revisit a specialized FBX path later. Do not start there.

## Importer Pattern Comparison

The table below compares the main importer patterns used in game engines and sample engines. This is the decision surface Sparkle should evaluate against, not just "FBX SDK vs Assimp."

| Pattern | Common engine examples | Core idea | Strengths | Weaknesses | Best fit | Worst fit | Sparkle assessment |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Direct runtime source import | Small tools, prototypes, editor-only test paths | Engine loads source files like FBX or glTF directly at runtime | Fast to start, minimal toolchain, easy iteration for early experiments | Slow loads, large runtime dependencies, weak reproducibility, importer bugs leak into runtime, hard to optimize or validate once | Prototype engine, debug-only import, very small scene scope | Production scene loading, large scenes, stable shipping pipeline | Not recommended as the long-term Sparkle architecture |
| Runtime importer with engine-common scene result | Many sample engines in simplified form, Sparkle current glTF path moving toward this | Format parser converts source data into engine-owned scene import data, then runtime scene consumes that | Cleaner than direct renderer import, format parsing separated from runtime scene logic, good stepping stone | Still keeps source-format cost in runtime, harder to cook/optimize, source parser remains a shipping dependency | Mid-stage engine, tools still immature, small-to-medium content scale | Mature shipping asset pipeline with many scenes and platforms | Good transitional step for Sparkle before offline cooking |
| Offline converter to engine-native cooked asset | MiniEngine pattern, many custom engines | Tool imports FBX or other formats offline and writes engine-native cooked content | Fast runtime load, stable reproducible data, importer isolated to tools, easier optimization and validation, best separation of concerns | More tooling work up front, requires asset build path, iteration loop needs tool support | Medium-to-large projects, Bistro-scale scenes, long-term maintainable engine pipeline | Very early prototype where tool investment is premature | Strongest long-term target for Sparkle |
| Dual-format pipeline with canonical intermediate scene result | Modern engines with importer front-ends and shared scene build steps | Multiple source importers normalize into one engine-common intermediate before cooking or runtime scene creation | Reuse across glTF/FBX/USD/etc., importer code stays format-local, strong validation boundary | Requires up-front schema design, more abstraction work before visible results | Engines expecting multiple content formats | Tiny engine that only ever loads one simple format | Best architectural target once Sparkle adds FBX |
| Third-party library in runtime, thin wrapper only | Simple Assimp integrations, game jam engines | Use Assimp or similar almost directly, little normalization | Fastest path to "it loads" across many formats | Data model mismatch, unstable semantics across exporters, hard to control quality, third-party types tend to leak everywhere | Throwaway utilities, early experiments | Serious engine architecture, predictable materials and transforms | Acceptable only for experimentation, not as a stable Sparkle boundary |
| Third-party library in offline tool, private wrapper | MiniEngine ModelConverter, many custom pipelines | Use Assimp privately in tools, translate into engine-owned structures and write cooked assets | Keeps external dependency out of runtime, easier upgrades, strong ownership boundary | Requires converter tool and asset build steps | Best balance for engines growing beyond prototype | Overkill for a tiny one-week prototype | Very attractive for Sparkle if Bistro import becomes real scope |
| Autodesk FBX SDK in offline conversion only | Proprietary studio pipelines, DCC-heavy content workflows | Use official FBX SDK in tools for maximum FBX fidelity, then convert to engine-native data | Best control over tricky FBX features, more faithful DCC transform/material interpretation | Licensing/distribution friction, more build complexity, still need translation into engine semantics | Studios with strong FBX dependency and controlled toolchain | Open-source-friendly runtime path, lightweight build setup | Possible fallback later, not a good first move |
| Autodesk FBX SDK directly in runtime | Rare outside internal tools or highly specialized pipelines | Runtime directly parses FBX via official SDK | Maximum raw FBX access in shipped runtime | Heavy dependency, hard deployment story, poor runtime architecture, weak separation of authored vs runtime data | Almost never the best default | Sparkle current stage and scope | Do not do this |
| JSON or text scene description plus imported asset references | Sample engines, editor-friendly pipelines, Sparkle current level desc file pattern | Level file stores scene-specific authored setup while mesh/material assets are imported separately | Human-readable, easy diff/merge, great for level-specific metadata, good with descs | Not sufficient alone for heavy mesh import, needs imported asset pipeline underneath | Level data, camera/light setup, asset references, debug workflows | Raw heavy mesh data or final cooked geometry storage | Good for level descs in Sparkle, not a substitute for FBX asset conversion |
| Binary cooked package with reflection/editor source retained separately | Unreal-like production pattern in spirit | Editable source assets stay in editor/tool world, runtime loads cooked binary packages | Best runtime performance, scalable, platform-specific cooking possible | Highest tooling investment, harder to inspect without tools | Production engine at scale | Very early engine prototype | Long-term production direction, too early for Sparkle to fully build now |

## Pattern Arguments

The broad options above reduce to a smaller set of arguments that matter most for Sparkle.

| Option | Arguments for | Arguments against | Sparkle fit now |
| --- | --- | --- | --- |
| Keep direct runtime import | Lowest initial implementation cost, useful for quick validation, easiest way to prove Bistro can be parsed at all | Locks source-format complexity into runtime, weak content validation boundary, makes renderer/runtime loading harder to stabilize | Acceptable only as a temporary proving step |
| Add generic runtime importer boundary first | Cleans up current glTF path, makes FBX addition technically sane, preserves current desc and scene ownership model | Still leaves heavy import cost in runtime, does not by itself solve shipping-scale asset loading | Strong near-term step |
| Build offline conversion pipeline | Mirrors MiniEngine's strongest pattern, isolates Assimp/FBX complexity, gives best long-term runtime behavior | Requires tool investment before visible user-facing wins | Best long-term answer |
| Use Assimp first | Fastest realistic multi-format path, lower friction than FBX SDK, good enough for static-scene first pass | Material and transform fidelity can vary, not ideal for advanced FBX edge cases | Best first implementation choice |
| Use Autodesk FBX SDK first | Better control over FBX-specific semantics, fewer Assimp interpretation surprises | Higher build, licensing, and maintenance cost, encourages over-committing to FBX too early | Not the best starting point |

## MiniEngine-Specific Takeaway

MiniEngine is a useful reference specifically because it does not make Assimp the runtime model loader.

The pattern to mirror from MiniEngine is:

1. import with Assimp in a separate tool boundary
2. normalize into engine-owned model data
3. optimize and convert there
4. load engine-native data at runtime

The parts not to mirror literally are:

- MiniEngine's exact `.mini` format
- MiniEngine's exact mesh and material structs
- MiniEngine's runtime renderer-specific model layout

For Sparkle, the MiniEngine-inspired path should be:

1. define `SceneImportResult`
2. normalize glTF and FBX into that result
3. add an offline converter when content scale justifies it
4. keep `GameScene` and renderer consuming Sparkle-owned data only

## Upstream Pattern To Follow

### AMD-style pattern

AMD Cauldron keeps file-format parsing in a common scene loader and keeps GPU upload and render-pass data construction in later layers.

The important behavior to copy is:

- common scene structures are API-agnostic
- loader owns JSON or asset parsing concerns
- renderer-side systems translate common scene data into GPU buffers, material tables, and pass data

For Sparkle, that means FBX parsing should not allocate renderer objects or build render-pass structs.

### NVIDIA-style pattern

NVIDIA sample engines generally keep importer output in engine-owned scene types, then let runtime scene and renderer systems consume that stable representation.

The important behavior to copy is:

- importer creates engine-owned scene content
- scene ownership stays in runtime scene systems
- renderer reads snapshots or extracted scene data, not importer internals

For Sparkle, that means the importer should not bypass `GameScene`, `SceneMaterials`, `SceneTextures`, or mesh snapshot capture.

## Current Sparkle Constraints

Current Sparkle import is a small glTF-specific path:

- `GltfLoader` parses one format directly
- `LoadResult` is glTF-shaped, not importer-agnostic
- `GameScene` converts that result directly into scene materials, scene textures, and mesh components

Current limitations that matter for FBX:

- no generic importer interface yet
- no importer-neutral scene result
- mesh/material binding is still transitional and not section-based
- material model is simple PBR-oriented `MaterialDesc`
- no dedicated offline asset conversion pipeline yet

Because of that, a direct FBX adapter inside the current `GltfLoader` style would be the wrong shape.

## Proposed Architecture

### New import abstraction

Add a new format-agnostic import result in GameFramework:

`SceneImportResult`

Suggested contents:

- imported mesh payloads
- imported mesh transforms
- imported material payloads
- imported material bindings per imported mesh or per future mesh section
- imported texture references
- optional warnings
- optional source metadata

This should replace the idea that `LoadResult` is glTF-specific engine API.

`GltfLoader` can then become one producer of `SceneImportResult`.

`FbxImporter` becomes another producer of `SceneImportResult`.

### New importer boundary

Add an importer front-end boundary:

- `SceneImporter`
- `SceneImporterRegistry` or simple extension dispatch helper

Responsibilities:

- choose importer by extension
- normalize source path and reporting
- return `SceneImportResult`

Non-responsibilities:

- no renderer objects
- no material cache manipulation
- no texture residency decisions
- no direct mutation of `GameScene`

### Keep runtime ownership where it already belongs

`GameScene` remains the owner of runtime scene content.

That means:

- `SceneMaterials` still owns runtime material set
- `SceneTextures` still owns texture references
- `SceneMeshes` still owns mesh components
- renderer still extracts snapshots from the scene

This avoids the common mistake of turning the importer into a second scene system.

## Dependency Choice

### Phase 1 dependency

Use Assimp for FBX import.

Scope of Assimp usage:

- mesh geometry
- node transforms
- material enumeration
- texture path extraction
- optional camera and light extraction if needed later

Do not expose Assimp types outside the importer implementation.

### Phase 2 fallback decision

If Bistro material fidelity or transform fidelity is not acceptable through Assimp, evaluate one of these:

1. offline FBX-to-Sparkle conversion tool using Assimp plus custom post-processing
2. Autodesk FBX SDK only for offline conversion, not runtime engine loading

This keeps the runtime engine on a single canonical import result shape.

## Implementation Stages

### Stage 0: Refactor current glTF path to the right abstraction

Before adding FBX, do this cleanup:

- introduce `SceneImportResult`
- make `GltfLoader` fill `SceneImportResult`
- move shared import helpers out of `GltfLoader` where reasonable
- add importer warnings collection to the result

Exit criteria:

- `GameScene` no longer depends on a glTF-specific result type
- the glTF path still works with no behavior regression

### Stage 1: Introduce importer dispatch

Add a small importer entry point:

- `SceneImporter::Load(path)`

Dispatch rules:

- `.gltf` and `.glb` route to glTF importer
- `.fbx` routes to FBX importer

Exit criteria:

- `GameScene` calls a generic importer entry point instead of calling `GltfLoader` directly

### Stage 2: Add Assimp-backed `FbxImporter`

Implement first-pass static import only.

Supported in MVP:

- static meshes
- node transforms
- material names
- diffuse/base color texture path extraction where available
- basic scalar material properties where mapping is reasonable
- warning collection for unsupported features

Not supported in MVP:

- skeletal animation
- morph targets
- full DCC material graph translation
- cameras and lights unless Bistro requires them immediately
- embedded media conversion beyond a minimal path

Exit criteria:

- simple FBX scenes load into `GameScene`
- imported meshes appear with correct transforms
- missing or unsupported material data degrades predictably

### Stage 3: Material translation hardening

FBX materials are not a clean PBR contract. Add a translation layer that maps source materials into Sparkle material policy.

Rules:

- prefer stable fallback mapping over trying to preserve every DCC feature
- map to `MaterialDesc` only where semantics are well understood
- report unsupported material features as warnings

Recommended MVP mapping:

- material name
- base color or diffuse color
- opacity if clearly provided
- normal texture if clearly provided
- emissive color and emissive texture if clearly provided

Deferred features:

- layered materials
- specular workflow nuance
- clearcoat, anisotropy, subsurface, transmission
- DCC shader graph semantics

Exit criteria:

- Bistro-class scenes do not crash on materials
- fallback materials look predictable even if not fully faithful

### Stage 4: Scene scale hardening for Bistro

This stage is about practical Bistro import, not just correctness.

Add:

- import timing and warning summary
- duplicate texture-path normalization
- material dedup review where safe
- large-scene memory checks
- explicit logging for unsupported nodes and material models

Exit criteria:

- Bistro import completes
- scene loads without importer crashes
- logs make unsupported content obvious

## Data Model Changes Needed In Sparkle

### Required now

- add `SceneImportResult`
- add importer warning collection type
- add generic scene importer dispatch

### Strongly recommended soon after

- move mesh/material binding toward mesh sections or imported primitives instead of a single per-mesh binding
- preserve imported node names for debugging and inspection
- separate importer-local material offsets from runtime material handles explicitly in the shared result

### Explicitly defer for MVP

- animation
- skinning
- blend shapes
- authored light and camera import
- full hierarchy editing support in editor

## File Layout Proposal

Suggested additions under GameFramework:

- `Public/Assets/SceneImportResult.h`
- `Public/Assets/SceneImporter.h`
- `Private/Assets/SceneImporter.cpp`
- `Public/Assets/FbxImporter.h`
- `Private/Assets/FbxImporter.cpp`
- `Private/Assets/ImportWarnings.h`

Suggested dependency additions:

- Assimp fetched in `Scripts/FetchDependencies.cmake`
- linked privately in `Engine/GameFramework/CMakeLists.txt`

## Risks

### Material fidelity risk

FBX material semantics vary by exporter and DCC.

Mitigation:

- keep MVP material mapping intentionally narrow
- warn aggressively for unsupported semantics
- use default material fallback predictably

### Transform correctness risk

FBX often includes pivots, axis conversions, geometric transforms, and exporter-specific transform quirks.

Mitigation:

- normalize transforms inside the importer only
- keep `Transform` as the engine-side output contract
- validate against known reference scenes early

### Dependency and build risk

Assimp is much heavier than cgltf.

Mitigation:

- keep it private to GameFramework
- wrap all third-party types in `.cpp` files only
- avoid exposing Assimp headers in public Sparkle headers

### Architecture drift risk

It is easy to let FBX import bypass the engine scene model because Bistro pressure will encourage shortcuts.

Mitigation:

- require all importers to return `SceneImportResult`
- keep `GameScene` as the only runtime scene owner
- keep renderer extraction unchanged

## Acceptance Criteria

### Architecture acceptance

- no renderer code depends on Assimp or FBX SDK
- no public engine API exposes third-party FBX types
- glTF and FBX both flow through one importer-neutral result

### MVP feature acceptance

- import static FBX meshes into `GameScene`
- preserve world transforms correctly enough for Bistro-style placement
- import a usable subset of materials and textures
- unsupported features produce warnings, not crashes

### Bistro acceptance

- Bistro scene can be ingested through the FBX path or an offline FBX conversion path
- importer completes without fatal parse failures on valid source data
- runtime scene loads and renders with predictable fallback behavior where fidelity is incomplete

## Recommended Execution Order

1. Introduce `SceneImportResult` and refactor current glTF import to use it
2. Introduce `SceneImporter` dispatch and keep glTF behavior stable
3. Integrate Assimp privately and add `FbxImporter` static-mesh MVP
4. Add importer warnings and reporting
5. Harden material mapping for Bistro
6. Revisit mesh-section material binding after MVP import succeeds

## Final Recommendation

Bring FBX in only if the real goal is broader DCC interoperability.

If the real goal is specifically Bistro, first check whether an offline conversion path to glTF or a Sparkle-native intermediate format is cheaper and more stable.

For Sparkle's current state, the correct NVIDIA/AMD-style move is:

- importer front-end stays format-specific
- imported scene result becomes engine-common
- runtime scene owns the loaded data
- renderer remains downstream of runtime scene extraction

That is the shape that keeps FBX from becoming a permanent architectural shortcut.