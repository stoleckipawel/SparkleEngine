# FBX Pipeline Architecture — Offline Converter Pattern

## 1. High-Level Pipeline Overview

```
  OFFLINE (Tool — runs outside engine runtime)          RUNTIME (Engine — ships to user)
 ─────────────────────────────────────────────         ───────────────────────────────────

  ┌──────────────┐   ┌──────────────┐
  │  Bistro.fbx  │   │ Scene.gltf   │  ... any source format
  └──────┬───────┘   └──────┬───────┘
         │                  │
         ▼                  ▼
  ┌─────────────────────────────────────┐
  │         SparkleAssetConverter       │   NEW standalone tool exe
  │                                     │
  │  ┌─────────────┐  ┌──────────────┐  │
  │  │ FbxImporter  │  │ GltfImporter │  │   format-specific front-ends
  │  │  (Assimp)    │  │  (cgltf)     │  │   each produces SceneImportResult
  │  └──────┬───────┘  └──────┬───────┘  │
  │         │                 │          │
  │         ▼                 ▼          │
  │  ┌─────────────────────────────┐     │
  │  │     SceneImportResult       │     │   format-agnostic intermediate
  │  │  meshes, materials, xforms  │     │
  │  │  textures, warnings         │     │
  │  └─────────────┬───────────────┘     │
  │                │                     │
  │                ▼                     │
  │  ┌─────────────────────────────┐     │
  │  │      CookedAssetWriter      │     │   serializes to engine-native
  │  │  .smesh  .smat  .stex       │     │   binary files on disk
  │  └─────────────┬───────────────┘     │
  └────────────────│─────────────────────┘
                   │
                   ▼
          ┌────────────────────┐
          │  Cooked Asset Dir  │   engine-native binary files
          │                    │
          │  models/           │      bistro/
          │    bistro.smesh    │        ├─ bistro.smesh
          │    bistro.smat     │        ├─ bistro.smat
          │    bistro.stex     │        └─ textures/
          │    textures/       │             ├─ wall.ktx2
          │      wall.ktx2    │             └─ floor.ktx2
          └────────┬───────────┘
                   │
    ───────────────│──────────── runtime boundary ────────────────
                   │
                   ▼
          ┌────────────────────┐
          │  CookedAssetLoader │   NEW — reads engine-native files
          └────────┬───────────┘
                   │
                   ▼
          ┌────────────────────┐
          │     GameScene       │   EXISTING — unchanged ownership
          │  ┌──────────────┐  │
          │  │SceneMaterials│  │
          │  │SceneTextures │  │
          │  │SceneMeshes   │  │
          │  └──────────────┘  │
          └────────┬───────────┘
                   │ CaptureSnapshot()
                   ▼
          ┌────────────────────┐
          │     Renderer       │   EXISTING — unchanged extraction
          │  RenderSceneData   │
          │  GPU upload        │
          └────────────────────┘
```

## 2. What Changes, What Stays

