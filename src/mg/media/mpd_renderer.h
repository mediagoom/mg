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

#include "mp4dynamicinfo.h"
//#include <splitter.h>


class MPDRenderer: public IDynamicRenderer
{
	 Cstring _xml;
	 int _stream_id;
	 int _group_id;

	 Cstring _stream_xml;
	 Cstring _points_xml;
	 Cstring _rappresentation_xml;

	 Cstring _codecs;
     int _maxWidth;
     int _maxHeight;

	 Cstring _current_stream;
	 stream_type _stype;

	 uint64_t _current_stream_time;
	 uint64_t _last_stream_duration;
	 uint64_t _last_stream_count;

	 bool _use_stream_name;

	 std::vector<Cstring> _v_content_protection;

public:
	MPDRenderer() : _xml(10480), _stream_id(0), _group_id(1), _maxWidth(0), _maxHeight(0), _use_stream_name(true)
	{
	}

	void add_content_protection(Cstring & cp){_v_content_protection.push_back(cp);}

	
	virtual void begin(uint64_t duration)
	{	
		uint64_t TNANO = duration % 1000000UL;
	
		_xml += _T("<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
		_xml += _T("profiles=\"urn:mpeg:dash:profile:isoff-live:2011\" type=\"static\" ");

		if(_v_content_protection.size())
		{
			_xml += _T("xmlns:cenc=\"urn:mpeg:cenc:2013\" xmlns:mspr=\"urn:microsoft:playready\" ");
		}

        _xml += _T("mediaPresentationDuration=\"PT");
		_xml += duration / 10000000UL;
		_xml += _T(".");
		_xml += TNANO;
		_xml += _T("S\" minBufferTime=\"PT3S\">");

		_xml += _T("<Period>");
	}

	virtual void begin_stream(const TCHAR * psz_name, int points, int bitrates, stream_type stype)
	{
		_stream_id++;

		_codecs = _T("");
		_rappresentation_xml = _T("");
		_points_xml = _T("");

		_current_stream = psz_name;
		_stype = stype;
		_current_stream_time = 0;
		_last_stream_duration = 0;
	    _last_stream_count = 0;

		if(!_use_stream_name)
		{
			if(stream_type::stream_video == stype)
				_current_stream = _T("video");
			else
				_current_stream = _T("audio");
		}

		_stream_xml = _T("<AdaptationSet id=\"");
		_stream_xml += _stream_id;
		_stream_xml += _T("\" group=\"");
		_stream_xml += _group_id;
		_stream_xml += _T("\" profiles=\"ccff\" bitstreamSwitching=\"false\" segmentAlignment=\"true\" contentType=\"");
		_stream_xml += (stream_type::stream_video == stype)?_T("video"):_T("audio");
        _stream_xml += "\" mimeType=\"";
		_stream_xml += (stream_type::stream_video == stype)?_T("video/mp4"):_T("audio/mp4");
		_stream_xml += "\" ";

	}

	virtual void add_video_bitrate(int bit_rate, const TCHAR * psz_type, int Width, int Height, BYTE* codec_private_data, int size)
	{
		Cstring codecs;

		_ASSERTE(size > 6);

		codecs += _T("avc1.");
		codecs.append_hex_buffer(codec_private_data + 3, 1);

		unsigned char profile_compatibility = codec_private_data[4];
		              profile_compatibility = 
						                      (profile_compatibility & 0x80) >> 7
										   || (profile_compatibility & 0x40) << 6
										   || (profile_compatibility & 0x20) << 5
										   || (profile_compatibility & 0x10) << 4
										   ;

		codecs.append_hex_buffer(&profile_compatibility, 1);

		codecs.append_hex_buffer(codec_private_data + 5, 1);

		bool add_codec = false;

		if(0 == _codecs.size())
		{
			_codecs = codecs;
		}
		else
		{
			if(!Equals(_codecs, codecs))
				add_codec = true;
		}

		if(_maxWidth < Width)
			_maxWidth = Width;

		if(_maxHeight < Height)
			_maxHeight = Height;


		_rappresentation_xml += _T("<Representation id=\"");
		_rappresentation_xml += _stream_id;
	    _rappresentation_xml += _T("_");
		_rappresentation_xml += _current_stream;
		_rappresentation_xml += _T("_");
		_rappresentation_xml += _group_id;
		_rappresentation_xml += "\" bandwidth=\"";
		_rappresentation_xml += bit_rate;

		if(add_codec)
		{
			_rappresentation_xml += "\" codecs=\"";
			_rappresentation_xml += codecs;			
		}

		_rappresentation_xml += "\" width=\"";
		_rappresentation_xml += Width;
		_rappresentation_xml += "\" height=\"";
		_rappresentation_xml += Height;
		_rappresentation_xml += "\"/>";
		

		_group_id++;
	}
	virtual void add_audio_bitrate(int bit_rate
		, int AudioTag, int SamplingRate, int Channels, int BitsPerSample, int PacketSize
		, BYTE* codec_privet_data, int size)
	{
		
		if(0 == _codecs.size())
		{
			_codecs = _T("mp4a.40.2");
		}

		_rappresentation_xml += _T("<Representation id=\"");
		_rappresentation_xml += _stream_id;
	    _rappresentation_xml += _T("_");
		_rappresentation_xml += _current_stream;
		_rappresentation_xml += _T("_");
		_rappresentation_xml += _group_id;
		_rappresentation_xml += "\" bandwidth=\"";
		_rappresentation_xml += bit_rate;
		_rappresentation_xml += "\" audioSamplingRate=\"";
		_rappresentation_xml += SamplingRate;
		_rappresentation_xml += "\"/>\r\n";
		

		_group_id++;
		 

		

		
	}
	virtual void add_point(uint64_t computed_time, uint64_t duration)
	{
		if(_last_stream_duration != duration || computed_time != _current_stream_time || 0 == _current_stream_time)
		{

			if( (computed_time != _current_stream_time
				  ||   
					_last_stream_duration != duration)
				&& _last_stream_count > 0)
			{
				_points_xml += _T("<S d=\"");
				_points_xml += _last_stream_duration;
				
				if(1 < _last_stream_count)
				{
					_points_xml += _T("\" r=\"");
					_points_xml += (_last_stream_count - 1);
				}
				_points_xml += _T("\" />\r\n");

				_last_stream_count = 0;
				_last_stream_duration = 0;
			}
		}



		if( computed_time != _current_stream_time )
		{
			_ASSERTE(0 == _last_stream_count);
			
			_points_xml += _T("<S d=\"");
			_points_xml += duration;
			_points_xml += _T("\" t=\"");
			_points_xml += computed_time;
			_points_xml += _T("\" />\r\n");

			_current_stream_time = computed_time;
			_last_stream_duration = 0;

		}
		else
		{
			_ASSERTE( (_last_stream_duration == 0 && _last_stream_count == 0 ) || _last_stream_duration == duration);
			
			_last_stream_count++;
			_last_stream_duration = duration;

		}
			
		_current_stream_time += duration;
		
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
			if(_last_stream_count > 0)
			{
				_points_xml += _T("<S d=\"");
				_points_xml += _last_stream_duration;
				if(1 < _last_stream_count)
				{
					_points_xml += _T("\" r=\"");
					_points_xml += (_last_stream_count - 1);
				}
				_points_xml += _T("\" />\r\n");

				_last_stream_count = 0;
			}

			
			_stream_xml += _T(" codecs=\"");
			_stream_xml += _codecs;

			if(stream_type::stream_video == _stype)
			{
				_stream_xml += _T("\" maxWidth=\"");
			    _stream_xml += _maxWidth;
			    _stream_xml += _T("\" maxHeight=\"");
				_stream_xml += _maxHeight;
			}

			_stream_xml += _T("\" startWithSAP=\"1\" >");

			_stream_xml += _T("\r\n");


			for(uint32_t k = 0; k < _v_content_protection.size(); k++)
			{
				_stream_xml += _v_content_protection[k];
			}

			_stream_xml += _T("<SegmentTemplate timescale=\"10000000\" media=\"");
			_stream_xml += _current_stream;
			_stream_xml += _T("_$Bandwidth$_$Time$.m4");
			_stream_xml += (stream_type::stream_video == _stype)?_T("v"):_T("a");
			_stream_xml += _T("\" initialization=\"");
			_stream_xml += _current_stream;
			_stream_xml += _T("_$Bandwidth$_i.m4");
			_stream_xml += (stream_type::stream_video == _stype)?_T("v"):_T("a");
			_stream_xml += _T("\" >\r\n");

			_stream_xml += _T("<SegmentTimeline>\r\n");

			_stream_xml += _points_xml;

			_stream_xml += _T("\r\n</SegmentTimeline>");
			_stream_xml += _T("\r\n</SegmentTemplate>");

			_stream_xml += _rappresentation_xml;

			_stream_xml += _T("\r\n</AdaptationSet>");

			_xml += _stream_xml;

	   
	}
	virtual void end()
	{
		_xml += "</Period></MPD>";

	}

	Cstring & xml(){return _xml;}

	void set_use_stream_name(bool rhs){ _use_stream_name = rhs; }

	
};


