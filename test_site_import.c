#include "main.h"

#include "import.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    ProgramState stateL2 = {0};
    stateL2.nCalibrationStars = N_CALIBRATION_STARS;
    stateL2.starSearchBoxWidth = STAR_SEARCH_BOX_WIDTH;
    stateL2.starMaxJitterPixels = STAR_MAX_PIXEL_JITTER;
    stateL2.l1dir = ".";
    stateL2.l2dir = ".";
    stateL2.skymapdir = ".";
    stateL2.stardir = ".";
    stateL2.site = "rank";
    stateL2.firstCalDateString = "2013-12-13T06:00:00.000";
    stateL2.firstCalTime = parseEPOCH4(stateL2.firstCalDateString);
    stateL2.lastCalDateString = "2013-12-13T06:00:03.000";
    stateL2.lastCalTime = parseEPOCH4(stateL2.lastCalDateString);

    ProgramState stateIdl = {0};
    stateIdl.nCalibrationStars = N_CALIBRATION_STARS;
    stateIdl.starSearchBoxWidth = STAR_SEARCH_BOX_WIDTH;
    stateIdl.starMaxJitterPixels = STAR_MAX_PIXEL_JITTER;
    stateIdl.l1dir = ".";
    stateIdl.l2dir = ".";
    stateIdl.skymapdir = ".";
    stateIdl.stardir = ".";
    stateIdl.site = "rank";
    stateIdl.firstCalDateString = "2013-12-13T06:00:00.000";
    stateIdl.firstCalTime = parseEPOCH4(stateIdl.firstCalDateString);
    stateIdl.lastCalDateString = "2013-12-13T06:00:03.000";
    stateIdl.lastCalTime = parseEPOCH4(stateIdl.lastCalDateString);

    int nOptions = 0;

    int statusL2 = loadThemisLevel2(&stateL2);
    int statusIdl = loadSkymap(&stateIdl);

    for (int c = 0; c < IMAGE_COLUMNS; c++)
    {
        for (int r = 0; r < IMAGE_ROWS; r++)
        {
            printf("%f %f %f %f %d %d\n", stateL2.l2CameraAzimuths[c][r], stateIdl.l2CameraAzimuths[c][r], stateL2.l2CameraElevations[c][r], stateIdl.l2CameraElevations[c][r], stateL2.sitePixelOffsets[c][r], stateIdl.sitePixelOffsets[c][r]);
        }
    }

    return EXIT_SUCCESS;
}