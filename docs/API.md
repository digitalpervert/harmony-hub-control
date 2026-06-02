# Local Control API

The web UI intentionally has no HTTP authentication. Run it only on a trusted
LAN or behind your own access controls.

All write endpoints accept `application/x-www-form-urlencoded` bodies. JSON
responses use `ok: true` on success and `ok: false` with `error` on failure.

## Inventory

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/inventory"
```

Returns hub limits, configured devices, command names, keycode/raw flags, and
local command counts.

## IR Control

Send one saved command:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/ir-send" -Method Post -Body @{
  deviceId = "<device-id>"
  command  = "PowerOff"
}
```

Send a cancellable batch:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/ir-batch-send" -Method Post -Body @{
  deviceId = "<device-id>"
  commands = "PowerOff`nInputHdmi1"
  delayMs  = "80"
  dryRun   = "0"
  runId    = "example-run-1"
}
```

Cancel a running batch:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/ir-cancel" -Method Post -Body @{
  runId = "example-run-1"
}
```

Create or reuse the temporary IR sweep target:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/ir-lab-target" -Method Post
```

Import commands from database-converted lines:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/irdb-import" -Method Post -Body @{
  deviceId = "<device-id>"
  payload  = "PowerOff|G:Toshiba 32 Bit:(0xE0E040BF)(Repeat)():3"
}
```

Raw timing imports use:

```text
CommandName|raw|F9470P20D0S1068...
```

## IR Learning

Capture from a remote:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/capture" -Method Post -Body @{
  timeout = "8"
}
```

Test a learned signal before saving:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/ir-test-learned" -Method Post -Body @{
  deviceId = "<device-id>"
  name     = "PowerToggle"
  mode     = "raw"
  raw      = "F9470P20D0S1068..."
}
```

## Bluetooth HID

Make the hub discoverable and pairable as a keyboard:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-call" -Method Post -Body @{
  action = "pairing_on"
  type   = "btkeyboard"
  name   = "Harmony Keyboard"
}
```

Check adapter and connection state:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-call" -Method Post -Body @{
  action = "adapter_status"
}
```

Check the FIFO keyboard runtime:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-text-status"
```

Send exact text through the keyboard FIFO runtime:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-text" -Method Post -Body @{
  text = "Hello World`n"
}
```

The installer starts `/data/codex/bin/codex_bthid_keyboard` automatically and
creates `/cache/bin/bthid_keyboard` as a friendly symlink. The runtime reads
`/tmp/bthid_input`, sends exact press/release reports for ASCII text, and uses
the paired target saved by the Bluetooth controls.

Send a named key or shortcut through the low-level HID report path:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-call" -Method Post -Body @{
  action = "report"
  type   = "btkeyboard"
  code   = "ctrl+l"
}
```

Send multiple named keys. Each key is encoded as a press and release pair:

```powershell
Invoke-RestMethod "http://<hub-ip>:8080/api/bt-call" -Method Post -Body @{
  action = "reportseq"
  type   = "btkeyboard"
  code   = "enter`nspace`nalt+f4"
  gapMs  = "35"
}
```

## Exports

```text
GET /export/bundle
GET /export/devices
GET /export/functions
GET /export/protocols
GET /export/mqtt
GET /export/wifi
```

Exports are for backups and debugging. Do not share files containing local
network or credential material.
