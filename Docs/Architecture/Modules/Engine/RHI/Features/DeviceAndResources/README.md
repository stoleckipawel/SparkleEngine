# RHI Device And Resources

Status: RHI feature-family index

Scope: route device creation and aggregate lifetime, neutral capability truth, resource memory, formats, and descriptor binding

| Document | Open it for |
| --- | --- |
| [Backend Selection And Device Capabilities](BackendSelectionAndDeviceCapabilities.md) | compiled/requested backend selection, device/queue creation, capability truth, and partial-create rejection |
| [Device Lifecycle And Failure Recovery](DeviceLifecycleAndFailureRecovery.md) | aggregate publication, owner-thread lifetime, settlement, swapchain recovery boundary, device loss, and explicit non-recovery |
| [Resource Lifetime And Memory](ResourceLifetimeAndMemory.md) | formats, allocation, views, upload/readback, aliasing, budgets, pressure, and GPU-safe retirement |
| [Descriptor Binding](DescriptorBinding.md) | layout/set/table identity, allocation, writes, arrays, capacity, recording lifetime, and backend lowering |

The parent [RHI Feature Dossiers](../README.md) index owns capability routing. Each child retains its own D3D12/Vulkan and feature-local proof contract.
