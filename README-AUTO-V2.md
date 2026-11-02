# Lenovo AD103M v5-auto-v2 — experimental SS0 admission

## Built artifact; no hardware execution or USB deployment

`build/Lenovo-AD103M-Unlock-v5-auto-v2.efi`

- Size: **84,570,112 bytes**.
- SHA256: `3cb4a2725b953ec6c99ea7ac1c179a004ba34ee2f568a569952b5c5ccbd59faa`.
- `build/Lenovo-AD103M-Unlock-v5-auto-v2-VENTOY.efi` is byte-identical; no padding was needed.
- Unsigned x86-64 PE32+ EFI application, actual entry EfiMain. Compiled and structurally verified only. The Windows host fixture executes actual application/core C with synthetic firmware callbacks, not the EFI on hardware.
- Separate sibling of immutable `ad103m-uefi-port-candidate-v5-auto-v1`; all 826 files in that directory match the captured whole-tree hash manifest. Baseline EFI SHA256 remains `6f467dfcbfd513ba46ccd791616f74b1d25ae5aa6e73f9291fa01933e23689f0`.

## Admission and keys

Launching authorizes one automatic attempt **only when all guards pass** and SS0 is exactly **0x53422157 or 0x53402157**, with SS1 exactly 6. These two observed values need neither U nor S nor a working input interface. There is no success photo pause and no retry.

An unfamiliar SS0 is **not another automatic value or a safety-approved whitelist entry**. If the complete initial cold check, target/device, graph/ownership, BAR, ROM/PROM, assets and Windows preload succeed, a visible warning displays the exact unfamiliar SS0 and asks whether to continue. Only a fresh **C/c** after quiet-input sampling and post-prompt queue draining grants one experimental exception for that exact observed SS0 during this invocation. **Q, Esc or Space cancels before detach.** U, S, Enter and contradictory scan-code+C do not authorize it. No key or only stale C never accepts automatically: after the bounded consent polls, execution refuses and enters the existing exit-only refusal handling.

**Zero, all-ones, unavailable/failed/warning reads and wrong SS1 are never overridable.** No other predicate, error, warning or Gu.fault is ignored. A malformed/missing input interface, input/output warning or error, stuck queue, or failed Stall prevents exceptional admission. No input-device reset is added.

Post-consent configuration/display and complete cold checks remain mandatory. Exceptional SS0 must equal the initially displayed value, including after driver release; changing even to one of the two automatic values invalidates that exception. Immediately after the last owner graph check, SS0/SS1 are reread without another console/input callback before detach. A changed value or read fault refuses before detach; changes detected after release are terminal, not a new confirmation opportunity. The permission resets on every EfiMain invocation. This is an observed-value check, not an atomic hardware lock: firmware/device changes between reads cannot be ruled out.

**Continuing with an unfamiliar SS0 is explicitly experimental. Its hardware meaning and transaction compatibility are unknown. Consent is not proof of safety.** The exception can still hang or leave an unsafe state. It exists only at the user's explicitly requested SS0 boundary; it does not make any other STOP resumable.

## Preserved boundaries

Intended experimental target remains Lenovo 83G0 / NXCN26WW; confirm BIOS yourself. The banner does not authenticate BIOS. There are no added hardware writes, retries, power/TDP/clock/voltage/VBIOS/security-key/boot-configuration changes. Existing identity/BDF/config, loaded-device path, SecureBoot, ROM/PROM, embedded-payload, ownership/display, DMA, containment and recovery guards remain.

Transaction/port/ownership/core, recovery/handoff and final PCI classifier files are byte-identical to auto-v1. The application tail from the original `EFI_PCI_IO_PROTOCOL *prior` through EfiMain return is byte-identical. The fresh final graph plus full original PCI256 classifier still validate both capability chains and permit only the already-established narrow clearing of previously set PCIe Device Status CED/URD. No added status-register acknowledgement, broad mask, original-snapshot rebase or final-check omission. No output/input callback occurs after the last PCI comparison before StartImage.

