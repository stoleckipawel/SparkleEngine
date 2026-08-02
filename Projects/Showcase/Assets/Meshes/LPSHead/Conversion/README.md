# LPS Head Conversion

The source is the `lpshead.zip` package from [Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html). The Infinite Realities 3D head scan by Lee Perry-Smith is licensed under CC BY 3.0; the archive conversion was prepared by Morgan McGuire and Guedis Cardenas. The publisher archive remains immutable and external under `Publisher/`.

`ConvertLPSHead.py` converts only `head.OBJ` geometry to one atomically published, tracked GLB 2.0 artifact. It validates the pinned source inventory, resolves OBJ indices, preserves position/UV seams deterministically, rejects zero-area faces, triangulates the remaining polygons in authored order, flips the OBJ V texture coordinate for glTF, generates area-weighted smooth normals, and maps the single `defaultMat` material to a dielectric metallic-roughness baseline. The GLB references the publisher's `Publisher/lambertian.jpg`. The source bump maps are retained in the external pack but are intentionally not treated as tangent-space normal maps.

Run from the repository root after syncing the LPS Head pack:

```powershell
python Projects/Showcase/Assets/Meshes/LPSHead/Conversion/ConvertLPSHead.py
```

Expected conversion inventory:

- 9,223 indexed vertices after preserving position/UV seams;
- 17,682 triangles after deterministic quad triangulation;
- one zero-area source quad rejected and reported by the converter;
- `LPSHead.glb` SHA-256: `ecbf4b39963b3f3a51a59997bc16feacf17d2c3ffb661fe73b2b3824ce4b8ece`.

The source archive is pinned in `Projects/Showcase/Levels.catalog` by byte count and SHA-256. Re-run the conversion only when that source identity or the conversion policy changes, then update the output hashes above and validate the cooked scene before publishing the result.
