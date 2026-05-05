# Documentation map

This project's docs follow [Diátaxis](https://diataxis.fr/), split by audience.

## Audiences

- **User** — you write ESP32 sketches and want to test them with this framework. Start at
  [`user/tutorials/`](user/tutorials/).
- **Dev** — you contribute to the framework itself. Start at
  [`dev/tutorials/setting-up-dev-environment.md`](dev/tutorials/setting-up-dev-environment.md).

## Categories (per Diátaxis)

- **Tutorials** — learning-oriented lessons.
- **How-to** — task-oriented recipes.
- **Reference** — information-oriented authoritative listings.
- **Explanation** — understanding-oriented background and rationale.

## Index

```text
docs/
├── user/
│   ├── tutorials/         (T1+) "your first GPIO test", etc.
│   ├── how-to/            (T1+) recipes for testing scenarios
│   ├── reference/         (T1+) sim API, supported Arduino APIs, peripheral list
│   └── explanation/       (T1+) why virtual time, framework vs ArduinoFake, etc.
├── dev/
│   ├── tutorials/         (T0)  setting-up-dev-environment
│   ├── how-to/            (T1+) add a new Arduino API, add a peripheral fake
│   ├── reference/         (T2+) control protocol, repo layout
│   └── explanation/       (T0+) architecture-overview
├── decisions/             ADRs (immutable; supersede with new ADR)
└── superpowers/
    ├── specs/             design specs (master + per-tier)
    └── plans/             implementation plans (one per tier)
```

## Audience discipline

Same fact in two docs: one of them must be a link, not a duplicate. Stale docs are worse
than missing docs (per [`AGENTS.md`](../AGENTS.md)).
