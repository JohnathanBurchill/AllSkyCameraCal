#ifndef _MAIN_H
#define _MAIN_H

#define N_CALIBRATION_STARS 25
#define MIN_N_CALIBRATION_STARS 10

enum ASCC_STATUS
{
    ASCC_OK = 0,
    ASCC_L1_FILE = 1,
    ASCC_L2_FILE = 2,
    ASCC_BSC5_FILE = 3,
    ASCC_MEM = 4
};

typedef struct ProgramState
{
    char *site;
    char *firstCalDateString;
    char *lastCalDateString;

    double firstCalTime;
    double lastCalTime;

    int nCalibrationStars;

    char *l1dir;
    char *l2filename;

    char *l2dir;
    float siteLatitudeGeodetic;
    float siteLongitudeGeodetic;
    float siteAltitudeMetres;

    float *sitePixelOffsets;
    float *l2CameraElevations;
    float *l2CameraAzimuths;
    float *l2CameraRightAscensions;

    float *newCameraElevations;
    float *newCameraAzimuths;

} ProgramState;

void usage(char *name);
void about(void);

#endif // _MAIN_H
