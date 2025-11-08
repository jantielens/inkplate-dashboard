# Copilot Agent Instructions for inkplate-dashboard

## Project Overview

**inkplate-dashboard** is an Arduino/ESP32 firmware for Inkplate e-ink displays by Soldered Electronics. It transforms devices into configurable image dashboards with WiFi setup, web-based configuration, OTA updates, optional MQTT telemetry, and power-efficient deep sleep. The project supports 4 boards: Inkplate 2, Inkplate 5 V2, Inkplate 6 Flick, and Inkplate 10.

**Key Technologies:**
- **Language:** C++ (Arduino framework)
- **Platform:** ESP32 (Inkplate_Boards package)
- **Build Tool:** Arduino CLI 1.3.1
- **Libraries:** InkplateLibrary, ArduinoJson, PubSubClient (MQTT)
- **Target Runtime:** ESP32 with e-ink displays (1-bit and 3-bit grayscale)

**Project Size:** ~30 source files, ~50 documentation files

## Critical Build Requirements

### Prerequisites (ALWAYS REQUIRED)

Before building, Arduino CLI and board packages MUST be installed:

**On Windows (PowerShell):**
```powershell
.\setup.ps1
```

**On Linux/macOS (Bash):**
```bash
# Install Arduino CLI first: https://arduino.github.io/arduino-cli/latest/installation/
chmod +x build.sh
./scripts/setup_local_testing.sh  # If testing web flasher
```

**Required libraries** (installed by setup scripts):
- InkplateLibrary
- ArduinoJson
- PubSubClient

### Build Commands (TRUST THESE)

**Build a specific board:**
```powershell
# Windows
.\build.ps1 inkplate5v2

# Linux/macOS
./build.sh inkplate5v2
```

**Valid board names:** `inkplate2`, `inkplate5v2`, `inkplate10`, `inkplate6flick`

**Build all boards:**
```powershell
# Windows
.\build.ps1 all

# Linux/macOS
./build.sh all
```

**Upload to device (development only):**
```powershell
.\upload.ps1 -Board inkplate5v2 -Port COM7
```

### Build System Architecture (CRITICAL)

The build system uses a **non-standard Arduino pattern** to share code across 4 hardware variants:

1. **Common code** lives in `common/src/` (`.cpp`, `.h` files)
2. **Shared setup()/loop()** is in `common/src/main_sketch.ino.inc` (NOT a `.ino` file!)
3. **Board-specific sketches** are minimal (10 lines) in `boards/{board}/{board}.ino`
4. **Board configs** define hardware constants in `boards/{board}/board_config.h`

**Build Process (automatic via scripts):**
- Build script temporarily **copies** all `.cpp` files from `common/src/` to each board's sketch directory
- Arduino CLI compiles with custom include paths and partition scheme
- Build script **cleans up** copied files after compilation
- **NEVER manually copy/move files** - let the build scripts handle this

**Key Build Property:**
- Partition scheme: `min_spiffs` (1.9MB APP with OTA / 190KB SPIFFS)
- This is **required** for OTA updates to work

### Build Outputs

Binaries are in `build/{board}/`:
- `{board}.ino.bin` - Main firmware
- `{board}.ino.bootloader.bin` - ESP32 bootloader
- `{board}.ino.partitions.bin` - Partition table
- `{board}-v{version}.bin` - Versioned copies (for releases)

## Version Management (MUST FOLLOW)

### Version Update Process

**ALWAYS update version when making code changes:**

1. **Before starting work:** Check current version on main branch in `common/src/version.h`

2. **Before finalizing PR:** Re-check main branch version to avoid conflicts:
   ```bash
   git fetch origin main
   git show origin/main:common/src/version.h | grep FIRMWARE_VERSION
   ```
   - If version changed since you started, increment from the NEW version
   - If your version conflicts, bump to next appropriate version
   - Update your PR branch: `git rebase origin/main` or `git merge origin/main`

3. Edit `common/src/version.h`:
   ```cpp
   #define FIRMWARE_VERSION "1.3.2"  // Update this
   ```

4. Update `CHANGELOG.md` with new version section:
   ```markdown
   ## [1.3.2] - 2025-11-06
   
   ### Added
   - Feature description
   
   ### Fixed
   - Bug fix description
   ```

