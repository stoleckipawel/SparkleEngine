## Plan: Texture Mip Generation

Add offline mip generation to TextureCooker as a policy-driven post-load stage, not as loader behavior and not as runtime behavior. Keep SourceLoading responsible only for decoding source files, keep TextureAssetCooker responsible only for format validation plus cooked asset emission, and add a dedicated private `MipGeneration` module that owns mip planning, filtering, and special-case rules. Follow production-engine practice by resolving defaults from texture semantics upstream and carrying an explicit effective texture-processing profile into the cook request so the cooker stays execution-only. That resolved profile should eventually cover mip existence policy, mip kernel family, gamma or linear processing policy, and compression family preference. After the first material-texture mip slice is stable, extend the cooker to support full cubemap processing as a distinct phase before any compression-targeting work.

The first implementation target should be material textures, because they are the primary mip receivers in runtime rendering. That means phase 1 should focus on textures that currently arrive through the standard raster path, especially albedo, normal, metallic-roughness, occlusion, and emissive textures. HDR/EXR extension, DDS nuance, full cubemap support, and final compression targeting should come later. For now, avoid hardcoding broad texture-category defaults for mip enablement. Instead, assume mip generation is on by default and carry an explicit per-texture policy so later phases can tune behavior precisely. Also keep the first implementation honest about filter families: ordinary data textures need a regular downsample path, normal maps need vector-aware renormalizing behavior, and diffuse or albedo textures should support a wider sharpen-capable kernel such as Kaiser.

