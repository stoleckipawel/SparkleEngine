# Scene Hierarchy And Component Architecture Plan

## Goal

Define the target scene hierarchy and component architecture for the Sparkle rewrite before implementation begins.

This document is intentionally planning-first.

It does not try to turn Sparkle into a copy of another engine.

It does assume Sparkle should study one strong external reference and translate its useful architectural ideas into a simpler engine architecture that fits Sparkle.

For this rewrite, NVIDIA Donut is the main reference.

## Main Reference

Sparkle should use NVIDIA Donut as the main architecture reference for:

- scene graph ownership
- transform hierarchy
- imported-scene representation
- separation between scene maintenance and render consumption

Why Donut is the best fit:

- it is close to Sparkle's actual problem space
- it is scene-graph-first
- it handles glTF and scene composition directly
- it separates engine scene representation from rendering passes
- it is much closer to Sparkle's current scale than a full gameplay engine framework

Donut is not a game engine.

That is useful here.

Sparkle does not currently need to copy a giant gameplay framework. It needs to fix scene ownership first.

## Core Links To Study

Repository:

- https://github.com/NVIDIA-RTX/Donut

README:

- https://github.com/NVIDIA-RTX/Donut#readme

Structure overview:

- https://github.com/NVIDIA-RTX/Donut#structure

Engine headers:

- https://github.com/NVIDIA-RTX/Donut/tree/main/include/donut/engine

Engine source:

- https://github.com/NVIDIA-RTX/Donut/tree/main/src/engine

Scene file documentation:

- https://github.com/NVIDIA-RTX/Donut/blob/main/doc/SceneFile.md

Samples:

- https://github.com/NVIDIA-RTX/Donut-Samples

## Study Order

Do not start from render passes.

Start with scene ownership and graph representation.

### 1. Read The README And Structure First

Read:

- https://github.com/NVIDIA-RTX/Donut#readme
- https://github.com/NVIDIA-RTX/Donut#structure

Learn:

- Donut has clear module boundaries: core, engine, render, app
- `donut_engine` is the layer Sparkle should study first
- scene maintenance is a separate concern from render passes

### 2. Read SceneGraph Next

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/SceneGraph.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/SceneGraph.cpp

Learn:

- where hierarchy ownership lives
- how parent-child relationships are represented
- how transform propagation is handled
- how scene graph refresh stays separate from rendering

### 3. Read Scene After That

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/Scene.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/Scene.cpp

Learn:

- what the scene owns in addition to the graph
- how graph data and resource data are organized together
- how Donut treats the scene as an explicit runtime object rather than a bag of unrelated managers

### 4. Read SceneTypes

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/SceneTypes.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/SceneTypes.cpp

Learn:

- how node-attached scene data is represented
- how scene data records are kept separate from the graph itself

### 5. Read glTF Import

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/GltfImporter.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/GltfImporter.cpp

Learn:

- how imported data becomes runtime scene representation
- how authored node relationships are preserved
- how import targets the scene graph instead of flat lists

### 6. Read View And Render-Facing Engine Structures

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/View.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/View.cpp

Learn:

- how scene-side state is turned into view-side render input
- where render consumption starts and scene ownership ends

### 7. Read Supporting Resource Systems Last

Read:

- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/Material.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/Material.cpp
- https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/TextureCache.h
- https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/TextureCache.cpp

Learn:

- how resource ownership is separated from graph ownership
- how nodes refer to assets and render resources without becoming asset managers themselves

## What Donut Is Teaching Sparkle

Donut is teaching Sparkle four core lessons.

### 1. The Graph Must Be Real

Hierarchy is not metadata.

Hierarchy is the runtime backbone.

If Sparkle wants production-style scene architecture, then parent-child transforms, activation, and scene relationships must live in one explicit runtime owner.

### 2. Transforms Belong To The Graph

The graph should own:

- local transforms
- world transforms
- dirtiness
- parent-child structure

That means transforms should not remain split between `Mesh`, `ImportedMesh`, and other scene wrappers.

