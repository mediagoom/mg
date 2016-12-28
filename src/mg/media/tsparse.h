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

#include <TBitstream.h>
#include "TS.H"
#include <map>
#include <iostream>
#include "h264nal.h"
#include "mp4/aac.h"
#include "hnano.h"

__ALX_BEGIN_NAMESPACE

enum TSPROCESSRESULT
{
	  PROCESSED
	, NOT_FOUND
	, MORE_INPUT
	, NO_PAT
	, LATE
	, TSEOF
	, DISCARDED
};

class ITSProcessor
{
public:
	virtual TSPROCESSRESULT process(Transport_Packet &ts, unsigned __int64 position) = 0;
};

class BaseProgramElementaryStreamData: public IProgramElementaryStreamData
{
	unsigned __int64 _sample;
	bool _pes_detail;

protected:

	



public:
	BaseProgramElementaryStreamData():_sample(0)
		, _pes_detail(false)
	{}

	void set_pes_detail(bool d){_pes_detail = d;}
	bool get_pes_detail(){return _pes_detail;}

	virtual bool ReceiveElementaryStreamData(tspacket &Packet, const PesData &pesdata, CBufferRead &payload, unsigned __int64 offset)
	{

		/*
		std::wcout 
			<< "PES " ;

		if(Packet.HasPCR())
			std::wcout << "\tPCR: "      << HNS(Packet.PCR());

			std::wcout
			<< "\tSize: "     << payload.size()
			<< "\t PES PTS: " << HNS(pesdata.GetTime())

			<< "\t PES audio: " << pesdata.IsAudio()
			<< "\t PES video: " << pesdata.IsVideo()

			<< "\t PID: " << Packet.PID()
			
			<< std::endl;
		*/

	

		std::wcout 
					<< L"ES\t"
					<< pesdata.stream_id
					<< L"\t"
					<< _sample++
					<< L"\t"
					<< HNS(pesdata.GetTime())
					<< L"\t";

		if(pesdata.DTS_flags)
		{
					std::wcout << HNS(pesdata.GetDTSTime());
		}
		else
			std::wcout << L"DTS NOT AVL";

		std::wcout	<< L"\t";

		if(Packet.HasFirstPCR())
			std::wcout << HNS(Packet.FirstPCR());
		else			              
			std::wcout << L"PCR NOT AVL";

		std::wcout
			<< L"\t\t";

		if(Packet.HasFirstAdaptation())
				std::wcout << Packet.first_adaptation()->random_access_indicator;
		else
				std::wcout << L"--";

			std::wcout
					<< L"\t"
					<< offset
					<< L"\t"
					<< payload.size()
					<< "\t"
					<< pesdata.PES_header_data_length
					<< "\t"
					<< std::endl;


			if(pesdata.PES_extension_flag)
			{
				std::wcout 
					<< "\t\t\tPES EXTENSION " 
					
					<< "\tprivate data flag "
					<< pesdata.PES_private_data_flag

					<< "\tprivate data flag "
					<< pesdata.PES_private_data_flag

					<< "\t.pack_header_field_flag "
					<< pesdata.pack_header_field_flag

					<< "\t.program_packet_sequence_counter_flag "
					<< pesdata.program_packet_sequence_counter_flag

					<< "\t.P_STD_buffer_flag "
					<< pesdata.P_STD_buffer_flag

					<< "\t.PES_extension_field_flag "
					<< pesdata.PES_extension_field_flag

					<< "\t.P_STD_buffer_scale "
					<< pesdata.P_STD_buffer_scale

					<< "\t.P_STD_buffer_size "
					<< pesdata.P_STD_buffer_size

					<< std::endl;
			}   

			if(_pes_detail)
			{
			}


			return true;
		
	}

};

class InfoTSProgramElementaryStream: public TSProgramElementaryStream<>, public BaseProgramElementaryStreamData
{
protected:
	virtual void incomplete_pes()
	{
		std::wcerr << L"incomplete PES" << std::endl;
	};
	
