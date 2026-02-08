<!-- ========================================================================= -->
<!-- TEXTURE INTEGRATION PLAN — Per-Draw SRV Binding (Option A)               -->
<!-- Full Sponza PBR Texture Support                                          -->
<!-- Sparkle Engine                                                           -->
<!-- ========================================================================= -->

# Texture Integration Plan — Full PBR Texture Support

**Parent:** [TEXTURE_PIPELINE_DESIGN.md](TEXTURE_PIPELINE_DESIGN.md) (Option A selected)
**Status:** Planned
**Date:** 2026-02-08

---

## 1. Scope

Support all texture types present in Sponza and standard glTF PBR materials:

| Texture Type | glTF Source | Shader Register | Channel Usage |
|---|---|---|---|
| Base Color (Albedo) | `pbrMetallicRoughness.baseColorTexture` | t0 | RGB = color, A = alpha/opacity |
| Normal | `normalTexture` | t1 | RGB = tangent-space normal |
| Metallic-Roughness | `pbrMetallicRoughness.metallicRoughnessTexture` | t2 | G = roughness, B = metallic |
| Occlusion | `occlusionTexture` | t3 | R = ambient occlusion |
| Emissive | `emissiveTexture` | t4 | RGB = emissive color |

**Alpha / opacity mask** is handled through the base color alpha channel and the glTF `alphaMode` field — no separate texture slot required.

---

## 2. Phase Overview

| Phase | Description | Depends On |
|---|---|---|
| 0 | Third-party: integrate stb_image, stb_image_resize2, AMD Compressonator | — |
| 1 | Data model: expand MaterialDesc and MaterialData | — |
| 2 | glTF loader: extract all maps and alpha mode | Phase 1 |
| 3 | TextureManager: path cache + stb_image + stb_image_resize2 mip generation | Phase 0 |
| 4 | D3D12Texture: support full mip chains | Phase 3 |
| 5 | Root signature: expand SRV table to 5 slots | — |
| 6 | Constant buffer: add texture flags and alpha params | — |
| 7 | Renderer: wire MaterialData with GPU handles | Phase 1, 3 |
| 8 | ForwardOpaquePass: per-draw SRV binding | Phase 5, 6, 7 |
| 9 | Shaders: full PBR sampling with alpha mask support | Phase 5, 6, 8 |
| 10 | Default textures: create fallback set | Phase 3 |
| 11 | BC compression: AMD Compressonator + KTX2 output | Phase 3 |
| 12 | Cleanup: remove WIC image support | Phase 3, 11 |
| 13 | Unit tests for current engine capabilities | Phase 12 |

---

## 3. Phase Details

### Phase 0 — Integrate Third-Party Texture Libraries

**Goal:** Replace WIC-based texture loading with cross-platform libraries for loading, mip generation, and compression.
WIC support will be removed after validation in Phase 12.

**All third-party dependencies use CMake FetchContent** — sources are downloaded automatically at configure time into `build/_deps/`. No manual cloning or vendoring needed. Configuration is in `Scripts/FetchDependencies.cmake`.

**Libraries (~64 MB total, fetched automatically):**

| Library | Role | License | FetchContent Target | Pinned Version |
|---|---|---|---|---|
| **Dear ImGui** | Immediate-mode GUI (DX12 + Win32 backends) | MIT | `imgui` (STATIC) | `v1.92.5` |
| **cgltf** | Single-header glTF 2.0 parser | MIT | `cgltf` (INTERFACE) | `v1.15` |
| **stb** | Load PNG/JPEG/BMP/TGA/HDR + CPU mip generation | Public domain | `stb` (INTERFACE) | `master` |
| **AMD Compressonator** | BC1–BC7 block compression (CMP_Core only) | MIT | `CMP_Core` (STATIC, sparse checkout) | `master` |
| **KTX-Software** | KTX2 container I/O (pre-compressed asset pipeline) | Apache 2.0 | `ktx` (STATIC) | `v4.3.2` |

**How it works:**
1. User runs `cmake -B build` — FetchContent clones repos automatically
2. Sources go into `build/_deps/` (gitignored, cached between configures)
3. Only the minimal subsets are built (e.g., CMP_Core only, not full Compressonator GUI)
4. Compressonator uses git sparse checkout (`cmp_core/` + `cmp_math/` only, ~5 MB vs ~450 MB full repo)

**Note:** `Engine/third_party/` only contains `d3dx12.h` (Microsoft D3D12 helper header).

