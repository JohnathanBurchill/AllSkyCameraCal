#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{

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
    }

    return EXIT_SUCCESS;
}


void usage(char *name)
{
    printf("Usage: %s <site> <firstCalDate> <lastCalDate> <l1dir> <l2dir> [--help] [--usage]\n", name);
    printf(" estimates new THEMIS ASI elevation and azimuth map for <site> using suitable ASI images from <firstCalDate> to <lastCalDate>.\n");
    printf(" Dates have the form yyyy-mm-ddTHH:MM:SS.sss interpreted as universal times.\n");
    printf(" <l1dir> is the path to the directory containing the THEMIS level 1 ASI files.\n");
    printf(" <l2dir> is the path to the directory containing the THEMIS l2 ASI files.\n");
    printf(" Options:\n");
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