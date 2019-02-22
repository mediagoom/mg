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

#include "mpeg.h"
#include "mp4/mpeg2ts.h"
#include "mp4/mpeg2au.h"
//#include "TBitstream.h"

#include "wincrc.h"

//#include "intsafe.h"

//#include <crtdbg.h>

#include <set>
#include <map>

using namespace ALX;


#define STREAM_TYPE_VIDEO_MPEG2 0x02	///ISO/IEC 13818 Video
	//0x03	ISO/IEC 11172 Audio
#define STREAM_TYPE_AUDIO_MPEG2 0x04	//ISO/IEC 13818 Audio
	//0x05	ISO/IEC 13818 private_sections
	//0x06	ISO/IEC 13818 PES packets containing private data
	//0x07	ISO/IEC 13522 MHEG
	//0x08	ISO/IEC 13818 DSM CC
	//0x09	ISO/IEC 13818 Private data
#define STREAM_TYPE_AUDIO_AAC       0x0f
	//#define STREAM_TYPE_AUDIO_AAC_LATM  0x11
	//#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b


#define ARRAY_SIZE 256

class tspacket
{
    CBufferRead  _payload;
    unsigned int _pid;
	bool         _gotfirst;
	
	Adaptation_Field _Adaptation;
	Adaptation_Field _FirstAdaptation;

	uint64_t _begin_payload_packet_count;
	uint64_t _last_packet_count;

	uint64_t _packet_count;

	uint64_t _adaptation_packet;

	bool _HasAdaptation;
	bool _HasFirstAdaptation;

	void CleanUp()
	{
		//if(NULL != _pAdaptation)
		//	delete _pAdaptation;

		//_pAdaptation = NULL;
	}
protected:
	tspacket():
		   _gotfirst(false)
		 , _begin_payload_packet_count(0)
		 , _last_packet_count(0)
		 , _HasAdaptation(false)
		 , _HasFirstAdaptation(false)
		 , _packet_count(0)
		 , _adaptation_packet(0)		 
	{}
	virtual ~tspacket(){CleanUp();}

	//virtual void incomplete(){}
	virtual void BeforeUnitStart(CBufferRead &payload){}
	virtual void process(Transport_Packet &ts, CBufferRead &payload) = 0;

	virtual void addPayload(const BYTE* source, size_t size)
	{
		if(!size)
			return;

		_payload.add(source, size);
		_gotfirst = true;
	}

	virtual void set_first_adaptation()
	{
		_HasFirstAdaptation = true;
		_FirstAdaptation.random_access_indicator = 1;
	}
	
	
public:

	virtual void Reset()
	{
		_gotfirst = false;
		_begin_payload_packet_count = 0;
		_last_packet_count = 0;
		_HasFirstAdaptation = false;
		CleanUp();
		_payload.Reset();
	}
	
	virtual void add(Transport_Packet &ts)
	{
		_packet_count++;
		
		if(ts.transport_error_indicator)
			return;

		if(_gotfirst && ts.PID != _pid)
		{
			_ASSERTE(ts.PID == _pid);
			throw;
		}

		_pid = ts.PID;
        _last_packet_count = ts._packet_count;

		if(ts.payload_unit_start_indicator)
		{
			if(_gotfirst)
				BeforeUnitStart(_payload);
			//else
			//	incomplete();

			CleanUp();
			_payload.Reset();
			_begin_payload_packet_count = _last_packet_count;
		}
		
		
	    if(ts.payload_field)
		{
			_HasAdaptation = (ts.adaptation_flag)?true:false;
			if(ts.adaptation_flag)
			{
				_Adaptation = *ts.adaptation_field;

				unsigned int size = 183 - ts.adaptation_field->adaptation_field_length;
				
				if(size)
				{
					addPayload(ts.payload_after_adaptation_byte, size);

					if(!_HasFirstAdaptation)
					{
						_adaptation_packet = _packet_count;
						_FirstAdaptation   = _Adaptation;
						_HasFirstAdaptation = true;
					}
				}
				
			}
			else
			{
				addPayload(ts.payload_byte, 184);
			}
		}

		

		process(ts, _payload);
	}

	unsigned int PID(){return _pid;}
	bool HasPID(){return _gotfirst;}
	bool HasAdaptation(){return _HasAdaptation;}

	bool HasPCR(){return HasAdaptation() && _Adaptation.PCR_flag;}
	uint64_t PCR()
	{
		_ASSERTE(HasAdaptation());
		_ASSERTE(_Adaptation.PCR_flag);

		return MpegTime::Time(_Adaptation.program_clock_ref_1,
            _Adaptation.program_clock_ref_2,
            _Adaptation.program_clock_ref_3,
			_Adaptation.program_clock_reference_extension);
	}

