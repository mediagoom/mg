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

#include "stdafx.h"

#include "tsparse.h"
#include "tsmux.h"
#include "MP4DynamicInfo.h"
#include "hls_renderer.h"

using namespace ALX;

#define MAX_STREAMS 16

enum TSINFOFLAG
{
	  HEAD = 1
	, PICK = 2
	, PES  = 4
	, DETAIL = 8
};


class CTSProcessConsole: public CTSProcessor
{

	unsigned __int64 _previus_position;
	unsigned __int64 _packet;
	int _flags;
	public:

	CTSProcessConsole(int flags):_previus_position(0)
		, _packet(0)
		, _flags(flags)
	{}

protected:

	virtual void on_base_stream(BaseProgramElementaryStreamData * stream)
	{
		if(_flags & PES)
			stream->set_pes_detail(true);
	}
	
	virtual TSPROCESSRESULT process(Transport_Packet &ts, unsigned __int64 position)
	{
		int pid = ts.PID;

		TSPROCESSRESULT res = CTSProcessor::process(ts, position);

		if(_flags & HEAD)
		{

			if(res == TSPROCESSRESULT::PROCESSED)
			{
				if(0 == pid)
				{
					std::wcout << "PAT\tGOT Program Association Table " << position << "\t" << _packet 
						<< "\tcontinuity:\t" << ts.continuity_counter << std::endl;

					for(int i = 0; i < pat()->Count(); i++)
					{
						if(0 != pat()->GetProgramNumber(i))
						{
							std::wcout 
								<< L"\t\tProgram Number: " << 
									pat()->GetProgramNumber(i)
								<< L"\tProgram Pid: " << 
									pat()->GetProgramPid(i)
								<< std::endl;
						}
					}
				}
				else if(pat()->CanWork() && pat()->IsProgramPid(pid))
				{
					std::wcout << L"PMT\tGot Program Map Table PIN TABLE "  << position << L"\t" << _packet 
						<< L"\tcontinuity:\t" << ts.continuity_counter << std::endl;

					for(int i = 0; i < pmt(pid)->Count(); i++)
					{
						std::wcout 
							<< L"\t\t" << 
								pmt(pid)->GetPid(i)
							<< L"\t" <<
								pmt(pid)->GetType(i)
							<< L"\t"
							<< pmt(pid)->GetPCRPid()
							<< std::endl;
					}
					
				}
				else if(IsStream(pid))
				{
					if(_flags & PES)
					{
						std::wcout << L"PES\tGot PES " << pid  << L"\t" << position << L"\t" << _packet 
							<< L"\tcontinuity:\t" << ts.continuity_counter << std::endl;
					}

					
				}

			
			}
		}

		if(_flags & DETAIL)
		{

			if(res == TSPROCESSRESULT::NOT_FOUND)
			{
				std::wcout << L"U\tUNKNOWN PID " << pid << L"\t"  << position << "\t" << _packet << std::endl;
			}

			if(res == TSPROCESSRESULT::MORE_INPUT)
			{
	
				int size = this->get_pes(pid)->payload_current_size();

				
				std::wcout << L"INFO\tMORE INPUT PID " << pid 
					<< L"\tsize\t"  << size << L"\t"  << position << L"\t" << _packet 
					<< L"\tcontinuity:\t" << ts.continuity_counter
					<< L"\tadaptation flag:\t" << ts.adaptation_flag 
					<< L"\tpayload flag:\t" << ts.payload_field;

				if(ts.adaptation_flag)
					std::wcout << L"\tadaptation size:\t" << ts.adaptation_field->adaptation_field_length;

				std::wcout
					<< std::endl;
			}

			if(res == TSPROCESSRESULT::LATE)
			{
				std::wcout << L"LATE INPUT PID " << pid << L"\t"  << position << "\t" << _packet << L"\t"  << _packet 
					<< "\t" << ts.continuity_counter << std::endl;
			}

			if(res == TSPROCESSRESULT::NO_PAT)
			{
				std::wcout << L"NO_PAT INPUT PID " << pid << L"\t"  << position << L"\t"  << _packet << std::endl;
			}
		}//not pick

		_previus_position = position;
		_packet++;

		return res;

	}

} ;