**Steps**
1. Phase 1: Lock the effective texture-processing policy model and the first implementation scope. Introduce an explicit resolved policy for cooked requests rather than teaching TextureCooker about material semantics directly. The policy model should have separate axes for mip existence policy, mip kernel family, gamma or linear processing policy, and later compression family preference. Recommended mip-existence surface remains `Generate`, `PreserveExisting`, and `NoMips`. `Generate` means the cooker should ensure a generated full mip chain for eligible inputs. `PreserveExisting` means authored mip chains are kept as-is and un-mipped sources remain base-only. `NoMips` strips output to the base level. The initial receiver set should explicitly prioritize material textures coming from the standard raster loader path, while the default effective behavior for now should be `Generate` unless explicitly overridden.
2. Phase 2: Extend the public request contract and request-file versioning. Update the TextureCookRequest surface and request parser or serializer to carry the resolved mip policy, mip kernel family, gamma or linear processing policy, and a placeholder compression family preference field, then bump the request-file header version and keep backward-compatible defaults for older request files where practical. This keeps request semantics stable before the cooker implementation changes.
3. Phase 3: Resolve initial defaults upstream in AssetConverter without hardcoding broad texture-category disable rules yet. In the first slice, default the effective request value to `Generate`, preserve an explicit override seam so individual textures can later opt into `PreserveExisting` or `NoMips`, choose default mip kernels from texture semantics, and add an Unreal-style default compression family preference that resolves from semantic rather than from ad hoc filename rules. The initial intent should be regular downsample for generic data textures, a Kaiser-style kernel for diffuse or albedo textures, and a dedicated normal-aware path for normal maps.
4. Phase 4: Add a dedicated `Private/MipGeneration/` module instead of growing `SourceLoading/` or `Cooking/`. Put policy types, mip-count planning, filter selection, gamma or linear color-domain handling, and image downsampling there. Reuse `stb_image_resize2`, which the repo already fetches, for the first-phase uncompressed 2D mip generation backend and keep the filtering abstraction narrow enough that a later backend change remains possible. Keep this module independent from request parsing and from final cooked-asset serialization.
5. Phase 5: Integrate mip generation first into the standard raster material path. After the raster loader path returns a TextureLoadResult and before TextureAssetCooker serializes it, run a small orchestration step that either preserves authored mips, generates the full chain for eligible uncompressed textures, or strips to the base level according to the effective policy. This keeps the first implementation narrowly focused on the highest-value receiver path instead of trying to solve every loader at once, while still exposing `Generate`, `PreserveExisting`, and `NoMips` from day one.
6. Phase 6: Add explicit kernel-family handling for uncompressed textures. The mip-generation module should support at least a regular downsample path for generic linear data, a Kaiser path for diffuse or albedo textures where a wider sharpen-capable filter is desirable, and a dedicated normal-map path for vector data. Keep kernel choice driven by the resolved semantic or explicit override rather than hidden inside individual loaders.
7. Phase 7: Add normal-map-aware mip generation behavior. Normal textures must preserve normalized vectors after downsampling, so the mip-generation module should include a dedicated normal-map path that reconstructs or interprets the vector correctly, averages in vector space, and renormalizes the result per texel before storing the mip payload. Keep this rule driven by the resolved texture semantic, not by filename guessing.
8. Phase 8: Add explicit gamma or linear processing rules. Diffuse or other color textures authored in sRGB should be downsampled in linear light and encoded back to their target representation after filtering. Linear data textures such as masks, metallic-roughness, occlusion, and height-like maps should remain in linear space with no gamma transform applied. The normal-aware path should stay vector-space specific and never reuse the color-texture gamma workflow.
9. Phase 9: Extend the mip-generation stage beyond standard raster material textures where justified. At this point, decide whether HDR or EXR material-like textures should gain generated mips, while DDS remains a preserve-authored path unless an explicit strip policy is selected.
10. Phase 10: Add full cubemap support before any compression phase. TextureCooker should be able to ingest true cubemap content, not only lat-long textures. This phase should add the request/schema/runtime contract needed to represent cube textures explicitly, support per-face or cubemap-source ingestion, generate mip chains across all cube faces consistently, and keep cubemap processing as a first-class capability rather than a sky-only workaround. Cubemap work should also decide whether cube textures need their own angular or seam-aware filtering path rather than inheriting the ordinary 2D filters blindly.
11. Phase 11: Update artifact identity and validation. Include mip policy, mip kernel family, gamma or linear processing policy, cubemap-related cook settings, compression family preference, and any normal-map generation mode in the cook identity/settings hash and bump the TextureCooker version so cache metadata invalidates correctly when behavior changes. Add focused validation coverage for base-only, generated-chain, preserved-authored-chain, stripped-authored-chain, Kaiser-filtered diffuse cases, renormalized normal-map cases, gamma-correct color mip cases, and cubemap mip-chain cases.
12. Phase 12: Add manual and executable verification. Validate generated `.stex` outputs for representative material textures: one PNG or JPG albedo texture should gain a full mip chain under `Generate` with the expected wider filter behavior, one normal map should gain a full mip chain with normalized vectors preserved, one linear data texture should prove that no gamma conversion is applied during filtering, one DDS with authored mips should preserve its chain under `PreserveExisting`, one explicitly overridden texture should stay single-mip under `NoMips`, and one cubemap source should cook into a valid full cubemap texture with a complete mip chain.
13. Phase 13: Add cooked format and compression targeting as the last phase. Only after mip generation behavior and full cubemap support are stable should the pipeline choose output formats such as BC1, BC5, BC7, R8, and related targets. This final phase should own an Unreal-style compression family preference model that resolves upstream from texture semantics and then maps to actual platform or runtime output formats, along with block-compression preparation, cubemap compression rules, SNORM versus UNORM decisions where relevant, and any BC-specific mip rules, rather than mixing that work into the earlier mip-generation or cubemap-support slices.

