#include <stdio.h>
#include "SOUNDMNU.H"
#include "PROFONT.H"

SoundMenu::SoundMenu(VgaScreen &display,Sound &audio)
    :screen(display),sound(audio),selected(audio.selectedDevice()+1),
     sbPort(audio.blasterPort()),mpuPort(audio.midiPort()),notice(0) {}

void SoundMenu::draw()
{
    screen.setDrawingScale(1);
    enum { Frame=10,Gold=11,Face=12,Accent=13,Shadow=14 };
    screen.setPaletteColor(Frame,14,9,8);screen.setPaletteColor(Gold,63,48,16);
    screen.setPaletteColor(Face,59,56,43);screen.setPaletteColor(Accent,35,21,19);
    screen.setPaletteColor(Shadow,4,4,4);
    screen.fillRect(116,62,416,366,Shadow);
    screen.fillRect(112,58,416,366,Frame);
    screen.fillRect(114,60,412,362,Gold);
    screen.fillRect(116,62,408,358,Face);
    ProFont::drawCentered(screen,128,74,384,36,"SOUND",Frame,2);
    screen.drawLine(136,116,504,116,Accent);
    drawContents();
}

void SoundMenu::drawContents()
{
    screen.setDrawingScale(1);
    screen.fillRect(128,126,384,284,12);
    for(int row=0;row<5;++row) {
        int y=132+row*28;
        if(row==selected)screen.fillRect(136,y-6,368,24,13);
        screen.drawText(148,y,row==selected ? ">" : " ",row==selected ? 12 : 10);
        screen.drawText(172,y,row==1 ? "Off (music and effects)" : Sound::deviceName((Sound::Device)(row-1)),row==selected ? 12 : 10);
    }
    char text[48];
    sprintf(text,"M: Music %s",sound.musicEnabled()?"ON":"OFF");
    screen.drawText(140,282,text,10);
    sprintf(text,"E: Effects %s",sound.effectsEnabled()?"ON":"OFF");
    screen.drawText(332,282,text,10);
    sprintf(text,"B: SB port %03X",sbPort);screen.drawText(140,306,text,13);
    sprintf(text,"P: MIDI port %03X",mpuPort);screen.drawText(332,306,text,13);
    sprintf(text,"Active: %s",Sound::deviceName(sound.activeDevice()));screen.drawText(140,334,text,10);
    screen.drawText(140,354,notice ? notice : sound.status(),13);
    screen.drawLine(136,376,504,376,13);
    screen.fillRect(140,382,168,22,10);screen.fillRect(141,383,166,20,12);
    screen.fillRect(332,382,168,22,10);screen.fillRect(333,383,166,20,12);
    screen.drawText(180,389,"APPLY Enter",10);
    screen.drawText(380,389,"CLOSE Esc",10);
    screen.drawText(140,408,"Arrows: select     Settings are saved",13);
}

int SoundMenu::handleKey(int key)
{
    if(key==27)return 0;
    notice=0;
    if(key==0x148 && selected>0)--selected;
    else if(key==0x150 && selected<4)++selected;
    else if(key==13) {
        sound.setPorts(sbPort,mpuPort);
        sound.configure((Sound::Device)(selected-1));
        sound.playEffect(Sound::Select);
        if(!sound.saveSettings())notice="Cannot save SOUND.CFG";
    } else if(key=='m' || key=='M') {
        sound.toggleMusic();if(!sound.saveSettings())notice="Cannot save SOUND.CFG";
    } else if(key=='e' || key=='E') {
        sound.toggleEffects();sound.playEffect(Sound::Click);
        if(!sound.saveSettings())notice="Cannot save SOUND.CFG";
    } else if(key=='b' || key=='B') {
        sbPort=sbPort==0x280 ? 0x220 : sbPort+0x20;
        notice="Press Enter to apply the new ports";
    } else if(key=='p' || key=='P') {
        mpuPort=mpuPort==0x330 ? 0x300 : mpuPort+0x10;
        notice="Press Enter to apply the new ports";
    } else return 1;
    drawContents();
    return 1;
}