	void End()
	{
		BeforeUnitStart(_payload);
	}

	uint64_t get_begin_payload_packet_count(){return _begin_payload_packet_count;}
	uint64_t get_last_packet_count(){return _last_packet_count;}

	Adaptation_Field * adaptation(){return &_Adaptation;}

	Adaptation_Field * first_adaptation(){return &_FirstAdaptation;}

	bool HasFirstAdaptation(){return _HasFirstAdaptation;}
    bool HasFirstPCR(){return HasFirstAdaptation() && _FirstAdaptation.PCR_flag;}
	uint64_t FirstPCR()
	{
		_ASSERTE(HasFirstAdaptation());
		_ASSERTE(_FirstAdaptation.PCR_flag);

		return MpegTime::Time(_FirstAdaptation.program_clock_ref_1,
            _FirstAdaptation.program_clock_ref_2,
            _FirstAdaptation.program_clock_ref_3,
			_FirstAdaptation.program_clock_reference_extension);
	}

	size_t payload_current_size()
	{
		return _payload.size();
	}
};

template<unsigned int TABLE_ID = 0xFFFFFFFF>
class psi: public tspacket
{
	CWinCrc _crc;

	unsigned int _CRC;

protected:
	unsigned int CRC(){return _CRC;}
	bool _operational;
	psi():_operational(false){}
	virtual void readTable(IBitstream &payload, unsigned int table_id)
	{
		if(TABLE_ID == table_id)
			return readTable(payload);
		
		_ASSERTE(false);
	}
	virtual void readTable(IBitstream &payload){};
	virtual void process(Transport_Packet &ts, CBufferRead &payload)
	{   
		BaseSection s;

		payload.SetPosition(0);
		{
		
			fixed_memory_bitstream ms(payload.get()
				, payload.size()
			);

		Pointer_Field f;
		f.get(ms);
		s.get(ms);
		}
 
		
		if(TABLE_ID == s.table_id || 0xFFFFFFFF == TABLE_ID)
		{
			if(s.section_length > payload.size())
				return;
			//OK we have the full table

			
			//+1 step over the pointer field
			//the section length must add the first 3 byte not included and exclude the crc ending field
			_CRC = _crc.crc_32(payload.get() + 1, s.section_length + 3 - 4);

			payload.SetPosition(0);

			{		

				fixed_memory_bitstream ms(payload.get()
					, payload.size()
				);

			Pointer_Field f;
		    f.get(ms);



			readTable(ms, s.table_id);
			}

			//TODO:Check we have other tables following into the payload
		}

	}
public:
	bool CanWork(){return _operational;}
};


class ProgramAssociationTable:public psi<0>
{
	Program_Association_Section _pas;
protected:
	virtual void readTable(IBitstream &payload)
	{
		_pas.get(payload);

		_ASSERTE(_pas.CRC_32 == CRC());

		//should check the numbers of sections
		_operational = true;
	}
public:
	ProgramAssociationTable()
	{
	}

	unsigned int Count(){return _pas.i;}
	unsigned int GetProgramNumber(unsigned int idx){return _pas.program_number[idx];}
	unsigned int GetProgramPid   (unsigned int idx){return _pas.program_map_PID[idx];}

	bool HasNetworkPID(unsigned int & pid)
	{
		for(uint32_t i = 0; i < _pas.i; i++)
		{
			if(_pas.program_number[i] == 0)
			{
				pid = _pas.network_PID;
				return true;
			}
		}
	}

	bool IsProgramPid(unsigned int pid)
	{
		for(uint32_t i = 0; i < _pas.i; i++)
		{
			if(_pas.program_map_PID[i] == pid)
			{
				return true;
			}
		}

		return false;
	}


	bool IsNetworkPID(unsigned int pid)
	{
		unsigned int pid2(0);

		if(HasNetworkPID(pid2))
		{
			return pid2 == pid;
		}

		return false;
	}

	bool IsProgramMapPID(unsigned int pid, unsigned int & programid)
	{
		for(uint32_t i = 0; i < _pas.i; i++)
		{
			if(_pas.program_map_PID[i] == pid)
			{
				programid = _pas.program_number[i];
				return true;
			}
		}

		return false;
	}

	bool IsProgramMapPID(unsigned int pid)
	{
		unsigned int programid(0);

		return IsProgramMapPID(pid, programid);
	}
};


