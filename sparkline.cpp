#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "sparkline.h"
#include "utils.h"

#define MAXCHARS 256 // pretty arbitrary

extern const wchar_t *sparkchars;

void sparky(char *instr)
{
    char str[MAXCHARS];
    strcpy(str, instr);

    double sparkvals[MAXCHARS] = {0};
    int numvals = 0;

    char *sep = ",; ";
    char *word, *brkt;
    for (word = strtok_r(str, sep, &brkt); word;
         word = strtok_r(NULL, sep, &brkt))
    {
        sparkvals[numvals++] = atof(word);
    }

    // int sparklen = numvals*2 + 1;
    int sparklen = numvals + 1;
    wchar_t sparkline[sparklen];
    for (int i = 0; i < numvals; i++)
    {
        int idx =
            (int)floor(scaleybum(0, 1.1, 0, wcslen(sparkchars), sparkvals[i]));
        sparkline[i] = sparkchars[idx];
        // sparkline[i*2] = sparkchars[idx];
        // sparkline[i*2+1] = sparkchars[idx];
    }
    sparkline[sparklen] = '\0';
    wprintf(L"FULL WURD: %ls\n", sparkline);
    printf("\n");
}
