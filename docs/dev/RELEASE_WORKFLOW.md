# Release and Deployment Workflow

## Overview

This document describes the complete workflow from feature development to production deployment, including all GitHub Actions workflows, scripts, and their interactions.

## Workflow Architecture

```
Feature Branch
    │
    ├─ Development & Testing
    │
    ▼
  main (PR merge)
    │
    ├─ Version Check
    ├─ Build Validation
    │
    ▼
  Create Tag (v0.14.0)
    │
    ▼
  Release Workflow
    │
    ├─ Build all boards (4 × 3 binaries)
    ├─ Create GitHub Release
    ├─ Generate manifests
    ├─ Commit to main-flasher
    │
    ▼
  Flasher Deployment (auto-triggered)
    │
    ├─ Deploy flasher to GitHub Pages
    │
    ▼
  Users can flash from browser
```

## The Complete Flow

### 1. Feature Development

**Branch:** `feature/new-feature` or similar

**Activities:**
- Develop new features
- Update version in `common/src/version.h` if needed
- Update `CHANGELOG.md`
- Test locally with `./build.sh all`

**Local Testing:**
```bash
# Build all boards
./build.sh all

# Test flasher locally (optional)
./scripts/setup_local_testing.sh
cd flasher && python3 -m http.server 8080
```

### 2. Pull Request to main

**Branch:** `feature/new-feature` → `main`

**Triggered Workflows:**

#### A. Version Check (`.github/workflows/version-check.yml`)
```yaml
on:
  pull_request:
    branches: [main]
```

**What it does:**
1. Extracts version from `common/src/version.h`
2. Checks if version tag already exists
3. Validates CHANGELOG.md has entry for this version
4. Fails PR if version already released

**Purpose:** Prevents accidentally releasing same version twice

#### B. Build Validation (`.github/workflows/build.yml`)
```yaml
on:
  pull_request:
    branches: [main]
  push:
    branches: [main]
```

**What it does:**
1. Builds all 4 boards in parallel
2. Validates compilation succeeds
3. Checks firmware size fits in memory
4. Uploads build artifacts (for testing)

**Purpose:** Catch build errors before merge

### 3. Merge to main

**Action:** PR approved and merged

**What happens:**
- Feature branch merged into `main`
- Build workflow runs again on `main` branch
- Code is now ready for release

**Next step:** Create release tag

### 4. Create Release Tag

**Manual step:**

```bash
# Ensure version.h and CHANGELOG.md are updated
git checkout main
git pull

# Create and push tag (must match version in version.h)
git tag v0.14.0
git push origin v0.14.0
```

**Automated alternative (`.github/workflows/create-tag.yml`):**
```yaml
on:
  workflow_dispatch:
    inputs:
      version:
        description: 'Version to release (e.g., 0.14.0)'
```

This workflow:
1. Creates tag from `main` branch
2. Pushes tag to repository
3. Automatically triggers release workflow

### 5. Release Workflow

**Triggered by:** Tag push (`v*.*.*`)

**Workflow:** `.github/workflows/release.yml`

#### Step 1: Build All Boards (Parallel)

**Strategy:**
```yaml
strategy:
  matrix:
    board: [inkplate2, inkplate5v2, inkplate10, inkplate6flick]
```

**For each board:**
1. Install Arduino CLI and dependencies
2. Run `./build.sh <board>`
3. Generates 3 binaries:
   - `inkplate2-v0.14.0.bin` (firmware)
   - `inkplate2-v0.14.0.bootloader.bin`
   - `inkplate2-v0.14.0.partitions.bin`
4. Upload as artifacts

**Output:** 12 binary files total (4 boards × 3 files)

#### Step 2: Create GitHub Release

**Dependencies:** Wait for all builds to complete

**Actions:**
1. Download all build artifacts
2. Extract CHANGELOG section for this version
3. Create GitHub Release with:
   - Tag: `v0.14.0`
   - Name: `Release v0.14.0`
   - Body: Changelog excerpt
   - Assets: All 12 binary files
   - Published (not draft)

**Result:** Users can download binaries from GitHub Releases page

#### Step 3: Generate Web Flasher Manifests