class TS2MP4H264: public TSProgramElementaryStreamAnalyzeH264
{
	MP4Mux & _mux;
	int _stream_idx;

	CBuffer<BYTE> _sps;
	CBuffer<BYTE> _pps;

	CBufferRead   _sample_payload;
protected:

	virtual void PacketEnd(tspacket &Packet, const PesData &pesdata)
	{
		bool iframe = false;

		if(Packet.HasFirstAdaptation())
				iframe = Packet.first_adaptation()->random_access_indicator;
		
		_mux.add_sample(_stream_idx
			, _sample_payload.get()
			, _sample_payload.size()
			, iframe
			, pesdata.GetTime()
			, pesdata.GetDTSTime()
			, 0);

		_sample_payload.Reset();
	}

	virtual void NalPayload(NALTYPE nal_type, tspacket &Packet, const PesData &pesdata, CBufferRead &payload, int size)
	{
			if(NALTYPE::AU == nal_type)
				return;

			if(NALTYPE::SEQUENCE == nal_type)
			{
				if(!_sps.size())
				{
					_sps.add(payload.get(), payload.size());
				}

				return;
			}

			if(NALTYPE::PICTURE == nal_type)
			{
				if(!_pps.size())
				{
					_pps.add(payload.get(), payload.size());

					_ASSERTE(_sps.size());

					_stream_idx = _mux.add_visual_stream
					(
						  _sps.get()
						, _sps.size()
						, _pps.get()
						, _pps.size()
					);

					_mux.set_auto_decoding_time(_stream_idx, true);
				}

				return;
			}


		 _ASSERTE(_stream_idx >= 0);

		 //_mux.add_sample(

		 BYTE bsize[4];
			  bsize[0] = payload.size() >> 24 & 0xFF;
		      bsize[1] = payload.size() >> 16 & 0xFF;
			  bsize[2] = payload.size() >> 8 & 0xFF;
			  bsize[3] = payload.size() & 0xFF;

		 _sample_payload.add(bsize, 4);
		 _sample_payload.add(payload.get(), payload.size());


	}

public:
	TS2MP4H264(MP4Mux & mux):_mux(mux)
		, _sps(1024)
		, _pps(1024)
		, _stream_idx(-1)
	{

	}
};


class TS2MP4AAC: public TSProgramElementaryStreamAnalyzeAAC
{
	MP4Mux & _mux;
	int _stream_idx;

protected:

	virtual void AACPayload(ADTS &adts, tspacket &Packet, const PesData &pesdata, CBufferRead &payload, int frame_count)
	{
		int target_bit_rate = payload.size() * (adts.mpeg_4_sampling_frequency / 1024);

		if(-1 == _stream_idx)
		{
			_stream_idx = _mux.add_audio_stream(
				adts.get_mpeg_4_audio_object()
				, adts.mpeg_4_sampling_frequency
				, adts.channel_configuration
				, target_bit_rate * 8
				);
		}
	
		bool iframe = true;

		unsigned __int64 frame_time = 10000000ULL / adts.mpeg_4_sampling_frequency * 1024;

		_mux.add_sample(_stream_idx
			, payload.get()
			, payload.size()
			, iframe
			, pesdata.GetTime() + ( frame_time * frame_count )
			, pesdata.GetTime() + ( frame_time * frame_count )
			, frame_time);

		
	}

public:
	TS2MP4AAC(MP4Mux & mux):_mux(mux)
		, _stream_idx(-1)
	{

	}
};



class CTSProcessConsoleMP4: public CTSProcessConsole
{
	MP4Mux * _p_mux;

protected:

	virtual MP4Mux * create()
	{
		return new MP4Mux;
	}

	virtual void cleanup()
	{
		if(_p_mux)
			delete _p_mux;
	}
	
	virtual MP4Mux & get_mux()
	{
		if(!_p_mux)
			_p_mux = create();

		return (*_p_mux);
	}

