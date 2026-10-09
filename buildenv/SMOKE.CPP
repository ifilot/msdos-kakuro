#include <stdio.h>
#include <dos.h>
#include <sys/stat.h>

class Sum {
    int total;
public:
    Sum() : total(0) {}
    void add(int value) { total += value; }
    int value() const { return total; }
};

int main()
{
    Sum sum;
    for (int digit = 1; digit <= 9; ++digit)
        sum.add(digit);
    if (sizeof(int) != 2 || sizeof(void far *) != 4 || sum.value() != 45)
        return 1;
    FILE *result = fopen("RESULT.TXT", "w");
    if (!result)
        return 2;
    fprintf(result, "Borland C++ DOS smoke test passed: sum=%d\n", sum.value());
    return fclose(result) == 0 ? 0 : 3;
}