```
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │                          CHANGE SUMMARY                                    │
 ├─────────────┬────────┬──────────────────────────────────────────────────────┤
 │  Component  │ Action │ Details                                             │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  NEW TOOL   │  ADD   │ SparkleAssetConverter (standalone .exe)              │
 │             │        │   - FbxImporter  (Assimp, lives only in tool)       │
 │             │        │   - GltfImporter (cgltf,  lives only in tool)       │
 │             │        │   - SceneImportResult (shared intermediate)         │
 │             │        │   - CookedAssetWriter (serialization)               │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  NEW IN     │  ADD   │ CookedAssetLoader                                   │
 │  ENGINE     │        │   - Reads .smesh/.smat/.stex at runtime             │
 │             │        │   - Feeds data into GameScene subsystems            │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  LevelDesc  │ CHANGE │ ImportedMeshRequest.assetPath points to cooked      │
 │             │        │ asset dir instead of raw .gltf/.fbx source files    │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  GltfLoader │ REMOVE │ Replaced by GltfImporter inside converter tool      │
 │  (runtime)  │        │ Runtime no longer parses raw glTF at startup        │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  GameScene  │  KEEP  │ Still owns SceneMaterials/Textures/Meshes           │
 │             │        │ LoadLevel changes to call CookedAssetLoader         │
 │             │        │ instead of GltfLoader                               │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  Renderer   │  KEEP  │ Completely unchanged                                │
 │             │        │ Still reads RenderSceneSnapshot from GameScene      │
 │             │        │ Still builds RenderSceneData + GPU upload           │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  Snapshots  │  KEEP  │ MeshSnapshot, MaterialSnapshot, TextureSnapshot     │
 │             │        │ CameraSnapshot, LightingSnapshot — all unchanged    │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  cgltf dep  │ MOVE   │ Moves from Engine runtime to converter tool only    │
 │             │        │                                                     │
 ├─────────────┼────────┼──────────────────────────────────────────────────────┤
 │             │        │                                                     │
 │  Assimp dep │  ADD   │ Linked only in converter tool, never in runtime     │
 │             │        │                                                     │
 └─────────────┴────────┴──────────────────────────────────────────────────────┘
```

## 3. Converter Tool Internals

```
  SparkleAssetConverter.exe  <input_path>  <output_dir>
       │
       ▼
  ┌──────────────────────────────────────────────────────────────────┐
  │                    CONVERTER PIPELINE                            │
  │                                                                  │
  │  1. DISPATCH                                                     │
  │  ┌────────────────────────────────────────────────────────┐      │
  │  │  SceneImporter::Load(path)                             │      │
  │  │                                                        │      │
  │  │  extension check:                                      │      │
  │  │    .fbx  ──► FbxImporter::Import()   (uses Assimp)     │      │
  │  │    .gltf ──► GltfImporter::Import()  (uses cgltf)      │      │
  │  │    .glb  ──► GltfImporter::Import()  (uses cgltf)      │      │
  │  │    .obj  ──► ObjImporter::Import()   (uses Assimp)     │      │
  │  │                    ▲ future                             │      │
  │  └──────────┬─────────────────────────────────────────────┘      │
  │             │                                                    │
  │             ▼                                                    │
  │  2. FORMAT-AGNOSTIC INTERMEDIATE                                 │
  │  ┌────────────────────────────────────────────────────────┐      │
  │  │  SceneImportResult                                     │      │
  │  │  ├─ meshPayloads[]          VertexData + indices       │      │
  │  │  │    ├─ vertices[]         pos, uv, normal, tangent   │      │
  │  │  │    ├─ indices[]          uint32                     │      │
  │  │  │    └─ name               debug identifier           │      │
  │  │  ├─ transforms[]            per-mesh world xforms      │      │
  │  │  ├─ materialDescs[]         MaterialDesc structs        │      │
  │  │  ├─ materialBindings[]      mesh → material index      │      │
  │  │  ├─ textureRefs[]           resolved texture paths     │      │
  │  │  ├─ warnings[]              unsupported feature notes   │      │
  │  │  └─ sourceMetadata          optional origin info        │      │
  │  └──────────┬─────────────────────────────────────────────┘      │
  │             │                                                    │
  │             ▼                                                    │
  │  3. VALIDATION + OPTIMIZATION                                    │
  │  ┌────────────────────────────────────────────────────────┐      │
  │  │  ImportValidator                                       │      │
  │  │  ├─ dedup duplicate materials                          │      │
  │  │  ├─ normalize texture paths                            │      │
  │  │  ├─ warn on degenerate triangles                       │      │
  │  │  ├─ validate transform correctness                     │      │
  │  │  └─ collect import timing + stats                      │      │
  │  └──────────┬─────────────────────────────────────────────┘      │
  │             │                                                    │
  │             ▼                                                    │
  │  4. TEXTURE COOKING  (optional but recommended)                  │
  │  ┌────────────────────────────────────────────────────────┐      │
  │  │  TextureCooker                                         │      │
  │  │  ├─ load source images   (stb_image)                   │      │
  │  │  ├─ generate mipmaps     (stb_image_resize2)           │      │
  │  │  ├─ block compress       (Compressonator CMP_Core)     │      │
  │  │  │    albedo     ──► BC7                               │      │
  │  │  │    normal     ──► BC5                               │      │
  │  │  │    metalRough ──► BC5                               │      │
  │  │  │    emissive   ──► BC7                               │      │
  │  │  └─ write .ktx2          (KTX-Software)                │      │
  │  └──────────┬─────────────────────────────────────────────┘      │
  │             │                                                    │
  │             ▼                                                    │
  │  5. SERIALIZATION                                                │
  │  ┌────────────────────────────────────────────────────────┐      │
  │  │  CookedAssetWriter                                     │      │
  │  │  ├─ write .smesh  (vertex + index buffers, binary)     │      │
  │  │  ├─ write .smat   (MaterialDesc array, binary)         │      │
  │  │  ├─ write .stex   (texture manifest + cooked refs)     │      │
  │  │  └─ write .sasset (scene graph: xforms, bindings)      │      │
  │  └────────────────────────────────────────────────────────┘      │
  │                                                                  │
  └──────────────────────────────────────────────────────────────────┘
```

