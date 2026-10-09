/*
 * SNDDRV - see snddrv.h. Stream formats are described in docs/FORMAT.md.
 *
 * OPL stream (.OPL): 16-byte header "OPLR", then (reg, value) byte pairs;
 *   reg 0 means: value 1..255 = wait that many ticks, value 0 = end of stream.
 * MIDI stream (.MDS): 16-byte header "MIDR", then complete MIDI messages;
 *   0xFD n means: n 1..255 = wait n ticks, n 0 = end of stream.
 * Header: magic[4] ver flags rate[2] loop_offset[4] channel_mask[2] reserved[2]
 */
#include "SNDDRV.H"

#ifdef SNDDRV_HOST
/* native test build: port I/O goes to the test harness */
void host_outp(unsigned port, unsigned char value);
unsigned char host_inp(unsigned port);
#define OUTP(p, v)  host_outp((p), (unsigned char)(v))
#define INP(p)      host_inp(p)
#define DISABLE()
#define ENABLE()
#else
#include <dos.h>
#include <stdlib.h>
#define OUTP(p, v)  outportb((p), (unsigned char)(v))
#define INP(p)      inportb(p)
#define DISABLE()   disable()
#define ENABLE()    enable()
#endif

#define HEADER_SIZE   16
#define MAX_PER_TICK  256      /* guard against a stream without waits */

typedef struct {
    const unsigned char far *base;
    const unsigned char far *pos;
    unsigned loop;              /* offset of the loop point, 0 = no loop */
    unsigned wait;
    unsigned mask;              /* channels used (header) */
    unsigned char active;
} snd_stream;

static int device = 0;
static volatile unsigned tick_count = 0;
static unsigned port_base = 0;
static snd_stream music;
static snd_stream sfx;

/* ---------------------------------------------------------------- OPL */

