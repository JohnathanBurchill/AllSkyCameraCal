#ifndef _MAIN_H
#define _MAIN_H

#include "star.h"

#include <stdlib.h>
#include <stdint.h>

#define N_CALIBRATION_STARS 50
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
    ASCC_STAR_FILE = 7
};

typedef struct ProgramState
{
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


    char *l1dir;
    char *l2filename;

    char *l2dir;
    float siteLatitudeGeodetic;
    float siteLongitudeGeodetic;
    float siteAltitudeMetres;

    uint16_t sitePixelOffsets[IMAGE_COLUMNS][IMAGE_ROWS];
    float l2CameraElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float l2CameraAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];

    float pixelX[IMAGE_COLUMNS][IMAGE_ROWS];
    float pixelY[IMAGE_COLUMNS][IMAGE_ROWS];
    float pixelZ[IMAGE_COLUMNS][IMAGE_ROWS];

    float newCameraElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float newCameraAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];

} ProgramState;

void usage(char *name);
void about(void);

#endif // _MAIN_H
