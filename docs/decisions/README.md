# Architecture Decision Records

ADRs capture significant architectural choices: their context, the decision, the consequences, and the alternatives considered. They live forever as a record of *why* we built it this way; they are never edited after acceptance, only superseded by newer ADRs.

## Conventions

- **Filename:** `NNNN-short-slug.md`, four-digit zero-padded sequence.
- **Status:** `Proposed`, `Accepted`, `Superseded by ADR-NNNN`, `Deprecated`.
- **Body:** Context → Decision → Consequences → Alternatives considered → Self-critique → Revisit-at.
- **Editing:** Once accepted, an ADR is immutable. To change a decision, write a new ADR that supersedes it; update the old one's status to point at the new one.

## Index

| ID | Status | Title |
|---|---|---|
| [0001](0001-esp32-s3-primary-target.md) | Accepted | ESP32-S3 as primary target |
| [0002](0002-arduinofake-coexistence.md) | Superseded by 0003 | Coexistence with ArduinoFake |
| [0003](0003-supersede-arduinofake-coexistence.md) | Accepted | Supersede coexistence with ArduinoFake — we don't ship it as a dep |
