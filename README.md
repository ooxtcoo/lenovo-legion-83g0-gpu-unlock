# Lenovo Legion RTX 4090 Laptop GPU Unlock

Experimental UEFI research port for the **Lenovo Legion 9 16IRX9 / Legion 9i Gen 9 (83G0)** with an **RTX 4090 Laptop GPU (AD103M)**, followed by a Windows handoff.

This repository preserves the **auto-v2 source**, build helpers and test evidence. It is not a general-purpose unlocker for every Lenovo RTX 4090 laptop, an official Lenovo/NVIDIA tool, or a ready-to-run binary release.

> **Hardware risk:** This application performs a GPU firmware transaction, driver detach/reconnect and function-level reset before attempting to start Windows. It can hang or leave the GPU in an unsafe state requiring a physical power-off. Host tests are not proof of hardware safety. Do not bypass identity or recovery checks to make another laptop pass.

## Why this exists

The project adapts GPU-unlock research to a specific Lenovo laptop and a Windows workflow. Development progressed through read-only diagnostics, ownership/ROM inspection, reconnect tests, a manual unlock/handoff implementation and finally an automatic variant.

On the development machine, the earlier manual implementation reached the expected SS0/SS1 values after reset and graphics-driver reconnect; the owner then confirmed arrival in Windows. These observations are separate from proving that the state persists under the Windows driver or that auto-v2 is safe on other hardware.

## Reported Strata performance

The owner reports **approximately 38 tokens/s before, versus 48 tokens/s or more afterward** in Strata on the development laptop.

This is an encouraging **user observation, not a controlled benchmark or a guaranteed speedup**. The report does not yet include a reproducible model/quantization, context length, engine configuration and repeated matched runs. Earlier testing also involved a change to a stronger power adapter, so power conditions are a potential confounder. The gain cannot currently be attributed exclusively to the unlock.

For a useful comparison, keep the model, quantization, engine version/options, prompt/context, cache state, power adapter, power profile and thermal conditions fixed. Report prompt processing and token generation separately, with repeated baseline and unlocked runs. Do not infer a universal LLM speedup from a register match.

## Hardware scope

| Item | Development machine |
| --- | --- |
| Laptop | Lenovo Legion 9 16IRX9 / 9i Gen 9 |
| Machine type | `83G0` |
| BIOS | `NXCN26WW` |
| GPU | NVIDIA RTX 4090 Laptop, AD103M |
| PCI vendor/device | `10DE:2757` |
| Subsystem vendor/device | `17AA:3D34` |
| VBIOS | `95.03.40.00.11` |
| Reported VRAM | 16,376 MiB |

**Other Lenovo models, BIOS/VBIOS versions and RTX 4090 laptops are unverified.** Even a matching marketing name or GPU device ID is insufficient. The intended BIOS is documented, but the application banner does not authenticate the BIOS version. Existing device, ROM/PROM, ownership and configuration guards are part of the port, not obstacles to remove.

## What auto-v2 does

Starting the EFI authorizes one attempt when all admission checks pass:

- Cold SS0 must be exactly `0x53422157` or `0x53402157`, with SS1 exactly `0x00000006`, for automatic admission.
- An unfamiliar SS0 can prompt for a **fresh C/c** to authorize an experimental exception for that exact observed value and that invocation only. **Q, Esc or Space cancels.** No response does not authorize execution.
- Zero, all-ones, failed/warning reads, wrong SS1 and other guard failures cannot be overridden.
- A successful path attempts Windows handoff without the earlier U/S confirmations or success-photo pause.
- Unsafe post-release outcomes remain terminal. There is no blanket “continue despite errors” option and no automatic retry.

The two accepted SS0 values were observed on the development machine; their acceptance is not a proof of equivalent hardware meaning. Confirming an unfamiliar value is particularly risky and does not establish compatibility.

The implementation requires a successful `SecureBoot=0` check and disables the UEFI watchdog during its existing flow. It is **not a BIOS/VBIOS flash or a TDP/voltage tuning tool**. It is not an Advanced BIOS unlock package. Do not clear security keys or TPM data to try to satisfy its checks. Review your recovery arrangements before considering firmware experiments.

## Source and build

