#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "GAME.H"
#include "DIGITS.H"
#include "START.H"
#include "MENU.H"
#include "DOCVIEW.H"
#include "SOUNDMNU.H"
#include "MOUSE.H"

unsigned _stklen = 16384;

struct Input { int key, x, y, button, motion; };

static Input readInput(Sound &sound,Mouse &mouse,Hover &hover)
{
    Input input={0,0,0,0,0};
    // Show only while waiting. Every caller draws with the cursor hidden,
    // so a driver's saved cursor background cannot overwrite our VGA pixels.
    mouse.show();
    for(;;) {
        sound.service();
        if(kbhit()) {
            input.key=getch();
            if(input.key==0 || input.key==0xE0)input.key=0x100 | getch();
            // Keep button edges current even while typing.
            mouse.poll(input.x,input.y);
            break;
        }
        int event=mouse.poll(input.x,input.y);
        if(event==Mouse::Motion) { input.motion=1;break; }
        if(event) { input.button=event;break; }
    }
    mouse.hide();
    if(!input.motion) { hover.clear();mouse.refreshPosition(); }
    return input;
}

static void soundPanel(VgaScreen &screen,Sound &sound,Mouse &mouse,Hover &hover)
{
    SoundMenu panel(screen,sound);
    panel.draw();
    int running=1;
    while(running) {
        Input input=readInput(sound,mouse,hover);
        if(input.motion) { hover.update(panel.hoverAt(input.x,input.y));continue; }
        running=input.button ? panel.handleMouse(input.x,input.y,input.button)
                             : panel.handleKey(input.key);
    }
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
    Mouse mouse;
    mouse.initialize();
    Hover hover(screen);
    game.setMouseAvailable(mouse.available());
    sound.initialize();
    sound.playMusic(Sound::MenuMusic);
    // Discard keys buffered during startup, matching the CX16 splash.
    while (kbhit()) getch();
    int choice = 0;
    while (!choice) {
        Input input=readInput(sound,mouse,hover);
        if(input.motion)continue;
        choice=start.handleKey(input.button ? (input.button==1 ? 13 : 27) : input.key);
    }
    if (choice < 0) return 0;
    MenuScreen menu(screen);
    menu.setSound(&sound);
    menu.loadCatalog(filename);
    menu.draw();
    int currentPuzzle = -1;
    for (;;) {
        Input input=readInput(sound,mouse,hover);
        if(input.motion) { hover.update(menu.hoverAt(input.x,input.y));continue; }
        int oldSelection=menu.selectedIndex(),oldPage=menu.pageNumber();
        int action=input.button ? menu.handleMouse(input.x,input.y,input.button)
                                : menu.handleKey(input.key);
        if (action == MenuScreen::Exit) break;
        if(menu.selectedIndex()!=oldSelection)
            sound.playEffect(menu.pageNumber()!=oldPage ? Sound::Page : Sound::Click);
        if(action == MenuScreen::Audio) {
            sound.playEffect(Sound::Select);
            soundPanel(screen,sound,mouse,hover);
            menu.draw();
            continue;
        }
        if (action == MenuScreen::Help || action == MenuScreen::About) {
            sound.playEffect(Sound::Select);
            DocumentScreen document(screen);
            document.show(action == MenuScreen::Help ? DocumentScreen::Help : DocumentScreen::About);
            int reading=1;
            while(reading) {
                input=readInput(sound,mouse,hover);
                if(input.motion) { hover.update(document.hoverAt(input.x,input.y));continue; }
                reading=input.button ? document.handleMouse(input.x,input.y,input.button)
                                     : document.handleKey(input.key);
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
        sound.playPuzzleMusic(menu.entry(selected)->id);
        sound.playEffect(game.completionModalVisible() ? Sound::Solved : Sound::Select);
        game.draw();
        int running = 1;
        while (running) {
            input=readInput(sound,mouse,hover);
            if(input.motion) { hover.update(game.hoverAt(input.x,input.y));continue; }
            int action=input.button ? game.handleMouse(input.x,input.y,input.button)
                                    : input.key==0x13F ? Game::AudioSettings
                                    : game.handleKey(input.key);
            if(action==Game::AudioSettings) {
                soundPanel(screen,sound,mouse,hover);
                game.draw();
                continue;
            }
            running=action;
        }
        menu.markPlayed(selected, game.isCompleted());
        sound.playMusic(Sound::MenuMusic);
        menu.draw();
    }
    return 0;
}
