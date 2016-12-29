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
#include "tsinfo.h"

using namespace ALX;


enum TSINFOFLAG
{
	  HEAD = 1
	, PICK = 2
	, PES  = 4
	, DETAIL = 8
};


class CTSProcessConsole: public CTSProcessor
{

	uint64_t _previus_position;
	uint64_t _packet;
	int _flags;
	std::ostream & _ost;
	public:

	CTSProcessConsole(int flags, std::ostream & ost):_previus_position(0)
		, _packet(0)
		, _flags(flags)
		, _ost(ost)
	{}

protected:

	virtual void on_base_stream(BaseProgramElementaryStreamData * stream)
	{
		if(_flags & PES)
			stream->set_pes_detail(true);
	}
	
	virtual TSPROCESSRESULT process(Transport_Packet &ts, uint64_t position
	)
	{
		int pid = ts.PID;

		TSPROCESSRESULT res = CTSProcessor::process(ts, position);

		if(_flags & HEAD)
		{

			if(res == TSPROCESSRESULT::PROCESSED)
			{
				if(0 == pid)
				{
					_ost << "PAT\tGOT Program Association Table " << position << "\t" << _packet 
						<< "\tcontinuity:\t" << ts.continuity_counter << std::endl;

					for(int i = 0; i < pat()->Count(); i++)
					{
						if(0 != pat()->GetProgramNumber(i))
						{
							_ost 
								<< _T("\t\tProgram Number: ") << 
									pat()->GetProgramNumber(i)
								<< _T("\tProgram Pid: ") << 
									pat()->GetProgramPid(i)
								<< std::endl;
						}
					}
				}
				else if(pat()->CanWork() && pat()->IsProgramPid(pid))
				{
					_ost << _T("PMT\tGot Program Map Table PIN TABLE ")  
						<< position << _T("\t") << _packet 
						<< _T("\tcontinuity:\t") << ts.continuity_counter << std::endl;

					for(int i = 0; i < pmt(pid)->Count(); i++)
					{
						_ost
							<< _T("\t\t") << 
								pmt(pid)->GetPid(i)
							<< _T("\t") <<
								pmt(pid)->GetType(i)
							<< _T("\t")
							<< pmt(pid)->GetPCRPid()
							<< std::endl;
					}
					
				}
				else if(IsStream(pid))
				{
					if(_flags & PES)
					{
						_ost << _T("PES\tGot PES ") << pid  
							<< _T("\t") << position 
							<< _T("\t") 
							<< _packet 
							<< _T("\tcontinuity:\t") << ts.continuity_counter << std::endl;
					}

					
				}

			
			}
		}

		if(_flags & DETAIL)
		{

			if(res == TSPROCESSRESULT::NOT_FOUND)
			{
				_ost << _T("U\tUNKNOWN PID ") << pid 
					<< _T("\t")  << position 
					<< "\t" << _packet << std::endl;
			}

			if(res == TSPROCESSRESULT::MORE_INPUT)
			{
	
				int size = this->get_pes(pid)->payload_current_size();

				
				_ost << _T("INFO\tMORE INPUT PID ") << pid 
					<< _T("\tsize\t")  << size 
					<< _T("\t")  << position 
					<< _T("\t") << _packet 
					<< _T("\tcontinuity:\t") << ts.continuity_counter
					<< _T("\tadaptation flag:\t") << ts.adaptation_flag 
					<< _T("\tpayload flag:\t") << ts.payload_field;

				if(ts.adaptation_flag)
					_ost << _T("\tadaptation size:\t") << ts.adaptation_field->adaptation_field_length;

				std::wcout
					<< std::endl;
			}

			if(res == TSPROCESSRESULT::LATE)
			{
				_ost << _T("LATE INPUT PID ") << pid 
					<< _T("\t")  << position 
					<< "\t" << _packet 
					<< _T("\t")  << _packet 
					<< "\t" << ts.continuity_counter << std::endl;
			}

			if(res == TSPROCESSRESULT::NO_PAT)
			{
				_ost << _T("NO_PAT INPUT PID ") << pid 
					<< _T("\t")  << position
					<< _T("\t")  << _packet << std::endl;
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

		uint64_t frame_time = 10000000ULL / adts.mpeg_4_sampling_frequency * 1024;

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

	CTSProcessConsoleMP4(std::ostream & ost):_p_mux(NULL), CTSProcessConsole(true, ost)
	{

	}

	virtual void start(const TCHAR * file_out)
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
		uint64_t   composition;
		uint64_t   decoding;
		uint64_t   stream_offset;
		uint64_t   total_stream_samples;
	};

template<typename T> 
class CTSEditConsoleT: public T
{

	
	bool    _begin[MAX_STREAMS];
	__int64 _end[MAX_STREAMS];

			
	

	LL _ll[MAX_STREAMS];
	

	void output_composition(const TCHAR * pmsg, const int stream_id, stream_edit_info * ps) const
	{
		
	}

protected:
	virtual void info_input_stream(const MP4Reader &reader) const
	{
		
	}

	virtual void info_process_sample(const sample_stream_info & ms
		, uint64_t composition
		, uint64_t decoding
		, uint64_t stream_offset
		, uint64_t total_stream_samples) const 
	{
		
		

		if(_begin[ms.stream])
		{
			 if(0 < _end[ms.stream])
			 {
				 std::wcout 
					 << _T("E:")
					 << _T("\t")
				<< ms.stream
				<< _T("\t")
				<< ms.sample_number
				<< _T("\t")
				<< HNS(_end[ms.stream])
				<< _T("\t")
				<< HNS(composition)
				<< _T("\t")
				<< HNS(ms.composition_time - _end[ms.stream] )
				<< _T("\t")
				<< HNS( (ms.composition_time + stream_offset) - _end[ms.stream])
				<< _T("\t")
				<< HNS(ms.duration)
				<< _T("\t")
				<< ms.bIsSyncPoint
				<< _T("\t")
				<< ms.offset
				<< _T("\t")
				<< ms.size
				<< std::endl;
			 }
			
		}

		if(_begin[ms.stream])
		{
			std::wcout 
					 << _T("S:")
					 << _T("\t")
				<< ms.stream
				<< _T("\t")
				<< ms.sample_number
				<< _T("\t")
				<< HNS(ms.composition_time + stream_offset)
				<< _T("\t")
				<< HNS(composition)
				<< _T("\t")
				<< HNS(ms.decoding_time)
				<< _T("\t")
				<< HNS(decoding)
				<< _T("\t")
				<< HNS(ms.duration)
				<< _T("\t")
				<< ms.bIsSyncPoint
				<< _T("\t")
				<< ms.offset
				<< _T("\t")
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
		//output_composition(_T("PRE "), stream_id, ps);
	}

	virtual void info_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		//output_composition(_T("POST "), stream_id, ps);
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
				<< _T("F:")
				<< _T("\t")
				<< _ll[i].ms.stream
				<< _T("\t")
				<< _ll[i].ms.sample_number
				<< _T("\t")
				<< HNS(_ll[i].ms.composition_time + _ll[i].stream_offset)
				<< _T("\t")
				<< HNS(_ll[i].composition)
				<< _T("\t")
				<< HNS(_ll[i].ms.decoding_time)
				<< _T("\t")
				<< HNS(_ll[i].decoding)
				<< _T("\t")
				<< HNS(_ll[i].ms.duration)
				<< _T("\t")
				<< _ll[i].ms.bIsSyncPoint
				<< _T("\t")
				<< _ll[i].ms.offset
				<< _T("\t")
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
					  , uint64_t start_time
					  , uint64_t end_time
					  )
{
		mp4edit.start(ts_output_file);
		mp4edit.Add(mp4_input_file, start_time, end_time);
		mp4edit.End();

		return 0;
}

#ifdef HAVE_LIBGYPAES
int hls_encrypt(const TCHAR* ts_file, int sequence, unsigned char * pkey)
{
	sync_file_bitstream inf;
	                    inf.open(ts_file);

	unsigned int size = ENC_SIZE(inf.size());

	CBuffer<unsigned char> clear(inf.size());

	size_t fr = inf.read(size, clear.get());

	inf.close();

	_ASSERTE(fr == size);

	CBuffer<unsigned char> encrypted(size);

	long hr = HLSRenderer::hls3_encrypt_buffer(
		  encrypted.get()
		, &size
		, clear.get()
		, inf.size()
		, sequence
		, pkey);

	_ASSERTE(SUCCEEDED(hr));

	if(FAILED(hr))
		return 7;

	sync_file_bitstream outf;
	                    outf.open(ts_file, false);
	         outf.write(encrypted.get(), size);
			 outf.flush();
			 outf.close();


	return 0;


}

#endif

int do_ts_mux(console_command &c, MP42TS & mp4edit, std::ostream & ost)
{
	unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input")
					<< std::endl;

				ost << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}
			

			if(!c.command_specified(_T("output")))
			{

				std::wcerr << 
					_T("ee: output not specified")
					<< std::endl;
				return 2021;
			}

			

			if(c.command_specified(_T("ctts")))
			{
				bool use_ctts = false;
				use_ctts = c.get_integer64_value(_T("ctts"));

				mp4edit.set_ctts_offset(use_ctts);
			}

			mp4edit.start(c.get_value(_T("output")));
 			
			ost << _T("Process Body") << std::endl;
	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				ost << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< std::endl;

				Ctime start_time;

				mp4edit.Add(c.get_value(_T("input"), idx)
				    , c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					); 

				Ctime now;

				uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

				ost << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();	

			return 0;
}






int do_hls_mux(console_command &c, std::ostream & ost)
{
			unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input")
					<< std::endl;

				ost << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}

			unsigned int npaths    = c.get_command_count(_T("path"));
			unsigned int nbitrates = c.get_command_count(_T("bitrate"));

			if(npaths + cnt != nbitrates)
			{
				std::wcerr << 
					_T("ee: input path and bitrates do not match")
					<< std::endl;

				ost << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2021;
			}

			if(0 != (npaths % cnt))
			{
				std::wcerr << 
					_T("ee: input and path not in proportion")
					<< std::endl;

				ost << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2022;
			}

			ost << _T("hls Operation ") << std::endl;

			CMP4DynamicInfo mp4edit;
			
			__int64 segment_size = 100000000;
			__int64 end_overflow =  20000000;
			__int64 fragment_tolerance = 660000;

			if(c.command_specified(_T("segment")))
				segment_size = c.get_integer64_value(_T("segment"));

				mp4edit.set_fragment_length(segment_size);
				mp4edit.set_fragment_tolerance(fragment_tolerance);
			
 			
			ost << _T("Process Body") << std::endl;

			int path_for_input = npaths / cnt;

			int path_idx    = 0;
			int bitrate_idx = 0;

	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				ost << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("bitrate"), bitrate_idx)
					<< std::endl;


				CBuffer<dynamic_item> ditem(npaths + 1);
				                      ditem.updatePosition(npaths + 1);

				for(unsigned int pp = 0; pp <= npaths; pp++)
				{
					ditem.getAt(pp).bitrate = c.get_integer64_value(_T("bitrate"), bitrate_idx++) * 1000UL;
					ditem.getAt(pp).audio_bitrate = 96000;
					
					if(0 == pp)
						ditem.getAt(pp).psz_path = c.get_value(_T("input"), idx);
					else
						ditem.getAt(pp).psz_path = c.get_value(_T("path"), path_idx++);
						
				}


				Ctime start_time;

				mp4edit.Add(c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					, ditem.get()
					, ditem.size()
					); 

				Ctime now;

				uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

				ost << _T("Edit add time ") 
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
			
			if(c.command_specified(_T("key")))
			{
				Cstring k = c.get_value(_T("key"));

				_ASSERTE(k.size() == (2*BLOCK_SIZE));

				k.extract_binary_hex(key, BLOCK_SIZE);

				encrypted = true;

				aes_init();

				pKey = key;

				r.set_key(pKey);

			}

			mp4edit.render(&r);

			/*
			ost << static_cast<const TCHAR*>(r.main()) 
				<< std::endl;
			*/
			ost << "==========================HLS============================" << std::endl;

			Cstring out_dir;

			if(c.command_specified(_T("output")))
			{
				 out_dir = c.get_value(_T("output"));
				 Cstring out_xml_file = out_dir.clone();

				 out_xml_file += _T("\\main.m3u8");

				 {
				    CstringT<char> main;
					               main += static_cast<const TCHAR*>(r.main());

					/*
					CFileOut outfile(out_xml_file, 1024, false);

					outfile.add(reinterpret_cast<const BYTE*>(static_cast<char*>(main)), main.size());
					*/

								   sync_file_bitstream outfile;
								   outfile.open(out_xml_file, false);

								   outfile.write(reinterpret_cast<const unsigned char *>(static_cast<const char *>(main))
									   , main.size()
								   );

								   outfile.close();
				 }

				 if(encrypted)
				 {
					 Cstring keyfile = out_dir.clone();
					         keyfile += _T("\\");
							 keyfile += r.get_key_file_name();

							 /*
							 CFileOut keyfileout(keyfile);
							          keyfileout.add(pKey, BLOCK_SIZE);
							*/

							 sync_file_bitstream outfile;
							 outfile.open(keyfile, false);

							 outfile.write(pKey
								 , BLOCK_SIZE
							 );

							 outfile.close();
				 }

				 for(int idx = 0; idx < r.bit_rate_count(); idx++)
				 {
					 Cstring out_m3u_file = out_dir.clone();
					         out_m3u_file += _T("\\");
							 out_m3u_file += r.bit_rate_name(idx);
							
							 {
							 //CFileOut outfile(out_m3u_file, 1024, false);
							 CstringT<char> body;
							                body += static_cast<const TCHAR*>(r.bit_rate_body(idx));
							 //outfile.add(reinterpret_cast<const BYTE*>(static_cast<char*>(body)), body.size());

											sync_file_bitstream outfile;
											outfile.open(out_m3u_file, false);

											outfile.write(reinterpret_cast<const BYTE*>(static_cast<char*>(body)), body.size()
												);

											outfile.close();
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
					std::map<uint64_t, DynamicBitrate *>::const_iterator bitrate = s->begin();

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

						//*****Initialization Segment//

						//**************************//

						__int64 end[MAX_STREAMS];
						for(int i = 0; i < p.Count(); i++)
						{
							end[i] = 0;
						}

						std::map<__int64, __int64>::const_iterator point = s->get_point_begin();
						while(point != s->get_point_end())
						{
							uint64_t computed_time    = point->first;
							uint64_t composition_time = point->second;

							_ASSERTE(composition_time == s->get_original_time(computed_time));

							point++;

							if(point != s->get_point_end())
							{
								uint64_t end_time     = s->get_end_original_time(computed_time);//(composition_time);
								uint64_t end_computed = point->first - computed_time;

								_ASSERTE(end_time == point->second || cnt > 1); //only for single files
								
								Cstring out_chunk_file = out_dir.clone();

								
								if(s->is_audio())
										out_chunk_file += _T("\\audio_");

								if(s->is_video())
										out_chunk_file += _T("\\video_");
								

								out_chunk_file += bitrate->first;
								out_chunk_file += _T("_");
								out_chunk_file += computed_time;
								
								if(s->is_audio())
									out_chunk_file += _T(".ts");

								if(s->is_video())
									out_chunk_file += _T(".ts");


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
										ts4edit.set_composition_end_time(s->id
											, s->get_end_computed_time(computed_time));

										ost << "HLS: \t\t\t" 
												   << HNS(computed_time) 
												   << "\t"
												   << HNS(end_computed)
												   << "\t"
												   << HNS(composition_time) 
												   << "\t"
												   << HNS(end_time)
												   << _T("\t\t")
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

											

											ost << "HLS: \t" 
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
											, composition_time
											, end_time /*+ end_overflow*/
										);

										ts4edit.stuff_streams();
										ts4edit.flush();
										ts4edit.End();

										/*
										ost << "HLS END: \t" 
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


int tsinfo(console_command & c, std::ostream & ost)
{
	
	
	/*
	c.add(_T("input")
		, console_command::type_string , false, _T("input file name"), 'i');
	c.add(_T("kind")
		,  console_command::type_string , false, _T("kind of execution \n\r \
														   Could be: \n\r \
														   MP42TS \n\r \
														   auto  \n\r \
														   hls  \n\r \
														   pick  \n\r \
														   PES  \n\r \
														   all  \n\r \
														   ts2mp4  \n\r \
														  "), 'k');

	c.add(_T("position")
		,  console_command::type_int , false, _T("file position"), 'p');
	c.add(_T("xml")     
		,  console_command::type_string , false, _T("xml"), 'x');
	c.add(_T("handler") 
		,  console_command::type_string , false, _T("handler"), 'h');
	c.add(_T("size")    
		,  console_command::type_int , false, _T("size"), 'z');

	c.add(_T("output")
		, console_command::type_string , false, _T("output file"), 'o');
	c.add(_T("starttime")
		,  console_command::type_int, false, _T("starttime"), 's');
	c.add(_T("endtime")
		, console_command::type_int, false, _T("end time"), 'e');

	c.add(_T("stream")
		, console_command::type_int , false, _T("target stream"));
	c.add(_T("test")
		, console_command::type_string , false, _T("target test name"), 't');
	c.add(_T("help")
		, console_command::type_string , false, _T("Display Help"), 'h');

	c.add(_T("segment")
		, console_command::type_int , false, _T("segment duration"));
			
	c.add(_T("bitrate")
		, console_command::type_int , false, _T("bitrate"), 'b');
	c.add(_T("path")   
		, console_command::type_string , false, _T("path"), 'j');

	*/

	if(c.command_specified(_T("help")))
	{
		ost << static_cast<const TCHAR*>(c.get_help())
			<< std::endl;

		return 0;
	}

	try{

		STDTSTRING kind = _T("auto");

		if(c.command_specified(_T("kind")))
			kind = c.get_value(_T("kind"));


		ost << _T("WORKING TYPE: ") << kind.c_str() << std::endl;


		if(kind == _T("test"))
		{
			STDTSTRING test = c.get_value(_T("test"));
			//return auto_test(test);
		}


		Ctime start_time;


		if(kind == _T("MP42TS"))
		{
			ost << _T("Mux Operation ") << std::endl;
			
			CTSEditConsole mp4edit;

			int r = do_ts_mux(c, mp4edit, ost);

			if(0 != r)
				return r;
		}
		else if(kind == _T("hls"))
		{
			
			int r = do_hls_mux(c, ost);

			if(0 != r)
				return r;
		}
		else //TS INPUT
		{
		
		
				SYNCCTSFile ts;

				if(!c.command_specified(_T("input")))
				{
					ost << "Missing Input File " << std::endl;

					ost << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

					return 2;
				}

				ost << _T("Opening File ") << c.get_value(_T("input")) << std::endl;

				ts.open(c.get_value(_T("input")));
				//nt argc, _TCHAR* argv[])



				if(kind == _T("auto") || kind == _T("pick") 
									  || kind == _T("PES") 
									  || kind == _T("all"))
				{
					bool pick = false;

					int flags = HEAD | PICK | DETAIL;

					int size = 100;

					bool stop = false;

					if(c.command_specified(_T("size")))
					{
						size = c.get_integer64_value(_T("size"));
						stop = true;
					}

					if(kind     == _T("pick") 
						|| kind == _T("PES") 
						|| kind == _T("all"))
					{
						flags &= ~DETAIL;

						if(kind == _T("PES"))
							flags |= PES;

						if(kind == _T("all"))
							flags = flags | DETAIL | PES;

						std::wcout
							<< _T("kind")
							<< _T("\t")
							<< _T("stream")
							<< _T("\t")
							<< _T("sample")
							<< _T("\t")
							<< _T("composition_time")
							<< _T("\t")
							<< _T("decoding_time")
							<< _T("\t")
							<< _T("pcr")
							<< _T("\t\t")
							<< _T("bIsSyncPoint")
							<< _T("\t")
							<< _T("offset")
							<< _T("\t")
							<< _T("size")
							<< std::endl;
					}								
					
					CTSProcessConsole processor(flags, std::cout);

					if(stop)
						ts.process_packets(processor, size);
					else
						while(TSPROCESSRESULT::TSEOF != ts.process_packets(processor, size))
						{}
				}//auto - pick
				else if(kind == _T("ts2mp4"))
				{

					CTSProcessConsoleMP4 processor(std::cout);

					if(!c.command_specified(_T("output")))
					{
						std::wcerr << _T("output file not specified") << std::endl;
						return 2;
					}

					processor.start(c.get_value(_T("output")));

					int size = 100;
					
					if(c.command_specified(_T("size")))
					{
						size = static_cast<int>(c.get_integer64_value(_T("size")));
					}

					while(TSPROCESSRESULT::TSEOF != ts.process_packets(processor, size))
					{}

					processor.end();
				}
		}
		 

		Ctime now;

		uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

		ost << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
	
	}
	catch (std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 125;

	}
	catch (...)
	{
		std::cout << "...." << std::endl;
		return 127;
	}

	return 0;

}

