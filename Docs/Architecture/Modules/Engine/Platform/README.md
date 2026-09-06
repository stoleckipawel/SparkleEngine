# Platform Capability Inventory

Status: capability snapshot; current, but not portability or runtime evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/Platform` public/private source and CMake membership inspected; evidence `S` only

Scope: native window ownership, DPI, message pumping, Win32 input acquisition, layered routing, capture, and cursor control

Owner: `Engine/Platform` / `SparklePlatform`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Platform Boundary

The present implementation is Windows-only: public `Window` exposes `HWND` and Win32 message types, the input backend is `Win32InputBackend`, the application manifest is Windows-specific, and `dwmapi` is linked privately. There is no Linux/macOS backend in this snapshot.

## Capability Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `PLAT-001` | Native Win32 window | Implemented path | Registers one Sparkle window class, creates one top-level popup/resizable application window, extends DWM frame into client area, exposes its `HWND`, and fatals on registration/creation failure. | `S` |
| `PLAT-002` | Message pump | Implemented path | Non-blocking `PollEvents` drains the thread queue and synchronously broadcasts raw window messages; `WaitForEvent` blocks while minimized/idle until a message arrives. | `S` |
| `PLAT-003` | Size/state | Implemented path | Atomically published client width/height, valid-size check, and Normal/Minimized/Maximized/FullScreen state. Minimum interactive size is 320x240. | `S` |
| `PLAT-004` | Window controls | Implemented path | Request close, minimize, maximize, restore, maximize/restore toggle, borderless monitor fullscreen toggle, and native drag move. Initial show state is maximized; startup fullscreen currently always returns false. | `S` |
| `PLAT-005` | DPI awareness/events | Implemented path | Manifest plus per-window DPI query, scale relative to 96 DPI, DPI-change event, suggested-rect handling, and DPI-aware resize-border metrics. Multi-monitor behavior remains unrun. | `S` |
| `PLAT-006` | Window event contracts | Implemented path | Synchronous resized, DPI-scale-changed, and capacity-16 raw message events. Subscribers run on the polling thread and can mark raw messages handled. | `S` |
| `PLAT-007` | Win32 keyboard/mouse backend | Implemented path | Translates Windows messages into typed keyboard, mouse-button, move, vertical/horizontal wheel events and modifier state. No gamepad, touch, pen, IME composition, or raw-input device catalog was found. | `S` |
| `PLAT-008` | Per-frame input state | Implemented path | Global and per-layer pressed/released/held state, pointer position/delta, wheel accumulation, modifiers, capture, and cursor-hidden state reset at `BeginFrame`. | `S` |
| `PLAT-009` | Immediate/deferred dispatch | Implemented path | Subscribers choose typed event, Gameplay/UI/System layer, and immediate/deferred delivery. Deferred events are processed explicitly after the native message pump. | `S` |
| `PLAT-010` | Layer routing | Implemented path | Gameplay/UI/System enablement, active fallback layer, per-frame UI target rectangles, text-input/interactions-disabled policy, point/mouse-move targeting, and layer cancellation. | `S` |
| `PLAT-011` | UI capture integration | Implemented path | Host supplies a capture query (currently ImGui keyboard/mouse capture); routing uses it with registered viewport bounds to prevent UI/gameplay double-consumption. | `S` |
| `PLAT-012` | Mouse/cursor control | Implemented path | Capture/release, hide/show/visibility, center cursor, and capture-layer tracking. Focus-loss and alt-tab recovery require runtime evidence. | `S` |
| `PLAT-013` | Owner-thread enforcement | Implemented path | Registration and dispatch are guarded by Core `OwnerThread`; the module is not generally thread-safe. | `S` |

## Vertical Input Trace

Win32 sends a window message -> `Window::WindowProc` exposes it through `OnWindowMessage` -> `InputSystem` asks `Win32InputBackend` to translate it -> global state is updated -> focus router selects System/UI/Gameplay using capture state and registered rectangles -> immediate subscribers run or an event is queued -> Application calls `ProcessDeferredEvents` -> camera/editor/console consumers receive only their enabled layer.

## Explicit Non-Capabilities And Risks

- No non-Windows window/input backend, multiple-window manager, headless platform host, gamepad, controller hotplug, clipboard, file dialog, drag-and-drop, accessibility API, touch, pen, or IME composition capability was found in `Engine/Platform`.
- Fullscreen is borderless-monitor fullscreen, not an exclusive display-mode API.
- `Window` directly exposes Win32 types; this is not an opaque cross-platform contract.
- Resize/DPI/focus/capture behavior needs paired normal, minimized, fullscreen, multi-monitor, alt-tab, and high-DPI execution evidence.