**CMake usage in any module:**
```cmake
target_link_libraries(YourTarget PRIVATE stb CMP_Core ktx imgui cgltf)
```

**What each library provides:**
- `stbi_load` / `stbi_load_16` / `stbi_loadf` — load PNG, JPEG, BMP, TGA, HDR to raw pixels
- `stbir_resize` — generate each mip level with selectable filter (Kaiser recommended)
- `CompressBlockBC7()` / `DecompressBlockBC7()` — BC1–BC7 block compression via CMP_Core
- `ktxTexture2_Create` / `ktxTexture2_WriteToFile` — write KTX2 containers (future)

---

### Phase 1 — Data Model Expansion

**Goal:** All texture paths, scalar fallbacks, and alpha mode flow through the pipeline.

#### MaterialDesc (GameFramework — CPU side)

Add or confirm these fields:

```cpp
struct MaterialDesc
{
    // Identity
    std::string name;

    // PBR scalars
    XMFLOAT4 baseColor = {1, 1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float f0 = 0.04f;
    XMFLOAT3 emissiveColor = {0, 0, 0};     // NEW

    // Alpha
    AlphaMode alphaMode = AlphaMode::Opaque; // NEW — Opaque, Mask, Blend
    float alphaCutoff = 0.5f;                // NEW — for Mask mode

    // Texture paths (relative to asset root)
    optional<path> albedoTexture;
    optional<path> normalTexture;
    optional<path> metallicRoughnessTexture;
    optional<path> occlusionTexture;         // NEW
    optional<path> emissiveTexture;          // NEW
};
```

**Integration note:** during rollout, texture sampling will progressively replace the uniform scalar values (baseColor/metallic/roughness/emissiveColor) whenever a texture is present, while keeping the scalar values as fallbacks.

**New enum:**

```cpp
enum class AlphaMode : uint8_t
{
    Opaque,
    Mask,
    Blend
};
```

#### MaterialData (Renderer — GPU side)

```cpp
struct MaterialData
{
    // Scalars
    XMFLOAT4 baseColor;
    float metallic, roughness, f0;
    XMFLOAT3 emissiveColor;                    // NEW
    AlphaMode alphaMode;                        // NEW
    float alphaCutoff;                          // NEW

    // GPU texture handles
    D3D12_GPU_DESCRIPTOR_HANDLE albedoSRV = {};
    D3D12_GPU_DESCRIPTOR_HANDLE normalSRV = {};           // NEW
    D3D12_GPU_DESCRIPTOR_HANDLE metallicRoughnessSRV = {}; // NEW
    D3D12_GPU_DESCRIPTOR_HANDLE occlusionSRV = {};         // NEW
    D3D12_GPU_DESCRIPTOR_HANDLE emissiveSRV = {};          // NEW

    // Flags
    bool hasAlbedo = false;
    bool hasNormal = false;             // NEW
    bool hasMR = false;                 // NEW
    bool hasOcclusion = false;          // NEW
    bool hasEmissive = false;           // NEW

    static MaterialData FromDesc(const MaterialDesc& desc, TextureManager& textures);
    PerObjectPSConstantBufferData ToPerObjectPSData() const;
};
```

---

### Phase 2 — glTF Loader: Extract All Maps

**Goal:** Populate every MaterialDesc field from glTF data.

**File:** `Engine/GameFramework/Private/Assets/GltfLoader.cpp`

Extract from each `cgltf_material`:

| glTF Field | Target |
|---|---|
| `pbr_metallic_roughness.base_color_texture` | `albedoTexture` |
| `pbr_metallic_roughness.base_color_factor` | `baseColor` |
| `pbr_metallic_roughness.metallic_roughness_texture` | `metallicRoughnessTexture` |
| `pbr_metallic_roughness.metallic_factor` | `metallic` |
| `pbr_metallic_roughness.roughness_factor` | `roughness` |
| `normal_texture` | `normalTexture` |
| `occlusion_texture` | `occlusionTexture` |
| `emissive_texture` | `emissiveTexture` |
| `emissive_factor` | `emissiveColor` |
| `alpha_mode` | `alphaMode` |
| `alpha_cutoff` | `alphaCutoff` |

**glTF metallic-roughness packing convention:**
- Green channel = roughness
- Blue channel = metallic
- The shader must unpack accordingly

---

### Phase 3 — TextureManager: Path Cache + stb_image + Mip Generation

