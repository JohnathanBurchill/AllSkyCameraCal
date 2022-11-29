#ifndef _STAR_H
#define _STAR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Star
{
    float catalogNumber;
    double rightAscensionRadian;
    double declinationRadian;
    char spectralType[2];
    int16_t visualMagnitudeTimes100;
    float raProperMotionRadianPerYear;
    float decProperMotionRadianPerYear;
} Star;

typedef struct CalibrationStar
{
    Star *star;
    long catalogIndex;
    float predictedAzElX;
    float predictedAzElY;
    float predictedAzElZ;
    float predictedAz;
    float predictedEl;
    float measuredAzElX;
    float measuredAzElY;
    float measuredAzElZ;
    float measuredAz;
    float measuredEl;
    float deltaAz;
    float deltaEl;
    float magnitude;

    long predictedImageColumn;
    long predictedImageRow;
    float imageMomentColumn;
    float imageMomentRow;
    float previousImageMomentColumn;
    float previousImageMomentRow;
    float backgroundThreshold;
    float meanImageSignalAboveThreshold;
    bool newStarAtThisIndex;

    
    bool includeInCalibration;


} CalibrationStar;


#endif // _STAR_H