	virtual ts_pes * get_ts_pes(int type)
	{
		if(STREAM_TYPE_VIDEO_H264 == type)
		{
			ts_pes * p = new TS2MP4H264(get_mux());
			return p;
		}

		if(STREAM_TYPE_AUDIO_AAC == type)
		{
			ts_pes * p = new TS2MP4AAC(get_mux());
			return p;
		}

		ts_pes * p = new InfoTSProgramElementaryStream;
		return p;
	}

	

public:

	virtual ~CTSProcessConsoleMP4()
	{
		cleanup();
	}

	CTSProcessConsoleMP4():_p_mux(NULL), CTSProcessConsole(true)
	{

	}

	virtual void start(LPCWSTR file_out)
	{
		get_mux().start(file_out);
	}

	virtual void end()
	{
		get_mux().end();
	}

};



struct LL
	{
		sample_stream_info ms;
		unsigned __int64   composition;
		unsigned __int64   decoding;
		unsigned __int64   stream_offset;
		unsigned __int64   total_stream_samples;
	};

template<typename T> 
class CTSEditConsoleT: public T
{

	
	bool    _begin[MAX_STREAMS];
	__int64 _end[MAX_STREAMS];

			
	

	LL _ll[MAX_STREAMS];
	

	void output_composition(const TCHAR * pmsg, const int stream_id, stream_edit_info * ps) const
	{
		/*
		    std::wcout 
				
				   << pmsg
			       << L"\t"
			       << stream_id

				   << L" start comp\t"
				   << HNS(ps->next_composition)
				   << L" start dur\t"
				   << HNS(ps->next_duration)
				   << L" start dec\t"
				   << HNS(ps->next_decoding)
				   << L" prev comp\t"
				   << HNS(ps->stream_composition)
				   << L" prev dur \t"
				   << HNS(ps->stream_duration)
				   << L" prev dec\t"
				   << HNS(ps->stream_decoding)

				   << std::endl;

	   */

				  //<< L"last_composition\t"
				  //<< HNS(ps->last_composition)
				  //<< L"last_duration\t"
				  //<< HNS(ps->last_duration)
				  //<< L"last_decoding\t"
				  //<< HNS(ps->last_decoding)
	}

protected:
	virtual void info_input_stream(const MP4Reader &reader) const
	{
		/*
		for(int i = 0; i < reader.stream_count(); i++)
		{		
				std::wcout << i
				<< L") offset: "
				<< HNS(reader.get_stream_offset(i))
				<< L" duration: " 
				<< HNS(reader.get_duration(i))
				<< L" sample duration: "
				<< HNS(reader.get_sample_duration(i))
				<< L" ["
				<< reader.get_sample_duration(i)
				<< L"]"
				<< L" samples count: "
				<< reader.get_sample_count(i)
				<< std::endl;
		}
		*/
	}

