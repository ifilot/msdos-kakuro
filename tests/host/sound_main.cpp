/*
 * Native harness for drivers/dos/snddrv.c (built with -DSNDDRV_HOST).
 * Fakes the OPL2 / MPU-401 ports and prints, after every tick, either the OPL
 * register image (regs 0x20..0xF5) or the MIDI bytes sent during that tick.
 *
 *   snd_host <adlib|sb|mpu> <music|-> <sfx|-> <sfx_tick> <ticks>
 */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/SNDDRV.H"

static unsigned char regs[256];
static unsigned char latch;
static int timer_fired;
static int is_mpu;
static unsigned char tickbuf[65536];
static unsigned ticklen;

void host_outp(unsigned port, unsigned char value)
{
    if (is_mpu) {
        if ((port & 1) == 0) tickbuf[ticklen++] = value;     /* data port */
        return;
    }
    if ((port & 1) == 0) { latch = value; return; }
    regs[latch] = value;
    if (latch == 0x04) {
        if (value & 0x80) timer_fired = 0;                   /* IRQ reset */
        else if (value & 0x01) timer_fired = 1;              /* timer 1 started: expires */
    }
}

unsigned char host_inp(unsigned port)
{
    if (is_mpu) return (port & 1) ? 0x00 : 0xFE;             /* always ready, ACK */
    return timer_fired ? 0xC0 : 0x00;                        /* OPL status */
}

static unsigned char *load(const char *path)
{
    FILE *f;
    long n;
    unsigned char *p;
    if (path[0] == '-') return 0;
    f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    p = (unsigned char *)malloc((size_t)n);
    if (fread(p, 1, (size_t)n, f) != (size_t)n) { perror(path); exit(2); }
    fclose(f);
    return p;
}

int main(int argc, char **argv)
{
    int dev, err, r;
    long t, sfx_tick, ticks;
    unsigned i;
    unsigned char *music, *sfx;
    if (argc == 4 && argv[1][0] == 'v') {
        FILE *f = fopen(argv[3], "rb");
        long size;
        unsigned char *bytes;
        if (!f) return 2;
        fseek(f, 0, SEEK_END); size = ftell(f); fclose(f);
        bytes = load(argv[3]);
        dev = atoi(argv[2]);
        err = snd_validate(bytes, (unsigned)size, dev);
        free(bytes);
        return err ? 0 : 1;
    }
    if (argc != 6) { fprintf(stderr, "usage: snd_host dev music sfx sfx_tick ticks\n"); return 2; }
    dev = argv[1][0] == 'a' ? SND_ADLIB : argv[1][0] == 's' ? SND_SBFM : SND_MPU401;
    is_mpu = dev == SND_MPU401;
    music = load(argv[2]);
    sfx = load(argv[3]);
    sfx_tick = atol(argv[4]);
    ticks = atol(argv[5]);
    err = snd_init(dev, 0);
    if (err) { fprintf(stderr, "init failed %d\n", err); return 1; }
    ticklen = 0;
    if (music && (err = snd_music_play(music)) != 0) { fprintf(stderr, "music %d\n", err); return 1; }
    for (t = 0; t < ticks; t++) {
        if (sfx && t == sfx_tick && (err = snd_sfx_play(sfx)) != 0) { fprintf(stderr, "sfx %d\n", err); return 1; }
        snd_tick();
        for (i = 0; i < 8; ++i) snd_service();
        printf("%ld ", t);
        if (is_mpu) {
            for (i = 0; i < ticklen; i++) printf("%02x", tickbuf[i]);
            ticklen = 0;
        } else {
            for (r = 0x20; r <= 0xF5; r++) printf("%02x", regs[r]);
        }
        printf("\n");
    }
    snd_shutdown();
    return 0;
}