class ProgramMapTable:public psi<2>
{
	Program_Map_Section _pms;
protected:
	virtual void readTable(IBitstream &payload)
	{
		_pms.get(payload);

		_ASSERTE(_pms.CRC_32 == CRC());

		//should check the numbers of sections
		_operational = true;
	}
public:
	ProgramMapTable()
	{
	}
    //<summary>
	//0x00	ISO/IEC Reserved
	//0x01	ISO/IEC 11172 Video
	//0x02	ISO/IEC 13818 Video
	//0x03	ISO/IEC 11172 Audio
	//0x04	ISO/IEC 13818 Audio
	//0x05	ISO/IEC 13818 private_sections
	//0x06	ISO/IEC 13818 PES packets containing private data
	//0x07	ISO/IEC 13522 MHEG
	//0x08	ISO/IEC 13818 DSM CC
	//0x09	ISO/IEC 13818 Private data
	//#define STREAM_TYPE_AUDIO_AAC       0x0f
	//#define STREAM_TYPE_AUDIO_AAC_LATM  0x11
	//#define STREAM_TYPE_VIDEO_MPEG4     0x10
	//#define STREAM_TYPE_VIDEO_H264      0x1b
	//0x0A-0x7F	ISO/IEC 13818 Reserved
	//0x80-0xFF	User Private
    //</summary>
	bool GetElementaryType(unsigned int PID, unsigned int &stream_type)
	{
		for(uint32_t i = 0; i < _pms.stream_count; ++i)
		{
			if(_pms.elementary_PID[i] == PID)
			{
				stream_type = _pms.stream_type[i];
				return true;
			}
		}

		return false;
	}

	bool IsElementary(uint32_t PID)
	{
		unsigned int stream_type(0);
		if(!GetElementaryType(PID, stream_type))
			return false;

		if(IsVideoType(stream_type))
			return true;

		if(IsAudioType(stream_type))
			return true;

		return false;
	}

	static bool IsVideoType(unsigned int stream_type)
	{
		if(0x01 == stream_type ||
			0x02 == stream_type)
			return true;

		return false;
	}

	static bool IsAudioType(unsigned int stream_type)
	{
		if(0x03 == stream_type ||
			0x04 == stream_type)
			return true;

		return false;
	}

	bool IsVideo(unsigned int PID)
	{
		unsigned int stream_type(0);

		if(!GetElementaryType(PID, stream_type))
			return false;

		return IsVideoType(stream_type);
	}

	bool IsAudio(unsigned int PID)
	{
		unsigned int stream_type(0);

		if(!GetElementaryType(PID, stream_type))
			return false;

		return IsAudioType(stream_type);
	}

	bool GetFirstAudio(unsigned int & PID)
	{
		for(uint32_t i = 0; i < _pms.stream_count; ++i)
		{
			if(IsAudioType(_pms.stream_type[i]))
			{
				PID = _pms.elementary_PID[i];
				return true;
			}
		}

		return false;
	}

	bool GetFirstVideo(unsigned int & PID)
	{
		for(uint32_t i = 0; i < _pms.stream_count; ++i)
		{
			if(IsVideoType(_pms.stream_type[i]))
			{
				PID = _pms.elementary_PID[i];
				return true;
			}
		}

		return false;
	}

	unsigned int Count(){return _pms.stream_count;}
	unsigned int GetPid(int idx){return _pms.elementary_PID[idx];}
	unsigned int GetType(int idx){return _pms.stream_type[idx];}

	unsigned int GetPCRPid(){return _pms.PCR_PID;}
};
/** 
* \brief This class parse an ts Program Elementary Stream.
* This class parse an ts Program Elementary Stream.
*/
class ts_pes:public tspacket
{
	PesData      _data;
	bool         _hasData;
	unsigned int _streamid;

protected:

	virtual void incomplete_pes(){_ASSERTE(false);};
	
	virtual void PayloadParseError(CBufferRead &payload, CMediaParserErr &ex)
	{_ASSERTE(false);}

	virtual void EmptyPayload(PesData & pesdata, CBufferRead &payload)
	{_ASSERTE(false);}
	///here you get just the payload
	virtual void ReceivePesPayload(const BYTE* p_data
		             , size_t length)
	{}
	///this is the function that receive the full PES body + header
	virtual void Receive(CBufferRead &payload)
	{
		_ASSERTE(_data.PES_extension_flag == 0 || 0 == _data.pack_header_field_flag);
       
		//only payload
		int y = 9 + _data.PES_header_data_length;
		
				
		ReceivePesPayload(payload.get() + y, payload.size() - y);
	}