### 3. Imported Scenes Must Enter The Runtime Through The Graph

Imported glTF nodes should become runtime scene nodes.

Meshes, cameras, and lights should attach to those nodes.

They should not flatten directly into final top-level scene buckets.

### 4. The Renderer Must Consume, Not Own

The renderer should consume render-facing scene data derived from the graph.

It should not become the hidden authority for gameplay scene organization.

## Why Donut Fits Better Than Falcor Or Cauldron

### Donut

Strongest at:

- graph-first runtime scene ownership
- transform propagation
- imported scene composition
- explicit scene maintenance

This matches Sparkle's biggest problem directly.

### Falcor

Strongest at:

- scene building for rendering
- render-oriented scene organization
- later optimization and render representation shaping

Falcor is more useful after Sparkle already has one authoritative scene model.

### AMD Cauldron

Strongest at:

- import pragmatism
- renderer-first ingestion
- glTF handling details

Cauldron is useful as a fallback reference, but not as the main ownership model for Sparkle.

## Sparkle's Current Biggest Problem

Sparkle does not currently have one authoritative runtime scene model.

Today:

- `GameScene` owns flat domain buckets
- `Mesh` owns `Transform`
- `ImportedMesh` owns a raw world matrix
- the renderer still snapshots from flat scene state directly

That creates:

- split transform ownership
- no first-class hierarchy semantics
- imported and authored scene data taking different paths
- render extraction compensating for missing scene authority

This is the main cleanup target.

## Sparkle Starting Architecture

```text
+-----------------------------------------------------------------------------------+
| LevelManager                                                                      |
|-----------------------------------------------------------------------------------|
| - owns active LevelAsset                                                          |
| - applies level data into GameScene                                               |
+-----------------------------------+-----------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------------------+
| GameScene                                                                          |
|-----------------------------------------------------------------------------------|
| +------------------+  +------------------+  +------------------+                  |
| | SceneCamera      |  | SceneLighting    |  | SceneMaterials   |                  |
| +------------------+  +------------------+  +------------------+                  |
|                                                                                   |
| +------------------+  +------------------+                                        |
| | SceneMeshes      |  | SceneTextures    |                                        |
| +------------------+  +------------------+                                        |
|                                                                                   |
| SceneMeshes is a flat mesh collection, not a hierarchy.                           |
+--------------------------+--------------------------------------+------------------+
                           |                                      |
                           | owns Mesh objects                    | owns ImportedMesh objects
                           v                                      v
              +-----------------------------+        +--------------------------------------+
              | Mesh                        |        | ImportedMesh                         |
              |-----------------------------|        |--------------------------------------|
              | - Transform m_transform     |        | - XMFLOAT4X4 m_worldTransform        |
              +-----------------------------+        +--------------------------------------+


+-----------------------------------------------------------------------------------+
| Renderer                                                                           |
|-----------------------------------------------------------------------------------|
| - keeps GameScene*                                                                 |
| - builds RenderSceneSnapshot from GameScene                                        |
| - scene render helpers also keep GameScene coupling                                |
+-----------------------------------+-----------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------------------+
| RenderSceneSnapshot::Capture(GameScene)                                            |
|-----------------------------------------------------------------------------------|
| Reads camera, lighting, textures, materials, and meshes directly from GameScene.  |
+-----------------------------------------------------------------------------------+
```

## Donut-Inspired Target Architecture

Sparkle should become graph-first.

Not generic-ECS-first.

Not renderer-first.

