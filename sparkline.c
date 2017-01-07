#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "sparkline.h"
#include "utils.h"

// const wchar_t *sparkchars =
// L"\u2581\u2582\u2583\u2584\u2585\u2586\u2587\u2588";
const wchar_t *sparkchars = L"\u2581\u2582\u2583\u2585\u2586\u2587";

void sparky(char *instr)
{
    // I'm making an assumption that input are
    // floats in the range 0.0 - 1.0

    printf("GOts %s\n", instr);
    char str[80]; // arbitrary number here
    strcpy(str, instr);

    double sparkvals[80] = {0};
    int numvals = 0;

    char *sep = ",; ";
    char *word, *brkt;
    for (word = strtok_r(str, sep, &brkt); word;
         word = strtok_r(NULL, sep, &brkt)) {
        sparkvals[numvals++] = atof(word);
    }

    double min, max;
    for (int i = 0; i < numvals; i++) {
        // printf("SVAL: %f\n", sparkvals[i]);
        if (!min || sparkvals[i] < min)
            min = sparkvals[i];
        if (!max || sparkvals[i] > max)
            max = sparkvals[i];
    }
    printf("Max: %.2f Min: %.2f\n", max, min);

    int charset_len = wcslen(sparkchars);
    printf("Len %d\n", charset_len);
    double diff = max - min;
    double skip = diff == 0 ? 1 : charset_len / diff;
    printf("Skip %f\n", skip);

    for (int i = 0; i < numvals; i++) {
        // double v = (sparkvals[i] - min) * skip;
        // TODO - not really a sparkline now
        // as i'm making data assumptions
        // rather than just showing variations,
        // i want to
        int idx = (int)floor(scaleybum(0, 1, 0, 5, sparkvals[i]));
        // printf("IDX: %d\n", idx);
        wprintf(L"%lc%lc", sparkchars[idx], sparkchars[idx]);
        // printf("SVAL: %f\n", sparkvals[i]);
    }
    printf("\n");
}