**Script:** `scripts/generate_manifests.sh`

**What it does:**
```bash
./scripts/generate_manifests.sh v0.14.0 artifacts flasher
```

For each board:
1. Finds the 3 binary files in artifacts
2. Creates ESP Web Tools manifest JSON:
   ```json
   {
     "name": "Inkplate Dashboard for Inkplate 2",
     "version": "0.14.0",
     "builds": [{
       "chipFamily": "ESP32",
       "parts": [
         {
           "path": "https://github.com/.../inkplate2-v0.14.0.bootloader.bin",
           "offset": 4096
         },
         {
           "path": "https://github.com/.../inkplate2-v0.14.0.partitions.bin",
           "offset": 32768
         },
         {
           "path": "https://github.com/.../inkplate2-v0.14.0.bin",
           "offset": 65536
         }
       ]
     }]
   }
   ```
3. Saves as `flasher/manifest_inkplate2.json`

**Output:** 4 manifest files pointing to GitHub Release URLs

#### Step 4: Generate Latest Metadata

**Script:** `scripts/generate_latest_json.sh`

**What it does:**
```bash
./scripts/generate_latest_json.sh v0.14.0 artifacts flasher/latest.json
```

Creates `flasher/latest.json`:
```json
{
  "tag_name": "v0.14.0",
  "published_at": "2025-10-22T12:00:00Z",
  "assets": [
    {
      "board": "inkplate2",
      "filename": "inkplate2-v0.14.0.bin",
      "url": "https://github.com/.../inkplate2-v0.14.0.bin",
      "bootloader_url": "https://github.com/.../bootloader.bin",
      "partitions_url": "https://github.com/.../partitions.bin",
      "display_name": "Inkplate 2"
    }
    // ... more boards
  ]
}
```

**Purpose:** Provides API for flasher site to discover available firmware

#### Step 5: Commit Manifests to main-flasher

**Critical step:** Manifests must be on `main-flasher` branch for GitHub Pages

**Actions:**
```bash
# Configure git
git config user.email "actions@github.com"
git config user.name "GitHub Action"

# Fetch and checkout main-flasher branch
git fetch origin main-flasher
git checkout main-flasher

# Copy generated manifests from release branch
git checkout ${{ github.ref_name }} -- flasher/manifest_*.json flasher/latest.json

# Commit and push
git add flasher/manifest_*.json flasher/latest.json
git commit -m "Update flasher manifests for v0.14.0 [skip ci]"
git push origin main-flasher
```

**Why main-flasher?**
- GitHub Pages is configured to deploy from `main-flasher` branch
- Keeps flasher site separate from main codebase
- Manifests are where the deployment workflow expects them

**Note:** `[skip ci]` prevents infinite workflow loops

### 6. Flasher Deployment (Automatic)

**Triggered by:** Release workflow completion

**Workflow:** `.github/workflows/flasher-static-site.yml`

**Trigger configuration:**
```yaml
on:
  push:
    branches: ["main-flasher"]  # Manual commits
  workflow_run:
    workflows: ["Create Release"]  # After release completes
    types:
      - completed
  workflow_dispatch:  # Manual trigger
```

**What it does:**
1. Checks out `main-flasher` branch (explicitly)
2. Uploads `/flasher` directory as Pages artifact
3. Deploys to GitHub Pages

**Deployment:**
- **URL:** `https://jantielens.github.io/inkplate-dashboard/`
- **Contains:**
  - `index.html` - Web flasher UI
  - `manifest_*.json` - Board-specific manifests (just updated)
  - `latest.json` - Metadata (just updated)
  - `README.md` - Documentation

**Result:** Users can visit the site and flash firmware directly from browser

### 7. User Experience

**User visits:** `https://jantielens.github.io/inkplate-dashboard/`

**Flow:**
1. Page loads, reads `latest.json` to show latest version
2. User selects their board (Inkplate 2, 5 V2, 6 Flick, or 10)
3. JavaScript loads appropriate manifest (`manifest_inkplate2.json`)
4. User clicks "Install Inkplate Dashboard"
5. Browser prompts for serial port selection
6. ESP Web Tools:
   - Downloads 3 binaries from GitHub Releases
   - Flashes bootloader → partitions → firmware
   - Verifies flash
