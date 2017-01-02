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

#include <mgcore.h>
#include "mp4dynamicinfo.h"
//#include <splitter.h>

#ifdef HAVE_LIBGYPAES
#include <aes.h>
#endif

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

#define ENC_MISSING_SIZE(s) BLOCK_SIZE - (s % BLOCK_SIZE) 
#define ENC_SIZE(s) s + (ENC_MISSING_SIZE(s))

class HLSRenderer: public IDynamicRenderer
{
	 Cstring _root_m3u;
	 Cstring _body;

	 std::map<int, Cstring> _bit_rate_m3u;
	 std::vector<int> _bit_rate;

	 int _index;

	 Cstring _video_codec;
	 Cstring _audio_codec;

	 bool _has_audio;
	 bool _begin_stream;
	 bool _video_points;

	 int _audio_bit_rate;

	 Cstring _prefix;

	 bool _use_stream_name;

	 unsigned char key[BLOCK_SIZE];
	 bool _encrypted;

	 Cstring _key_file_name;

public:
	HLSRenderer() :
		  _index(-1)
		, _has_audio(false)
		, _audio_bit_rate(96000)
		, _root_m3u(10240)
		, _body(10240)
		, _begin_stream(false)
		, _video_points(false)
		, _prefix(_T("video"))
		, _use_stream_name(true)
		, _encrypted(false)
		, _key_file_name(_T("hls3.key"))
	{
	}

	void set_use_stream_name(bool rhs){ _use_stream_name = rhs; }

	void set_prefix(const TCHAR* psz_prefix)
	{
		_prefix = psz_prefix;
	}

	virtual void begin(uint64_t duration)
	{		
		_audio_codec += _T("mp4a.40.2");

		_root_m3u += _T("#EXTM3U\n");
		_root_m3u += _T("#EXT-X-VERSION:3\n");
	}

	virtual void begin_stream(const TCHAR* psz_name, int points, int bitrates, stream_type stype)
	{
		_begin_stream = true;
		_video_points = false;

		if (stream_type::stream_video == stype)
		{
			_video_points = true;

			if (_use_stream_name)
				_prefix = psz_name;
		}

	}

	virtual void add_video_bitrate(int bit_rate, const TCHAR * psz_type, int Width, int Height, BYTE* codec_private_data, int size)
	{
		_index++;

		_ASSERTE(size > 6);

		_video_codec.Clear();
		_video_codec += _T("avc1.");
		//_video_codec.append_hex_buffer(codec_private_data + 3, 3);

		_video_codec.append_hex_buffer(codec_private_data + 3, 1);

		unsigned char profile_compatibility = codec_private_data[4];
		              profile_compatibility = 
						                      (profile_compatibility & 0x80) >> 7
										   || (profile_compatibility & 0x40) << 6
										   || (profile_compatibility & 0x20) << 5
										   || (profile_compatibility & 0x10) << 4
										   ;

	    _video_codec.append_hex_buffer(&profile_compatibility, 1);

		_video_codec.append_hex_buffer(codec_private_data + 5, 1);
	    

		_root_m3u += _T("#EXT-X-STREAM-INF:BANDWIDTH=");
		_root_m3u += (bit_rate + _audio_bit_rate);
		_root_m3u += _T(",RESOLUTION=");
		_root_m3u += Width;
		_root_m3u += _T("x");
		_root_m3u += Height;
		_root_m3u += _T(",CODECS=\"");
		_root_m3u += _video_codec; //TODO LOWER CASE?
		_root_m3u += _T(",");
		_root_m3u += _audio_codec;
		_root_m3u += _T("\"\n");

		Cstring file_name = _prefix.clone();
				file_name += _T("_");
		        file_name += bit_rate;
				file_name += ".m3u8";

		_bit_rate.push_back(bit_rate);

		_bit_rate_m3u[bit_rate] = file_name;

		_root_m3u += file_name;
		_root_m3u += _T("\n");

	}
	virtual void add_audio_bitrate(int bit_rate
		, int AudioTag, int SamplingRate, int Channels, int BitsPerSample, int PacketSize
		, BYTE* codec_privet_data, int size)
	{
		_index++;

		_audio_bit_rate = bit_rate;
		_has_audio = true;
		 

		

		
	}
	virtual void add_point(uint64_t computed_time, uint64_t duration)
	{
		if(!_video_points)
			return;
		
		if(_begin_stream && 0 <= computed_time )
		{
			_begin_stream = false;
			
			_body += _T("#EXTM3U\r\n");
			_body += _T("#EXT-X-VERSION:3\r\n");
			_body += _T("#EXT-X-ALLOW-CACHE:NO\r\n");
			_body += _T("#EXT-X-MEDIA-SEQUENCE:0\r\n");
			_body += _T("#EXT-X-TARGETDURATION:");
			_body += duration / 10000000UL;
			_body += "\r\n";
			_body += _T("#EXT-X-PROGRAM-DATE-TIME:1970-01-01T00:00:00Z\r\n");

			if(_encrypted)
			{
				_body += _T("#EXT-X-KEY:METHOD=AES-128,URI=\"");
				_body += _key_file_name;
				_body += _T("\"\r\n");
			}

		}

		//duration = duration / 10UL;

		uint64_t TNANO = duration % 1000000UL;

		_body += _T("#EXTINF:");
		_body += duration / 10000000UL;
		_body += _T(".");
		_body += TNANO;
		_body += _T(",no-desc\r\n");
		_body += _prefix;
		_body += _T("_");
		_body += _T("$(BITRATE)");
		_body += _T("_");
		_body += computed_time;
		_body += _T(".ts\r\n");
		
	}
    virtual void add_point_info(const TCHAR * psz_path, uint64_t composition, uint64_t computed)
	{
		
	}
	virtual void add_point_info_cross(const TCHAR * psz_path, uint64_t composition, uint64_t computed)
	{
		
	}
	virtual void end_point()
	{
		
	}
	virtual void end_stream()
	{
		
	}
	virtual void end()
	{
		if(!_video_points)
			return;

		_body += _T("#EXT-X-ENDLIST\r\n");

	}

