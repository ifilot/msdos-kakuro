#include "ASSETS.H"
#include <string.h>

static unsigned long littleLong(const unsigned char *bytes)
{
    return (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8)
        | ((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
}

AssetStream::AssetStream() : file(0), position(0), available(0),
    packetRemaining(0), packedRemaining(0), rawRemaining(0),
    repeatValue(0), repeated(0), planeCount(0)
{
    memset(rgb, 0, sizeof(rgb));
}

AssetStream::~AssetStream() { close(); }

void AssetStream::close()
{
    if (file) fclose(file);
    file = 0;
    position = available = packetRemaining = 0;
    packedRemaining = rawRemaining = 0;
    planeCount = 0;
}

int AssetStream::open(const char *bitmapName, const char *packName)
{
    close();
    // Explicit custom paths continue to use the ordinary loose-file loaders.
    if (!bitmapName || strchr(bitmapName, '\\') || strchr(bitmapName, '/')
        || strchr(bitmapName, ':')) return 0;
    file = fopen(packName, "rb");
    if (!file) return 0;
    unsigned char header[9], entry[37];
    if (fread(header, 1, 9, file) != 9
        || memcmp(header, "KAKPAK1", 8) || !header[8] || header[8] > 32) {
        close(); return 0;
    }
    for (int i = 0; i < header[8]; ++i) {
        if (fread(entry, 1, 37, file) != 37) { close(); return 0; }
        if (!memchr(entry, 0, 12)) { close(); return 0; }
        if (stricmp((const char *)entry, bitmapName)) continue;
        unsigned long offset = littleLong(entry+12);
        packedRemaining = littleLong(entry+16);
        rawRemaining = littleLong(entry+20);
        planeCount = entry[24];
        memcpy(rgb, entry+25, 12);
        for (int color = 0; color < 12; ++color)
            if (rgb[color] > 63) { close(); return 0; }
        if (planeCount != 2 || rawRemaining != 76800UL || !packedRemaining
            || offset < 9UL + (unsigned long)header[8]*37UL
            || offset > 0x7FFFFFFFUL || packedRemaining > 0x7FFFFFFFUL-offset
            || fseek(file, 0L, SEEK_END)) { close(); return 0; }
        long size = ftell(file);
        if (size < 0 || offset + packedRemaining > (unsigned long)size
            || fseek(file, (long)offset, SEEK_SET)) { close(); return 0; }
        return 1;
    }
    close();
    return 0;
}

int AssetStream::refill()
{
    unsigned int amount = packedRemaining > sizeof(input)
        ? sizeof(input) : (unsigned int)packedRemaining;
    if (!amount || fread(input, 1, amount, file) != amount) return 0;
    packedRemaining -= amount;
    position = 0;
    available = amount;
    return 1;
}

int AssetStream::nextByte()
{
    if (position == available && !refill()) return -1;
    return input[position++];
}

int AssetStream::read(unsigned char *destination, unsigned int length)
{
    if (!file || (unsigned long)length > rawRemaining) return 0;
    unsigned int remaining = length;
    while (remaining) {
        if (!packetRemaining) {
            int control = nextByte();
            if (control < 0) return 0;
            repeated = control & 128;
            packetRemaining = repeated ? control-127 : control+1;
            if ((unsigned long)packetRemaining > rawRemaining) return 0;
            if (repeated) {
                int value = nextByte();
                if (value < 0) return 0;
                repeatValue = (unsigned char)value;
            }
        }
        unsigned int amount = remaining < packetRemaining ? remaining : packetRemaining;
        if (repeated) memset(destination, repeatValue, amount);
        else {
            if (position == available && !refill()) return 0;
            if (amount > available-position) amount = available-position;
            memcpy(destination, input+position, amount);
            position += amount;
        }
        destination += amount;
        remaining -= amount;
        rawRemaining -= amount;
        packetRemaining -= amount;
    }
    // Reject extra packets/trailing bytes as well as truncated output.
    return rawRemaining || (!packetRemaining && !packedRemaining && position == available);
}
