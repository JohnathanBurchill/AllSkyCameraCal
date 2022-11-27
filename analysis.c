#include "analysis.h"

#include "main.h"
#include <stdio.h>
#include <fts.h>
#include <string.h>

#include <cdf.h>

int sortL1Listing(const FTSENT **first, const FTSENT **second)
{
    // Push abnormal entries to the end...
    // Otherwise sort by date and UT

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

    if (strlen(file1) != 34)
        return 1;
    else if (strlen(file2) != 34)
        return -1;

    int datesCompared = strncmp(file1 + 16, file2 + 16, 10);

    if (datesCompared > 0)
        return 1;
    else if (datesCompared < 0)
        return -1;
    else
        return 0;
        
}

int analyzeImagery(ProgramState *state)
{

    if (state == NULL)
        return ASCC_ARGUMENTS;

    int status = ASCC_OK;

    // Only one version of each L1 file can be in the l1dir
    char *dir[2] = {state->l1dir, NULL};
    FTS *fts = fts_open(dir, FTS_LOGICAL, &sortL1Listing);

    FTSENT *e = fts_read(fts);

    double fileStartEpoch = 0;
    double fileStopEpoch = 0;

    double t1 = state->firstCalTime;
    double t2 = state->lastCalTime;

    while (e != NULL)
    {
        fileStartEpoch = epochFromL1Filename(e->fts_name);
        fileStopEpoch = fileStartEpoch + 3600000; // one hour: L1 files cover 1 hour intervals
        if (fileStartEpoch != ILLEGAL_EPOCH_VALUE && !((fileStartEpoch < t1 && fileStopEpoch <= t1) || (fileStartEpoch >= t2 && fileStopEpoch > t2)))
        {
            printf("Analyzing images in %s\n", e->fts_name);
        }

        e = fts_read(fts);
    }

cleanup:

    if (fts != NULL)
        fts_close(fts);

    return status;
}

double epochFromL1Filename(char *filenameNoPath)
{
    if (filenameNoPath == NULL)
        return ILLEGAL_EPOCH_VALUE;

    if (strlen(filenameNoPath) != 34)
        return ILLEGAL_EPOCH_VALUE;

    if (strncmp(filenameNoPath, "thg_l1_asf", 10) != 0)
        return ILLEGAL_EPOCH_VALUE;

    long y = 0;
    long m = 0;
    long d = 0;
    long h = 0;
    long min = 0;
    long s = 0;
    long ms = 0;


    int nAssigned = sscanf(filenameNoPath + 16, "%4ld%2ld%2ld%2ld", &y, &m, &d, &h);
    if (nAssigned != 4)
        return ILLEGAL_EPOCH_VALUE;

    double epoch = computeEPOCH(y, m, d, h, min, s, ms);

    return epoch;
}