7. Device restarts with new firmware
8. Done! ✅

## Scripts Reference

### Build Scripts

#### `build.sh`
**Purpose:** Build firmware for one or all boards

**Usage:**
```bash
./build.sh inkplate2          # Build specific board
./build.sh all                # Build all boards
```

**What it does:**
1. Copies common `.cpp` files to board directory
2. Runs `arduino-cli compile` with correct FQBN
3. Generates 3 binaries with version numbers
4. Cleans up copied files

**Output location:** `build/<board>/`

#### `upload.ps1`
**Purpose:** Upload firmware to connected device (development)

**Usage:**
```powershell
.\upload.ps1 -Board inkplate2 -Port COM7
```

**Not used in CI/CD** - only for local development

### Release Scripts

#### `scripts/generate_manifests.sh`
**Purpose:** Generate ESP Web Tools manifest files

**Usage:**
```bash
./scripts/generate_manifests.sh v0.14.0 artifacts flasher
```

**Arguments:**
- `v0.14.0` - Version tag
- `artifacts` - Directory containing binaries
- `flasher` - Output directory

**Creates:**
- `flasher/manifest_inkplate2.json`
- `flasher/manifest_inkplate5v2.json`
- `flasher/manifest_inkplate6flick.json`
- `flasher/manifest_inkplate10.json`

#### `scripts/generate_latest_json.sh`
**Purpose:** Generate metadata file for flasher site

**Usage:**
```bash
./scripts/generate_latest_json.sh v0.14.0 artifacts flasher/latest.json
```

**Creates:** `flasher/latest.json` with release info

#### `scripts/prepare_release.sh`
**Purpose:** Manual release preparation (alternative to CI/CD)

**Usage:**
```bash
./scripts/prepare_release.sh v0.14.0
```

**What it does:**
1. Builds all boards
2. Copies binaries to `artifacts/`
3. Generates all manifests
4. Shows summary and next steps

**Use case:** Manual releases or pre-flight testing

### Local Testing Scripts

#### `scripts/setup_local_testing.sh`
**Purpose:** Setup local flasher testing environment

**Usage:**
```bash
./scripts/setup_local_testing.sh
```

**What it does:**
1. Creates `flasher/builds/` directory structure
2. Copies binaries from `build/*/` to `flasher/builds/*/`
3. Generates local manifests pointing to `builds/` directory

**After running:**
```bash
cd flasher
python3 -m http.server 8080
# Visit http://localhost:8080
```

## Workflow States & Branches

### Branch Strategy

```
main (default branch)
├── Protected
├── Requires PR
├── CI/CD validates builds
└── Source of truth for releases

main-flasher (deployment branch)
├── Contains flasher site
├── Manifests committed here by release workflow
├── Deployed to GitHub Pages
└── Auto-updated on release

feature/* (development branches)
├── Created from main
├── Merged via PR
└── Deleted after merge
```

### File Locations by Branch

**main branch:**
- Source code (`boards/`, `common/`)
- Build system (`build.sh`, `build.ps1`)
- Scripts (`scripts/`)
- Workflows (`.github/workflows/`)
- Base flasher (`flasher/index.html`, `flasher/README.md`)
- ❌ NO manifests (generated during release)

**main-flasher branch:**
- Flasher site (`flasher/`)
- ✅ Manifests (`flasher/manifest_*.json`)
- ✅ Metadata (`flasher/latest.json`)
- Deployed to GitHub Pages

## GitHub Actions Workflows Summary

| Workflow | Trigger | Purpose | Output |
|----------|---------|---------|--------|
| `version-check.yml` | PR to main | Validate version is new | PR check ✓/✗ |
| `build.yml` | PR/push to main | Validate builds | Build artifacts |
| `create-tag.yml` | Manual dispatch | Create release tag | Git tag |
| `release.yml` | Tag push `v*.*.*` | Build & release | GitHub Release + Manifests |
| `flasher-static-site.yml` | Push to main-flasher<br>Release completion | Deploy flasher | GitHub Pages site |
| `update-latest-json.yml` | Legacy/unused | Old manifest update | N/A |