## 4. Runtime Loading Path (Engine Side)

```
  ┌──────────────────────────────────────────────────────────────────┐
  │                   RUNTIME LOAD FLOW                              │
  │                                                                  │
  │  LevelDesc                                                       │
  │  ┌──────────────────────────────────────────────────────┐        │
  │  │  name: "Bistro"                                      │        │
  │  │  cameraDesc: { ... }                                 │        │
  │  │  lightingDesc: { ... }                               │        │
  │  │  importedMeshRequests:                               │        │
  │  │    - assetPath: "cooked/bistro/bistro.sasset"  ◄──── CHANGED  │
  │  │      (was: "content/bistro/bistro.fbx")              │        │
  │  └───────────┬──────────────────────────────────────────┘        │
  │              │                                                   │
  │              ▼                                                   │
  │  LevelManager::LoadLevelFromUnloadedState()                      │
  │              │                                                   │
  │              ▼                                                   │
  │  GameScene::LoadLevel(desc)                                      │
  │              │                                                   │
  │    ┌─────────▼──────────┐                                        │
  │    │ CookedAssetLoader  │  NEW                                   │
  │    │                    │                                         │
  │    │  Read .sasset ─────┤──► scene graph, xforms, bindings       │
  │    │  Read .smesh  ─────┤──► vertex + index data                 │
  │    │  Read .smat   ─────┤──► MaterialDesc array                  │
  │    │  Read .stex   ─────┤──► texture manifest (paths to .ktx2)   │
  │    └─────────┬──────────┘                                        │
  │              │  returns populated data                           │
  │              ▼                                                   │
  │    ┌─────────────────────────────────┐                           │
  │    │  GameScene  (UNCHANGED OWNER)   │                           │
  │    │                                 │                           │
  │    │  SceneMaterials                 │                           │
  │    │    .AppendMaterials(descs)      │  same API as today        │
  │    │                                 │                           │
  │    │  SceneTextures                  │                           │
  │    │    .AppendTexturePaths(paths)   │  paths now point to .ktx2 │
  │    │                                 │                           │
  │    │  SceneMeshes                    │                           │
  │    │    .AppendMeshComponents(meshs) │  mesh data pre-validated  │
  │    │                                 │                           │
  │    └─────────┬───────────────────────┘                           │
  │              │                                                   │
  │              │ CaptureSnapshot()                                 │
  │              ▼                                                   │
  │    ┌─────────────────────────────────┐                           │
  │    │  RenderSceneSnapshot            │  UNCHANGED                │
  │    │    camera, lighting,            │                           │
  │    │    textures, meshes, materials  │                           │
  │    └─────────┬───────────────────────┘                           │
  │              │                                                   │
  │              ▼                                                   │
  │    ┌──────────────────────────────────────┐                      │
  │    │  SceneRenderStateCoordinator         │  UNCHANGED           │
  │    │    ├─ MaterialCacheManager           │                      │
  │    │    │    build descriptor tables      │                      │
  │    │    ├─ TextureManager                 │                      │
  │    │    │    load .ktx2 ──► D3D12 SRVs    │                      │
  │    │    └─ GPUMeshCache                   │                      │
  │    │         upload ──► D3D12 VB/IB       │                      │
  │    └─────────┬────────────────────────────┘                      │
  │              │                                                   │
  │              ▼                                                   │
  │    ┌──────────────────────────────────────┐                      │
  │    │  RenderSceneData                     │  UNCHANGED           │
  │    │    meshDraws[], materials[], lights  │                      │
  │    └──────────────────────────────────────┘                      │
  │                                                                  │
  └──────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow: Before vs After

### TODAY (raw glTF at runtime)
```
  .gltf file ──► GltfLoader (cgltf, runtime) ──► LoadResult ──► GameScene ──► Renderer
                 ^^^^^^^^^^^^^^^^^^^^^^^^^
                 parsing + validation at
                 every engine startup
