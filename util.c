/*

    AllSkyCameraCal: util.c

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

#include "util.h"

#include <time.h>

#include <cdf.h>

double currentEpoch(void)
{
    double epoch = 0.0;
    double unixTime = (double) time(NULL);
    UnixTimetoEPOCH(&unixTime, &epoch, 1);
    return epoch;
}
