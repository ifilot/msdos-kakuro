#include "ASSETS.H"
#include <stdio.h>
#include <string.h>

// Decoder tests need no VGA mode and also run on an 8088.
unsigned _stklen = 16384;
static int failures = 0;

static void check(int good, const char *message)
{
    if (!good) { printf("FAIL: %s\n", message); ++failures; }
}

static void compare(const char *name, unsigned int chunk)
{
    AssetStream stream;
    unsigned char decoded[2048], original[2048];
    FILE *source = fopen(name, "rb");
    check(source != 0, "loose comparison image exists");
    if (!source) return;
    int valid = stream.open(name);
    check(valid, "packed image opens");
    if (!valid) { fclose(source); return; }
    check(stream.planes() == 2, "two stored image planes");
    unsigned long remaining = 76800UL;
    while (remaining && valid) {
        unsigned int amount = remaining < chunk ? (unsigned int)remaining : chunk;
        valid = stream.read(decoded, amount)
            && fread(original, 1, amount, source) == amount
            && !memcmp(decoded, original, amount);
        remaining -= amount;
    }
    check(valid, "all expanded bytes match original image");
    check(!stream.read(decoded, 1), "expanded data has exact length");
    fclose(source);
    stream.close();
    stream.close();
}

static int badFixture(int trailing, int badPalette)
{
    FILE *file = fopen("BAD.DAT", "wb");
    if (!file) return 0;
    unsigned char header[9] = {'K','A','K','P','A','K','1',0,1};
    unsigned char entry[37] = {0};
    memcpy(entry, "START.VGA", 9);
    entry[12] = 46; // 9-byte header + 37-byte directory.
    unsigned int packed = trailing ? 1202 : 2;
    entry[16] = packed & 255; entry[17] = packed >> 8;
    entry[20] = 0; entry[21] = 44; entry[22] = 1; // 76800.
    entry[24] = 2;
    entry[25] = badPalette ? 64 : 0;
    fwrite(header, 1, 9, file);
    fwrite(entry, 1, 37, file);
    unsigned char repeat[2] = {255, 0};
    for (int i = 0; i < (trailing ? 600 : 1); ++i)
        fwrite(repeat, 1, 2, file);
    if (trailing) { repeat[0] = 0; fwrite(repeat, 1, 2, file); }
    return !fclose(file);
}

int main()
{
    compare("START.VGA", 80);
    compare("BOARD.VGA", 2048);
    compare("MENU.VGA", 127);
    AssetStream stream;
    unsigned char buffer[2048];
    check(!stream.open("MISSING.VGA"), "unknown packed image rejected");
    check(!stream.open("custom/START.VGA"), "custom paths use loose loader");
    check(stream.open("start.vga"), "case insensitive DOS names");
    stream.close();

    check(badFixture(0, 1), "bad palette fixture written");
    check(!stream.open("START.VGA", "BAD.DAT"), "invalid DAC value rejected");
    check(badFixture(0, 0), "truncated stream fixture written");
    check(stream.open("START.VGA", "BAD.DAT"), "valid directory opens");
    check(!stream.read(buffer, 129), "truncated packet stream rejected");
    stream.close();
    check(badFixture(1, 0), "trailing packet fixture written");
    int valid = stream.open("START.VGA", "BAD.DAT");
    unsigned long remaining = 76800UL;
    while (remaining && valid) {
        unsigned int amount = remaining > sizeof(buffer) ? sizeof(buffer) : (unsigned int)remaining;
        valid = stream.read(buffer, amount);
        remaining -= amount;
    }
    check(!valid, "extra compressed packet rejected");
    stream.close();
    remove("BAD.DAT");
    FILE *result = fopen("RESULT.TXT", "w");
    if (!result) return 2;
    fprintf(result, failures ? "Asset tests failed\n" : "Asset tests passed\n");
    if (fclose(result)) return 2;
    return failures ? 1 : 0;
}
