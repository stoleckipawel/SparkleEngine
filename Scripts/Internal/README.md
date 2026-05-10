# Internal Script Modules

Files under this folder are implementation details for the public commands in `Scripts/` and `Scripts/Cook/`.

## Boundaries

- `Core/` owns shared path/tool constants and logging bootstrap.
- `Build/` owns CMake configure/build orchestration and generated-build-file freshness checks.
- `Toolchain/` owns Visual Studio 2026, CMake, MSBuild, and optional host-tool validation.
- `Cook/` owns cook-tool preparation only. Cook planning and asset behavior belong in `AssetCooker` and the focused C++ cooker tools.
- `Projects/` owns lightweight project/target discovery needed by script orchestration.
- `Utilities/` owns small fallback helpers that are awkward in batch.

## Guidelines

- Keep user-facing `.bat` files thin: argument handling, status output, and delegation.
- Prefer CMake targets or C++ tools for real build, cook, validation, and asset logic.
- Keep PowerShell helpers narrow and data-oriented; avoid growing parallel product logic in scripts.
- Add new internal helpers to the nearest category folder instead of the `Internal/` root.
- Visual Studio 2026 is the required Windows generator path for this repository.
