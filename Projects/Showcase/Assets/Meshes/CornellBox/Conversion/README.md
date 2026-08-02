# Cornell Box Conversion

The source is `CornellBox.zip` from [Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html). The archive's Cornell Box set was created by Guedis Cardenas and Morgan McGuire and is distributed under CC BY 3.0. `CornellBox-Original.obj` and its MTL additionally identify that original variant as public domain; Sparkle retains the archive-level attribution.

`ConvertCornellBox.py` converts the original OBJ/MTL pair to one atomically published, tracked GLB 2.0 artifact. It validates the selected variant's source and material inventories, resolves OBJ indices, triangulates polygons in authored order, generates flat face normals, preserves diffuse colors and Phong roughness intent, and maps the ceiling panel emission through `KHR_materials_emissive_strength`. The other publisher variants stay in the external pack for future explicit levels; this level does not silently substitute them.

Run from the repository root after syncing the Cornell Box pack:

```powershell
python Projects/Showcase/Assets/Meshes/CornellBox/Conversion/ConvertCornellBox.py
```

Expected conversion inventory:

- 8 materials and 8 material primitives;
- 72 face vertices and 36 triangles;
- `CornellBox.glb` SHA-256: `a2af112abeaa1b08087009afa3efef2c6b665c59bd7a6efe70d703e8f2349d91`.

The source archive is pinned in `Projects/Showcase/Levels.catalog` by byte count and SHA-256. Re-run the conversion only when that source identity or conversion policy changes, then update the output hashes above and validate the cooked scene before publishing the result.
