#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SOUND.H"
#include "SNDDRV.H"
#include "SNDDATA.H"

static unsigned word(const unsigned char *p)
{ return p[0] | ((unsigned)p[1] << 8); }
static unsigned long dword(const unsigned char *p)
{ return word(p) | ((unsigned long)word(p+2) << 16); }
static int validSB(unsigned p)
{ return p == 0x220 || p == 0x240 || p == 0x260 || p == 0x280; }
static int validMPU(unsigned p)
{ return p == 0x300 || p == 0x310 || p == 0x320 || p == 0x330; }

Sound::Sound() : active(Off), selected(Automatic), sbPort(0x220),
    mpuPort(0x330), musicOn(1), effectsOn(1), currentTrack(-1), message("Sound off")
{
    for (int i=0;i<3;++i) { data[i]=0; sizes[i]=0; }
    unsigned base=snd_blaster_base();
    if(validSB(base))sbPort=base;
    const char *env=getenv("BLASTER");
    if(env) for(;*env;++env) if(*env=='P' || *env=='p') {
        base=(unsigned)strtoul(env+1,0,16);
        if(validMPU(base))mpuPort=base;
        break;
    }
}
Sound::~Sound() { shutdown(); }

void Sound::initialize()
{
    FILE *file=fopen("SOUND.CFG","r");
    int choice, music, effects;
    unsigned sb,mpu;
    if(file) {
        if(fscanf(file,"KSC1 %d %x %x %d %d",&choice,&sb,&mpu,&music,&effects)==5
            && choice>=-1 && choice<=3 && validSB(sb) && validMPU(mpu)
            && (music==0 || music==1) && (effects==0 || effects==1)) {
            selected=(Device)choice; sbPort=sb; mpuPort=mpu;
            musicOn=music; effectsOn=effects;
        }
        fclose(file);
    }
    configure(selected);
}

void Sound::shutdown()
{
    snd_stop_timer();
    if(active!=Off)snd_shutdown();
    active=Off;
    for(int i=0;i<3;++i) { free(data[i]); data[i]=0; sizes[i]=0; }
}

void Sound::setPorts(unsigned sb,unsigned mpu)
{ if(validSB(sb))sbPort=sb; if(validMPU(mpu))mpuPort=mpu; }

static int detectBlaster(unsigned port)
{
    // Reset the DSP only to identify the card. Playback uses its FM chip.
    outportb(port+6,1);
    for(unsigned n=0;n<16;++n)(void)inportb(port+6);
    outportb(port+6,0);
    for(n=0;n<4096;++n)
        if((inportb(port+14)&0x80) && inportb(port+10)==0xAA)return 1;
    return 0;
}

int Sound::startDevice(Device choice)
{
    if(choice==SoundBlaster && !detectBlaster(sbPort))return 0;
    unsigned port=choice==SoundBlaster ? sbPort : choice==MPU401 ? mpuPort : 0x388;
    if(snd_init(choice,port)!=SND_OK)return 0;
    active=choice;
    if(!loadAssets(choice)) { shutdown(); message="SOUND.DAT missing or damaged"; return -1; }
    snd_start_timer(60);
    message=choice==MPU401 ? "Ready; General MIDI synth required" : "Ready";
    if(currentTrack>=0 && musicOn)playMusic((Track)currentTrack);
    return 1;
}

int Sound::configure(Device choice)
{
    shutdown();
    selected=choice;
    if(choice==Off) { message="Sound off"; return 1; }
    if(choice==Automatic) {
        int result=startDevice(SoundBlaster);
        if(!result)result=startDevice(AdLib);
        if(!result)result=startDevice(MPU401);
        if(result)return result>0;
    } else if(choice>=AdLib && choice<=MPU401) {
        int result=startDevice(choice);
        if(result)return result>0;
    }
    message="Device not found; playing silently";
    return 0;
}

int Sound::loadAssets(Device choice)
{
    FILE *file=fopen("SOUND.DAT","rb");
    if(!file)return 0;
    unsigned char header[56];
    int ok=fread(header,1,56,file)==56 && !memcmp(header,"KSN1",4)
        && word(header+4)==6 && !word(header+6);
    if(fseek(file,0,SEEK_END))ok=0;
    long length=ftell(file);
    if(length<56)ok=0;
    int format=choice==MPU401 ? 1 : 0;
    for(int i=0;i<3 && ok;++i) {
        const unsigned char *record=header+8+(format*3+i)*8;
        unsigned long offset=dword(record);
        sizes[i]=word(record+4);
        if(offset<56 || sizes[i]<18 || sizes[i]>=32768u
            || offset>(unsigned long)length || sizes[i]>(unsigned long)length-offset) {ok=0;break;}
        data[i]=(unsigned char *)malloc(sizes[i]);
        if(!data[i] || fseek(file,(long)offset,SEEK_SET)
            || fread(data[i],1,sizes[i],file)!=sizes[i]) {ok=0;break;}
        unsigned sum=0;
        for(unsigned j=0;j<sizes[i];++j)sum+=data[i][j];
        if(sum!=word(record+6))ok=0;
        if(i<2)ok=ok && snd_validate(data[i],sizes[i],choice);
        else {
            if(sizes[i]!=soundOffsets[format][EffectCount])ok=0;
            for(int effect=0;effect<EffectCount && ok;++effect) {
                unsigned from=soundOffsets[format][effect],to=soundOffsets[format][effect+1];
                ok=from<to && to<=sizes[i] && snd_validate(data[i]+from,to-from,choice);
                // Effects must finish; music and MIDI effects own separate channels.
                if(ok && (data[i][from+5] || (format &&
                    (word(data[i]+from+12)&word(data[0]+12)))))ok=0;
            }
        }
    }
    if(fclose(file))ok=0;
    return ok;
}

void Sound::playMusic(Track track)
{
    if(track!=MenuMusic && track!=GameMusic)return;
    int changed=currentTrack!=(int)track;
    currentTrack=track;
    if(active!=Off && musicOn && (changed || !snd_music_playing()))
        snd_music_play(data[track]);
}
void Sound::playEffect(Effect effect)
{
    if(active!=Off && effectsOn && effect>=Click && effect<EffectCount)
        snd_sfx_play(data[2]+soundOffsets[active==MPU401 ? 1 : 0][effect]);
}
void Sound::service()
{
    snd_service();
    if(snd_failed()) { shutdown(); message="Audio stalled; playing silently"; }
}
void Sound::toggleMusic()
{
    musicOn=!musicOn;
    if(!musicOn)snd_music_stop();
    else if(currentTrack>=0)playMusic((Track)currentTrack);
}
void Sound::toggleEffects()
{ effectsOn=!effectsOn; if(!effectsOn)snd_sfx_stop(); }
int Sound::saveSettings() const
{
    FILE *file=fopen("SOUND.CFG","w");
    if(!file)return 0;
    int ok=fprintf(file,"KSC1 %d %x %x %d %d\n",selected,sbPort,mpuPort,musicOn,effectsOn)>0;
    return fclose(file)==0 && ok;
}
const char *Sound::deviceName(Device dev)
{
    switch(dev) {
        case Automatic:return "Automatic";
        case AdLib:return "AdLib (OPL2)";
        case SoundBlaster:return "Sound Blaster FM";
        case MPU401:return "MPU-401 / General MIDI";
        default:return "Off";
    }
}
