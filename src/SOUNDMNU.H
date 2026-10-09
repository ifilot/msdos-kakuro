#ifndef KAKURO_SOUNDMNU_H
#define KAKURO_SOUNDMNU_H
#include "VGA.H"
#include "SOUND.H"
class SoundMenu {
public:
    SoundMenu(VgaScreen &screen, Sound &sound);
    void draw();
    int handleKey(int key);
    int selectedIndex() const { return selected; }
private:
    VgaScreen &screen;
    Sound &sound;
    int selected;
    unsigned sbPort, mpuPort;
    const char *notice;
    void drawContents();
};
#endif
