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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

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
    snprintf(cdfFilename, CDF_PATHNAME_LEN, "themis_%s_camera_pointing_errors_%s_%s_%s", state->site, firstTime, lastTime, EXPORT_CDF_VERSION_STRING);
    CDFstatus cdfstatus = CDFcreateCDF(cdfFilename, &cdf);

    int status = ASCC_OK;

    if (cdfstatus != CDF_OK)
    {
        fprintf(stderr, "Could not create CDF file.\n");
        return ASCC_CDF_WRITE;
    }

    // TODO
    // Blocking factor

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

    cdfstatus = CDFcreateAttr(cdf, "ProcessingDate", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    double unixTime = (double) time(NULL);
    double epochTime = 0;
    UnixTimetoEPOCH(&unixTime, &epochTime, 1);
    char date[EPOCHx_STRING_MAX+1];
    char attrformat[EPOCHx_FORMAT_MAX+1] = "UTC=<year>-<mm.02>-<dom.02>T<hour>:<min>:<sec>";
    encodeEPOCHx(epochTime, attrformat, date);
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(date), date);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "Filename", GLOBAL_SCOPE, &attrNum);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    char cdfFullFilename[CDF_PATHNAME_LEN + 5];
    snprintf(cdfFullFilename, CDF_PATHNAME_LEN + 5, "%s.cdf", cdfFilename);
    char *cdfBasename = basename(cdfFullFilename);
    cdfstatus = CDFputAttrgEntry(cdf, attrNum, entry, CDF_CHAR, strlen(cdfBasename), cdfBasename);
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

    cdfstatus = CDFcreateAttr(cdf, "CalibrationFilename", GLOBAL_SCOPE, &attrNum);
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

    cdfstatus = CDFcreateAttr(cdf, "ImageFilenames", GLOBAL_SCOPE, &attrNum);
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

    cdfstatus = addVariableAttributes(cdf, "Timestamp", "CDF Epoch", "milliseconds from 0000-01-01:00:00:00 UT");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }
    cdfstatus = addVariableAttributes(cdf, "PointingErrorDCM", "3x3 direction cosine matrix that best rotates predicted star positions to measured star positions (in a least-squares sense). ENU system.", "-");
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
    cdfstatus = addVariableAttributes(cdf, "RotationAngle", "Pointing error as a rotation angle about the rotation axis.", "degrees");
    if (cdfstatus != CDF_OK)
    {
        status = ASCC_CDF_WRITE;
        goto cleanup;
    }

cleanup:
    if (cdf != NULL)
        CDFclose(cdf);

    if (cdfstatus != CDF_OK)
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