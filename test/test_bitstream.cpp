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

#include <iostream>
#include <mgmedia.h>

using namespace MGCORE;

#include "test_bitstream.h"
#include "test_base.h"

#include <bitset>



void ulout(unsigned long x)
{
	/*
	std::cout << std::hex 
		<< (x & 0x000000FF)
		<< "\t"
		<< (( x >> 8) & 0x000000FF)
		<< "\t"
		<< (( x >> 16) & 0x000000FF)
		<< "\t"
		<< (( x >> 24) & 0x000000FF)
		<< "\t"
		<< std::endl;
	*/
}

int compare_buffer(const unsigned char * src, const unsigned char * dst, uint32_t size)
{
	for (uint32_t k = 0; k < size; k++)
		if (src[k] != dst[k])
			return k;

	return 0;
}


int check_char4_bitstream(IBitstream & bs)
{
	uint32_t res = bs.getbits(4);

	CHECK(0x00, res, "INVALID FIRST 4 BIT");

	res = bs.getbits(4);

	CHECK(0x0f, res, "INVALID 4-8 BIT");

	res = bs.getbits(4);

	CHECK(0x00, res, "INVALID 8-12 BIT");

	res = bs.getbits(4);

	CHECK(0x0f, res, "INVALID 12-16 BIT");

	res = bs.getbits(7);

	CHECK(0x00, res, "INVALID 16-23 BIT");

	res = bs.getbits(1);

	CHECK(0x01, res, "INVALID 23-24 BIT");

	res = bs.getbits(6);

	CHECK(0x00, res, "INVALID 24-30 BIT");

	res = bs.getbits(2);

	CHECK(0x02, res, "INVALID 30-32 BIT");

	return 0;
}

void fill_bitset(unsigned char (&mem)[4][4])
{
	std::bitset<32> bitset;

	/*0x01*/ bitset[7] = 0; bitset[6] = 0; bitset[5] = 0; bitset[4] = 1;
	/*0x0A*/ bitset[3] = 1; bitset[2] = 0; bitset[1] = 1; bitset[0] = 0;


	bitset[15] = 1; bitset[14] = 1; bitset[13] = 1; bitset[12] = 1;
	bitset[11] = 1; bitset[10] = 1; bitset[9] = 1; bitset[8] = 1;


	bitset[23] = 0; bitset[22] = 0; bitset[21] = 0; bitset[20] = 0;
	bitset[19] = 1; bitset[18] = 0; bitset[17] = 0; bitset[16] = 0;


	bitset[31] = 1; bitset[30] = 0; bitset[29] = 0; bitset[28] = 0;
	bitset[27] = 0; bitset[26] = 0; bitset[25] = 0; bitset[24] = 0;


	//std::wcout << bitset.to_string().c_str() << std::endl;

	unsigned long x = bitset.to_ulong();

	//std::cout << x  << std::endl;

	ulout(x);



	mem[0][0] = x & 0x000000FF;
	mem[0][1] = ((x >> 8) & 0x000000FF);
	mem[0][2] = ((x >> 16) & 0x000000FF);
	mem[0][3] = ((x >> 24) & 0x000000FF);

	for (int i = 1; i < 4; i++)
		for (int h = 0; h < 4; h++)
			mem[i][h] = mem[0][h];
}

int check_bitset_bitream(IBitstream & bs)
{
	uint32_t k = bs.nextbits(32);

	ulout(k);

	for (int i = 0; i < 4; i++)
	{

		//	std::cout << "REPEAT--> " << i <<  std::endl;

		uint32_t res = bs.getbits(4);

		CHECK(0x0001, res, "INVALID FIRST 4 BIT");

		res = bs.getbits(1); CHECK(0x0001, res, "INVALID 5 BIT");
		res = bs.getbits(1); CHECK(0x0000, res, "INVALID 6 BIT");
		res = bs.getbits(2); CHECK(0x0002, res, "INVALID 7-8 BIT");

		res = bs.getbits(8); CHECK(0x00FF, res, "INVALID 8-16 BIT");

		res = bs.getbits(8); CHECK(0x0008, res, "INVALID 16-24 BIT");

		res = bs.getbits(8); CHECK((0x0001 << 7), res, "INVALID 24-32 BIT");

	}

	return 0;
}

