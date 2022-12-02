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

#include "main.h"

#include <stdio.h>

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
    CDFstatus status = CDFcreateCDF(cdfFilename, &cdf);

    // TODO
    // Compression
    // Blocking factor

    // Timestamp
    long nDims = 0;
    long dimSizes[CDF_MAX_DIMS] = {0};
    long recVariance = VARY;
    long dimsVariance[2] = {VARY,VARY};
    long varNum = 0;
    long compressionParam[1] = {6};
    status = CDFcreatezVar(cdf, "Timestamp", CDF_EPOCH, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->imageTimes);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }

    nDims = 2;
    dimSizes[0] = 3;
    dimSizes[1] = 3;
    dimsVariance[0] = VARY;
    dimsVariance[1] = VARY;
    status = CDFcreatezVar(cdf, "PointingErrorDCM", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->pointingErrorDcms);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }

    nDims = 1;
    dimSizes[0] = 3;
    dimsVariance[0] = VARY;
    status = CDFcreatezVar(cdf, "RotationAxis", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->rotationVectors);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }

    nDims = 0;
    status = CDFcreatezVar(cdf, "RotationAngle", CDF_REAL4, 1, nDims, dimSizes, recVariance, dimsVariance, &varNum);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFsetzVarCompression(cdf, varNum, GZIP_COMPRESSION, compressionParam);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }
    status = CDFputzVarAllRecordsByVarID(cdf, varNum, state->nImages, state->rotationAngles);
    if (status != CDF_OK)
    {
        CDFgetStatusText(status, statusMessage);
        fprintf(stderr, "%s.\n", statusMessage);
        return ASCC_CDF_WRITE;
    }


    // Global attributes
    // Variable attributes

    CDFclose(cdf);

    return ASCC_OK;
}