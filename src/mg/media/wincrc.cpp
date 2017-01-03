#include "stdafx.h"
#include <stddef.h>
#include "wincrc.h"

#include "stdint.h"


struct CRC_TABLE_PARAM{
    unsigned char  le;
    unsigned char  bits;
    unsigned int poly;
} ;

CRC_TABLE_PARAM av_crc_table_params[AV_CRC_MAX]; 


static AVCRC av_crc_table[AV_CRC_MAX][CRC_TABLE_SIZE];

int av_crc_init(AVCRC *ctx, int le, int bits, unsigned int poly, int ctx_size)
{
    unsigned int i, j;
    unsigned int c;

    if (bits < 8 || bits > 32 || poly >= (1LL << bits))
        return -1;
    if (ctx_size != sizeof(AVCRC) * 257 && ctx_size != sizeof(AVCRC) * 1024)
        return -1;

    for (i = 0; i < 256; i++) {
        if (le) {
            for (c = i, j = 0; j < 8; j++)
                c = (c >> 1) ^ (poly & (-(c & 1)));
            ctx[i] = c;
        } else {
            for (c = i << 24, j = 0; j < 8; j++)
                c = (c << 1) ^ ((poly << (32 - bits)) & (((int) c) >> 31));
            ctx[i] = AV_BSWAP32C(c);
        }
    }
    ctx[256] = 1;

    if (ctx_size >= sizeof(AVCRC) * 1024)
        for (i = 0; i < 256; i++)
            for (j = 0; j < 3; j++)
                ctx[256 *(j + 1) + i] =
                    (ctx[256 * j + i] >> 8) ^ ctx[ctx[256 * j + i] & 0xFF];


    return 0;
}

const AVCRC *av_crc_get_table(AVCRCId crc_id)
{
    if (!av_crc_table[crc_id][FF_ARRAY_ELEMS(av_crc_table[crc_id]) - 1])
        if (av_crc_init(av_crc_table[crc_id],
                        av_crc_table_params[crc_id].le,
                        av_crc_table_params[crc_id].bits,
                        av_crc_table_params[crc_id].poly,
                        sizeof(av_crc_table[crc_id])) < 0)
            return NULL;

	return av_crc_table[crc_id];

}

unsigned int av_crc(const AVCRC *ctx, unsigned int crc,
                const unsigned char *buffer, size_t length)
{
    const unsigned char *end = buffer + length;


    if (!ctx[256]) {
        while (((intptr_t) buffer & 3) && buffer < end)
            crc = ctx[((unsigned char) crc) ^ *buffer++] ^ (crc >> 8);

        while (buffer < end - 3) {
            crc ^= (*(const unsigned int *) buffer); buffer += 4;
            crc = ctx[3 * 256 + ( crc        & 0xFF)] ^
                  ctx[2 * 256 + ((crc >> 8 ) & 0xFF)] ^
                  ctx[1 * 256 + ((crc >> 16) & 0xFF)] ^
                  ctx[0 * 256 + ((crc >> 24)       )];
        }
    }

    while (buffer < end)
        crc = ctx[((unsigned char) crc) ^ *buffer++] ^ (crc >> 8);

    return crc;
}


CWinCrc::CWinCrc(void)
{	
	CRC_TABLE_PARAM tp0 = { 0,  8,       0x07 };
	CRC_TABLE_PARAM tp1 = { 0, 16,     0x8005 };
	CRC_TABLE_PARAM tp2 = { 0, 16,     0x1021 };
	CRC_TABLE_PARAM tp3 = { 0, 24,   0x864CFB };
	CRC_TABLE_PARAM tp4 = { 0, 32, 0x04C11DB7 };
	CRC_TABLE_PARAM tp5 = { 1, 32, 0xEDB88320 };

	av_crc_table_params[AV_CRC_8_ATM]      = tp0;

	av_crc_table_params[AV_CRC_16_ANSI]    = tp1;
    av_crc_table_params[AV_CRC_16_CCITT]       = tp2;
    av_crc_table_params[AV_CRC_24_IEEE]        = tp3;
    av_crc_table_params[AV_CRC_32_IEEE]        = tp4;
    av_crc_table_params[AV_CRC_32_IEEE_LE]     = tp5;

}

CWinCrc::~CWinCrc(void)
{
}

unsigned int CWinCrc::crc_32(const unsigned char *buffer, unsigned int length, unsigned int crc)
{
		return AV_BSWAP32C(av_crc(av_crc_get_table(AV_CRC_32_IEEE), crc, buffer, length));
}