```

### AFTER (cooked assets)
```
  .fbx/.gltf ──► SparkleAssetConverter (offline) ──► .smesh/.smat/.stex/.sasset
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                  runs once, outside engine                    │
                                                               ▼
                                              CookedAssetLoader (runtime) ──► GameScene ──► Renderer
                                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                              fast binary read, no parsing
```

## 6. File Layout — What Gets Added

```
  SparkleEngine/
  │
  ├─ Engine/
  │   ├─ GameFramework/
  │   │   ├─ Public/
  │   │   │   ├─ Assets/
  │   │   │   │   ├─ MaterialDesc.h              KEEP  (shared by tool + runtime)
  │   │   │   │   ├─ GltfLoader.h                REMOVE (replaced by tool-side GltfImporter)
  │   │   │   │   ├─ CookedAssetLoader.h         ADD    runtime loader for .smesh/.smat etc.
  │   │   │   │   └─ CookedAssetFormat.h         ADD    header/magic/version structs
  │   │   │   └─ Scene/
  │   │   │       └─ ...                         KEEP   all scene ownership unchanged
  │   │   └─ Private/
  │   │       └─ Assets/
  │   │           ├─ GltfLoader.cpp              REMOVE
  │   │           ├─ CookedAssetLoader.cpp       ADD
  │   │           └─ CookedAssetFormat.cpp       ADD
  │   │
  │   └─ Renderer/                               KEEP   completely unchanged
  │
  ├─ Tools/                                      ADD    new top-level directory
  │   └─ AssetConverter/
  │       ├─ CMakeLists.txt                      ADD    standalone exe build
  │       ├─ Main.cpp                            ADD    CLI entry point
  │       ├─ SceneImportResult.h                 ADD    format-agnostic intermediate
  │       ├─ SceneImporter.h                     ADD    dispatch interface
  │       ├─ SceneImporter.cpp                   ADD
  │       ├─ FbxImporter.h                       ADD    Assimp-backed FBX front-end
  │       ├─ FbxImporter.cpp                     ADD
  │       ├─ GltfImporter.h                      ADD    cgltf-backed glTF front-end
  │       ├─ GltfImporter.cpp                    ADD    (logic migrated from GltfLoader)
  │       ├─ ImportValidator.h                   ADD    validation + optimization
  │       ├─ ImportValidator.cpp                 ADD
  │       ├─ TextureCooker.h                     ADD    compress + mipmap + write ktx2
  │       ├─ TextureCooker.cpp                   ADD
  │       ├─ CookedAssetWriter.h                 ADD    serialize to engine formats
  │       └─ CookedAssetWriter.cpp               ADD
  │
  ├─ Scripts/
  │   ├─ FetchDependencies.cmake                 CHANGE  add Assimp fetch
  │   └─ CookAssets.bat                          ADD     batch script to run converter
  │
  └─ CmakeLists.txt                              CHANGE  add Tools/ subdirectory
