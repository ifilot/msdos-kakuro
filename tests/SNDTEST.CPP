#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "SOUND.H"
#include "SNDDRV.H"
#include "SOUNDMNU.H"
#include "MENU.H"
unsigned _stklen=16384;
static int errors;
static FILE *checks;
static void check(int ok,const char *message)
{ if(!ok) {++errors;fprintf(checks,"FAIL: %s\n",message);} }
static unsigned long biosTicks()
{ return *(volatile unsigned long far *)MK_FP(0x40,0x6C); }
static void waitBIOS(unsigned ticks)
{
    unsigned long start=biosTicks();
    while(biosTicks()-start<ticks)snd_service();
}
int main()
{
    checks=fopen("CHECKS.TXT","w");if(!checks)return 2;
    remove("SOUND.CFG");
    void interrupt (*original)(...)=getvect(8);
    Sound sound;
    check(sound.configure(Sound::Automatic),"Automatic hardware detection");
    check(sound.activeDevice()==Sound::SoundBlaster,"Automatic prefers detected SB FM");
    sound.playMusic(Sound::MenuMusic);
    check(snd_music_playing(),"Music starts");
    unsigned long before=biosTicks();
    waitBIOS(20);
    unsigned elapsed=snd_ticks();
    fprintf(checks,"Timer: %u audio ticks, %lu BIOS ticks\n",elapsed,biosTicks()-before);
    check(elapsed>=60 && elapsed<=72,"PIT plays at 60 Hz on an 8086 CPU");
    check(biosTicks()-before>=19 && biosTicks()-before<=22,"Chained BIOS clock retains 18.2 Hz");
    for(int effect=0;effect<Sound::EffectCount;++effect) {
        sound.playEffect((Sound::Effect)effect);
        check(snd_sfx_playing(),"Every effect starts");
    }
    sound.playEffect(Sound::Click);waitBIOS(6);
    check(!snd_sfx_playing() && snd_music_playing(),"Effect finishes while music continues");
    sound.toggleMusic();check(!snd_music_playing(),"Music toggle off");
    sound.toggleMusic();check(snd_music_playing(),"Music toggle on");
    sound.toggleEffects();sound.playEffect(Sound::Solved);
    check(!snd_sfx_playing(),"Disabled effects stay silent");sound.toggleEffects();
    sound.playMusic(Sound::GameMusic);check(snd_music_playing(),"Game track starts");
    sound.shutdown();
    check(getvect(8)==original,"Shutdown restores original INT 08h vector");
    check(sound.configure(Sound::AdLib),"AdLib OPL detection and asset loading");
    sound.playMusic(Sound::MenuMusic);waitBIOS(2);
    outportb(0x388,0x04);outportb(0x389,0x80);
    check((inportb(0x388)&0xE0)==0,"Music never interferes with OPL timer registers");
    sound.shutdown();
    check(sound.configure(Sound::MPU401),"MPU reset ACK, UART ACK and MIDI assets");
    fprintf(checks,"MPU result: %s\n",sound.status());
    sound.playMusic(Sound::GameMusic);sound.playEffect(Sound::Solved);
    waitBIOS(4);check(snd_music_playing() && snd_sfx_playing(),"Concurrent MIDI music and effects");
    sound.shutdown();check(getvect(8)==original,"MIDI shutdown restores timer");
    sound.setPorts(0x280,0x330);
    check(!sound.configure(Sound::SoundBlaster) && sound.activeDevice()==Sound::Off,
          "Missing hardware falls back to silence");
    sound.setPorts(0x220,0x330);
    check(sound.configure(Sound::Off),"Explicit Off");
    check(sound.saveSettings(),"Save settings");
    sound.configure(Sound::AdLib);sound.initialize();
    check(sound.selectedDevice()==Sound::Off && sound.activeDevice()==Sound::Off,
          "Saved Off survives initialization");
    check(rename("SOUND.DAT","SOUND.BAK")==0,"Hide audio pack");
    check(!sound.configure(Sound::AdLib) && sound.activeDevice()==Sound::Off,
          "Missing audio pack is safe");
    rename("SOUND.BAK","SOUND.DAT");
    FILE *bad=fopen("SOUND.DAT","r+b");
    if(!bad)return 2;fseek(bad,70,SEEK_SET);int byte=fgetc(bad);
    fseek(bad,70,SEEK_SET);fputc(byte^1,bad);fclose(bad);
    check(!sound.configure(Sound::AdLib),"Damaged stream checksum rejected");
    bad=fopen("SOUND.DAT","r+b");if(!bad)return 2;
    fseek(bad,70,SEEK_SET);fputc(byte,bad);fclose(bad);
    check(sound.configure(Sound::AdLib),"Recovered pack loads");
    VgaScreen screen;screen.initialize();
    MenuScreen menu(screen);menu.loadCatalog();menu.draw();
    check(menu.handleKey(0x13D)==MenuScreen::Audio,"F3 opens audio settings");
    SoundMenu panel(screen,sound);panel.draw();
    panel.handleKey(0x148);panel.handleKey(0x148); // Automatic, then clamp.
    check(panel.selectedIndex()==0,"Sound selection clamps at top");
    for(int i=0;i<6;++i)panel.handleKey(0x150);
    check(panel.selectedIndex()==4,"Sound selection clamps at bottom");
    panel.handleKey('p');panel.handleKey('b');
    check(sound.blasterPort()==0x220 && sound.midiPort()==0x330,"Port edits wait for Enter");
    panel.handleKey(0x148);panel.handleKey(0x148);panel.handleKey(0x148); // Off
    panel.handleKey(13);check(sound.activeDevice()==Sound::Off,"Panel applies Off");
    check(!panel.handleKey(27),"Escape closes panel");
    sound.setPorts(0x220,0x330);sound.configure(Sound::AdLib);
    SoundMenu preview(screen,sound);preview.draw();
    FILE *file=fopen("SCREEN.RAW","wb");if(!file)return 2;
    volatile unsigned char far *v=(volatile unsigned char far *)MK_FP(0xA000,0);
    unsigned char row[640];
    for(int y=0;y<480;++y) {
        memset(row,0,640);
        for(int plane=0;plane<4;++plane) {
            outport(0x3CE,(plane<<8)|4);
            for(int x=0;x<640;++x)if(v[(unsigned)y*80+x/8]&(0x80>>(x&7)))row[x]|=1<<plane;
        }
        if(fwrite(row,1,640,file)!=640)return 2;
    }
    fclose(file);file=fopen("PALETTE.RAW","wb");if(!file)return 2;
    outportb(0x3C7,0);for(i=0;i<768;++i)fputc(inportb(0x3C9),file);fclose(file);
    sound.shutdown();remove("SOUND.CFG");
    check(getvect(8)==original,"Final cleanup restores vector");
    fclose(checks);file=fopen("RESULT.TXT","w");if(!file)return 2;
    fputs(errors ? "Sound tests failed\n" : "Sound tests passed\n",file);
    fclose(file);return errors ? 1 : 0;
}