**Goal:** Load any texture by path, generate mips, cache by path, provide fallback defaults.

**Files:**
- `Engine/Renderer/Public/TextureManager.h`
- `Engine/Renderer/Private/TextureManager.cpp`

**New API:**

```cpp
class TextureManager
{
    // Existing
    D3D12Texture* GetTexture(TextureId id) const;
    D3D12Texture* GetDefaultTexture() const;

    // New
    D3D12Texture* LoadFromPath(const std::filesystem::path& absolutePath);
    D3D12Texture* GetFallback(TextureSlot slot) const; // per-slot fallback

    // Fallbacks
    D3D12Texture* GetDefaultAlbedo() const;       // checker or grey
    D3D12Texture* GetDefaultNormal() const;       // flat normal (0.5, 0.5, 1.0)
    D3D12Texture* GetDefaultMR() const;           // metallic=0, roughness=1
    D3D12Texture* GetDefaultOcclusion() const;    // white (1.0)
    D3D12Texture* GetDefaultEmissive() const;     // black (0.0)

private:
    std::unordered_map<std::string, std::unique_ptr<D3D12Texture>> m_cache;
};
```

**Load pipeline per texture:**
1. Check cache by canonical path string → return if cached
2. Load with `stbi_load(path, &w, &h, &channels, 4)` → RGBA8 pixels
3. Generate mip chain using `stbir_resize()` with `STBIR_FILTER_KAISER`
   - Compute `mipCount = floor(log2(max(w, h))) + 1`
   - For sRGB textures (albedo, emissive): use `STBIR_COLORSPACE_SRGB` flag
   - For linear textures (normal, MR, occlusion): use `STBIR_COLORSPACE_LINEAR`
   - Store each mip level as a separate allocation in a `vector<vector<uint8_t>>`
4. (Phase 11) Optionally compress via AMD Compressonator (`CMP_ConvertTexture`)
5. Create D3D12 resource with `MipLevels = mipCount`
6. Upload all mip subresources via staging buffer
7. Create SRV with `MipLevels = mipCount`
8. Store in cache, return pointer

**stb_image notes:**
- `stbi_load` returns `unsigned char*` — caller must `stbi_image_free()`
- Force 4 channels (RGBA) for consistent GPU format
- For HDR: use `stbi_loadf` → `DXGI_FORMAT_R32G32B32A32_FLOAT`
- Thread-safe: stateless, no global state

---

### Phase 4 — D3D12Texture: Full Mip Chain Support

**Goal:** Support multi-mip textures instead of hardcoded `MipLevels = 1`.

**Files:**
- `Engine/RHI/Public/D3D12/Resources/D3D12Texture.h`
- `Engine/RHI/Private/D3D12/Resources/D3D12Texture.cpp`

**Changes:**
- Accept raw subresource data (array of mip levels) + mip count instead of a file path
- Create resource with `MipLevels = N`
- Upload N subresources via `UpdateSubresources`
- SRV desc uses `MipLevels = N`

**Constructor options:**
- Keep existing path-based constructor for legacy default textures
- Add new constructor: `D3D12Texture(D3D12Rhi&, const TextureData& data, D3D12DescriptorHeapManager&)`

**RHI-agnostic TextureData struct (shared between backends):**

```cpp
struct TextureData
{
    uint32_t width;
    uint32_t height;
    uint32_t mipCount;
    uint32_t channels = 4;    // always RGBA
    bool isSRGB = false;      // affects GPU format selection
    std::vector<std::vector<uint8_t>> mipLevels; // [0]=full res, [1]=half, ...
};
```

This struct is RHI-agnostic — the same `TextureData` can be consumed by both the D3D12 and future Vulkan backend.

---

### Phase 5 — Root Signature: 5 SRV Slots

**Goal:** Expand the SRV descriptor table from 1 slot to 5 slots.

**Files:**
- `Engine/RHI/Private/D3D12/Pipeline/D3D12RootSignature.cpp`
- `Engine/RHI/Public/D3D12/Pipeline/D3D12RootBindings.h`

**Current layout:**
```
RootParam 4: Descriptor Table — 1 SRV at t0 (PIXEL)
```

**New layout:**
```
RootParam 4: Descriptor Table — 5 SRVs at t0..t4 (PIXEL)
  t0 = BaseColor
  t1 = Normal
  t2 = MetallicRoughness
  t3 = Occlusion
  t4 = Emissive
```

