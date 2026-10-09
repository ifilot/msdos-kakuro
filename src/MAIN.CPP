#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "GAME.H"
#include "DIGITS.H"
#include "START.H"
#include "MENU.H"
#include "DOCVIEW.H"
#include "SOUNDMNU.H"

unsigned _stklen = 16384;

static int readKey(Sound &sound)
{
    // Service the MIDI UART between ticks without blocking in BIOS getch.
    sound.service();
    while(!kbhit())sound.service();
    int key=getch();
    if(key==0 || key==0xE0)key=0x100 | getch();
    return key;
}

static void soundPanel(VgaScreen &screen,Sound &sound)
{
    SoundMenu panel(screen,sound);
    panel.draw();
    while(panel.handleKey(readKey(sound))) {}
}

int main(int argc, char **argv)
{
    VgaScreen screen;
    if (argc > 1 && !strcmp(argv[1], "/FONTS")) {
        screen.initializeModeX();
        DigitFonts::drawSamples(screen);
        getch();
        return 0;
    }
    Sound sound;
    Game game(screen);
    game.setSound(&sound);
    const char *filename = argc > 1 ? argv[1] : "PUZZLES\\001.PUZ";
    StartScreen start(screen);
    if (!start.show()) {
        fprintf(stderr, "Cannot load the start screen from KAKURO.DAT\n");
        return 1;
    }
    sound.initialize();
    sound.playMusic(Sound::MenuMusic);
    // Discard keys buffered during startup, matching the CX16 splash.
    while (kbhit()) getch();
    int choice = 0;
    while (!choice) {
        int key = readKey(sound);
        choice = start.handleKey(key);
    }
    if (choice < 0) return 0;
    MenuScreen menu(screen);
    menu.loadCatalog(filename);
    menu.draw();
    int currentPuzzle = -1;
    for (;;) {
        int key = readKey(sound);
        int oldSelection=menu.selectedIndex(),oldPage=menu.pageNumber();
        int action = menu.handleKey(key);
        if (action == MenuScreen::Exit) break;
        if(menu.selectedIndex()!=oldSelection)
            sound.playEffect(menu.pageNumber()!=oldPage ? Sound::Page : Sound::Click);
        if(action == MenuScreen::Audio) {
            sound.playEffect(Sound::Select);
            soundPanel(screen,sound);
            menu.draw();
            continue;
        }
        if (action == MenuScreen::Help || action == MenuScreen::About) {
            sound.playEffect(Sound::Select);
            DocumentScreen document(screen);
            document.show(action == MenuScreen::Help ? DocumentScreen::Help : DocumentScreen::About);
            int reading=1;
            while(reading) {
                key=readKey(sound);
                reading=document.handleKey(key);
            }
            sound.playEffect(Sound::Back);
            menu.draw();
            continue;
        }
        if (action != MenuScreen::Play) continue;
        int selected = menu.selectedIndex();
        if (selected != currentPuzzle) {
            if (!game.loadPuzzle(menu.selectedFile())) {
                menu.showError("Cannot load this puzzle");
                continue;
            }
            currentPuzzle = selected;
        }
        menu.markPlayed(selected, game.isCompleted());
        sound.playMusic(Sound::GameMusic);
        sound.playEffect(game.completionModalVisible() ? Sound::Solved : Sound::Select);
        game.draw();
        int running = 1;
        while (running) {
            key = readKey(sound);
            if(key==0x13D) {
                soundPanel(screen,sound);
                game.draw();
                continue;
            }
            running = game.handleKey(key);
        }
        menu.markPlayed(selected, game.isCompleted());
        sound.playMusic(Sound::MenuMusic);
        menu.draw();
    }
    return 0;
}
