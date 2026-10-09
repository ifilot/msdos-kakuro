#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "PUZZLE.H"

Puzzle::Puzzle() : rowCount(0), columnCount(0), stars(0)
{
    memset(cells, 0, sizeof(cells));
    sourceText[0] = errorText[0] = 0;
}

int Puzzle::fail(const char *message)
{
    strncpy(errorText, message, sizeof(errorText) - 1);
    errorText[sizeof(errorText) - 1] = 0;
    return 0;
}

static int packedIdentifier(const char *filename)
{
    const char *name = filename;
    if (!strnicmp(name, "PUZZLES\\", 8) || !strnicmp(name, "PUZZLES/", 8)) name += 8;
    if (strlen(name) != 7 || stricmp(name + 3, ".PUZ")) return -1;
    if (!strnicmp(name, "XXX", 3)) return 0;
    if (name[0] < '0' || name[0] > '9' || name[1] < '0' || name[1] > '9'
        || name[2] < '0' || name[2] > '9') return -1;
    int id = (name[0]-'0')*100 + (name[1]-'0')*10 + name[2]-'0';
    return id >= 1 && id <= 100 ? id : -1;
}

int Puzzle::load(const char *filename)
{
    int id = packedIdentifier(filename);
    if (id >= 0) return loadPacked(id);
    Puzzle candidate;
    if (!candidate.read(filename))
        return fail(candidate.error());
    *this = candidate;
    return 1;
}

int Puzzle::loadPacked(unsigned int id, const char *archive)
{
    Puzzle candidate;
    if (!candidate.readPacked(id, archive)) return fail(candidate.error());
    *this = candidate;
    return 1;
}

static unsigned int littleWord(const unsigned char *bytes)
{
    return bytes[0] | ((unsigned int)bytes[1] << 8);
}

int Puzzle::readPacked(unsigned int id, const char *archive)
{
    if (id > 100) return fail("Unknown puzzle number");
    FILE *file = fopen(archive, "rb");
    if (!file) return fail("Cannot open PUZZLES.DAT");
    unsigned char header[10], directory[9], packed[240], raw[240];
    int valid = fread(header, 1, 10, file) == 10
        && !memcmp(header, "KAKPUZ1", 8) && littleWord(header+8) == 101;
    if (!valid || fseek(file, 10L + id * 9U, SEEK_SET)
        || fread(directory, 1, 9, file) != 9) {
        fclose(file); return fail("Invalid puzzle archive index");
    }
    unsigned int offset = littleWord(directory), length = littleWord(directory+2);
    unsigned int size = littleWord(directory+4), expectedSum = littleWord(directory+7);
    int codec = directory[6];
    if (!offset) { fclose(file); return fail("Puzzle is not in the archive"); }
    if (offset < 919U || !length || length > 240U || size < 4U || size > 240U
        || codec > 1 || (!codec && length != size) || fseek(file, 0L, SEEK_END)) {
        fclose(file); return fail("Invalid puzzle archive entry");
    }
    long fileSize = ftell(file);
    if (fileSize < 0 || (long)offset + length > fileSize || fileSize > 65535L
        || fseek(file, offset, SEEK_SET) || fread(packed, 1, length, file) != length) {
        fclose(file); return fail("Truncated puzzle archive");
    }
    if (fclose(file)) return fail("Error reading puzzle archive");
    unsigned int input = 0, output = 0;
    if (!codec) { memcpy(raw, packed, size); output = size; }
    else while (input < length && output < size) {
        unsigned int control = packed[input++];
        unsigned int count = control & 128 ? control - 127 : control + 1;
        if (count > size - output) return fail("Invalid puzzle compression");
        if (control & 128) {
            if (input == length) return fail("Truncated puzzle repeat");
            memset(raw + output, packed[input++], count);
        } else {
            if (count > length - input) return fail("Truncated puzzle literal");
            memcpy(raw + output, packed + input, count); input += count;
        }
        output += count;
    }
    if (output != size || (codec && input != length)) return fail("Invalid puzzle payload length");
    unsigned int sum = 0;
    unsigned int i;
    for (i = 0; i < size; ++i) sum += raw[i];
    if (sum != expectedSum) return fail("Puzzle checksum mismatch");
    rowCount = raw[0]; columnCount = raw[1]; stars = raw[2];
    unsigned int sourceLength = raw[3];
    if (!rowCount || rowCount > MaxDimension || !columnCount || columnCount > MaxDimension
        || stars > 9 || sourceLength >= sizeof(sourceText)
        || size != 4U + sourceLength + (rowCount * columnCount * 5U + 7U) / 8U)
        return fail("Invalid packed puzzle dimensions");
    for (i = 0; i < sourceLength; ++i)
        if (raw[4+i] < 32 || raw[4+i] > 126) return fail("Invalid puzzle metadata");
    memcpy(sourceText, raw+4, sourceLength); sourceText[sourceLength] = 0;
    unsigned int bits = 0, available = 0;
    input = 4 + sourceLength;
    for (i = 0; i < (unsigned int)(rowCount * columnCount); ++i) {
        if (available < 5) { bits |= (unsigned int)raw[input++] << available; available += 8; }
        unsigned char code = bits & 31;
        bits >>= 5; available -= 5;
        if (code > 18) return fail("Invalid packed puzzle cell");
        int hint = code >= 10;
        unsigned char solution = hint ? code - 9 : code;
        Cell &target = cells[i];
        target.type = solution ? Answer : Blocked;
        target.solution = solution; target.value = hint ? solution : 0;
        target.revealed = (unsigned char)hint;
        target.locked = (unsigned char)(hint || !solution);
    }
    if (bits) return fail("Invalid puzzle padding");
    deriveClues();
    return 1;
}

