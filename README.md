# Harmony Hub Control

Local web UI and helper runtime for an already rooted Logitech Harmony Hub.

This repository is for post-root device ownership work: the web dashboard, IR
database tooling, Bluetooth HID controls, MQTT/Home Assistant bridge, recovery
AP helpers, and the installer that deploys those pieces over SSH.

It does not contain rooting tools, device compromise notes, private keys, live
MQTT credentials, firmware dumps, or personal backups.

## Current Status

- Web UI runs on `http://<hub-ip>:8080/`.
- HTTP authentication is intentionally disabled for LAN-only use.
- IR devices can be configured from database lookup or manual learning.
- Database import supports IRDB, Flipper-IRDB, and RemoteCentral-style Pronto
  sources.
- Flipper parsed `RC5`, `RC6`, `SIRC`, `SIRC15`, and `SIRC20` entries are
  converted to raw timing replays when possible.
- The IR sweep page can stage large command sets in browser memory, import
  selected commands to the hub, and send them in cancellable batches.
- Bluetooth HID mode can expose the hub as a keyboard-class device and send
  keystroke scripts through the hub-side FIFO runtime.
- MQTT bridge publishes Home Assistant discovery and exposes hub/device state.
- Recovery helpers can start a local AP workflow from the reset button path.

## Repository Layout

```text
.
  install_webui.ps1        Windows installer for rooted hubs with SSH
  restore_backup.ps1       Restores the installer's hub-side backup
  payload/
    bin/                   MIPS binaries shipped to the hub
    scripts/               Init, recovery, Dropbear wrappers, cloud suppression
    mqtt/                  MQTT bridge Lua plugin
    source/                C sources for the native helper binaries
  tools/
    ir_database_smoke_test.mjs
  build/
    build_harmony_tools_kali.sh
  docs/
    AI_HANDOFF.md
    API.md
    BUILD.md
    GITHUB_SETUP.md
    SECURITY.md
  examples/
    mqtt-config.example.json
```

The shipped binaries target the Harmony Hub's MIPS big-endian Linux userspace.
No build server is required to install the current payload.

## Quick Install

Run from Windows PowerShell after the hub already has working root SSH:

```powershell
.\install_webui.ps1 -HubHost <hub-ip> -KeyPath "$env:USERPROFILE\.ssh\<root-key-file>"
```

The installer will prompt for missing values, create a backup on the hub, upload
the runtime, start Dropbear if needed, start the web UI, and write MQTT config
if provided.

Open the UI afterward:

```text
http://<hub-ip>:8080/
```

## Rollback

Restore the newest backup created by the installer:

```powershell
.\restore_backup.ps1 -HubHost <hub-ip> -KeyPath "$env:USERPROFILE\.ssh\<root-key-file>"
```

## Development Workflow

Keep changes scoped and reviewable:

1. Edit `payload/source/codex_webui.c` or the relevant payload script/plugin.
2. Rebuild MIPS binaries only when native source changes.
3. Replace the corresponding file under `payload/bin/`.
4. Update `payload/bin/MANIFEST.txt`.
5. Install to a test hub with `install_webui.ps1`.
6. Verify the dashboard, IR import, Bluetooth HID, MQTT, and rollback paths.

Do not commit local secrets, hub backups, firmware dumps, root tooling, or
credentials. See `docs/SECURITY.md` before sharing the repository.

For script and integration control, see `docs/API.md`.

## Useful Checks

Page check:

```powershell
Invoke-WebRequest -Uri "http://<hub-ip>:8080/" -UseBasicParsing
```

Process and checksum check:

```powershell
ssh -i "$env:USERPROFILE\.ssh\<root-key-file>" root@<hub-ip> "ps | grep -E '[c]odex_webui|[d]ropbear'; md5sum /data/codex/bin/codex_webui"
```

Logs:

```powershell
ssh -i "$env:USERPROFILE\.ssh\<root-key-file>" root@<hub-ip> "tail -80 /cache/codex-init.log; tail -80 /data/codex/ir-events.log 2>/dev/null"
```

IR database parser smoke test:

```powershell
node .\tools\ir_database_smoke_test.mjs --sample=24 --per-device=10 --source=all --dry-run
```

To create test devices and import supported commands without sending IR:

```powershell
node .\tools\ir_database_smoke_test.mjs --sample=8 --per-device=8 --source=all --configure --hub=http://<hub-ip>:8080
```
