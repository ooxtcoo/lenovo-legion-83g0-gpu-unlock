# Weiterentwickeln (Windows x64)

Diese Schritte bauen und testen lokal auf der CPU. Sie starten keine EFI, schreiben keinen USB-Stick und ändern keine Firmwareeinstellungen. In einem frischen Klon arbeiten: erzeugte Nachweise überschreiben teilweise historische Dateien.

## 1. Einrichten

Git, **Python 3.11 oder neuer** und **uv** installieren. Im Terminal:

```powershell
git clone https://github.com/ooxtcoo/lenovo-legion-83g0-gpu-unlock.git
cd lenovo-legion-83g0-gpu-unlock
python --version
uv --version
uv run --with ziglang==0.13.0 python -m ziglang version
```

Die letzte Ausgabe muss `0.13.0` sein. Danach verwenden die Helfer den Offline-Cache; EDK2-Header liegen bereits unter `deps/`.

## 2. Assets importieren

Die fünf Binärdateien fehlen im öffentlichen Klon. Einen rechtmäßig verfügbaren lokalen Ordner mit den exakten Dateien aus `ASSET-PINS.json` bereitstellen und den Beispielpfad ersetzen:

```powershell
python import_assets.py "C:/path/to/locally-owned/assets"
```

Der Import prüft Dateigrößen und SHA-256. Keine anderen Firmwaredateien einsetzen und keine Prüfungen entfernen. `src/assets.json` ist nicht das aktive Asset-Verzeichnis dieser Variante.

## 3. Testen, bauen, prüfen

```powershell
python test_classifier.py
python build_efi.py backup-build
python verify_image.py
```

Der Klassifikator benötigt keine Firmware-Assets. Das Build-Ergebnis liegt unter `build/Lenovo-AD103M-Unlock-v5-auto-v2.efi`. **`backup-build` genau übernehmen:** Der Bildprüfer erwartet diesen Protokollnamen und prüft PE-Struktur, Relokationen und eingebettete Assets. Bei Fehlern stoppen.

Ausgaben: `evidence/backup-classifier.json`, `evidence/backup-build*.json` und `evidence/image-verification-auto-v2.json`. Keine Binärdateien, privaten Dumps, Schlüssel oder lokalen Pfadprotokolle veröffentlichen.

Die vollständigen historischen Anwendungstests benötigen ausgelassene private PROM-Daten und alte Nachbarverzeichnisse; sie sind hier nicht vollständig reproduzierbar. Übernommene Ergebnisse sind keine neuen Testläufe, CPU-/Bildprüfungen kein Hardwaretest.

Für Änderungen: `README-AUTO-V2.md`, `START_HERE.md` und `evidence/` lesen. Der Build bindet Quellhashes an historische Testnachweise; Änderungen brauchen neue belastbare Nachweise, keine entfernten Sperren. Schutzprüfungen, Lizenztexte und Herkunftsnachweise erhalten.
