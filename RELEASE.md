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

- **Running as `lemmingDev`**: publishes unchanged, as `lemmingdev/ESP32-BLE-Gamepad`
  (the package already registered on the PlatformIO Registry).
- **Running as any other owner (a fork)**: a fork can't publish under a
  package name/namespace it doesn't own, so publishing is **opt-in** —
  disabled unless the fork has set both the `PLATFORMIO_PACKAGE_NAME` and
  `PLATFORMIO_PACKAGE_OWNER` repo variables (see setup below). If they
  aren't set, `release.yml` logs a warning and skips the publish step
  entirely, rather than guessing a name. If they are set, the workflow
  patches the manifest's `name` and `repository.url` to match — only in
  the workflow's working copy for that run, not committed. This is why a
  PR carrying these CI files back to upstream needs no editing: merged
  as-is, it just publishes as the canonical package.

## One-time setup (per repository/fork)

**Upstream (`lemmingDev/ESP32-BLE-Gamepad`)** only needs the auth token
(step 4 below) — the package name/owner are hardcoded to the canonical
values.

**Any fork** that wants to publish its own PlatformIO package needs both
the repo variables and the auth token:

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
4. Generate a token with `pio account token`, and add it as a repo secret
   named `PLATFORMIO_AUTH_TOKEN` (Settings → Secrets and variables →
   Actions → **Secrets**).
5. Add two repo **variables** (same page, **Variables** tab instead of
   Secrets):
   - `PLATFORMIO_PACKAGE_NAME` — the package name to publish, e.g.
     `ESP32-BLE-Gamepad-LeeNX`.
   - `PLATFORMIO_PACKAGE_OWNER` — the PlatformIO account username from
     step 3 that owns it (used both to pass `--owner` to `pio pkg publish`
     explicitly and to build the registry link in the job summary).

Until both variables are set, `release.yml` still tags and creates a
GitHub Release as normal — it just skips the PlatformIO publish step with
a warning instead of failing or guessing a name. Once a real publish
succeeds, the job summary for that run includes a direct link to the
package on the PlatformIO Registry.

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
- Resolves the PlatformIO package identity as described above (canonical
  package upstream, opt-in `PLATFORMIO_PACKAGE_NAME`/`_OWNER` for forks, or
  skip), then runs `pio pkg publish` and links the published package in
  the run's job summary.

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