	virtual void BeforeUnitStart(CBufferRead &payload)
	{	
		
		bool data = _hasData;
		
		if(!data)
		{
			payload.SetPosition(0);
			
			//TBitstream<CBufferRead*> ms;
			//ms.Create(&payload, BS_INPUT);

			fixed_memory_bitstream ms(payload.get()
				, payload.size()
			);

			unsigned int skipped(0);

			try{
					
				skipped = ms.nextcode(0x000001, 24, 8);

				_data.get(ms);
					
				//reposition payload to the beginning of the PES packet after PES header
				payload.SetPosition(ms.get_position());

			}catch (EndOfData e) 
			{return;}

			if(0 != _data.PES_packet_length)
			{
				incomplete_pes();
				_hasData = false;
			}
			else
			{
				_hasData = true;
			}
		}

		if(0 != _data.PES_packet_length)
			return;

        Receive(payload);

		//_hasData = false; //we will process the next one
	}
	virtual void process(Transport_Packet &ts, CBufferRead &payload)
	{

		if(6 > payload.size()) //size of PesBase
			return;

		unsigned int skipped(0);

		PesBase pb;

		_ASSERTE(6 /*size of(PesBase)*/ <= payload.size());

		payload.SetPosition(0);

		{
			//TBitstream<CBufferRead*> ms;
			//ms.Create(&payload, BS_INPUT);

			fixed_memory_bitstream ms(payload.get()
				, payload.size()
			);

			try{
					skipped = ms.nextcode(0x000001, 24, 8);
			}catch (EndOfData e) {return;}
		

			if((skipped / 8) >= payload.size())
			{
				return;
			}

			pb.get(ms);

		}

		if(0 == pb.PES_packet_length)
			return;

		if((pb.PES_packet_length + 6) > (payload.size() - (skipped/8)))
			return;

		//OK we should have the full PES
		payload.SetPosition(0);

		try{
				{
					fixed_memory_bitstream ms(payload.get()
						, payload.size()
					);

					if(skipped > 0)
						ms.skipbits(skipped);

					_data.get(ms);
					
					//reposition payload to the beginning of the pes packet after pes header

					payload.SetPosition(ms.getpos()/8);

				}

				//_data.PES_header_data_length

				_hasData = true;

				//TODO: HANDLE EXTENSIONS BYTE IF WANT TO COPY
				_ASSERTE(0 == _data.PES_extension_flag);
				
				if(0 == _streamid)
					_streamid = pb.stream_id;
				else
				{
					if(_streamid != pb.stream_id)
						ALXTHROW_T(_T("INVALID STREAM ID"));
				}

				//if(_data.PES_packet_length != 0 /*&& _hasReceiver*/)
				//{					
					//_pStreamData->ReceiveElementaryStreamData(*this, _data, payload);
					Receive(payload);
				//}
				

		}
		catch(CMediaParserErr &ex)
		{PayloadParseError(payload, ex);}


	}
public:
	ts_pes():
        _hasData(false)
	  , _streamid(0)

	{}

	virtual void Reset()
	{
		tspacket::Reset();
		_hasData = false;
	}

	      bool      HasData() const {return _hasData;}
	const PesData & Data()    const {return _data;}
	      bool      HasPTS()  const {return (_data.PTS_flags)?true:false;}
	      bool      HasDTS()  const {return (_data.DTS_flags)?true:false;}
	unsigned int    StreamID()const {return _streamid;}

	uint64_t DTS() const
	{
		//_ASSERTE(_hasData);
		_ASSERTE(_data.DTS_flags);

		return MpegTime::Time(_data.DTS_32_30, _data.DTS_29_15, _data.DTS_14_0);
		
	}

	uint64_t PTS() const
	{
		//_ASSERTE(_hasData);
		_ASSERTE(_data.PTS_flags);

		return MpegTime::Time(_data.PTS_32_30, _data.PTS_29_15, _data.PTS_14_0);
		
	}
};

/*
class empty_ts_pes: public ts_pes
{
protected:
	virtual void ReceivePesPayload(const BYTE* p_data
		             , unsigned int length)
	{
		this->Reset();
	}
};
*/

class IProgramElementaryStreamData
{
public:
	virtual bool ReceiveElementaryStreamData(tspacket &Packet, const PesData &pesdata, CBufferRead &payload, uint64_t offset)
	{return false;}
};

/**
This class perform the analisys of the paiload of the pes inside a pes.
The payload is delivered to tye IProgramElementaryStreamData pointer.
*/
template<typename T=IProgramElementaryStreamData*>
class TSProgramElementaryStream:public ts_pes
{
	T       _pStreamData;
	bool    _hasReceiver;

protected:

