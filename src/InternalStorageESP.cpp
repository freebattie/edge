/*
  Copyright (c) 2019 Juraj Andrassy.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "InternalStorageESP.h"

InternalStorageESPClass::InternalStorageESPClass()
{
}
int InternalStorageESPClass::open(int length)
{
  return Update.begin(length, U_FLASH);
}

size_t InternalStorageESPClass::write(uint8_t b)
{
  return Update.write(&b, 1);
}

void InternalStorageESPClass::close()
{
  Update.end(false);
}

void InternalStorageESPClass::apply()
{
  ESP.restart();
}

long InternalStorageESPClass::maxSize()
{
  return ESP.getFlashChipSize() / 2; // Update.begin() in open() does the exact check
}

InternalStorageESPClass InternalStorage;
