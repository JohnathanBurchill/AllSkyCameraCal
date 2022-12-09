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
#include "info.h"
#include "options.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int status = ASCC_OK;

    ProgramState state = {0};
    status = setOptions(&state, argc, argv);
    if (status != ASCC_OK)
        return EXIT_FAILURE;
    if (state.showHelp == true || state.showOptions == true)
    {
        usage(&state, argv[0]);
        return EXIT_SUCCESS;
    }
    if (state.showAbout == true)
    {
        aboutASCC();
        return EXIT_SUCCESS;
    }

    if (argc - state.nOptions != 4)
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

    // Read in pixel elevations and azimuths and site geodetic position from calibration file.
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

    // Estimate the calibration for each time
    status = analyzeImagery(&state);
    if (state.showProgress && state.expectedNumberOfImages > 0)
        fprintf(stderr, "\r\n");

    state.processingStopEpoch = currentEpoch();

    if (state.verbose)
        fprintf(stderr, "Processed %zu images.\n", state.expectedNumberOfImages);

    status = updateCalibration(&state);

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

