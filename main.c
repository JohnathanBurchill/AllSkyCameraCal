/*

    AllSkyCameraCal: main.c

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

#include "main.h"

#include "import.h"
#include "analysis.h"
#include "export.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cdf.h>

#define QT(x) #x
#define STR(macro) QT(macro)

int main(int argc, char **argv)
{

    ProgramState state = {0};
    state.processingStartEpoch = currentEpoch();
    state.nCalibrationStars = N_CALIBRATION_STARS;
    state.starSearchBoxWidth = STAR_SEARCH_BOX_WIDTH;
    state.starMaxJitterPixels = STAR_MAX_PIXEL_JITTER;
    state.exportdir = ".";
    state.l1dir = ".";
    state.l2dir = ".";
    state.skymapdir = ".";
    state.stardir = ".";
    state.exportdir = ".";
    state.processingCommand = argv;
    state.processingCommandLength = argc;

    int nOptions = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            usage(&state, argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--help-options") == 0)
        {
            state.showOptions = true;
            usage(&state, argv[0]);
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "--about") == 0)
        {
            aboutASCC();
            return EXIT_SUCCESS;
        }
        else if (strncmp(argv[i], "--number-of-calibration-stars=", 30) == 0)
        {
            nOptions++;
            char *errStr = NULL;
            state.nCalibrationStars = atoi(argv[i]+30);
            if (state.nCalibrationStars < MIN_N_CALIBRATION_STARS_PER_IMAGE)
            {
                fprintf(stderr, "Number of calibration stars must be at least %d.\n", MIN_N_CALIBRATION_STARS_PER_IMAGE);
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--star-search-box-width=", 24) == 0)
        {
            nOptions++;
            char *errStr = NULL;
            state.starSearchBoxWidth = atoi(argv[i]+24);
            if (state.starSearchBoxWidth < MIN_STAR_SEARCH_BOX_WIDTH)
            {
                fprintf(stderr, "Star search box width must be at least %d pixels.\n", MIN_STAR_SEARCH_BOX_WIDTH);
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--star-max-jitter-pixels=", 25) == 0)
        {
            nOptions++;
            char *errStr = NULL;
            state.starMaxJitterPixels = atof(argv[i]+25);
            if (state.starMaxJitterPixels <= 0.0)
            {
                fprintf(stderr, "Star search box width must be greater than 0.0 pixels.\n");
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--exportdir=", 12) == 0)
        {
            nOptions++;
            state.exportdir = argv[i]+12;
        }
        else if (strncmp(argv[i], "--l1dir=", 8) == 0)
        {
            nOptions++;
            state.l1dir = argv[i]+8;
        }
        else if (strncmp(argv[i], "--l2dir=", 8) == 0)
        {
            nOptions++;
            state.l2dir = argv[i]+8;
        }
        else if (strncmp(argv[i], "--skymapdir=", 12) == 0)
        {
            nOptions++;
            state.skymapdir = argv[i]+12;
        }
        else if (strncmp(argv[i], "--skymap=", 9) == 0)
        {
            nOptions++;
            state.skymap = true;
            state.skymapfilename = argv[i]+9;
        }
        else if (strcmp(argv[i], "--skymap") == 0)
        {
            nOptions++;
            state.skymap = true;
        }
        else if (strncmp(argv[i], "--stardir=", 10) == 0)
        {
            nOptions++;
            state.stardir = argv[i]+10;
        }
        else if (strcmp(argv[i], "--print-star-info") == 0)
        {
            nOptions++;
            state.printStarInfo = true;
        }
        else if (strcmp(argv[i], "--show-progress") == 0)
        {
            nOptions++;
            state.showProgress = true;
        }
        else if (strncmp(argv[i], "--exportdir=", 12) == 0)
        {
            nOptions++;
            state.exportdir = argv[i]+12;
        }
        else if (strcmp(argv[i], "--overwrite-cdf") == 0)
        {
            nOptions++;
            state.overwriteCdf = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            nOptions++;
            state.verbose = true;
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (argc - nOptions != 4)
    {
        usage(&state, argv[0]);
        return EXIT_FAILURE;
    }

    state.site = argv[1];
    if (strlen(state.site) != 4)
    {
        fprintf(stderr, "site name must be 4 letters, like \"rank\" for Rankin Inlet.\n");
        return EXIT_FAILURE;
    }

    state.firstCalDateString = argv[2];
    state.firstCalTime = parseEPOCH4(state.firstCalDateString);
    if (state.firstCalTime == ILLEGAL_EPOCH_VALUE)
    {
        fprintf(stderr, "The first calibration date is garbage.\n");
        return EXIT_FAILURE;
    }

    state.lastCalDateString = argv[3];
    state.lastCalTime = parseEPOCH4(state.lastCalDateString);
    if (state.lastCalTime == ILLEGAL_EPOCH_VALUE)
    {
        fprintf(stderr, "The last calibration date is garbage.\n");
        return EXIT_FAILURE;
    }

    if (state.lastCalTime <= state.firstCalTime)
    {
        fprintf(stderr, "Last calibration time must be greater than first calibration time.\n");
        return EXIT_FAILURE;
    }

    if (access(state.l1dir, F_OK) != 0)
    {
        fprintf(stderr, "Level 1 directory %s not found.\n", state.l1dir);
        return EXIT_FAILURE;
    }

    if (!state.skymap && (access(state.l2dir, F_OK) != 0))
    {
        fprintf(stderr, "Level 2 directory %s not found.\n", state.l2dir);
        return EXIT_FAILURE;
    }

    if (state.skymap)
    {
        if (state.skymapfilename == NULL && access(state.skymapdir, F_OK) != 0)
        {
            fprintf(stderr, "Skymap directory %s not found.\n", state.skymapdir);
            return EXIT_FAILURE;
        }
        else if (access(state.skymapfilename, F_OK) != 0)
        {
            fprintf(stderr, "Skymap file %s not found.\n", state.skymapfilename);
            return EXIT_FAILURE;
        }
    }

    int status = ASCC_OK;

    // Read in pixel elevations and azimuths and site geodetic position from L2 file.
    if (state.skymap)
    {
        status = loadSkymap(&state);
        if (status != ASCC_OK && state.verbose)
        {
            fprintf(stderr, "Could not load skymap file %s.\n", state.skymapfilename);
            return status;
        }
    }
    else
    {
        status = loadThemisLevel2(&state);
        if (status != ASCC_OK && state.verbose)
        {
            fprintf(stderr, "Could not load THEMIS level 2 calibration file %s.\n", state.l2filename);
            return status;
        }
    }

    if (state.verbose)
        fprintf(stderr, "Estimating THEMIS %s ASI optical calibration using %s %s for level 1 imagery between %s UT and %s UT\n", state.site, state.skymap ? "SKYMAP" : "L2", state.skymap ? state.skymapfilename : state.l2filename, state.firstCalDateString, state.lastCalDateString);

    // Read in the star catalog (BCS5) sorted by right ascension (BCS5ra)
    status = loadStars(&state);
    if (status != ASCC_OK)
    {
        if (state.verbose)
            fprintf(stderr, "Could not load star catalog file.\n");
        goto cleanup;
    }
    if (state.nStars > 0)
    {
        if (state.verbose)
            fprintf(stderr, "Expected J2000 format for star entries, got B1950.\n");
        goto cleanup;
    }
    state.nStars = -state.nStars;

    if (state.verbose)
    {
        fprintf(stderr, "Read %d stars from BSC5ra database in %s\n", state.nStars, state.stardir);
    }

    // Don't need L2 pixel directions.
        // Need only an approximate starting point to locate stars
        // Like center pixel as zenith and bottom is geographic north
        // Then calculate a new starting map of elevation and azimuth
        // using geocentric coordinates
        // Latitude and longitude can then be easily calculated in geocentric
        // and magnetic field-line tracing and comparison with satellite
        // position (all geocentric) are simpler.
        // But it would be good to validate results against official L2
        // calibration. So leave this as a future development?

    // Estimate the L2 calibration for each time
    // OR: get at least one star in each image and accumlate the positions with
    // time bins of one hour, or a day or whatever, then analyze the results statistically.

    // Reject calibrations that are too different from the THEMIS L2 calibration
    // Average the calibrations to get one calibration

    // Exclude any for which the moon was too high?
    // or get the direction of the moon at that time and exclude pixels
    // within a neighbourhood of that direction.
    // Or just enforce a maximum signal for each "star" region.

    // Perform any further image processing
    // Exclude obvious snow and cloud?
    // Could count the number of stars and reject images if the count is too low?

    // For each image

        // get the universal time
        // Calculate the right ascension of each pixel
        // If the first image or if the RA min-max has changed by more than 5 degrees
            // Get stars within the min-max RA and min-max elevation of the image
            // Sort those stars from brightest to dimmest
        // Keep the N brightest stars, say N = 20
    
        // This method relies on the camera orientation being very close to the 
        // original camera orientation. Arclengths of star displacements 
        // no more than 5 degrees, say?

        // For each calibration star
        // Defining two concentric rings in arc-length about 
        // each star labeled R1 and R2
        // the first ring being a disk and the second an adjacent annulus,        
            // get the mean pixel count within R2 as a background estimate
            // subtract this from the pixel values within R1 and calculate the
            // first moment and total signal in R1
            // If the total signal in R1 is too small, reject this star.

        // Continue to the next L1 image if there are not enough calibration stars

        // The remaining moments are the "actual" star positions

        // Calculate the mean arc length of the displacements 
        // "expected star positions" minus "estimated star positions from first moments"
        // If this is too large, continue to the next image

        // Iterate to an estimate of the new pixel elevation and right ascension map

        // If that worked, calculate the new azimuths from the new right ascensions and the time

    // If the number of estimated calibrations is too few, fail the overall calbration.

    // Calculate the median (or mean?) and standard deviation of the arc lengths of the 
    // new el-az maps minus the original L2 el-az map.
    // If the average arc length or the standard deviation is too large, flag this
    // Save the new L2 calibration in a CDF file.

    // Loop over all L1 files between the requested calibration time.
    status = analyzeImagery(&state);
    if (state.showProgress && state.expectedNumberOfImages > 0)
        fprintf(stderr, "\r\n");

    state.processingStopEpoch = currentEpoch();

    if (state.verbose)
        fprintf(stderr, "Processed %lu images.\n", state.expectedNumberOfImages);

    // Export error DCMs to CDF file
    status = exportCdf(&state);
    
    if (state.verbose)
    {
        if (status != ASCC_OK)
            fprintf(stderr, "Could not create the CDF.\n");
        else
            fprintf(stderr, "Created %s\n", state.cdfFullFilename);

    }


cleanup:

    // freeProgramState(&state);
    if (state.starData != NULL)
        free(state.starData);
    if (state.imageTimes != NULL)
        free(state.imageTimes);
    if (state.pointingErrorDcms != NULL)
        free(state.pointingErrorDcms);
    if (state.rotationVectors != NULL)
        free(state.rotationVectors);
    if (state.rotationAngles != NULL)
        free(state.rotationAngles);
    for (int i = 0; i < state.nl1filenames; i++)
    {
        if (state.l1filenames[i] != NULL)
            free(state.l1filenames[i]);
    }
    if (state.l1filenames != NULL)
        free(state.l1filenames);

    return status;
}

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
