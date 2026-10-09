#ifndef KAKURO_ASSETS_H
#define KAKURO_ASSETS_H

#include <stdio.h>

// Buffered resource stream. No whole image or >64KB allocation is needed.
class AssetStream {
public:
    AssetStream();
    ~AssetStream();
    int open(const char *bitmapName, const char *packName = "KAKURO.DAT");
    int read(unsigned char *destination, unsigned int length);
    void close();
    int planes() const { return planeCount; }
    const unsigned char *palette() const { return rgb; }

private:
    FILE *file;
    unsigned char input[2048], rgb[12];
    unsigned int position, available, packetRemaining;
    unsigned long packedRemaining, rawRemaining;
    unsigned char repeatValue;
    int repeated, planeCount;
    int refill();
    int nextByte();
    AssetStream(const AssetStream &);
    AssetStream &operator=(const AssetStream &);
};

#endif
