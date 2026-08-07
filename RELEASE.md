# Release process

This fork publishes releases as git tags, GitHub Releases, and a PlatformIO
Registry package. Everything after the tag push is automated by
[`.github/workflows/release.yml`](.github/workflows/release.yml).

The PlatformIO package name for this fork is **`ESP32-BLE-Gamepad-LeeNX`**,
not `ESP32-BLE-Gamepad` — that name is already registered on the PlatformIO
Registry by the upstream author (`lemmingdev`), and publishing requires
owning the package name.

## One-time setup (repo owner)

The publish step needs a PlatformIO auth token stored as a repo secret:

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
5. In the `LeeNX/ESP32-BLE-Gamepad` GitHub repo, add the token as a secret
   named `PLATFORMIO_AUTH_TOKEN` (Settings → Secrets and variables →
   Actions).

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
- Runs `pio pkg publish` to push the new version to the PlatformIO
  Registry under `ESP32-BLE-Gamepad-LeeNX`.

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