## Troubleshooting

### Manifests not updating on flasher site

**Symptoms:** Old version shown on flasher site after release

**Cause:** Manifests not committed to `main-flasher` branch

**Fix:**
1. Check release workflow logs
2. Verify manifests committed to `main-flasher`
3. Check flasher deployment workflow ran
4. Manual fix:
   ```bash
   git checkout main-flasher
   git pull
   # Verify manifests are present
   ls flasher/manifest_*.json
   ```

### Release workflow fails at manifest commit

**Symptoms:** Release succeeds but manifests not committed

**Cause:** Git conflicts or permissions issue

**Fix:**
1. Check workflow logs for error message
2. Verify no manual changes on `main-flasher` conflicting
3. Manual commit if needed:
   ```bash
   # Generate manifests locally
   ./scripts/prepare_release.sh v0.14.0
   
   # Commit to main-flasher
   git checkout main-flasher
   git pull
   cp flasher/manifest_*.json flasher/latest.json ../
   git add manifest_*.json latest.json
   git commit -m "Update flasher manifests for v0.14.0"
   git push
   ```

### Flasher deployment doesn't trigger

**Symptoms:** Release completes but flasher site not updated

**Cause:** `workflow_run` trigger not firing

**Fix:**
1. Manually trigger deployment:
   - Go to Actions tab
   - Select "Deploy static content to Pages"
   - Click "Run workflow"
   - Select `main-flasher` branch
2. Check workflow permissions in repository settings

### Binary files not found during manifest generation

**Symptoms:** Manifest generation fails with "file not found"

**Cause:** Build artifacts not uploaded correctly

**Fix:**
1. Check build job logs
2. Verify all 12 binaries created
3. Check artifact upload step
4. Verify artifact download in release job

## Best Practices

### Before Creating a Release

1. ✅ Update `common/src/version.h`
2. ✅ Update `CHANGELOG.md` with new version section
3. ✅ Test builds locally: `./build.sh all`
4. ✅ Merge to main via PR
5. ✅ Wait for CI checks to pass
6. ✅ Create and push tag

### Version Numbers

Follow semantic versioning:
- **MAJOR.MINOR.PATCH** (e.g., `0.14.0`)
- **MAJOR:** Breaking changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes

### CHANGELOG.md Format

```markdown
## [0.14.0] - 2025-10-22

### Added
- New features listed here

### Changed
- Modified features listed here

### Fixed
- Bug fixes listed here
```

### Commit Messages

- Release workflow: `Update flasher manifests for v0.14.0 [skip ci]`
- Feature branch: `feat: add new feature`
- Bug fix: `fix: resolve issue with X`

## Manual Override Procedures

### Emergency Manual Release

If CI/CD fails, you can manually create a release:

```bash
# 1. Build all boards locally
./scripts/prepare_release.sh v0.14.0

# 2. Manually create GitHub Release
gh release create v0.14.0 \
  --title "Release v0.14.0" \
  --notes-file release_notes.md \
  artifacts/*.bin

# 3. Commit manifests to main-flasher
git checkout main-flasher
git pull
git add flasher/manifest_*.json flasher/latest.json
git commit -m "Update flasher manifests for v0.14.0"
git push

# 4. Trigger flasher deployment manually via GitHub Actions UI
```

### Rollback a Release

To rollback to a previous version:

```bash
# 1. Checkout main-flasher
git checkout main-flasher
git pull

# 2. Revert manifest commit
git revert <commit-hash>

# 3. Push
git push

# 4. Flasher will automatically redeploy with previous manifests
```

**Note:** This only rolls back the flasher site. The GitHub Release remains published.

## Summary

The workflow automates the complete release process from code merge to production deployment:

1. **Developer:** Merges feature to `main` with updated version
2. **CI/CD:** Validates build on PR
3. **Developer:** Creates release tag
4. **Release Workflow:** Builds firmware, creates release, generates manifests
5. **Flasher Deployment:** Automatically updates web flasher
6. **Users:** Can immediately flash new version from browser

All workflows are designed to be resilient, with manual override options and clear error messages for troubleshooting.
