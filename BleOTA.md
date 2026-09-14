# BLE OTA updates (`BleOTA` / `BleOTARollback`)

This library can expose a firmware-update service over BLE: a browser (via
Web Bluetooth), a phone app, or any GATT client that speaks the protocol
below can push a new image into a free OTA slot, validate it, and reboot
into it — with automatic rollback if the new image never proves itself. See
[GattVsHid.md](GattVsHid.md) for how this fits alongside the HID and NUS
services this library also exposes.

This is opt-in: it needs a non-default partition table (below), so it only
applies to sketches that ask for it, not every user of the library.

## Partition layout

`BleOTA` needs three app partitions instead of the usual factory-only or
factory+ota_0 layout: a `factory` slot that's **never** written by OTA (the
last-resort recovery image), plus `ota_0`/`ota_1` for the library to
alternate between on each update.

```
# Name,     Type, SubType,  Offset,   Size,     Comment
nvs,        data, nvs,      0x9000,   0x5000,
otadata,    data, ota,      0xE000,   0x2000,
factory,    app,  factory,  0x10000,  0x140000, # 1.25MB, recovery image, never OTA-written
ota_0,      app,  ota_0,    0x150000, 0x140000, # 1.25MB
ota_1,      app,  ota_1,    0x290000, 0x140000, # 1.25MB
spiffs,     data, spiffs,   0x3D0000, 0x30000,
```

This is `examples/ota/partitions.csv`, sized for 4MB flash (sums to exactly
0x400000). Select it per-sketch with `board_build.partitions` in
`platformio.ini` — see `examples/OTAUpdate/platformio.ini`. `esp_ota_ops`
accepts a `factory`-subtype partition as a valid `esp_ota_set_boot_partition()`
target even though the stock `partitions_two_ota.csv` example doesn't
demonstrate combining all three slots — this design intentionally does.

## Security

`Control` and `Data` require an encrypted, bonded link
(`NIMBLE_PROPERTY::WRITE_ENC`) — unlike `BleNUS`, which stays open. OTA write
access means "can brick or reflash the device," so an unauthenticated write
path isn't acceptable here. `BleGamepad` already enables bonding globally
(`NimBLEDevice::setSecurityAuth(true, false, false)`), so the first write
from an unbonded peer makes the NimBLE host trigger the standard pairing
flow automatically; on Chrome/Edge this surfaces as the OS's own Bluetooth
pairing dialog the first time the page writes to Control or Data.

A sketch using `BleOTA` directly (not through `BleGamepad` — see
`examples/OTARecovery`) must call `NimBLEDevice::setSecurityAuth(true, false, false)`
itself before `NimBLEDevice::createServer()`.

## GATT service

UUIDs are specific to this library — don't reuse them for an unrelated
service.

| Characteristic | UUID | Properties | Direction |
|---|---|---|---|
| OTA Service | `56d794fe-92e6-4227-8570-08eb990b4648` | — | — |
| Control | `4a0b7397-e27a-40c6-b8a0-2ae63eb2d5df` | `WRITE`, `WRITE_ENC` | client → device |
| Data | `28afb7ac-af3e-4e53-a58f-2fef9e5ad51b` | `WRITE_NR`, `WRITE_ENC` | client → device |
| Status | `f2c7832e-1e92-42b2-8d84-540b7db3979b` | `NOTIFY` | device → client |

Like NUS, the service UUID goes in the scan-response data, not the main
31-byte advertisement (which is already full with the HID service UUID).

### MTU and chunk sizing

`BleOTA::begin()` requests the largest usable MTU
(`NimBLEDevice::setMTU(517)`), but **Web Bluetooth's JS API has no way to
read back the negotiated ATT MTU** — this is a real platform gap, not an
oversight. Clients must connect, bond (any encrypted write triggers this),
send `GET_INFO`, and read the reported MTU from the Status response before
sizing their `writeValueWithoutResponse()` chunks to `MTU - 3`. Don't guess.

### Control opcodes

