#ifndef KAKURO_BLOSSOM_H
#define KAKURO_BLOSSOM_H
#include "VGA.H"

class Blossom {
public:
    // 13x13 difficulty icon; shared warm UI palette entries 1,6,7,8.
    static void draw(VgaScreen &screen, int x, int y, int filled);
};
#endif
