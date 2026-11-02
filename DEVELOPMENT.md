# Developer setup (Windows x64)

These steps compile and exercise CPU tests only. They do not launch the EFI, write USB media, change firmware settings or reboot. Keep the original EFI and historical evidence separately; a rebuild writes new outputs.

## 1. Obtain the source and toolchain

Install Git, Python 3.11 or newer and `uv`, then use a new working directory:

```powershell
git clone https://github.com/ooxtcoo/lenovo-legion-83g0-gpu-unlock.git
cd lenovo-legion-83g0-gpu-unlock
python --version
uv --version
uv run --with ziglang==0.13.0 python -m ziglang version
```

The last command acquires the pinned Zig toolchain; it must report **0.13.0**. Build/test helpers use `uv --offline`, so complete this acquisition before running them. Python 3.11 was used in the preserved backup rehearsal. Bundled EDK2 headers under `deps/` need no separate checkout. Do not install a random newer Zig as a substitute.

## 2. Supply the exact local firmware assets

**The public clone is intentionally not a complete firmware build input.** It does not ship the firmware or offer an automatic download that establishes distribution permission. See [binary availability](BINARY-AVAILABILITY.md).

Obtain a locally authorized assets directory containing these exact filenames, sizes and SHA-256 values from `ASSET-PINS.json`:

- `booter_load_ad102.bin`
- `v67-ad102-610.43.03-four-plm.bin`
- `gsp_ga10x.bin`
- `gsp_boot_ad102.bin`
- `gsp_boot_desc_ad102.bin`

Then import (replace the example path):

```powershell
python import_assets.py "C:/path/to/locally-owned/assets"
```

The importer validates all five inputs before copying to `assets/`; it refuses missing/mismatched bytes. The owner's original local/USB archive contains the authentic inputs. `firmware-tools/` preserves upstream extraction/generation tools and notices for provenance; it is not an alternate turnkey auto-v2 build. `src/assets.json` is inherited GA102 metadata, **not** this build's active asset manifest. Never substitute other firmware or relax hashes.

## 3. Run the CPU test and build

From the repository directory:

```powershell
python test_classifier.py
python build_efi.py backup-build
python verify_image.py
```

- The classifier executes the real final-configuration classifier on the Windows host, not firmware. It does not need private dumps or firmware assets.
- The build verifies source hashes against the preserved closure and produces `build/Lenovo-AD103M-Unlock-v5-auto-v2.efi`. Use the **exact `backup-build` label** because `verify_image.py` reads `evidence/backup-build.json`.
- The image verifier checks PE layout, entry, imports, relocations and all five embedded payloads. Its relocation simulation never executes the EFI.
- `test_classifier.py` writes `evidence/backup-classifier.json`; build writes `evidence/backup-build*.json`; the image verifier **overwrites** `evidence/image-verification-auto-v2.json`. Use a disposable clone if preserving historical evidence. Do not commit local absolute-path logs or generated binaries.

Six unused C-option warnings for the assembly input occurred in the historical successful build; a nonzero command exit is still failure. If offline acquisition or a required asset is missing, fix that prerequisite rather than deleting a check.

A rebuilt PE may differ in its COFF timestamp. Do not claim raw identity with the original unless the entire SHA-256 matches; claim timestamp-only equivalence only after comparing every differing byte. Original size/hash are in [BOOTING.md](BOOTING.md) and the historical image report.

## What the evidence does and does not prove

`BACKUP-VERIFICATION.json` records a clean-extraction build, structure/payload verification, and **457,888** classifier assertions. The original development closure records **575** whole-application host cases plus four negative controls. Those use synthetic firmware callbacks, not a live UEFI boot.

The full historical application suite is **not self-contained** here: `run_app.py`, `replay.py` and `verify_auto_v2_closure.py` retain dependencies on intentionally omitted private PROM fixtures, old sibling trees and historical evidence. Do not fabricate replacements, delete assertions or present inherited results as a fresh run. The portable image verifier cannot rerun the private-ROM exclusion scan without omitted inputs.

The earlier manual build reached Windows on the development machine; **auto-v2 hardware execution, a plain FAT32 launch, persistent Windows-driver state and controlled performance gains remain unverified**. Image validation and CPU test success are not hardware safety approval.

## Runtime details and reports

The application embeds its five stock assets and logs only to the screen. Windows preload is separate. It requires `SecureBoot=0`, retains loaded-device/identity/ROM/ownership/recovery guards, and disables the UEFI watchdog before target admission. Known admitted values run without U/S confirmation; unfamiliar-value consent is exceptional, not a safety endorsement. See [README-AUTO-V2.md](README-AUTO-V2.md) for the exact boundary and test catalog.

For another machine, report exact IDs, firmware versions, build hash and the last stage/error before attempting source changes. Do not bypass guards to force admission. For performance comparisons keep model, quantization, engine version/options, prompt/context, cache state, power adapter/profile and temperatures matched; separate prompt processing from decode and repeat runs.

Retain all licenses and credits. Never upload private ROM/PROM dumps, inventories, credentials or recovery keys. `MANIFEST-SHA256.json` is the original backup manifest, not a promise that later documentation is byte-identical.
