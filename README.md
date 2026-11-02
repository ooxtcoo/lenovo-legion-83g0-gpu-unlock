# Lenovo Legion RTX 4090 Laptop GPU Unlock

Experimental UEFI GPU-unlock port for the **Lenovo Legion 9 16IRX9 / Legion 9i Gen 9 (83G0)**, followed by a Windows handoff. This repository shares the source, build instructions and research so other owners do not have to repeat the investigation.

> **Launching auto-v2 can immediately run the GPU transaction and start Windows, without confirmation. It can hang or require physical power-off. Other devices are unverified. Do not bypass a failed check.** This is not an official Lenovo/NVIDIA tool, a BIOS/VBIOS flasher, or a guaranteed performance upgrade.

## Hardware and test status

The only development target is:

- **Machine:** Lenovo `83G0`, BIOS `NXCN26WW`
- **GPU:** RTX 4090 Laptop / AD103M, PCI `10DE:2757`, subsystem `17AA:3D34`
- **VBIOS:** `95.03.40.00.11`

The earlier **manual** implementation reached the expected register state and the owner confirmed Windows boot on this machine. **This auto-v2 build has host-test and image-verification evidence, not a confirmed hardware boot.** Neither the model name nor the banner authenticates the BIOS or establishes compatibility with another laptop.

## Performance observation

The owner reports about **38 → 48+ tokens/s in Strata**. This was not a controlled benchmark; power-adapter conditions also changed. It is not proof of a persistent unlock under the Windows driver, nor a promised speedup.

## Get started

- **[Boot instructions: Ventoy or a normal USB stick](BOOTING.md)** — for an appropriately obtained exact EFI.
- **[Development setup and build](DEVELOPMENT.md)** — toolchain, required assets and tests.
- **[Report a result](../../issues/new?template=hardware-report.md)** — include exact hardware, build and outcome; no private dumps.

**EFI download:** no public binary is offered yet. The EFI embeds NVIDIA firmware; public redistribution permission for this UEFI/Windows package has not been established. The existing owner's binary is retained only in an unpublished draft. [Details and license references](BINARY-AVAILABILITY.md). The public source does not include the five firmware build inputs.

## Research and licensing

Historical details remain in [README-AUTO-V2.md](README-AUTO-V2.md), [START_HERE.md](START_HERE.md) and `evidence/`. They are not new hardware results. `MANIFEST-SHA256.json` covers the original backup snapshot, not later documentation changes.

Built on upstream GPU-unlock research and vendor/UEFI components. Preserve [upstream credits](UPSTREAM-THIRD_PARTY_NOTICES.md), [LICENSE](LICENSE), [LICENSE-POLICY.md](LICENSE-POLICY.md), `UPSTREAM-LICENSE` and `LICENSES/`. Code licenses do not relicense embedded firmware. No endorsement by Lenovo, NVIDIA or Strata is implied.
