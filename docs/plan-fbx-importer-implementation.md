# FBX Importer Implementation Plan

## Goal

Reach the FBX pipeline end state defined in the architecture document, not merely "get FBX loading somehow."

That final state is:

- source-format import lives outside the runtime engine
- FBX and glTF both normalize into one engine-owned `SceneImportResult`
- a standalone converter writes cooked Sparkle-native assets
- runtime loads only cooked assets through `CookedAssetLoader`
- `GameScene` remains the runtime owner of meshes, materials, and textures
- renderer continues to consume scene snapshots and never depends on importer code

The implementation plan should still allow a practical transitional phase where FBX and glTF import run inside the engine first. That temporary runtime path is acceptable because it accelerates bring-up and debugging, but every stage must be structured so it can be extracted cleanly into the final offline pipeline.

## Planning Principle

Use a two-step architecture strategy:

1. first make importers work together through one runtime abstraction
2. then move that abstraction and both importers behind a dedicated offline converter boundary

This preserves momentum without locking Sparkle into a permanent raw-source runtime import path.

## Recommendation

Do not build a one-off FBX path that writes directly into `GameScene` or renderer-facing structures.

Instead, build FBX in layers:

1. `SceneImportResult` as the importer-neutral contract
2. `SceneImporter` dispatch as the shared entry point
3. `FbxImporter` and `GltfImporter` as sibling front-ends
4. `GameScene` integration that consumes the shared result
5. later extraction of the same importer stack into `SparkleAssetConverter`
6. final replacement of runtime source import with cooked asset loading

Use Assimp first, not Autodesk FBX SDK.

Reasoning:

- lower integration friction in CMake and CI
- easier redistribution story for an open repo
- enough capability to validate Sparkle's importer abstraction and Bistro-scale content
- keeps Sparkle from committing to Autodesk-specific behavior before the pipeline shape is proven

If Bistro fidelity later proves Assimp-insufficient, revisit a specialized offline FBX path. Do not change the engine-side architecture to compensate.

## Final Architecture Target

The end state must match [architecture-fbx-pipeline.md](c:/Users/stole/Documents/GitHub/SparkleEngine/docs/architecture-fbx-pipeline.md):

- `SparkleAssetConverter` owns raw-format import
- `FbxImporter` and `GltfImporter` both live behind tool-side dispatch
- `SceneImportResult` remains the shared normalized intermediate
- `CookedAssetWriter` emits `.sasset`, `.smesh`, `.smat`, and `.stex`
- runtime engine replaces raw import with `CookedAssetLoader`
- `LevelDesc` points at cooked assets, not `.fbx` or `.gltf`
- runtime no longer links Assimp or cgltf for scene import

Anything added during transitional runtime bring-up should be evaluated by one question:

"Does this make extraction into the converter easier, or does it deepen runtime-only coupling?"

If it deepens runtime-only coupling, it is the wrong move.

## Transitional Development Strategy

Starting by implementing `FbxImporter` and then introducing the abstraction is a valid execution order, as long as the concrete importer is immediately shaped around the final shared contract.

That means the temporary runtime phase may include:

- `FbxImporter` in engine code
- `GltfImporter` or adapted `GltfLoader` in engine code
- `SceneImporter::Load()` dispatch inside the engine
- `GameScene` consuming `SceneImportResult`

But it must not include:

- FBX-specific data structures leaking into public engine APIs
- direct importer mutation of renderer state
- Assimp types escaping `.cpp` implementation files
- runtime-only shortcuts that bypass `SceneImportResult`
- content formats or material mappings that cannot be serialized into the final cooked pipeline

This is the key distinction:

- temporary runtime location is acceptable
- temporary runtime architecture is not

## MiniEngine-Specific Takeaway

MiniEngine is a useful reference because it treats format parsing as a conversion problem, not a runtime rendering-system responsibility.

The pattern to mirror is:

1. import in a separate boundary
2. normalize into engine-owned model data
3. validate and optimize there
4. load engine-native data at runtime

The parts not to mirror literally are:

- MiniEngine's exact `.mini` format
- MiniEngine's exact mesh and material structs
- MiniEngine's renderer-specific model layout

For Sparkle, the correct interpretation is:

1. define `SceneImportResult`
2. normalize glTF and FBX into that result
3. use a temporary runtime importer phase only to prove correctness
4. then extract that importer stack into the offline converter
5. finish by removing runtime raw-format loading

## Current Sparkle Constraints

Current Sparkle import is still a small glTF-specific path:

- `GltfLoader` parses one format directly
- `LoadResult` is glTF-shaped, not importer-agnostic
- `GameScene` converts that result directly into scene materials, scene textures, and mesh components

Current limitations that matter for FBX:

- no generic importer interface yet
- no importer-neutral scene result
- mesh/material binding is still transitional and not section-based
- material model is simple PBR-oriented `MaterialDesc`
- no dedicated offline asset conversion pipeline yet

These constraints justify a staged migration. They do not justify stopping at runtime source import.

## Proposed Architecture During Migration

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

This replaces the idea that `LoadResult` is a glTF-specific engine contract.

### New importer boundary

Add a shared importer front-end boundary:

- `SceneImporter`
- simple extension dispatch first, registry only if later justified

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

`GameScene` remains the owner of runtime scene content throughout every stage.

That means:

- `SceneMaterials` still owns runtime material set
- `SceneTextures` still owns texture references
- `SceneMeshes` still owns mesh components
- renderer still extracts snapshots from the scene

This avoids turning the importer into a second scene system during bring-up.

## Dependency Choice

### Transitional dependency

Use Assimp for FBX import during the runtime bring-up phase and keep it private to importer implementation files.

Scope of Assimp usage:

- mesh geometry
- node transforms
- material enumeration
- texture path extraction
- optional camera and light extraction if needed later

Do not expose Assimp types outside importer implementation.

### Final dependency placement

Once `SparkleAssetConverter` exists, Assimp should move out of runtime engine linkage and live only in the tool.

cgltf should follow the same pattern when the glTF importer is extracted.

## Implementation Stages

### Stage 0: Lock the target and transitional rules

Before code movement, align the plan around explicit target invariants:

- `SceneImportResult` is the only importer output contract
- `GameScene` stays the sole runtime scene owner
- renderer remains unchanged
- Assimp and cgltf stay private behind importer implementations
- the runtime importer phase is temporary and will be deleted

Exit criteria:

- the team agrees the architecture destination is the offline converter document
- temporary runtime stages are documented as migration steps, not the end state

### Stage 1: Refactor current glTF path to the shared result

Before adding FBX, do this cleanup:

- introduce `SceneImportResult`
- make `GltfLoader` or a renamed `GltfImporter` fill `SceneImportResult`
- move shared import helpers out of glTF-specific code where reasonable
- add importer warnings collection to the result

Exit criteria:

- `GameScene` no longer depends on a glTF-specific result type
- the glTF path still works with no behavior regression
- importer output is already shaped for later serialization

### Stage 2: Add runtime importer dispatch

Add a small importer entry point inside the engine:

- `SceneImporter::Load(path)`

Dispatch rules:

- `.gltf` and `.glb` route to glTF importer
- `.fbx` routes to FBX importer

This is the stage where both formats can be imported through one runtime-only abstraction, which is the workflow you prefer for early productivity.

Exit criteria:

- `GameScene` calls a generic importer entry point instead of `GltfLoader` directly
- both glTF and FBX now share the same runtime import contract

### Stage 3: Add Assimp-backed `FbxImporter`

Implement first-pass static import only.

Supported in MVP:

- static meshes
- node transforms
- material names
- diffuse or base-color texture path extraction where available
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
- no FBX-specific types leak past the importer boundary

### Stage 4: Runtime hardening on the shared abstraction

