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

#include "wincrc.h"
#include <map>

#define MAX_PES_SIZE 65535

class CTSW
{

protected:
	IBitstream3 *    _p_f;
	CTSW():_p_f(NULL){}

private:
	CTSW(CTS &rhs);
	int operator=(CTSW &rhs);

protected:

public:
	template<typename T> void put(T &t)
	{
		t.put(*_p_f);
	}

	void write(const BYTE* pByte, DWORD size)
	{
		/*
		unsigned long written(0);
		_p_f->Write(pByte, size, &written);
		_ASSERTE(written == size);
		*/
		_p_f->write(pByte, size);
	}

	virtual uint64_t get_position()
	{
		_ASSERTE(NULL != _p_f);
		return _p_f->get_position();
	}
		

	virtual void write_bytes(const BYTE* p_bytes, unsigned int size)
	{
		/*
		for(unsigned int i = 0; i < size; i++)
			_p_f->putbits(p_bytes[i], 8);
		*/
		
		this->write(p_bytes, size);
	}

	virtual void write_uint(unsigned int x)
	{
		_p_f->putbits(x, 32);
	}

	virtual void write_byte(unsigned char x)
	{
		_p_f->putbits(x, 8);
	}

	/*
	void pin_position()
	{
		_ASSERTE(_UI64_MAX == _pinned_position);
		_ASSERTE(_UI64_MAX == _last_position);
		_pinned_position = get_position();
	}
	void pop_pin()
	{
		_ASSERTE(_UI64_MAX != _pinned_position);
		_ASSERTE(_UI64_MAX == _last_position);
		_last_position = get_position();
		_p_f->set_position(_pinned_position);

		_pinned_position = _UI64_MAX;

	}
	void un_pin()
	{
		_ASSERTE(_UI64_MAX == _pinned_position);
		_ASSERTE(_UI64_MAX != _last_position);
		_p_f->set_position(_last_position);

		_last_position = _UI64_MAX;
	}
	*/
	void set_position(uint64_t position)
	{
		_p_f->set_position(position);
	}

	void flush()
	{
		_p_f->flush();
	}

	virtual void close(){}

	virtual ~CTSW()
	{}
};

class CPESWrite
{

protected:


	void init_pes(PesData & pes , uint32_t stream_id)
	{
		pes.stream_id = stream_id;
		
		pes.PES_scrambling_control = 0;
        pes.PES_priority = 0;
        pes.data_alignment_indicator = 0;
        pes.copyright = 0;
        pes.original_or_copy = 0;
		
		pes.ESCR_flag = 0;
        pes.ES_rate_flag = 0;
        pes.DSM_trick_mode_flag  = 0;
        pes.additional_copy_info_flag = 0;
		pes.PES_extension_flag = 0;
        pes.PES_CRC_flag = 0;
		//9 14 22

		pes.PES_header_data_length = 0;

		pes.PES_packet_length = 3;

	}

	void close_pes(PesData & pes, uint32_t body_size)
	{
		pes.PES_packet_length += pes.PES_header_data_length;
		pes.PES_packet_length += body_size;

		if(!body_size || MAX_PES_SIZE <= body_size)
			pes.PES_packet_length = 0; //no length packet

	}

	void set_pts_dts(
		  PesData & pes
		, uint64_t timePTS = UINT64_MAX
		, uint64_t timeDTS = UINT64_MAX
		, uint64_t timePCR = UINT64_MAX
		)
	{
		 pes.PTS_flags = 0; 
         pes.DTS_flags = 0;
		 pes.ESCR_flag = 0;

		if(timePTS < UINT64_MAX)
		{
			
            pes.PTS_flags = 1; 

            pes.PES_header_data_length += 5;

			MpegTime::ToMpegTime(timePTS,
				       pes.PTS_32_30,
					   pes.PTS_29_15,
					   pes.PTS_14_0);

		}

		if(timeDTS < UINT64_MAX)
		{
			
           
            pes.DTS_flags = 1;

            pes.PES_header_data_length += 5;

			MpegTime::ToMpegTime(timeDTS,
				        pes.DTS_32_30,
						pes.DTS_29_15,
						pes.DTS_14_0);

		}		

		if(timePCR < UINT64_MAX)
		{
			pes.ESCR_flag = 1;

			 pes.PES_header_data_length += 6;

			MpegTime::ToMpegTime(timePCR
				, pes.ESCR_base_32_30
				, pes.ESCR_base_29_15
				, pes.ESCR_base_14_0
				, pes.ESCR_extension
				);
		}
	}

public:

