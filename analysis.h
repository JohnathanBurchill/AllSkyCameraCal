#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "main.h"
#include "star.h"

int analyzeImagery(ProgramState *state);
double epochFromL1Filename(char *filenameNoPath);
int analyzeL1FileImages(ProgramState *state, char *l1file);


int radecToazel(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float ra, float dec, float *az, float *el);
int azelToradec(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float az, float el, float *ra, float *dec);
void geodeticToXYZ(float glat, float glon, float altm, float *x, float *y, float *z, float *dVal);

int selectStars(ProgramState *state, double imageTime, CalibrationStar *calStars);

int calculateMoments(ProgramState *state, uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], CalibrationStar *cal, int boxHalfWidth, float boxCenterColumn, float boxCenterRow, int pixelThreshold);
float calculateMeanSignal(uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], int boxHalfWidth, float boxCenterColumn, float boxCenterRow);
int calculatePositionOfMax(uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], int boxHalfWidth, float boxCenterColumn, float boxCenterRow, int *cmax, int *rmax);

#endif // _ANALYSIS_H
