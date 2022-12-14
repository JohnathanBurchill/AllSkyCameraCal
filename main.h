/*

    AllSkyCameraCal: main.h

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

#ifndef _MAIN_H
#define _MAIN_H

#include "star.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <cdf.h>

#define EXPORT_CDF_VERSION_STRING "0001"
#define PROGRAM_VERSION_STRING "20221208"

#define N_CALIBRATION_STARS 20
#define MIN_N_CALIBRATION_STARS_PER_IMAGE 1
#define IMAGE_ROWS 256
#define IMAGE_COLUMNS 256
#define STAR_SEARCH_BOX_WIDTH 9
#define MIN_STAR_SEARCH_BOX_WIDTH 3
#define MAX_PEAK_SIGNAL_FOR_MOMENTS 30000
#define MAX_BACKGROUND_SIGNAL_FOR_MOMENTS 4000
#define STAR_MAX_PIXEL_JITTER 2.0
#define J200EPOCH 63113947200000.0

// How close to the horizon to look for calibration stars
#define CALIBRATION_ELEVATION_BOUND 20

enum ASCC_STATUS
{
    ASCC_OK = 0,
    ASCC_L1_FILE = 1,
    ASCC_L2_FILE = 2,
    ASCC_BSC5_FILE = 3,
    ASCC_MEM = 4,
    ASCC_ARGUMENTS = 5,
    ASCC_CDF_READ = 6,
    ASCC_STAR_FILE = 7,
    ASCC_SKYMAP_FILE = 8,
    ASCC_CDF_EXPORT_NO_DATA = 9,
    ASCC_CDF_WRITE = 10,
    ASCC_NO_CALIBRATION_DATA = 11
};

typedef struct ProgramState
{
    int nOptions;
    bool verbose;
    bool showOptions;
    bool showHelp;
    bool showAbout;

    char *site;
    char *firstCalDateString;
    char *lastCalDateString;

    double firstCalTime;
    double lastCalTime;

    int nCalibrationStars;
    int starSearchBoxWidth;
    float starMaxJitterPixels;

    char *stardir;
    Star *starData;
    int32_t nStars;
    int32_t starSequenceOffset;
    int32_t firstStarNumber;
    int32_t hasStarIds;
    int32_t properMotionIncluded;
    int32_t nMagnitudes;
    int32_t bytesPerStarEntry;

    char *exportdir;
    char cdfFullFilename[CDF_PATHNAME_LEN + 5];
    bool overwriteCdf;

    char *l1dir;
    char **l1filenames;
    size_t nl1filenames;

    char *l2dir;
    char *l2filename;

    char *skymapdir;
    char *skymapfilename;
    bool skymap;

    char *calibrationDateUsed;
    char *calibrationDateGenerated;
    float siteLatitudeGeodetic;
    float siteLongitudeGeodetic;
    float siteAltitudeMetres;

    uint16_t sitePixelOffsets[IMAGE_COLUMNS][IMAGE_ROWS];
    float referenceElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float referenceAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];

    bool calibrationUpdated;
    float calibratedElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float calibratedAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];
    double calibratedEpoch;

    float pixelX[IMAGE_COLUMNS][IMAGE_ROWS];
    float pixelY[IMAGE_COLUMNS][IMAGE_ROWS];
    float pixelZ[IMAGE_COLUMNS][IMAGE_ROWS];

    size_t nImages;
    double *imageTimes;
    float *pointingErrorDcms;
    float *rotationVectors;
    float *rotationAngles;
    uint16_t *nCalibrationStarsUsed;


    bool printStarInfo;

    bool showProgress;
    size_t expectedNumberOfImages;

    double processingStartEpoch;
    double processingStopEpoch;

    char **processingCommand;
    int processingCommandLength;

} ProgramState;

#endif // _MAIN_H