	Cstring & main(){return _root_m3u;}
	int bit_rate_count(){return _bit_rate.size();}

	Cstring bit_rate_name(int idx)
	{
		int bit_rate = _bit_rate[idx];
		return _bit_rate_m3u[bit_rate];
	}

	Cstring bit_rate_body(Cstring bit_rate)
	{
		Cstring body = _body.clone();

		return replace(body, bit_rate, _T("$(BITRATE)"));

	}
	
	Cstring bit_rate_body(int idx)
	{
		int bit_rate = _bit_rate[idx];
		Cstring sbit;
		        sbit += bit_rate;

		return bit_rate_body(sbit);
	}

	void set_key(unsigned char * pkey)
	{
		::memcpy(&key, pkey, BLOCK_SIZE);
		_encrypted = true;
	}

	const unsigned char * get_key() const {return key;}

	Cstring get_key_file_name() { return _key_file_name;}
	void set_key_file_name(const TCHAR* pszfilename){_key_file_name = pszfilename;}

//#pragma optimize( "", off )

#ifdef HAVE_LIBGYPAES
	static long hls3_encrypt_buffer(unsigned char * pdest
		, uint64_t * pdest_size
		, const unsigned char * pclear
		, uint64_t clear_size
		, int sequence
		, unsigned char * pkey
		)
	{
		_ASSERTE(0 < clear_size);

		if(0 >= clear_size)
			return E_INVALIDARG;

		if(0 == (*pdest_size))
		{
			*pdest_size = ENC_SIZE(clear_size);
			 return 1L;
		}

		_ASSERTE(ENC_SIZE(clear_size) <= (*pdest_size));

		if(ENC_SIZE(clear_size) > (*pdest_size))
		{
			return E_INVALID_INPUT;
		}

		uint64_t dest_size = ENC_SIZE(clear_size);
		
		int missing = ENC_MISSING_SIZE(clear_size);

		if(1 > missing || missing > 16)
		{
			ALXTHROW_T(_T("INVALID PCKS7"));
		}

//#define OPTERR
#ifdef OPTERR


		Cstring msg = _T("hls3_encrypt_buffer:\t");
		        msg += clear_size;
				msg += _T("\t");
				msg += dest_size;
				msg += _T("\t");
				msg += missing;
				msg += _T("\t");
				msg += sequence;


				::OutputDebugString(msg);


#endif

		//pcks7
		_ASSERTE((clear_size + missing) == dest_size);

		uint64_t full_block = (clear_size / BLOCK_SIZE) * BLOCK_SIZE;

		_ASSERTE(0 == (full_block % BLOCK_SIZE));

		unsigned char last[BLOCK_SIZE]; //here we store the last block

		::memset(&last, missing, BLOCK_SIZE);

		::memcpy(&last, pclear + full_block, BLOCK_SIZE - missing);



		unsigned char iv[BLOCK_SIZE];

		::memset(iv, 0, BLOCK_SIZE);

		int sn = sequence;
		
		iv[15] =  sn        & 0x000000FF;
		iv[14] = (sn >> 8)  & 0x000000FF;
		iv[13] = (sn >> 16) & 0x000000FF;
		iv[12] = (sn >> 24) & 0x000000FF;

		aes_encrypt_ctx ctx[1];

		AES_RETURN ret = aes_encrypt_key(pkey, BLOCK_SIZE, ctx);
		_ASSERTE(0 == ret);

		ret = aes_cbc_encrypt(pclear, pdest, 
			static_cast<uint32_t>(full_block), iv, ctx);
		_ASSERTE(0 == ret);

		if(BLOCK_SIZE >= missing)
		{
			_ASSERTE(0 != (clear_size % BLOCK_SIZE));

			ret = aes_cbc_encrypt(last, pdest + full_block, BLOCK_SIZE, iv, ctx);
			_ASSERTE(0 == ret);
		}

		*pdest_size = dest_size;

		return 0;
	}
#endif

//#pragma optimize( "", on )

};


