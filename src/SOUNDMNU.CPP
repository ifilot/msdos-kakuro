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
    screen.setPaletteColor(Frame,16,12,12);screen.setPaletteColor(Gold,63,52,28);
    screen.setPaletteColor(Face,55,53,42);screen.setPaletteColor(Accent,33,25,25);
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
        screen.drawText(172,y,Sound::deviceName((Sound::Device)(row-1)),row==selected ? 12 : 10);
    }
    char text[48];
    sprintf(text,"M: Music %s   E: Effects %s",sound.musicEnabled()?"ON":"OFF",
        sound.effectsEnabled()?"ON":"OFF");screen.drawText(140,282,text,10);
    sprintf(text,"B: SB port %03X   P: MIDI port %03X",sbPort,mpuPort);
    screen.drawText(140,306,text,13);
    sprintf(text,"Active: %s",Sound::deviceName(sound.activeDevice()));screen.drawText(140,334,text,10);
    screen.drawText(140,354,notice ? notice : sound.status(),13);
    screen.drawLine(136,376,504,376,13);
    screen.drawText(140,390,"Arrows: select  Enter: apply/test",10);
    screen.drawText(140,406,"Esc: close      Settings are saved",13);
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
