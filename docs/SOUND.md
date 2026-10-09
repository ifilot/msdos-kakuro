# Sound integration

`Sound` owns hardware initialization, resident assets, music, effects and
settings. `SoundMenu` draws the F3 panel. `SNDDRV` handles register/event
playback, effect channel lending and INT 08h timing. Game and menu code use
the device-independent `Sound::Track` and `Sound::Effect` enums.

Music switches between the menu and game loops. Effects accompany selection,
page changes, movement, digit entry, erasing, attempts to edit locked cells,
returning and puzzle completion. On FM, effects temporarily borrow channels
and restore music's instruments afterwards. MIDI effects use separate channels.

Automatic detection tries the Sound Blaster DSP reset signature and FM timer
test at the configured SB base, then the OPL timer test at 388h, then MPU-401
reset/UART acknowledgements at the configured MIDI base. BLASTER's A and P
fields supply defaults; otherwise these are 220h and 330h. Automatic does not
scan every ISA port. The F3 panel permits SB bases 220/240/260/280 and MPU
bases 300/310/320/330. AdLib uses 388h. Original Sound Blasters without an
MPU-compatible interface can use the Sound Blaster FM backend.

An MPU-401 acknowledgement identifies the interface, not a connected synth.
The MIDI arrangement needs a General MIDI module; MT-32 is not supported.
UART interfaces which do not implement the reset/UART ACK handshake are not
detected. Missing hardware/assets leave the game playable and silent.

Settings are five fields after `KSC1` in SOUND.CFG: device (-1 auto, 0 off,
1 AdLib, 2 SB FM, 3 MIDI), hexadecimal SB/MPU ports, music/effects flags.
Enter applies/tests/saves; M/E toggle and save. Port edits apply on Enter.
The configuration is optional and is not shipped with releases.

The PIT runs at 60 Hz. INT 08h adds the divisor to an accumulator and
tail-chains the original handler at the BIOS rate. No allocation, file I/O
or DOS calls occur inside the interrupt. MIDI events enter a 512-byte ring;
nonblocking output runs from the timer and the keyboard wait loop. FM port
delays use inline 8086 assembly. Exiting or changing devices stops playback,
restores the timer vector/divisor and frees the selected assets.

`make run` enables audio and emulates a Sound Blaster Pro 2 by default. With
Docker/WSLg, the runner forwards the PulseAudio socket. The existing image
also contains FluidSynth and a system GM soundfont, which the runner enables
when present. Tests/builds keep host audio muted. DOSBox exposes a reset-capable
MPU-401 and the driver switches it to UART; DOSBox-X's forced UART emulation
does not provide the detection acknowledgements.

Optional runner variables:

| Variable | Purpose |
| --- | --- |
| `DOS_SOUND_CARD` | DOSBox SB type (default `sbpro2`) |
| `DOS_MIDI_DEVICE` | Override MIDI backend (`fluidsynth`, `none`, etc.) |
| `DOS_MIDI_CONFIG` | DOSBox host MIDI device configuration |
| `DOS_SOUNDFONT` | Host `.sf2` file; Docker mounts it read-only |
| `DOS_CYCLES` | Emulated CPU budget, e.g. 3000 for a slower test |

`make test-sound` compares both tracks and representative effects against an
independent host model, across loop points, for all three device paths. It
also checks every effect, malformed streams and pack contents. The DOS test
checks detection, concurrent playback, settings, failure handling, timer
restoration and panel controls on DOSBox's 8086 CPU. Real ISA hardware timing
and exact 4.77 MHz 8088 performance still require hardware measurements.

Hardware references: [Yamaha YM3812 application manual](https://c64.xentax.com/media/Yamaha_YM3812_Application_Manual.pdf),
[MPU reset/UART handshake in the Microchip controller manual](https://ww1.microchip.com/downloads/en/DeviceDoc/00002492A.pdf),
[DOSBox-X MIDI configuration](https://dosbox-x.com/wiki/Guide%3ASetting-up-MIDI-in-DOSBox%E2%80%90X).
