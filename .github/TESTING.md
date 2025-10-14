# Testing the CI/CD Workflow

## Before You Commit

### 1. Test Build Script Locally (Windows)

```powershell
# Test single board build
.\build.ps1 inkplate5v2

# Test all boards
.\build.ps1 all

# Verify artifacts exist
ls build/inkplate5v2/*.bin
ls build/inkplate10/*.bin
```

### 2. Test Build Script (Linux/WSL)

If you have WSL or a Linux environment:

```bash
# Make script executable
chmod +x build.sh

# Test single board build
./build.sh inkplate5v2

# Test all boards
./build.sh all

# Verify artifacts exist
ls build/inkplate5v2/*.bin
ls build/inkplate10/*.bin
```

### 3. Validate Workflow YAML

```powershell
# Install actionlint (YAML validator for GitHub Actions)
# https://github.com/rhysd/actionlint

# Validate workflow file
actionlint .github/workflows/build.yml
```

## After Committing (First Time Setup)

### 1. Initialize Git Repository (if not already done)

```powershell
git init
git add .
git commit -m "Add CI/CD workflow"
```

### 2. Push to GitHub

```powershell
# Add remote (replace with your repository)
git remote add origin https://github.com/YOUR_USERNAME/inkplate-dashboard-new.git

# Push to main branch
git push -u origin main
```

### 3. Enable GitHub Actions

1. Go to your repository on GitHub
2. Click on the **Actions** tab
3. If prompted, click **"I understand my workflows, go ahead and enable them"**

### 4. Test with a Pull Request

```powershell
# Create a test branch
git checkout -b test-ci-workflow

# Make a small change (e.g., update README)
echo "Testing CI" >> README.md
git add README.md
git commit -m "Test CI workflow"

# Push branch
git push -u origin test-ci-workflow
```

Then:
1. Go to GitHub and create a Pull Request from `test-ci-workflow` to `main`
2. Wait for the workflow to run (~2-3 minutes first time)
3. Check the PR for size comments
4. Click on "Details" next to the checks to see build logs
5. Go to Actions tab → Select the workflow run → Download artifacts

## Troubleshooting

### Build Script Not Executable in CI

**Symptom:** CI fails with "Permission denied: ./build.sh"

**Solution:**
```powershell
# On Windows, mark as executable in Git
git update-index --chmod=+x build.sh
git commit -m "Make build.sh executable"
git push
```

### Wrong Line Endings

**Symptom:** CI fails with `\r command not found`

**Solution:**
The `.gitattributes` file should handle this. If not:
```powershell
# Convert to LF
(Get-Content build.sh -Raw) -replace "`r`n", "`n" | Set-Content build.sh -NoNewline
git add build.sh
git commit -m "Fix line endings"
git push
```

### Cache Not Working

**First build is always slow** - this is expected. Subsequent builds should be faster.

To verify cache is working:
1. Push a new commit to an existing PR
2. Build should be ~50% faster
3. Check logs for "Cache restored" message

### Workflow Not Triggering

**Check:**
1. Workflow is in `.github/workflows/` directory
2. File is named `build.yml` (not `build.yaml`)
3. Trigger conditions are met (not just MD changes)

### Manual trigger:**
You can add `workflow_dispatch` to test manually:
```yaml
on:
  pull_request:
    paths-ignore:
      - '**.md'
  workflow_dispatch:  # Already configured!
    inputs:
      board:
        description: 'Board to build'
        required: false
        default: 'all'
        type: choice
        options:
          - all
          - inkplate5v2
          - inkplate10
```

**To trigger manually:**
1. Go to Actions → Build Firmware → Run workflow
2. Select branch and board
3. Click "Run workflow"

### Size Comment Not Appearing

**Permissions issue:**
- Public repos: Should work by default
- Private repos: May need to grant `pull-requests: write` permission

Add to workflow if needed:
```yaml
jobs:
  build:
    permissions:
      contents: read
      pull-requests: write
```

## Local Testing with Act

**Act** lets you run GitHub Actions locally:

### Install Act

```powershell
# Using Chocolatey
choco install act-cli

# Or download from https://github.com/nektos/act
```

### Run Workflow Locally

```bash
# Test pull_request event
act pull_request

# Test specific job
act pull_request -j build

# Test with specific board
act pull_request -j build --matrix board:inkplate5v2
```

**Note:** Act may not perfectly replicate GitHub's environment. Always test with a real PR.

## Verification Checklist

Before marking Phase 9 complete:

- [ ] Build script works on Windows (`.\build.ps1 all`)
- [ ] Build script works on Linux (`./build.sh all`)
- [ ] Workflow YAML is valid (`actionlint`)
- [ ] `.gitattributes` enforces LF for `.sh`
- [ ] build.sh is executable in Git
- [ ] Test PR triggers workflow successfully
- [ ] Both boards build in parallel
- [ ] Firmware size comments appear
- [ ] Artifacts are downloadable
- [ ] Cache works on subsequent builds
- [ ] Markdown-only PRs are skipped

## Performance Benchmarks

Expected times on GitHub Actions (ubuntu-latest):

| Build Type | Cold Cache | Warm Cache |
|------------|------------|------------|
| Single board | ~3-4 min | ~1-2 min |
| Both boards (parallel) | ~3-4 min | ~1-2 min |

**Cache storage:**
- Arduino CLI + ESP32 core: ~500 MB
- Libraries: ~50 MB
- Total: ~550 MB

**Artifact storage:**
- Per board: ~1-2 MB
- Retention: 30 days (configurable)

## Next Steps

After successful testing:

1. **Merge the PR** to main
2. **Update README.md** with CI badge
3. **Start Phase 10** (Versioning & Releases)
4. **Consider adding:**
   - Size comparison with base branch
   - Automated release workflow
   - Changelog validation

## Useful Links

- GitHub Actions Docs: https://docs.github.com/en/actions
- Arduino CLI Docs: https://arduino.github.io/arduino-cli/
- Act (local testing): https://github.com/nektos/act
- actionlint (YAML validator): https://github.com/rhysd/actionlint
