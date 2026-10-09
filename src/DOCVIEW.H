#ifndef KAKURO_DOCVIEW_H
#define KAKURO_DOCVIEW_H
#include "VGA.H"
class DocumentScreen {
public:
    enum Document { Help, About };
    enum { VisibleLines = 18 };
    DocumentScreen(VgaScreen &display);
    void show(Document document);
    int handleKey(int key); // 0 returns to the journal; 1 keeps reading.
    int topLine() const { return top; }
    int lineCount() const { return count; }
private:
    VgaScreen &screen;
    Document current;
    int top, count;
    void renderText();
    void showAbout();
};
#endif