**Relevant files**
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Public\TextureCookRequestList.h` — extend the public cook request contract with the effective mip policy field while keeping the tool-facing surface minimal.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\Requests\TextureCookRequestList.cpp` — bump request-file header version and parse/serialize the new mip policy token.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\AssetConverter\Private\Cooking\TextureCookRequestBuilder.cpp` — resolve the initial default effective mip policy, preserve explicit per-texture override support, and build stable cook identity keys, including the normal-map path.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\GameFramework\Public\Assets\Cooked\CookedTextureReference.h` — reference point for the material texture semantic categories that should drive defaults.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\SourceLoading\RasterTextureSourceLoader.cpp` — primary first-phase decode entrypoint for material textures.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\SourceLoading\TextureSourceLoaderUtils.cpp` — current base-surface builders for raster/HDR/EXR paths; keep these decode-only and do not move mip generation into them.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\SourceLoading\DdsTextureSourceLoader.cpp` — preserve current authored DDS mip-chain ingestion and use it as the preserve-authored path.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\SourceLoading\` — later cubemap-ingestion extension point for true cube sources and face-based loading paths.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\MipGeneration\` — new dedicated folder for mip policy types, mip planners, standard raster mip generation, and normal-map renormalization logic.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\Cooking\TextureAssetCooker.h` — keep this limited to format validation and cooked asset emission; use it as the seam the new mip stage feeds.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\Cooking\TextureAssetCooker.cpp` — current post-load serialization path; add only the orchestration hook needed to consume the mip-generation result, not the resize or renormalization algorithm itself.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\Constants\TextureCookerConstants.h` — bump cooker version for cache invalidation.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\Private\Cooking\TextureCookArtifactKeyBuilder.cpp` — incorporate mip policy and later compression policy into the cook settings hash.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\CMake\Dependencies\FetchDependencies.cmake` — confirms `stb_image_resize2` is available for first-phase mip generation, `CMP_Core` is available for the later BCn compression phase, and `KTX-Software` is available for later KTX2 container or interchange work if the project chooses to use it.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\D3D12\Textures\TextureLoadResult.h` — existing mip-vector contract the new mip-generation stage should populate and preserve.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\D3D12\Textures\CookedTextureAsset.h` — existing cooked schema already supports variable mip counts; use it as the serialization contract and avoid unnecessary schema churn.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\Textures\CookedTextureAssetLoader.cpp` — likely runtime seam for full cubemap cooked-asset loading once cube schema support is added.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\Resources\D3D12Texture.cpp` — likely runtime seam for explicit cube resource creation and SRV handling once full cubemap support is added.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\TextureCooker\CMakeLists.txt` — add the new `Private/MipGeneration/` files and keep folder ownership explicit.

**Verification**
1. Build the touched tool slice with the narrow TextureCooker MSBuild target after the request-contract and mip-generation changes.
2. Run `TextureCooker.exe cook-request-file` on a small hand-authored request set that covers `Generate`, `PreserveExisting`, and `NoMips`.
3. Inspect generated `.stex` metadata and confirm `mipCount` changes as expected for material textures using the raster path.
4. Re-run the same cook requests twice and verify artifact caching skips unchanged outputs after the version/hash update.
5. Validate that a diffuse or albedo source can use the intended wider filter path, such as Kaiser, while linear data textures still use the simpler regular downsample path unless explicitly overridden.
6. Validate that a normal map source gains generated mips while maintaining normalized vectors in every mip level.
7. Validate that sRGB color textures are downsampled in linear light and re-encoded correctly, while masks or packed data textures do not undergo unwanted gamma conversion.
8. Validate that a DDS source with authored mips preserves its chain under `PreserveExisting` and strips to one mip under `NoMips` if that behavior is included.
9. Validate that an explicitly overridden texture remains single-mip under `NoMips`.
10. Validate that a cubemap source cooks into a valid cube texture with a complete mip chain before any compression-targeting phase begins.
11. Defer BC1, BC5, BC7, R8, and other output-format targeting validation until the final compression phase.

**Decisions**
- Included scope: offline mip generation primarily for material textures that currently flow through the standard raster loader path.
- Included scope: a default-on mip strategy for now, with explicit `Generate`, `PreserveExisting`, and `NoMips` overrides flowing into TextureCooker so later texture-specific tuning stays easy.
- Included scope: explicit mip kernel-family policy, with at least a regular downsample path, a Kaiser-capable path for diffuse or albedo textures, and a dedicated normal-aware path.
- Included scope: explicit gamma or linear processing rules so color textures can filter in linear light while data textures remain untouched by gamma conversion.
- Included scope: a new dedicated private `MipGeneration` folder so decode, policy resolution, mip algorithms, and serialization stay separated.
- Included scope: normal-map-aware mip generation that renormalizes vectors after downsampling.
- Included scope before compression: full cubemap support as a first-class TextureCooker capability rather than limiting the plan to lat-long texture handling.
- Included scope before compression lands: an Unreal-style compression family preference that resolves upstream from texture semantics, even if the actual BCn target mapping still lands in the last compression phase.
- Deferred to the last phase: BC1, BC5, BC7, R8, and other cooked-format or compression targeting decisions.
- Excluded from the early phases: hardcoded broad no-mip category rules, alpha-coverage preservation, and broader platform-specific compression policy until later phases make those decisions concrete.
- Recommended ownership rule: TextureCooker must not infer texture meaning from filenames or material semantics; it should execute the explicit policy carried in the request.
- Recommended dependency fit: use `stb_image_resize2` as the first-phase uncompressed mip-generation backend, use `CMP_Core` when the BCn compression phase lands, and treat `KTX-Software` as a later KTX2 container or interchange dependency rather than as the primary first-phase mip filtering backend.

**Execution prompts**
Use the following prompts sequentially to drive implementation from the first policy change to the final validated compression phase. Each prompt is written to preserve the architectural decisions in this document, keep scope narrow, and require validation before moving on.

1. Prompt for phase 1: effective processing policy model.
```
Implement phase 1 of the TextureCooker mip-generation roadmap in SparkleEngine.