This stage is deliberately still runtime-based so correctness problems are easier to debug before extraction.

Add:

- material translation hardening into `MaterialDesc`
- import timing and warning summary
- duplicate texture-path normalization
- material dedup review where safe
- large-scene memory checks
- explicit logging for unsupported nodes and material models

This is also the point to validate Bistro-scale content using the same shared import result that will later be serialized.

Exit criteria:

- Bistro-scale scenes complete import through `SceneImporter`
- scene loads without importer crashes
- logs make unsupported content obvious
- the shared result contains enough stable data to define cooked formats confidently

### Stage 5: Define cooked asset formats from the proven shared result

After the runtime abstraction is stable, define the engine-native cooked boundary:

- `CookedAssetFormat` headers and versioning
- `.sasset` for scene graph and references
- `.smesh` for mesh data
- `.smat` for material payloads
- `.stex` for texture manifest and cooked texture references

Do not invent cooked formats before the shared runtime importer has proven which data is actually required.

Exit criteria:

- cooked formats map directly from `SceneImportResult` plus validated derived data
- format definitions support both glTF and FBX without format-specific exceptions

### Stage 6: Extract the importer stack into `SparkleAssetConverter`

Move the now-proven shared importer pipeline into a standalone tool:

- move `FbxImporter` out of runtime engine code
- move `GltfImporter` out of runtime engine code
- keep `SceneImportResult` as the normalized intermediate, shared where appropriate
- add `CookedAssetWriter`
- add converter CLI entry point and output directory handling

At this stage the tool may coexist briefly with the old runtime source-import path during migration, but the runtime path should already be treated as deprecated.

Exit criteria:

- the converter can import FBX and glTF and emit Sparkle cooked assets
- Assimp no longer needs to be part of runtime scene loading
- importer code has one home: the converter pipeline

### Stage 7: Switch runtime to cooked asset loading

Replace source-format runtime import with engine-native cooked loading:

- add `CookedAssetLoader`
- update `LevelDesc` and imported asset references to point at cooked assets
- route scene creation through cooked asset loading instead of source parsing
- keep `GameScene` ownership and renderer extraction unchanged

Exit criteria:

- runtime scene load no longer parses `.fbx`, `.gltf`, or `.glb`
- `GameScene` populates from cooked assets only
- renderer behavior remains unchanged

### Stage 8: Remove transitional runtime source import

Finish the migration completely:

- remove or retire runtime-only source import entry points
- remove raw-format loader calls from level loading
- remove Assimp and cgltf from the runtime asset import path
- keep only cooked-data loading in the shipped engine path

Exit criteria:

- the codebase matches the offline converter architecture precisely
- raw import remains only in tooling
- model baking and cooked-content generation are the canonical path forward

## Data Model Changes Needed In Sparkle

### Required early

- add `SceneImportResult`
- add importer warning collection type
- add generic scene importer dispatch

### Strongly recommended during runtime hardening

- move mesh/material binding toward mesh sections or imported primitives instead of a single per-mesh binding
- preserve imported node names for debugging and inspection
- separate importer-local material offsets from runtime material handles explicitly in the shared result

### Required for final pipeline completion

- define cooked asset format structures and versioning
- add `CookedAssetWriter`
- add `CookedAssetLoader`
- move level references from source assets to cooked assets

### Explicitly defer for MVP

- animation
- skinning
- blend shapes
- authored light and camera import
- full hierarchy editing support in editor

## File Layout Proposal By Phase

### Transitional runtime phase

Suggested additions under GameFramework:

- `Public/Assets/SceneImportResult.h`
- `Public/Assets/SceneImporter.h`
- `Private/Assets/SceneImporter.cpp`
- `Public/Assets/FbxImporter.h` only if a public declaration is unavoidable, otherwise keep private
- `Private/Assets/FbxImporter.cpp`
- `Private/Assets/ImportWarnings.h`

