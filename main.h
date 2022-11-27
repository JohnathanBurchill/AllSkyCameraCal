#ifndef _MAIN_H
#define _MAIN_H

#define N_CALIBRATION_STARS 25
#define MIN_N_CALIBRATION_STARS 10
#define IMAGE_ROWS 256
#define IMAGE_COLUMNS 256

enum ASCC_STATUS
{
    ASCC_OK = 0,
    ASCC_L1_FILE = 1,
    ASCC_L2_FILE = 2,
    ASCC_BSC5_FILE = 3,
    ASCC_MEM = 4,
    ASCC_ARGUMENTS = 5,
    ASCC_CDF_READ = 6
};

typedef struct ProgramState
{
    char *site;
    char *firstCalDateString;
    char *lastCalDateString;

    double firstCalTime;
    double lastCalTime;

    int nCalibrationStars;

    char *stardir;

    char *l1dir;
    char *l2filename;

    char *l2dir;
    float siteLatitudeGeodetic;
    float siteLongitudeGeodetic;
    float siteAltitudeMetres;

    float *sitePixelOffsets;
    float l2CameraElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float l2CameraAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];
    float l2CameraRightAscensions[IMAGE_COLUMNS][IMAGE_ROWS];

    float newCameraElevations[IMAGE_COLUMNS][IMAGE_ROWS];
    float newCameraAzimuths[IMAGE_COLUMNS][IMAGE_ROWS];

} ProgramState;

void usage(char *name);
void about(void);

#endif // _MAIN_H