One byte opcode + payload, all multi-byte integers little-endian (matches
both the ESP32 and every browser's `DataView` default).

| Opcode | Name | Payload | Effect |
|---|---|---|---|
| `0x01` | `BEGIN` | `uint32 imageSize, uint8 sha256[32], uint8 versionLen, char version[versionLen]` | Opens the next free OTA slot (`esp_ota_get_next_update_partition`), `esp_ota_begin()`. |
| `0x03` | `END` | — | Verifies size + SHA-256, then `esp_ota_end()` (ESP-IDF's own image-header validation). Does **not** reboot. |
| `0x04` | `REBOOT` | — | `esp_ota_set_boot_partition()` + `esp_restart()` after ~500ms (lets the status notify flush). |
| `0x05` | `ABORT` | — | `esp_ota_abort()`, resets to idle. |
| `0x06` | `CONFIRM_VALID` | — | Calls `BleOTARollback::confirmValid()` remotely — mainly for tooling; the sketch itself should normally call this directly after its own health check. |
| `0x07` | `ENTER_RECOVERY` | `uint32 magic` (must equal `0x4F544152`, `"OTAR"`) | `esp_ota_set_boot_partition(factory)` + `esp_restart()`. |
| `0x08` | `GET_INFO` | — | Triggers an `INFO` status notification. |

`BEGIN`/`END`/`REBOOT` are deliberately separate: a client can finish
writing and validate an image (`END`) without committing to a reboot,
useful for a "flashed and verified — reboot now?" confirmation step.

### Status notifications

One byte state + payload:

| State | Value | Payload |
|---|---|---|
| `IDLE` | 0 | — |
| `RECEIVING` | 1 | `uint32 bytesWritten, uint32 totalSize` |
| `VALIDATING` | 2 | — |
| `READY_TO_REBOOT` | 3 | — |
| `REBOOTING` | 4 | — |
| `ERROR` | 5 | `uint8 errorCode` |
| `INFO` | 6 | `char label[16], uint8 confirmed, uint8 bootAttempts, uint16 mtu` |
| `CONFIRMED` | 7 | — |

Error codes: `SIZE_MISMATCH=1, HASH_MISMATCH=2, FLASH_WRITE_FAILED=3, NO_FREE_PARTITION=4, ALREADY_IN_PROGRESS=5, NOT_STARTED=6, IMAGE_INVALID=7, TOO_LARGE=8, BAD_REQUEST=9`.

### Why writes are queued, not handled inline

`esp_ota_write()` does sector erase+program and can take multiple
milliseconds. Running it directly inside the NimBLE host's characteristic
`onWrite()` callback risks stalling BLE connection timing — a common,
easy-to-hit OTA-over-BLE pitfall. `BleOTA` only enqueues each incoming
Control opcode or Data chunk and returns immediately; a dedicated FreeRTOS
task drains the queue and does the actual flash I/O, in the order the peer
sent it (a single BLE link preserves per-characteristic write ordering).

## Rollback (`BleOTARollback`)

Runs at boot, before any BLE/HID setup — call
`BleOTARollback::checkAndHandleRollback()` as the literal first statement in
`setup()`. It doesn't depend on the bootloader having been built with
`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` (this varies across arduino-esp32
core versions): it tries the native `esp_ota_mark_app_invalid_rollback_and_reboot()`
first, and falls back to its own NVS-backed boot-attempt counter +
`esp_ota_set_boot_partition()` when that isn't supported.

State machine, per boot:

1. Identify the running partition and its image SHA-256
   (`esp_ota_get_partition_description`).
2. If the SHA differs from what's stored in NVS for this slot, it's a
   freshly-flashed image: reset the attempt counter to 1, mark unconfirmed.
3. Else if this slot is already marked confirmed: normal reboot, counter
   untouched.
4. Else: still on probation — increment the counter.
5. If the counter exceeds `maxAttempts` (default 3): roll back — the other
   OTA slot if it's confirmed-good, otherwise `factory` — and reboot.
6. Otherwise boot normally. The sketch calls `confirmValid()` once it's
   satisfied the new build is healthy, which clears the counter and cancels
   any pending native rollback.

NVS (not RTC memory) backs the counter, because RTC_NOINIT clears on
power-on reset — exactly the failure mode rollback needs to survive (a truly
bad image causing a brownout/watchdog power-cycle, not just a clean
`esp_restart()`).

`esp_ota_get_next_update_partition()` never returns `factory`, so "OTA
writes never touch factory" is enforced by ESP-IDF itself, not by this
code.

### Deliberate recovery entry

Two ways to force a boot into `factory` regardless of rollback state:

- Remotely: the `ENTER_RECOVERY` control opcode.
- Physically: `BleOTARollback::setRecoveryButtonPin(gpio)` — if set, a held
  GPIO (LOW, internal pull-up) at boot forces `factory` before any other
  rollback logic runs. Wire it to the board's standard boot button.

## Usage

```cpp
void setup() {
    BleOTARollback::checkAndHandleRollback(); // first line, before anything else

    BleGamepad bleGamepad("My Gamepad", "Me", 100, /*delayAdvertising=*/true);
    bleGamepad.begin();
    bleGamepad.beginOTA();

    // ... your own post-update health check here, if this boot followed an OTA ...
    BleOTARollback::confirmValid();
}
```

See `examples/OTAUpdate/OTAUpdate.ino` for the full sketch, and
`examples/OTARecovery/OTARecovery.ino` for the minimal image meant to live
in `factory` (no `BleGamepad`/HID at all — just enough to re-flash a good
image if both OTA slots are ever exhausted).

## Web Bluetooth client

`tools/ota-web/index.html` is a static, no-build-step page implementing this
protocol against a real browser. Web Bluetooth requires HTTPS or
`localhost`, and isn't available on iOS Safari or (without a flag) some
desktop Linux browsers.

## What's out of scope here

Real-hardware verification (flash throughput at the real negotiated MTU,
the rollback state machine end-to-end, physical recovery entry, and the
WebBLE client against real browsers/OSes) needs actual boards and isn't
covered by compiling this code. Wiring this into the sibling HIL rig for
repeatable automated coverage is a tracked follow-up, not part of this
change.
