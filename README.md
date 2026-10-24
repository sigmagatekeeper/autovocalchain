# VocalChain – All-in-One Vocal Plugin (VST3)

Signalkette: **Autotune → EQ 1 (Cleanup) → De-Esser → Compressor → Distortion → EQ 2 (Sound) → Delay → Reverb → Output + Limiter**

Jedes Modul hat einen eigenen An/Aus-Schalter.

## Kompilieren mit GitHub (kein Visual Studio noetig)
1. Auf github.com ein neues Repository erstellen (z. B. `VocalChain`).
2. Den kompletten Inhalt dieses Ordners hochladen (inkl. dem versteckten Ordner `.github`).
   Am einfachsten mit GitHub Desktop oder `git push`.
3. Im Repo auf den Tab **Actions** gehen. Der Build startet automatisch (ca. 5-10 Min.).
4. Wenn er gruen ist: Build anklicken, unten bei **Artifacts** `VocalChain-VST3-Windows` herunterladen.
5. Den Ordner `VocalChain.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
6. FL Studio: **Options > Manage plugins > Find plugins** (Verify plugins aktiv lassen).
7. Plugin in einen Mixer-Slot laden: Suche nach "VocalChain".

## Tipps fuer den Rage/Hyperpop-Sound
- Autotune: Key auf den Song-Key, Retune 0-10 ms, Amount 100 %
- Distortion: Type "Hard" oder "Fold", Drive 10-20 dB, Mix 30-60 %
- EQ 2: Presence +3 bis +5 dB, Air +4 bis +6 dB
- De-Esser nach starker Distortion nachziehen (Thresh senken)
- Delay 1/8-Note: bei 140 BPM ca. 214 ms; Mix 10-20 %