	virtual void info_process_sample(const sample_stream_info & ms
		, unsigned __int64 composition
		, unsigned __int64 decoding
		, unsigned __int64 stream_offset
		, unsigned __int64 total_stream_samples) const 
	{
		
		//if(ms.sample_number < 5 
		//		|| (total_stream_samples - ms.sample_number) < 5)
		/*
		if(ms.sample_number < 2)
			{
				std::wcout << ms.stream
				<< L"\t"
				<< ms.sample_number
				<< L"\t"
				<< HNS(ms.composition_time + stream_offset)
				<< L"\t"
				<< HNS(composition)
				<< L"\t"
				<< HNS(ms.decoding_time)
				<< L"\t"
				<< HNS(decoding)
				<< L"\t"
				<< HNS(ms.duration)
				<< L"\t"
				<< ms.bIsSyncPoint
				<< L"\t"
				<< ms.offset
				<< L"\t"
				<< ms.size
				<< std::endl;
			}
		*/

		if(_begin[ms.stream])
		{
			 if(0 < _end[ms.stream])
			 {
				 std::wcout 
					 << L"E:"
					 << L"\t"
				<< ms.stream
				<< L"\t"
				<< ms.sample_number
				<< L"\t"
				<< HNS(_end[ms.stream])
				<< L"\t"
				<< HNS(composition)
				<< L"\t"
				<< HNS(ms.composition_time - _end[ms.stream] )
				<< L"\t"
				<< HNS( (ms.composition_time + stream_offset) - _end[ms.stream])
				<< L"\t"
				<< HNS(ms.duration)
				<< L"\t"
				<< ms.bIsSyncPoint
				<< L"\t"
				<< ms.offset
				<< L"\t"
				<< ms.size
				<< std::endl;
			 }
			
		}

		if(_begin[ms.stream])
		{
			std::wcout 
					 << L"S:"
					 << L"\t"
				<< ms.stream
				<< L"\t"
				<< ms.sample_number
				<< L"\t"
				<< HNS(ms.composition_time + stream_offset)
				<< L"\t"
				<< HNS(composition)
				<< L"\t"
				<< HNS(ms.decoding_time)
				<< L"\t"
				<< HNS(decoding)
				<< L"\t"
				<< HNS(ms.duration)
				<< L"\t"
				<< ms.bIsSyncPoint
				<< L"\t"
				<< ms.offset
				<< L"\t"
				<< ms.size
				<< std::endl;

			const_cast<CTSEditConsoleT *>(this)->_begin[ms.stream] = false;
		}

		LL ll = {ms 
			            , composition
						, decoding
						, stream_offset
						, total_stream_samples};

		const_cast<CTSEditConsoleT *>(this)->_ll[ms.stream] = ll;
		
	}
	
	virtual void info_pre_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		//output_composition(L"PRE ", stream_id, ps);
	}

	virtual void info_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		//output_composition(L"POST ", stream_id, ps);
	}
public:

	CTSEditConsoleT()
	{
		for(int i = 0; i < MAX_STREAMS; i++)
		{
			_begin[i] = true;
			_end[i]   = 0;
		}
	}

	void set_end(int idx, __int64 t)
	{
		_end[idx] = t;
	}

	virtual ~CTSEditConsoleT()
	{
		for(int i = 0; i < MAX_STREAMS; i++)
		{
			if(!_begin[i])
			{
				std::wcout 
				<< L"F:"
				<< L"\t"
				<< _ll[i].ms.stream
				<< L"\t"
				<< _ll[i].ms.sample_number
				<< L"\t"
				<< HNS(_ll[i].ms.composition_time + _ll[i].stream_offset)
				<< L"\t"
				<< HNS(_ll[i].composition)
				<< L"\t"
				<< HNS(_ll[i].ms.decoding_time)
				<< L"\t"
				<< HNS(_ll[i].decoding)
				<< L"\t"
				<< HNS(_ll[i].ms.duration)
				<< L"\t"
				<< _ll[i].ms.bIsSyncPoint
				<< L"\t"
				<< _ll[i].ms.offset
				<< L"\t"
				<< _ll[i].ms.size
				<< std::endl;
			}
		}
	}
};

typedef CTSEditConsoleT<MP42TS>  CTSEditConsole;
typedef CTSEditConsoleT<MP42HLS> CTSEditConsoleHls;




int mp4_to_ts_segment(  MP42TS & mp4edit
					  , const TCHAR* mp4_input_file
					  , const TCHAR* ts_output_file //NULL TO USE MEMORY
					  , unsigned __int64 start_time
					  , unsigned __int64 end_time
					  )
{
		mp4edit.start(ts_output_file);
		mp4edit.Add(mp4_input_file, start_time, end_time);
		mp4edit.End();

		return 0;
}

int hls_encrypt(const TCHAR* ts_file, int sequence, unsigned char * pkey)
{
	CFileIn inf(ts_file);

	unsigned int size = ENC_SIZE(inf.size());

	CBuffer<unsigned char> encrypted(size);

	HRESULT hr = HLSRenderer::hls3_encrypt_buffer(encrypted.get()
		, &size
		, inf
		, inf.size()
		, sequence
		, pkey);

	_ASSERTE(SUCCEEDED(hr));

	if(FAILED(hr))
		return 7;

	CFileOut outf(ts_file);
	         outf.add(encrypted.get(), size);
}

