# FBX Importer Implementation Plan

## Goal

Add FBX scene import to Sparkle in a way that scales beyond a one-off Bistro bring-up.

The target is not "parse FBX directly into renderer data." The target is:

- format-specific import front-end
- engine-common scene import result
- existing `GameScene` integration path
- renderer continuing to consume scene snapshots instead of importer-owned data

This follows the MiniEngine offline converter pattern: a tool imports source formats offline and writes engine-native cooked content. The runtime engine only loads cooked data.

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

## Chosen Pattern: Offline Converter to Engine-Native Cooked Asset

This is the MiniEngine pattern: a standalone tool imports FBX or other source formats offline using Assimp, normalizes into engine-owned structures, and writes cooked engine-native assets. The runtime engine only loads cooked data.

Strengths:

- fast runtime load, no source-format parsing at startup
- stable reproducible data, importer isolated to tools
- easier optimization and validation
- best separation of concerns
- keeps external dependency (Assimp) out of runtime
- easier upgrades, strong ownership boundary

Tradeoffs:

- more tooling work up front
- requires asset build path
- iteration loop needs tool support

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

For Sparkle's current state, the correct move is the MiniEngine pattern:

- offline converter tool imports source formats using Assimp
- converter writes engine-native cooked assets
- runtime engine loads cooked data only
- renderer remains downstream of runtime scene extraction

That is the shape that keeps FBX from becoming a permanent architectural shortcut.