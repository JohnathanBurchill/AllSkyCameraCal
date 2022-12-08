/*

    AllSkyCameraCal: analysis.h

    Copyright (C) 2022  Johnathan K Burchill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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

size_t numberOfL1FileImagesToProcess(char *l1file, double firstCalTime, double lastCaltime);


int updateCalibration(ProgramState *state);

#endif // _ANALYSIS_H