int do_ts_mux(console_command &c, MP42TS & mp4edit)
{
	unsigned int cnt = c.get_command_count(L"input");
			if(cnt != c.get_command_count(L"starttime") ||
				cnt != c.get_command_count(L"endtime"))
			{
				std::wcerr << 
					L"ee: input start and end must be specified for every input"
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}
			

			if(!c.command_specified(L"output"))
			{

				std::wcerr << 
					L"ee: output not specified"
					<< std::endl;
				return 2021;
			}

			

			if(c.command_specified(L"ctts"))
			{
				bool use_ctts = false;
				use_ctts = c.get_integer64_value(L"ctts");

				mp4edit.set_ctts_offset(use_ctts);
			}

			mp4edit.start(c.get_value(L"output"));
 			
			std::wcout << L"Process Body" << std::endl;
	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				std::wcout << L"Add File " 
					<< c.get_value(L"input", idx)
					<< L" " 
					<< c.get_integer64_value(L"starttime", idx)
					<< L" "
					<< c.get_integer64_value(L"endtime", idx)
					<< std::endl;

				Ctime start_time;

				mp4edit.Add(c.get_value(L"input", idx)
				    , c.get_integer64_value(L"starttime", idx)
					, c.get_integer64_value(L"endtime", idx)
					); 

				Ctime now;

				unsigned __int64 total_time = now.TotalHNano() - start_time.TotalHNano();

				std::wcout << L"Edit add time " 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();			
}






