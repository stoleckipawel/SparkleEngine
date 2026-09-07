# Engineering Foundations

**Status:** foundation index

These standards apply across source modules when their named concern is touched. Select them from the change surface; they are not a mandatory linear reading list.

## Choose By Change

| If the change introduces... | Read first | Then verify |
| --- | --- | --- |
| a new owner, dependency, public API, or lifetime | Module Ownership | build membership and every producer/consumer |
| a new name, ID, path, command, setting, or diagnostic | Naming | existing vocabulary and discoverability |
| a holder, copy, cache, snapshot, layout, or publication | Data And Memory | single truth, copy budget, capacity, and retirement |
| C++ structure beyond formatter policy | Code Style | ownership placement and compiler/static-analysis behavior |

Several rows often apply together; for example, a new cache changes ownership, naming, data lifetime, and code shape.

## Foundation Documents

| Document | Read it when... |
| --- | --- |
| [Module Ownership](ModuleOwnership.md) | changing repository structure, module boundaries, dependencies, public APIs, ownership, or lifetime |
| [Naming](Naming.md) | introducing or renaming code, files, concepts, commands, configuration, or diagnostics |
| [Data And Memory](DataAndMemory.md) | adding data holders, copies, snapshots, layouts, allocation, or publication boundaries |
| [Code Style](CodeStyle.md) | writing or reviewing C++ source conventions beyond executable formatter/compiler policy |

Module-specific rules live in [Engineering Modules](../Modules/README.md).
