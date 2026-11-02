# Verwenden und testen

**Nur untersucht:** Lenovo **83G0**, BIOS **NXCN26WW**, RTX 4090 Laptop / AD103M (`10DE:2757`, `17AA:3D34`), VBIOS `95.03.40.00.11`. Andere Hardware ist ungeprüft; auch auto-v2 ist noch nicht durch einen bestätigten Hardwarestart validiert.

**Die EFI ist derzeit nur ein unveröffentlichter Entwurf, kein öffentlicher Download.** Diese Anleitung setzt eine bereits rechtmäßig erhaltene `Lenovo-AD103M-Unlock-v5-auto-v2.efi` voraus.

1. **Vorbereiten:** Wichtige Daten sichern, bei Geräteverschlüsselung den BitLocker-Wiederherstellungsschlüssel auf einem anderen Gerät bereithalten und Netzteil anschließen. Gerätekennung und Firmwareversionen prüfen; bei Unsicherheit nicht starten.
2. **Secure Boot prüfen:** Erforderlich ist `SecureBoot=0`. Eine Änderung nur bewusst und mit gesichertem Wiederherstellungsschlüssel vornehmen. **Keine Secure-Boot-Schlüssel löschen, TPM nicht zurücksetzen.**
3. **Eine USB-Variante wählen:**
   - **Vorhandener Ventoy-Stick:** EFI auf die große Haupt-Datenpartition kopieren. Die kleine **VTOYEFI**-Partition unverändert lassen. Direkter EFI-Start; keine ISO und kein zusätzlicher Asset-Ordner nötig.
   - **Separater, bereits FAT32-vorbereiteter Stick:** Ordner `EFI/BOOT` anlegen. EFI hineinkopieren und nur die Kopie `BOOTX64.EFI` nennen. Ziel: **`EFI/BOOT/BOOTX64.EFI`**. Keinen vorhandenen Bootloader überschreiben; sonst einen anderen Stick nehmen.
4. **Starten:** Stick sicher auswerfen, anschließen, Lenovo einschalten und **F12 → USB-UEFI-Eintrag** wählen. Bei Ventoy anschließend die EFI auswählen. **Damit autorisierst du den automatischen Hardwareversuch; eine weitere Bestätigung ist nicht vorgesehen.** Beim FAT32-Stick kann er unmittelbar starten.
5. **Bei Problemen:** Unbekannte Werte nicht bestätigen; Aufforderungen mit Q, Esc oder Leertaste abbrechen. Bei **STOP oder Hänger physisch vollständig ausschalten**, nicht Windows erzwingen oder Prüfungen umgehen. Wenn möglich Fehler fotografieren; es wird kein Bericht gespeichert. Stick erst im ausgeschalteten Zustand entfernen.

Nicht formatieren, keine interne EFI-Partition verändern und keine Boot-Einträge anlegen. Die FAT32-Startvariante ist hardwareseitig ungeprüft.