int do_hls_mux(console_command &c)
{
			unsigned int cnt = c.get_command_count(L"input");
			if(cnt != c.get_command_count(L"starttime") ||
				cnt != c.get_command_count(L"endtime"))
			{
				std::wcerr << 
					L"ee: input start and end must be specified for every input"
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}

			unsigned int npaths    = c.get_command_count(L"path");
			unsigned int nbitrates = c.get_command_count(L"bitrate");

			if(npaths + cnt != nbitrates)
			{
				std::wcerr << 
					L"ee: input path and bitrates do not match"
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2021;
			}

			if(0 != (npaths % cnt))
			{
				std::wcerr << 
					L"ee: input and path not in proportion"
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2022;
			}

			std::wcout << L"hls Operation " << std::endl;

			CMP4DynamicInfo mp4edit;
			
			__int64 segment_size = 100000000;
			__int64 end_overflow =  20000000;
			__int64 fragment_tolerance = 660000;

			if(c.command_specified(L"segment"))
				segment_size = c.get_integer64_value(L"segment");

				mp4edit.set_fragment_length(segment_size);
				mp4edit.set_fragment_tolerance(fragment_tolerance);
			
 			
			std::wcout << L"Process Body" << std::endl;

			int path_for_input = npaths / cnt;

			int path_idx    = 0;
			int bitrate_idx = 0;

	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				std::wcout << L"Add File " 
					<< c.get_value(L"input", idx)
					<< L" " 
					<< c.get_integer64_value(L"starttime", idx)
					<< L" "
					<< c.get_integer64_value(L"endtime", idx)
					<< L" "
					<< c.get_integer64_value(L"bitrate", bitrate_idx)
					<< std::endl;


				CBuffer<dynamic_item> ditem(npaths + 1);
				                      ditem.updatePosition(npaths + 1);

				for(unsigned int pp = 0; pp <= npaths; pp++)
				{
					ditem.getAt(pp).bitrate = c.get_integer64_value(L"bitrate", bitrate_idx++) * 1000UL;
					ditem.getAt(pp).audio_bitrate = 96000;
					
					if(0 == pp)
						ditem.getAt(pp).psz_path = c.get_value(L"input", idx);
					else
						ditem.getAt(pp).psz_path = c.get_value(L"path", path_idx++);
						
				}


				Ctime start_time;

				mp4edit.Add(c.get_integer64_value(L"starttime", idx)
					, c.get_integer64_value(L"endtime", idx)
					, ditem.get()
					, ditem.size()
					); 

				Ctime now;

				unsigned __int64 total_time = now.TotalHNano() - start_time.TotalHNano();

				std::wcout << L"Edit add time " 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();	

			HLSRenderer r;
			//SSFRenderer r;
			//DASHRenderer d;

			bool          encrypted = false;
			unsigned char key[BLOCK_SIZE];
			unsigned char * pKey       = NULL;
			
			if(c.command_specified(L"key"))
			{
				Cstring k = c.get_value(L"key");

				_ASSERTE(k.size() == (2*BLOCK_SIZE));

				k.extract_binary_hex(key, BLOCK_SIZE);

				encrypted = true;

				aes_init();

				pKey = key;

				r.set_key(pKey);

			}

			mp4edit.render(&r);

			/*
			std::wcout << static_cast<const TCHAR*>(r.main()) 
				<< std::endl;
			*/
			std::wcout << "==========================HLS============================" << std::endl;

			Cstring out_dir;

			if(c.command_specified(L"output"))
			{
				 out_dir = c.get_value(L"output");
				 Cstring out_xml_file = out_dir.clone();

				 out_xml_file += L"\\main.m3u8";

				 {
				    CstringT<char> main;
					               main += static_cast<const WCHAR*>(r.main());

					CFileOut outfile(out_xml_file, 1024, false);

					outfile.add(reinterpret_cast<const BYTE*>(static_cast<char*>(main)), main.size());
				 }

				 if(encrypted)
				 {
					 Cstring keyfile = out_dir.clone();
					         keyfile += L"\\";
							 keyfile += r.get_key_file_name();

							 CFileOut keyfileout(keyfile);
							          keyfileout.add(pKey, BLOCK_SIZE);
				 }

				 for(int idx = 0; idx < r.bit_rate_count(); idx++)
				 {
					 Cstring out_m3u_file = out_dir.clone();
					         out_m3u_file += L"\\";
							 out_m3u_file += r.bit_rate_name(idx);
							
							 {
							 CFileOut outfile(out_m3u_file, 1024, false);
							 CstringT<char> body;
							                body += static_cast<const WCHAR*>(r.bit_rate_body(idx));
							 outfile.add(reinterpret_cast<const BYTE*>(static_cast<char*>(body)), body.size());
							 }
							 
				 }

				 //std::map<int, int> sequence_map;

				 const DynamicPresentation & p = mp4edit.get_presentation();

				 int mpd_track_id(1);

				 bool separated_audio = false;
				 int  joined_audio_stream_id = 0;

				 for(int i = 0; i < p.Count(); i++)
				 {
					const DynamicStream      * s = p.get_by_index(i);
					if(s->is_audio())
					{
						joined_audio_stream_id = s->id;
						break;
					}
				 }

				 

				 for(int i = 0; i < p.Count(); i++)
				 {
					const DynamicStream      * s = p.get_by_index(i);
					std::map<unsigned __int64, DynamicBitrate *>::const_iterator bitrate = s->begin();

					int track_id(-1);

					if(s->is_audio() && (!separated_audio))
					{
						joined_audio_stream_id = s->id;
						continue;
					}

					

					while(s->end() != bitrate)
					{

						track_id++;
						unsigned char sequence(0);

						//*****Inizialization Segment//

						//**************************//

						__int64 end[MAX_STREAMS];
						for(int i = 0; i < p.Count(); i++)
						{
							end[i] = 0;
						}

						std::map<__int64, __int64>::const_iterator point = s->get_point_begin();
						while(point != s->get_point_end())
						{
							unsigned __int64 computed_time    = point->first;
							unsigned __int64 composition_time = point->second;

							_ASSERTE(composition_time == s->get_original_time(computed_time));

							point++;

							if(point != s->get_point_end())
							{
								unsigned __int64 end_time     = s->get_end_original_time(computed_time);//(composition_time);
								unsigned __int64 end_computed = point->first - computed_time;

								_ASSERTE(end_time == point->second || cnt > 1); //only for single files
								
								Cstring out_chunk_file = out_dir.clone();

								
								if(s->is_audio())
										out_chunk_file += L"\\audio_";

								if(s->is_video())
										out_chunk_file += L"\\video_";
								
								

								out_chunk_file += bitrate->first;
								out_chunk_file += L"_";
								out_chunk_file += computed_time;
								
								if(s->is_audio())
									out_chunk_file += L".ts";

								if(s->is_video())
									out_chunk_file += L".ts";


								if(false)
								{
									CTSEditConsole ts4edit;
									mp4_to_ts_segment(ts4edit
									, s->get_path(bitrate->first, computed_time) //bitrate->second->get_path(computed_time)
									, out_chunk_file
									, composition_time
									, end_time);
								}
								else
								{								
										//======================HLS========================//
										CTSEditConsoleHls ts4edit;
			                           
										ts4edit.set_composition_start_time(s->id, computed_time);
										ts4edit.set_composition_end_time(s->id, end_time);

										std::wcout << "HLS: \t\t\t" 
												   << HNS(computed_time) 
												   << "\t"
												   << HNS(end_computed)
												   << "\t"
												   << HNS(composition_time) 
												   << "\t"
												   << HNS(end_time)
												   << L"\t\t"
												   << static_cast<const TCHAR*>(s->get_path(bitrate->first, computed_time))
												   << std::endl;

										ts4edit.set_section_continuity(sequence++);
										ts4edit.start(out_chunk_file);
										
										if(false)//(!separated_audio)
										{
											const DynamicStream      * as = p.get(joined_audio_stream_id);
											
											__int64 audio_computed = as->get_closest(computed_time);
											__int64 audio_end      = as->get_end_original_time(audio_computed);

											ts4edit.set_composition_start_time(joined_audio_stream_id, audio_computed);
											ts4edit.set_composition_end_time(joined_audio_stream_id, audio_end);

											

											std::wcout << "HLS: \t" 
												       << HNS(computed_time) 
													   << "\t"
													   << HNS(end_time)
																<< "\t"
																<< HNS(composition_time)
																<< "\t"
																<< HNS(audio_computed)
																<< "\t"
																<< HNS(audio_end)
																<< std::endl;

										}


										for(int i = 0; i < p.Count(); i++)
										{
											ts4edit.set_end(i, end[i]);
										}										
										
										ts4edit.Add(s->get_path(bitrate->first, computed_time)
											, composition_time, end_time /*+ end_overflow*/
										);

										ts4edit.stuff_streams();
										ts4edit.flush();
										ts4edit.End();

										/*
										std::wcout << "HLS END: \t" 
											<< HNS(ts4edit.get_last(0)) 
											<< "\t"
											<< HNS(ts4edit.get_last(1)) 
											<< std::endl;
										*/

#if _DEBUG
										for(int i = 0; i < p.Count(); i++)
										{
											end[i] = ts4edit.get_last(i);
										}
#endif
										
										


										//ts4edit.g

										//======================HLS========================//
								}


								if(encrypted)
								{
											hls_encrypt(out_chunk_file, (sequence), pKey);
								}


							}

						}

						bitrate++;
					}
					
					mpd_track_id += s->get_bitrate_count();

				 }

			}

	return 0;
			
}


