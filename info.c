#include "info.h"
#include "main.h"
#include "util.h"

#include <stdio.h>

void usage(ProgramState *state, char *name)
{
    printf("Usage: %s <site> <firstCalDate> <lastCalDate> [options] [--help] [--help-options]\n", name);
    printf("\nEstimate THEMIS ASI elevation and azimuth errors for <site> from <firstCalDate> to <lastCalDate>.\n");
    printf("\n<site> is a 4-letter THEMIS site abbreviation, lowercase (e.g., rank).\n");
    printf("\nDates have the form yyyy-mm-ddTHH:MM:SS.sss interpreted as universal times without leap seconds (THEMIS time).\n");
    if (state->showOptions)
    {
        printf("\nOptions:\n");
        printOptMsg("--exportdir=<dir>", "sets the directory to export the results to. Defaults to \".\".");
        printOptMsg("--l1dir=<dir>", "sets the directory containing THEMIS level 1 (ASI) files. Defaults to \".\". Only one version of each L1 file can be in this directory.");
        printOptMsg("--l2dir=<dir>", "sets the directory containing THEMIS level 2 (calibration) files. Defaults to \".\".");
        printOptMsg("--stardir=<dir>", "sets the directory containing the Yale Bright Star Catalog file (BSC5ra). Defaults to \".\".");
        printOptMsg("--skymap", "use an IDL skymap file instead of a THEMIS L2 calibration file.");
        printOptMsg("--skymapdir=<dir>", "sets the directory containing the IDL skymap files. Defaults to \".\".");
        printOptMsg("--skymap=<file>", "use a specific IDL skymap file.");
        printOptMsg("--number-of-calibration-stars=N", "set the number of calibration stars. Defaults to " STR(N_CALIBRATION_STARS) ".");
        printOptMsg("--star-search-box-width=N", "set the width of the calibration star search box. Defaults to " STR(STAR_SEARCH_BOX_WIDTH) ".");
        printOptMsg("--star-max-jitter-pixels=<value>", "set the maximum change in star image position from previous image to be included in error estimation. Defaults to " STR(STAR_MAX_PIXEL_JITTER) ".");
        printOptMsg("--print-star-info", "print calibration star information for each image.");
        printOptMsg("--show-progress", "show image processing progress.");
        printOptMsg("--exportdir=<dir>", "set the directory for the exported calibration CDF.");
        printOptMsg("--overwrite-cdf", "overwrite the target CDF if it exists.");
        printOptMsg("--verbose", "print more information during processing.");
        printOptMsg("--help", "show how to run this program.");
        printOptMsg("--help-options", "show program options.");
        printOptMsg("--about", "print author name and license.");
    }

    return;
}

void aboutASCC(void)
{

    printf("allskycameracal. Copyright (C) 2022 Johnathan K. Burchill.\n");
    printf("\nEstimates errors in THEMIS ASI pixel elevations and azimuths using level 1 imagery and the Yale Bright Star Catalog BSC5.\n");
    printf("\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under the terms of the GNU General Public License.\n");
    printf("See the file LICENSE in the source repository for details.\n");

    return;
}
