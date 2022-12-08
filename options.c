#include "main.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int setOptions(ProgramState *state, int argc, char **argv)
{
    state->processingStartEpoch = currentEpoch();
    state->nCalibrationStars = N_CALIBRATION_STARS;
    state->starSearchBoxWidth = STAR_SEARCH_BOX_WIDTH;
    state->starMaxJitterPixels = STAR_MAX_PIXEL_JITTER;
    state->exportdir = ".";
    state->l1dir = ".";
    state->l2dir = ".";
    state->skymapdir = ".";
    state->stardir = ".";
    state->exportdir = ".";
    state->processingCommand = argv;
    state->processingCommandLength = argc;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            state->showHelp = true;
            return ASCC_OK;
        }
        if (strcmp(argv[i], "--help-options") == 0)
        {
            state->showOptions = true;
            return ASCC_OK;
        }
        else if (strcmp(argv[i], "--about") == 0)
        {
            state->showAbout = true;
            return ASCC_OK;
        }
        else if (strncmp(argv[i], "--number-of-calibration-stars=", 30) == 0)
        {
            state->nOptions++;
            char *errStr = NULL;
            state->nCalibrationStars = atoi(argv[i]+30);
            if (state->nCalibrationStars < MIN_N_CALIBRATION_STARS_PER_IMAGE)
            {
                fprintf(stderr, "Number of calibration stars must be at least %d.\n", MIN_N_CALIBRATION_STARS_PER_IMAGE);
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--star-search-box-width=", 24) == 0)
        {
            state->nOptions++;
            char *errStr = NULL;
            state->starSearchBoxWidth = atoi(argv[i]+24);
            if (state->starSearchBoxWidth < MIN_STAR_SEARCH_BOX_WIDTH)
            {
                fprintf(stderr, "Star search box width must be at least %d pixels.\n", MIN_STAR_SEARCH_BOX_WIDTH);
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--star-max-jitter-pixels=", 25) == 0)
        {
            state->nOptions++;
            char *errStr = NULL;
            state->starMaxJitterPixels = atof(argv[i]+25);
            if (state->starMaxJitterPixels <= 0.0)
            {
                fprintf(stderr, "Star search box width must be greater than 0.0 pixels.\n");
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--exportdir=", 12) == 0)
        {
            state->nOptions++;
            state->exportdir = argv[i]+12;
        }
        else if (strncmp(argv[i], "--l1dir=", 8) == 0)
        {
            state->nOptions++;
            state->l1dir = argv[i]+8;
        }
        else if (strncmp(argv[i], "--l2dir=", 8) == 0)
        {
            state->nOptions++;
            state->l2dir = argv[i]+8;
        }
        else if (strncmp(argv[i], "--skymapdir=", 12) == 0)
        {
            state->nOptions++;
            state->skymapdir = argv[i]+12;
        }
        else if (strncmp(argv[i], "--skymap=", 9) == 0)
        {
            state->nOptions++;
            state->skymap = true;
            state->skymapfilename = argv[i]+9;
        }
        else if (strcmp(argv[i], "--skymap") == 0)
        {
            state->nOptions++;
            state->skymap = true;
        }
        else if (strncmp(argv[i], "--stardir=", 10) == 0)
        {
            state->nOptions++;
            state->stardir = argv[i]+10;
        }
        else if (strcmp(argv[i], "--print-star-info") == 0)
        {
            state->nOptions++;
            state->printStarInfo = true;
        }
        else if (strcmp(argv[i], "--show-progress") == 0)
        {
            state->nOptions++;
            state->showProgress = true;
        }
        else if (strncmp(argv[i], "--exportdir=", 12) == 0)
        {
            state->nOptions++;
            state->exportdir = argv[i]+12;
        }
        else if (strcmp(argv[i], "--overwrite-cdf") == 0)
        {
            state->nOptions++;
            state->overwriteCdf = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            state->nOptions++;
            state->verbose = true;
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return ASCC_OK;
}