5. Follow **semantic versioning**:
   - **MAJOR**: Breaking changes (config format changes, API incompatibility)
   - **MINOR**: New features (backward compatible)
   - **PATCH**: Bug fixes (backward compatible)

**The version-check.yml workflow will validate this on PRs and detect conflicts.**

### Handling Version Conflicts

If the version-check workflow fails with "version already exists":

1. Fetch latest main: `git fetch origin main`
2. Check current version: `git show origin/main:common/src/version.h | grep FIRMWARE_VERSION`
3. Rebase or merge main into your branch: `git rebase origin/main`
4. Increment to the next version beyond what's now on main
5. Update `CHANGELOG.md` accordingly
6. Push updated changes

**CRITICAL:** Never push a version that already exists on main. Always increment from the latest version.

**Resolving CHANGELOG.md conflicts during rebase:**

When rebasing, CHANGELOG.md conflicts are common. Resolve by keeping BOTH version entries:

```markdown
## [Unreleased]

## [1.3.5] - 2025-11-08    # Your PR version (newer)
### Changed
- Your changes here

## [1.3.4] - 2025-11-08    # Version from main (older)
### Added
- Changes from main here
```

**Steps:**
1. Edit CHANGELOG.md to include both versions (yours first, then main's)
2. Remove conflict markers (`<<<<<<<`, `=======`, `>>>>>>>`)
3. `git add CHANGELOG.md`
4. `git rebase --continue`
5. **VERIFY the rebase completed successfully:**
   ```bash
   git log --oneline -5
   # Should show your commits AFTER main's latest commit
   ```
6. **VERIFY branch is ready to push:**
   ```bash
   git status
   # Should say "Your branch and 'origin/<branch>' have diverged"
   # Should NOT say "rebase in progress" or show conflicts
   ```

**CRITICAL - Completing the rebase:**

If you **CAN** force-push:
```bash
git push --force-with-lease origin <branch-name>
```

If you **CANNOT** force-push (due to tool limitations):
1. **VERIFY** the rebase succeeded locally: `git log --oneline -5`
2. **DO NOT** claim the rebase is complete until the push succeeds
3. Create a PR comment with **EXACT COMMANDS** for the user:
   ```markdown
   Rebase completed locally but I cannot force-push. Please run:
   
   ```bash
   git fetch origin
   git checkout <branch-name>
   git log --oneline -5  # Verify rebase looks correct
   git push --force-with-lease origin <branch-name>
   ```
   
   Current HEAD commit: <commit-sha>
   Expected to replace: <old-sha>
   ```
4. **DO NOT** update the PR description claiming success until push is verified

## GitHub Workflows & CI/CD

### Pull Request Workflows

**On PR to main:**

1. **`version-check.yml`** - Validates version increment and CHANGELOG entry
   - Compares `common/src/version.h` between PR and main
   - Checks for CHANGELOG entry for new version
   - Posts PR comment with guidance

2. **`build.yml`** - Builds all 4 boards in parallel
   - Validates compilation succeeds
   - Checks firmware sizes
   - Uploads build artifacts
   - Posts PR comment with build summary

**Both must pass before merge.**

### Release Workflow

**After merging to main:**

1. **User creates git tag** (DO NOT automate this step):
   ```bash
   git tag v1.3.2
   git push origin v1.3.2
   ```
   **CRITICAL:** Only users should create release tags. Never create or push tags automatically.

2. **`release.yml`** automatically:
   - Verifies tag matches `version.h`
   - Builds all 4 boards (12 binaries total)
   - Creates GitHub Release with CHANGELOG excerpt
   - Generates ESP Web Tools manifests
   - Commits binaries and manifests to `main-flasher` branch

3. **`flasher-static-site.yml`** automatically:
   - Deploys flasher site to GitHub Pages
   - Users can flash firmware from browser

**IMPORTANT:** The `main-flasher` branch hosts the web flasher and firmware binaries. Do NOT manually edit this branch.

## Project Structure & Key Files

### Repository Root Files

```
build.ps1               # Windows build script
build.sh                # Linux/macOS build script
setup.ps1               # Windows Arduino CLI setup
upload.ps1              # Development upload script
CHANGELOG.md            # Version history (MUST UPDATE)
README.md               # User-facing documentation
```

### Directory Structure

```
boards/{board}/                      # Board-specific sketches (4 boards)
  {board}.ino                        # 10-line entry point (validation + includes)
  board_config.h                     # Hardware constants (screen size, pins, etc.)

common/                              # Shared code library
  library.properties                 # Arduino library metadata
  src/
    main_sketch.ino.inc              # Shared setup() and loop() (200+ lines)
    version.h                        # FIRMWARE_VERSION (UPDATE THIS)
    config_manager.h/cpp             # Configuration storage
    wifi_manager.h/cpp               # WiFi management
    config_portal.h/cpp              # Web configuration portal
    display_manager.h/cpp            # Display abstraction
    image_manager.h/cpp              # Image download/display
    power_manager.h/cpp              # Deep sleep management
    mqtt_manager.h/cpp               # Home Assistant MQTT integration
    github_ota.h/cpp                 # OTA firmware updates
    logger.h/cpp                     # Serial logging
    utils.h/cpp                      # Utility functions
    modes/                           # Mode controllers
      ap_mode_controller.h/cpp       # AP mode (WiFi setup)
      config_mode_controller.h/cpp   # Config mode (dashboard setup)
      normal_mode_controller.h/cpp   # Normal mode (display images)
    ui/                              # UI components
      ui_error.h/cpp                 # Error screens
      ui_messages.h/cpp              # Info screens
      ui_status.h/cpp                # Status screens

.github/workflows/
  version-check.yml                  # Version validation on PRs
  build.yml                          # Build validation on PRs
  release.yml                        # Release automation on tags
  flasher-static-site.yml            # GitHub Pages deployment

scripts/
  generate_manifests.sh              # Generate ESP Web Tools manifests
  generate_latest_json.sh            # Generate release metadata
  prepare_release.sh                 # Manual release preparation
  setup_local_testing.sh             # Local flasher testing setup

docs/
  user/                              # End user documentation
    USING.md                         # Main user guide
    config_portal_guide.md           # Config portal reference
    FACTORY_RESET.md                 # Factory reset instructions
  dev/                               # Developer documentation
    BUILD_SYSTEM.md                  # Build system deep dive
    RELEASE_WORKFLOW.md              # Complete release process
    README.md                        # Developer doc index
    adr/                             # Architecture Decision Records
      ADR-ARCHITECTURE.MD            # Multi-board design
      ADR-BUILD_SYSTEM.MD            # Build system details
      ADR-MODES.MD                   # Device modes
      ADR-ADDBOARD.MD                # Adding board support
      ... (see docs/dev/adr/ for full list)
```

### Configuration Files

- **`common/library.properties`**: Arduino library metadata
- **`boards/{board}/board_config.h`**: Board-specific hardware constants
  - Screen dimensions, pins, display modes, optimization flags
- **`common/src/version.h`**: Firmware version (semantic versioning)

## Code Change Guidelines

### Adding New Features

1. **Add code to `common/src/`** (shared across all boards)
2. **Use board-specific constants** from `board_config.h`:
   ```cpp
   #include "board_config.h"
   
   void myFunction() {
     if (SCREEN_WIDTH > 800) {
       // Large display logic
     }
     
     if (HAS_TOUCHSCREEN) {
       // Touchscreen logic
     }
   }
   ```

3. **Update version** in `common/src/version.h`
4. **Update CHANGELOG.md** with feature description
5. **Update documentation** if needed:
   - `docs/user/` - End user guides (USING.md, config guides, etc.)
   - `docs/dev/` - Developer docs (BUILD_SYSTEM.md, RELEASE_WORKFLOW.md, etc.)
   - `docs/dev/adr/` - Architecture Decision Records (ADR-*.MD)
   - Create new ADR for significant architectural decisions
   - Update relevant README.md files to index new documentation
6. **Test on all boards**: `./build.sh all`

### Board-Specific Optimizations

Some boards have performance differences:

- **Inkplate 2**: Slow refresh (20+ seconds)
  - Uses `DISPLAY_MINIMAL_UI=true` to skip intermediate screens
  - Uses `DISPLAY_FAST_REFRESH=false` flag
  
- **Other boards**: Fast refresh (6-10 seconds)
  - Uses `DISPLAY_MINIMAL_UI=false` for full UI
  - Uses `DISPLAY_FAST_REFRESH=true` flag

**Use these flags instead of hardcoded board checks:**
```cpp
// GOOD - flexible and board-agnostic
if (DISPLAY_MINIMAL_UI) {
  // Skip intermediate screen
}

// BAD - hardcoded board check
#ifdef ARDUINO_INKPLATE2
  // Skip intermediate screen
#endif
```

### Adding a New Board

See `docs/dev/adr/ADR-ADDBOARD.MD` for complete guide. Summary:

1. Create `boards/newboard/` directory
2. Add `newboard.ino` (copy from existing board, change validation)
3. Add `board_config.h` with hardware constants
4. Update `$boards` hashtable in `build.ps1` and `build.sh`
5. Test build: `./build.sh newboard`

### Modifying UI Screens

UI components are in `common/src/ui/`:
- `ui_error.h/cpp` - Error messages
- `ui_messages.h/cpp` - Info/status messages
- `ui_status.h/cpp` - Status displays

**CRITICAL for small displays (Inkplate 2):**
- Check `DISPLAY_MINIMAL_UI` flag before showing intermediate screens
- Use conditional Y-positioning for logos:
  ```cpp
  int y = DISPLAY_MINIMAL_UI ? MARGIN : (SCREEN_HEIGHT / 2 - 50);
  ```

## Validation & Testing

### Pre-Commit Checklist

1. ✅ **Re-check main branch version** to avoid conflicts:
   - `git fetch origin main`
   - `git show origin/main:common/src/version.h | grep FIRMWARE_VERSION`
   - If version changed, rebase/merge main and increment from NEW version
2. ✅ Updated `common/src/version.h`
3. ✅ Updated `CHANGELOG.md`
4. ✅ Reviewed and updated documentation:
   - Check if changes affect user guides in `docs/user/`
   - Check if changes affect developer docs in `docs/dev/`
   - Create new ADR in `docs/dev/adr/` for architectural decisions
   - Update README.md files to index new documentation
5. ✅ Built all boards: `./build.sh all`
6. ✅ No compilation errors
7. ✅ Firmware sizes reasonable (<1.5MB per board)

**NOTE:** Do NOT create or push git tags. Release tagging is a manual user action only.

### CI/CD Validation

**PR checks (automatic):**
- Version increment validated
- CHANGELOG entry validated
- All 4 boards build successfully
- Firmware sizes within limits

**Release checks (automatic on tag push):**
- Tag matches `version.h`
- All builds succeed
- GitHub Release created
- Manifests generated
- Flasher deployed

### Manual Testing (Optional)

**Local build test:**
```bash
./build.sh inkplate5v2
ls -lh build/inkplate5v2/*.bin
```

**Local flasher test:**
```bash
./scripts/setup_local_testing.sh
cd flasher
python3 -m http.server 8000
# Visit http://localhost:8000
```

## Common Issues & Solutions

### Build Failures

**Issue: "Wrong board selection" error**
- Cause: Board-specific validation in `.ino` file
- Solution: Use build scripts, which set correct FQBN automatically

**Issue: "Include file not found"**
- Cause: Include paths not set
- Solution: Use build scripts (they set `--build-property` flags)

**Issue: ".cpp files not compiling"**
- Cause: Arduino CLI doesn't auto-compile library `.cpp` files
- Solution: Build scripts copy `.cpp` to sketch directory (automatic)

### Version/Release Issues

**Issue: PR blocked by version check**
- Cause: Version not incremented or CHANGELOG missing
- Solution: Update `version.h` and `CHANGELOG.md`

**Issue: Release workflow fails**
- Cause: Tag doesn't match `version.h`
- Solution: Ensure tag is `v{VERSION}` (e.g., `v1.3.2`)

**Issue: Flasher not updating**
- Cause: Manifests not committed to `main-flasher`
- Solution: Check release workflow logs, verify `main-flasher` branch

### Display Issues

**Issue: UI not visible on Inkplate 2**
- Cause: Text positioned off 104px screen
- Solution: Use `DISPLAY_MINIMAL_UI` flag and conditional Y-positioning

**Issue: Slow boot on Inkplate 2**
- Cause: Too many intermediate screens
- Solution: Skip screens when `DISPLAY_MINIMAL_UI=true`

### Rebase/Merge Conflict Issues

**Issue: Agent claims rebase succeeded but PR still shows conflicts**
- Cause: Agent completed rebase locally but failed to push, or push didn't succeed
- Solution: 
  1. Check branch history: `git log --oneline origin/<branch> -5`
  2. If commits don't show rebase, the push never succeeded
  3. Agent must either: (a) successfully push, OR (b) provide user with exact commands and NOT claim success

**Issue: CHANGELOG.md conflicts during rebase**
- Cause: Multiple versions added to CHANGELOG between PR start and rebase
- Solution: Keep BOTH version entries, newer version first (see "Handling Version Conflicts" section)

**Issue: PR shows "mergeable_state: dirty" after claimed rebase**
- Cause: Rebase was attempted but not completed or pushed
- Solution: Re-run the rebase process:
  1. `git fetch origin`
  2. `git checkout <branch>`
  3. `git rebase origin/main`
  4. Resolve conflicts
  5. `git rebase --continue`
  6. Verify: `git log --oneline -5` (should show PR commits AFTER main's latest)
  7. `git push --force-with-lease origin <branch>`

## Documentation Requirements

### When to Update Documentation

**ALWAYS check and update documentation when making changes:**

1. **User-Facing Changes** → Update `docs/user/`
   - New features or configuration options → Update `USING.md` or relevant guides
   - Changes to config portal → Update `config_portal_guide.md`
   - New troubleshooting steps → Add to appropriate user guide

2. **Build/Development Changes** → Update `docs/dev/`
   - Build system changes → Update `BUILD_SYSTEM.md`
   - Release process changes → Update `RELEASE_WORKFLOW.md`
   - New development workflows → Update relevant dev docs

3. **Architectural Decisions** → Create new ADR in `docs/dev/adr/`
   - New board support → Document in `ADR-ADDBOARD.MD` or create new ADR
   - Significant design decisions → Create `ADR-{TOPIC}.MD`
   - Performance optimizations → Document rationale and trade-offs
   - **ADR Naming:** Use `ADR-` prefix, all caps, descriptive name (e.g., `ADR-MQTT_TELEMETRY.MD`)

4. **Always Update Index Files**
   - When adding new docs, update the README.md in the same directory
   - Keep documentation tables/lists current

### Key Documentation Files

**User Documentation** (`docs/user/`):
- `USING.md` - Main user guide
- `config_portal_guide.md` - Configuration reference
- `FACTORY_RESET.md` - Factory reset instructions

**Developer Documentation** (`docs/dev/`):
- `BUILD_SYSTEM.md` - Build system deep dive
- `RELEASE_WORKFLOW.md` - Complete release process
- `README.md` - Developer doc index

**Architecture Decision Records** (`docs/dev/adr/`):
- `ADR-ARCHITECTURE.MD` - Multi-board design rationale
- `ADR-BUILD_SYSTEM.MD` - Build system deep dive
- `ADR-MODES.MD` - Device mode state machine
- `ADR-HOURLY_SCHEDULING.MD` - Sleep/wake scheduling
- `ADR-CRC32_GUIDE.MD` - Battery-saving CRC checks
- `ADR-ADDBOARD.MD` - Adding new board support
- See `docs/dev/adr/` for complete list

**Trust the ADRs for detailed implementation context.**

## Key Commands Reference

```bash
# Setup (first time)
./setup.ps1                                    # Windows
# (Install Arduino CLI first on Linux/macOS)

# Build
./build.sh inkplate5v2                         # Build one board
./build.sh all                                 # Build all boards

# Upload (development)
./upload.ps1 -Board inkplate5v2 -Port COM7     # Windows
./upload.ps1 -Board inkplate5v2 -Port COM7 -Erase  # Erase flash first

# Release (MANUAL USER ACTION ONLY - DO NOT AUTOMATE)
git tag v1.3.2                                 # Create tag (user only)
git push origin v1.3.2                         # Push tag (triggers release)

# Local testing
./scripts/setup_local_testing.sh               # Setup local flasher
cd flasher && python3 -m http.server 8000      # Serve flasher locally
```

## Final Notes

**Trust these instructions.** The project uses non-standard Arduino patterns for code sharing. Only search for additional information if:
- Instructions are incomplete for your specific task
- You encounter an error not covered here
- Documentation appears outdated (check CHANGELOG for recent changes)

**When in doubt:**
1. Check `docs/dev/BUILD_SYSTEM.md` for build details
2. Check `docs/dev/RELEASE_WORKFLOW.md` for release process
3. Check relevant ADR in `docs/dev/adr/` for architecture decisions
4. Check `CHANGELOG.md` for recent changes affecting your task
