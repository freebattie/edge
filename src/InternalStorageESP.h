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

#ifndef _INTERNAL_STORAGE_ESP_H_INCLUDED
#define _INTERNAL_STORAGE_ESP_H_INCLUDED

// #include "OTAStorage.h"
#include <Arduino.h>
#include <Update.h>
class InternalStorageESPClass
{
public:
  InternalStorageESPClass();
  int open(int length);
  size_t write(uint8_t);
  void close();
  void apply();
  long maxSize();

private:
};

extern InternalStorageESPClass InternalStorage;

#endif
