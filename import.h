#ifndef _IMPORT_H
#define _IMPORT_H

#include "main.h"
#include <cdf.h>

int loadThemisLevel2(ProgramState *state);

float getCdfFloat(CDFid cdf, char *site, char *varNameTemplate);
int getCdfFloatArray(CDFid cdf, char *site, char *varNameTemplate, long recordIndex, void **data);

int loadStars(ProgramState *state);
int readBSC5Int32(FILE *f, int32_t *value);
void reverseBytes(uint8_t *word, int nBytes);

int loadSkymap(ProgramState *state);
int loadSkymapFromFile(ProgramState *state);


#endif // _IMPORT_H