**Decision — binding approach:**

| Approach | Description | Chosen? |
|---|---|---|
| Single 5-SRV table per draw | Allocate 5 contiguous descriptors per material, bind table once | **Yes** |
| 5 separate root descriptor tables | Bind each SRV individually | No — more root param changes |

This requires allocating 5 contiguous SRV descriptors per material in the shader-visible heap. The `D3D12DescriptorHeapManager::AllocateContiguous(5)` method already exists.

---

### Phase 6 — Constant Buffer: Texture Flags + Alpha

**Goal:** Pass texture presence flags and alpha params to the shader.

**Files:**
- `Engine/RHI/Public/D3D12/Resources/D3D12ConstantBufferData.h` (C++)
- `Engine/Assets/Shaders/Resources/ConstantBuffers.hlsli` (HLSL)

**PerObjectPS layout (both C++ and HLSL must match):**

```cpp
struct PerObjectPSConstantBufferData
{
    XMFLOAT4 BaseColor;
    float Metallic;
    float Roughness;
    float F0;
    uint32_t TextureFlags;   // NEW — bitmask: bit 0=albedo, 1=normal, 2=MR, 3=occlusion, 4=emissive
    XMFLOAT3 EmissiveColor;  // NEW
    float AlphaCutoff;       // NEW
    uint32_t AlphaMode;      // NEW — 0=Opaque, 1=Mask, 2=Blend
    float _Pad[3];           // Align to 16-byte boundary
};
```

**TextureFlags bitmask:**
```
bit 0 (0x01) = HasAlbedo
bit 1 (0x02) = HasNormal
bit 2 (0x04) = HasMetallicRoughness
bit 3 (0x08) = HasOcclusion
bit 4 (0x10) = HasEmissive
```

Using a bitmask keeps CB size small and avoids 5 separate uint fields.

---

### Phase 7 — Renderer: Wire MaterialData with GPU Handles

**Goal:** `BuildMaterials` resolves all textures into GPU SRV handles.

**File:** `Engine/Renderer/Private/Renderer.cpp`

**Changes to `BuildMaterials`:**
- Pass `*m_textureManager` to `MaterialData::FromDesc`
- `FromDesc` calls `TextureManager::LoadFromPath()` for each texture path
- Stores GPU handle if loaded, otherwise uses default fallback
- Sets flags accordingly

**Descriptor table allocation per material:**
- Allocate 5 contiguous SRV descriptors per material
- Copy the 5 GPU handles (or defaults) into contiguous slots
- Store the table base handle in `MaterialData` for per-draw binding

---

### Phase 8 — ForwardOpaquePass: Per-Draw SRV Binding

**Goal:** Bind the correct texture table per draw call.

**File:** `Engine/Renderer/Private/Passes/ForwardOpaquePass.cpp`

**Changes:**

Remove global texture binding from `BindGlobalResources()`.

In `DrawOpaqueMeshes()`, per draw:
```
for each draw:
    bind per-object VS CB (transforms)
    bind per-object PS CB (material scalars + flags + alpha)
    bind material texture table at RootParam::TextureSRV  ← per draw
    bind vertex/index buffers
    DrawIndexedInstanced
```

**Fallback:** if material has no textures at all, bind the default table (5 default textures).

---

### Phase 9 — Shaders: Full PBR Sampling

**Goal:** Sample all texture types with correct channel unpacking and fallback.

**Files:**
- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/Passes/Forward/ForwardLitPS.hlsl`
- `Engine/Assets/Shaders/Resources/ConstantBuffers.hlsli`

#### Texture declarations:

```hlsl
Texture2D TexBaseColor        : register(t0);
Texture2D TexNormal           : register(t1);
Texture2D TexMetallicRoughness: register(t2);
Texture2D TexOcclusion        : register(t3);
Texture2D TexEmissive         : register(t4);
```

#### Sampling logic:

```hlsl
// Base color
float4 albedo = (TextureFlags & 0x01)
    ? TexBaseColor.Sample(SamplerAniso, uv)
    : BaseColor;

// Normal (tangent space)
float3 normal = (TextureFlags & 0x02)
    ? UnpackNormalMap(TexNormal.Sample(SamplerAniso, uv).rgb)
    : input.WorldNormal;

