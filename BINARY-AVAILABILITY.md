# Why there is no public EFI download yet

The source and documentation are public. The existing exact auto-v2 EFI is retained only as an **unpublished draft for the owner's authorized private storage**, not a public release. Repository publication is not permission to redistribute third-party firmware.

## Concrete unresolved permission

The EFI embeds five payloads, including NVIDIA `gsp_ga10x.bin`, the AD102 boot assets and a separately generated V67 image. Their exact hashes are in `ASSET-PINS.json` and `evidence/image-verification-auto-v2.json`; these are not private ROM dumps.

The bundled [NVIDIA Driver License Agreement](LICENSES/NVIDIA-Driver.txt), section **1.1(d)**, grants a conditional distribution right for software “provided for use with operating system kernels” under an OSI-approved open-source license, with unchanged binaries and the agreement supplied. Section **2.7** restricts distribution “Except as expressly granted in this Agreement”. The current NVIDIA primary text contains the same relevant conditions.[7]

We have **not established that this standalone UEFI/Windows handoff package and generated payload fall within that grant**, nor found a separate permission covering this package. The upstream [firmware notice](firmware-tools/NOTICE.md) likewise does not claim redistribution permission. GPL/source-code licenses do not by themselves license NVIDIA firmware. The owner's approval to make the project public does not resolve that third-party condition.

Therefore the EFI is **not published publicly**. This is a conservative publication boundary, not a definitive legal ruling or a claim that private storage grants additional distribution rights. Public binary publication needs an applicable documented grant or permission, with corresponding notices/source obligations satisfied. Do not publish the draft or mirror its assets until that is resolved.

GitHub documents draft releases as unpublished and lists them only for users with push access.[5] Private storage must not be confused with a public download link. The existing binary remains unchanged; no firmware or private dumps were added to Git history.

## Sources

[5] https://docs.github.com/en/rest/releases/releases
[7] https://www.nvidia.com/en-us/drivers/nvidia-license