const Puzzle::Cell *Puzzle::cell(int row, int column) const
{
    if (row < 0 || column < 0 || row >= rowCount || column >= columnCount)
        return 0;
    return &cells[row * columnCount + column];
}

int Puzzle::setValue(int row, int column, int value)
{
    const Cell *target = cell(row, column);
    if (!target || value < 0 || value > 9 || target->type != Answer || target->locked)
        return 0;
    cells[row * columnCount + column].value = (unsigned char)value;
    return 1;
}

int Puzzle::isSolved() const
{
    if (!rowCount || !columnCount) return 0;
    unsigned char coverage[MaxCells];
    memset(coverage, 0, sizeof(coverage));
    int answers = 0;
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const Cell &target = cells[row * columnCount + col];
            if (target.type == Answer) {
                if (target.value < 1 || target.value > 9) return 0;
                ++answers;
                continue;
            }
            for (int direction = 0; direction < 2; ++direction) {
                int clue = direction ? target.down : target.across;
                if (!clue) continue;
                int sum = 0, count = 0;
                unsigned int used = 0;
                int y = row + (direction ? 1 : 0);
                int x = col + (direction ? 0 : 1);
                while (y < rowCount && x < columnCount) {
                    const Cell &entry = cells[y * columnCount + x];
                    if (entry.type != Answer) break;
                    if (entry.value < 1 || entry.value > 9) return 0;
                    unsigned int bit = 1U << entry.value;
                    if (used & bit) return 0;
                    used |= bit;
                    sum += entry.value;
                    ++count;
                    coverage[y * columnCount + x] |= direction ? 2 : 1;
                    y += direction ? 1 : 0;
                    x += direction ? 0 : 1;
                }
                if (count < 2 || sum != clue) return 0;
            }
        }
    }
    if (!answers) return 0;
    for (int i = 0; i < rowCount * columnCount; ++i)
        // The source format omits clues for single-cell runs. Every answer
        // must still participate in at least one displayed clue.
        if (cells[i].type == Answer && !coverage[i]) return 0;
    return 1;
}

int Puzzle::read(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) return fail("Cannot open puzzle file");
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (!strchr(line, '\n') && !feof(file)) {
            fclose(file);
            return fail("Puzzle line is too long");
        }
        char *text = line;
        while (isspace((unsigned char)*text)) ++text;
        if (!*text) continue;
        if (*text == '#' && !isdigit((unsigned char)text[1])) {
            if (!rowCount && !sourceText[0]) {
                ++text;
                while (isspace((unsigned char)*text)) ++text;
                text[strcspn(text, "\r\n")] = 0;
                strncpy(sourceText, text, sizeof(sourceText) - 1);
                sourceText[sizeof(sourceText) - 1] = 0;
                const char *lastSlash = strrchr(sourceText, '/');
                if (lastSlash && lastSlash[1] >= '1' && lastSlash[1] <= '9'
                    && lastSlash[2] == '*') stars = lastSlash[1] - '0';
            }
            continue;
        }
        if (rowCount >= MaxDimension) {
            fclose(file);
            return fail("Too many puzzle rows");
        }
        int column = 0;
        // Whitespace-separated 0..9 tokens, optionally prefixed by # for hints.
        while (*text) {
            while (isspace((unsigned char)*text)) ++text;
            if (!*text) break;
            int hint = *text == '#';
            if (hint) ++text;
            if (*text < '0' || *text > '9' || (hint && *text == '0')) {
                fclose(file);
                return fail("Invalid puzzle cell (expected 0..9 or #1..#9)");
            }
            unsigned char solution = *text++ - '0';
            if (*text && !isspace((unsigned char)*text)) {
                fclose(file);
                return fail("Puzzle cells must be separated by whitespace");
            }
            if (column >= MaxDimension) {
                fclose(file);
                return fail("Too many puzzle columns");
            }
            // Use a temporary fixed stride until the row width is known.
            Cell &target = cells[rowCount * MaxDimension + column++];
            target.type = solution ? Answer : Blocked;
            target.solution = solution;
            target.value = hint ? solution : 0;
            target.revealed = (unsigned char)hint;
            target.locked = (unsigned char)(hint || !solution);
        }
        if (!rowCount) columnCount = column;
        if (column != columnCount) {
            fclose(file);
            return fail("Puzzle rows have different lengths");
        }
        ++rowCount;
    }
    int ioError = ferror(file);
    if (fclose(file)) ioError = 1;
    if (ioError) return fail("Error reading puzzle file");
    if (!rowCount || !columnCount) return fail("Puzzle file is empty");
    for (int row = 0; row < rowCount; ++row)
        for (int col = 0; col < columnCount; ++col)
            cells[row * columnCount + col] = cells[row * MaxDimension + col];
    deriveClues();
    return 1;
}

void Puzzle::deriveClues()
{
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            Cell &target = cells[row * columnCount + col];
            if (target.type == Answer) continue;
            int count = 0, sum = 0;
            for (int x = col + 1; x < columnCount; ++x) {
                unsigned char value = cells[row * columnCount + x].solution;
                if (!value) break;
                sum += value;
                ++count;
            }
            if (count >= 2) target.across = (unsigned char)sum;
            count = sum = 0;
            for (int y = row + 1; y < rowCount; ++y) {
                unsigned char value = cells[y * columnCount + col].solution;
                if (!value) break;
                sum += value;
                ++count;
            }
            if (count >= 2) target.down = (unsigned char)sum;
            if (target.across || target.down) target.type = Clue;
        }
    }
}