	uint64_t _offset;
	bool _receiver_written;

	virtual void Receive(CBufferRead &payload)
	{
		if(HasReceiver() && HasData())
		    _receiver_written = _pStreamData->ReceiveElementaryStreamData(*this, Data(), payload, _offset);

		if(_receiver_written)
			_offset = 0;
	}

public:
	TSProgramElementaryStream():
	   _hasReceiver(false)
	  , _offset(0)
	  , _receiver_written(false)
	{}
    void SetReceiver(T pStreamData)
	{
		_pStreamData   = pStreamData;
		_hasReceiver = true;
	}

	bool HasReceiver(){return _hasReceiver;}
	
};

/**
This class perform the analysis of the payload of the pes inside a pes.
The payload is delivered to the IProgramElementaryStreamData pointer.
It identify the starting point and get the correct structure describing the stream.
*/
template<typename T, typename I=IProgramElementaryStreamData*>
class TSProgramElementaryStreamAnalyze: public TSProgramElementaryStream<I>
{
	T    _stru;
    bool _IsDirty;
	bool _HasStruct;

protected:
	virtual void incomplete_pes()
	{
		_ASSERTE(false);
		_ASSERTE(!_IsDirty);
		_HasStruct = false;
	};
	virtual uint64_t getMinAnalyzeSize(){return 100;}
	virtual uint64_t getMinAnalyzePayloadSize(){return 25;}
	/**
	Identify the starting point of the stream
	*/
	virtual bool IsHim(IBitstream &ms)
	{
		return (0x000001b3 == ms.nextbits(32));
	}
	/**
	Position to the starting point of a stream
	*/
	virtual bool LookUp(CBufferRead &payload, IBitstream &ms)
	{
		//payload.ByteToRead() + ms.getUnreadBufferSize())

		while(getMinAnalyzePayloadSize <= payload.size() - ms.get_position())
		{
			if(IsHim(ms))
				return true;

			ms.skipbits(8);
		}
		//ms.nextcode(0x000001b3, 32, 8);

		return false;
	}

	/**
	Read the structure
	*/
	virtual void structure_get(IBitstream &ms)
	{
		_stru.get(ms);
	}

	/**
	Position and get the structure
	*/
	virtual bool UpdateLookUp(CBufferRead &payload)
	{
		//TBitstream<CBufferRead*> ms(&payload);

		fixed_memory_bitstream ms(payload.get()
			, payload.size()
		);

		if(LookUp(payload, ms))
		{
		   TSProgramElementaryStream<I>::_offset = ms.get_position();//payload.GetPosition() - ms.getUnreadBufferSize();
		   structure_get(ms);
		   _HasStruct = true;

		   return true;
		}

		return false;

	}
	virtual void Receive(CBufferRead &payload)
	{
		if(_HasStruct)//ONLY THROW AWAY STUFF BEFORE THE FIRST VALID HEADER 
		{	
			if(!TSProgramElementaryStream<I>::_receiver_written && TSProgramElementaryStream<I>::HasData())
			{
				//WE NEVER WROTE ANYTHING
                bool res = UpdateLookUp(payload);
				_ASSERTE(true); //we never wrote anything so we should have the starting point in it
			}

			_IsDirty = true;
			TSProgramElementaryStream<I>::Receive(payload);
		}

		
	}
	virtual void process(Transport_Packet &ts, CBufferRead &payload)
	{
		if(payload.size() >= getMinAnalyzeSize() && !_HasStruct)
		{
			payload.SetPosition(0);
			UpdateLookUp(payload);
		}

		try{TSProgramElementaryStream<I>::process(ts, payload);}
		catch(...)
		{if(_HasStruct){throw;}}
	}
public:
	TSProgramElementaryStreamAnalyze()
		:_IsDirty(false),
		_HasStruct(false)
	{
	}
	bool IsDirty() {return _IsDirty;}
	bool CanWork() {return (_HasStruct && (0 != this->StreamID()) );}
	T & GetStruct(){return _stru;}

	virtual void Reset()
	{
		TSProgramElementaryStream<I>::Reset();
		_IsDirty = false;
	}
};


typedef TSProgramElementaryStreamAnalyze<SequenceHeader> PESVideo;


//template<typename T>
class MpegAudioAnalize
{
	IBitstream & _bs;
	uint32_t _frame_lenght;
	uint64_t _last_good_frame_lenght;
	uint64_t _last_count;
	
public:
	MpegAudioAnalize(IBitstream & bs):_bs(bs)
	{
		
	}

