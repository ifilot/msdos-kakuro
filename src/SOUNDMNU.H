#ifndef KAKURO_SOUNDMNU_H
#define KAKURO_SOUNDMNU_H
#include "VGA.H"
#include "HOVER.H"
#include "SOUND.H"
class SoundMenu {
public:
    SoundMenu(VgaScreen &screen, Sound &sound);
    void draw();
    static void drawButtons(VgaScreen &screen, const Sound *sound, int y);
    static HoverRect buttonHover(int x, int y, int top, unsigned char dialog=0);
    static int buttonKey(int x, int y, int top);
    HoverRect hoverAt(int x, int y) const;
    int handleMouse(int x, int y, int button);
    static int toggleKey(Sound *sound, int key);
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