Any unsafe post-release outcome or returned StartImage stays terminal: physical POWER OFF, not retry or launcher return. Successful Windows handoff can erase transient results immediately. Logging remains screen-only; no report-file writes. Five stock assets remain embedded; no runtime asset directory or private ROM is embedded. Windows filesystem discovery/preload is separate and unchanged.

The inherited watchdog call **SetWatchdogTimer(0,0,0,NULL)** still runs after SecureBoot and before target admission. It is not a new policy change. Existing FLR/recovery still occurs. Poll limits do not interrupt hanging firmware calls; SimpleTextIn cannot prove physical key release. PE image memory is **84,578,304 bytes**, plus asset copies, DMA, Windows and launcher overhead. Large-image loader behavior, this build's actual firmware execution and performance gains remain unproven. The earlier manual Windows boot is not automatic-v2 safety or performance evidence.

## Executed verification

- Actual whole-EfiMain Windows fixture: **575 distinct cases, IDs 0–574, PASS**, in 20 completed batches of at most 30 cases, each command given a 600-second foreground budget. Source hashes, exact enumerated case lists and raw traces are bound in `evidence/code-closure-auto-v2.json`.
- **92 added cases, IDs 483–574**. Catalog: `evidence/new-case-catalog.json`. The existing case 473 now expects the explicitly requested second observed value to pass; other legacy guard assertions are retained. Coverage includes both automatic values/no keys, fresh C/c, no/stale/stuck/typematic input, Q/Esc/Space, interface/console/input/Stall errors and warnings, zero/all-ones/wrong SS1, all 14 cold-read error/warning positions with unfamiliar SS0, ROM/PROM/assets, post-C config/display/fault/value changes, last pre-detach reread faults/changes, and terminal post-release changes.
- First real EfiMain RED for 0x53402157, followed by actual GREEN, retained as `ss0-second-red.json` / `ss0-second-green.json`. Unfamiliar-value C RED/GREEN retained separately.
- **Four final negative controls** compiled and failed the intended runtime assertions: frozen auto-v1 against the final second-value test; omitted fresh-C consent; omitted final SS0 reread; and removed zero-sentinel rejection. Their separate host executables never overwrite the final tested executable.
- **43 unchanged core cases and existing classifier evidence reused**, with source and evidence hashes verified. These unchanged standalone suites were not rerun or counted as new executions.
- PE machine/subsystem, entry mapping, bounded/nonoverlapping sections, no W+X sections, no imports/signature, 70 DIR64 relocations including all ten embedded pointer fields, offline relocation simulation and all five extracted authentic payload hashes verified. Private original ROM and host PROM fixture blobs are absent.
- Minimal complete source/test diff: `evidence/auto-v2-vs-auto-v1.patch`. Only app.c, app_io.c (label), cold_report.inc, lifecycle.c/h and v5_app.c differ.

### Preserved fixture/compiler issues

The first exceptional GREEN attempt reached StartImage but hit the old fixture's blanket no-key assertion. The assertion was narrowed only for the newly explicit C path; that failed run remains `exceptional-green.json`, not counted as PASS. The final-pre-detach read-failure fixture uses the **third SS0 read**, not aggregate MMIO call count, because PROM reads increase that aggregate between cold passes. The final full matrix passed after this correction.

The delivered EFI compiler exited zero with six unused C-option warnings for the assembly input. No C compiler errors occurred. These build-time diagnostics are distinct from firmware warning statuses, which remain rejected.

## Local reproduction (compile/test only; do not execute EFI)

1. `python verify_auto_v2_closure.py`
2. `python build_efi.py <new-evidence-label>`
3. `python verify_image.py`

Image verification binds the delivered build log `auto-v2-efi.json`; preserve delivered bytes/logs before rebuilding. Build uses cached ziglang 0.13.0 and existing pinned sibling EDK2 headers. Inherited assembly references immutable keyexit-v1 asset paths; closure verifies those files equal local assets and the PE extractor verifies actual linked payloads. Host reproduction also needs the private local fixture, which is not a runtime delivery file.

No firmware boot, USB/removable-media write, hardware benchmark, deployment or independent reviewer approval was performed by this task.