Goal:
- Lock the effective texture-processing policy model for TextureCooker without implementing broad compression yet.
- Introduce explicit policy axes for mip existence policy, mip kernel family, gamma/linear processing policy, and a placeholder compression family preference.

Requirements:
- Keep TextureCooker execution-only. Do not teach TextureCooker to infer semantics from filenames.
- Preserve the current ownership split: SourceLoading decodes source files, AssetConverter resolves policy defaults, TextureCooker executes a resolved request.
- Keep the initial mip existence surface as Generate, PreserveExisting, and NoMips.
- Do not add runtime loading of authoring formats.
- Do not add compression behavior in this phase beyond the policy model placeholder.

Deliverables:
- Introduce policy types in the correct tool-facing layer.
- Ensure the naming is explicit and production-friendly.
- Keep public surface minimal and avoid speculative extra flags beyond the agreed axes.
- Update comments or docstrings only where needed to clarify ownership.

Validation:
- Build the narrow TextureCooker slice or the narrowest affected target.
- Confirm no unrelated module boundaries were violated.
- Summarize the final policy model and the ownership split that remains in place.
```

2. Prompt for phase 2: request contract and request-file versioning.
```
Implement phase 2 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Extend the public TextureCookRequest contract and request-file serialization so the resolved processing profile can travel explicitly through the pipeline.

Requirements:
- Add fields for resolved mip policy.
- Leave a clean growth path for mip kernel family, gamma/linear processing policy, and compression family preference.
- Bump the request-file header version.
- Keep backward-compatible loading behavior where practical for older request files.
- Preserve stable parsing and serialization behavior.

Constraints:
- Do not implement the actual mip-generation algorithms in this phase.
- Do not push semantic inference into TextureCooker.
- Keep request parsing focused and deterministic.

Deliverables:
- Updated public request contract.
- Updated parser/serializer.
- Any necessary validation around malformed inputs and default migration behavior.

Validation:
- Build the affected TextureCooker shared/request slice.
- Write and load a small hand-authored request file that exercises the new schema.
- Confirm old request files still fail or load in a deliberate, understandable way rather than silently misbehaving.
```

3. Prompt for phase 3: upstream default resolution and Unreal-style compression family preference.
```
Implement phase 3 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Resolve the initial effective processing profile upstream in AssetConverter.
- Add an Unreal-style default compression family preference that resolves from texture semantic rather than from filenames or ad hoc rules.

Requirements:
- Default mip policy to Generate unless explicitly overridden.
- Preserve an override seam for PreserveExisting and NoMips.
- Resolve default kernel family by semantic.
- Resolve a logical compression family preference by semantic, but do not map to concrete BC formats yet.
- Keep TextureCooker execution-only.

Default intent:
- Generic data textures use a regular downsample path.
- Diffuse/albedo textures prefer a Kaiser-capable path.
- Normal maps use a dedicated normal-aware path.
- Compression preference is logical and semantic-driven, not a DXGI-format guess.

Validation:
- Build the affected AssetConverter and TextureCooker request flow.
- Inspect representative generated requests and confirm the resolved defaults are explicit in the request data.
- Summarize the default mapping by semantic.
```

4. Prompt for phase 4: dedicated MipGeneration module.
```
Implement phase 4 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Introduce a dedicated Private/MipGeneration module that owns mip planning, filter selection, gamma/linear handling, and image downsampling.