```text
+-----------------------------------------------------------------------------------+
| LevelManager                                                                      |
|-----------------------------------------------------------------------------------|
| - owns active LevelAsset                                                          |
| - requests scene load into GameScene                                              |
+-----------------------------------+-----------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------------------+
| GameScene                                                                          |
|-----------------------------------------------------------------------------------|
| +-----------------------------------------------------------------------+         |
| | SceneHierarchy                                                        |         |
| |-----------------------------------------------------------------------|         |
| | - owns nodes                                                          |         |
| | - owns parent/child links                                             |         |
| | - owns local transforms                                               |         |
| | - owns cached world transforms                                        |         |
| | - owns active state                                                   |         |
| +----------------------------+------------------------------------------+         |
|                              |                                                    |
|                              v                                                    |
|       +-------------------------------------------------------------------+      |
|       | SceneNode                                                         |      |
|       |-------------------------------------------------------------------|      |
|       | node id                                                           |      |
|       | local transform                                                   |      |
|       | world transform                                                   |      |
|       | parent                                                            |      |
|       | children                                                          |      |
|       | component mask                                                    |      |
|       +------------------+-------------------+----------------------------+      |
|                          |                   |                                   |
|                          v                   v                                   |
|              +------------------+  +------------------+                          |
|              | MeshComponent    |  | CameraComponent  |                          |
|              +------------------+  +------------------+                          |
|                                                                                   |
|              +-------------------------+                                          |
|              | DirectionalLightComponent|                                          |
|              +-------------------------+                                          |
+-----------------------------------+-----------------------------------------------+
                                    |
                                    | extraction boundary
                                    v
+-----------------------------------------------------------------------------------+
| Scene Extraction                                                                  |
|-----------------------------------------------------------------------------------|
| Traverses hierarchy and attached data to build render-facing scene packages.       |
+-----------------------------------+-----------------------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------------------+
| Renderer                                                                           |
|-----------------------------------------------------------------------------------|
| - owns GPU resources                                                               |
| - owns caches and pass setup                                                       |
| - consumes extracted scene data                                                    |
| - does not own live gameplay scene objects                                         |
+-----------------------------------------------------------------------------------+
```

## Core Decision

Sparkle should adopt a Donut-inspired scene-hierarchy-first component model.

That means:

- every runtime scene object lives on a hierarchy node
- the hierarchy owns parent-child relationships and transforms
- typed attached data describes mesh, camera, and light content on nodes
- render extraction reads from hierarchy plus attached data, not from unrelated ad hoc scene containers

Sparkle should not start with a generic gameplay ECS that ignores spatial hierarchy.

For Sparkle, hierarchy is the primary runtime abstraction.

## What Sparkle Should Copy From Donut

### 1. Real Scene Graph Ownership

Copy directly:

- one explicit scene graph owner
- one explicit path for transform propagation

### 2. Runtime Scene Built Around Nodes

Copy directly:

- imported scenes become node graphs
- attached scene content hangs off those nodes

### 3. Separation Between Graph And Resource Systems

Copy directly:

- graph owns spatial truth
- resource systems own materials, textures, and render-side assets

### 4. Separation Between Scene And Rendering

Copy directly:

- renderer consumes scene-derived render data
- renderer is not the owner of gameplay scene structure

## What Sparkle Should Not Copy Literally

### 1. Exact Type Names

Copy the architecture, not the names.

### 2. Render-Framework Scope

Donut is not a full gameplay engine.

Sparkle still needs to remain an engine runtime, not a rendering framework sample.

### 3. Extra Complexity Before It Is Needed

Sparkle does not need every scene feature immediately.

Start with:

- nodes
- transforms
- mesh attachment
- camera attachment
- directional light attachment

## Target Sparkle Types

### SceneNodeId

Responsibility:

- stable handle for scene node access
- supports generation-based validation

### SceneNodeRecord

Responsibility:

- compact hierarchy record owned by `SceneHierarchy`

Should contain:

- node id
- parent id
- child list
- local transform
- cached world transform
- dirty flag
- active flag
- component mask
- optional debug name

### SceneHierarchy

Responsibility:

- authoritative owner of runtime spatial scene structure

Should do:

- create nodes
- destroy nodes
- destroy subtrees
- reparent nodes
- update transforms
- propagate world transforms
- traverse active nodes

### MeshComponent

Responsibility:

- attach renderable mesh and material references to a node

Should contain:

- mesh handle or asset reference
- material handle or override
- visibility flags
- shadow flags