	static bool find_audio_stream(
		std::map<uint64_t, uint64_t> & possible_positions
		, uint64_t & start
		, uint64_t & count
		, uint64_t & frame_length)
	{
		bool return_value = false;
		
		start = 0;
		count = 0;
		frame_length = 0;

		if(!possible_positions.size())
			return false;

		std::set<uint64_t> candidates;
		
		uint64_t certified = UINT64_MAX;

		std::map<uint64_t, uint64_t>::const_iterator it = possible_positions.end();

		while(it != possible_positions.begin() && UINT64_MAX == certified)
		{
			it--;

			uint64_t p = it->first + it->second;

			if(candidates.find(p) != candidates.end())
			{
				certified = p;
				break;
			}

			candidates.insert(it->first);
		}

		if(certified == UINT64_MAX)
			return false;


		it = possible_positions.end();

		while(it != possible_positions.begin())
		{
			it--;

			uint64_t p = it->first + it->second;

			if(p == certified)
			{
				count++;
				certified = it->first;
				frame_length = it->second;
			}
		}

		start = certified;

		return true;

	}



	///only RAI MUX
//	bool Analize(uint64_t & start, uint64_t max_byte = UINT64_MAX)
//	{
//		uint64_t start_position = _bs.getpos();
//		int count     = 0;
//		uint64_t byte_read = 0;
//		uint64_t last_position  = _bs.getpos();
//		uint64_t last_good_position = last_position;
//		uint64_t      position  = 0;
//
//		bool first_match = false;
//		
//
//		AudioHeader ah;
//		
//	    _frame_lenght = 0;
//		_last_good_frame_lenght = 0;
//
//		start = 0;
//
//		try{while(byte_read <= max_byte)
//		{
//			if(0x000007FF == _bs.nextbits(11))
//			{				
//				position = _bs.getpos();
//				if(  last_position < position &&
//					 _frame_lenght > 0)
//				{
//					if(last_position + _frame_lenght == position)
//					{
//						if(count == 0)
//							start = position - start_position;
//
//						count++;
//						
//						_last_good_frame_lenght = _frame_lenght;
//						last_good_position      = last_position;
//					}else if(last_good_position + _last_good_frame_lenght == position)
//					{
//						//there was a fake
//						count++;
//						last_good_position += _last_good_frame_lenght;
//					}
//				}
//				
//				ah.syncword = 0;
//				ah.layer    = 0xFF;
//				
//				ah.get(_bs);
//				byte_read += 4;
//
//				if(0 < ah.layer && 4 > ah.layer 
//					&& ah.getFrameLength() > 0
//					&& ah.getBitRate() > 0
//					&& ah.getFrequency() > 0
//					)
//				{
//					_frame_lenght  = 8 * ah.getFrameLength();
//					last_position = position;
//					if(0 == position)
//						first_match = true;
//#ifdef _DEBUG
//					unsigned int bit_rate = ah.getBitRate();
//					uint64_t ft = ah.getFrameTime();
//					uint64_t ff = ah.getFrequency();
//					uint64_t ff1 = ah.getFrequency();
//#endif
//				}	
//				
//			}
//			else
//			{
//				_bs.skipbits(8);
//				byte_read++;
//			}
//		}}catch (EndOfData e) {}
//
//		if(count)
//		{
//			_last_count = count;
//			//_ASSERTE(_last_good_frame_lenght == _frame_lenght);
//			start = start - _last_good_frame_lenght;
//			
//			if(0 == (start % _last_good_frame_lenght)
//				&& first_match)
//			{
//				start = 0;
//				_last_count++;
//			}
//			
//			_ASSERTE(start < _last_good_frame_lenght);
//
//			return true;
//		}
//
//		return false;
//	}


	bool Analize(uint64_t & start, uint64_t max_byte = UINT64_MAX)
	{
		uint64_t start_position = _bs.getpos();
		int count     = 0;
		uint64_t byte_read = 0;
		uint64_t last_position  = _bs.getpos();
		uint64_t last_good_position = last_position;
		uint64_t      position  = 0;

		bool first_match = false;
		
		std::map<uint64_t, uint64_t> possible_positions;
		AudioHeader ah;
		
	    _frame_lenght = 0;
		_last_good_frame_lenght = 0;

		start = 0;

		try{while(byte_read <= max_byte)
		{
			if(0x000007FF == _bs.nextbits(11))
			{				
				position = _bs.getpos();
								
				ah.syncword = 0;
				ah.layer    = 0xFF;
				
				ah.get(_bs);
				byte_read += 4;

				if(ah.IsValid())
				{
					_frame_lenght  = 8 * ah.getFrameLength();
					last_position = position;
					if(0 == position)
						first_match = true;
#ifdef _DEBUG
					unsigned int bit_rate = ah.getBitRate();
					uint64_t ft = ah.getFrameTime();
					uint64_t ff = ah.getFrequency();
					uint64_t ff1 = ah.getFrequency();
#endif

					possible_positions[position] = _frame_lenght;

				}	
				
			}
			else
			{
				_bs.skipbits(8);
				byte_read++;
			}
		}}catch (EndOfData e) {}

		return find_audio_stream(
								  possible_positions
								, start
								, _last_count
								, _last_good_frame_lenght);
	}

