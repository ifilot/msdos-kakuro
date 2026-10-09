/*
 * SNDDRV - minimal music + sound effect driver for x16snd streams on MS-DOS.
 *
 * Devices
 *   SND_ADLIB    AdLib / any OPL2-compatible FM at port 0x388      plays .OPL streams
 *   SND_SBFM     Sound Blaster FM (OPL2) at base+8, e.g. 0x228     plays .OPL streams
 *   SND_MPU401   General MIDI on an MPU-401 in UART mode (0x330)  plays .MDS streams
 *
 * One music stream and one sound effect stream play at the same time. On OPL
 * an effect borrows the FM channels in its header mask; the music keeps
 * running in a shadow copy and those channels are restored when the effect
 * ends. On MIDI, effects use their own MIDI channels (x16snd assigns 16, 15..).
 *
 * Usage
 *   snd_init(SND_ADLIB, 0);            port 0 = device default
 *   snd_start_timer(60);               tick rate from the stream header (x16snd: 60)
 *   snd_music_play(menu_data);         pointer to the whole file as loaded
 *   snd_sfx_play(sfx_bank + SFX_PLACE5);
 *   ...
 *   snd_stop_timer(); snd_shutdown();
 *
 * Written in C89 for Borland C / Borland C++ (large or compact model, or any
 * model with far data). Streams must be smaller than 64 KiB each.
 */
#ifndef SNDDRV_H
#define SNDDRV_H

#ifdef SNDDRV_HOST          /* native test build: no segmented pointers */
#define far
#endif

#define SND_ADLIB   1
#define SND_SBFM    2
#define SND_MPU401  3

#define SND_OK          0
#define SND_ERR_DEVICE  1   /* no device answered at that port */
#define SND_ERR_FORMAT  2   /* stream is not for this device (wrong magic) */

int  snd_init(int device, unsigned port);
void snd_shutdown(void);

int  snd_music_play(const unsigned char far *stream);
void snd_music_stop(void);
int  snd_music_playing(void);

int  snd_sfx_play(const unsigned char far *stream);
void snd_sfx_stop(void);
int  snd_sfx_playing(void);

void snd_service(void);
int snd_failed(void);
unsigned snd_ticks(void);
int snd_validate(const unsigned char far *stream, unsigned size, int device);
void snd_tick(void);                /* advance both streams by one tick */

void snd_start_timer(unsigned hz);  /* INT 08h handler calling snd_tick, chains BIOS at 18.2 Hz */
void snd_stop_timer(void);

unsigned snd_blaster_base(void);    /* base port from the BLASTER variable, 0 if unset */

#endif
