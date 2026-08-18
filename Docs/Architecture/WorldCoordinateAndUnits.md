# World Coordinate, Units, and Transform Contract

Status: canonical architecture decision
Adoption: accepted contract; code, cooked content, and executable tests prove conformance

## Responsibility

This document is the single source of truth for SparkleEngine world axes, physical units, transform math, source normalization, animation and skinning spaces, and the boundary between canonical engine data and backend-specific representations.

It owns semantic coordinate decisions. Source-format specifications own their source conventions; the [GameFramework and ECS standard](../Engineering/Standards/GameFrameworkAndEcs.md) owns world and animation integration rules; the [Graphics Engineering standard](../Engineering/Standards/GraphicsEngineering.md) owns graphics implementation and evidence; and the [Renderer/RHI boundary](RendererRhiBoundary.md) owns backend responsibilities. Code, cooked schemas, and tests remain the proof that this decision is implemented.

No project, level, importer, renderer path, or future physics integration may define an alternate Sparkle world basis or unit scale.

## Canonical World

Sparkle uses one world coordinate system:

| Property | Canonical value |
| --- | --- |
| Handedness | Left-handed |
| Right | `+X`, vector `(1, 0, 0)` |
| Up | `+Y`, vector `(0, 1, 0)` |
| Forward | `+Z`, vector `(0, 0, 1)` |
| Back | `-Z`, vector `(0, 0, -1)` |
| Distance | metres |
| Angle | radians, except explicitly named presentation fields such as `FovYDegrees` |
| Time | seconds |

```text
                 +Y Up
                   |
                   |
                   O---------- +X Right
                  /
                 /
          +Z Forward
```

The world origin is scene-defined. Origin placement, large-world rebasing, and geospatial coordinates may change position representation, but they do not change the canonical axes or units. A geospatial or physics SDK adapter converts at its boundary and publishes canonical engine values.

Axis gizmos use red for X, green for Y, and blue for Z. A gizmo label must include the space it visualizes when that space is not WorldSpace.

## Canonical Units

- One world unit is one metre.
- Linear positions, translations, distances, extents, bounds, camera clip distances, light ranges, velocities, and accelerations use metres or metre-derived SI units.
- Scalar time crossing a boundary names seconds. Animation input times and frame delta values are seconds.
- Runtime angular values use radians. A degree-valued UI or serialized field names `Degrees` and converts once at its owner.
- Scale, normalized directions, skin weights, morph weights, and quaternion components are dimensionless.
- Light quantities keep the radiometric or photometric unit named by their owning contracts; coordinate conversion must not reinterpret intensity as distance.

Names use the physical unit when a type does not carry it, for example `MoveSpeedMetersPerSecond`, `DurationSeconds`, and `FovYRadians`. Established projection fields retain `NearZ` and `FarZ`, with their metre unit fixed by this contract. `WorldUnits` is not an acceptable substitute for metres in a persistent or cross-module contract.

## Transform Math

### Semantic convention

Sparkle evaluates positions as row vectors on the left:

```text
transformedPoint = point * matrix
```

For authored translation, rotation, and scale:

```text
LocalMatrix = ScaleMatrix * RotationMatrix * TranslationMatrix
WorldMatrix = LocalMatrix * ParentWorldMatrix
RootWorldMatrix = RootLocalMatrix
```

Hierarchy traversal order does not change this multiplication order. Computing a root-to-leaf chain still prepends each child local transform to its parent world transform.

Quaternion storage is `X, Y, Z, W`, where W is the scalar component. Runtime quaternions are finite, non-zero, normalized before use, and kept in a continuous sign hemisphere along animation tracks.

### Storage and ABI

Canonical C++ matrices use DirectXMath row-major element naming. Translation is stored in `_41`, `_42`, and `_43`. Canonical HLSL payloads declare matrices `row_major` and transform with `mul(rowVector, matrix)`. Cooked matrices preserve that canonical C++ representation.

Row-vector semantics do not imply one universal backend byte layout. Source formats, D3D12, and Vulkan may expose different matrix storage or packing rules. The owning boundary performs an explicit transpose or pack exactly once. In particular, a native ray-tracing instance transform is a row-major 3x4 column-vector affine payload, so the RHI packing contract transposes the canonical linear transform and places `_41`, `_42`, and `_43` in the native translation column. A `memcpy` is valid only when a known-value ABI test proves both layout and semantics.

