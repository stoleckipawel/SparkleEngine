# Renderer Runtime Configuration

Status: Renderer feature-family index

Scope: route the independently maintained catalogs for aggregate rendering settings, persistence, selectors, defaults, requested state, and active-state reachability

| Document | Open it for |
| --- | --- |
| [Settings State And Persistence](SettingsStateAndPersistence.md) | aggregate settings transport, defaults, startup/editor commit, persistence, restart requirements, and diagnostics |
| [Feature Selector Catalog](FeatureSelectorCatalog.md) | exact public/CVar selector membership, consumers, requested-versus-active behavior, and ineffective or absent controls |

Settings state owns the durable aggregate. The selector catalog owns exact reachability and must not become a second persistence schema. The parent [Renderer Feature Dossiers](../README.md) index owns capability routing.
