# Boot from USB

**For the exact [83G0 configuration](README.md#hardware-and-test-status) only. Other devices are unverified. Launching this EFI can run the unlock and start Windows automatically, without asking again.** Both routes below are documented preparation methods, not hardware validation of auto-v2. The normal FAT32-stick route is hardware-unverified.

There is currently **[no public EFI download](BINARY-AVAILABILITY.md)**. These steps assume you already have the appropriately obtained `Lenovo-AD103M-Unlock-v5-auto-v2.efi`; developers can use [DEVELOPMENT.md](DEVELOPMENT.md).

## Before starting

1. Back up important files. If BitLocker/device encryption is active, have your [recovery key](https://support.microsoft.com/en-us/windows/find-your-bitlocker-recovery-key-6b71ad27-0b89-ea08-f143-056f5ab347d6) accessible on another device; boot/security changes can trigger recovery. If unsure, stop.[6]
2. Keep reliable external power connected. Confirm the machine, BIOS and GPU identifiers against the README yourself.
3. **Secure Boot must be off (`SecureBoot=0`).** If enabled, review recovery arrangements before deliberately changing it in firmware setup. Do not clear Secure Boot keys or the TPM. Ventoy's own Secure Boot support does not bypass this requirement.
4. Use an expendable **external USB stick**. Installation/formatting erases the chosen stick: back it up and verify its identity and capacity. **Never use the internal ESP, replace Windows boot files or add boot entries.**

## Option A — Ventoy

1. [Download Ventoy for Windows](https://www.ventoy.net/en/download.html), extract it, and open `Ventoy2Disk.exe`. Select your expendable USB and choose **Install**, following the [official instructions](https://www.ventoy.net/en/doc_start.html). **Double-check the selected drive before confirming erasure.** If you already have a working Ventoy stick, skip installation.[1][3]
2. Copy `Lenovo-AD103M-Unlock-v5-auto-v2.efi` onto the stick's **large main data partition**, for example in a `Lenovo83G0` folder. Leave the small **VTOYEFI** partition and Ventoy's boot files untouched. No ISO or extra asset folder is needed; Ventoy supports direct EFI files.[1][2]
3. Verify the copied file using the optional checksum section below, then safely eject it.
4. Connect the stick, power on/restart the Lenovo, press **F12**, and choose the USB's **UEFI** boot entry. In Ventoy select the EFI **only when ready to authorize the attempt**.[4]

## Option B — separate FAT32 USB

1. Use a **different empty stick**, not your Ventoy or backup drive. In Windows File Explorer, right-click that USB → **Format** → **FAT32** → **Start**, only after checking the correct drive and backing it up. If FAT32 is unavailable, use an already prepared FAT32 stick; do not select an internal partition instead.
2. Create an `EFI` folder on the USB, and a `BOOT` folder inside it.
3. Copy the EFI there and rename **the copy** to `BOOTX64.EFI`. Keep the original download unchanged. The final path must be **`USB:\EFI\BOOT\BOOTX64.EFI`**. Do not overwrite an existing bootloader; choose another empty stick.[8]
4. Verify the copied file, safely eject, then reconnect it. Power on/restart and press **F12** → select the USB's **UEFI** entry. **This can launch the executor immediately**, without a Ventoy menu.[4]

## If something goes wrong

- If an unfamiliar-value prompt appears, cancel with **Q, Esc or Space** rather than guessing. A guard refusal is not permission to bypass it. Different launcher device paths can also be refused.
- On **STOP**, an unsafe state or a hang, use the **physical power button to shut down completely**. Do not force Windows startup or retry in the same session. Remove the experimental stick while powered off before a normal startup.
- Photograph visible errors first if possible; no report file is saved. Record the last stage/error in a [hardware report](../../issues/new?template=hardware-report.md).

<details>
<summary>Optional: verify the exact original EFI (PowerShell)</summary>

From the download folder:

```powershell
Get-FileHash -LiteralPath '.\Lenovo-AD103M-Unlock-v5-auto-v2.efi' -Algorithm SHA256
(Get-Item -LiteralPath '.\Lenovo-AD103M-Unlock-v5-auto-v2.efi').Length
```

Repeat with the actual USB path (for Option B, `X:\EFI\BOOT\BOOTX64.EFI`, replacing `X:` with the USB drive letter). Stop on a mismatch.[9]

Original size: **84,570,112 bytes**. Original SHA-256:

```text
3cb4a2725b953ec6c99ea7ac1c179a004ba34ee2f568a569952b5c5ccbd59faa
```

A local rebuild can have a different hash; follow the developer verification steps. A matching hash proves identity, not safety.

</details>

<details>
<summary>Primary references (checked November 2, 2026)</summary>

General boot mechanics are source-checked; this is not a live boot test. Application behavior is documented in `README-AUTO-V2.md` and the unchanged application source.

## Sources

[1] https://www.ventoy.net/en/doc_start.html
[2] https://www.ventoy.net/en/index.html
[3] https://www.ventoy.net/en/download.html
[4] https://download.lenovo.com/pccbbs/pubs/legion_9_16_irx9/user_guide/en/help_bios.html
[6] https://support.microsoft.com/en-us/windows/find-your-bitlocker-recovery-key-6b71ad27-0b89-ea08-f143-056f5ab347d6
[8] https://raw.githubusercontent.com/tianocore/edk2/c1415771ac592ad8509351c46cd3cda4f38d7d52/MdePkg/Include/Uefi/UefiSpec.h
[9] https://learn.microsoft.com/en-us/powershell/module/Microsoft.PowerShell.Utility/Get-FileHash

</details>
