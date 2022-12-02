/*

    AllSkyCameraCal: analysis.c

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

#include "analysis.h"

#include "main.h"
#include "star.h"

#include <stdio.h>
#include <stdbool.h>
#include <fts.h>
#include <string.h>
#include <math.h>

#include <cdf.h>

#include <gsl/gsl_statistics_float.h>
#include <gsl/gsl_sort_float.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>


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
            // printf("Analyzing images in %s\n", e->fts_name);
            analyzeL1FileImages(state, fts->fts_path);
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

// Mixing import and analysis, split to separate files?
int analyzeL1FileImages(ProgramState *state, char *l1file)
{

    if (state == NULL || l1file == NULL)
        return ASCC_ARGUMENTS;

    CDFid cdf = NULL;
    CDFstatus cdfStatus = CDFopen(l1file, &cdf);
    if (cdfStatus != CDF_OK)
        return ASCC_L1_FILE;

    int status = ASCC_OK;    
 
    long nImagesAnalyzed = 0;
    long maxFileRecord = 0;
    long nFileImages = 0;
    CalibrationStar *calStars = calloc(state->nCalibrationStars, sizeof *calStars);
    if (calStars == NULL)
        return ASCC_MEM;
    CalibrationStar *cal = NULL;

    float *azVals = calloc(state->nCalibrationStars, sizeof *azVals);
    float *elVals = calloc(state->nCalibrationStars, sizeof *elVals);
    double *predictedAzElXYZ = calloc(state->nCalibrationStars, 3 * (sizeof *predictedAzElXYZ));
    double *measuredAzElXYZ = calloc(state->nCalibrationStars, 3 * (sizeof *measuredAzElXYZ));
    // TODO handle freeing memory better
    if (azVals == NULL || elVals == NULL || predictedAzElXYZ == NULL || measuredAzElXYZ == NULL)
        return ASCC_MEM;

    char cdfVarName[CDF_VAR_NAME_LEN + 1] = {0};

    // Get time and continue only if within requested analysis time range
    snprintf(cdfVarName, CDF_VAR_NAME_LEN + 1, "thg_asf_%s_epoch", state->site);
    cdfStatus = CDFgetzVarMaxWrittenRecNum(cdf, CDFgetVarNum(cdf, cdfVarName), &maxFileRecord);
    if (cdfStatus != CDF_OK || maxFileRecord == 0)
    {
        status = ASCC_L1_FILE;
        goto cleanup;
    }

    nFileImages = maxFileRecord + 1;
    // printf("file records: %ld\n", nFileImages);


    size_t startImage = state->nImages;
    size_t imageCounter = startImage;
    state->nImages += nFileImages;
    void *mem = realloc(state->imageTimes, sizeof(double) * state->nImages);
    if (mem == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    state->imageTimes = mem;

    mem = realloc(state->pointingErrorDcms, 9 * sizeof(float) * state->nImages);
    if (mem == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    state->pointingErrorDcms = mem;

    mem = realloc(state->rotationVectors, 3 * sizeof(float) * state->nImages);
    if (mem == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    state->rotationVectors = mem;

    mem = realloc(state->rotationAngles, sizeof(float) * state->nImages);
    if (mem == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    state->rotationAngles = mem;

    double imageTime = 0.0;
    char timeString[EPOCH4_STRING_LEN+1];

    uint16_t imagery[256][256] = {0};
    uint16_t *imagePointer = &imagery[0][0];
    long index = 0;

    // Updated on each function call
    int nCalStars = 0;
    int nCalStarsKept = 0;
    float starAzElDistance = 0.0;
    float starMinAzElDistance = 0.0;
    int cmax = 0;
    int rmax = 0;
    float totalOuter = 0.0;
    int momentCounter = 0;
    float starx = 0.0;
    float stary = 0.0;
    float starz = 0.0;
    float dx = 0.0;
    float dy = 0.0;
    float dz = 0.0;
    float magnitude = 0.0;
    float azhatx = 0.0;
    float azhaty = 0.0;
    float elhatx = 0.0;
    float elhaty = 0.0;
    float elhatz = 0.0;

    float meanSignal = 0.0;

    bool foundNearest = false;

    int boxHalfWidth = state->starSearchBoxWidth / 2;

    // printf("El minmax: %.2f %.2f\n", state->minElevation, state->maxElevation);
    // printf("Az minmax: %.2f %.2f\n", state->minAzimuth, state->maxAzimuth);

    // For rotation matrix estimation
    double cArr[9] = {0.0};
    double vArr[9] = {0.0};
    double v1Arr[9] = {0.0};
    double sArr[3] = {0.0};
    double workArr[3] = {0.0};
    double dcmArr[9] = {0.0};
    double cDetSignArr[9] = {0.0};
    double rotationVectorArr[3] = {0.0};
    double rotationVectorLength = 0.0;
    double rotationAngle = 0.0;

    gsl_matrix_view c = gsl_matrix_view_array(cArr, 3, 3);
    gsl_matrix_view v = gsl_matrix_view_array(vArr, 3, 3);
    gsl_matrix_view v1 = gsl_matrix_view_array(v1Arr, 3, 3);
    gsl_vector_view s = gsl_vector_view_array(sArr, 3);
    gsl_vector_view work = gsl_vector_view_array(workArr, 3);
    gsl_matrix_view dcm = gsl_matrix_view_array(dcmArr, 3, 3);
    gsl_matrix_view cDetSign = gsl_matrix_view_array(cDetSignArr, 3, 3);
    gsl_vector_view rotationVector = gsl_vector_view_array(rotationVectorArr, 3);

    float statAz = 0.0;
    float statEl = 0.0;

    for (long ind = 0; ind < nFileImages; ind++)
    {
        index = ind;
        // printf("%s: i = %ld, nImageFiles: %ld\n", l1file, ind, nFileImages);
        snprintf(cdfVarName, CDF_VAR_NAME_LEN + 1, "thg_asf_%s_epoch", state->site);
        // printf("index: %ld\n", index);
        cdfStatus = CDFgetVarRangeRecordsByVarName(cdf, cdfVarName, index, index, &imageTime);
        if (cdfStatus != CDF_OK)
            continue;
        if (imageTime < state->firstCalTime || imageTime > state->lastCalTime)
            continue;
        // printf("%ld: %lf\n", imageCounter, imageTime);
        state->imageTimes[imageCounter] = imageTime;
        encodeEPOCH4(imageTime, timeString);
        // printf("Analyzing image at %s\n", timeString);

        // Assume sensible file validation - same number of images as epochs
        snprintf(cdfVarName, CDF_VAR_NAME_LEN + 1, "thg_asf_%s", state->site);
        cdfStatus = CDFgetVarRangeRecordsByVarName(cdf, cdfVarName, index, index, imagePointer);
        if (cdfStatus != CDF_OK)
            continue;

        for (int c = 0; c < IMAGE_COLUMNS; c++)
            for (int r = 0; r < IMAGE_ROWS; r++)
            {
                if (imagery[c][r] > state->sitePixelOffsets[c][r])
                    imagery[c][r] -= state->sitePixelOffsets[c][r];
                else
                    imagery[c][r] = 0;
                // printf("%d,%d: %d\n", c, r, imagery[c][r]);
            }

        nCalStars = selectStars(state, imageTime, calStars);

        // printf("Got %d calibration stars.\n", nCalStars);
        // for (int i = 0; i < nCalStars; i++)
        //     printf("star %d (ind %ld): magnitude = %.2lf\n", i, calStarInds[i], state->starData[calStarInds[i]].visualMagnitudeTimes100/100.0);
        nCalStarsKept = 0;

        float azelX = 0.0;
        float azelY = 0.0;
        float azelZ = 0.0;

        for (int i = 0; i < nCalStars; i++)
        {
            cal = &calStars[i];
            starMinAzElDistance = 10000000000.0;
            foundNearest = false;
            cmax = 0;
            rmax = 0;
            for (int c = 0; c < IMAGE_COLUMNS; c++)
            {
                for (int r = 0; r < IMAGE_COLUMNS; r++)
                {
                    if (!isfinite(state->l2CameraAzimuths[c][r]) || !isfinite(state->l2CameraElevations[c][r]))
                        continue;
                    // TODO use a GPU
                    starx = cos((90.0 - cal->predictedAz)*M_PI/180.0) * cos(cal->predictedEl*M_PI/180.0);
                    stary = sin((90.0 - cal->predictedAz)*M_PI/180.0) * cos(cal->predictedEl*M_PI/180.0);
                    starz = sin(cal->predictedEl*M_PI/180.0);
                    cal->predictedAzElX = starx;
                    cal->predictedAzElY = stary;
                    cal->predictedAzElZ = starz;
                    dx = starx - state->pixelX[c][r];
                    dy = stary - state->pixelY[c][r];
                    dz = starz - state->pixelZ[c][r];
                    starAzElDistance = sqrt(dx * dx + dy * dy + dz * dz);
                    if (starAzElDistance < starMinAzElDistance)
                    {
                        starMinAzElDistance = starAzElDistance;
                        cal->predictedImageColumn = c;
                        cal->predictedImageRow = r;
                        foundNearest = true;
                    }
                }
            }
            if (foundNearest)
            {
                momentCounter = 0.0;
                // Do a first search of neighbors for actual star signal
                meanSignal = calculateMeanSignal(imagery, boxHalfWidth, cal->predictedImageColumn, cal->predictedImageRow);
                if (!isfinite(meanSignal) || meanSignal > MAX_BACKGROUND_SIGNAL_FOR_MOMENTS)
                    continue;

                momentCounter = calculatePositionOfMax(imagery, boxHalfWidth, cal->predictedImageColumn, cal->predictedImageRow, &cmax, &rmax);
                if (momentCounter == 0)
                    continue;

                // Refine search using new estimate for box center and a small box size
                momentCounter = calculateMoments(state, imagery, cal, 2, (float)cmax, (float)rmax, roundf(meanSignal) + 10);

                if (momentCounter > 0 && cal->meanImageSignalAboveThreshold > 0.0)
                {
                    // printf("momentCounter: %d\n", momentCounter);

                    nCalStarsKept++;
                    cal->includeInCalibration = true;
                    // Calculate dRa and dDec from measuremed (interpolated) values minus predicted values
                    dx = cal->predictedAzElX - cal->measuredAzElX;
                    dy = cal->predictedAzElY - cal->measuredAzElY;
                    dz = cal->predictedAzElZ - cal->measuredAzElZ;
                    // printf("dx,dy,dz = %f %f %f\n", dx, dy, dz);
                    cal->measuredAz = 90.0 - atan2(cal->measuredAzElY, cal->measuredAzElX) / M_PI * 180.0;
                    cal->measuredEl = atan(cal->measuredAzElZ / hypotf(cal->measuredAzElX, cal->measuredAzElY)) / M_PI * 180.0;

                    // detlaAz and deltaEl
                    // rhat is measuredAzElX, measuredAzElY, measuredAzElZ
                    azhatx = - cal->measuredAzElY;
                    azhaty = cal->measuredAzElX;
                    magnitude = sqrt(azhatx * azhatx + azhaty * azhaty);
                    azhatx /= magnitude;
                    azhaty /= magnitude;
                    elhatx = - cal->measuredAzElZ * azhaty;
                    elhaty = cal->measuredAzElZ * azhatx;
                    elhatz = cal->measuredAzElX * cal->measuredAzElY - cal->measuredAzElY * cal->measuredAzElX;
                    magnitude = sqrt(elhatx * elhatx + elhaty * elhaty + elhatz * elhatz);
                    elhatx /= magnitude;
                    elhaty /= magnitude;
                    elhatz /= magnitude;
                    // TODO need to multiply stardRas by cos(dec)?
                    cal->deltaAz= (dx * azhatx + dy * azhaty);
                    cal->deltaEl = (dx * elhatx + dy * elhaty + dz * elhatz);
                }
                else
                {
                    // Flag this star as not used
                    cal->includeInCalibration = false;
                }
            }
        }
        // If enough cal stars,
        // Find zenith from centre of rotation
        //  model rotation angle as function of time and least-squares fit rotation centre?
        // Rotate look direction to align zenith
        // Calculate angular displacements about zenith
        // Rotate around zenith
        // Collect information about quality of solution

        // Or, can always just rotate about centre of CCD image?
        // CCD center is given in L2 file? No. Just in idlsav SKYMAP files
        // Iterating rotations about middle of image might work
        if (nCalStarsKept >= MIN_N_CALIBRATION_STARS_PER_IMAGE)
        {
            int statCounter = 0;

            for (int i = 0; i < nCalStars; i++)
            {
                cal = &calStars[i];
                if (cal->includeInCalibration && (imageCounter == startImage || cal->newStarAtThisIndex || (fabsf(cal->imageMomentColumn - cal->previousImageMomentColumn) < STAR_MAX_PIXEL_JITTER && fabsf(cal->imageMomentRow - cal->previousImageMomentRow) < STAR_MAX_PIXEL_JITTER)))
                {
                    // A rotation away from zenith (in declination)
                    // will be positive on one side and negative on the other
                    // TODO improve this estimate taking this into account?
                    // For now, take magnitude of error only for elevations
                    azVals[statCounter] = cal->deltaAz;
                    elVals[statCounter] = fabsf(cal->deltaEl);

                    // For rotation matrix estimation
                    // Using double type to be able to use GSL SVD
                    predictedAzElXYZ[statCounter*3] = (double)cal->predictedAzElX;
                    predictedAzElXYZ[statCounter*3 + 1] = (double)cal->predictedAzElY;
                    predictedAzElXYZ[statCounter*3 + 2] = (double)cal->predictedAzElZ;

                    measuredAzElXYZ[statCounter*3] = (double)cal->measuredAzElX;
                    measuredAzElXYZ[statCounter*3 + 1] = (double)cal->measuredAzElY;
                    measuredAzElXYZ[statCounter*3 + 2] = (double)cal->measuredAzElZ;

                    statCounter++;
                }
                cal->previousImageMomentColumn = cal->imageMomentColumn;
                cal->previousImageMomentRow = cal->imageMomentRow;
            }
            statAz = gsl_stats_float_median(azVals, 1, statCounter);
            statEl = gsl_stats_float_median(elVals, 1, statCounter);
            // gsl_sort_float(azVals, 1, statCounter);
            // float statAz = gsl_stats_float_trmean_from_sorted_data(0.25, azVals, 1, statCounter);
            // gsl_sort_float(elVals, 1, statCounter);
            // float statEl = gsl_stats_float_trmean_from_sorted_data(0.25, elVals, 1, statCounter);

            // Calculate rotation matrix for this image
            // statCounter x 3 matrices
            gsl_matrix_view a = gsl_matrix_view_array(predictedAzElXYZ, statCounter, 3);
            gsl_matrix_view b = gsl_matrix_view_array(measuredAzElXYZ, statCounter, 3);
            // These are Nx3 matrices. Using the method of https://cnx.org/contents/HV-RsdwL@23/Molecular-Distance-Measures, the matrices should be 3xN.
            // calculate C = X^T * Y
            int gslStatus = gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, &a.matrix, &b.matrix, 0, &c.matrix);
            if (gslStatus != GSL_SUCCESS)
                goto cleanup;

            gsl_matrix *d = &c.matrix;
            // From wikipedia article for determinant
            double da = gsl_matrix_get(d, 0, 0);
            double db = gsl_matrix_get(d, 0, 1);
            double dc = gsl_matrix_get(d, 0, 2);
            double dd = gsl_matrix_get(d, 1, 0);
            double de = gsl_matrix_get(d, 1, 1);
            double df = gsl_matrix_get(d, 1, 2);
            double dg = gsl_matrix_get(d, 2, 0);
            double dh = gsl_matrix_get(d, 2, 1);
            double di = gsl_matrix_get(d, 2, 2);
            double cDet = da*de*di + db*df*dg + dc*dd*dh - dc*de*dg - db*dd*di - da*df*dh;
            gsl_matrix_set(&cDetSign.matrix, 0, 0, 1.0);
            gsl_matrix_set(&cDetSign.matrix, 1, 1, 1.0);
            gsl_matrix_set(&cDetSign.matrix, 2, 2, cDet >= 0.0 ? 1.0 : -1.0);

            gslStatus = gsl_linalg_SV_decomp(&c.matrix, &v.matrix, &s.vector, &work.vector);
            if (gslStatus != GSL_SUCCESS)
                goto cleanup;

            // C now contains W for the SVD of C as W S V^T
            // The DCM is then
            gslStatus = gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, &cDetSign.matrix, &v.matrix, 0, &v1.matrix);
            if (gslStatus != GSL_SUCCESS)
                goto cleanup;

            gslStatus = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &c.matrix, &v1.matrix, 0, &dcm.matrix);
            if (gslStatus != GSL_SUCCESS)
                goto cleanup;

            // Probably safe to assume that the DCM is not symmetric. 
            // TODO check this

            // from https://en.wikipedia.org/wiki/Rotation_matrix#Conversion_from_rotation_matrix_to_axis–angle
            gsl_vector_set(&rotationVector.vector, 0, gsl_matrix_get(&dcm.matrix, 2, 1) - gsl_matrix_get(&dcm.matrix, 1, 2));
            gsl_vector_set(&rotationVector.vector, 1, gsl_matrix_get(&dcm.matrix, 0, 2) - gsl_matrix_get(&dcm.matrix, 2, 0));
            gsl_vector_set(&rotationVector.vector, 2, gsl_matrix_get(&dcm.matrix, 1, 0) - gsl_matrix_get(&dcm.matrix, 0, 1));
            double *r = rotationVectorArr;
            rotationVectorLength = sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
            rotationAngle = asin(rotationVectorLength / 2.0) / M_PI * 180.0;
            if (rotationVectorLength > 0.0)
            {
                r[0] /= rotationVectorLength;
                r[1] /= rotationVectorLength;
                r[2] /= rotationVectorLength;
            }
            else
            {
                r[0] = 1.0;
                r[1] = 0.0;
                r[2] = 0.0;
            }
            // Store fit for later export
            for (int m = 0; m < 9; m++)
                state->pointingErrorDcms[imageCounter * 9 + m] = dcmArr[m];
            for (int m = 0; m < 3; m++)
                state->rotationVectors[imageCounter* 3 + m] = rotationVectorArr[m];
            state->rotationAngles[imageCounter] = rotationAngle;

            for (int i = 0; i < nCalStars && state->printStarInfo; i++)
            {
                cal = &calStars[i];
                if (cal->includeInCalibration)
                {
                    // TODO update the pixel azimuths and elevations by adding the dAzs and dEls?
                    // and store all info in a CDF
                    // Looks like the AZ / EL map needs to be rotated and translated,
                    // not just translated.
                    printf("%lf %ld %ld %.3f %.3f %.3f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.6lf %.6lf %.6lf %.6lf\n", imageTime, cal->predictedImageColumn, cal->predictedImageRow, cal->imageMomentColumn, cal->imageMomentRow, cal->magnitude, cal->predictedAz, cal->predictedEl, cal->measuredAz, cal->measuredEl, cal->deltaAz / M_PI * 180.0, cal->deltaEl / M_PI * 180.0, statAz / M_PI * 180.0, statEl / M_PI * 180.0, dcmArr[0], dcmArr[1], dcmArr[2], dcmArr[3], dcmArr[4], dcmArr[5], dcmArr[6], dcmArr[7], dcmArr[8], rotationVectorArr[0], rotationVectorArr[1], rotationVectorArr[2], rotationAngle);
                }
                else
                {
                    printf("%lf %ld %ld %.3f %.3f %.3f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf %.9lf\n", imageTime, cal->predictedImageColumn, cal->predictedImageRow, NAN, NAN, cal->magnitude, cal->predictedAz, cal->predictedEl, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN);
                }
            }
        }
        else
        {
            for (int m = 0; m < 9; m++)
                state->pointingErrorDcms[imageCounter * 9 + m] = NAN;
            for (int m = 0; m < 3; m++)
                state->rotationVectors[imageCounter * 3 + m] = NAN;
            state->rotationAngles[imageCounter] = NAN;
        }
        imageCounter++;
    }
    if (imageCounter > startImage)
    {
        state->nl1filenames++;
        mem = realloc(state->l1filenames, state->nl1filenames * sizeof(char*));
        if (mem == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
        state->l1filenames = mem;
        state->l1filenames[state->nl1filenames - 1] = strdup(l1file);
        if (state->l1filenames[state->nl1filenames - 1] == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
    }
    if (state->nImages != imageCounter)
    {
        state->nImages = imageCounter;
        // Resize to match number of images analyzed
        mem = realloc(state->imageTimes, sizeof(double) * state->nImages);
        if (mem == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
        state->imageTimes = mem;

        mem = realloc(state->pointingErrorDcms, 9 * sizeof(float) * state->nImages);
        if (mem == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
        state->pointingErrorDcms = mem;

        mem = realloc(state->rotationVectors, 3 * sizeof(float) * state->nImages);
        if (mem == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
        state->rotationVectors = mem;

        mem = realloc(state->rotationAngles, sizeof(float) * state->nImages);
        if (mem == NULL)
        {
            status = ASCC_MEM;
            goto cleanup;
        }
        state->rotationAngles = mem;
    }


cleanup:
    if (cdf != NULL)
        CDFclose(cdf);

    if (calStars != NULL)
        free(calStars);

    if (azVals != NULL)
        free(azVals);

    if (elVals != NULL)
        free(elVals);

    if (predictedAzElXYZ != NULL)
        free(predictedAzElXYZ);

    if (measuredAzElXYZ != NULL)
        free(measuredAzElXYZ);

    return status;
}

int radecToazel(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float ra, float dec, float *az, float *el)
{
    int status = ASCC_OK;

    float x0 = 0.0;
    float y0 = 0.0;
    float z0 = 0.0;
    float d = 0.0;

    geodeticToXYZ(geodeticLatitudeDeg, longitudeDeg, altitudeM, &x0, &y0, &z0, &d);

    float vertx = x0;
    float verty = y0;
    float vertz = z0 + d;
    float mag = sqrt(vertx * vertx + verty * verty + vertz * vertz);
    
    // zenith
    float zenithx = vertx / mag;
    float zenithy = verty / mag;
    float zenithz = vertz / mag;

    // zhat
    float zhatx = 0.0;
    float zhaty = 0.0;
    float zhatz = 1.0;
    // East is zhat cross vert
    float eastx = zhaty * zenithz - zhatz * zenithy;
    float easty = -zhatx * zenithz + zhatz * zenithx;
    float eastz = zhatx * zenithy - zhaty * zenithx;

    mag = sqrt(eastx * eastx + easty * easty + eastz * eastz);
    eastx /= mag;
    easty /= mag;
    eastz /= mag;

    // Not anywhere near a pole. Ignore cases with lat = 90 or -90
    // north = zenith cross east
    float northx = zenithy * eastz - zenithz * easty;
    float northy = -zenithx * eastz + zenithz * eastx;
    float northz = zenithx * easty - zenithy * eastx;
    // Should be fine, but force unit length anyway
    mag = sqrt(northx * northx + northy * northy + northz * northz);
    northx /= mag;
    northy /= mag;
    northz /= mag;

    // printf("x: %.2f y: %.2f z: %.2f\n", x0, y0, z0);

    // Now have a coordinate system local east, north, up (geodetic up)
    // represented in X, Y, Z (ECEF) coodinates.
    // They share the same origin.
    // A unit vector in the ENU system can be represented in the XYZ system,
    // from which RA and DEC can be calculated

    float theta = M_PI_2 - dec;

    // Take into account the time
    // add Earth's rotation since J2000 12 noon.
    // https://en.wikipedia.org/wiki/Sidereal_time
    // TODO take into account leap seconds
    double tj2000 = computeEPOCH(2000, 1, 1, 12, 0, 0, 0);
    double deltat = (time - tj2000) / 1000.0 / 86400.0;
    double rotationAngleRad = fmod(2.0 * M_PI * (0.7790572732640 + 1.00273781191135448 * deltat), 2.0 * M_PI);
    double raVal = ra - rotationAngleRad;
    if (raVal < 0)
        raVal += 2.0 * M_PI;

    float phi = raVal;
    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    float e = eastx * x + easty * y + eastz * z;
    float n = northx * x + northy * y + northz * z;
    float u = zenithx * x + zenithy * y + zenithz * z;

    float elVal = atan(u / sqrt(e*e + n*n)) / M_PI * 180.0;
    float azVal = 90 - atan2(n, e) / M_PI * 180.0;

    // Star catalog RA and DEC are int radian, return values in these units
    if (az != NULL)
        *az = azVal;

    if (el != NULL)
        *el = elVal;

    return status;

}
int azelToradec(double time, float geodeticLatitudeDeg, float longitudeDeg, float altitudeM, float az, float el, float *ra, float *dec)
{
    int status = ASCC_OK;

    float x0 = 0.0;
    float y0 = 0.0;
    float z0 = 0.0;
    float d = 0.0;

    geodeticToXYZ(geodeticLatitudeDeg, longitudeDeg, altitudeM, &x0, &y0, &z0, &d);

    float vertx = x0;
    float verty = y0;
    float vertz = z0 + d;
    float mag = sqrt(vertx * vertx + verty * verty + vertz * vertz);
    
    // zenith
    float zenithx = vertx / mag;
    float zenithy = verty / mag;
    float zenithz = vertz / mag;

    // zhat
    float zhatx = 0.0;
    float zhaty = 0.0;
    float zhatz = 1.0;
    // East is zhat cross vert
    float eastx = zhaty * zenithz - zhatz * zenithy;
    float easty = -zhatx * zenithz + zhatz * zenithx;
    float eastz = zhatx * zenithy - zhaty * zenithx;

    mag = sqrt(eastx * eastx + easty * easty + eastz * eastz);
    eastx /= mag;
    easty /= mag;
    eastz /= mag;

    // Not anywhere near a pole. Ignore cases with lat = 90 or -90
    // north = zenith cross east
    float northx = zenithy * eastz - zenithz * easty;
    float northy = -zenithx * eastz + zenithz * eastx;
    float northz = zenithx * easty - zenithy * eastx;
    // Should be fine, but force unit length anyway
    mag = sqrt(northx * northx + northy * northy + northz * northz);
    northx /= mag;
    northy /= mag;
    northz /= mag;

    // printf("x: %.2f y: %.2f z: %.2f\n", x0, y0, z0);

    // Now have a coordinate system local east, north, up (geodetic up)
    // represented in X, Y, Z (ECEF) coodinates.
    // They share the same origin.
    // A unit vector in the ENU system can be represented in the XYZ system,
    // from which RA and DEC can be calculated
    
    // Azimuth is east from north
    float theta = (90.0 - el) * M_PI / 180.0;
    float phi = (90.0 - az) * M_PI / 180.0;
    float e = cos(phi) * sin(theta);
    float n = sin(phi) * sin(theta);
    float u = cos(theta);


    float x = e * eastx + n * northx + u * zenithx;
    float y = e * easty + n * northy + u * zenithy;
    float z = e * eastz + n * northz + u * zenithz;

    float decVal = atan(z / sqrt(x*x + y*y));
    float raVal = atan2(y, x);
    // Take into account the time
    // add Earth's rotation since J2000 12 noon.
    // https://en.wikipedia.org/wiki/Sidereal_time
    // TODO take into account leap seconds
    double tj2000 = computeEPOCH(2000, 1, 1, 12, 0, 0, 0);
    double deltat = (time - tj2000) / 1000.0 / 86400.0;
    double rotationAngleRad = fmod(2.0 * M_PI * (0.7790572732640 + 1.00273781191135448 * deltat), 2.0 * M_PI);
    double raVal1 = raVal + rotationAngleRad;
    raVal1 = fmod(raVal1, 2.0 * M_PI);
    // printf("raVal modded: %lf %lf\n", raVal / M_PI * 180.0, raVal1 / M_PI * 180.0);

    // Star catalog RA and DEC are int radian, return values in these units
    if (ra != NULL)
        *ra = raVal1;

    if (dec != NULL)
        *dec = decVal;

    return status;
}

void geodeticToXYZ(float glat, float glon, float altm, float *x, float *y, float *z, float *dVal)
{
    // http://wiki.gis.com/wiki/index.php/WGS84 for values a and b.
    double a = 6378137.0;
    double b = 6356752.314245;


    double bovera = b / a;
    double a2 = a * a;
    double b2 = b * b;

    double latrad = glat * M_PI / 180.0;
    double lonrad = glon * M_PI / 180.0;

    double tanlat = tan(latrad);
    double tanlat2 = tanlat * tanlat;
    double rho0 = a / sqrt(tanlat2 * bovera + 1.0);
    double z0 = b * sqrt(1.0 - rho0 * rho0 / a2);
    if (glat < 0)
        z0 *= -1.0;
    double d = z0 * (a2 / b2 - 1.0);
    double s = sqrt((z0 + d)*(z0 + d) + rho0 * rho0);
    double s1 = s + altm;
    double rho1 = s1 * cos(latrad);

    double z1 = (s1 * sin(latrad) - d);
    double x1 = rho1 * cos(lonrad);
    double y1 = rho1 * sin(lonrad);

    // printf("latrad: %f, lonrada: %f, rho1: %f, x1: %f, y1: %f, z1: %f\n", latrad, lonrad, rho1, x1, y1, z1);

    if (x != NULL)
        *x = (float)x1;
    if (y != NULL)
        *y = (float)y1;
    if (z != NULL)
        *z = (float)z1;

    if (dVal != NULL)
        *dVal = (float)d;

    return;
}

int selectStars(ProgramState *state, double imageTime, CalibrationStar *calStars)
{
    if (state == NULL || calStars == NULL)
        return 0;
        
    int starInd = 0;
    Star *star = NULL;

    float starRa = 0.0;
    float starDec = 0.0;
    float starAz = 0.0;
    float starEl = 0.0;

    // Years since J2000: approximate enough for proper motion calculation
    // TODO implement a precise Julian day and year calculator
    // TODO take into account parallax
    float yearsSinceJ2000 = (float) (imageTime - J200EPOCH) / 1000.0 / 86400. / 365.25;

    int nStars = 0;

    while (nStars < state->nCalibrationStars && starInd < state->nStars)
    {
        star = &state->starData[starInd];
        starRa = fmod(star->rightAscensionRadian + star->raProperMotionRadianPerYear * yearsSinceJ2000, 2.0 * M_PI);
        starDec = fmod(star->declinationRadian + star->decProperMotionRadianPerYear * yearsSinceJ2000, 2.0 * M_PI);

        // Convert RA and DEC to az and el
        radecToazel(imageTime, state->siteLatitudeGeodetic, state->siteLongitudeGeodetic, state->siteAltitudeMetres, starRa, starDec, &starAz, &starEl);
        // If star is in field of view, increase nCalStars
        // and store this star's index in the list of calibration stars
        if (starEl > CALIBRATION_ELEVATION_BOUND)
        {
            calStars[nStars].star = star;
            if (starInd != calStars[nStars].catalogIndex)
                calStars[nStars].newStarAtThisIndex = true;
            else
                calStars[nStars].newStarAtThisIndex = false;
            calStars[nStars].catalogIndex = starInd;
            calStars[nStars].predictedAz = starAz;
            calStars[nStars].predictedEl = starEl;
            calStars[nStars].magnitude = star->visualMagnitudeTimes100 / 100.0;
            // TODO handle change in star ID for this image
            // Used to reject stars which have moved too much from one image to the next
            calStars[nStars].previousImageMomentColumn= calStars[nStars].imageMomentColumn;
            calStars[nStars].previousImageMomentRow = calStars[nStars].imageMomentRow;
            nStars++;
        }
        // Next brightest star
        starInd++;
    }

    return nStars;

}

// Returns number of pixels used to construct moments
int calculateMoments(ProgramState *state, uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], CalibrationStar *cal, int boxHalfWidth, float boxCenterColumn, float boxCenterRow, int pixelThreshold)
{
    if (state == NULL || image == NULL || cal == NULL)
        return 0;

    int c0 = 0;
    int r0 = 0;
    float c1 = 0;
    float r1 = 0;
    int pixVal = 0.0;
    int momentCounter = 0;
    int boxTotal = 0;
    float meanSignal = 0.0;
    int maxSignalAboveThreshold = 0;
    float meanAzElX = 0.0;
    float meanAzElY = 0.0;
    float meanAzElZ = 0.0;
    // printf("Threshold: %d, meanHeight: %f\n", pixelThreshold, cal->meanImageSignalAboveThreshold);

    for (int c = -boxHalfWidth; c <= boxHalfWidth; c++)
    {
        for (int r = -boxHalfWidth; r <= boxHalfWidth; r++)
        {

            c0 = floorf(boxCenterColumn) + c;
            r0 = floorf(boxCenterRow) + r;
            // printf("%d\n", image[c0][r0]);
            if (c0 >=0 && c0 < IMAGE_COLUMNS && r0 >= 0 && r0 < IMAGE_ROWS)
            {
                pixVal = image[c0][r0];
                if (pixVal < pixelThreshold || pixVal > MAX_PEAK_SIGNAL_FOR_MOMENTS)
                    continue;
                pixVal -= pixelThreshold;
                if (pixVal < 0)
                    pixVal = 0;
                momentCounter++;
                boxTotal += pixVal;
                c1 += (float)c0 * (float)pixVal;
                r1 += (float)r0 * (float)pixVal;
                meanAzElX += state->pixelX[c0][r0] * (float)pixVal;
                meanAzElY += state->pixelY[c0][r0] * (float)pixVal;
                meanAzElZ += state->pixelZ[c0][r0] * (float)pixVal;
                if (pixVal > maxSignalAboveThreshold)
                    maxSignalAboveThreshold = pixVal;
            }        
        }
    }
    if (momentCounter > 0 && boxTotal > 0)
    {
        // printf("momC: %f\n", c1 / (float)boxTotal);
        cal->imageMomentColumn = c1 / (float)boxTotal + 0.5;
        cal->imageMomentRow = r1 / (float)boxTotal + 0.5;
        cal->measuredAzElX = meanAzElX / (float)boxTotal;
        cal->measuredAzElY = meanAzElY / (float)boxTotal;
        cal->measuredAzElZ = meanAzElZ / (float)boxTotal;
        cal->meanImageSignalAboveThreshold = (float)boxTotal / (float)momentCounter;
        cal->backgroundThreshold = (float)pixelThreshold;
    }
    else
    {
        cal->imageMomentColumn = 0.0;
        cal->imageMomentRow = 0.0;
        cal->measuredAzElX = 0.0;
        cal->measuredAzElY = 0.0;
        cal->measuredAzElZ = 0.0;
        cal->meanImageSignalAboveThreshold = 0.0;
        cal->backgroundThreshold = 0.0;
    }

    return momentCounter;
}

float calculateMeanSignal(uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], int boxHalfWidth, float boxCenterColumn, float boxCenterRow)
{
    if (image == NULL)
        return NAN;

    int c0 = 0;
    int r0 = 0;
    int momentCounter = 0;
    int boxTotal = 0;
    float mean = NAN;

    for (int c = -boxHalfWidth; c <= boxHalfWidth; c++)
    {
        for (int r = -boxHalfWidth; r <= boxHalfWidth; r++)
        {
            c0 = floorf(boxCenterColumn) + c;
            r0 = floorf(boxCenterRow) + r;
            if (c0 >=0 && c0 < IMAGE_COLUMNS && r0 >= 0 && r0 < IMAGE_ROWS)
            {
                momentCounter++;
                boxTotal += image[c0][r0];
            }        
        }
    }
    if (momentCounter > 0 && boxTotal > 0)
        mean = (float)boxTotal / (float)momentCounter;

    return mean;

}

// Returns number of pixels used to estimate maximum
int calculatePositionOfMax(uint16_t image[IMAGE_COLUMNS][IMAGE_ROWS], int boxHalfWidth, float boxCenterColumn, float boxCenterRow, int *cmax, int *rmax)
{
    if (image == NULL || cmax == NULL || rmax == NULL)
        return 0;

    int c0 = 0;
    int r0 = 0;
    int momentCounter = 0;
    int cm = 0;
    int rm = 0;
    int max = 0;

    for (int c = -boxHalfWidth; c <= boxHalfWidth; c++)
    {
        for (int r = -boxHalfWidth; r <= boxHalfWidth; r++)
        {
            c0 = floorf(boxCenterColumn) + c;
            r0 = floorf(boxCenterRow) + r;
            if (c0 >=0 && c0 < IMAGE_COLUMNS && r0 >= 0 && r0 < IMAGE_ROWS)
            {
                momentCounter++;
                if (image[c0][r0] > max)
                {
                    max = image[c0][r0];
                    cm = c0;
                    rm = r0;
                }
            }        
        }
    }
    if (momentCounter > 0)
    {
        *cmax = cm;
        *rmax = rm;
    }

    return momentCounter;

}