int _tmain(int argc, _TCHAR* argv[])
{
	
	console_command c;

	c.add(L"input", console_command::type_string , false, L"input file name", 'i');
	c.add(L"kind",  console_command::type_string , false, L"kind of execution \n\r \
														   Could be: \n\r \
														   MP42TS \n\r \
														   auto  \n\r \
														   hls  \n\r \
														   pick  \n\r \
														   PES  \n\r \
														   all  \n\r \
														   ts2mp4  \n\r \
														  ", 'k');

	c.add(L"position",  console_command::type_int , false, L"file position", 'p');
	c.add(L"xml"     ,  console_command::type_string , false, L"xml", 'x');
	c.add(L"handler" ,  console_command::type_string , false, L"handler", 'h');
	c.add(L"size"    ,  console_command::type_int , false, L"size", 'z');

	c.add(L"output", console_command::type_string , false, L"output file", 'o');
	c.add(L"starttime",  console_command::type_int, false, L"starttime", 's');
	c.add(L"endtime", console_command::type_int, false, L"end time", 'e');

	c.add(L"stream", console_command::type_int , false, L"target stream");
	c.add(L"test", console_command::type_string , false, L"target test name", 't');
	c.add(L"help", console_command::type_string , false, L"Display Help", 'h');

	c.add(L"segment", console_command::type_int , false, L"segment duration");

		
	c.add(L"bitrate", console_command::type_int , false, L"bitrate", 'b');
	c.add(L"path"   , console_command::type_string , false, L"path", 'j');

	bool r = c.process(argc, argv);

	if(!r)
	{
		std::wcerr << 
			static_cast<const TCHAR*>(c.get_error_message())
			<< std::endl;

		std::wcout << static_cast<const TCHAR*>(c.get_help())
			<< std::endl;

		return 1;
	}

	if(c.command_specified(L"help"))
	{
		std::wcout << static_cast<const TCHAR*>(c.get_help())
			<< std::endl;

		return 0;
	}

	try{

		std::wstring kind = L"auto";

		if(c.command_specified(L"kind"))
			kind = c.get_value(L"kind");


		std::wcout << L"WORKING TYPE: " << kind << std::endl;


		if(kind == L"test")
		{
			std::wstring test = c.get_value(L"test");
			//return auto_test(test);
		}




		

		Ctime start_time;


		if(kind == L"MP42TS")
		{
			std::wcout << L"Mux Operation " << std::endl;
			
			CTSEditConsole mp4edit;

			int r = do_ts_mux(c, mp4edit);

			if(0 != r)
				return r;
		}
		else if(kind == L"hls")
		{
			
			int r = do_hls_mux(c);

			if(0 != r)
				return r;
		}
		else //TS INPUT
		{
		
		
				CTSFile ts;

				if(!c.command_specified(L"input"))
				{
					std::wcout << "Missing Input File " << std::endl;

					std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

					return 2;
				}

				std::wcout << L"Opening File " << c.get_value(L"input") << std::endl;

				ts.Open(c.get_value(L"input"));
				//nt argc, _TCHAR* argv[])



				if(kind == L"auto" || kind == L"pick" || kind == L"PES" || kind == L"all")
				{
					bool pick = false;

					int flags = HEAD | PICK | DETAIL;

					int size = 100;

					bool stop = false;

					if(c.command_specified(L"size"))
					{
						size = c.get_integer64_value(L"size");
						stop = true;
					}

					if(kind == L"pick" || kind == L"PES" || kind == L"all")
					{
						flags &= ~DETAIL;

						if(kind == L"PES")
							flags |= PES;

						if(kind == L"all")
							flags = flags | DETAIL | PES;

						std::wcout
							<< L"kind"
							<< L"\t"
							<< L"stream"
							<< L"\t"
							<< L"sample"
							<< L"\t"
							<< L"composition_time"
							<< L"\t"
							<< L"decoding_time"
							<< L"\t"
							<< L"pcr"
							<< L"\t\t"
							<< L"bIsSyncPoint"
							<< L"\t"
							<< L"offset"
							<< L"\t"
							<< L"size"
							<< std::endl;
					}								
					
					CTSProcessConsole processor(flags);

					if(stop)
						ts.process_packets(processor, size);
					else
						while(TSPROCESSRESULT::TSEOF != ts.process_packets(processor, size))
						{}
				}//auto - pick
				else if(kind == L"ts2mp4")
				{

					CTSProcessConsoleMP4 processor;

					if(!c.command_specified(L"output"))
					{
						std::wcerr << L"output file not specified" << std::endl;
						return 2;
					}

					processor.start(c.get_value(L"output"));

					int size = 100;
					
					if(c.command_specified(L"size"))
					{
						size = c.get_integer64_value(L"size");
					}

					while(TSPROCESSRESULT::TSEOF != ts.process_packets(processor, size))
					{}

					processor.end();
				}
		}
		 

		Ctime now;

		unsigned __int64 total_time = now.TotalHNano() - start_time.TotalHNano();

		std::wcout << L"Edit add time " 
					<< HNS(total_time)
					<< std::endl;
	
	}catch(alx::Cexception &ex){
		std::wcerr << static_cast<LPCTSTR>(ex) << std::endl;
		return 1;
	}

	return 0;

}