Suggested dependency additions:

- Assimp fetched in `Scripts/FetchDependencies.cmake`
- linked privately during the temporary runtime phase only

### Final offline pipeline phase

Target layout should converge to the architecture document:

- `Tools/AssetConverter/` owns `SceneImporter`, `FbxImporter`, `GltfImporter`, `ImportValidator`, `TextureCooker`, and `CookedAssetWriter`
- runtime owns `CookedAssetLoader` and cooked format definitions only
- raw-format loaders leave the runtime engine

## Risks

### Material fidelity risk

FBX material semantics vary by exporter and DCC.

Mitigation:

- keep MVP material mapping intentionally narrow
- warn aggressively for unsupported semantics
- use default material fallback predictably
- avoid encoding source-format quirks into runtime-facing material APIs

### Transform correctness risk

FBX often includes pivots, axis conversions, geometric transforms, and exporter-specific transform quirks.

Mitigation:

- normalize transforms inside the importer only
- keep `Transform` as the engine-side output contract
- validate against known reference scenes early
- ensure the normalized transform representation is the same one serialized into cooked assets later

### Dependency and build risk

Assimp is much heavier than cgltf.

Mitigation:

- keep it private to importer implementation files
- keep all third-party types in `.cpp` files only
- remove it from runtime linkage once converter extraction is done

### Architecture drift risk

The highest-risk failure mode is shipping the transitional runtime importer shape as if it were the finished pipeline.

Mitigation:

- require all importers to return `SceneImportResult`
- require all importer improvements to remain serializable into cooked formats
- schedule converter extraction and cooked loading as mandatory stages, not optional cleanup
- declare runtime raw import deprecated as soon as the converter path exists

## Acceptance Criteria

### Transitional architecture acceptance

- no renderer code depends on Assimp or FBX SDK
- no public engine API exposes third-party FBX types
- glTF and FBX both flow through one importer-neutral runtime result
- runtime bring-up remains structurally extractable into a converter

### MVP feature acceptance

- import static FBX meshes into `GameScene`
- preserve world transforms correctly enough for Bistro-style placement
- import a usable subset of materials and textures
- unsupported features produce warnings, not crashes

### Final pipeline acceptance

- `SparkleAssetConverter` owns raw FBX and glTF import
- runtime scene loading uses `CookedAssetLoader` only
- `LevelDesc` points to cooked content
- Assimp and cgltf are no longer required for shipped runtime scene import
- the codebase structure matches the offline converter architecture document

## Recommended Execution Order

1. Lock the final target and transitional invariants
2. Introduce `SceneImportResult` and refactor glTF import to use it
3. Introduce runtime `SceneImporter` dispatch and stabilize both formats there
4. Integrate Assimp privately and add `FbxImporter` static-mesh MVP
5. Harden materials, transforms, warnings, and Bistro-scale import on the shared runtime abstraction
6. Define cooked asset formats from the proven shared result
7. Extract importers into `SparkleAssetConverter` and add `CookedAssetWriter`
8. Add `CookedAssetLoader` and move runtime loading to cooked assets
9. Remove transitional runtime raw import paths

## Final Recommendation

Your preferred path is sound:

- start with FBX import working inside the engine
- quickly introduce the shared importer abstraction so glTF and FBX import the same way
- use that runtime path to stabilize transforms, materials, warnings, and Bistro-scale behavior
- then move the importer stack into the dedicated offline converter
- finish by removing runtime raw import entirely and relying on cooked assets plus model baking

That path keeps implementation practical early while still finishing at the precise FBX pipeline architecture you want, rather than stopping at a halfway design.

For Sparkle's current state, the correct move is the MiniEngine pattern:

- offline converter tool imports source formats using Assimp
- converter writes engine-native cooked assets
- runtime engine loads cooked data only
- renderer remains downstream of runtime scene extraction

That is the shape that keeps FBX from becoming a permanent architectural shortcut.