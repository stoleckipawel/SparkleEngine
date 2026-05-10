# Sparkle Build Configurations

Sparkle supports exactly six build profiles. The canonical build-system definition lives in `CMake/SparkleBuildProfiles.cmake`.

| Profile | Target shape | Optimization | Diagnostics | Typical use |
| --- | --- | --- | --- | --- |
| `DebugEditor` | Editor | Off | Full debug diagnostics | Debugging editor-capable project launches |
| `DebugGame` | Editorless runtime | Off | Full debug diagnostics | Debugging standalone runtime behavior |
| `DevelopmentEditor` | Editor | On | Developer diagnostics and debug info | Normal editor development |
| `DevelopmentGame` | Editorless runtime | On | Developer diagnostics and debug info | Normal standalone runtime development and cooking |
| `ShippingEditor` | Editor | Highest shipping policy | Minimal diagnostics | Final editor-capable verification when needed |
| `ShippingGame` | Editorless runtime | Highest shipping policy | Minimal diagnostics | Final standalone runtime verification |

Profile naming follows the Unreal-style two-keyword model:

- `Debug`, `Development`, and `Shipping` describe the build state.
- `Editor` and `Game` describe the launch target shape.

`BuildProject.bat` derives the CMake target from the suffix: `*Editor` builds `<Project>Editor`, and `*Game` builds `<Project>Runtime`.