```

## 7. Cooked Asset Formats (Binary)

```
  ┌─────────────────────────────────────────────────────────────┐
  │  .sasset  (Scene Asset — top-level manifest)                │
  │                                                             │
  │  ┌──────────────┐                                           │
  │  │ Header       │  magic "SAST", version, mesh count,       │
  │  │              │  material count, texture count             │
  │  ├──────────────┤                                           │
  │  │ MeshEntries  │  per mesh: name hash, transform,          │
  │  │              │  materialIndex, vertexOffset, indexOffset  │
  │  ├──────────────┤                                           │
  │  │ References   │  relative paths to .smesh, .smat, .stex   │
  │  └──────────────┘                                           │
  └─────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────┐
  │  .smesh  (Mesh Data — geometry blob)                        │
  │                                                             │
  │  ┌──────────────┐                                           │
  │  │ Header       │  magic "SMSH", version, mesh count        │
  │  ├──────────────┤                                           │
  │  │ MeshTable    │  per mesh: vertex count, index count,     │
  │  │              │  vertex offset, index offset               │
  │  ├──────────────┤                                           │
  │  │ Vertex Blob  │  tightly packed VertexData[]               │
  │  │              │  (pos, uv, color, normal, tangent)         │
  │  ├──────────────┤                                           │
  │  │ Index Blob   │  tightly packed uint32[]                   │
  │  └──────────────┘                                           │
  └─────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────┐
  │  .smat  (Material Array)                                    │
  │                                                             │
  │  ┌──────────────┐                                           │
  │  │ Header       │  magic "SMAT", version, count             │
  │  ├──────────────┤                                           │
  │  │ Materials[]  │  per material: name, baseColor,           │
  │  │              │  metallic, roughness, f0, emissiveColor,  │
  │  │              │  alphaMode, alphaCutoff,                   │
  │  │              │  texture slot indices (into .stex)         │
  │  └──────────────┘                                           │
  └─────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────┐
  │  .stex  (Texture Manifest)                                  │
  │                                                             │
  │  ┌──────────────┐                                           │
  │  │ Header       │  magic "STEX", version, count             │
  │  ├──────────────┤                                           │
  │  │ TextureRefs[]│  per texture: relative path to .ktx2,     │
  │  │              │  original source name, format hint         │
  │  └──────────────┘                                           │
  └─────────────────────────────────────────────────────────────┘
```

## 8. Dependency Graph

```
  ┌──────────────────────────────────────────────────────────────────────┐
  │                     BUILD TARGETS                                    │
  │                                                                      │
  │                                                                      │
  │   SparkleAssetConverter (exe)              SparkleEngine (exe)        │
  │   ┌──────────────────────┐                 ┌─────────────────────┐   │
  │   │                      │                 │                     │   │
  │   │  Links:              │                 │  Links:             │   │
  │   │   ├─ Assimp     NEW  │                 │   ├─ GameFramework  │   │
  │   │   ├─ cgltf           │                 │   ├─ Renderer       │   │
  │   │   ├─ stb_image       │                 │   ├─ RHI            │   │
  │   │   ├─ stb_resize      │                 │   ├─ Application    │   │
  │   │   ├─ CMP_Core        │                 │   ├─ Platform       │   │
  │   │   ├─ KTX-Software    │                 │   ├─ Editor         │   │
  │   │   └─ CookedFormat*   │                 │   └─ imgui          │   │
  │   │                      │                 │                     │   │
  │   │  Does NOT link:      │                 │  Does NOT link:     │   │
  │   │   ✗ Renderer         │                 │   ✗ Assimp          │   │
  │   │   ✗ RHI              │                 │   ✗ cgltf           │   │
  │   │   ✗ D3D12            │                 │                     │   │
  │   │   ✗ imgui            │                 │                     │   │
  │   │                      │                 │                     │   │
  │   └──────────────────────┘                 └─────────────────────┘   │
  │                                                                      │
  │   * CookedAssetFormat.h is a shared header used by both.             │
  │     It defines magic numbers, version, and binary layout structs.    │
  │     No library link needed — header-only or compiled into each.      │
  │                                                                      │
  └──────────────────────────────────────────────────────────────────────┘