	virtual void PayloadParseError(CBufferRead &payload, CMediaParserErr &ex)
	{
		std::wcerr << L"PES payload error" << std::endl;
	}

	virtual void EmptyPayload(PesData & pesdata, CBufferRead &payload)
	{
		std::wcerr << L"Empty PES" << std::endl;
	}
public:
	InfoTSProgramElementaryStream()
	{
		this->SetReceiver(this);
	}

	
};


class TSProgramElementaryStreamAnalyzeAAC: public TSProgramElementaryStreamAnalyze<ADTS>, public BaseProgramElementaryStreamData
{
	
	CBufferRead _payload;
	ADTS        _adts;


protected:
	virtual void EmptyPayload(PesData & pesdata, CBufferRead &payload)
	{
		std::wcerr << L"Empty PES" << std::endl;
	}


	virtual bool IsHim(IBitstream &ms)
	{
		unsigned int nextbits =  ms.nextbits(12);
		return nextbits == 0x000FFF;
	}

	
	virtual void AACPayload(ADTS &adts, tspacket &Packet, const PesData &pesdata, CBufferRead &payload, int frame_count)
	{
	}
public:

	TSProgramElementaryStreamAnalyzeAAC()//:_has_picture(false)
	{
		this->SetReceiver(this);
	}

	

	virtual bool ReceiveElementaryStreamData(tspacket &Packet, const PesData &pesdata, CBufferRead &payload, unsigned __int64 offset)
	{
		bool ret = BaseProgramElementaryStreamData::ReceiveElementaryStreamData(Packet, pesdata, payload, offset);

		//std::wcout << L"\t";

		int frame_count(0);

		//AAC STUFF
		fixed_memory_bitstream ms(payload.get() + payload.GetPosition()
			, payload.size() - payload.GetPosition());
		                         //ms.set_throw_on_end(false);

	    unsigned int start_position(0);//, end_position(0);

		while(!ms.atend())
		{
			while(ms.has_bits(12) && ms.nextbits(12) != 0x000FFF)
			{
				if(!ms.atend())
					ms.skipbits(8);
				else
					break;
			}

			if(!ms.atend())
			{
				_adts.get(ms);

				_payload.SetPosition(0);
				_payload.Reset();

				BYTE b = 0;

				for(int i = 0; i < _adts.get_body_length(); i++)
				{
					b = ms.getbits(8);
					_payload.add(&b, 1);

					if(ms.atend())
						break;
				}

				AACPayload(_adts, Packet, pesdata, _payload, frame_count++);

			}


		}

		//std::wcout << L"(" << ((ms.getpos() - start_position)/8) << L")";
		//std::wcout << std::endl;


		return ret;
	}

};



class TSProgramElementaryStreamAnalyzeH264: public TSProgramElementaryStreamAnalyze<H264Sequence>, public BaseProgramElementaryStreamData
{
	pic_parameter_set_rbsp _picture;
	bool _has_picture;

	CBufferRead _payload;


protected:

	virtual void EmptyPayload(PesData & pesdata, CBufferRead &payload)
	{
		std::wcerr << L"Empty PES" << std::endl;
	}

	virtual void structure_get(IBitstream &ms)
	{
		_ASSERTE(IsHim(ms));
		ms.skipbits(32);

		TSProgramElementaryStreamAnalyze<H264Sequence>::structure_get(ms);

		
		unsigned int nextbits =  ms.nextbits(32);
		if((0xFFFFFF1F & nextbits) == 0x00000108)
		{
			ms.skipbits(32);

			_picture.get(ms);

			_has_picture = true;

		}
	}

	virtual bool IsHim(IBitstream &ms)
	{
		unsigned int nextbits =  ms.nextbits(32);
		return (0xFFFFFF1F & nextbits) == 0x00000107;
	}

