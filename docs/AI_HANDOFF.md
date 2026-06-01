# AI Handoff

You are working on the Harmony Hub Control post-root runtime.

## Goal

Improve and maintain the local web UI and supporting hub services for an
already rooted Logitech Harmony Hub.

## Important Boundaries

- This repository is not the rooting tool.
- Do not add LAN or USB rooting material here.
- Do not include private keys, tokens, MQTT passwords, Home Assistant tokens,
  firmware dumps, or hub backups.
- The web UI intentionally has no HTTP authentication right now. Treat it as a
  trusted-LAN-only tool.
- The installer expects root SSH to already work.

## Main Files

- `payload/source/codex_webui.c`: single-binary web server and front-end assets.
- `payload/mqtt/codexmqtt.lua`: Home Assistant MQTT bridge.
- `payload/scripts/init.sh`: hub boot startup for local services.
- `payload/scripts/recovery_ap.sh`: reset-button recovery AP flow.
- `payload/scripts/netservicestarter.lua`: cloud-suppressed local service
  starter.
- `install_webui.ps1`: Windows SSH uploader/installer.
- `restore_backup.ps1`: rollback helper.

## Runtime Paths On Hub

```text
/data/codex/bin/codex_webui
/data/codex/bin/codex_hbus
/data/codex/bin/codex_hal_ltcp
/data/codex/bin/codex_dhcpd
/data/codex/bin/codex_portal
/data/codex/init.sh
/data/codex/recovery_ap.sh
/data/codex/hub_id
/data/codexmqtt/config.json
/pkg/codexmqtt/codexmqtt.lua
/usr/sbin/dropbear
/usr/sbin/dropbearkey
/etc/init.d/rcS.local
```

## Current Feature Notes

- IR import sources should stay separate in the UI, with an `All databases`
  option.
- Unsupported IR database rows should be visible with enough detail to debug
  parser coverage.
- Flipper parsed protocols currently include `RC5`, `RC6`, `SIRC`, `SIRC15`,
  and `SIRC20` conversion to raw timing.
- Learned IR signals should be testable before saving.
- The IR sweep page should favor fast staging in browser memory and hub-side
  batch sends that can be stopped.
- Bluetooth HID keystroke accuracy matters. Prefer FIFO writes that emit
  complete press/release frames per key and avoid long key-held repeats.
- MQTT should publish enough state for Home Assistant debugging, including IP
  address and bridge health.

## Verification Checklist

After changing web UI or runtime behavior:

1. Deploy with `install_webui.ps1`.
2. Open `http://<hub-ip>:8080/`.
3. Check Dashboard, IR Devices, IR Sweep, Bluetooth, MQTT, Wi-Fi, Backup, and
   System sections.
4. Confirm no browser auth prompt appears.
5. Import a small IR database file and verify supported/unsupported counts.
6. Send one known-good IR command.
7. If Bluetooth changed, pair and send a short exact text script.
8. If MQTT changed, verify discovery and state topics in Home Assistant.
9. Tail `/cache/codex-init.log` and `/data/codex/ir-events.log`.
10. Confirm rollback can find the newest backup.