Should not contain:

- authoritative transform
- GPU cache ownership

### CameraComponent

Responsibility:

- attach camera settings to a node

Should contain:

- projection settings
- near and far planes
- field of view or ortho settings
- optional primary marker

### DirectionalLightComponent

Responsibility:

- attach directional light settings to a node

Should contain:

- color
- intensity
- shadow settings later

### SceneExtractor

Responsibility:

- read the graph and attached components
- build renderer-facing scene packages

### SceneResourceTables

Responsibility:

- hold scene-referenced meshes, materials, and textures separately from graph ownership

## How The Pieces Compose

```text
LevelAsset / Imported glTF / Authored scene data
                    |
                    v
            +------------------+
            | SceneHierarchy   |
            +------------------+
                    |
                    v
        +---------------------------+
        | SceneNode                 |
        |---------------------------|
        | transform + hierarchy     |
        +-------------+-------------+
                      |
        +-------------+-------------+------------------+
        |                           |                  |
        v                           v                  v
+------------------+    +------------------+   +-------------------------+
| MeshComponent    |    | CameraComponent  |   | DirectionalLightComponent|
+------------------+    +------------------+   +-------------------------+
        |
        v
+------------------+
| SceneExtractor   |
+------------------+
        |
        v
+------------------+
| Renderer         |
+------------------+
```

Composition rule:

- node owns spatial existence
- attached component describes what exists on that node
- extraction translates scene state into render state

## Sparkle Adjustment Map

### 1. GameScene Must Become Graph Owner

Current file:

- `Engine/GameFramework/Public/Scene/GameScene.h`

Required change:

- `GameScene` should own `SceneHierarchy` as the authoritative runtime scene structure
- flat scene buckets should become transitional adapters or extracted views only

### 2. Mesh Transform Ownership Must Be Removed

Current files:

- `Engine/GameFramework/Public/Scene/Mesh.h`
- `Engine/GameFramework/Public/Scene/ImportedMesh.h`

Required change:

- move transform authority into `SceneNode`
- make mesh objects non-authoritative for transforms

### 3. glTF Load Must Build The Graph

Current files:

- `Engine/GameFramework/Private/Assets/GltfLoader.cpp`
- `Engine/GameFramework/Private/Scene/GameScene.cpp`

Required change:

- imported nodes should become runtime scene nodes
- mesh, camera, and light attachments should be attached to those nodes

### 4. Renderer Snapshot Must Become Formal Extraction

Current files:

