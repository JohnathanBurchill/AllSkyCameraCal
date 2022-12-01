/*

    AllSkyCameraCal: import.h

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

#ifndef _IMPORT_H
#define _IMPORT_H

#include "main.h"
#include <cdf.h>

int loadThemisLevel2(ProgramState *state);

float getCdfFloat(CDFid cdf, char *site, char *varNameTemplate);
int getCdfFloatArray(CDFid cdf, char *site, char *varNameTemplate, long recordIndex, void **data);

int loadStars(ProgramState *state);
int readBSC5Int32(FILE *f, int32_t *value);
void reverseBytes(uint8_t *word, int nBytes);

int loadSkymap(ProgramState *state);
int loadSkymapFromFile(ProgramState *state);


#endif // _IMPORT_H
