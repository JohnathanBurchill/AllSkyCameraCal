#include "analysis.h"

#include "main.h"
#include <stdio.h>
#include <stdbool.h>
#include <fts.h>
#include <string.h>
#include <math.h>

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

    char cdfVarName[CDF_VAR_NAME_LEN + 1] = {0};

    long *calStarInds = NULL;
    float *starPredictedAzs = NULL;
    float *starPredictedEls = NULL;
    float *starXs = NULL;
    float *starYs = NULL;
    float *starZs = NULL;
    float *starMeasuredAzs = NULL;
    float *starMeasuredEls = NULL;
    float *stardAzs = NULL;
    float *stardEls = NULL;
    float *starMagnitudes = NULL;
    long *starImageColumns = NULL;
    long *starImageRows = NULL; 
    float *starImageMomentColumns = NULL;
    float *starImageMomentRows = NULL; 

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

    double imageTime = 0.0;
    char timeString[EPOCH4_STRING_LEN+1];

    uint16_t imagery[256][256] = {0};
    uint16_t *imagePointer = &imagery[0][0];
    long index = 0;

    float minRa = 0.0;
    float maxRa = 0.0;
    float minDec = 0.0;
    float maxDec = 0.0;
    float tmp = 0.0;

    // Updated on each function cal;
    Star *star = NULL;
    double t0J2000 = computeEPOCH(2000, 1, 1, 12, 0, 0, 0);
    float yearsSinceJ2000 = 0.0;
    double starRa = 0.0;
    double starDec = 0.0;
    int nCalStars = 0;
    int nCalStarsKept = 0;
    int starInd = 0;
    float starAz = 0.0;
    float starEl = 0.0;
    float starAzElDistance = 0.0;
    float starMinAzElDistance = 0.0;
    calStarInds = calloc(state->nCalibrationStars, sizeof(long));
    starPredictedAzs = calloc(state->nCalibrationStars, sizeof(float));
    starPredictedEls = calloc(state->nCalibrationStars, sizeof(float));
    starXs = calloc(state->nCalibrationStars, sizeof(float));
    starYs = calloc(state->nCalibrationStars, sizeof(float));
    starZs = calloc(state->nCalibrationStars, sizeof(float));
    stardAzs = calloc(state->nCalibrationStars, sizeof(float));
    stardEls = calloc(state->nCalibrationStars, sizeof(float));
    starMeasuredAzs = calloc(state->nCalibrationStars, sizeof(float));
    starMeasuredEls = calloc(state->nCalibrationStars, sizeof(float));
    starMagnitudes = calloc(state->nCalibrationStars, sizeof(float));
    starImageColumns = calloc(state->nCalibrationStars, sizeof(long));
    starImageRows = calloc(state->nCalibrationStars, sizeof(long));
    starImageMomentColumns = calloc(state->nCalibrationStars, sizeof(float));
    starImageMomentRows = calloc(state->nCalibrationStars, sizeof(float));
    float totalInner = 0.0;
    int c0 = 0;
    int r0 = 0;
    float c1 = 0.0;
    float r1 = 0.0;
    int pixVal = 0;
    float totalOuter = 0.0;
    float momentCounter = 0.0;
    float starx = 0.0;
    float stary = 0.0;
    float starz = 0.0;
    float dx = 0.0;
    float dy = 0.0;
    float dz = 0.0;
    float magnitude = 0.0;
    float azhatx = 0.0;
    float azhaty = 0.0;
    float azhatz = 0.0;
    float elhatx = 0.0;
    float elhaty = 0.0;
    float elhatz = 0.0;
    float meanX = 0.0;
    float meanY = 0.0;
    float meanZ = 0.0;

    float nSignal = 0.0;
    float minSignal = 0.0;
    float maxSignal = 0.0;
    float meanSignal = 0.0;

    bool foundNearest = false;

    float middleC = IMAGE_COLUMNS / 2.0;
    float middleR = IMAGE_ROWS / 2.0;
    float angularDisplacementAboutZenithRadian = 0.0;
    float angularDisplacementOfZenithRadian = 0.0;

    if (calStarInds == NULL || starPredictedAzs == NULL || starPredictedEls == NULL || starMeasuredAzs == NULL || starMeasuredEls == NULL || starMagnitudes == NULL || starImageColumns == NULL || starImageRows == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    for (int i = 0; i < state->nCalibrationStars; i++)
        calStarInds[i] = -1;

    // printf("El minmax: %.2f %.2f\n", state->minElevation, state->maxElevation);
    // printf("Az minmax: %.2f %.2f\n", state->minAzimuth, state->maxAzimuth);
    for (long ind = 0; ind < nFileImages; ind++)
    {
        index = ind;
        // printf("%s: i = %ld, nImageFiles: %ld\n", l1file, ind, nFileImages);
        snprintf(cdfVarName, CDF_VAR_NAME_LEN + 1, "thg_asf_%s_epoch", state->site);
        cdfStatus = CDFgetVarRangeRecordsByVarName(cdf, cdfVarName, index, index, &imageTime);
        if (cdfStatus != CDF_OK)
            continue;
        if (imageTime < state->firstCalTime || imageTime > state->lastCalTime)
            continue;

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

        yearsSinceJ2000 = (imageTime - t0J2000) / 1000.0 / 86400. / 365.25;
        nCalStars = 0;
        starInd = 0;
        while (nCalStars < state->nCalibrationStars && starInd < state->nStars)
        {
            star = &state->starData[starInd];
            // Years since J2000: approximate enough for proper motion calculation
            // TODO implement a precise Julian day and year calculator
            // TODO take into account parallax
            starRa = fmod(star->rightAscensionRadian + star->raProperMotionRadianPerYear * yearsSinceJ2000, 2.0 * M_PI);
            starDec = fmod(star->declinationRadian + star->decProperMotionRadianPerYear * yearsSinceJ2000, 2.0 * M_PI);

            // Convert RA and DEC to az and el
            radecToazel(imageTime, state->siteLatitudeGeodetic, state->siteLongitudeGeodetic, state->siteAltitudeMetres, starRa, starDec, &starAz, &starEl);
            // If star is in field of view, increase nCalStars
            // and store this star's index in the list of calibration stars
            if (starEl > CALIBRATION_ELEVATION_BOUND)
            {
                calStarInds[nCalStars] = starInd;
                starPredictedAzs[nCalStars] = starAz;
                starPredictedEls[nCalStars] = starEl;
                starMagnitudes[nCalStars] = star->visualMagnitudeTimes100/100.0;
                nCalStars++;
            }
            // Next brightest star
            starInd++;
        }
        // printf("Got %d calibration stars.\n", nCalStars);
        // for (int i = 0; i < nCalStars; i++)
        //     printf("star %d (ind %ld): magnitude = %.2lf\n", i, calStarInds[i], state->starData[calStarInds[i]].visualMagnitudeTimes100/100.0);
        nCalStarsKept = 0;
        for (int i = 0; i < nCalStars; i++)
        {
            starMinAzElDistance = 10000000000.0;
            foundNearest = false;
            for (int c = 0; c < IMAGE_COLUMNS; c++)
            {
                for (int r = 0; r < IMAGE_COLUMNS; r++)
                {
                    if (!isfinite(state->l2CameraAzimuths[c][r]) || !isfinite(state->l2CameraElevations[c][r]))
                        continue;
                    // TODO use a GPU
                    starx = cos((90.0 - starPredictedAzs[i])*M_PI/180.0) * cos(starPredictedEls[i]*M_PI/180.0);
                    stary = sin((90.0 - starPredictedAzs[i])*M_PI/180.0) * cos(starPredictedEls[i]*M_PI/180.0);
                    starz = sin(starPredictedEls[i]*M_PI/180.0);
                    starXs[i] = starx;
                    starYs[i] = stary;
                    starZs[i] = starz;
                    dx = starx - state->pixelX[c][r];
                    dy = stary - state->pixelY[c][r];
                    dz = starz - state->pixelZ[c][r];                    
                    starAzElDistance = sqrt(dx * dx + dy * dy + dz * dz);
                    if (starAzElDistance < starMinAzElDistance)
                    {
                        starMinAzElDistance = starAzElDistance;
                        starImageColumns[i] = c;
                        starImageRows[i] = r;
                        foundNearest = true;
                    }
                }
            }
            if (foundNearest)
            {
                totalInner = 0.0;
                momentCounter = 0.0;
                c1 = 0.0;
                r1 = 0.0;
                meanX = 0.0;
                meanY = 0.0;
                meanZ = 0.0;
                maxSignal = -1.0;
                minSignal = 1e10;
                for (int c = -2; c < 3; c++)
                {
                    for (int r = -2; r < 3; r++)
                    {
                        c0 = starImageColumns[i] + c;
                        r0 = starImageRows[i] + r;
                        if (c0 >=0 && c0 < IMAGE_COLUMNS && r0 >= 0 && r0 < IMAGE_ROWS)
                        {
                            if (fabsf(c) > 1 || fabsf(c) > 1)
                            {
                                meanSignal += imagery[c][r];
                                nSignal++;
                                if (imagery[c][r] > maxSignal)
                                    maxSignal = imagery[c][r];
                                else if (imagery[c][r] < minSignal)
                                    minSignal = imagery[c][r];
                            }
                        }
                    }
                    meanSignal /= nSignal;                    
                }
                for (int c = -1; c < 2; c++)
                {
                    for (int r = -1; r < 2; r++)
                    {
                        c0 = starImageColumns[i] + c;
                        r0 = starImageRows[i] + r;
                        if (c0 >=0 && c0 < IMAGE_COLUMNS && r0 >= 0 && r0 < IMAGE_ROWS)
                        {
                            pixVal = imagery[c0][r0] - meanSignal - 100;
                            if (pixVal < 0.0)
                                pixVal = 0.0;
                            momentCounter++;
                            totalInner += pixVal;
                            c1 += c0 * pixVal;
                            r1 += r0 * pixVal;
                            meanX += state->pixelX[c0][r0] * pixVal;
                            meanY += state->pixelY[c0][r0] * pixVal;
                            meanZ += state->pixelZ[c0][r0] * pixVal;
                        }                    
                    }
                }
                if (momentCounter > 0 && totalInner > 0)
                {
                    nCalStarsKept++;
                    c1 /= totalInner;
                    r1 /= totalInner;
                    starImageMomentColumns[i] = c1;
                    starImageMomentRows[i] = r1;
                    meanX /= totalInner;
                    meanY /= totalInner;
                    meanZ /= totalInner;
                    // Calculate dRa and dDec from measuremed (interpolated) values minus predicted values
                    dx = starXs[i] - meanX;
                    dy = starYs[i] - meanY;
                    dz = starZs[i] - meanZ;
                    // printf("dx,dy,dz = %f %f %f\n", dx, dy, dz);
                    starMeasuredAzs[i] = 90.0 - atan2(meanY, meanX) / M_PI * 180.0;
                    starMeasuredEls[i] = atan(meanZ / hypotf(meanX, meanY)) / M_PI * 180.0;

                    // dRa and dDec
                    // rhat is (starx, stary, starz)
                    azhatx = - stary;
                    azhaty = starx;
                    azhatz = 0.0;
                    magnitude = sqrt(azhatx * azhatx + azhaty * azhaty + azhatz * azhatz);
                    azhatx /= magnitude;
                    azhaty /= magnitude;
                    elhatx = - starz * azhaty;
                    elhaty = starz * azhatx;
                    elhatz = starx * azhaty - stary * azhatx;
                    magnitude = sqrt(elhatx * elhatx + elhaty * elhaty + elhatz * elhatz);
                    elhatx /= magnitude;
                    elhaty /= magnitude;
                    elhatz /= magnitude;

                    // TODO need to multiply stardRas by cos(dec)?
                    stardAzs[i] = (dx * azhatx + dy * azhaty + dz * azhatz);
                    stardEls[i] = (dx * elhatx + dy * elhaty + dz * elhatz);
                }
                else
                {
                    // Flag this star as not used
                    calStarInds[i] = -1;
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
            float meandAz = 0.0;
            float meandEl = 0.0;
            for (int i = 0; i < nCalStars; i++)
            {
                if (calStarInds[i] != -1)
                {
                    meandAz += stardAzs[i];
                    // A rotation away from zenith (in declination)
                    // will be positive on one side and negative on the other
                    // TODO improve this estimate taking this into account?
                    // For now, take magnitude of error only for elevations
                    meandEl += fabsf(stardEls[i]); 
                }
            }
            meandAz /= (float)nCalStarsKept;
            meandEl /= (float)nCalStarsKept;

            for (int i = 0; i < nCalStars; i++)
            {
                if (calStarInds[i] != -1)
                {
                    // TODO update the pixel azimuths and elevations by adding the dAzs and dEls?
                    // and store all info in a CDF
                    // Looks like the AZ / EL map needs to be rotated and translated,
                    // not just translated.
                    printf("%lf %ld %ld %.3f %.3f %.3f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f\n", imageTime, starImageColumns[i], starImageRows[i], starImageMomentColumns[i], starImageMomentRows[i], starMagnitudes[i], starPredictedAzs[i], starPredictedEls[i], starMeasuredAzs[i], starMeasuredEls[i], stardAzs[i] / M_PI * 180.0, stardEls[i] / M_PI * 180.0, meandAz / M_PI * 180.0, meandEl / M_PI * 180.0);
                }
            }
        }


    }

cleanup:
    if (cdf != NULL)
        CDFclose(cdf);

    if (calStarInds != NULL)
        free(calStarInds);
    if (starPredictedAzs != NULL)
        free(starPredictedAzs);
    if (starPredictedEls != NULL)
        free(starPredictedEls);
    if (starXs != NULL)
        free(starXs);
    if (starYs != NULL)
        free(starYs);
    if (starZs != NULL)
        free(starZs);
    if (stardAzs != NULL)
        free(stardAzs);
    if (stardEls != NULL)
        free(stardEls);
    if (starMeasuredAzs != NULL)
        free(starMeasuredAzs);
    if (starMeasuredEls != NULL)
        free(starMeasuredEls);
    if (starMagnitudes != NULL)
        free(starMagnitudes);
    if (starImageColumns != NULL)
        free(starImageColumns);
    if (starImageRows != NULL)
        free(starImageRows);
    if (starImageMomentColumns != NULL)
        free(starImageMomentColumns);
    if (starImageMomentRows != NULL)
        free(starImageMomentRows);

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