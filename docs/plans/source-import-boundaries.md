# Source Import Boundaries

Source import should translate authored files into ordinary Sparkle scene data. Runtime, editor, renderer, and RHI code should not branch on whether a camera, light, mesh, or animation came from glTF, FBX, a level file, or an editor-created object.

## Boundary Shape

```text
SourceImportAdapters -> cook records -> SceneAssetManager -> GameFramework scene data -> Renderer snapshots
```

- `SourceImportAdapters` may use source-format names and metadata while extracting data.
- Cooking may preserve compact source diagnostics and cooked records needed for reproducible asset loading.
- `SceneAssetManager` is the translation boundary from cooked records into normal GameFramework descriptions.
- `GameScene` owns normal scene entries such as cameras, lights, meshes, and animations.
- Renderer/RHI consume scene snapshots and GPU-ready resources. They should not know which source format created an object.

## Scaling Rule

When adding imported lights, animations, skins, or future scene features, add or extend a normal runtime description first. The import pipeline should fill that description. Avoid adding parallel runtime concepts like imported camera, imported light, glTF animation, or FBX skin unless the type is private to import/cook code.

## File Placement

- Public source import data types live under `Tools/Import/SourceImportAdapters/Public/Types`.
- `SourceImportResult.h` should stay a small aggregate over `ImportedScene` and diagnostics.
- Source-format extraction details live under `Tools/Import/SourceImportAdapters/Private/<Format>`.
- Cook feature builders live under `Tools/Cooking/SceneCooker/Private/Features/<Feature>`.
- Cooked runtime records may get dedicated headers under `Engine/GameFramework/Public/Assets/Cooked` when they are feature-specific.
- Cooked-record to runtime-description translators live under `Engine/GameFramework/Private/Assets/Translators`.
- Orchestrators such as `SceneCooker`, `SceneAssetManager`, and `GameScene` should call feature helpers; they should not accumulate feature conversion details.

## Camera Example

glTF camera import writes cooked camera records. `SceneAssetManager` maps those records into `CameraDesc`. `GameScene` appends them to its regular scene camera list beside the default camera. The editor outliner and viewport select cameras by scene camera index, not by source format.
