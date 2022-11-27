#include "import.h"

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
    // TODO sort FTS listings to get lastest L2 version first
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

    while (e)
    {
        if ((strncmp(e->fts_name, "thg_l2_asc_", 11) == 0) && (strncmp(e->fts_name + 11, state->site, 4) == 0) && strncmp(e->fts_name + e->fts_namelen - 4, ".cdf", 4) == 0)
        {
            state->l2filename = strdup(e->fts_name); // Track input files, but full path not needed.
            // printf("Got L2 file %s\n", state->l2filename);
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

            pointer = &state->l2CameraElevations[0][0];
            status = getCdfFloatArray(cdf, state->site, "thg_asf_%s_elev", 0, &pointer);
            if (status != ASCC_OK)
                break;

            pointer = &state->l2CameraAzimuths[0][0];
            status = getCdfFloatArray(cdf, state->site, "thg_asf_%s_azim", 0, &pointer);
            if (status != ASCC_OK)
                break;

            // printf("Site location (%s): %.3fN %.3fE, altitude %.0f m\n", state->site, state->siteLatitudeGeodetic, state->siteLongitudeGeodetic, state->siteAltitudeMetres);

            // for (int i = 0; i < IMAGE_COLUMNS; i++)
            //     for (int j = 0; j < IMAGE_ROWS; j++)
            //         printf("pixel(%d,%d): elev: %.2f azim %.2f\n", i, j, state->l2CameraElevations[i][j], state->l2CameraAzimuths[i][j]);

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
int getCdfFloatArray(CDFid cdf, char *site, char *varNameTemplate, long recordIndex, float **data)
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
    // Convert big endian to little endian
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
        s->raProperMotionRadianPerYear = *(float*)(bytes + i*state->bytesPerStarEntry + 24);
        reverseBytes((uint8_t*)&s->raProperMotionRadianPerYear, 4);
        s->decProperMotionRadianPerYear = *(float*)(bytes + i*state->bytesPerStarEntry + 28);
        reverseBytes((uint8_t*)&s->decProperMotionRadianPerYear, 4);
    }

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