static unsigned char shadow[256];       /* music's view of every OPL register */
static unsigned char chan_of[256];      /* register -> OPL channel, 0xFF = global */
static unsigned sfx_owned = 0;          /* OPL channels currently lent to the effect */
static const unsigned char op_slot[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

static void opl_write(unsigned char reg, unsigned char val)
{
#if defined(__BORLANDC__) && !defined(SNDDRV_HOST)
    /* Inline I/O avoids 41 far calls per write on the 8088. Preserve the
       conventional six/35 status reads required by an OPL2 ISA interface. */
    asm mov dx,port_base;
    asm mov al,reg;
    asm out dx,al;
    asm in al,dx;
    asm in al,dx;
    asm in al,dx;
    asm in al,dx;
    asm in al,dx;
    asm in al,dx;
    asm inc dx;
    asm mov al,val;
    asm out dx,al;
    asm dec dx;
    asm mov cx,35;
opl_delay:
    asm in al,dx;
    asm loop opl_delay;
#else
    int i;
    OUTP(port_base, reg);
    for (i = 0; i < 6; i++) (void)INP(port_base);      /* 3.3 us after the address */
    OUTP(port_base + 1, val);
    for (i = 0; i < 35; i++) (void)INP(port_base);     /* 23 us after the data */
#endif
}

static void opl_build_tables(void)
{
    static const unsigned char bases[5] = { 0x20, 0x40, 0x60, 0x80, 0xE0 };
    int i, c, b;
    for (i = 0; i < 256; i++) chan_of[i] = 0xFF;
    for (c = 0; c < 9; c++) {
        chan_of[0xA0 + c] = (unsigned char)c;
        chan_of[0xB0 + c] = (unsigned char)c;
        chan_of[0xC0 + c] = (unsigned char)c;
        for (b = 0; b < 5; b++) {
            chan_of[bases[b] + op_slot[c]] = (unsigned char)c;
            chan_of[bases[b] + op_slot[c] + 3] = (unsigned char)c;
        }
    }
}

static int opl_detect(void)
{
    unsigned char s1, s2;
    int i;
    opl_write(0x04, 0x60);
    opl_write(0x04, 0x80);
    s1 = INP(port_base);
    opl_write(0x02, 0xFF);
    opl_write(0x04, 0x21);
    for (i = 0; i < 200; i++) (void)INP(port_base);    /* > 80 us */
    s2 = INP(port_base);
    opl_write(0x04, 0x60);
    opl_write(0x04, 0x80);
    return (s1 & 0xE0) == 0x00 && (s2 & 0xE0) == 0xC0;
}

static void opl_reset(void)
{
    int r;
    for (r = 0x01; r <= 0xF5; r++) {
        opl_write((unsigned char)r, 0);
        shadow[r] = 0;
    }
    opl_write(0x01, 0x20);          /* enable waveform select */
    shadow[0x01] = 0x20;
}

static void opl_music_write(unsigned char reg, unsigned char val)
{
    unsigned char c = chan_of[reg];
    shadow[reg] = val;
    if (c != 0xFF && (sfx_owned & (1u << c))) return;     /* channel lent to the effect */
    opl_write(reg, val);
}

static void opl_key_off(unsigned char c, unsigned char b0)
{
    opl_write((unsigned char)(0xB0 + c), (unsigned char)(b0 & ~0x20));
}

/* give channels back to the music: rewrite its registers, notes stay released */
static void opl_restore(unsigned mask)
{
    int r;
    unsigned char c;
    for (r = 0x20; r <= 0xF5; r++) {
        c = chan_of[r];
        if (c == 0xFF || !(mask & (1u << c))) continue;
        if (r >= 0xB0 && r <= 0xB8) opl_key_off(c, shadow[r]);
        else opl_write((unsigned char)r, shadow[r]);
    }
}

/* ---------------------------------------------------------------- MPU-401 */

/* IRQ output never waits for the 31.25 kbaud UART. Events enter a ring;
   each timer tick drains what the interface can accept. No DOS calls here. */
static unsigned char midi_queue[512];
static unsigned midi_head = 0, midi_tail = 0;
static int midi_failed = 0;

static void mpu_write(unsigned char b)
{
    unsigned next = (midi_head + 1) & 511;
    if (next == midi_tail) { midi_failed = 1; return; }
    midi_queue[midi_head] = b;
    midi_head = next;
}

static void mpu_drain(void)
{
    unsigned count = 0;
    while (midi_tail != midi_head && count++ < 64) {
        if (INP(port_base + 1) & 0x40) break;
        OUTP(port_base, midi_queue[midi_tail]);
        midi_tail = (midi_tail + 1) & 511;
    }
}

static int mpu_ready(void)
{
    unsigned n;
    for (n = 0; n < 4096; ++n)
        if (!(INP(port_base + 1) & 0x40)) return 1;
    return 0;
}

static int mpu_ack(void)
{
    unsigned n;
    for (n = 0; n < 60000u; ++n)
        if (!(INP(port_base + 1) & 0x80) && INP(port_base) == 0xFE) return 1;
    return 0;
}

static int mpu_init(void)
{
    unsigned n;
    midi_head = midi_tail = 0;
    midi_failed = 0;
    /* A reset from UART mode may omit its ACK. Retry after leaving UART. */
    for (n = 0; n < 2; ++n) {
        if (!mpu_ready()) return 0;
        OUTP(port_base + 1, 0xFF);
        if (mpu_ack()) break;
    }
    if (n == 2 || !mpu_ready()) return 0;
    OUTP(port_base + 1, 0x3F);
    return mpu_ack();
}

static void mpu_all_off(unsigned mask)
{
    unsigned char c;
    for (c = 0; c < 16; c++) {
        if (!(mask & (1u << c))) continue;
        mpu_write((unsigned char)(0xB0 | c)); mpu_write(123); mpu_write(0);   /* all notes off */
        mpu_write((unsigned char)(0xE0 | c)); mpu_write(0);   mpu_write(64);  /* centre the bend */
    }
}

static unsigned midi_len(unsigned char status)
{
    switch (status & 0xF0) {
    case 0xC0: case 0xD0: return 2;
    case 0xF0: return 1;
    default:   return 3;
    }
}

/* ---------------------------------------------------------------- streams */

static int is_opl(void) { return device == SND_ADLIB || device == SND_SBFM; }

static int stream_open(snd_stream *s, const unsigned char far *p)
{
    unsigned flags;
    if (is_opl()) {
        if (p[0] != 'O' || p[1] != 'P' || p[2] != 'L' || p[3] != 'R') return SND_ERR_FORMAT;
    } else {
        if (p[0] != 'M' || p[1] != 'I' || p[2] != 'D' || p[3] != 'R') return SND_ERR_FORMAT;
    }
    flags = p[5];
    s->base = p;
    s->pos = p + HEADER_SIZE;
    s->loop = (flags & 1) ? (unsigned)(p[8] | (p[9] << 8)) : 0;
    s->mask = (unsigned)(p[12] | (p[13] << 8));
    s->wait = 0;
    s->active = 1;
    return SND_OK;
}

static void sfx_finish(void)
{
    if (is_opl()) {
        unsigned owned = sfx_owned;
        unsigned char c;
        for (c = 0; c < 9; c++)
            if (owned & (1u << c)) opl_key_off(c, 0);
        sfx_owned = 0;
        opl_restore(owned);
    } else {
        mpu_all_off(sfx.mask);
    }
    sfx.active = 0;
}

/* run one tick of a stream; returns 0 when the stream ended */
static int stream_tick(snd_stream *s, int is_music)
{
    unsigned guard = 0;
    unsigned char a, b;
    unsigned n, i;
    if (s->wait) {
        if (--s->wait) return 1;
    }
    while (guard++ < MAX_PER_TICK) {
        a = s->pos[0];
        if (is_opl()) {
            b = s->pos[1];
            s->pos += 2;
            if (a == 0) {
                if (b) { s->wait = b; return 1; }
                if (s->loop) { s->pos = s->base + s->loop; continue; }
                return 0;
            }
            if (is_music) opl_music_write(a, b);
            else opl_write(a, b);
        } else {
            if (a == 0xFD) {
                b = s->pos[1];
                s->pos += 2;
                if (b) { s->wait = b; return 1; }
                if (s->loop) { s->pos = s->base + s->loop; continue; }
                return 0;
            }
            n = midi_len(a);
            for (i = 0; i < n; i++) mpu_write(s->pos[i]);
            s->pos += n;
        }
    }
    return 1;
}

void snd_tick(void)
{
    ++tick_count;
    if (!device) return;
    if (music.active && !stream_tick(&music, 1)) music.active = 0;
    if (sfx.active && !stream_tick(&sfx, 0)) sfx_finish();
    if (!is_opl()) {
        mpu_drain();
        if (midi_failed) { music.active = sfx.active = 0; }
    }
}

/* ---------------------------------------------------------------- API */

int snd_init(int dev, unsigned port)
{
    device = dev;
    music.active = sfx.active = 0;
    sfx_owned = 0;
    if (dev == SND_ADLIB || dev == SND_SBFM) {
        if (!port) port = (dev == SND_SBFM) ? 0x220 : 0x388;
        port_base = (dev == SND_SBFM) ? port + 8 : port;     /* SB: FM at base+8 */
        opl_build_tables();
        if (!opl_detect()) { device = 0; return SND_ERR_DEVICE; }
        opl_reset();
        return SND_OK;
    }
    if (dev == SND_MPU401) {
        port_base = port ? port : 0x330;
        if (!mpu_init()) { device = 0; return SND_ERR_DEVICE; }
        return SND_OK;
    }
    device = 0;
    return SND_ERR_DEVICE;
}

void snd_shutdown(void)
{
    snd_music_stop();
    snd_sfx_stop();
    if (is_opl()) opl_reset();
    else if (device == SND_MPU401) {
        /* Foreground shutdown: drain note-offs, then release UART mode. */
        while (midi_tail != midi_head && mpu_ready()) {
            OUTP(port_base, midi_queue[midi_tail]);
            midi_tail = (midi_tail + 1) & 511;
        }
        if (mpu_ready()) OUTP(port_base + 1, 0xFF);
        midi_head = midi_tail = 0;
    }
    device = 0;
}

int snd_music_play(const unsigned char far *stream)
{
    int err;
    snd_music_stop();
    DISABLE();
    err = stream_open(&music, stream);
    ENABLE();
    return err;
}

void snd_music_stop(void)
{
    unsigned char c;
    DISABLE();
    if (music.active) {
        music.active = 0;
        if (is_opl()) {
            for (c = 0; c < 9; c++) {
                shadow[0xB0 + c] &= ~0x20;
                if (!(sfx_owned & (1u << c))) opl_key_off(c, shadow[0xB0 + c]);
            }
        } else {
            mpu_all_off(music.mask);
        }
    }
    ENABLE();
}

int snd_music_playing(void) { return music.active; }

int snd_sfx_play(const unsigned char far *stream)
{
    int err;
    unsigned char c;
    DISABLE();
    if (sfx.active) sfx_finish();
    err = stream_open(&sfx, stream);
    if (err == SND_OK && is_opl()) {
        sfx_owned = sfx.mask & 0x1FF;
        for (c = 0; c < 9; c++)
            if (sfx_owned & (1u << c)) opl_key_off(c, shadow[0xB0 + c]);
    }
    ENABLE();
    return err;
}

void snd_sfx_stop(void)
{
    DISABLE();
    if (sfx.active) sfx_finish();
    ENABLE();
}

int snd_sfx_playing(void) { return sfx.active; }

/* ---------------------------------------------------------------- timer + BLASTER */

#ifndef SNDDRV_HOST

#ifdef __cplusplus
#define ISR_ARGS ...
#else
#define ISR_ARGS
#endif

static void interrupt (*old_int8)(ISR_ARGS);
static unsigned long pit_divisor = 0;
static unsigned long pit_accum = 0;

static void interrupt timer_isr(ISR_ARGS)
{
    snd_tick();
    pit_accum += pit_divisor;
    if (pit_accum >= 0x10000UL) {          /* keep the BIOS clock at 18.2 Hz */
        pit_accum -= 0x10000UL;
        _chain_intr(old_int8);             /* tail-chain; BIOS acknowledges PIC */
    } else {
        outportb(0x20, 0x20);
    }
}

void snd_start_timer(unsigned hz)
{
    if (old_int8 || hz != 60) return;
    tick_count = 0;
    pit_divisor = 1193182UL / hz;
    pit_accum = 0;
    disable();
    old_int8 = getvect(0x08);
    setvect(0x08, timer_isr);
    outportb(0x43, 0x36);
    outportb(0x40, (unsigned char)(pit_divisor & 0xFF));
    outportb(0x40, (unsigned char)(pit_divisor >> 8));
    enable();
}

void snd_stop_timer(void)
{
    if (!old_int8) return;
    disable();
    outportb(0x43, 0x36);
    outportb(0x40, 0);
    outportb(0x40, 0);
    setvect(0x08, old_int8);
    old_int8 = 0;
    enable();
}

unsigned snd_blaster_base(void)
{
    const char *s = getenv("BLASTER");
    if (!s) return 0;
    while (*s) {
        if (*s == 'A' || *s == 'a') return (unsigned)strtoul(s + 1, 0, 16);
        s++;
    }
    return 0;
}

#else   /* SNDDRV_HOST */

void snd_start_timer(unsigned hz) { (void)hz; tick_count = 0; }
void snd_stop_timer(void) { }
unsigned snd_blaster_base(void) { return 0; }

#endif

unsigned snd_ticks(void) { return tick_count; }
int snd_failed(void) { return device == SND_MPU401 && midi_failed; }

/* Validate in the foreground before an interrupt sees a stream. Loop points
   must be event boundaries and contain a wait: no malformed tight loops. */
int snd_validate(const unsigned char far *p, unsigned size, int dev)
{
    unsigned pos = 16, loop, mask, n, i;
    int saw_loop = 0, waited = 0;
    unsigned char a;
    int fm = dev == SND_ADLIB || dev == SND_SBFM;
    if (size < 18 || size >= 32768u || p[4] != 1 || (p[5] & ~1)
        || p[6] != 60 || p[7] || p[10] || p[11] || p[14] || p[15]) return 0;
    if (fm ? (p[0]!='O' || p[1]!='P' || p[2]!='L' || p[3]!='R')
           : (p[0]!='M' || p[1]!='I' || p[2]!='D' || p[3]!='R')) return 0;
    loop = p[8] | ((unsigned)p[9] << 8);
    mask = p[12] | ((unsigned)p[13] << 8);
    if ((fm && (mask & ~0x1FFu)) || !mask) return 0;
    if (p[5] ? (loop < 16 || loop >= size) : loop != 0) return 0;
    while (pos < size) {
        if (pos == loop) { saw_loop = 1; waited = 0; }
        a = p[pos];
        if (fm || a == 0xFD) {
            if (size - pos < 2) return 0;
            if (a == (fm ? 0 : 0xFD)) {
                if (!p[pos+1]) return pos+2 == size && (!loop || (saw_loop && waited));
                waited = 1;
            } else if (a == 1 || a == 2 || a == 3 || a == 4 || a == 8 || a == 0xBD)
                return 0; /* OPL global/timer registers belong to the driver */
            pos += 2;
        } else {
            if (a < 0x80 || a >= 0xF0) return 0;
            n = midi_len(a);
            if (size - pos < n) return 0;
            for (i = 1; i < n; ++i) if (p[pos+i] & 0x80) return 0;
            pos += n;
        }
    }
    return 0;
}

/* Foreground UART service while waiting for input. */
void snd_service(void)
{
    if (device != SND_MPU401) return;
    DISABLE(); mpu_drain(); ENABLE();
}
