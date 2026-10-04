# Anisotropy Barn Lamp Acquisition

The model payload is external and intentionally absent from the repository. Launcher acquires the pinned Khronos glTF revision from the six loose files declared in [SourceFiles.txt](SourceFiles.txt). Every file's byte count and SHA-256 are checked before the complete directory replaces any previous local acquisition.

The loose glTF variant keeps base-color, normal, and metallic-roughness textures available to the source importer. The level is available as a generic metallic-roughness preview after sync and cook. The importer reports omitted optional anisotropy, clearcoat, transmission, and volume lobes; their authored parameters are not preserved in the cooked material or GBuffer. Opening this level does not validate those shading models.

Copyright 2023 Wayfair, LLC. Model and textures by Eric Chadwick, licensed under CC BY 4.0. The publisher license remains authoritative.
