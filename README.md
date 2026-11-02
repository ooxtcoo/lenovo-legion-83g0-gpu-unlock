# Lenovo Legion RTX 4090 GPU Unlock

An experimental UEFI GPU unlock for a specific Lenovo Legion laptop, with automatic Windows handoff. This project shares the work so other owners and developers do not have to start from scratch.

The owner reports **38 → 48+ tokens/s in Strata**. This is an encouraging observation, not a controlled benchmark: power-adapter and other test conditions were not fully matched. Your results may differ.

## 1. Use and test

### Check your laptop first

This is the configuration investigated during development—not a compatibility list for all Lenovo RTX 4090 laptops:

| Component | Development machine |
| --- | --- |
| Laptop | Lenovo Legion 9 16IRX9 / Legion 9i Gen 9 |
| Machine type | **83G0** |
| BIOS | **NXCN26WW** |
| GPU | **RTX 4090 Laptop / AD103M** |
| GPU PCI / subsystem IDs | `10DE:2757` / `17AA:3D34` |
| VBIOS | **95.03.40.00.11** |

Other configurations are unverified. The earlier manual version successfully handed off to Windows on the development laptop; auto-v2 has host-test evidence but no separately confirmed hardware boot in the recorded results.

> **Important:** Starting the EFI starts an automatic hardware attempt when its checks pass. It is not a read-only test. It may hang and require a full power-off. Do not bypass failed checks, even on an apparently identical laptop.

**Download status:** The EFI is uploaded as an unpublished release draft, so there is currently no public binary download. The instructions below apply if you already have the file. See [binary availability](BINARY-AVAILABILITY.md) for the unresolved distribution issue.

### Prepare

- Back up important data, connect your power adapter and keep your BitLocker/device-encryption recovery key available on another device if encryption is enabled.
- This build requires **Secure Boot disabled**. Do not delete Secure Boot keys or reset the TPM. If you are unsure about changing this setting, stop here.
- The only runtime file needed is **`Lenovo-AD103M-Unlock-v5-auto-v2.efi`**. No extra asset folder or ISO is needed.

### Option A — Ventoy USB stick

1. If you do not have a Ventoy stick, follow the official [Ventoy installation guide](https://www.ventoy.net/en/doc_start.html). **Installing Ventoy erases the selected stick: check the drive carefully.**
2. Copy the EFI onto the stick's main data partition, alongside your ISOs. Leave the small **VTOYEFI** partition alone.
3. Restart the laptop and open its boot menu with **F12**. Select the USB's UEFI entry.
4. In Ventoy, select the EFI. **Selecting it starts the attempt.** If all checks and recovery succeed, it attempts to start Windows automatically.

### Option B — Ordinary FAT32 USB stick

1. Use a separate, empty FAT32-formatted USB stick. Formatting erases its contents; double-check the selected drive.
2. Create the folders `EFI/BOOT` on that stick.
3. Copy the EFI there and rename **the copy** to `BOOTX64.EFI`. The result must be `EFI/BOOT/BOOTX64.EFI`.
4. Restart and select the USB's UEFI entry from **F12**. **The attempt can start immediately.** This standard USB layout has not been hardware-tested with this build.

Never copy it to your laptop's internal EFI partition or replace an existing bootloader.

### If it does not work

- For an unfamiliar-value prompt, cancel with **Q, Esc or Space** rather than guessing.
- At **STOP**, or if it hangs, fully power off the laptop. Do not force Windows handoff or bypass checks. Remove the stick while powered off to boot normally next time.
- Photograph the error if possible and [report your result](https://github.com/ooxtcoo/lenovo-legion-83g0-gpu-unlock/issues/new/choose), including model, BIOS, VBIOS, build and whether Windows started. Do not upload private dumps or recovery keys.

## 2. Develop further

### Set up on Windows x64

Install **Git, Python 3.11+ and uv**, then use a fresh clone:

```powershell
git clone https://github.com/ooxtcoo/lenovo-legion-83g0-gpu-unlock.git
cd lenovo-legion-83g0-gpu-unlock
uv run --with ziglang==0.13.0 python -m ziglang version
```

The last command must report `0.13.0`. EDK2 headers are included in `deps/`.

### Import assets, test and build

The five required firmware files are **not included in Git**. Obtain an appropriately licensed local copy matching `ASSET-PINS.json`, then replace the example path:

```powershell
python import_assets.py "C:/path/to/local/assets"
python test_classifier.py
python build_efi.py backup-build
python verify_image.py
```

The importer checks exact sizes and SHA-256 hashes. The output is `build/Lenovo-AD103M-Unlock-v5-auto-v2.efi`. Keep the label **`backup-build`**: the image verifier expects that log name. These commands do not boot the EFI or modify GPU hardware.

The standalone classifier can run without firmware assets. The complete historical application tests require omitted private fixtures and older development trees; they are not fully reproducible from this clone. A successful CPU test or build is not a hardware-safety guarantee.

For code changes, start with `src/`, [START_HERE.md](START_HERE.md) and [README-AUTO-V2.md](README-AUTO-V2.md). Preserve device/recovery checks, add evidence for changed behavior, and keep license and attribution notices. Do not remove validation gates just to make another device pass.
