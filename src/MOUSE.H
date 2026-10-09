#ifndef KAKURO_MOUSE_H
#define KAKURO_MOUSE_H

// Optional Microsoft-compatible DOS mouse driver (INT 33h).
class Mouse {
public:
    Mouse() : installed(0), visible(0), previous(0), previousX(-1), previousY(-1) {}
    ~Mouse() { hide(); }
    int initialize();
    int available() const { return installed; }
    void show();
    void hide();
    enum { Motion = 3 };
    void refreshPosition() { previousX = previousY = -1; }
    // Return a new button (1 left, 2 right), Motion, or zero.
    int poll(int &x, int &y);
private:
    int installed, visible;
    unsigned previous;
    int previousX, previousY;
    Mouse(const Mouse &);
    Mouse &operator=(const Mouse &);
};
#endif
