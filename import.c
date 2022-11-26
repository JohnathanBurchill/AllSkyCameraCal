#include "import.h"

#include <stdlib.h>
#include <stdio.h>
#include <fts.h>
#include <string.h>
#include <stdbool.h>

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

int loadThemisL2File(ProgramState *state)
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
    char cdfVarName[CDF_VAR_NAME_LEN+1] = {0};
    long numRecs = 0;
    CDFdata data = NULL;

    while (e)
    {
        if ((strncmp(e->fts_name, "thg_l2_asc_", 11) == 0) && (strncmp(e->fts_name + 11, state->site, 4) == 0) && strncmp(e->fts_name + e->fts_namelen - 4, ".cdf", 4) == 0)
        {
            state->l2filename = strdup(e->fts_name); // Track input files, but full path not needed.
            printf("Got L2 file %s\n", state->l2filename);
            // TODO load data
            cdfStatus = CDFopen(e->fts_path, &cdf);
            if (cdfStatus != CDF_OK)
            {
                status = ASCC_L2_FILE;
                break;
            }
            sprintf(cdfVarName, "thg_asc_%s_glat", state->site);
            cdfStatus = CDFreadzVarDataByVarName(cdf, cdfVarName, &numRecs, &data);
            if (cdfStatus != CDF_OK)
            {
                CDFclose(cdf);
                status = ASCC_L2_FILE;
                break;
            }
            state->siteLatitudeGeodetic = *(float*)data;

            sprintf(cdfVarName, "thg_asc_%s_glon", state->site);
            cdfStatus = CDFreadzVarDataByVarName(cdf, cdfVarName, &numRecs, &data);
            if (cdfStatus != CDF_OK)
            {
                CDFclose(cdf);
                status = ASCC_L2_FILE;
                break;
            }
            state->siteLongitudeGeodetic = *(float*)data;
            if (state->siteLongitudeGeodetic > 180.0)
                state->siteLongitudeGeodetic -= 360.0;

            sprintf(cdfVarName, "thg_asc_%s_alti", state->site);
            cdfStatus = CDFreadzVarDataByVarName(cdf, cdfVarName, &numRecs, &data);
            if (cdfStatus != CDF_OK)
            {
                CDFclose(cdf);
                status = ASCC_L2_FILE;
                break;
            }
            state->siteAltitudeMetres = *(float*)data;

            CDFclose(cdf);

            printf("Site location (%s): %.3fN %.3fE, altitude %.0f m\n", state->site, state->siteLatitudeGeodetic, state->siteLongitudeGeodetic, state->siteAltitudeMetres);

            status = ASCC_OK;
            break;


        }
        e = fts_read(fts);
    }
    fts_close(fts);

    return status;
}