	void WritePesHeader(uint32_t stream_id
		,  uint64_t timePTS
		,  uint64_t timeDTS
		,  uint64_t timePCR
		, uint32_t body_size
		, CTSW &TSW
	)
	{
		PesData pes;
		
		init_pes(pes, stream_id);
        
		set_pts_dts(pes, timePTS, timeDTS, timePCR);

		close_pes(pes, body_size);

		TSW.put(pes);

		
		
	}

};

//typedef memory_write_media_bitstream<CTSW> CTSWriteMemory;
class CTSWriteMemory : public memory_write_media_bitstream<CTSW>
{
public:
	CTSWriteMemory()
	{
		this->open(1024);
	}

	virtual ~CTSWriteMemory()
	{}

};

typedef file_media_bitstream<CTSW, O_CREAT | O_WRONLY | O_TRUNC > CTSWriteFile;
typedef sync_file_media_bitstream<CTSW, false> SYNCTSWriteFile;

class CTSWrite
{

	Program_Association_Section _pas;
	Program_Map_Section			_pam;

	CTSWriteMemory				_memory_pas;
	CTSWriteMemory				_memory_pam;

	std::map<int, unsigned char> _continuity_counter;

	void init_adaptation(Adaptation_Field & af)
	{

		af.adaptation_field_length = 1;
		af.discontinuity_indicator = 0;

		af.random_access_indicator = 0;
		af.elementary_stream_priority_indicator = 0;
		af.PCR_flag  = 0;
		af.reserved  = 0;
		af.OPCR_flag = 0;
		af.splicing_point_flag = 0;
		af.transport_private_data_flag = 0;
		af.adaptation_field_extension_flag = 0;
	}

	void init_tp(Transport_Packet & packet)
	{
		packet.transport_error_indicator = 0;
		packet.payload_unit_start_indicator = 1;
		packet.transport_priority = 0;
		packet.transport_scrambling_control = 0;
		packet.adaptation_flag = 0;
		packet.payload_field   = 1;

		packet.continuity_counter = 0;
	}

protected:

	void init_base(BaseSection & base)
	{
		
		base.marker = 0;
		base.reserved = 0;
		base.id = 1;
		base.version_number = 0;
		base.current_next_indicator = 1;
		base.section_number = 0;
		base.last_section_number = 0;
		base.section_length = 9;
	}

	void init_pas()
	{
		_pas.table_id = 0;
		//_pas.program_number[0] = 1;
		
		init_base(_pas);

		_pas.CRC_32 = 0;

		_continuity_counter[0] = 0;
	}

	void add_pas(int idx = 0, int pam_id = 0x1000
		, int program_id = 1)
	{

		_ASSERTE(program_id > 0);
		_pas.program_map_PID[idx] = pam_id;
		_pas.program_number[idx]  = program_id;

		_continuity_counter[pam_id] = 0;
		
		_pas.section_length += 4;
	}

	void init_pam(int program_id = 1)
	{
		_pam.table_id = 0x02;

		init_base(_pam);

		_pam.id = program_id;
		_pam.PCR_PID = 0;//0x100;

		_pam.reserved3 = 0;
		_pam.reserved4 = 0;
		_pam.program_info_length = 0;
        _pam.reserved5 = 0;
        _pam.reserved6 = 0;
		_pam.ES_info_length = 0;
		_pam.program_info_length = 0;

		_pam.section_length += 4;

		_pam.CRC_32 = 0;
	}	
	

public:
	
	void set_pcr_pid(int pid)
	{
		_pam.PCR_PID = pid;
	}

	void add_pam(int pid, int type, int idx = 0)
	{
		_pam.stream_type[idx]    = type;
		_pam.elementary_PID[idx] = pid;
		_pam.ES_info_length      = 0;
		_pam.section_length     += 5;

		_continuity_counter[pid] = 0;
	}

	CTSWrite()
	{
		init_pas();
		add_pas();

		init_pam();

		//_memory_pas.open(1024);
		//_memory_pam.open(1024);
	}