```

## 9. User Workflow

```
  CONTENT ARTIST / DEVELOPER WORKFLOW
  ════════════════════════════════════

  Step 1: Author content in DCC (Blender, 3ds Max, Maya)
  ────────────────────────────────────────────────────────
       Export ──► Bistro.fbx  (or .gltf)
                  saved to content/raw/bistro/


  Step 2: Cook assets  (one-time, or re-run when source changes)
  ──────────────────────────────────────────────────────────────
       > Scripts\CookAssets.bat content/raw/bistro/Bistro.fbx content/cooked/bistro

       SparkleAssetConverter.exe
         ├─ reads Bistro.fbx via Assimp
         ├─ extracts meshes, materials, textures
         ├─ validates + optimizes
         ├─ compresses textures ──► .ktx2 (BC7/BC5)
         └─ writes:
              content/cooked/bistro/
                ├─ bistro.sasset
                ├─ bistro.smesh
                ├─ bistro.smat
                ├─ bistro.stex
                └─ textures/
                     ├─ wall_albedo.ktx2
                     ├─ wall_normal.ktx2
                     └─ ...

       Console output:
         [OK]  Imported 847 meshes, 92 materials, 156 textures
         [WARN] 3 materials have unsupported clearcoat — using default
         [WARN] 1 mesh has degenerate triangles — removed
         [TIME] Total: 12.4s (parse: 2.1s, textures: 8.9s, write: 1.4s)


  Step 3: Reference cooked asset in level desc
  ─────────────────────────────────────────────
       levels/bistro.json:
       {
         "name": "Bistro",
         "cameraDesc": { ... },
         "lightingDesc": { ... },
         "importedMeshRequests": [
           { "assetPath": "content/cooked/bistro/bistro.sasset" }
         ]
       }


  Step 4: Run engine — loads cooked data only
  ────────────────────────────────────────────
       > bin/Debug/SparkleEngine.exe

       CookedAssetLoader reads .sasset ──► .smesh, .smat, .stex
         fast binary memcpy-style load
         no Assimp, no cgltf, no source format parsing

       GameScene populates:
         SceneMaterials ← 92 MaterialDescs
         SceneTextures  ← 156 .ktx2 paths
         SceneMeshes    ← 847 MeshComponents with transforms

       Renderer unchanged:
         CaptureSnapshot() ──► RenderSceneData ──► draw calls
