#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "main.h"

int analyzeImagery(ProgramState *state);
double epochFromL1Filename(char *filenameNoPath);
int analyzeL1FileImages(ProgramState *state, char *l1file);

int radecToazel(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float ra, float dec, float *az, float *el);
int azelToradec(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float az, float el, float *ra, float *dec);
void geodeticToXYZ(float glat, float glon, float altm, float *x, float *y, float *z, float *dVal);

#endif // _ANALYSIS_H