Backend clip-space depth, viewport orientation, and projection packing do not redefine WorldSpace. Renderer/RHI projection code owns those API adaptations after WorldSpace and ViewSpace semantics are fixed.

### Semantic value types

| Value | Homogeneous W | Conversion rule |
| --- | ---: | --- |
| Point or position | `1` | Basis conversion plus unit scale and translation where applicable |
| Translation or position delta | `0` | Basis conversion plus unit scale; no origin translation |
| Direction | `0` | Basis conversion only, then normalize when the contract requires a unit direction |
| Normal | `0` | Inverse-transpose of the relevant linear transform, then normalize |
| Tangent XYZ | `0` | Linear direction conversion, orthogonalization where required, then normalize |
| Tangent W | n/a | Multiply by the sign of the basis determinant when a reflection changes handedness |
| Morph position delta | `0` | Same basis and unit conversion as a translation delta |
| Morph normal/tangent delta | `0` | Same linear basis conversion as its semantic vector |

A generic `ConvertFloat3` API is prohibited at source boundaries because it cannot distinguish points, directions, normals, and deltas.

### Reflections

A handedness-changing basis conversion has a negative determinant. The importer must therefore convert all related semantics as one operation:

- vertex positions, normals, tangents, and morph deltas;
- triangle winding;
- tangent-frame handedness;
- node, camera, light, and instance transforms;
- translations, rotations, and cubic animation tangents;
- inverse bind matrices and skeleton reference transforms;
- bounds regenerated from converted values.

Changing one vector component without changing winding, tangent handedness, animation, and bind data is not a complete conversion.

## Named Spaces

| Space | Meaning |
| --- | --- |
| SourceSpace | Format-owned basis and units before normalization; never crosses the importer boundary |
| MeshLocalSpace | Canonical coordinates stored in a static mesh |
| SkinReferenceSpace | Canonical object/model space shared by skinned vertices, inverse binds, and evaluated joint matrices; its placement in WorldSpace belongs to the mesh instance |
| JointLocalSpace | A joint node's canonical animated TRS relative to its source parent |
| JointParentSpace | Fixed transform collapsing non-joint ancestors between a joint and its nearest parent joint, or between a root joint and SkinReferenceSpace |
| JointModelSpace | Evaluated joint transform in SkinReferenceSpace |
| ObjectSpace | The canonical local space to which an entity's `WorldMatrix` applies; for a skinned object this is SkinReferenceSpace |
| WorldSpace | Canonical runtime scene coordinates |
| ViewSpace | Camera-relative coordinates after `WorldToViewMatrix` |
| ClipSpace / NDC | Renderer and backend projection spaces; not source or world coordinates |

Matrix and variable names state both directions when ambiguity is possible, such as `WorldToViewMatrix`. `WorldMatrix` retains its established meaning: ObjectSpace to WorldSpace.

## Source Import Boundary

All source formats enter one pipeline:

```text
Source asset space
    |  format-specific basis and units
    v
Importer normalization - exactly once
    |  produces canonical engine coordinates
    v
ImportedScene - LH, +Y up, +Z forward, metres
    |  validated as canonical import output
    v
Cooked assets - canonical; no source-space remnants
    v
Runtime ECS / animation / renderer
    |
    +-- Raster: backend packing only
    `-- Ray tracing: the same transforms and joint matrices