	void output_ts(
		  unsigned int pid
		, const unsigned char * pbytes
		, int32_t size
		, CTSW &TSW
		, unsigned int random_access_indicator = 0
		, int64_t      timePCR = -1
		, unsigned int discontinuity_indicator = 0
		)
	{

		//can set pcr only on random_access
		_ASSERTE((-1 == timePCR) || (1 == random_access_indicator));

		Adaptation_Field af;
		
		/*
		af.adaptation_field_length = 1;
		af.discontinuity_indicator = 0;

		af.random_access_indicator = 0;
		af.elementary_stream_priority_indicator = 0;
		af.PCR_flag  = 0;
		af.reserved  = 0;
		af.OPCR_flag = 0;
		af.splicing_point_flag = 0;
		af.transport_private_data_flag = 0;
		af.adaptation_field_extension_flag = 0;
		*/
		init_adaptation(af);

		Transport_Packet packet; //24 bit;
		/*
		packet.transport_error_indicator = 0;
		packet.payload_unit_start_indicator = 1;
		packet.transport_priority = 0;
		packet.PID = pid;
		packet.transport_scrambling_control = 0;
		packet.adaptation_flag = 0;
		packet.payload_field   = 1;

		packet.continuity_counter = 0;
		*/
		init_tp(packet);
		packet.PID = pid;

		int32_t first_ts_packet = 184;

		bool first_adaptation = false;

		
		if(random_access_indicator) //we need at leat one adaptation
		{
			first_ts_packet -= 2;
			first_adaptation = true;

			if(-1 < timePCR)
			{
				af.PCR_flag  = 1;

				MpegTime::ToMpegTime(timePCR,
				       af.program_clock_ref_1,
					   af.program_clock_ref_2,
					   af.program_clock_ref_3,
					   af.program_clock_reference_extension);

				first_ts_packet -= 6;

				af.adaptation_field_length += 6;
			}
		}


		int body_size = 184;
		int written   = 0;

		if(size < first_ts_packet)
		{
			int diff = first_ts_packet - size;

			first_adaptation = true;

			if(1 == diff)
			{
				af.adaptation_field_length = 0;
				
			}
			else
			{	
				if(!random_access_indicator)
				{
					af.adaptation_field_length += (first_ts_packet - 2 - size); 
				}
				else
					af.adaptation_field_length += (diff); //2 alredy removed before

			}
		}

		if(first_adaptation)
		{
			af.random_access_indicator = random_access_indicator;
			packet.adaptation_field = &af;
			packet.adaptation_flag  = 1;

			body_size -= 1;
			body_size -= af.adaptation_field_length;

			memcpy(packet.payload_after_adaptation_byte, pbytes, body_size);
		}
		else
			memcpy(packet.payload_byte, pbytes, body_size);

		packet.continuity_counter = _continuity_counter[pid]++;




		//output first packet
		TSW.put(packet);
		//TSW.write(pbytes, body_size);

		written += body_size;
		pbytes  += body_size;
	
		body_size = 184;

		af.random_access_indicator = 0;
		af.PCR_flag                = 0;
		af.adaptation_field_length = 1;
		packet.adaptation_field = NULL;
		packet.adaptation_flag = 0;
		packet.payload_unit_start_indicator = 0;
		
				 
		int full_ts_packets = (size - first_ts_packet) / (TRANSPORT_PACKET_SIZE - 4); 
		int last_ts_packet  = (size - first_ts_packet) % (TRANSPORT_PACKET_SIZE - 4); 

		for(int idx = 0; idx < full_ts_packets; idx++)
		{
			packet.continuity_counter = _continuity_counter[pid]++;

			memcpy(packet.payload_byte, pbytes, body_size);

			TSW.put(packet);
			//TSW.write(pbytes, body_size);

			written += body_size;
			pbytes  += body_size;
		}

		if(last_ts_packet > 0)
		{
			af.adaptation_field_length = 183 - last_ts_packet;
			first_adaptation = true;

			packet.adaptation_field = &af;
			packet.continuity_counter = _continuity_counter[pid]++;
			packet.adaptation_flag = 1;

			memcpy(packet.payload_after_adaptation_byte, pbytes, last_ts_packet);

				//output first packet
				TSW.put(packet);
				//TSW.write(pbytes, last_ts_packet);

				written += last_ts_packet;

			packet.adaptation_field = NULL;
		}


	}