- `Engine/Renderer/Private/SceneData/RenderSceneSnapshot.cpp`
- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/SceneData/SceneRenderStateCoordinator.h`

Required change:

- replace flat-scene snapshot capture with graph-driven extraction
- keep renderer consumption read-only

### 5. LevelManager Must Stay Orchestration-Only

Current file:

- `Engine/GameFramework/Public/Runtime/Level/LevelManager.h`

Required change:

- keep level loading orchestration there
- do not make it the place where graph ownership or scene semantics accumulate

## Implementation Phases

The migration should keep Sparkle working after each phase.

### Phase 1: Build Graph Types In Isolation

Implement:

- `SceneNodeId`
- `SceneNodeRecord`
- `SceneHierarchy`
- `MeshComponent`
- `CameraComponent`
- `DirectionalLightComponent`

Do not integrate into the renderer yet.

What to test:

- node creation
- node destruction
- subtree destruction
- reparenting
- local-to-world transform propagation
- component attach and detach

Success gate:

- the graph works in isolation without touching rendering

### Phase 2: Prove Imported Scene Representation

Implement:

- glTF-to-SceneHierarchy conversion

What to test:

- imported node counts
- parent-child relationships
- transform correctness
- existing imported demo levels retain visual placement

Success gate:

- imported scenes are represented correctly in hierarchy form

### Phase 3: Add Transitional Adapters

Implement:

- compatibility adapters that rebuild current flat scene views from the graph

What to test:

- current rendering still works
- current camera flow still works
- scene loads produce equivalent visible results through the adapter path

Success gate:

- graph becomes authoritative while old runtime paths still function through adapters

### Phase 4: Introduce Formal Extraction

Implement:

- `SceneExtractor`
- graph-driven render-scene package build

What to test:

- render output matches current visual behavior
- mesh, material, and light counts match expected scene state
- renderer no longer depends on flat scene ownership directly

Success gate:

- renderer consumes graph-derived scene packages instead of flat scene buckets

### Phase 5: Remove Legacy Scene Ownership

Remove or demote as final authorities:

- `SceneMeshes`
- `SceneCamera`
- `SceneLighting`

Retain only if needed temporarily:

- lightweight compatibility views

What to test:

- startup scene load
- camera motion
- lighting correctness
- shadow correctness
- imported scene placement

Success gate:

- one authoritative owner exists for runtime spatial scene state

## Keep-The-Engine-Working Rules

### Rule 1: One Authority At A Time

If hierarchy becomes transform authority, remove competing transform authority from old paths once an adapter exists.

### Rule 2: Adapt Before Delete

Build compatibility views first.

Delete old ownership only after the graph path is proven stable.

### Rule 3: Preserve Visual Equivalence

At the end of each phase, verify:

- imported mesh placement
- active camera behavior
- light orientation and intensity
- material assignment
- scene load correctness

### Rule 4: Renderer Must Remain A Consumer

Even during transition, renderer code may consume adapted scene data, but it should not become the hidden owner of gameplay scene truth.

### Rule 5: Prefer Vertical Slices

Good slice:

- graph types
- glTF graph conversion
- adapter back into current renderer path

Bad slice:

- graph rewrite, scripting, physics, and renderer ownership rewrite all at once

## Final Recommendation

For Sparkle, Donut should be the main scene architecture reference.

Use it to drive these decisions:

- hierarchy is the runtime backbone
- transforms belong to the hierarchy
- imported scenes must become runtime graph nodes
- scene resources stay separate from graph ownership
- renderer consumes extracted scene state instead of owning gameplay scene state

Sparkle should not try to become Donut.

Sparkle should translate Donut's graph-first architecture into a cleaner engine runtime of its own.

## Component Responsibilities

### MeshInstanceComponent

Should contain:

- mesh handle or imported mesh reference
- material handle or override data
- visibility flags
- shadow participation flags

Should not contain:

- transform ownership
- renderer-owned GPU resource lifetime
- scene-parenting policy

### CameraComponent

Should contain:

- projection settings
- near and far settings
- field of view settings
- optional camera role or primary marker

Should not contain:

- free-fly controller logic
- input ownership
- node lifetime

### DirectionalLightComponent

Should contain:

- color
- intensity
- shadow settings later

Direction can either be stored directly or derived from node orientation, but the policy should be chosen once and kept consistent across all light types.

## Ownership Rules

These rules should be established before implementation starts.

### Node Ownership

- SceneHierarchy owns nodes
- destroying a node destroys its subtree
- node handles become invalid through generation changes

### Transform Ownership

- every node always has a transform
- no component owns the authoritative transform
- systems may modify node transforms through hierarchy APIs

### Component Ownership

- components are owned by SceneHierarchy or its typed component stores
- components reference assets or scene resources but do not own global managers
- node and component lifetimes are linked through node ownership

### Renderer Ownership

- renderer owns render-side caches and GPU residency only
- renderer does not own gameplay or scene hierarchy objects

## Hierarchy Semantics

### Creation

Creating a node should require only:

- optional name
- optional parent
- optional initial local transform

### Reparenting

Reparenting should support an explicit policy:

- preserve local transform
- preserve world transform

The default should be selected deliberately and documented.

### Activation

Nodes should support active or inactive state.

Inactive nodes should be excluded from normal extraction and update traversal unless a system explicitly requests otherwise.

### Destruction

Destroying a node should recursively destroy all descendants and attached components.

This should be a strong structural rule, not a best-effort convention.

## Data Access Strategy

The first implementation does not need a full sparse-set ECS.

Preferred starting point:

- stable node handle type
- vector-backed node storage with free list and generation
- simple typed component stores keyed by node ID

That is enough to keep the implementation understandable while still avoiding the dynamic_cast-heavy tutorial model.

If scale later demands denser storage or archetypes, that can be introduced after the hierarchy contract is stable.

## Non-Goals For The First Architecture Pass

Do not try to solve all future engine architecture at once.

Explicit non-goals:

- full generic ECS framework
- scripting integration
- physics integration
- prefab system
- world streaming model
- network replication model
- generalized event bus

These may come later, but they should not define the initial hierarchy design.

## Relationship To Existing Sparkle Structure

The current engine has scene wrappers for camera, lighting, materials, textures, and meshes.

During the rewrite, the destination should be:

- SceneHierarchy becomes the authoritative spatial and attached-content structure
- renderer extraction reads from SceneHierarchy plus referenced scene resources
- old wrapper-style scene ownership is either removed or reduced to transitional adapters

The rewrite should not preserve current wrappers as the final authority if they conflict with the hierarchy-first architecture.

## Staged Plan

### Phase 0: Lock The Structure On Paper

Before writing implementation code, define:

- node responsibilities
- component responsibilities
- lifetime rules
- transform policy
- reparenting rules
- component uniqueness rules
- renderer extraction boundary

Deliverable:

- this architecture note
- a follow-up API sketch if needed

### Phase 1: Build A Standalone Hierarchy Module

Implement only the hierarchy package and typed component storage.

Do not hook it into the whole engine yet.

Recommended package shape:

- Engine/GameFramework/Public/Scene/Hierarchy
- Engine/GameFramework/Private/Scene/Hierarchy

Expected types:

- SceneNodeId
- SceneNodeFlags
- SceneNodeRecord
- SceneHierarchy
- MeshInstanceComponent
- CameraComponent
- DirectionalLightComponent

Success condition:

- a small test harness can create a tree, attach components, dirty transforms, and compute world transforms correctly

### Phase 2: Prove It Can Represent Imported And Authored Scenes

Build conversion helpers from imported scene data into the new hierarchy.

Do not replace the full runtime yet.

Success condition:

- imported scenes and simple authored setups can be represented without awkward exceptions

### Phase 3: Introduce Render Extraction Boundary

Add extraction logic that converts hierarchy plus components into renderer-facing data.

At this phase:

- renderer still consumes extracted snapshots or packages
- renderer does not traverse arbitrary gameplay objects directly

Success condition:

- renderable scene data can be produced entirely from hierarchy state

### Phase 4: Replace Legacy Scene Ownership

Once extraction is stable, remove or demote legacy scene ownership structures that duplicate hierarchy authority.

Success condition:

- there is one clear owner for spatial scene state

## Recommended Folder Direction

When implementation starts, prefer a clean module grouping that makes hierarchy obvious.

Suggested destination:

- Engine/GameFramework/Public/Scene/Hierarchy/SceneHierarchy.h
- Engine/GameFramework/Public/Scene/Hierarchy/SceneNodeId.h
- Engine/GameFramework/Public/Scene/Hierarchy/Components/MeshInstanceComponent.h
- Engine/GameFramework/Public/Scene/Hierarchy/Components/CameraComponent.h
- Engine/GameFramework/Public/Scene/Hierarchy/Components/DirectionalLightComponent.h

Keep the structure explicit. Avoid hiding the hierarchy model inside broad generic folders with unclear ownership.

## Decision Summary

For the Sparkle rewrite:

- follow the Vulkan tutorial in principle
- use AMD and NVIDIA repositories only when the tutorial is too abstract
- build a scene-hierarchy-first component model
- make transform intrinsic to hierarchy nodes
- start with a small typed component set
- keep renderer ownership separate from scene ownership
- plan the replacement structure first, then integrate later

This is the architecture Sparkle should target before implementation details begin.