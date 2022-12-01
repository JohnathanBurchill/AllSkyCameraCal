/*

    AllSkyCameraCal: star.h

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