Requirements:
- Keep SourceLoading decode-only.
- Keep TextureAssetCooker focused on validation and cooked asset emission.
- Reuse stb_image_resize2 for the first-phase uncompressed 2D mip-generation backend.
- Do not force KTX-Software into the first raster mip path.
- Keep the filtering abstraction narrow enough that a future backend change remains possible.

Deliverables:
- New folder and source layout for MipGeneration.
- Initial policy and planner types in the new module.
- Clear call seam for later orchestration.

Validation:
- Build TextureCooker.
- Confirm the new files are wired in CMake cleanly.
- Summarize why stb_image_resize2 is used here and why KTX remains deferred to a later container-oriented phase.
```

5. Prompt for phase 5: first raster-path orchestration.
```
Implement phase 5 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Integrate mip-generation orchestration into the standard raster material path only.

Requirements:
- After raster decode and before cooked serialization, route TextureLoadResult through the mip stage.
- Respect Generate, PreserveExisting, and NoMips.
- Keep the first integration narrow: raster material textures first.
- Avoid broad loader rewrites.

Constraints:
- Do not widen scope to EXR/HDR/DDS algorithm changes yet.
- Do not add compression here.

Validation:
- Build TextureCooker.
- Run cook-request-file on a small request set covering Generate, PreserveExisting, and NoMips.
- Confirm mipCount changes exactly as expected for raster inputs.
```

6. Prompt for phase 6: explicit kernel-family support.
```
Implement phase 6 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Add explicit kernel-family handling for uncompressed textures.

Requirements:
- Support at least three early filter families:
	- Regular downsample for generic linear data and ordinary textures.
	- Kaiser for diffuse/albedo textures where a wider sharpen-capable filter is desired.
	- Dedicated normal-aware handling for normal maps.
- Resolve kernel choice from explicit semantic/profile policy, not hidden call-site behavior.

Constraints:
- Do not introduce a large menu of speculative kernels.
- Keep the first kernel set intentionally small and well-justified.

Validation:
- Build TextureCooker.
- Cook at least one diffuse/albedo texture and one linear data texture.
- Confirm the selected filter path is traceable and deterministic.
```

7. Prompt for phase 7: normal-aware mip generation.
```
Implement phase 7 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Add proper normal-map-aware mip generation.

Requirements:
- Reconstruct or interpret the normal correctly.
- Average in vector space.
- Renormalize per texel before storing the mip payload.
- Keep the rule driven by resolved semantic, not filenames.
- Do not reuse the color-texture gamma workflow for normals.

Validation:
- Build TextureCooker.
- Cook representative normal-map sources.
- Verify lower mips preserve normalized vectors and do not visibly collapse detail through unnormalized averaging.
```

8. Prompt for phase 8: gamma/linear processing policy.
```
Implement phase 8 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Add explicit gamma/linear processing rules to mip generation.

Requirements:
- sRGB-authored color textures must be filtered in linear light and then encoded back to the intended representation.
- Linear data textures such as masks, metallic-roughness, occlusion, and similar packed maps must remain in linear space with no unwanted gamma transform.
- Normal maps remain on their own vector-aware path.

Constraints:
- Do not hide color-domain conversion behind loader-specific assumptions.
- Keep the policy explicit in the resolved processing profile.

Validation:
- Build TextureCooker.
- Cook at least one sRGB color texture and one linear data texture.
- Confirm the color texture uses linear-light filtering and the data texture does not undergo color-style gamma conversion.
```

9. Prompt for phase 9: extension beyond standard raster.
```
Implement phase 9 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Extend mip-generation coverage beyond the standard raster material path where justified.

Requirements:
- Evaluate whether HDR/EXR material-like textures should gain generated mips.
- Keep DDS as a preserve-authored path unless an explicit strip policy is selected.
- Preserve the current ownership split and avoid one-off hacks per loader.

Deliverables:
- Minimal, justified extension of supported source categories.
- Explicit documentation or comments on what still remains deferred.

Validation:
- Build TextureCooker.
- Validate one HDR/EXR path if added and one DDS preserve-authored case.
```

10. Prompt for phase 10: full cubemap support.
```
Implement phase 10 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Add full cubemap support before compression.