```

## 10. Role Summary

```
  ┌──────────────────────────┬──────────────────────────────────────────────────┐
  │  Component               │  Role                                           │
  ├──────────────────────────┼──────────────────────────────────────────────────┤
  │                          │                                                 │
  │  FbxImporter             │  Reads .fbx via Assimp, fills SceneImportResult │
  │                          │  Lives in converter tool only                   │
  │                          │                                                 │
  │  GltfImporter            │  Reads .gltf/.glb via cgltf, fills same result  │
  │                          │  Replaces current runtime GltfLoader            │
  │                          │  Lives in converter tool only                   │
  │                          │                                                 │
  │  SceneImporter           │  Dispatches by file extension to correct        │
  │                          │  importer front-end                             │
  │                          │                                                 │
  │  SceneImportResult       │  Format-agnostic intermediate data structure    │
  │                          │  Meshes + materials + transforms + textures     │
  │                          │  Never crosses into runtime engine              │
  │                          │                                                 │
  │  ImportValidator         │  Validates + optimizes import result            │
  │                          │  Dedup, path normalize, degenerate removal      │
  │                          │                                                 │
  │  TextureCooker           │  Loads source images, generates mipmaps,        │
  │                          │  block-compresses, writes .ktx2                 │
  │                          │  Uses stb + Compressonator + KTX already in repo│
  │                          │                                                 │
  │  CookedAssetWriter       │  Serializes SceneImportResult + cooked textures │
  │                          │  into .smesh/.smat/.stex/.sasset binary files   │
  │                          │                                                 │
  │  CookedAssetFormat       │  Shared header defining binary layout structs,  │
  │                          │  magic numbers, version — used by writer+loader │
  │                          │                                                 │
  │  CookedAssetLoader       │  Runtime: reads cooked binaries, populates      │
  │                          │  GameScene subsystems. No format parsing.       │
  │                          │  Replaces GltfLoader in runtime load path       │
  │                          │                                                 │
  │  GameScene               │  UNCHANGED OWNER of runtime scene data          │
  │                          │  SceneMaterials, SceneTextures, SceneMeshes     │
  │                          │                                                 │
  │  LevelDesc               │  CHANGED: assetPath now points to .sasset       │
  │                          │  instead of raw .gltf/.fbx                      │
  │                          │                                                 │
  │  LevelManager            │  UNCHANGED orchestration, just calls            │
  │                          │  CookedAssetLoader instead of GltfLoader        │
  │                          │                                                 │
  │  RenderSceneSnapshot     │  UNCHANGED: captures from GameScene             │
  │                          │                                                 │
  │  RenderSceneDataBuilder  │  UNCHANGED: builds GPU-ready data from snapshot │
  │                          │                                                 │
  │  MaterialCacheManager    │  UNCHANGED: builds descriptor tables            │
  │                          │                                                 │
  │  GPUMeshCache            │  UNCHANGED: uploads VB/IB to GPU               │
  │                          │                                                 │
  │  TextureManager          │  MINOR: may need .ktx2 load path if not present │
  │                          │                                                 │
  └──────────────────────────┴──────────────────────────────────────────────────┘
```

## 11. Implementation Order

```
  PHASE 0                PHASE 1               PHASE 2              PHASE 3
  Define formats         Build converter       Wire runtime         Harden
  ─────────────          ───────────────       ────────────         ───────

  ┌─────────────┐        ┌──────────────┐      ┌──────────────┐    ┌──────────────┐
  │CookedAsset  │        │SceneImport   │      │CookedAsset   │    │Material      │
  │Format.h     │───────►│Result        │─────►│Loader        │───►│translation   │
  │             │        │              │      │              │    │hardening     │
  │ magic nums  │        │GltfImporter  │      │GameScene     │    │              │
  │ version     │        │(migrate from │      │wiring change │    │Bistro-scale  │
  │ binary      │        │ GltfLoader)  │      │              │    │validation    │
  │ layout      │        │              │      │LevelDesc     │    │              │
  │ structs     │        │FbxImporter   │      │path change   │    │Texture       │
  │             │        │(Assimp)      │      │              │    │cooking       │
  │             │        │              │      │Remove old    │    │integration   │
  │             │        │CookedAsset   │      │GltfLoader    │    │              │
  │             │        │Writer        │      │from runtime  │    │              │
  └─────────────┘        │              │      └──────────────┘    └──────────────┘
                         │ImportValid.  │
                         │              │
                         │Converter     │
                         │Main.cpp      │
                         └──────────────┘

  Exit: header           Exit: converter       Exit: engine         Exit: Bistro
  compiles in both       produces valid        loads cooked         loads and renders
  tool + runtime         .smesh/.smat etc      assets only          with good materials
                         from test .gltf
```