**Start with [START_HERE.md](START_HERE.md).** It describes the portable backup layout and the limitations of inherited scripts.

Requirements for the documented Windows x64 build:

- Python 3.11 or newer;
- `uv` and pinned `ziglang==0.13.0`;
- the five exact firmware assets listed in `ASSET-PINS.json`;
- bundled EDK2 headers and their license notices.

**Firmware assets and built EFI binaries are intentionally not in Git.** Their redistribution permission has not been established. A clone alone is therefore not a complete firmware build input. The owner's local/USB backup retains the authentic assets; the importer validates their exact sizes and hashes. Private ROM dumps are not required for compilation and must not be uploaded.

From the repository directory, using an appropriately obtained local assets directory:

```sh
uv run --with ziglang==0.13.0 python -m ziglang version
python import_assets.py "PATH/TO/LOCAL/ASSETS"
python test_classifier.py
python build_efi.py backup-build
python verify_image.py
```

These commands compile and test; **they do not launch the EFI**. Do not treat a successful build as permission to boot it on an unverified laptop.

The original auto-v2 EFI fingerprint is:

```text
SHA-256: 3cb4a2725b953ec6c99ea7ac1c179a004ba34ee2f568a569952b5c5ccbd59faa
```

This is an integrity reference, not a safety certificate. Rebuilding can change the PE/COFF timestamp and therefore the file hash.

## Verification status

- Original auto-v2 development recorded **575 whole-application host cases** and four negative controls. These exercise C code with synthetic firmware callbacks, not real UEFI hardware.
- The backup rehearsal built the EFI from a clean extraction and verified its structure and embedded payloads. That rebuild differed from the original only in COFF timestamp bytes.
- The standalone final-configuration classifier passed **457,888 assertions** during the backup rehearsal.
- The entire historical application suite is **not self-contained in this sanitized repository**: private fixtures, old sibling baselines and extensive historical caches were deliberately excluded. Its 575 cases were not rerun during packaging.
- A direct Windows-side read proving persistence of the unlock state and a controlled performance comparison remain outstanding.
- The earlier manual hardware success must not be presented as hardware validation of every later build or another laptop.

`README-AUTO-V2.md` and the selected files in `evidence/` are historical development records. Some original paths and commands refer to the earlier development tree; use `START_HERE.md` for this repository's portable build. `MANIFEST-SHA256.json` describes the original backup snapshot, not subsequent documentation commits.

## Repository map

| Path | Purpose |
| --- | --- |
| `src/` | Application, lifecycle, checks, transaction port and host-test source |
| `deps/` | Included header dependencies |
| `build_efi.py` | Portable compile-only build helper |
| `import_assets.py`, `ASSET-PINS.json` | Exact local firmware-asset import and pins |
| `test_classifier.py` | Standalone CPU classifier test |
| `verify_image.py` | Built-image structure and payload verification |
| `firmware-tools/` | Preserved upstream asset tooling and notices |
| `evidence/`, `provenance/` | Selected historical evidence and original helpers |
| `START_HERE.md` | Backup/build instructions and omissions |

## Contributing useful observations

Reports from other owners can help establish the actual compatibility boundary. Include the exact laptop model/machine type, BIOS, GPU PCI/subsystem IDs, VBIOS, Windows/NVIDIA driver version, the specific source revision/build hash and the last visible stage/error. Clearly separate a diagnostic refusal, a successful reconnect, a register match, a Windows boot and a performance result.

Redact serial numbers, account information and other private details. **Do not post ROM/PROM dumps, credentials or firmware payloads.** If a guard refuses the device, report the refusal rather than changing constants to force admission. A different model requires its own investigation, not an assumed-compatible binary.

## Credits and licensing

This port builds on prior GPU-unlock research and vendor/UEFI components. It does not claim original authorship of all included code. Preserve attribution and review `UPSTREAM-THIRD_PARTY_NOTICES.md`, `UPSTREAM-LICENSE`, `LICENSE`, `LICENSE-POLICY.md` and `LICENSES/` before reusing or distributing files. Repository code licenses must not be interpreted as permission to redistribute omitted firmware assets.

No affiliation with or endorsement by Lenovo, NVIDIA or Strata is implied.