```

Every importer must:

1. determine the source handedness, semantic right/up/forward axes, linear unit, angular unit, transform convention, and camera/light local directions from authoritative format metadata or specification;
2. reject missing or contradictory metadata when no deterministic format default exists;
3. normalize every spatial semantic into the canonical engine contract;
4. record source metres-per-unit in provenance;
5. validate finite values, invertibility where required, normalized directions/quaternions, hierarchy ownership, and converted bounds before publication;
6. publish only a canonical `ImportedScene`.

`ImportedScene` is the canonical-coordinate representation, so it carries no alternate-basis tag or contract version. Runtime loading is cooked-only and validates the current artifact's type identity, concrete record layout, ranges, and semantic invariants. When that representation changes, local cooked artifacts are cleared and regenerated from source; no historical reader or runtime compatibility conversion is retained.

### glTF 2.0 mapping

The [glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#coordinate-system-and-units) defines a right-handed system with `+Y` up, `+Z` forward, `-X` right, metres, and radians. Sparkle preserves those semantic directions:

| glTF semantic vector | Sparkle vector |
| --- | --- |
| Right `(-1, 0, 0)` | Right `(1, 0, 0)` |
| Up `(0, 1, 0)` | Up `(0, 1, 0)` |
| Forward `(0, 0, 1)` | Forward `(0, 0, 1)` |

The corresponding coordinate reflection is:

```text
engineVector = (-source.x, source.y, source.z)
```

This reflection is applied through semantic conversion functions. Rotation conversion is defined by matrix equivalence under the basis change, not by an unverified quaternion sign guess. For the selected reflection, a quaternion value maps linearly from `(x, y, z, w)` to `(x, -y, -z, w)` before normalization; quaternion cubic tangents receive the same linear map without normalization.

glTF cameras and punctual spot/directional lights use local `-Z` for viewing or emission. Their adapters construct canonical transforms whose local `+Z` points along the same authored world direction. The node transform is not treated as if the source convention already used engine camera/light forward.

### FBX and decoder-normalized formats

FBX axes and units come from file metadata. If a third-party decoder performs axis or global-scale conversion, that conversion is part of the importer and must be configured once, recorded, and covered by fixtures. The importer remains responsible for semantics the decoder does not convert, including camera clip distances, camera/light local offsets, light dimensions/ranges, attenuation coefficients, and any camera/light direction convention.

## Geometry and Instancing

- Static mesh vertices remain in MeshLocalSpace. Their instance `WorldMatrix` places them in WorldSpace once.
- An authored instance transform is local to its node. With row vectors: `InstanceWorld = InstanceLocal * NodeWorld`.
- A glTF skin defines `SkinReferenceToWorld` from the first inverse bind and first joint bind world transform. Joint bind models are rebased by `WorldToSkinReference`, while the mesh instance owns `SkinReferenceToWorld`. The skinned mesh node's own transform remains ignored as required by glTF.
- Bounds are computed or regenerated from canonical geometry and canonical transforms.
- Source-basis reflections preserve the selected front-face convention through explicit winding and tangent-frame handling before canonical geometry is published.
- Raster and ray-tracing geometry use the same MeshLocalSpace, instance world transforms, morph deltas, and skinning results.

## Cameras and Lights

- Engine camera local forward is `+Z`, up is `+Y`, and right is `+X`.
- `WorldToViewMatrix` is left-handed. Camera near/far distances are metres.
- Engine directional, spot, and projector local emission direction is `+Z` unless a focused feature contract explicitly names another local convention.
- Imported camera and light directions are derived from their normalized canonical transform or stored as canonical normalized WorldSpace directions. The transform and stored direction must agree.
- A format's camera/light local direction is adapted once during import; Renderer and RHI never branch on source format.

## Animation and Root Motion

- Animation translations are canonical metres, rotations are canonical normalized XYZW quaternions, scale is dimensionless, and input time is seconds.
- Translation and rotation cubic tangents undergo the same spatial basis map as their values. Quaternion tangents are not normalized.
- Quaternion values use sign continuity. Negating a cubic quaternion value also negates its associated in/out tangents.
- Joint animation samples the source joint-local TRS, then applies the joint's fixed `JointParentSpace` transform before parent-joint composition. This preserves non-joint ancestors without baking format-specific transforms into every key.
- Root-joint animation remains inside the evaluated pose and moves vertices in SkinReferenceSpace. Sparkle does not automatically transfer root motion to the entity `WorldMatrix`.
- A future gameplay root-motion feature must explicitly extract a named translation/rotation interval, remove that motion from the pose, and apply it to the entity once. It may not leave the same motion in both pose and object placement.

## Skeleton and Skinning Contract

Each joint stores:

- canonical source-node bind TRS as `BindLocalTransform`;
- a fixed `JointParentSpace` transform that collapses non-joint ancestors;
- canonical `BindModelTransform` in SkinReferenceSpace;
- an `InverseBindMatrix` mapping SkinReferenceSpace vertices into joint bind space;
- the nearest parent joint index, independent of intervening non-joint nodes.

Evaluation uses row-vector order:

```text
JointCollapsedLocal = AnimatedJointLocal * JointParentSpace
JointModel          = JointCollapsedLocal * ParentJointModel
JointMatrix         = InverseBindMatrix * JointModel
WorldPosition       = SkinnedObjectPosition * ObjectWorldMatrix
```

For a root joint, `ParentJointModel` is identity and `JointParentSpace` relates its source parent to SkinReferenceSpace. Source ancestors above that reference are retained by the mesh instance's `WorldMatrix`, so replacing an animated root joint's TRS cannot erase them.

The bind-pose invariant is:

```text
BindLocal * JointParentSpace * ParentBindModel == BindModel
```

The skinning invariant is that `JointMatrix` produces ObjectSpace/SkinReferenceSpace positions. The entity `WorldMatrix` is applied exactly once afterward. A source format such as glTF that requires a skinned mesh node transform to be ignored must not publish that ignored transform as the entity `WorldMatrix`; the instance publishes the independently derived `SkinReferenceToWorld` placement instead.

## Runtime, Renderer, RHI, and Physics

- ECS stores canonical local and world transforms only.
- Render extraction copies or derives canonical values; it does not perform source-format conversion.
- Raster and ray tracing consume the same `WorldMatrix`, previous transform, morph values, and object-space joint matrices.
- RHI backends pack canonical matrices for their ABI and projection conventions. They do not reflect the world or change its unit scale.
- A future physics integration stores gameplay authority in canonical metres with gravity toward `-Y` and adapts handedness, axes, units, shapes, transforms, velocities, normals, and query results inside the physics adapter.

## Conformance Gates

### Known-value math tests

- canonical right/up/forward basis and camera movement;
- source basis vectors mapping to the canonical basis;
- point, direction, normal, tangent, delta, winding, and tangent-W conversion;
- matrix conversion commuting with transformed points;
- quaternion conversion matching converted rotation matrices;
- normalized and sign-continuous quaternion tracks;
- noncommuting parent/child transforms proving `Local * ParentWorld`;
- non-uniform scale, inverse, and inverse-transpose behavior;
- native ray-tracing 3x4 packing preserving canonical rotation, scale, and translation.

### Import fixtures

- an asymmetric XYZ tripod with labelled positive axes;
- nested translation/rotation/scale with noncommuting parents;
- camera and directional/spot-light forward vectors;
- authored GPU instances;
- a skin with non-joint ancestors, root translation/rotation, and inverse binds;
- equivalent glTF and FBX fixtures producing the same canonical semantic values within a declared epsilon;
- unit-scaled FBX camera clips, light offsets/dimensions, and attenuation.

### Runtime and rendering

- bind pose does not jump when animation begins at time zero;
- reconstructed bind models satisfy the bind-pose invariant;
- root motion follows the converted source forward/right/up semantics;
- a skinned object's placement is applied exactly once;
- raster and ray-traced deformed vertices and bounds agree;
- D3D12 and Vulkan show the same winding, culling, lighting, camera orientation, and motion;
- the current skinned glTF regression asset, CesiumMan, is checked at multiple animation frames rather than only at bind pose.

### Artifact validation and regeneration

- imported provenance records source metres-per-unit while the imported scene type itself guarantees canonical coordinates;
- scene, mesh, skeleton, and animation cooked headers identify their asset type and declare only current structural data;
- loaders validate the current structure and semantics, with actionable recook errors where a malformed artifact is detected;
- a contract change clears and deterministically recooks all affected local spatial content instead of introducing a format version or compatibility path;
- no asset-specific axis rotations, per-level unit scales, or backend-specific world corrections remain.

## Rejected Patterns

Do not:

- fix one asset with a level rotation or an importer-name special case;
- convert geometry while leaving animation, inverse binds, cameras, lights, morphs, or instances in SourceSpace;
- apply a reflection without correcting winding and tangent handedness;
- infer a vector's semantic meaning from its storage type;
- use parent/child multiplication order copied from a column-vector source format;
- preserve both a source node world transform and an equivalent skin/root transform;
- call unspecified scale `world units` at a persistent boundary;
- convert at cook, load, ECS, renderer, shader, or backend stages after import has already published canonical data;
- accept old cooked spatial data through silent compatibility paths.
