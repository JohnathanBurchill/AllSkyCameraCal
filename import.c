/*

    AllSkyCameraCal: import.c

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

#include "import.h"

#include <readsave.h>

#include <stdlib.h>
#include <stdio.h>
#include <fts.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/stat.h>

#include <cdf.h>

int sortL2Listing(const FTSENT **first, const FTSENT **second)
{
    // Push abnormal entries to the end...

    if (first == NULL)
        return 1;
    else if (second == NULL)
        return -1;

    const FTSENT *a = *first;
    const FTSENT *b = *second;

    if (a == NULL)
        return 1;
    else if (b == NULL)
        return -1;

    const char *file1 = a->fts_name;
    const char *file2 = b->fts_name;

    if (file1 == NULL)
        return 1;
    else if (file2 == NULL)
        return -1;

    if (strlen(file1) != 32)
        return 1;
    else if (strlen(file2) != 32)
        return -1;

    int versionsCompared = strncmp(file1 + a->fts_namelen - 7, file2 + b->fts_namelen - 7, 3);

    if (versionsCompared > 0)
        return -1;
    else if (versionsCompared < 0)
        return 1;
    else
        return 0;
        
}

int loadThemisLevel2(ProgramState *state)
{
    char *dir[2] = {state->l2dir, NULL};
    FTS *fts = fts_open(dir, FTS_LOGICAL, &sortL2Listing);
    if (fts == NULL)
        return ASCC_L2_FILE;

    FTSENT *e = fts_read(fts);

    int status = ASCC_L2_FILE;

    CDFid cdf = NULL;
    CDFstatus cdfStatus = 0;
    long numRecs = 0;
    CDFdata data = NULL;
    float *pointer = NULL;
    uint16_t *pointeru16 = NULL;

    while (e)
    {
        if ((strncmp(e->fts_name, "thg_l2_asc_", 11) == 0) && (strncmp(e->fts_name + 11, state->site, 4) == 0) && strncmp(e->fts_name + e->fts_namelen - 4, ".cdf", 4) == 0)
        {
            state->l2filename = strdup(e->fts_name); // Track input files, but full path not needed.
            cdfStatus = CDFopen(e->fts_path, &cdf);
            if (cdfStatus != CDF_OK)
            {
                status = ASCC_L2_FILE;
                break;
            }
            state->siteLatitudeGeodetic = getCdfFloat(cdf, state->site, "thg_asc_%s_glat");
            state->siteLongitudeGeodetic = getCdfFloat(cdf, state->site, "thg_asc_%s_glon");
            state->siteAltitudeMetres = getCdfFloat(cdf, state->site, "thg_asc_%s_alti");

            if (!isfinite(state->siteLatitudeGeodetic) || !isfinite(state->siteLongitudeGeodetic) || !isfinite(state->siteAltitudeMetres))
            {
                status = ASCC_L2_FILE;
                break;
            }

            pointer = &state->cameraElevations[0][0];
            status = getCdfFloatArray(cdf, state->site, "thg_asf_%s_elev", 0, (void*)&pointer);
            if (status != ASCC_OK)
                break;

            pointer = &state->cameraAzimuths[0][0];
            status = getCdfFloatArray(cdf, state->site, "thg_asf_%s_azim", 0, (void*)&pointer);
            if (status != ASCC_OK)
                break;

            pointeru16 = &state->sitePixelOffsets[0][0];
            status = getCdfFloatArray(cdf, state->site, "thg_asf_%s_offset", 0, (void*)&pointeru16);
            if (status != ASCC_OK)
                break;

            long attrNum = CDFgetAttrNum(cdf, "Generation_date");
            if (attrNum < 0)
            {
                status = ASCC_CDF_READ;
                break;
            }
            long dataType = 0;
            long nVals = 0;
            status = CDFattrEntryInquire(cdf, attrNum, 0, &dataType, &nVals);
            if (status != CDF_OK)
                break;
            state->calibrationDateGenerated = calloc(nVals + 1, 1);
            if (state->calibrationDateGenerated == NULL)
            {
                status = ASCC_MEM;
                break;
            }
            status = CDFgetAttrgEntry(cdf, attrNum, 0, state->calibrationDateGenerated);
            if (status != CDF_OK)
                break;

            // The date used for calibrations is not clear for L2 files. Set to unknown.
            state->calibrationDateUsed = "unknown";
            
            for (int c = 0; c < IMAGE_COLUMNS; c++)
            {
                for (int r = 0; r < IMAGE_ROWS; r++)
                {
                    if (isfinite(state->cameraAzimuths[c][r]) && isfinite(state->cameraElevations[c][r]))
                    {
                        state->pixelX[c][r] = cos((90.0 - state->cameraAzimuths[c][r])*M_PI/180.0) * cos(state->cameraElevations[c][r]*M_PI/180.0);
                        state->pixelY[c][r] = sin((90.0 - state->cameraAzimuths[c][r])*M_PI/180.0) * cos(state->cameraElevations[c][r]*M_PI/180.0);
                        state->pixelZ[c][r] = sin(state->cameraElevations[c][r]*M_PI/180.0);
                    }
                    else
                    {
                        state->pixelX[c][r] = NAN;
                        state->pixelY[c][r] = NAN;
                        state->pixelZ[c][r] = NAN;
                    }
                }
            }
            if (state->verbose)
                fprintf(stderr, "Site location (%s): %.3fN %.3fE, altitude %.0f m\n", state->site, state->siteLatitudeGeodetic, state->siteLongitudeGeodetic, state->siteAltitudeMetres);

            status = ASCC_OK;
            break;

        }
        e = fts_read(fts);
    }

    if (data != NULL)
        CDFdataFree(data);
    if (cdf != NULL)
        CDFclose(cdf);

    fts_close(fts);

    return status;
}

float getCdfFloat(CDFid cdf, char *site, char *varNameTemplate)
{
    if (site == NULL || varNameTemplate == NULL)
        return NAN;

    long numRecs = 0;
    CDFdata data = NULL;
    char cdfVarName[CDF_VAR_NAME_LEN+1] = {0};
    sprintf(cdfVarName, varNameTemplate, site);
    CDFstatus cdfStatus = CDFreadzVarDataByVarName(cdf, cdfVarName, &numRecs, &data);
    if (cdfStatus != CDF_OK || numRecs != 1)
    {
        if (data != NULL)
            CDFdataFree(data);
        return NAN;
    }
    float value = *(float*)data;
    CDFdataFree(data);

    return value;
}

// Caller frees memory at pointer when values are no longer needed
// Caller needs to know how many elements are in the record from 
// THEMIS documentation or cdfdumping the L2 file
int getCdfFloatArray(CDFid cdf, char *site, char *varNameTemplate, long recordIndex, void **data)
{
    if (site == NULL || varNameTemplate == NULL || data == NULL)
        return ASCC_ARGUMENTS;

    char cdfVarName[CDF_VAR_NAME_LEN+1] = {0};
    sprintf(cdfVarName, varNameTemplate, site);
    long nRecs = 0;

    CDFstatus cdfStatus = CDFgetzVarMaxWrittenRecNum(cdf, CDFgetVarNum(cdf, cdfVarName), &nRecs);
    // Assumes data is NULL if cdfStatus != CDF_OK...Is it true?
    if (cdfStatus != CDF_OK || nRecs <= recordIndex)
        return ASCC_L2_FILE;

    // TODO: How to deal with multiple epochs and elevation-azimuth maps?
    // The test file has two epochs. What gives?
    // Can't assume the last record is more correct. Or are they even different?
    // For now let caller decide
    cdfStatus = CDFgetVarRangeRecordsByVarName(cdf, cdfVarName, recordIndex, recordIndex, *data);
    if (cdfStatus != CDF_OK)
        return ASCC_CDF_READ;

    return ASCC_OK;
}

int sortStars(const void *first, const void *second)
{
    if (first == NULL)
        return 1;
    else if (second == NULL)
        return -1;

    Star *a = (Star*)first;
    Star *b = (Star*)second;

    // The brighter star has the lower magnitude
    if (a->visualMagnitudeTimes100 >= b->visualMagnitudeTimes100)
        return 1;
    else if (a->visualMagnitudeTimes100 < b->visualMagnitudeTimes100)
        return -1;
    else
        return 0;
    
}

int loadStars(ProgramState *state)
{
    char bsc5raFile[FILENAME_MAX+1];
    int res = snprintf(bsc5raFile, FILENAME_MAX, "%s/BSC5ra", state->stardir);

    struct stat fileInfo = {0};

    int status = ASCC_OK;

    uint8_t *bytes = NULL;

    status = stat(bsc5raFile, &fileInfo);
    if (status != 0)
        return ASCC_STAR_FILE;

    size_t nStarBytes = (size_t)fileInfo.st_size - 28;

    FILE *starFile = fopen(bsc5raFile, "r");
    if (starFile == NULL)
        return ASCC_STAR_FILE;

    status = readBSC5Int32(starFile, &state->starSequenceOffset);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->firstStarNumber);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->nStars);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->hasStarIds);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->properMotionIncluded);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->nMagnitudes);
    if (status != ASCC_OK)
        goto cleanup;

    status = readBSC5Int32(starFile, &state->bytesPerStarEntry);
    if (status != ASCC_OK)
        goto cleanup;

    bytes = calloc(nStarBytes, 1);
    if (bytes == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }

    size_t read = fread(bytes, 1, nStarBytes, starFile);
    if (read != nStarBytes)
    {
        status = ASCC_STAR_FILE;
        goto cleanup;
    }

    int nStars = nStarBytes / state->bytesPerStarEntry;
    state->starData = (Star*) calloc(nStars, sizeof(Star));
    if (state->starData == NULL)
        goto cleanup;

    Star *s = NULL;
    // Convert big endian to little endian and fill the Star array
    for (int i = 0; i < nStars; i++)
    {
        s = &state->starData[i];
        s->catalogNumber = *(float*)(bytes + i*state->bytesPerStarEntry);
        reverseBytes((uint8_t*)&s->catalogNumber, 4);
        s->rightAscensionRadian = *(double*)(bytes + i*state->bytesPerStarEntry + 4);
        reverseBytes((uint8_t*)&s->rightAscensionRadian, 8);
        s->declinationRadian = *(double*)(bytes + i*state->bytesPerStarEntry + 12);
        reverseBytes((uint8_t*)&s->declinationRadian, 8);
        s->spectralType[0] = bytes[i*state->bytesPerStarEntry + 20];
        s->spectralType[1] = bytes[i*state->bytesPerStarEntry + 21];
        s->visualMagnitudeTimes100 = *(int16_t*)(bytes + i*state->bytesPerStarEntry + 22);
        reverseBytes((uint8_t*)&s->visualMagnitudeTimes100, 2);
        s->decProperMotionRadianPerYear = *(float*)(bytes + i*state->bytesPerStarEntry + 28);
        reverseBytes((uint8_t*)&s->decProperMotionRadianPerYear, 4);
        s->raProperMotionRadianPerYear = *(float*)(bytes + i*state->bytesPerStarEntry + 24);
        reverseBytes((uint8_t*)&s->raProperMotionRadianPerYear, 4);
        // RA proper motion is cos(dec) * d RA / dt. Convert to d RA / dt:
        // Website labels the proper motion in radian per year, but cross-checking
        // against ASCII catalog shows it is milliradian per year.
        s->raProperMotionRadianPerYear = s->raProperMotionRadianPerYear / cos(s->declinationRadian) / 1000.0;
        s->decProperMotionRadianPerYear = s->decProperMotionRadianPerYear / 1000.0;
    }

    // Sort from brightest to dimmest
    qsort(state->starData, nStars, sizeof(Star), &sortStars);


cleanup:
    fclose(starFile);
    if (bytes != NULL)
        free(bytes);

    if (status != ASCC_OK)
        if (state->starData != NULL)
            free(state->starData);

    return status;
}

int readBSC5Int32(FILE *f, int32_t *value)
{
    if (value == NULL)
        return ASCC_ARGUMENTS;

    uint8_t buf[4] = {0};
    uint8_t buf1[4] = {0};
    int res = fread(buf, 1, 4, f);
    if (res != 4)
        return ASCC_BSC5_FILE;
    for (int i = 0; i < 4; i++)
        buf1[i] = buf[3-i];
    *value = *(int32_t*)buf1;

    return ASCC_OK;
}

void reverseBytes(uint8_t *word, int nBytes)
{
    if (word == NULL)
        return;

    uint8_t buf = 0;
    for (int i = 0; i < nBytes / 2; i++)
    {
        buf = word[nBytes - 1 - i];
        word[nBytes -1 - i] = word[i];
        word[i] = buf;
    }

    return;
}


int loadSkymap(ProgramState *state)
{
    if (state == NULL)
        return ASCC_ARGUMENTS;

    int status = ASCC_OK;
    if (state->skymapfilename == NULL)
    {
        char *dir[2] = {state->skymapdir, NULL};
        // TODO sort FTS listings to get most recent prior skymap version first
        FTS *fts = fts_open(dir, FTS_LOGICAL, NULL);
        if (fts == NULL)
        {
            if (state->verbose)
                fprintf(stderr, "Unable to read skymap directory %s.\n", state->skymapdir);
            return ASCC_L2_FILE;
        }

        FTSENT *e = fts_read(fts);

        int status = ASCC_L2_FILE;

        CDFid cdf = NULL;
        CDFstatus cdfStatus = 0;
        long numRecs = 0;
        CDFdata data = NULL;
        float *pointer = NULL;
        uint16_t *pointeru16 = NULL;

        while (e)
        {
            if ((strncmp(e->fts_name, "themis_skymap_", 14) == 0) && (strncmp(e->fts_name + 14, state->site, 4) == 0) && strncmp(e->fts_name + e->fts_namelen - 4, ".sav", 4) == 0)
            {
                // Likely a skymap file
                state->skymapfilename = strdup(e->fts_path);
                break;
            }
            e = fts_read(fts);
        }
        fts_close(fts);
    }

    if (state->skymapfilename == NULL)
    {
        if (state->verbose)
            fprintf(stderr, "Unable to find skymap file.\n");
        return ASCC_SKYMAP_FILE;
    }

    status = loadSkymapFromFile(state);

    return status;
}

int loadSkymapFromFile(ProgramState *state)
{
    if (state == NULL)
        return ASCC_ARGUMENTS;

    VariableList variables = {0};
    SaveInfo fileInfo = {0};

    int status = ASCC_OK;

    status = readSave(state->skymapfilename, &fileInfo, &variables);
    if (status != READSAVE_OK)
    {
        if (state->verbose)
            fprintf(stderr, "Error reading skymap file %s.\n", state->skymapfilename);
        return ASCC_SKYMAP_FILE;
    }
    Variable *var = NULL;
    Variable *selectedVar = NULL;

    float *pointer = NULL;
    uint16_t *pointeru16 = NULL;
    void *mem = NULL;

    Variable *data = NULL;
    Variable *v = NULL;

    // Expecting 1 variable with format the same as "themis_skymap_rank_20130107-+_vXX.sav"
    var = variables.variableList;
    if (var == NULL)
        return ASCC_SKYMAP_FILE;
    data = ((Variable*)var->data);
    if (data == NULL)
        return ASCC_SKYMAP_FILE;

    v = variableData(data, "skymap.site_map_latitude");
    if (v == NULL)
        return ASCC_SKYMAP_FILE;
    state->siteLatitudeGeodetic  = *(float*)(v->data);

    v = variableData(data, "skymap.site_map_longitude");
    if (v == NULL)
        return ASCC_SKYMAP_FILE;
    state->siteLongitudeGeodetic  = *(float*)(v->data);

    v = variableData(data, "skymap.site_map_altitude");
    if (v == NULL)
        return ASCC_SKYMAP_FILE;
    state->siteAltitudeMetres  = *(float*)(v->data);

    v = variableData(data, "skymap.generation_info.date_generated");
    if (v == NULL)
        return ASCC_SKYMAP_FILE;
    state->calibrationDateGenerated  = strdup((char*)v->data);
    if (state->calibrationDateGenerated == NULL)
        return ASCC_MEM;

    v = variableData(data, "skymap.generation_info.date_time_used");
    if (v == NULL)
        return ASCC_SKYMAP_FILE;
    state->calibrationDateUsed  = strdup((char*)v->data);
    if (state->calibrationDateUsed == NULL)
        return ASCC_MEM;

    if (!isfinite(state->siteLatitudeGeodetic) || !isfinite(state->siteLongitudeGeodetic) || !isfinite(state->siteAltitudeMetres))
        return ASCC_SKYMAP_FILE;

    pointer = &state->cameraElevations[0][0];
    mem = variableData(data, "skymap.full_elevation")->data;
    if (mem == NULL)
        return ASCC_SKYMAP_FILE;
    memcpy(pointer, mem, IMAGE_COLUMNS * IMAGE_ROWS * sizeof(float));

    pointer = &state->cameraAzimuths[0][0];
    mem = variableData(data, "skymap.full_azimuth")->data;
    if (mem == NULL)
        return ASCC_SKYMAP_FILE;
    memcpy(pointer, mem, IMAGE_COLUMNS * IMAGE_ROWS * sizeof(float));

    pointeru16 = &state->sitePixelOffsets[0][0];
    mem = variableData(data, "skymap.full_subtract")->data;
    if (mem == NULL)
        return ASCC_SKYMAP_FILE;
    memcpy(pointeru16, mem, IMAGE_COLUMNS * IMAGE_ROWS * sizeof(uint16_t));

    float tmp = 0.0;
    uint16_t tmpu16 = 0;
    // First rearrange the arrays to match L2 indexing: two flips
    for (int c = 0; c < IMAGE_COLUMNS / 2; c++)
    {
        for (int r = 0; r < IMAGE_ROWS; r++)
        {
            tmp = state->cameraElevations[c][r];
            state->cameraElevations[c][r] = state->cameraElevations[IMAGE_COLUMNS - 1 - c][r];
            state->cameraElevations[IMAGE_COLUMNS - 1 - c][r] = tmp;

            tmp = state->cameraAzimuths[c][r];
            state->cameraAzimuths[c][r] = state->cameraAzimuths[IMAGE_COLUMNS - 1 - c][r];
            state->cameraAzimuths[IMAGE_COLUMNS - 1 - c][r] = tmp;

            tmpu16 = state->sitePixelOffsets[c][r];
            state->sitePixelOffsets[c][r] = state->sitePixelOffsets[IMAGE_COLUMNS - 1 - c][r];
            state->sitePixelOffsets[IMAGE_COLUMNS - 1 - c][r] = tmpu16;
        }
    }
    for (int c = 0; c < IMAGE_COLUMNS; c++)
    {
        for (int r = 0; r < IMAGE_ROWS / 2; r++)
        {
            tmp = state->cameraElevations[c][r];
            state->cameraElevations[c][r] = state->cameraElevations[c][IMAGE_ROWS - 1 - r];
            state->cameraElevations[c][IMAGE_ROWS - 1 - r] = tmp;

            tmp = state->cameraAzimuths[c][r];
            state->cameraAzimuths[c][r] = state->cameraAzimuths[c][IMAGE_ROWS - 1 - r];
            state->cameraAzimuths[c][IMAGE_ROWS - 1 - r] = tmp;

            tmpu16 = state->sitePixelOffsets[c][r];
            state->sitePixelOffsets[c][r] = state->sitePixelOffsets[c][IMAGE_ROWS - 1 - r];
            state->sitePixelOffsets[c][IMAGE_ROWS - 1 - r] = tmpu16;
        }
    }

    for (int c = 0; c < IMAGE_COLUMNS; c++)
    {
        for (int r = 0; r < IMAGE_ROWS; r++)
        {
            // Calculate AzEl Cartesian coordinates
            if (isfinite(state->cameraAzimuths[c][r]) && isfinite(state->cameraElevations[c][r]))
            {
                state->pixelX[c][r] = cos((90.0 - state->cameraAzimuths[c][r])*M_PI/180.0) * cos(state->cameraElevations[c][r]*M_PI/180.0);
                state->pixelY[c][r] = sin((90.0 - state->cameraAzimuths[c][r])*M_PI/180.0) * cos(state->cameraElevations[c][r]*M_PI/180.0);
                state->pixelZ[c][r] = sin(state->cameraElevations[c][r]*M_PI/180.0);
            }
            else
            {
                state->pixelX[c][r] = NAN;
                state->pixelY[c][r] = NAN;
                state->pixelZ[c][r] = NAN;
            }
        }
    }

    return ASCC_OK;

}