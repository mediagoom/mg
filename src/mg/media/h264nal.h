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

#include "mp4/h264.h"


__ALX_BEGIN_NAMESPACE

enum NALTYPE
{
	  UNSPECIFIED
	, NONIDR_SLICE
	, A_SLICE
	, B_SLICE
	, C_SLICE
	, IDR_SLICE
	, SEINAL
	, SEQUENCE
	, PICTURE
	, AU
	, END_SEQUENCE
	, END_STREAM
	, FILLER_NAL
	, 
};

class H264Nal
{
	CBuffer<unsigned char>     _last_decoded_nal;
	ResetBuffer<unsigned char> _reset;
	unsigned int               _rbsp_bytes;
	unsigned int               _nal_ref_idc;
	unsigned int               _nal_unit_type;

protected:
	void decode_nal(
		  unsigned int &nal_ref_idc
		, unsigned int &nal_unit_type
		, unsigned int &NumBytesInRBSP
		, unsigned int  NumBytesInNALunit
		, unsigned char * p_nal
		, IBitstream &bit_stream
		)
	{
		NumBytesInRBSP = 0;

		//copy nal_ref_idc and nal_unit_type to output
		p_nal[NumBytesInRBSP++] = bit_stream.nextbits(8);

		nal_ref_idc   = bit_stream.getbits(3);
		nal_unit_type = bit_stream.getbits(5);

		NumBytesInNALunit--;

		for(unsigned int idx = 0; idx < NumBytesInNALunit; idx++)
		{
			_ASSERTE((idx + 1) >= NumBytesInRBSP);

			if(idx + 1 == NumBytesInNALunit && 0x03 == bit_stream.nextbits(8))
				break;
			
			if( (idx + 2) < NumBytesInNALunit  && 0x000003 == bit_stream.nextbits(24))
			{
				p_nal[NumBytesInRBSP++] = 0x00;
				p_nal[NumBytesInRBSP++] = 0x00;

				unsigned int emulation_prevention = bit_stream.getbits(24);
				_ASSERTE(0x000003 == emulation_prevention);
				idx += 2;//3 byte read but must account for idx++ in the for at the beginning
			}
			else
			{
				p_nal[NumBytesInRBSP++] = bit_stream.getbits(8);
			}
		}
	}
public:

	H264Nal()
		: _last_decoded_nal(1024)
		, _reset(_last_decoded_nal)
	{
	}

	void decode_nal_in_place(
		  unsigned int &nal_ref_idc
		, unsigned int &nal_unit_type
		, unsigned int &NumBytesInRBSP
		, unsigned int  nal_size
		, unsigned char * p_nal
		)
	{
		FixedMemoryBitstream mem(p_nal, nal_size);
		
		decode_nal(
		  nal_ref_idc
		, nal_unit_type
		, NumBytesInRBSP
		, nal_size
		, p_nal
		, mem
		);
	}

	void decode_nal(
		  unsigned int  nal_size
		, const unsigned char * p_nal
		)
	{
		FixedMemoryBitstream mem(p_nal, nal_size);

		_reset.Reset();

		_last_decoded_nal.prepare(nal_size);
		
		decode_nal(
		  _nal_ref_idc
	    , _nal_unit_type
		, _rbsp_bytes
		, nal_size
		, _last_decoded_nal.get()
		, mem
		);

		_last_decoded_nal.updatePosition(_rbsp_bytes);
		_ASSERTE(_rbsp_bytes == _last_decoded_nal.size());
	}

	const unsigned char * decoded_rbsp()      const {return const_cast<CBuffer<unsigned char> *>(&_last_decoded_nal)->get();}
	      unsigned int    decoded_rbsp_size() const {return _rbsp_bytes;}
		  unsigned int    get_decoded_nal_unit_type() const {return _nal_unit_type;}
		  unsigned int    get_decoded_nal_ref_idc() const {return _nal_ref_idc;}

		  

};










__ALX_END_NAMESPACE