// Metallic-Roughness (glTF packing: G=roughness, B=metallic)
float metallic, roughness;
if (TextureFlags & 0x04)
{
    float4 mr = TexMetallicRoughness.Sample(SamplerAniso, uv);
    roughness = mr.g;
    metallic  = mr.b;
}
else
{
    metallic  = Metallic;
    roughness = Roughness;
}

// Occlusion (R channel)
float occlusion = (TextureFlags & 0x08)
    ? TexOcclusion.Sample(SamplerAniso, uv).r
    : 1.0;

// Emissive
float3 emissive = (TextureFlags & 0x10)
    ? TexEmissive.Sample(SamplerAniso, uv).rgb * EmissiveColor
    : EmissiveColor;

// Alpha mask
if (AlphaMode == 1) // MASK
    clip(albedo.a - AlphaCutoff);
```

#### Normal map unpacking:

```hlsl
float3 UnpackNormalMap(float3 sample)
{
    float3 n = sample * 2.0 - 1.0;
    return normalize(n);
}
```

Tangent-space to world-space transform uses the interpolated TBN from the vertex shader.

---

### Phase 10 — Default Textures

**Goal:** Create fallback textures so binding is always valid.

| Slot | Default Content | Size | Format |
|---|---|---|---|
| t0 Albedo | 1x1 white (1,1,1,1) | 1x1 | R8G8B8A8 |
| t1 Normal | 1x1 flat (0.5, 0.5, 1.0) | 1x1 | R8G8B8A8 |
| t2 MR | 1x1 (0, 1, 0, 0) — roughness=1, metallic=0 | 1x1 | R8G8B8A8 |
| t3 Occlusion | 1x1 white (1,1,1,1) | 1x1 | R8G8B8A8 |
| t4 Emissive | 1x1 black (0,0,0,0) | 1x1 | R8G8B8A8 |

These ensure shaders always have a valid SRV to sample even when a material lacks a specific map. The texture flags in the CB control whether the shader uses the sample or the scalar fallback.

---

### Phase 11 — BC Compression via AMD Compressonator (Last Step)

**Goal:** Optionally compress textures for reduced VRAM usage and bandwidth.

**When:** After all phases are correct and rendering is validated.

**Format selection:**

| Texture Type | BC Format | Quality | Rationale |
|---|---|---|---|
| Albedo (sRGB) | BC7 | High | Best quality for color with alpha |
| Normal | BC5 | High | Two-channel (RG), reconstruct Z in shader |
| Metallic-Roughness | BC5 or BC7 | Medium | Two channels needed (G, B) |
| Occlusion | BC4 | High | Single channel (R) |
| Emissive | BC7 | High | Full color |

**Implementation with AMD Compressonator:**

```cpp
// After mip generation, compress each mip level:
CMP_Texture srcTexture = {};
srcTexture.dwSize     = sizeof(CMP_Texture);
srcTexture.dwWidth    = mipWidth;
srcTexture.dwHeight   = mipHeight;
srcTexture.dwPitch    = mipWidth * 4;
srcTexture.format     = CMP_FORMAT_RGBA_8888;
srcTexture.dwDataSize = mipWidth * mipHeight * 4;
srcTexture.pData      = mipPixels;

CMP_Texture dstTexture = {};
dstTexture.dwSize     = sizeof(CMP_Texture);
dstTexture.dwWidth    = mipWidth;
dstTexture.dwHeight   = mipHeight;
dstTexture.format     = CMP_FORMAT_BC7;  // or BC5, BC4
dstTexture.dwDataSize = CMP_CalculateBufferSize(&dstTexture);
dstTexture.pData      = new CMP_BYTE[dstTexture.dwDataSize];

CMP_CompressOptions options = {};
options.dwSize        = sizeof(CMP_CompressOptions);
options.fquality      = 0.05f;  // 0.0 = fast, 1.0 = best