	virtual void NalPayload(NALTYPE nal_type, tspacket &Packet, const PesData &pesdata, CBufferRead &payload, int size)
	{
		
	}

	virtual void PacketEnd(tspacket &Packet, const PesData &pesdata)
	{
		
	}
public:

	TSProgramElementaryStreamAnalyzeH264():_has_picture(false)
	{
		this->SetReceiver(this);
	}

	

	virtual bool ReceiveElementaryStreamData(tspacket &Packet, const PesData &pesdata, CBufferRead &payload, unsigned __int64 offset)
	{
		bool ret = BaseProgramElementaryStreamData::ReceiveElementaryStreamData(Packet, pesdata, payload, offset);

		if(get_pes_detail())
			std::wcout << L"H264\t";

		//H264 STUFF
		fixed_memory_bitstream ms(payload.get() + payload.GetPosition()
			, payload.size() - payload.GetPosition());
		                         //ms.set_throw_on_end(false);

	    unsigned int start_position(0);//, end_position(0);

		while(!ms.atend())
		{
			while(ms.has_bits(24) && ms.nextbits(24) != 0x000001)
			{
				if(!ms.atend())
					ms.skipbits(8);
				else
					break;
			}

			if(!ms.atend())
			{

				_ASSERTE(ms.nextbits(24) == 0x000001);

				if(start_position > 0)
				{
					if(get_pes_detail())
						std::wcout << L"(" << ((ms.getpos() - start_position)/8) << L")";
				}

				ms.skipbits(24);

				start_position = ms.getpos();

				unsigned int nal_type = ms.nextbits(8);
				nal_type = (0x0000001F & nal_type);
						
				if(get_pes_detail())
					std::wcout << L"\tNAL: " << nal_type << L"\t" << (ms.getpos() / 8);

				_payload.SetPosition(0);
				_payload.Reset();

				BYTE b = 0;
				
				while( ms.has_bits(32)
					&& ms.nextbits(24) != 0x000001 
					&& ms.nextbits(32) != 0x00000001)
				{
					_ASSERTE(ms.has_bits(8));
					 b = ms.getbits(8);
					_payload.add(&b, 1);

					if(ms.atend())
						break;
				}

				//flush
				if(!ms.has_bits(32))
				{
					while(!ms.atend())
					{
						_ASSERTE(ms.has_bits(8));
						 b = ms.getbits(8);
					    _payload.add(&b, 1);
					}
				}


				NalPayload(static_cast<NALTYPE>(nal_type), Packet, pesdata, _payload, ((ms.getpos() - start_position)/8));

			}


		}

		PacketEnd(Packet, pesdata);

		if(get_pes_detail())
		{
			std::wcout << L"(" << ((ms.getpos() - start_position)/8) << L")";
			std::wcout << std::endl;
		}


		return ret;
	}

};



class CTSProcessor: public ITSProcessor
{
	ProgramAssociationTable          _pat;
	std::map<int, ProgramMapTable *> _pmts;

	std::map<int, ts_pes*>           _pes;
		//TSProgramElementaryStreamAnalyze<SequenceHeader>

	

protected:
	virtual void on_base_stream(BaseProgramElementaryStreamData * stream)
	{
	}

	virtual ts_pes * get_ts_pes(int type)
	{
		if(STREAM_TYPE_VIDEO_H264 == type)
		{
			TSProgramElementaryStreamAnalyzeH264 * p = new TSProgramElementaryStreamAnalyzeH264;
			on_base_stream(p);
			return p;
		}

		if(STREAM_TYPE_AUDIO_AAC == type)
		{
			TSProgramElementaryStreamAnalyzeAAC * p = new TSProgramElementaryStreamAnalyzeAAC;
			on_base_stream(p);
			return p;
		}

		InfoTSProgramElementaryStream * p = new InfoTSProgramElementaryStream;
		on_base_stream(p);
		return p;
	}
	
