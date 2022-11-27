#include "main.h"

#include "import.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cdf.h>

int main(int argc, char **argv)
{

    ProgramState state = {0};
    state.nCalibrationStars = N_CALIBRATION_STARS;

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
            about();
            return EXIT_SUCCESS;
        }
        else if (strncmp(argv[i], "--number-of-calibration-stars=", 30) == 0)
        {
            nOptions++;
            char *errStr = NULL;
            state.nCalibrationStars = atoi(argv[i]+30);
            if (state.nCalibrationStars <= MIN_N_CALIBRATION_STARS)
            {
                fprintf(stderr, "Number of calibration stars must be at least %d\n", MIN_N_CALIBRATION_STARS);
                return EXIT_FAILURE;
            }
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (argc - nOptions != 6)
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

    printf("Estimating THEMIS level 2 optical calibration for site %s using level 1 imagery between %s UT and %s UT\n", state.site, state.firstCalDateString, state.lastCalDateString);

    state.l1dir = argv[4];
    state.l2dir = argv[5];

    if (access(state.l1dir, F_OK) != 0)
    {
        fprintf(stderr, "Level 1 directory %s not found.\n", state.l1dir);
        return EXIT_FAILURE;
    }

    if (access(state.l2dir, F_OK) != 0)
    {
        fprintf(stderr, "Level 2 directory %s not found.\n", state.l2dir);
        return EXIT_FAILURE;
    }

    int status = ASCC_OK;

    // Read in pixel elevations and azimuths and site geodetic position from L2 file.
    status = loadThemisLevel2(&state);
    if (status != ASCC_OK)
    {
        fprintf(stderr, "Could not load THEMIS Level 2 calibration file.\n");
        return status;
    }

    // Read in the star catalog (BCS5) sorted by right ascension (BCS5ra)
    status = loadStars(&state);
    if (status != ASCC_OK)
    {
        fprintf(stderr, "Could not load star catalog file.\n");
        goto cleanup;
    }


    // Loop over all L1 files between the requested calibration time.
    // Estimate the L2 calibration for each time
    // Reject calibrations that are too different from the THEMIS L2 calibration
    // Average the calibrations to get one calibration

    // Exclude any for which the moon was too high?
    // or get the direction of the moon at that time and exclude pixels
    // within a neighbourhood of that direction.

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


cleanup:

    // freeProgramState(&state);

    return status;
}


void usage(char *name)
{
    printf("Usage: %s <site> <firstCalDate> <lastCalDate> <l1dir> <l2dir> [--number-of-calibration-stars=N] [--help] [--usage]\n", name);
    printf(" estimates new THEMIS ASI elevation and azimuth map for <site> using suitable ASI images from <firstCalDate> to <lastCalDate>.\n");
    printf(" Dates have the form yyyy-mm-ddTHH:MM:SS.sss interpreted as universal times.\n");
    printf(" <l1dir> is the path to the directory containing the THEMIS level 1 ASI files.\n");
    printf(" <l2dir> is the path to the directory containing the THEMIS l2 ASI files.\n");
    printf(" Options:\n");
    printf("%20s : sets the number of calibration stars. Defaults to %d.\n", "--number-of-calibration-stars=N", N_CALIBRATION_STARS);
    printf("%20s : prints this message.\n", "--help");
    printf("%20s : prints author name and license.\n", "--about");

    return;
}

void about(void)
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