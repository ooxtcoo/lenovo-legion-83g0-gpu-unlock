# Lenovo GPU Unlock auto-v2 — source backup

This preserves the named latest local auto-v2 source, not a hardware-tested or safety-certified release. Do not launch the EFI as a diagnostic: it is an experimental executor. No hardware execution, boot-variable changes or remote publication are part of this backup.

## Offline compilation and CPU test (Windows x64)

Requirements: Python 3.11+; `uv`; Zig 0.13.0 supplied by its pinned Python package. EDK2 headers are included under `deps/`, with their license. Python 3.11 and Zig 0.13.0 are used in the backup rehearsal.

1. One-time toolchain acquisition if not cached: `uv run --with ziglang==0.13.0 python -m ziglang version`.
2. The local USB source archive contains all five genuine build assets under `assets/`. The separate Git-ready copy deliberately omits firmware binaries and built EFI: upstream's firmware notice does not claim public redistribution permission. From a locally-owned copy, run `python import_assets.py <assets-directory>`; exact size/SHA-256 pins are enforced.
3. `python test_classifier.py` — CPU-only actual final-config classifier.
4. `python build_efi.py backup-build` — compile only. Never launches the resulting EFI.
5. `python verify_image.py` — PE sections, relocations and five embedded payload hashes, with an offline relocation simulation.

The existing build's exact SHA-256 is `3cb4a2725b953ec6c99ea7ac1c179a004ba34ee2f568a569952b5c5ccbd59faa`; it is backed up separately, not committed. A rebuilt PE may differ in its COFF timestamp; only report timestamp-only equivalence after comparing every byte.

## Preservation and changes

All 60 files below `src/` are byte-for-byte copies, including all tests and original guards. Portable `build_efi.py` uses included dependency roots; `generated/` uses relative local assets. `verify_image.py` uses `ASSET-PINS.json` and the current backup-build log. The original helper scripts are preserved in `provenance/`. No application C code or guard is changed.

`README-AUTO-V2.md` and selected `evidence/` are inherited historical records, NOT evidence that this backup reran 575 whole-application cases. `verify_auto_v2_closure.py`, `run_app.py`, and `replay.py` retain the legacy sibling topology and are not self-contained public test entry points. They require the deliberately excluded private PROM fixture, original auto-v1 baseline, old source/evidence snapshots, and other sibling trees. Do not fabricate substitutes or remove their assertions. The full historical closure cannot be recreated from this sanitized package. The included standalone classifier does not need private dumps.

`src/assets.json` is inherited legacy GA102 metadata; it is not the active auto-v2 payload manifest. Use `ASSET-PINS.json`, generated assembly, and actual embedded verification.

## Omitted intentionally

Private ROM/PROM dumps and machine inventories; credentials and environment files; VCS history; host executables/PDBs; duplicate EFI/VENTOY copies; extensive old traces, source snapshots and negative-build caches; historical sibling trees. No private ROM bytes are needed for compilation. The original image report's private-byte exclusion scan is retained, but not rerun by the portable image verifier without those private inputs. Git-ready staging additionally excludes all five firmware assets pending distribution review; local backup retains authentic bytes and license notices.

No new hardware success, performance gain, firmware boot or independent safety approval is claimed. See backup verification report for commands actually executed and current results.
