# Developer Documentation

This section contains documentation for developers, contributors, and maintainers.

| File | Description |
|------|-------------|
| [BUILD_SYSTEM.md](BUILD_SYSTEM.md) | Documentation for the custom build system and workflow |
| [RELEASE_WORKFLOW.md](RELEASE_WORKFLOW.md) | Complete release workflow from feature development to production deployment |
| [WEB_FLASHER.md](WEB_FLASHER.md) | Web-based firmware flasher implementation using ESP Web Tools |
| [DISPLAY_CYCLE.md](DISPLAY_CYCLE.md) | Overview of the device's display cycle and runtime flow |

**Architecture Decision Records & Reports**

| File | Description |
|------|-------------|
| [ADR-ADDBOARD.MD](adr/ADR-ADDBOARD.MD) | How to add support for a new Inkplate board |
| [ADR-ADR_PLAN.MD](adr/ADR-ADR_PLAN.MD) | Planning notes for architecture decisions |
| [ADR-ARCHITECTURE.MD](adr/ADR-ARCHITECTURE.MD) | Project architecture and multi-board design |
| [ADR-BUILD_EXAMPLES.MD](adr/ADR-BUILD_EXAMPLES.MD) | Build system explained with real examples |
| [ADR-BUILD_QUICK_REFERENCE.MD](adr/ADR-BUILD_QUICK_REFERENCE.MD) | Quick reference for the build system |
| [ADR-CRC32_GUIDE.MD](adr/ADR-CRC32_GUIDE.MD) | Guide to CRC32-based change detection |
| [ADR-HOURLY_SCHEDULING.MD](adr/ADR-HOURLY_SCHEDULING.MD) | Hourly scheduling feature overview |
| [ADR-HOURLY_SCHEDULING_IMPLEMENTATION.MD](adr/ADR-HOURLY_SCHEDULING_IMPLEMENTATION.MD) | Implementation details for hourly scheduling |
| [ADR-HOURLY_SCHEDULING_TEST_FIXES.MD](adr/ADR-HOURLY_SCHEDULING_TEST_FIXES.MD) | Test feedback and fixes for hourly scheduling |
| [ADR-IMPLEMENTATION_NOTES.MD](adr/ADR-IMPLEMENTATION_NOTES.MD) | Notes on retry mechanism and deferred CRC32 |
| [ADR-MODES.MD](adr/ADR-MODES.MD) | Device modes and configuration details |
| [ADR-PULL_REQUEST_TEMPLATE.MD](adr/ADR-PULL_REQUEST_TEMPLATE.MD) | Pull request template for contributors |
| [ADR-ROTATION_PERFORMANCE_OPTIMIZATION.MD](adr/ADR-ROTATION_PERFORMANCE_OPTIMIZATION.MD) | Screen rotation performance optimization |
| [ADR-SCREEN_ROTATION_IMPLEMENTATION.MD](adr/ADR-SCREEN_ROTATION_IMPLEMENTATION.MD) | Implementation of screen rotation feature |
| [ADR-TESTING.MD](adr/ADR-TESTING.MD) | Testing the CI/CD workflow |
| [ADR-TESTING_RETRY_MECHANISM.MD](adr/ADR-TESTING_RETRY_MECHANISM.MD) | Testing the image download retry mechanism |
| [ADR-TIMEZONE_ANALYSIS_REPORT.MD](adr/ADR-TIMEZONE_ANALYSIS_REPORT.MD) | Analysis of timezone handling and DST feasibility |
| [ADR-WEB_PORTAL_OPTIMIZATION.MD](adr/ADR-WEB_PORTAL_OPTIMIZATION.MD) | Web portal HTML/CSS/JavaScript separation for memory optimization |
| [ADR-WORKFLOWS_README.MD](adr/ADR-WORKFLOWS_README.MD) | GitHub Actions CI/CD workflow documentation |

**ADR Structure**

All ADR (Architecture Decision Record) and technical report files in this folder must follow this minimal structure:

```
# Title (short, descriptive)

## Status
Proposed | Accepted | Deprecated | Superseded

## Context
What is the background or problem being addressed?

## Decision
What decision was made and why?

## Consequences
What are the results or implications of this decision?
```

Additional sections (Motivation, Alternatives, References, etc.) are optional but encouraged for complex decisions.