CMP_ConvertTexture(&srcTexture, &dstTexture, &options, nullptr);
```

- Create GPU resource with compressed format (e.g., `DXGI_FORMAT_BC7_UNORM_SRGB` / `VK_FORMAT_BC7_UNORM_BLOCK`)
- SRV format must match
- Works identically on D3D12 and future Vulkan backend

**KTX2 output (optional, for pre-compressed asset pipeline):**
- After compression, write to KTX2 via KTX-Software
- KTX2 stores all mip levels + BC format metadata
- Both D3D12 and Vulkan can load KTX2 → skip runtime compression

**BC5 normal map shader change:**
```hlsl
float3 UnpackBC5Normal(float2 rg)
{
    float2 xy = rg * 2.0 - 1.0;
    float z = sqrt(saturate(1.0 - dot(xy, xy)));
    return float3(xy, z);
}
```

**Optional:** make compression a build-time step (offline) or a load-time toggle.

---

### Phase 12 — Remove WIC Support (Cleanup)

**Goal:** Remove WIC-based image loading and any legacy WIC helpers once stb_image-based loading is fully validated.

**Tasks:**
- Remove WIC loaders from the texture path (any `LoadFromWICFile` / WIC wrappers)
- Remove WIC-related includes, libs, and build flags
- Ensure texture loading and mip generation use only stb_image + stb_image_resize2
- Update any docs or comments referencing WIC

**Exit criteria:**
- All textures load correctly without WIC
- No WIC dependencies remain in the build or codebase

---

### Phase 13 — Unit Tests for Current Engine Capabilities

**Goal:** Add unit tests covering the current texture and material pipeline behavior.

**Targets:**
- TextureManager cache behavior (load-once, reuse by path)
- Mip generation count correctness
- Default texture fallbacks when texture paths are missing
- MaterialData flag setup from MaterialDesc

**Exit criteria:**
- Tests pass in CI and locally
- Core texture/material assumptions are locked down

---

## 4. Implementation Order (Recommended)

| Step | Phase | Description | Estimated Complexity |
|---|---|---|---|
| 1 | 0 | Integrate stb_image + stb_image_resize2 into third_party | Low |
| 2 | 0 | Integrate AMD Compressonator (link as static lib or CMake subproject) | Medium |
| 3 | 1 | Expand MaterialDesc + AlphaMode enum | Low |
| 4 | 2 | Expand glTF material extraction | Low |
| 5 | 4 | D3D12Texture: mip chain support via TextureData struct | Medium |
| 6 | 3 | TextureManager: path cache + stb_image load + stb_image_resize2 mips | Medium |
| 7 | 10 | Create default fallback textures | Low |
| 8 | 5 | Root signature: expand SRV to 5 slots | Low |
| 9 | 6 | PerObjectPS CB: add TextureFlags, EmissiveColor, AlphaCutoff, AlphaMode | Low |
| 10 | 1 | Expand MaterialData with 5 SRV handles + flags | Low |
| 11 | 7 | Renderer BuildMaterials: resolve textures to GPU handles | Medium |
| 12 | 8 | ForwardOpaquePass: per-draw texture table binding | Medium |
| 13 | 9 | Shaders: full PBR sampling + alpha mask | Medium |
| 14 | 11 | BC compression via AMD Compressonator (optional, last) | Low |
| 15 | 11 | KTX2 output via KTX-Software (optional, asset pipeline) | Low |
| 16 | 12 | Remove WIC loaders and build deps | Low |
| 17 | 13 | Add unit tests for current engine capabilities | Medium |

---

## 5. Validation Criteria

| Check | How to Verify |
|---|---|
| Albedo textures visible per material | Sponza walls, floor, columns show unique textures |
| Normal maps affecting shading | Edge highlights shift with viewing angle |
| Metallic-roughness working | Metal vs stone look distinct under lighting |
| Occlusion darkening crevices | Corners and recesses appear darker |
| Emissive elements glowing | Any emissive material adds color beyond lighting |
| Alpha mask working | Leaves, fences, flags clip correctly |
| Mips working | No aliasing at distance, textures remain sharp up close |
| No duplicate GPU uploads | Same texture path loaded once across materials |
| Default fallbacks correct | Materials without textures render with scalars, not black |

---

## 6. Files Changed (Summary)

| Layer | Files |
|---|---|
| Build | `Scripts/FetchDependencies.cmake` (all third-party deps via FetchContent) |
| Third-party | `build/_deps/` (auto-downloaded: stb, compressonator, ktx, imgui, cgltf) |
| Shared | `TextureData.h` (RHI-agnostic texture data struct) |
| GameFramework | `MaterialDesc.h`, `GltfLoader.cpp` |
| RHI | `D3D12Texture.h/cpp`, `D3D12RootSignature.cpp`, `D3D12RootBindings.h`, `D3D12ConstantBufferData.h` |
| Renderer | `TextureManager.h/cpp`, `MaterialData.h/cpp`, `Renderer.cpp`, `ForwardOpaquePass.cpp` |
| Shaders | `ConstantBuffers.hlsli`, `Material.hlsli`, `ForwardLitPS.hlsl` |