	void output_sections_tables(CTSW &TSW)
	{
		if(!_memory_pas.get_size())
		{
			Pointer_Field f;
			              f.pointer_field = 0;

		    CWinCrc CRC;
		    
			_memory_pas.put(f);
			_memory_pas.put(_pas);
			_memory_pas.flush();

			//+1 step over the pointer field
			//the section lenght must add the first 3 byte not included and exclude the crc ending field
			unsigned int crc = CRC.crc_32(_memory_pas.get_buffer() + 1, _pas.section_length + 3 - 4);

			_memory_pas.set_position(_memory_pas.get_position() - 4);
			_memory_pas.write_uint(crc);

			
			_memory_pam.put(f);
			_memory_pam.put(_pam);
			_memory_pam.flush();

			crc = CRC.crc_32(_memory_pam.get_buffer() + 1, _pam.section_length + 3 - 4);

			_memory_pam.set_position(_memory_pam.get_position() - 4);
			_memory_pam.write_uint(crc);

			_memory_pas.flush();
			_memory_pam.flush();


		}

		output_ts(0, _memory_pas.get_buffer()
			, static_cast<uint32_t>(_memory_pas.get_size())
			, TSW);
		//TODO: HANDLE MULTIPLE PROGRAM
		output_ts(_pas.program_map_PID[0], _memory_pam.get_buffer()
			, static_cast<uint32_t>(_memory_pam.get_size())
			, TSW);
	};

	void set_sections_tables_continuity(unsigned char continuity)
	{
		_continuity_counter[0] = continuity;
		//TODO: HANDLE MULTIPLE PROGRAM
		_continuity_counter[_pas.program_map_PID[0]] = continuity;
	}




	void fill_pid(unsigned int pid, CTSW &TSW)
	{
		Adaptation_Field af;
		init_adaptation(af);

		Transport_Packet packet; //24 bit;
		init_tp(packet);

		init_tp(packet);
		packet.PID = pid;

		af.adaptation_field_length = 183;

		packet.adaptation_field = &af;
		
		packet.adaptation_flag = 1;

		packet.payload_unit_start_indicator = 0;

		while(0 != (0x0F & _continuity_counter[pid]))
		{
			packet.continuity_counter = _continuity_counter[pid];
			//output packet
			TSW.put(packet);
			_continuity_counter[pid]++;
		}

		packet.adaptation_field = NULL;
	}
	
};


/*

class CTSWriteMemory: public CTSW
{
MemoryBitstream *_p_m;

int operator=(CTSWriteMemory &rhs);
public:
CTSWriteMemory():
_p_m(NULL)
{}

void Close()
{
if(_p_m)
delete _p_m;
_p_m = NULL;
}

virtual ~CTSWriteMemory()
{Close();}

void Open(const BYTE * p, ULONG size)
{
_ASSERTE(NULL == _p_f);
_ASSERTE(NULL == _p_m);
_p_m = new MemoryBitstream(p, size, 1024, false);
_p_f = _p_m;
}

void Open(MemoryBitstream * p)
{
_ASSERTE(NULL == _p_f);
_ASSERTE(NULL == _p_m);

_p_f = p;
}

void Open(ULONG size = 1024)
{
_p_m = new MemoryBitstream(size, 1024);
_p_f = _p_m;
}

uint64_t get_size() const {return _p_m->get_size();}
const BYTE * get_buffer() const {return _p_m->get_buffer();}

void flush(){_p_m->flushbits();}
};

class CTSWriteFile: public CTSW
{

FileBitstream * _p_ff;
public:
	CTSWriteFile():
	  _p_ff(NULL)
	{}

    virtual void close()
	{
		if(_p_ff)
			delete _p_ff;
		_p_ff = NULL;
	}

	virtual ~CTSWriteFile(){close();}


	void Open(const TCHAR *pFileName, int buflen)
	{
		_ASSERTE(NULL == _p_f);
		_ASSERTE(NULL == _p_ff);

		_p_ff = new FileBitstream(pFileName
			, BS_OUTPUT
			, buflen
			);

		_p_f = _p_ff;
	}
	
	void Open(const TCHAR *pFileName)
	{
		Open(pFileName, 1024);
	}
};
*/