    uint64_t get_last_count(){return _last_count;}
	uint64_t get_frame_lenght(){return _last_good_frame_lenght;}
	uint64_t get_frame_lenght_byte(){return _last_good_frame_lenght / 8;}
};

/** 
* \brief This class parse and MPEG Audio Stream.
* This class parse and MPEG Audio Stream and allow for discontinuity.
*/
class CMpegAudioStream_old
{
	AudioHeader      _ah;               ///The current audio header
	uint64_t _remaining;
	CBufferRead      _to_do;
	unsigned int     _min_buffer_size; ///do not process unless the buffer is at least this big
    bool             _to_do_certified; ///does the buffer start with a valid header
    
protected:
	
	virtual void process_left(IBitstream & bit_stream, const BYTE* p_data, size_t processed)
	{
		_to_do_certified = (0 == _remaining)?true:false;

		if(4 <= _remaining)
		{
			bit_stream.set_position(processed);
			if(0x000007FF == bit_stream.nextbits(11))
			{
				AudioHeader ah;
					ah.get(bit_stream);

				if(ah.operator==( _ah))
				  _to_do_certified = true;
			}
			
		}

		_to_do.add(p_data + processed, U64_ST(_remaining));

	}
	///huston we got frames
	virtual void frames(const BYTE* p_data              ///[in] pointer to the start of the buffer
		                , size_t frame_length ///[in] lenght of each frame in the buffer
						, size_t frame_count  ///[in] number of frame in the buffer
						)
	{}
	///we found a discontinuity
	virtual void discontinuity()
	{//_ASSERTE(false);
	}
	///inform we got the audio header
	virtual void got_header(AudioHeader& ah){}
	///inform we are about to start processing the first valid 
	///audio header in the buffer
	virtual void pre_buffer_process(){}
	///process at the beginning of a segment
	virtual bool process_begin(const BYTE* p_data
		             , size_t length)
	{
		
        fixed_memory_bitstream bit_stream(p_data, length);
		MpegAudioAnalize analize(bit_stream);

		uint64_t position(0);
		bool t = analize.Analize(position, length);

		if(!t)
		{
			//could not find anything
			return false;
		}
		
		uint64_t throw_away = position / 8;
		bit_stream.set_position(throw_away);
		_ah.get(bit_stream);
		_ASSERTE(analize.get_frame_lenght_byte() == _ah.getFrameLength());
        
		//inform we got header
		got_header(_ah);

		uint64_t send = analize.get_frame_lenght_byte() * analize.get_last_count();
		
		_remaining = length - throw_away - send;
		
		if(throw_away > 0)
			discontinuity();

		pre_buffer_process();
		frames(p_data + throw_away, _ah.getFrameLength(), U64_ST(send / _ah.getFrameLength()));
		
		process_left(bit_stream, p_data, U64_ST(throw_away + send));
		return true;
		
	}

	virtual bool process_to_do()
	{
		_ASSERTE(_to_do.size() == _remaining);

		CBuffer<BYTE> tmp(_to_do.size());

		tmp.add(_to_do.get(), _to_do.size());
        
		_to_do.Reset();
		_remaining = 0;

		return process_begin(tmp.get(), tmp.size());
	}