int test_fixed_memory()
{
	unsigned char mem[] = { CHAR4 };

	CResource<fixed_memory> fm; fm.Create();
	fm->_buf = mem;
	fm->_buf_len = sizeof(mem);

	_ASSERTE(4 == fm->_buf_len);

	bitstream<fixed_memory> bs(fm);

	return check_char4_bitstream(bs);

}

#define TESTSIZE 16

int test_bitset()
{	

	unsigned char mem[4][4];

	//std::wcout << sizeof(mem) << std::endl;

	::memset(mem, 0, (sizeof(mem)));
	
	fill_bitset(mem);

	CResource<fixed_memory> fm; fm.Create();

	fm->_buf     = reinterpret_cast<unsigned char*>(mem);
	fm->_buf_len = sizeof(mem);



	bitstream<fixed_memory> bs(fm);

	return check_bitset_bitream(bs);

	
}

//$c = ""; $tot = 0; $end = 400 * 8; while ($tot - lt $end) { $r = Get - Random - minimum 1 - maximum 32; $c += "outs.putbits(ins.getbits($r), $r);`r`n"; $tot += $r }; $c += "int tot = $tot / 8";

int transfer_bits(IBitstream & ins, IBitstream & outs)
{
	outs.putbits(ins.getbits(6), 6);
	outs.putbits(ins.getbits(13), 13);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(13), 13);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(5), 5);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(14), 14);
	outs.putbits(ins.getbits(27), 27);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(8), 8);
	outs.putbits(ins.getbits(5), 5);
	outs.putbits(ins.getbits(30), 30);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(8), 8);
	outs.putbits(ins.getbits(29), 29);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(11), 11);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(8), 8);
	outs.putbits(ins.getbits(12), 12);
	outs.putbits(ins.getbits(29), 29);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(31), 31);
	outs.putbits(ins.getbits(29), 29);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(31), 31);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(16), 16);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(1), 1);
	outs.putbits(ins.getbits(4), 4);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(5), 5);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(13), 13);
	outs.putbits(ins.getbits(6), 6);
	outs.putbits(ins.getbits(29), 29);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(6), 6);
	outs.putbits(ins.getbits(14), 14);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(1), 1);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(6), 6);
	outs.putbits(ins.getbits(12), 12);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(27), 27);
	outs.putbits(ins.getbits(6), 6);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(1), 1);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(13), 13);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(27), 27);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(11), 11);
	outs.putbits(ins.getbits(16), 16);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(19), 19);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(5), 5);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(11), 11);
	outs.putbits(ins.getbits(11), 11);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(1), 1);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(30), 30);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(27), 27);
	outs.putbits(ins.getbits(4), 4);
	outs.putbits(ins.getbits(9), 9);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(12), 12);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(16), 16);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(4), 4);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(29), 29);
	outs.putbits(ins.getbits(12), 12);
	outs.putbits(ins.getbits(31), 31);
	outs.putbits(ins.getbits(25), 25);
	outs.putbits(ins.getbits(21), 21);
	outs.putbits(ins.getbits(11), 11);
	outs.putbits(ins.getbits(16), 16);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(13), 13);
	outs.putbits(ins.getbits(8), 8);
	outs.putbits(ins.getbits(4), 4);
	outs.putbits(ins.getbits(14), 14);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(10), 10);
	outs.putbits(ins.getbits(3), 3);
	outs.putbits(ins.getbits(4), 4);
	outs.putbits(ins.getbits(15), 15);
	outs.putbits(ins.getbits(31), 31);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(28), 28);
	outs.putbits(ins.getbits(1), 1);
	outs.putbits(ins.getbits(2), 2);
	outs.putbits(ins.getbits(22), 22);
	outs.putbits(ins.getbits(7), 7);
	outs.putbits(ins.getbits(26), 26);
	outs.putbits(ins.getbits(17), 17);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(24), 24);
	outs.putbits(ins.getbits(23), 23);
	outs.putbits(ins.getbits(18), 18);
	outs.putbits(ins.getbits(20), 20);
	outs.putbits(ins.getbits(18), 18);
	
	int tot = 3202 / 8;

	outs.flush();

	return tot;

}

