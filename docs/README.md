
## Inkplate Dashboard Documentation

| File/Folder | Description |
|-------------|-------------|
| [user/README.md](user/README.md) | Guides and help for end users of Inkplate Dashboard |
| [dev/README.md](dev/README.md) | Documentation for developers, contributors, and maintainers |

This documentation is organized for two audiences:

### Folder Structure

```
docs/
	README.md                # This file (overview, structure, conventions)
	user/                    # User-facing guides and help
		USING.md               # Main user guide
		...                    # Other user docs (setup, troubleshooting, etc.)
	dev/                     # Developer and contributor docs
		BUILD_SYSTEM.md        # Build system documentation
		DISPLAY_CYCLE.md       # Device runtime flow
		README.md              # Developer doc index
		adr/                   # Architecture Decision Records (ADRs) and deep-dive reports
			ADR-*.MD             # All ADRs and technical reports (see naming below)
```

### What Goes Where

- Place all user-facing documentation (setup, usage, troubleshooting, FAQs) in `docs/user/`.
- Place all developer/contributor documentation (build, architecture, technical deep-dives) in `docs/dev/`.
- All architecture decisions, technical reports, and deep-dive docs must go in `docs/dev/adr/`.
- Images and diagrams should be placed in a top-level `assets/` folder and referenced from the docs.

### Markdown File Naming Requirements

- All Architecture Decision Records and technical reports in `docs/dev/adr/` must be named with the prefix `ADR-`, e.g. `ADR-ARCHITECTURE.md`, `ADR-BUILD_SYSTEM.md`.
- Use descriptive, all-uppercase names with hyphens or underscores for readability (e.g. `ADR-HOURLY_SCHEDULING.md`).
- User docs in `docs/user/` should use clear, descriptive names (e.g. `USING.md`, `FACTORY_RESET.md`).
- Avoid spaces in filenames; use underscores or hyphens instead.

See the respective folders for more details and a full index of available documentation.

**Note:** Whenever you add a new `.md` documentation file to any folder, update the corresponding `README.md` in that folder to include it in the documentation index table.