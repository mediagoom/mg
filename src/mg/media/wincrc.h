/*****************************************************************************
* Copyright (C) 2016 mg project
*
* Authors: MediaGoom <admin@mediagoom.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE or NONINFRINGEMENT.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*
* For more information, contact us at info@mediagoom.com.
*****************************************************************************/
#pragma once

typedef unsigned int AVCRC;

#define CRC_TABLE_SIZE 1024

#define AV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define AV_BSWAP32C(x) (  ( (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))  << 16 ) | (((x >> 16) << 8 & 0xff00)  | ((x >> 16) >> 8 & 0x00ff))  )
//#define AV_BSWAP32C(x) ( (AV_BSWAP16C(x) << 16) | AV_BSWAP16C((x) >> 16)
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    AV_CRC_8_ATM,
    AV_CRC_16_ANSI,
    AV_CRC_16_CCITT,
    AV_CRC_32_IEEE,
    AV_CRC_32_IEEE_LE,  /*< reversed bitorder version of AV_CRC_32_IEEE */
    AV_CRC_24_IEEE = 12,
    AV_CRC_MAX,         
}AVCRCId;


class CWinCrc
{
	
public:
	CWinCrc(void);
	~CWinCrc(void);


	unsigned int crc_32(const unsigned char *buffer, unsigned int length, unsigned int crc = -1);
	


};