Requirements:
- Support true cubemap content, not only lat-long textures.
- Add request/schema/runtime contract changes needed to represent cube textures explicitly.
- Support per-face or cubemap-source ingestion as designed.
- Generate mip chains consistently across all cube faces.
- Keep cubemap processing as a first-class capability, not a sky-only workaround.
- Decide whether cube textures need an angular or seam-aware filter path rather than blindly inheriting 2D filters.

Validation:
- Build the touched tool/runtime slice.
- Cook a representative cubemap source.
- Confirm the cooked result is recognized and loaded as a true cube texture with a complete mip chain.
```

11. Prompt for phase 11: artifact identity and validation infrastructure.
```
Implement phase 11 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Make cache identity and validation reflect the new processing behavior.

Requirements:
- Include mip policy, mip kernel family, gamma/linear policy, cubemap settings, compression family preference, and normal-map mode in artifact identity.
- Bump TextureCooker version appropriately.
- Add focused validation coverage for base-only, generated-chain, preserved-authored-chain, stripped-authored-chain, Kaiser-filtered diffuse, renormalized normal maps, gamma-correct color filtering, and cubemap cases.

Validation:
- Build the affected validation and tool slices.
- Re-run identical cook requests twice and confirm cache hits after the new hash/version behavior is in place.
```

12. Prompt for phase 12: manual and executable end-to-end verification.
```
Execute phase 12 verification for the TextureCooker roadmap in SparkleEngine.

Goal:
- Prove the implemented behavior works end-to-end before compression lands.

Required coverage:
- One PNG or JPG albedo texture gains a full mip chain under Generate and uses the intended wider filter behavior.
- One normal map gains a full mip chain with normalized vectors preserved.
- One linear data texture proves no unwanted gamma conversion is applied.
- One DDS with authored mips preserves its chain under PreserveExisting.
- One explicitly overridden texture stays single-mip under NoMips.
- One cubemap source cooks into a valid cube texture with a full mip chain.

Deliverables:
- Run the narrowest relevant build.
- Run the cooker on a representative request pack.
- Inspect resulting metadata and summarize outcomes by case.
- Call out any remaining gaps before compression work begins.
```

13. Prompt for phase 13: compression family preference and concrete format mapping.
```
Implement phase 13 of the TextureCooker roadmap in SparkleEngine.

Goal:
- Add the final compression phase using the previously resolved logical compression family preference.

Requirements:
- Follow Unreal-style intent: resolve logical compression family preference upstream from semantics and overrides first, then map that family to actual output formats in the final compression phase.
- Add concrete output-format mapping such as BC1, BC5, BC7, R8, and other justified targets.
- Include SNORM vs UNORM decisions where relevant.
- Use CMP_Core as the planned BC-family compression backend.
- Keep KTX-Software as a separate later seam for KTX2 container/interchange workflows rather than forcing it into the BC-compression path.
- Preserve cubemap support in the compression path.

Constraints:
- Do not collapse the logical family layer into hardcoded DXGI guesses scattered across the cooker.
- Keep compression backend decisions centralized and testable.

Validation:
- Build TextureCooker and the narrowest affected runtime/validation slices.
- Cook representative assets across the intended families.
- Confirm the resolved compression family preference maps to the expected concrete formats.
- Summarize the final semantic-to-family-to-format mapping.
```

14. Final end-to-end completion prompt.
```
Perform a final end-to-end review of the completed TextureCooker roadmap implementation in SparkleEngine.

Goal:
- Verify that the implemented system matches the agreed architecture and behavior across all phases.

Checklist:
- TextureCooker remains execution-only and does not infer semantics from filenames.
- SourceLoading remains decode-only.
- AssetConverter resolves explicit defaults and overrides, including compression family preference.
- The request contract carries the resolved processing profile cleanly.
- Mip policy, kernel family, gamma/linear handling, normal-map processing, cubemap support, cache identity, and compression-family mapping all work together coherently.
- stb_image_resize2 is used for the initial raster mip path, CMP_Core is used for BC compression, and KTX-Software remains a separate future container/interchange seam.

Deliverables:
- Run the narrowest relevant validation/build commands.
- Execute representative cook scenarios for all major cases.
- Report any remaining deferred items explicitly.
- Confirm whether the implementation is ready to be treated as the new production texture-cooking baseline.
```

