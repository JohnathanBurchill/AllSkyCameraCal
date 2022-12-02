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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cdf.h>

int main(int argc, char **argv)
{

    ProgramState state = {0};
    state.nCalibrationStars = N_CALIBRATION_STARS;
    state.starSearchBoxWidth = STAR_SEARCH_BOX_WIDTH;
    state.starMaxJitterPixels = STAR_MAX_PIXEL_JITTER;
    state.l1dir = ".";
    state.l2dir = ".";
    state.skymapdir = ".";
    state.stardir = ".";
    state.exportdir = ".";

    int nOptions = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
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
        else if (strncmp(argv[i], "--exportdir=", 12) == 0)
        {
            nOptions++;
            state.exportdir = argv[i]+12;
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (argc - nOptions != 4)
    {
        usage(argv[0]);
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

    printf("Estimating THEMIS level 2 optical calibration for site %s using level 1 imagery between %s UT and %s UT\n", state.site, state.firstCalDateString, state.lastCalDateString);

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
        status = loadSkymap(&state);
    else
        status = loadThemisLevel2(&state);
    if (status != ASCC_OK)
    {
        fprintf(stderr, "Could not load calibration file.\n");
        return status;
    }

    // Read in the star catalog (BCS5) sorted by right ascension (BCS5ra)
    status = loadStars(&state);
    if (status != ASCC_OK)
    {
        fprintf(stderr, "Could not load star catalog file.\n");
        goto cleanup;
    }
    if (state.nStars > 0)
    {
        fprintf(stderr, "Expected J2000 format for star entries, got B1950.\n");
        goto cleanup;
    }
    state.nStars = -state.nStars;

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

    // Export error DCMs to CDF file
    exportCdf(&state);

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

    return status;
}


void usage(char *name)
{
    printf("Usage: %s <site> <firstCalDate> <lastCalDate> [--l1dir=<dir>] [--l2dir=<dir>] [--skymap=<file>] [--use-skymap] [--skymapdir=<dir>] [--stardir=<star>] [--number-of-calibration-stars=N] [--star-search-box-width=<widthInPixels>] [--star-max-jitter-pixels=<value>] [--exportdir=<dir>] [--print-star-info] [--help] [--usage]\n", name);
    printf(" estimates new THEMIS ASI elevation and azimuth map for <site> using suitable ASI images from <firstCalDate> to <lastCalDate>.\n");
    printf(" Dates have the form yyyy-mm-ddTHH:MM:SS.sss interpreted as universal times.\n");
    printf(" <l1dir> is the path to the directory containing the THEMIS level 1 ASI files.\n");
    printf(" <l2dir> is the path to the directory containing the THEMIS l2 ASI files.\n");
    printf(" Options:\n");
    printf("%20s : sets the directory containing THEMIS level 1 (ASI) files. Defaults to \".\". Only one version of each L1 file can be in this directory.\n", "--l1dir=<dir>");
    printf("%20s : sets the directory containing THEMIS level 2 (calibration) files. Defaults to \".\".\n", "--l2dir=<dir>");
    printf("%20s : sets the directory containing the Yale Bright Star Catalog file (BSC5ra). Defaults to \".\".\n", "--stardir=<dir>");
    printf("%20s : use the IDL skymap file instead of the themis L2 calibration file.\n", "--skymap");
    printf("%20s : sets the directory containing the IDL skymap files. Defaults to \".\".\n", "--skymapdir=<dir>");
    printf("%20s : use a specific IDK skymap file.\n", "--skymap=<file>");
    printf("%20s : sets the number of calibration stars. Defaults to %d.\n", "--number-of-calibration-stars=N", N_CALIBRATION_STARS);
    printf("%20s : sets the width of the calibration star search box. Defaults to %d.\n", "--star-search-box-width=N", STAR_SEARCH_BOX_WIDTH);
    printf("%20s : sets the maximum change in star image position from previous image to be included in error estimation. Defaults to %.1f.\n", "--star-max-jitter-pixels=<value>", STAR_MAX_PIXEL_JITTER);
    printf("%20s : prints calibration star information for each image.\n", "--print-star-info");
    printf("%20s : sets the directory for the exported calibration CDF.\n", "--exportdir=<dir>");
    printf("%20s : prints this message.\n", "--help");
    printf("%20s : prints author name and license.\n", "--about");

    return;
}

void aboutASCC(void)
{

    printf("allskycameracal. Copyright (C) 2022 Johnathan K. Burchill.\n");
    printf(" Revises the level 2 calibration for THEMIS ASI pixel elevations and azimuths using level 1 imagery and the Yale Bright Star Catalog BSC5.\n");
    printf("\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under the terms of the GNU General Public License.\n");
    printf("See the file LICENSE in the source repository for details.\n");

    return;
}
