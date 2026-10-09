# Kakuro audio

Imported from `/mnt/c/PROGRAMMING/CX16/cx16-sound-generator`. At import the
exports lived in `build/kakuro-dos/fm` and `build/kakuro-dos/midi`; the current
generator uses `build/kakuro/opl` and `build/kakuro/gm` (identical stream
contents, with `.GMS` names for MIDI). The exports contain the
MENU and GAME music loops and 18 effects: navigation, selection, back, page,
nine digit tones, verification on/off, wrong, dialog and solved.

The `.OPL` exports target **both AdLib and Sound Blaster FM**, not the Sound
Blaster PCM/DMA path. `.MDS` exports target General MIDI over MPU-401 UART.
The original generator headers retain the effect offsets. All 18 effects
are available through `Sound::Effect`; verification and dialog effects are
reserved for future corresponding game features.

`make build` runs `tools/pack_sound.py` when needed. It produces `SOUND.DAT`
and `src/SNDDATA.H`. No reference to the external generator is needed to build
or run the game. To refresh the music, export the Kakuro project there, copy
the three binary files **and sfx.h** for each target into these directories,
renaming MIDI `.GMS` files to `.MDS`, then run `python3 tools/pack_sound.py`
and rebuild.

The release contains one 38,891-byte audio pack. The small register/event
streams are stored directly, avoiding a decompressor and interrupt-time I/O.
Only the chosen format is loaded: 19,844 bytes for FM or 18,991 for MIDI.

Pack format (little endian):

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 4 | `KSN1` |
| 4 | 2 | Record count, 6 |
| 6 | 2 | Reserved, zero |
| 8 | 48 | Six records: offset:u32, size:u16, sum of payload bytes modulo 65536:u16 |
| 56 | remaining | FM MENU/GAME/SFX, then MIDI MENU/GAME/SFX |

Each track/effect has the generator's 16-byte `OPLR` or `MIDR` header:
version:u8, flags:u8 (bit 0 loops), tick rate:u16 (60), loop offset:u32,
channel mask:u16, reserved:u16. OPL bodies contain `(register,value)` pairs;
`(0,n)` waits n ticks and `(0,0)` ends/loops. MIDI bodies contain complete
channel messages; `FD n` waits and `FD 00` ends/loops. Effect banks concatenate
18 such streams; the generated offset table locates each effect.

The loader checks length, checksum, header, event boundaries, loop points,
terminators and effect-channel separation before enabling playback. The
checksum detects ordinary corruption; it is not cryptographic authentication.

The C++ driver in `src/SNDDRV.CPP` is adapted from the generator's
`drivers/dos/snddrv.c`/`.h`, with bounded MPU handshakes, validated assets,
queued MIDI output, inline OPL port writes and timer cleanup.
