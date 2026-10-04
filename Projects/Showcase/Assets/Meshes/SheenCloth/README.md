# Sheen Cloth Acquisition

The model payload is external and intentionally absent from the repository. Launcher acquires the pinned Khronos glTF revision from the seven loose files declared in [SourceFiles.txt](SourceFiles.txt). Every file's byte count and SHA-256 are checked before the complete directory replaces any previous local acquisition.

The loose glTF variant keeps base material textures and their `KHR_texture_transform` mappings available to the source importer. The level is available as a generic metallic-roughness preview after sync and cook. The importer reports omitted optional sheen data; its authored parameters are not preserved in the cooked material or GBuffer. The cloth-sheen lighting lobe remains a separate gap, so opening this level does not validate cloth shading.

Donated by Microsoft for glTF testing and licensed under CC0 1.0. The publisher license remains authoritative.
