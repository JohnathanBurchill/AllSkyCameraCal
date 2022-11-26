#include "import.h"

#include "stdio.h"
#include "fts.h"
#include "string.h"
#include "stdbool.h"

#include "cdf.h"

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

    while (e)
    {
        if ((strncmp(e->fts_name, "thg_l2_asc_", 11) == 0) && (strncmp(e->fts_name + 11, state->site, 4) == 0) && strncmp(e->fts_name + e->fts_namelen - 4, ".cdf", 4) == 0)
        {
            state->l2filename = strdup(e->fts_name);
            printf("Got L2 file %s\n", state->l2filename);
            // TODO load data

            status = ASCC_OK;

            break;
        }
        e = fts_read(fts);
    }
    fts_close(fts);

    return status;
}