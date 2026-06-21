# RTIndirectSpecular Material Parity Validation

This note defines the Stage 8.6 validation coverage for `RTIndirectSpecular` material texture parity. It is validation coverage only; it does not introduce new rendering behavior.

## Validation Levels

Use the Showcase validation levels below as stable startup levels:

- `RTIndirectSpecularMaterialParity_DamagedHelmet`
  - asset: `DamagedHelmet/DamagedHelmet`
  - covers base-color texture, roughness texture, metallic texture, emissive texture, normal map, and direct-vs-reflected material comparison on a compact PBR asset
- `RTIndirectSpecularMaterialParity_AlphaTest`
  - asset: `DiffuseTransmissionPlant/DiffuseTransmissionPlant`
  - covers alpha-tested candidate-hit handling after Stage 8.5
- `RTIndirectSpecularMaterialParity_RoughnessRange`
  - asset: `ABeautifulGame/ABeautifulGame`
  - covers a broader textured scene for mirror and stochastic GGX review across varied material roughness

The current Showcase content does not include a dedicated synthetic constant-only swatch scene. Until that asset exists, validate constant-only behavior with untextured/default material content if present in the active project, and keep textured parity claims tied to the levels above.

## Required CVar Sets

Base enabled setup:

```powershell
--cvar r.RayTracing.Reflections.Enabled=1
--cvar r.RayTracing.Reflections.MaxDistance=50
--cvar r.Material.BindingMode=0
```

Mirror mode:

```powershell
--cvar r.RayTracing.Reflections.SampleMode=0
--cvar r.RayTracing.Reflections.DebugMode=0
```

Stochastic GGX mode:

```powershell
--cvar r.RayTracing.Reflections.SampleMode=1
--cvar r.RayTracing.Reflections.DebugMode=0
```

Material debug views:

```powershell
--cvar r.RayTracing.Reflections.DebugMode=15  # sampled base color
--cvar r.RayTracing.Reflections.DebugMode=16  # sampled roughness/metallic
--cvar r.RayTracing.Reflections.DebugMode=17  # sampled emissive
--cvar r.RayTracing.Reflections.DebugMode=20  # hit tangent
--cvar r.RayTracing.Reflections.DebugMode=21  # hit bitangent
--cvar r.RayTracing.Reflections.DebugMode=22  # sampled tangent-space normal
--cvar r.RayTracing.Reflections.DebugMode=23  # final sampled world normal
--cvar r.RayTracing.Reflections.DebugMode=24  # alpha accepted/rejected
--cvar r.RayTracing.Reflections.DebugMode=25  # sampled alpha
--cvar r.RayTracing.Reflections.DebugMode=26  # alpha cutoff
```

## Launcher Smoke Commands

D3D12 mirror material parity:

```powershell
.\artifacts\dev\tools\SparkleLauncher\DevelopmentEditor\SparkleLauncher.exe --project Showcase --launch-target runtime --startup-level RTIndirectSpecularMaterialParity_DamagedHelmet --smoke-test --smoke-backend d3d12 --smoke-view-mode Lit --smoke-skip-level-switching --cvar r.RayTracing.Reflections.Enabled=1 --cvar r.RayTracing.Reflections.SampleMode=0 --cvar r.Material.BindingMode=0
```

D3D12 stochastic material parity:

```powershell
.\artifacts\dev\tools\SparkleLauncher\DevelopmentEditor\SparkleLauncher.exe --project Showcase --launch-target runtime --startup-level RTIndirectSpecularMaterialParity_DamagedHelmet --smoke-test --smoke-backend d3d12 --smoke-view-mode Lit --smoke-skip-level-switching --cvar r.RayTracing.Reflections.Enabled=1 --cvar r.RayTracing.Reflections.SampleMode=1 --cvar r.Material.BindingMode=0
```

D3D12 alpha-tested candidate-hit validation:

```powershell
.\artifacts\dev\tools\SparkleLauncher\DevelopmentEditor\SparkleLauncher.exe --project Showcase --launch-target runtime --startup-level RTIndirectSpecularMaterialParity_AlphaTest --smoke-test --smoke-backend d3d12 --smoke-view-mode Lit --smoke-skip-level-switching --cvar r.RayTracing.Reflections.Enabled=1 --cvar r.RayTracing.Reflections.SampleMode=0 --cvar r.RayTracing.Reflections.DebugMode=24 --cvar r.Material.BindingMode=0
```

Supported Vulkan path:

```powershell
.\artifacts\dev\tools\SparkleLauncher\DevelopmentEditor\SparkleLauncher.exe --project Showcase --launch-target runtime --startup-level RTIndirectSpecularMaterialParity_DamagedHelmet --smoke-test --smoke-backend vulkan --smoke-view-mode Lit --smoke-skip-level-switching --cvar r.RayTracing.Reflections.Enabled=1 --cvar r.RayTracing.Reflections.SampleMode=0 --cvar r.Material.BindingMode=0
```

## Coverage Matrix

| Requirement | Level | Mode / Debug |
| --- | --- | --- |
| direct-vs-reflected base material | `RTIndirectSpecularMaterialParity_DamagedHelmet` | mirror and stochastic |
| base-color texture parity | `RTIndirectSpecularMaterialParity_DamagedHelmet` | debug 15 |
| roughness texture parity | `RTIndirectSpecularMaterialParity_DamagedHelmet` | debug 16 |
| metallic texture parity | `RTIndirectSpecularMaterialParity_DamagedHelmet` | debug 16 |
| emissive texture parity | `RTIndirectSpecularMaterialParity_DamagedHelmet` | debug 17 |
| normal-map parity | `RTIndirectSpecularMaterialParity_DamagedHelmet` | debug 20-23 |
| alpha-tested candidate rejection | `RTIndirectSpecularMaterialParity_AlphaTest` | debug 24-26 |
| broad roughness-scale review | `RTIndirectSpecularMaterialParity_RoughnessRange` | mirror and stochastic |
| D3D12 backend | all levels | `--smoke-backend d3d12` |
| Vulkan backend | all levels when supported | `--smoke-backend vulkan` |
| primary debug binding mode | all levels | `r.Material.BindingMode=0` |

## Fail-Closed Checks

- Unsupported descriptor indexing or material texture table capability must report an unavailable material-texture path; it must not shade flagged textured materials as constants.
- Invalid material texture descriptors are renderer material-table contract failures. They should prevent texture parity from being enabled, not appear as a supported shader debug path.
- `RaytracingOnly` is the primary parity configuration because raster/GBuffer remains bindful and acts as the comparison baseline.
- `Everything` should not be used for parity sign-off until raster bindless opt-in exists and can be toggled against the bindful baseline.

## Known Differences

- RT hit texture sampling currently uses fixed explicit mip 0; raster material sampling uses normal derivative-based texture filtering.
- One sample per pixel stochastic GGX is expected to be noisy without denoising or history.
- Secondary shadow rays at the reflection hit are not part of the current validation target.
- Alpha-blended geometry remains unsupported and should fail as `UnsupportedAlphaMode`.
- Skinned/deformed RT hit-data parity remains unsupported until TLAS geometry and hit buffers are built from the same deformed snapshot.
