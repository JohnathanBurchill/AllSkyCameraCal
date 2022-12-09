/*

    AllSkyCameraCal: export.c

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

#include "export.h"
#include "main.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <unistd.h>

#include <cdf.h>

int exportCdf(ProgramState *state)
{
    if (state == NULL)
        return ASCC_ARGUMENTS;

    if (state->nImages == 0)
        return ASCC_CDF_EXPORT_NO_DATA;

    CDFid cdf = NULL;
    char statusMessage[CDF_STATUSTEXT_LEN+1] = {0};

    char cdfFilename[CDF_PATHNAME_LEN+1];
    char firstTime[EPOCHx_STRING_MAX];
    char lastTime[EPOCHx_STRING_MAX];
    char format[EPOCHx_FORMAT_MAX] = "<year><mm.02><dom.02>T<hour><min><sec>";
    encodeEPOCHx(state->firstCalTime, format, firstTime);
    encodeEPOCHx(state->lastCalTime, format, lastTime);
    snprintf(cdfFilename, CDF_PATHNAME_LEN, "%s/themis_%s_camera_pointing_errors_%s_%s_%s", state->exportdir, state->site, firstTime, lastTime, EXPORT_CDF_VERSION_STRING);

    CDFstatus cdfstatus = CDF_OK;
    
    snprintf(state->cdfFullFilename, CDF_PATHNAME_LEN + 5, "%s.cdf", cdfFilename);
    if (access(state->cdfFullFilename, F_OK) == 0 && state->overwriteCdf)
    {
        if (state->verbose)
            fprintf(stderr, "Overwriting CDF\n");
        remove(state->cdfFullFilename);
    }

    cdfstatus = CDFcreateCDF(cdfFilename, &cdf);

    if (cdfstatus != CDF_OK)
    {
        if (state->verbose)
        {
            CDFgetStatusText(cdfstatus, statusMessage);
            fprintf(stderr, "%s\n", statusMessage);
        }
        return ASCC_CDF_WRITE;
    }

    cdfstatus = CDFsetEncoding(cdf, NETWORK_ENCODING);
    if (cdfstatus != CDF_OK)
    {
        if (state->verbose)
        {
            CDFgetStatusText(cdfstatus, statusMessage);
            fprintf(stderr, "%s\n", statusMessage);
        }
        return ASCC_CDF_WRITE;
    }

    // TODO
    // Blocking factor


    int status = ASCC_OK;

    // Timestamp
    long nDims = 0;
    long dimSizes[CDF_MAX_DIMS] = {0};
    long recVariance = VARY;
    long dimsVariance[2] = {VARY,VARY};
    long varNum = 0;
    long compressionParam[1] = {6};
    cdfstatus = CDFcreatezVar(cdf, "Timestamp", CDF_EPOCH, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->imageTimes);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 2;
    dimSizes[0] = 3;
    dimSizes[1] = 3;
    dimsVariance[0] = VARY;
    dimsVariance[1] = VARY;
    cdfstatus = CDFcreatezVar(cdf, "PointingErrorDCM", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->pointingErrorDcms);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 1;
    dimSizes[0] = 3;
    dimsVariance[0] = VARY;
    cdfstatus = CDFcreatezVar(cdf, "RotationAxis", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->rotationVectors);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    cdfstatus = CDFcreatezVar(cdf, "RotationAngle", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->rotationAngles);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "SiteAbbreviation", CDF_CHAR, strlen(state->site), nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->site);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "SiteLatitude", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, &state->siteLatitudeGeodetic);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "SiteLongitude", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, &state->siteLongitudeGeodetic);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "SiteAltitude", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, &state->siteAltitudeMetres);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "ReferenceDateGenerated", CDF_CHAR, strlen(state->calibrationDateGenerated), nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->calibrationDateGenerated);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreatezVar(cdf, "ReferenceDateUsed", CDF_CHAR, strlen(state->calibrationDateUsed), nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->calibrationDateUsed);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 2;
    dimSizes[0] = IMAGE_COLUMNS;
    dimSizes[1] = IMAGE_ROWS;
    dimsVariance[0] = VARY;
    dimsVariance[1] = VARY;
    cdfstatus = CDFcreatezVar(cdf, "ReferenceElevations", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->referenceElevations);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFcreatezVar(cdf, "ReferenceAzimuths", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->referenceAzimuths);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }


    nDims = 0;
    cdfstatus = CDFcreatezVar(cdf, "CalibrationEpoch", CDF_EPOCH, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, &state->calibratedEpoch);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 2;
    dimSizes[0] = IMAGE_COLUMNS;
    dimSizes[1] = IMAGE_ROWS;
    dimsVariance[0] = VARY;
    dimsVariance[1] = VARY;
    cdfstatus = CDFcreatezVar(cdf, "CalibratedElevations", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->calibratedElevations);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFcreatezVar(cdf, "CalibratedAzimuths", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, 1, state->calibratedAzimuths);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    nDims = 0;
    recVariance = VARY;
    cdfstatus = CDFcreatezVar(cdf, "CalibrationStarCount", CDF_UINT2, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->nCalibrationStarsUsed);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    // Global attributes
    long attrNum = 0;
    long entry = 0;
    cdfstatus = CDFcreateAttr(cdf, "Author", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *name = "Johnathan K. Burchill";
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(name), name);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "Mission", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *mission = "THEMIS ASI";
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(mission), mission);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "Site", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(state->site), state->site);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "Processor", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *processorString = "AllSkyCameraCal version " PROGRAM_VERSION_STRING;
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(processorString), processorString);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "ProcessingStart", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char date[EPOCHx_STRING_MAX+1];
    char attrformat[EPOCHx_FORMAT_MAX+1] = "UTC=<year>-<mm.02>-<dom.02>T<hour>:<min>:<sec>";
    encodeEPOCHx(state->processingStartEpoch, attrformat, date);
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(date), date);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "ProcessingStop", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    encodeEPOCHx(state->processingStopEpoch, attrformat, date);
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(date), date);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "ProcessingCommand", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    size_t commandLength = 0;
    for (int i = 0; i < state->processingCommandLength; i++)
        commandLength += strlen(state->processingCommand[i]) + 1;
    commandLength--;
    char *command = calloc(commandLength, 1);
    if (command == NULL)
    {
        status = ASCC_MEM;
        goto cleanup;
    }
    for (int i = 0; i < state->processingCommandLength; i++)
    {
        command = strncat(command, state->processingCommand[i], commandLength);
        if (i != state->processingCommandLength-1)
            command = strcat(command, " ");
    }
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(command), command);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    free(command);

    cdfstatus = CDFcreateAttr(cdf, "Filename", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *cdfBasename = basename(state->cdfFullFilename);
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(cdfBasename), cdfBasename);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "ReferenceCalibrationFilename", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *calFile = state->skymap ? state->skymapfilename : state->l2filename;
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(calFile), calFile);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "ImageryFilenames", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    for (int i = 0; i < state->nl1filenames; i++)
    {
        char *file = basename(state->l1filenames[i]);
        cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry + i, CDF_CHAR, strlen(file), file);
        if (cdfstatus != CDF_OK)
        {
            status = ASCC_CDF_WRITE;
            goto cleanup;
        }
    }
    cdfstatus = CDFcreateAttr(cdf, "TEXT", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char *text = "AllSkyCameraCal (https://github.com/JohnathanBurchill/AllSkyCameraCal).";
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(text), text);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    // Variable attributes
    cdfstatus = CDFcreateAttr(cdf, "Name", VARIABLE_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFcreateAttr(cdf, "Description", VARIABLE_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = CDFcreateAttr(cdf, "Unit", VARIABLE_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = addVariableAttributes(cdf, "Timestamp", "Epoch of image used for calibration. Milliseconds from 0000-01-01:00:00:00 UT, no leap seconds.", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "PointingErrorDCM", "Least-squares estimate of the direction cosine matrix that transforms predicted star positions to measured star positions. ENU system.", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "RotationAxis", "Rotation axis in ENU system calculated from pointing error DCM.", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "RotationAngle", "Pointing error as a rotation angle about the rotation axis.", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "SiteAbbreviation", "Camera site abbreviation", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "SiteLatitude", "Camera site geodetic latitude", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "SiteLongitude", "Camera site longitude", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "SiteAltitude", "Camera site altitude", "m");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "ReferenceDateGenerated", "Date the reference calibration file was generated", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "ReferenceDateUsed", "Date and UT of images files used for reference calibration", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "ReferenceElevations", "Reference calibration file elevations centred on each pixel", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "ReferenceAzimuths", "Reference calibration file azimuths centred on each pixel", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "CalibrationEpoch", "Epoch of revised calibration (mean of calibration image times). Milliseconds from 0000-01-01:00:00:00 UT, no leap seconds.", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "CalibratedElevations", "Revised calibration of elevations centred on each pixel", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "CalibratedAzimuths", "Revised calibration of azimuths centred on each pixel", "degree");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = addVariableAttributes(cdf, "CalibrationStarCount", "Number of stars used to estimate pointing error", "-");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

cleanup:
    if (cdf != NULL)
        CDFclose(cdf);

    if (cdfstatus != CDF_OK && state->verbose)
    {
        CDFgetStatusText(cdfstatus, statusMessage);
        fprintf(stderr, "export.c: %s\n", statusMessage);
    }

    return ASCC_OK;
}

int addVariableAttributes(CDFid cdf, char *name, char *description, char *units)
{
    int cdfstatus = CDF_OK;
    long varNum = CDFgetVarNum(cdf, name);

    cdfstatus = CDFputAttrzEntry(cdf, CDFgetAttrNum(cdf, "Name"), varNum, CDF_CHAR, strlen(name), name);
    if (cdfstatus != CDF_OK)
        goto cleanup;

    cdfstatus = CDFputAttrzEntry(cdf, CDFgetAttrNum(cdf, "Description"), varNum, CDF_CHAR, strlen(description), description);
    if (cdfstatus != CDF_OK)
        goto cleanup;

    cdfstatus = CDFputAttrzEntry(cdf, CDFgetAttrNum(cdf, "Unit"), varNum, CDF_CHAR, strlen(units), units);
    if (cdfstatus != CDF_OK)
        goto cleanup;

cleanup:
    return cdfstatus;
}