	template <typename M> void clean_up_map(M * pmap)
	{
		M::iterator iter;

		for( iter = pmap->begin(); iter != pmap->end(); iter++ ) 
		{
			delete iter->second;
		}

		pmap->clear();
	}

	void clean_up_pmt()
	{
		/*std::map<int, ProgramMapTable *>::iterator iter;

		for( iter = _pmts.begin(); iter != _pmts.end(); iter++ ) 
		{
			delete iter->second;
		}

		_pmts.clear();*/

		clean_up_map(&_pmts);
	}

	void clean_up_pes()
	{
		/*std::map<int, ProgramMapTable *>::iterator iter;

		for( iter = _pmts.begin(); iter != _pmts.end(); iter++ ) 
		{
			delete iter->second;
		}

		_pmts.clear();*/

		clean_up_map(&_pes);
	}

	
	ProgramAssociationTable * pat() {return &_pat;}
	ProgramMapTable * pmt(int pid)  {return _pmts[pid];}

	bool IsStream(int pid)
	{return !(_pes.find(pid) == _pes.end());}

	virtual ts_pes * get_pes(int pid){return _pes[pid];}

public:

	CTSProcessor()
	{
		
	}

	
	virtual TSPROCESSRESULT process(Transport_Packet &ts, unsigned __int64 position)
	{		

		if(ts.PID == 0)
		{
			if(!_pat.CanWork())
			{
				_pat.add(ts); 

				if(_pat.CanWork())
				{
					for(int idx = 0; idx < _pat.Count(); idx++)
					{
						int pid = _pat.GetProgramPid(idx);

						_pmts[pid] = new ProgramMapTable;
					}
					
					return TSPROCESSRESULT::PROCESSED;
				}
				else
					return TSPROCESSRESULT::MORE_INPUT;
			}
			else
				return TSPROCESSRESULT::LATE;
		}
		else //PID NOT 0
		{
			if(!_pat.CanWork())
			{
				return NO_PAT;
			}

			std::map<int, ProgramMapTable *>::iterator iter = _pmts.find(ts.PID);

			if(_pmts.end() != iter)
			{
				if(iter->second->CanWork())
					return TSPROCESSRESULT::LATE;

				iter->second->add(ts);

				if(iter->second->CanWork())
				{
					for(int idx = 0; idx < iter->second->Count(); idx++)
					{
						int pid  = iter->second->GetPid(idx);
						int type = iter->second->GetType(idx);

						ts_pes * ppes = get_ts_pes(type);

						/*
						switch(type)
						{

						case STREAM_TYPE_VIDEO_MPEG2:
								ppes = new TSProgramElementaryStreamAnalyze<SequenceHeader>;
							break;
						case STREAM_TYPE_AUDIO_MPEG2:
								ppes = new TSProgramElementaryStreamAnalyze<AudioHeader>;
							break;

						default:
							ppes = new ts_pes;


						};
						*/

						_pes[pid] = ppes;

					}

					return TSPROCESSRESULT::PROCESSED;
				} 
				else
					return TSPROCESSRESULT::MORE_INPUT;

			}//PAT

			std::map<int, ts_pes*>::iterator pesiter = _pes.find(ts.PID);

			if(pesiter != _pes.end())
			{

				if(pesiter->second)
				{				
					pesiter->second->add(ts);

					if(pesiter->second->HasData())
					{
						pesiter->second->Reset();
						return TSPROCESSRESULT::PROCESSED;
					}
					else 
						return TSPROCESSRESULT::MORE_INPUT;
				}
				else
					return TSPROCESSRESULT::DISCARDED;

			}

			return TSPROCESSRESULT::NOT_FOUND;

		}

		
	}

	void close()
	{
		clean_up_pmt();
		clean_up_pes();
	}

	virtual ~CTSProcessor()
	{
		close();
	}

};

