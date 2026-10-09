#ifndef KAKURO_MENU_H
#define KAKURO_MENU_H
#include "VGA.H"
#include "HOVER.H"
#include "SOUND.H"

class MenuScreen {
public:
    enum { MaxPuzzles = 101, PageSize = 24, Columns = 6 };
    enum Result { Exit = -1, Wait = 0, Play = 1, Help = 2, About = 3, Audio = 4 };
    enum Status { Ready, Played, Solved };
    struct Entry {
        char filename[20];
        unsigned char rows, columns, difficulty, status;
        unsigned int id; // numbered puzzle identifier (1-100).
    };
    MenuScreen(VgaScreen &display);
    int loadCatalog(const char *preferred = "001.PUZ");
    void draw();
    void setSound(Sound *audio) { sound = audio; }
    HoverRect hoverAt(int x, int y) const;
    int handleMouse(int x, int y, int button);
    int handleKey(int key);
    int count() const { return puzzleCount; }
    int selectedIndex() const { return selected; }
    int pageNumber() const { return selected / PageSize; }
    int pageCount() const { return (puzzleCount + PageSize - 1) / PageSize; }
    const Entry *entry(int index) const;
    const char *selectedFile() const;
    void markPlayed(int index, int solved);
    void showError(const char *message);
private:
    VgaScreen &screen;
    Sound *sound;
    Entry entries[MaxPuzzles];
    int puzzleCount, selected, visible;
    char errorText[48];
    void drawCard(int index);
    void drawDetails();
    void drawPageNumber();
};
#endif
