# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated firmware builds.

## Workflows

### `build.yml` - Firmware Build

**Triggers:**
- Pull requests (except for changes to `.md` files, `docs/`, `LICENSE`, `.gitignore`)

**What it does:**
1. Builds firmware for both `inkplate5v2` and `inkplate10` in parallel
2. Caches Arduino CLI and libraries for faster builds
3. Reports firmware size in PR comments
4. Uploads `.bin` firmware artifacts (retained for 30 days)

**Build Matrix:**
- **Board Types:** Inkplate 5 V2, Inkplate 10
- **Runner:** Ubuntu Latest

**Dependencies:**
- Arduino CLI 1.3.1 (pinned version)
- Inkplate_Boards ESP32 core
- Inkplate Library
- PubSubClient library

## Build Artifacts

After a successful build, firmware binaries are uploaded as artifacts:

- `firmware-inkplate5v2` → `inkplate5v2.ino.bin`
- `firmware-inkplate10` → `inkplate10.ino.bin`

Download artifacts from the Actions run page.

## Caching Strategy

To speed up builds, the workflow caches:
- Arduino CLI installation (`~/.arduino15`)
- Installed libraries (`~/Arduino/libraries`)

Cache key includes:
- Arduino CLI version (1.3.1)
- Hash of `setup.ps1` and `arduino-cli.yaml` (if present)

## Local Testing

To test the workflow locally before pushing:

```bash
# Install act (GitHub Actions local runner)
# https://github.com/nektos/act

# Run the workflow
act pull_request
```

## Workflow Configuration

### Adding New Boards

To add a new board to the build matrix:

1. Edit `.github/workflows/build.yml`
2. Add board to the `matrix.board` list:
   ```yaml
   matrix:
     board:
       - inkplate5v2
       - inkplate10
       - newboard  # Add here
   ```
3. Ensure board is configured in `build.sh`

### Changing Triggers

To build on push to main:

```yaml
on:
  push:
    branches:
      - main
  pull_request:
    paths-ignore:
      - '**.md'
```

### Changing Arduino CLI Version

Update in two places:
1. Cache key in `build.yml`:
   ```yaml
   key: ${{ runner.os }}-arduino-cli-1.3.1-...
   ```
2. Install step in `build.yml`:
   ```yaml
   sh -s 1.3.1
   ```

## Troubleshooting

### Build Fails on CI but Works Locally

- Check Arduino CLI version matches (1.3.1)
- Verify library dependencies are installed
- Check build.sh has Unix line endings (LF not CRLF)

### Cache Not Working

- Cache is scoped per branch
- Cache max size is 10 GB per repository
- Old caches are automatically evicted after 7 days

### PR Comments Not Appearing

- Ensure workflow has `pull-requests: write` permission (default for public repos)
- Check Actions run logs for API errors

## Future Enhancements

See `plan.md` Phase 10 for planned features:
- Semantic versioning
- Changelog validation
- Automated GitHub Releases
- Firmware size comparison (current vs base branch)
