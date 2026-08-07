# Release process

Releases are published as git tags, GitHub Releases, and a PlatformIO
Registry package. Everything after the tag push is automated by
[`.github/workflows/release.yml`](.github/workflows/release.yml), and the
workflow is fork-safe: it behaves the same whether it runs in
`lemmingDev/ESP32-BLE-Gamepad` or in any fork, with no fork-specific values
committed to `library.json`.

`library.json` is committed with the canonical upstream identity — package
name `ESP32-BLE-Gamepad`, repository pointing at `lemmingDev`. Before
publishing, the release workflow checks `github.repository_owner`:

- **Running as `lemmingDev`**: publishes unchanged, as `ESP32-BLE-Gamepad`
  (the package already registered on the PlatformIO Registry).
- **Running as any other owner (a fork)**: a fork can't publish under a
  package name it doesn't own, so the workflow publishes under
  `ESP32-BLE-Gamepad-<owner>` instead (e.g. `ESP32-BLE-Gamepad-LeeNX`) and
  points the manifest's `repository.url` at that fork — only in the
  workflow's working copy for that run, not committed. This is why a PR
  carrying these CI files back to upstream needs no editing: merged as-is,
  it just publishes as the canonical package.

## One-time setup (per repository/fork)

Each repository that wants `pio pkg publish` to actually run needs its own
PlatformIO auth token — the token determines which account, and therefore
which package namespace, it publishes to:

1. Create a PlatformIO account (or use an existing one) at
   [platformio.org](https://platformio.org).
2. Locally: `pip install platformio && pio account login`.
3. Make sure the account has a username set — a bare email/password
   account can't publish. `pio pkg publish` fails with
   `HTTPClientError: Please set a username for your account` until you run:
   ```sh
   pio account update --username <your-username>
   ```
   (prompts for your current password to confirm the change).
4. Generate a token with `pio account token`.
5. In that repository's GitHub settings, add the token as a secret named
   `PLATFORMIO_AUTH_TOKEN` (Settings → Secrets and variables → Actions).

Until this secret exists, releases still get tagged and get a GitHub
Release, but the `pio pkg publish` step logs a warning and skips itself
instead of failing.

## Cutting a release

1. Decide the new version, e.g. `0.7.5` (semver: `MAJOR.MINOR.PATCH`).
2. Bump the version in **both** manifest files — they must match the tag
   exactly, or the release workflow will fail on purpose:
   - [`library.properties`](library.properties): `version=0.7.5`
   - [`library.json`](library.json): `"version": "0.7.5"`
3. Commit that change on `master`:
   ```sh
   git add library.properties library.json
   git commit -m "Bump version to 0.7.5"
   git push
   ```
4. Tag the commit and push the tag:
   ```sh
   git tag v0.7.5
   git push origin v0.7.5
   ```

Pushing the tag triggers the existing
[`platformio.yml`](.github/workflows/platformio.yml) build matrix
(`esp32dev`, `lolin_c3_mini`, `esp32s3`). Once that build succeeds,
`release.yml` runs automatically and:

- Verifies `library.properties` / `library.json` versions match the tag
  (fails fast with a clear error if you forgot step 2).
- Creates a GitHub Release for the tag with auto-generated notes.
- Adapts the manifest for forks as described above, then runs
  `pio pkg publish` to push the new version to the PlatformIO Registry.

If the build matrix fails, no release or publish happens — fix the build
and re-tag (delete and re-push the tag, or bump to the next patch version).

## Why release.yml waits on the build workflow

`release.yml` doesn't trigger directly on the tag push. It triggers via
`workflow_run` on completion of the "ESP32-BLE-Gamepad platformio CI"
workflow, so a release is only ever created and published from a tag that
actually compiles on every supported board. This mirrors the pattern
already used by
[`report-size-trend.yml`](.github/workflows/report-size-trend.yml) for
decoupling follow-up reporting from the build.