	virtual bool process(const BYTE* p_data
		             , unsigned int length)
	{
		_ASSERTE(_to_do.size() == _remaining);
		
		bool ret = true;
		fixed_memory_bitstream bit_stream(p_data, length);
		MpegAudioAnalize analize(bit_stream);

		uint64_t position(0);
		bool t = analize.Analize(position, length);

		if(!t)
		{
			return false;
		}

#ifdef _DEBUG
		unsigned int frame_lenght = _ah.getFrameLength();
		unsigned int f2 = static_cast<uint32_t>(analize.get_frame_lenght_byte());
		_ASSERTE(frame_lenght == f2);
		_ASSERTE(_ah.getFrameLength() == analize.get_frame_lenght_byte());
#endif

		uint64_t send = analize.get_frame_lenght_byte() * analize.get_last_count();
		uint64_t byte_position = position / 8;
		if(0 == position && 0 == _to_do.size())
		{	
			pre_buffer_process();
		    frames(p_data, _ah.getFrameLength(), static_cast<uint32_t>(send / _ah.getFrameLength()));
		}
		else
		{
			if(byte_position)
				_to_do.add(p_data, U64_ST(byte_position));

			if(0 == 
				(byte_position + _remaining) % _ah.getFrameLength()
			) //ok we are in business
			{
				frames(_to_do.get(), _ah.getFrameLength(), _to_do.size() / _ah.getFrameLength());
			}
			else
			{
				if(_to_do_certified)
				{
					//no match
					discontinuity();
				}
				else
				{
					ret &= process_to_do();
				}
			}

			_to_do.Reset();

			pre_buffer_process();
            frames(p_data + byte_position, _ah.getFrameLength(), static_cast<uint32_t>(send / _ah.getFrameLength()));
			_remaining = length - byte_position - send;

			process_left(bit_stream, p_data, static_cast<uint32_t>(byte_position + send));
			return ret;
		}
	}

	
public:
	CMpegAudioStream_old():
		  _remaining(UINT64_MAX)
		, _min_buffer_size(2048)
		{}
	virtual void add( const BYTE* p_data
		             , unsigned int length)
	{
		bool ret = false;
		if(_remaining == UINT64_MAX)
		{
			_to_do.Reset(); //throw away left over

			if(length < _min_buffer_size)
			{
				_to_do.add(p_data, length);
				_to_do_certified = false;
				return;
			}

			ret = process_begin(p_data, length);

		}
		else
		{
			//just buffer stuff
			if(length < _min_buffer_size)
			{
				if(0 == _to_do.size())
					_to_do_certified = false;

				_to_do.add(p_data, length);
				_remaining += length;

				ret = true;
				
				if(_to_do.size() >= _min_buffer_size)
				{
					ret = process_to_do();
				}
			}
			else
			{
				ret = process(p_data, length);
			}
		}

		_ASSERTE(ret);

		if(!ret) //we are in trouble
		{
			_remaining = UINT64_MAX; //start over
		}
	}

	void flush()
	{
		process_to_do();
		_remaining = UINT64_MAX;
	}

	const AudioHeader & get_AudioHeader(){return _ah;}
};


/*
class TSProgramElementaryStreamAnalyzeAudio: public TSProgramElementaryStreamAnalyze<AudioHeader>
{
protected:
	virtual bool LookUp(CBufferRead &payload, IBitstream &ms)
	{
		fixed_memory_bitstream mem(payload.GetCurrentPosition() - ms.getUnreadBufferSize(), 
			                payload.ByteToRead() + ms.getUnreadBufferSize()
							);
		
		MpegAudioAnalize a(mem);

		uint64_t start(99);

		bool res = a.Analize(start);

		if(res)
		{
			if(start)
				ms.skipbits(start);
		}

		return res;
	}

	virtual bool IsHim(IBitstream &ms)
	{
		_ASSERTE(false);
		//return (0x000007FF == ms.nextbits(11));

		return false;
	}
};


typedef TSProgramElementaryStreamAnalyzeAudio PESAudio;
*/
class TransportPreprocess
{ 
	IBitstream * _pBitstream;
    
	ProgramAssociationTable _pas;
	ProgramMapTable _map;
public:
	TransportPreprocess(IBitstream * pBitstream)
		:_pBitstream(pBitstream)
	{
	}

	bool ReadTsMaps(unsigned int nTargetProgram = 0)
	{
		Transport_Packet ts;

		while(!(_pas.CanWork() && _map.CanWork()))
		{
			ts.get(*_pBitstream);

			if(ts.PID == 0)
			{
				_pas.add(ts); 	
			}
			else
			{
				if(_pas.CanWork())
			    {
					if(_pas.IsProgramMapPID(ts.PID))
					{
					    if(0 == nTargetProgram || ts.PID == nTargetProgram)
						{
							if(!_map.HasPID() || _map.PID() == ts.PID)
							{
								_map.add(ts);
							}//if _map.HasPID
						}//if(0 == nTargetProgram || ts.PID == nTargetProgram)

					}//_pas.IsProgramMapPID(ts.PID
				}//if(_pas.CanWork())
			}//else
		}//while

		return _pas.CanWork() && _map.CanWork();
	}//end function

	ProgramMapTable & GetMap(){return _map;}
	ProgramAssociationTable & GetAssociationTable(){return _pas;}

};