// Compact controls shared by the journal and puzzle. No palette changes:
// they use the host screen's paper, brown, rose and gold colors.
void SoundMenu::drawButtons(VgaScreen &screen, const Sound *sound, int y)
{
    if(!sound)return;
    static const unsigned short speaker[12]={
        0x008,0x018,0x038,0x1F8,0x1FA,0x1F9,
        0x1F9,0x1FA,0x1F8,0x038,0x018,0x008
    };
    static const unsigned short note[12]={
        0x01F,0x01F,0x011,0x011,0x011,0x011,
        0x011,0x071,0x0F7,0x0F7,0x066,0x000
    };
    static const unsigned short cog[12]={
        0x038,0x0BA,0x1FF,0x1C7,0x183,0x183,
        0x183,0x1C7,0x1FF,0x0BA,0x038,0x000
    };
    screen.setDrawingScale(1);
    for(int i=0;i<3;++i) {
        int x=508+i*40;
        int enabled=i==2 || (i ? sound->musicEnabled() : sound->effectsEnabled());
        screen.fillRect(x+2,y+2,34,32,3);
        screen.fillRect(x,y,34,32,enabled ? 6 : 1);
        screen.fillRect(x+1,y+1,32,30,enabled ? 0 : 2);
        unsigned char ink=enabled ? 2 : 0;
        screen.drawMonoRows(x+11,y+4,i==2 ? cog : i ? note : speaker,9,12,ink);
        if(!enabled)screen.drawLine(x+8,y+16,x+25,y+3,ink);
        screen.drawText(x+9,y+22,i==2 ? "F5" : i ? "F4" : "F3",ink);
    }
}

int SoundMenu::toggleKey(Sound *sound, int key)
{
    if(!sound)return 0;
    if(key==0x13D || key=='e' || key=='E')sound->toggleEffects();
    else if(key==0x13E || key=='m' || key=='M')sound->toggleMusic();
    else return 0;
    sound->saveSettings();
    return 1;
}

int SoundMenu::buttonKey(int x,int y,int top)
{
    if(y<top || y>=top+32)return 0;
    for(int i=0;i<3;++i)
        if(x>=508+i*40 && x<542+i*40)return 0x13D+i;
    return 0;
}

int SoundMenu::handleMouse(int x,int y,int button)
{
    if(button==2)return handleKey(27);
    if(button!=1)return 1;
    if(x>=136 && x<504 && y>=126 && y<266) {
        int row=(y-126)/28;
        if(y<126+row*28+24) { selected=row;notice=0;drawContents(); }
    } else if(x>=140 && x<500 && y>=278 && y<298)
        return handleKey(x<320 ? 'm' : 'e');
    else if(x>=140 && x<500 && y>=302 && y<322)
        return handleKey(x<320 ? 'b' : 'p');
    else if(x>=140 && x<308 && y>=382 && y<404)return handleKey(13);
    else if(x>=332 && x<500 && y>=382 && y<404)return handleKey(27);
    return 1;
}

HoverRect SoundMenu::buttonHover(int x,int y,int top,unsigned char dialog)
{
    int key=buttonKey(x,y,top);
    return key ? HoverRect(508+(key-0x13D)*40,top,34,32,dialog) : HoverRect();
}

HoverRect SoundMenu::hoverAt(int x,int y) const
{
    if(x>=136 && x<504 && y>=126 && y<266) {
        int row=(y-126)/28;
        if(y<126+row*28+24)return HoverRect(136,126+row*28,368,24,1);
    }
    if(x>=140 && x<500 && y>=278 && y<298)
        return HoverRect(x<320 ? 140 : 320,278,180,20,1);
    if(x>=140 && x<500 && y>=302 && y<322)
        return HoverRect(x<320 ? 140 : 320,302,180,20,1);
    if(y>=382 && y<404) {
        if(x>=140 && x<308)return HoverRect(140,382,168,22,1);
        if(x>=332 && x<500)return HoverRect(332,382,168,22,1);
    }
    return HoverRect();
}