class CTS
{

protected:
	IBitstream3 *    _p_f;
	CTS():_p_f(NULL){}

private:
	CTS(CTS &rhs);
	int operator=(CTS &rhs);
protected:
	
	void skip(unsigned int bits)
	{
		_ASSERTE(NULL != _p_f);
		_p_f->skipbits(bits);
	}
	
	void skipbytes(unsigned __int64 bits)
	{
		unsigned __int64 to_do_bits = bits * 8ULL;
		unsigned int max = 	INT_MAX / 1024;
		while((to_do_bits ) > max )
		{
			skip(max);
			to_do_bits -= max;
		}
		
		skip(to_do_bits);
	}

	void set_position(unsigned __int64 position)
	{
		_ASSERTE(NULL != _p_f);
		return _p_f->set_position(position);
	}

	
	unsigned int read_uint(unsigned int bits = 32)
	{
		_ASSERTE(32 >= bits);
		return _p_f->getbits(bits);
	}

	void read_bytes(BYTE * pbyte, const unsigned __int64 size)
	{
		for(unsigned __int64 i = 0; i < size; i++)
		{
			pbyte[i] = _p_f->sgetbits(8);
		}
	}
	
public:

	unsigned __int64 get_position()
	{
		_ASSERTE(NULL != _p_f);
		return _p_f->get_position();
	}
		
	bool eof() const {return _p_f->eof();}

	TSPROCESSRESULT process_packets(ITSProcessor & processor, int npacket = 1)
	{
	   Transport_Packet ts;
	   
	   INT64 packet_read = 0;

	   TSPROCESSRESULT result = TSPROCESSRESULT::MORE_INPUT;

	   //_p_f->set

	   //unsigned __int64 total_packet = p_input->get_file_size() / 188;

	   __int64 init_pos = get_position();

	   while((0 == npacket && !eof()) || npacket > packet_read)
	   {

		   if(eof())
			   break;

		   try{
				ts.get(*_p_f);
		  
		   }catch (EndOfData e) 
		   {return TSPROCESSRESULT::TSEOF;}

		   if(get_position() - init_pos != 188)
		   {
			   _ASSERTE(false);
			   DBGC1(L"invalid packet %d\n", init_pos);
		   }

		   init_pos = get_position();

		   /*
		   if(0 == packet_read)
		   {
				packet_read = (get_position() - init_pos) / 188;
				packet_read--;
		   }
		   */

		    result = processor.process(ts, get_position());
		   	
	       /*
		   if(!(packet_read + 1 == _p_f->getpos() / 8 / 188))
		   {
			   unsigned __int64 should = _p_f->getpos() / 8 / 188;
			   fprintf(stdout, "PACKET BOUND DISCONTINUITY %I32u %I64u %I64\n" 
				   , p_input->getpos()
				   , should
				   , packet_read + 1
				   );		
		   }
		   
		   packet_read = _p_f->getpos() / 8 / 188;
		   */

		   packet_read++;
	   }

	   if(eof())
		   return TSPROCESSRESULT::TSEOF;

	   return result;
	}

	
};



typedef file_media_bitstream<CTS> CTSFile;
//typedef sync_file_media_bitstream<CMP4> MP4File;
typedef sync_file_media_bitstream<CTS> SYNCCTSFile;


/*
class CTSFile: public CTS
{
	FileBitstream * _p_ff;
public:
	CTSFile():
	  _p_ff(NULL)
	{}
    void Close()
	{
		if(_p_ff)
			delete _p_ff;
		_p_ff = NULL;
	}
	virtual ~CTSFile(){Close();}
	
	void Open(const TCHAR *pFileName)
	{
		_ASSERTE(NULL == _p_f);
		_ASSERTE(NULL == _p_ff);

		_p_ff = new FileBitstream(pFileName
			, BS_INPUT
			, 1024
			);

		_p_f = _p_ff;
	}

};
*/

__ALX_END_NAMESPACE