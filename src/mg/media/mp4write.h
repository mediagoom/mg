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

#include "TBitstream.h"
#include "media_parser.h"
#include "mp4/mp4i_ext.h"

#include <map>
#include <vector>
#include <stack>
#include <algorithm>
#include <set>

#include "media_queue.h"

#include "hnano.h"

#include "mp4/mpeg4_odf.h"
#include "mp4/aac.h"

#include "mp4/ltcextension.h"

#include "mp4/cenc.h"

#define NOWRITE UINT64_MAX
#define DEFAULT_NOCTTS true

#ifdef _DEBUG
#define MAX_DECODING_DISTANCE 3000000llu
#endif

__ALX_BEGIN_NAMESPACE

inline uint64_t MP4NOW(){Ctime t1(1904, 1, 1);Ctime now(false);return now.Seconds(&t1.get_Time());}

class MP4Exception: public mgexception
{
public:
	MP4Exception(
		  const TCHAR* sDescription
		, const TCHAR* sFile
		, int iLine
		, uint32_t iNumber):
	mgexception(iNumber, sDescription, sFile, iLine)
	{
	}
};

#define THROW_MP4EX(error, number)  throw MP4Exception(error, _T(__FILE__), __LINE__, number);

class CMP4W
{
	struct box_parent{uint32_t type; uint64_t position;};
	std::stack<box_parent> _parents;

	uint64_t _pinned_position;
	uint64_t _last_position;

protected:
	IBitstream3 *    _p_f;
	CMP4W():_p_f(NULL)
		, _pinned_position(UINT64_MAX)
		, _last_position(UINT64_MAX)
	{}

public:
	virtual ~CMP4W()
	{}

	virtual uint64_t get_position()
	{
		_ASSERTE(NULL != _p_f);
		//return _p_f->getposex() / 8ULL;
		return _p_f->get_position();
	}
		
	template<typename BOX> void write_box(BOX &box)
	{
		box.put(*_p_f);
	};
	template<typename BOX> void write_child_box(uint32_t type, BOX &box)
	{
		uint64_t init = get_position();

		_p_f->putbits(0xFFFFFFFF, 32);//size
		_p_f->putbits(type, 32);
		
		write_box(box);

       uint64_t end  = get_position();
	   uint64_t size = end - init;

	   _ASSERTE(size < UINT32_MAX);

	   _p_f->set_position(init);
	   _p_f->putbits(static_cast<uint32_t>(size), 32);

	   _p_f->set_position(end);       

	}

    virtual void open_box(uint32_t type)
	{
		box_parent bp = {type, get_position()};
		
		_p_f->putbits(0xFFFFFFFF, 32);//size
		_p_f->putbits(type, 32);

		_parents.push(bp);
	}

	virtual void close_box(uint32_t type)
	{
		_ASSERTE(_parents.top().type == type);

		uint64_t start = _parents.top().position;
        uint64_t end   = get_position();
		uint64_t size  = end - start;

		_ASSERTE(size < UINT32_MAX);

	    _p_f->set_position(start);
	    _p_f->putbits(static_cast<uint32_t>(size), 32);
	    _p_f->set_position(end);

	    _parents.pop();
	}

	virtual void write_bytes(const BYTE* p_bytes, uint32_t size)
	{
		/*
		for(uint32_t i = 0; i < size; i++)
			_p_f->putbits(p_bytes[i], 8);
		*/
		_p_f->write(p_bytes, size);
	}

	virtual void write_uint(uint32_t x)
	{
		_p_f->putbits(x, 32);
	}

	void pin_position()
	{
		_ASSERTE(UINT64_MAX == _pinned_position);
		_ASSERTE(UINT64_MAX == _last_position);
		_pinned_position = get_position();
	}
	void pop_pin()
	{
		_ASSERTE(UINT64_MAX != _pinned_position);
		_ASSERTE(UINT64_MAX == _last_position);
		_last_position = get_position();
		_p_f->set_position(_pinned_position);

		_pinned_position = UINT64_MAX;

	}
	void un_pin()
	{
		_ASSERTE(UINT64_MAX == _pinned_position);
		_ASSERTE(UINT64_MAX != _last_position);
		_p_f->set_position(_last_position);

		_last_position = UINT64_MAX;
	}

	void set_position(uint64_t position)
	{
		_p_f->set_position(position);
	}
};

class CMP4Stream
{
	int _stream_id;
	struct chunk_des
	{
		uint64_t position; 
		uint64_t samples; 
		bool continuity;
	};
	struct sample_des
	{ 
	  uint64_t composition;
	  uint64_t decoding;
	  uint64_t size;
	};

	std::vector<sample_des>       _samples;
	std::vector<chunk_des>        _chunks;
	std::vector<uint64_t> _iframes;

	bool _auto_decoding_time;
	std::vector<uint64_t> _auto_decoding_vector;

	uint64_t _sample_max;
	uint64_t _sample_min;

	grow_queue<> _sample_queue;
	uint64_t _last_sample_end;
	uint64_t _last_chunk_start;
	uint64_t _sample_in_chunk;
	uint64_t _last_sample_chunk_count;
	uint64_t _stco_position;

	uint64_t _time_scale;
	uint64_t _mvhd_time_scale;

	bool _has_composition;
	bool _has_size;
	bool _chunk_flushed;

	bool _allow_empty_stream;

	TCHAR _lang[4];

	uint64_t _stream_offset;
	uint64_t _ctts_offset;
	uint64_t _stts_offset;


#ifdef CENC
	bool _has_cenc_id;
	unsigned char _cenc_id[ID_LEN];
#endif


public:

#ifdef CENC
	void set_key_id(unsigned char * p_key_id)
	{
			::memcpy(_cenc_id, p_key_id, ID_LEN);
		   _has_cenc_id = true;
	}
#endif

	static uint64_t time_scale(const uint64_t hnano, uint64_t timescale)
	{
		double seconds = (double)hnano / 10000000.0;
		double t = seconds * (double)timescale;

		return static_cast<uint64_t>(t);
	}
protected:

#ifdef CENC
	virtual uint32_t get_original_format_type() const = 0;
	bool has_cenc_id() const {return _has_cenc_id;}
	const unsigned char * get_key_id() const
	{
		return &_cenc_id[0];
	}
#endif


	virtual uint64_t get_timescale()const {return _time_scale;}
	
	virtual uint64_t time_scale(const uint64_t hnano) const
	{
		/*double seconds = (double)hnano / 10000000.0;
		double t = seconds * (double)get_timescale();
		
		return t;
		*/
		
		return time_scale(hnano, get_timescale());
	}
	virtual uint64_t get_hnano(const uint64_t time_scaled) const
	{
		double seconds = (double)time_scaled / (double)get_timescale();
		double hnano =  seconds * 10000000.0;

		return static_cast<uint64_t>(hnano);
	}

	virtual void register_chunk()
	{
		chunk_des c = {_last_chunk_start
			, _sample_in_chunk
		    , _last_sample_chunk_count == _sample_in_chunk
		};
		_chunks.push_back(c);
		_last_sample_chunk_count = _sample_in_chunk;

		_chunk_flushed = true;
	}

	virtual void write_stream_entry(CMP4W &mp4w) = 0;
	virtual void fill_track_header(TrackHeaderBox &box) = 0;
	
	virtual void fill_media_header(MediaHeaderBox &box)
	{
		box.set_lang(_lang);
		box.set_timescale(static_cast<uint32_t>(get_timescale()));
		box.set_duration(time_scale(get_duration()));
	}
	virtual bool IsVisual() = 0;

	virtual void fill_media_handler(HandlerBox &box)
	{
		if(IsVisual())
		   box.handler_type = BOX('v','i','d','e');
		else
		   box.handler_type = BOX('s', 'o','u','n');
	}

	virtual void write_media_header(CMP4W &mp4w)
	{
		if(IsVisual())
		{
			VideoMediaHeaderBox box;
			mp4w.write_child_box(box_vmhd, box);
		}
		else
		{
			SoundMediaHeaderBox box;
			mp4w.write_child_box(box_smhd, box);
		}
	}

    
#ifdef CENC
	virtual void write_protected_sinf(CMP4W & mp4w)
	{

		if(has_cenc_id())
		{
			mp4w.open_box(box_sinf);

			OriginalFormatBox frma;
							  frma.data_format = get_original_format_type();

							  mp4w.open_box(box_frma);
								mp4w.write_box(frma);
							  mp4w.close_box(box_frma);

		   SchemeTypeBox schm;
						 schm.scheme_type    = box_cenc;
						 schm.scheme_version = 0x10000;

								mp4w.open_box(box_schm);
									mp4w.write_box(schm);
								mp4w.close_box(box_schm);

								mp4w.open_box(box_schi);
								
									TrackEncryptionBox tenc;
													   tenc.default_isProtected = 1;
													   tenc.default_Per_Sample_IV_Size = 8;
													   //tenc.default_constant_IV_size = 8;

													   ::memcpy(&tenc.default_KID, get_key_id(), ID_LEN);

										   mp4w.open_box(box_tenc);
												mp4w.write_box(tenc);
										   mp4w.close_box(box_tenc);

								mp4w.close_box(box_schi);


			mp4w.close_box(box_sinf);
		}

	}
#endif


	virtual void write_stsd(CMP4W &mp4w)
	{
        //TODO: CHECK CENC 2 entry
		SampleDescriptionBox stsd;
#ifdef FRAGMENTEDSTYPFALSE
		                     stsd.entry_count = 1;
#else
                             stsd.entry_count = (has_cenc_id())?2:1;
#endif

		mp4w.open_box(box_stsd);
		mp4w.write_box(stsd);

        write_stream_entry(mp4w);

#ifdef FRAGMENTEDSTYPTRUE

        if(has_cenc_id())
        {
            _has_cenc_id = false;
            write_stream_entry(mp4w);
            _has_cenc_id = true;
        }

#endif

		mp4w.close_box(box_stsd);
	}
	
	virtual void write_stts(CMP4W &mp4w)
	{
		uint32_t entry_count(0);
		uint32_t sample_delta(0);
		uint64_t last(0);

		uint64_t current_count(0);
		//uint64_t current_time(0);

		FullBox stts;

		mp4w.open_box(box_stts);
		mp4w.write_box(stts);

		mp4w.pin_position();

		mp4w.write_uint(0xFFFFFFFF);

		
		for(uint32_t i = 0; i < _samples.size(); i++)
		{
			_ASSERTE(0 == i || _samples[i].decoding > last);

			uint64_t current_delta = time_scale(_samples[i].decoding - last);
			//last = _samples[i].decoding;
			last += get_hnano(current_delta);
			
			
			if(1 == i)
			{
				sample_delta  = static_cast<uint32_t>(current_delta);
				current_count = 0;
			}

			if(current_delta != sample_delta && 0 < i)
			{
				mp4w.write_uint(static_cast<uint32_t>(current_count));
				//if(!entry_count && _stts_offset)
				//{
				//   _ASSERTE(1 == current_count);
				//   mp4w.write_uint(sample_delta + _stts_offset);
				//}
				//else
				//{
					mp4w.write_uint(sample_delta);
				//}

				entry_count++;

				sample_delta  = static_cast<uint32_t>(current_delta);
				current_count = 0;
			}
			
			current_count++;
		}


		if(_samples.size() || !_allow_empty_stream)
		{
			current_count++;

			mp4w.write_uint(static_cast<uint32_t>(current_count));
			mp4w.write_uint(sample_delta);

			entry_count++;
		}

		mp4w.pop_pin();
		mp4w.write_uint(entry_count);
		mp4w.un_pin();

		mp4w.close_box(box_stts);
	}

	virtual void write_ctts(CMP4W &mp4w)
	{
		if(!_has_composition && !_ctts_offset)
			return;

		uint32_t entry_count(0);
		uint32_t sample_delta(0);

		uint64_t current_count(0);

		

		FullBox ctts;

		mp4w.open_box(box_ctts);
		mp4w.write_box(ctts);

		mp4w.pin_position();

		mp4w.write_uint(0xFFFFFFFF);		

		for(uint32_t i = 0; i < _samples.size(); i++)
		{ 
			int64_t delta = _samples[i].composition - _samples[i].decoding;
			
			if(_ctts_offset)
				delta += get_ctts_offset();

			_ASSERTE(0 <= delta);

			uint64_t current_delta = time_scale(delta);

			if(0 == i)
				sample_delta = static_cast<uint32_t>(current_delta);

			if(current_delta != sample_delta)
			{
				mp4w.write_uint(static_cast<uint32_t>(current_count));
				mp4w.write_uint(sample_delta);

				entry_count++;

				sample_delta  = static_cast<uint32_t>(current_delta);
				current_count = 0;
			}
			
			current_count++;
		}

			mp4w.write_uint(static_cast<uint32_t>(current_count));
			mp4w.write_uint(sample_delta);

			entry_count++;
		

		mp4w.pop_pin();
		mp4w.write_uint(entry_count);
		mp4w.un_pin();

		mp4w.close_box(box_ctts);

	}

	virtual void write_stsc(CMP4W &mp4w)
	{
        uint32_t entry_count(0);
		
		uint32_t sample_count(0);

		uint32_t chunk_start(0);

		uint32_t current_sample_count(0);

		uint64_t current_count(0);

		FullBox stsc;

		if(!_chunks.size() && !_allow_empty_stream)
		{
			ALX::Cstring error(_T("NO CHUNK TO WRITE. STREAM ID: "));
			             error += _stream_id;
			THROW_MP4EX(static_cast<const TCHAR*>(error)
			, E_UNKNOWN
			);
		}

		mp4w.open_box(box_stsc);
		mp4w.write_box(stsc);

		mp4w.pin_position();

		mp4w.write_uint(0xFFFFFFFF);

		int last = _chunks.size() -1;

		for(int i = 0; i < last; i++)
		{			
			uint64_t current_sample_count = _chunks[i].samples;
			
			if(0 == i)
			{
				chunk_start = i + 1;
				sample_count = static_cast<uint32_t>(current_sample_count);
			}

			if(current_sample_count != sample_count)
			{
				_ASSERTE(!_chunks[i].continuity);
				_ASSERTE(chunk_start > 0);
				_ASSERTE(sample_count > 0);

				mp4w.write_uint(chunk_start);
				mp4w.write_uint(sample_count);
				mp4w.write_uint(1);

				entry_count++;

				sample_count = static_cast<uint32_t>(current_sample_count);
				chunk_start = i + 1;
			}
	
		}

		if(1 < _chunks.size())
		{

			mp4w.write_uint(chunk_start);
			mp4w.write_uint(sample_count);
			mp4w.write_uint(1);

			entry_count++;

			_ASSERTE(static_cast<uint32_t>(last) < _chunks.size());

			if(static_cast<uint32_t>(last) >= _chunks.size())
			{
				Cstring err(_T("write_stsc chunks invalid number: chunks ["));
						err += _chunks.size();
						err += _T("] last [");
						err += last;
						err += _T("]\r\n");

				THROW_MP4EX(err, E_UNKNOWN);
			}
		}

		if(_chunks.size() || !_allow_empty_stream)
		{
			mp4w.write_uint(last + 1);
			mp4w.write_uint(static_cast<uint32_t>(_chunks[last].samples));
			mp4w.write_uint(1);

			entry_count++;
		}
		
		mp4w.pop_pin();
		mp4w.write_uint(entry_count);
		mp4w.un_pin();

		mp4w.close_box(box_stsc);
	}
	virtual void write_stsz(CMP4W &mp4w)
	{
		//if(!_samples.size() && _allow_empty_stream && !_has_size)
		//	return;

		uint32_t sample_size(0);
		
		FullBox stsz;

		mp4w.open_box(box_stsz);
		mp4w.write_box(stsz);

		//if(_has_size)
			mp4w.write_uint(0x00);	
		//else
		//	mp4w.write_uint(_samples[0].size);

		mp4w.write_uint(_samples.size());

		if(_has_size)
		{
			for(uint32_t i = 0; i < _samples.size(); i++)
			{
				mp4w.write_uint(static_cast<uint32_t>(_samples[i].size));
			}
		}

		mp4w.close_box(box_stsz);
	}


	virtual void write_edts(CMP4W &mp4w)
	{
		if(!_stream_offset)
			return;

		EditListBox elst;
		            elst.add_item(
						  time_scale(_stream_offset, _mvhd_time_scale)
						, -1
						,  1
						,  0);

					elst.add_item(
						  time_scale(get_duration(), _mvhd_time_scale)
						,  0
						,  1
						,  0);	


		mp4w.open_box(box_edts);
		mp4w.open_box(box_elst);
		
		mp4w.write_box(elst);

		mp4w.close_box(box_elst);
		mp4w.close_box(box_edts);

		//_ASSERTE(!_stream_offset);
	}

	virtual void write_open_trak_box(CMP4W &mp4w){mp4w.open_box(box_trak);}
	virtual void write_close_trak_box(CMP4W &mp4w){mp4w.close_box(box_trak);}

public:
	virtual bool IsLTC() const {return false;}
	virtual void write_stco_body(CMP4W &mp4w, uint64_t mdat_offset)
	{
		if(mp4w.get_position() != _stco_position)
			mp4w.set_position(_stco_position);

		for(uint32_t i = 0; i < _chunks.size(); i++)
		{
			mp4w.write_uint(static_cast<uint32_t>(_chunks[i].position + mdat_offset));
		}
	}	
protected:
	
	uint64_t get_chunks_size() const {return _chunks.size();}
	virtual void write_stco(CMP4W &mp4w)
	{		
				
		FullBox stco;

		mp4w.open_box(box_stco);
		mp4w.write_box(stco);

		mp4w.write_uint(_chunks.size());

		_stco_position = mp4w.get_position();

				write_stco_body(mp4w, 0);

		mp4w.close_box(box_stco);
	}


	virtual void write_stss(CMP4W &mp4w)
	{
		if(!_iframes.size())
			return;	   
		
		FullBox stss;

		mp4w.open_box(box_stss);
		mp4w.write_box(stss);

		mp4w.write_uint(_iframes.size());

		for(uint32_t i = 0; i < _iframes.size(); i++)
		{
			_ASSERTE(i > 0 || _iframes[i] == 0);

			mp4w.write_uint(static_cast<uint32_t>(_iframes[i] + 1));
		}

		mp4w.close_box(box_stss);
	}
	
	virtual void write_stbl(CMP4W &mp4w)
	{
         mp4w.open_box(box_stbl);

		    write_stsd(mp4w);
			write_stts(mp4w);
			write_ctts(mp4w);
			write_stsc(mp4w);
			write_stsz(mp4w);
			write_stco(mp4w);
			write_stss(mp4w);
		 
		 mp4w.close_box(box_stbl);
	}

	virtual uint64_t sample_written(
		  const media_sample & ms
		, uint64_t size
		, uint64_t position)
	{
        if(_last_sample_end == UINT64_MAX)
		{
		   _last_sample_end  = position;
		   _last_chunk_start = position;
		   _sample_in_chunk  = 0;
		   _last_sample_chunk_count = 0;

		}

		if(_last_sample_end == position) //same chunk
		{
			_sample_in_chunk++;
		}
		else
		{
			register_chunk();
			_sample_in_chunk = 1;
			_last_chunk_start = position;
		}

		_last_sample_end = position + size;

		if(!_has_composition)
		    _has_composition = (ms.composition_time != ms.decoding_time);

		if(!_has_size && 0 < _samples.size())
			_has_size = (size != _samples[_samples.size() - 1].size);

		

		uint64_t decoding = ms.decoding_time;
		
#ifdef _DEBUG
		if(_auto_decoding_time)
		{
			decoding = _samples.size();
		}
		else
		{
			_ASSERTE(ms.composition_time >= ms.decoding_time);
		}
#endif

		sample_des time = { static_cast<uint64_t>(ms.composition_time), decoding, size};

#ifdef _DEBUG
		if(!IsLTC())
		{
			if(!_auto_decoding_time)
			{
				if(_samples.size())
				{
					_ASSERTE(_samples[_samples.size()-1].decoding
						< decoding);
					/*
					_ASSERTE(MAX_DECODING_DISTANCE >
						(decoding - _samples[_samples.size()-1].decoding)
						);
					*/
					if(_samples[_samples.size()-1].decoding
						>= decoding)
					{
						DBGC5(_T("INVALID DECODING SEQUENCE\t%s\t%s\t%s\t%d\t%s\r\n")
							, HNS(ms.composition_time)
							, HNS(ms.decoding_time)
							, HNS(ms.duration)
							, _stream_id
							, HNS(_samples[_samples.size()-1].decoding)
							);

					}

					if(MAX_DECODING_DISTANCE <=
						(decoding - _samples[_samples.size()-1].decoding)
					)
					{
						DBGC5(_T("INVALID DISTANCE SEQUENCE\t%s\t%s\t%s\t%d\t%s\r\n")
							, HNS(ms.composition_time)
							, HNS(ms.decoding_time)
							, HNS(ms.duration)
							, _stream_id
							, HNS(_samples[_samples.size()-1].decoding)
							);

					}


				}
			}
		}
#endif
		
		_samples.push_back(time);

		if(_sample_max < time.composition)
			_sample_max = time.composition;

		if(_sample_min > time.composition)
			_sample_min = time.composition;


		if(_auto_decoding_time)
			_auto_decoding_vector.push_back(ms.composition_time);

		if(IsVisual())
		{
			if(ms.bIsSyncPoint)
				_iframes.push_back(_samples.size() - 1);
		}

		_chunk_flushed = false;

		return ms.composition_time;
	}

	CMP4Stream():
	        _last_sample_end(UINT64_MAX)
		  , _last_sample_chunk_count(UINT64_MAX)
		  , _has_composition(false)
		  , _has_size(false)
		  , _stco_position(0)
		  , _sample_in_chunk(0)
		  , _chunk_flushed(true)
		  , _time_scale(1000ULL)
		  , _stream_offset(0)
		  , _ctts_offset(0)
		  , _stts_offset(0)
		  , _auto_decoding_time(false)
		  , _sample_max(0)
	      , _sample_min(UINT64_MAX)
		  , _allow_empty_stream(false)
		  , _mvhd_time_scale(1000ULL)
#ifdef CENC
	      , _has_cenc_id(false)
#endif
	  {
		  _lang[0] = L'u';
		  _lang[1] = L'n';
		  _lang[2] = L'd';
		  _lang[3] = L'\0';

		  _sample_queue.allocate();
	  }
public:

	void set_allow_empty_stream(bool rhs)
	{
		_allow_empty_stream = rhs;
	}

	void set_lang(const TCHAR * lang)
	{
		_ASSERTE(2 < _tcslen(lang));

		_lang[0] = lang[0];
		_lang[1] = lang[1];
		_lang[2] = lang[2];
	}


	virtual void queue_sample(
		  const BYTE * body
		, const uint32_t body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		)
	{
		media_sample ms(composition_time, decoding_time, 0, false, IFrame);
		_sample_queue.push(ms, body, body_size);
	}

	virtual uint64_t write_samples(uint64_t max, CMP4W & mp4w, bool & is_over, bool use_composition = true)
	{
		uint64_t last_time = NOWRITE;
		BYTE * p_body(NULL);
		int body_size(0);
		while( _sample_queue.size()
			&& 
			   ( 
			 		(_sample_queue.pick_next(&body_size, &p_body)->composition_time <= static_cast<int64_t>(max) && use_composition)
					||
					(_sample_queue.pick_next(&body_size, &p_body)->decoding_time <= static_cast<int64_t>(max) && (!use_composition))
			   )
		)
		{
			uint64_t sample_position = mp4w.get_position();
			mp4w.write_bytes(p_body, body_size);

			last_time = sample_written(
				*_sample_queue.pick_next(&body_size, &p_body)
				, body_size
				, sample_position);

			_sample_queue.delete_item();
		}

		is_over = false;

		if( _sample_queue.size() && NOWRITE == last_time /* && _sample_queue.pick_next(&body_size, &p_body)->composition_time > max */)
			is_over = true;

		return last_time;
	}

	
	uint64_t get_start_time() const
	{
		if(_samples.size())
			return _samples[0].composition;

		return UINT64_MAX;
	}

	uint64_t get_decoding_start_time() const
	{
		if(_samples.size())
			return _samples[0].decoding;

		return UINT64_MAX;
	}

	uint64_t get_duration() const
	{
		if(_samples.size() )
			return _samples[_samples.size() - 1].composition
			     - _samples[0].composition;

		if(_allow_empty_stream)
			return 0;

		return UINT64_MAX;
	}

	uint64_t get_frame_duration() const
	{
		if(_samples.size() )
			return get_duration() / (_samples.size() - 1);

		return UINT64_MAX;
	}

	uint64_t get_last() const
	{
		if(_samples.size() )
			return _samples[_samples.size() - 1].composition;

		return UINT64_MAX;
	}

	uint64_t get_sample_max() const
	{
		return _sample_max;
	}

	uint64_t get_sample_min() const
	{
		return _sample_min;
	}



	void set_stream_id(int id){_stream_id = id;}
	int  get_stream_id(){return _stream_id;}

	void set_time_scale(const uint64_t time_scale){_time_scale = time_scale;}
	void set_mvhd_time_scale(const uint64_t mvhd_time_scale){_mvhd_time_scale = mvhd_time_scale;}

	void set_stream_offset(const uint64_t stream_offset)
	{
		_stream_offset = stream_offset;
	}
    void set_ctts_offset(const uint64_t ctts_offset_hnano)
	{
		_ctts_offset = ctts_offset_hnano;
	}
	uint64_t get_ctts_offset()
	{
		return _ctts_offset;
	}

	int get_sample_count()
	{
		return _samples.size();
	}
	
	//void set_stts_offset(const uint64_t stts_offset_hnano)
	//{
	//	_stts_offset = time_scale(stts_offset_hnano);
	//}
	void write_stream(CMP4W &mp4w)
	{
		//mp4w.open_box(box_trak);
		write_open_trak_box(mp4w);

		TrackHeaderBox box;

		box.set_flags(0x000001);

		box.set_track_ID(get_stream_id() + 1);
		box.set_creation_time(MP4NOW());
		box.set_modification_time(MP4NOW());
		
		//box.set_duration(get_duration() / 10000ULL);

		box.set_duration(time_scale(get_duration(), _mvhd_time_scale) + time_scale(_stream_offset, _mvhd_time_scale));

		fill_track_header(box);

		mp4w.write_child_box(box_tkhd, box);

		write_edts(mp4w);

			mp4w.open_box(box_mdia);

			     MediaHeaderBox mdhd;

				 mdhd.set_creation_time(MP4NOW());
		         mdhd.set_modification_time(MP4NOW());
				 fill_media_header(mdhd);

				 mp4w.write_child_box(box_mdhd, mdhd);

				 HandlerBox hdlr;
				 fill_media_handler(hdlr);
				 mp4w.write_child_box(box_hdlr, hdlr);

				       mp4w.open_box(box_minf);

					      write_media_header(mp4w);//vmhd|smhd

							  mp4w.open_box(box_dinf);
								DataReferenceBox dref;dref.entry_count=1;

								//mp4w.write_child_box(box_dref, dref);
								mp4w.open_box(box_dref);
								mp4w.write_box(dref);

								   FullBox fb;
								           fb.set_flags(0x000001);//data in the file
									   mp4w.write_child_box(box_url, fb);
							     mp4w.close_box(box_dref);
							  mp4w.close_box(box_dinf);

							  write_stbl(mp4w);

					   mp4w.close_box(box_minf);

			mp4w.close_box(box_mdia);

		//mp4w.close_box(box_trak);
		write_close_trak_box(mp4w);
	}

	void flush()
	{
		if(!_chunk_flushed)
		    register_chunk();
	}


	void set_auto_decoding_time(bool rhs){_auto_decoding_time = rhs;}
	bool get_auto_decoding_time() const {return _auto_decoding_time;}


	static uint64_t get_auto_decoding_duration(std::vector<uint64_t> & auto_decoding_vector, uint64_t composition_time)
	{
		std::vector<uint64_t>::iterator it = 
			std::lower_bound(auto_decoding_vector.begin()
			, auto_decoding_vector.end()
			, composition_time);


		bool found = (
			    it != auto_decoding_vector.end()
			 && composition_time == *it);

		if(!found)
			return 0;

		if(it == auto_decoding_vector.begin())
			return 0;

		//if(it == auto_decoding_vector.begin())
		//{
		//	uint64_t t = *it;
		//	it++;
		//	if(it == auto_decoding_vector.end())
		//		return 0;

		//	return (*it - t);
		//}

		_ASSERTE(it != auto_decoding_vector.begin());
		it--;

#if _DEBUG
		/*********************************************/
		while(*it < composition_time)
		{
			it++;
			if(it == auto_decoding_vector.end())
			{
				it--;
				break;
			}
		}

		if(*it >= composition_time)
			it--;
		/*********************************************/
#endif

		_ASSERTE(*it < composition_time);

		return (composition_time - *it);


	}


private:
//	uint64_t get_auto_decoding_duration(uint64_t composition_time)
//	{
//		std::vector<uint64_t>::iterator it = 
//			std::lower_bound(_auto_decoding_vector.begin()
//			, _auto_decoding_vector.end()
//			, composition_time);
//
//
//		bool found = (
//			    it != _auto_decoding_vector.end()
//			 && composition_time == *it);
//
//		if(!found)
//			return 0;
//
//		if(it == _auto_decoding_vector.begin())
//			return 0;
//
//		_ASSERTE(it != _auto_decoding_vector.begin());
//		it--;
//
//#if _DEBUG
//		/*********************************************/
//		while(*it < composition_time)
//		{
//			it++;
//			if(it == _auto_decoding_vector.end())
//			{
//				it--;
//				break;
//			}
//		}
//
//		if(*it >= composition_time)
//			it--;
//		/*********************************************/
//#endif
//
//		_ASSERTE(*it < composition_time);
//
//		return (composition_time - *it);
//
//
//	}

	uint64_t get_auto_decoding_duration(uint64_t composition_time)
	{
		return get_auto_decoding_duration(_auto_decoding_vector, composition_time);
	}
public:
	void compute_auto_decoding_time()
	{
		_ASSERTE(_auto_decoding_time);
		_ASSERTE(_auto_decoding_vector.size() == _samples.size());

		if(!_auto_decoding_vector.size())
		   return;
		

		   std::sort(_auto_decoding_vector.begin()
					, _auto_decoding_vector.end());

		//uint64_t last(0);
		uint64_t tot(0);

		uint64_t start_decoding_time = _auto_decoding_vector[0];
		uint64_t decoding_offset     = 0;


		tot  = start_decoding_time;
		//last = 0;

		//for(int i = 0; i < _auto_decoding_vector.size(); i++)
		for(uint32_t i = 0; i < _samples.size(); i++)
		{
/*
#ifdef _DEBUG
			_ASSERTE(_samples[i].decoding == i);
#endif	
*/

			uint64_t duration = get_auto_decoding_duration(_samples[i].composition);

			//_ASSERTE(_auto_decoding_vector[i] >= last || 0 == _auto_decoding_vector[i]);
			//tot += _auto_decoding_vector[i] - last;

			tot += duration;

			//if(0 == i)
			//{
			//	_samples[i].decoding = start_decoding_time;
			//}
			//else
			//{

				if(tot > _samples[i].composition)
				{
					if(tot - _samples[i].composition > decoding_offset)
					{
						DBGC5(_T("AUTO DECODING\t%s\t%s\t%s\t%s\t%d\r\n")
							, HNS(_samples[i].composition)
							, HNS(tot)
							, HNS(tot - _samples[i].composition)
							, HNS(decoding_offset)
							, i
							);

						decoding_offset     = tot - _samples[i].composition;
					}
				}
				
				_samples[i].decoding = tot;

			//}

			//last = _auto_decoding_vector[i];

			_ASSERTE(_samples[i].decoding <= ( _samples[i].composition + decoding_offset));

			_ASSERTE(0 == i ||
				_samples[i - 1].decoding <= _samples[i].decoding
				);
		}

		set_ctts_offset(decoding_offset);
	}

	virtual ~CMP4Stream(){}
};

class CMP4VisualStream:public CMP4Stream
{
	CMP4VisualEntry _v;
protected:

#ifdef CENC
	virtual uint32_t get_original_format_type() const {return box_avc1;}
#endif

	virtual void write_stream_entry(CMP4W &mp4w)
	{
#ifdef CENC
		if(has_cenc_id())
		{
			mp4w.open_box(box_encv);
		}
		else
		{
			mp4w.open_box(box_avc1);
		}
#else
		mp4w.open_box(box_avc1);
#endif

		const VisualSampleEntry & e = _v.get_entry(); 
				
		mp4w.write_box(const_cast<VisualSampleEntry &>(e));

#ifdef CENC 
#ifdef FRAGMENTEDSTYPTRUE
         write_protected_sinf(mp4w);
#endif
#endif

		mp4w.open_box(box_avcC);
		   mp4w.write_bytes(_v.get_body(), static_cast<uint32_t>(_v.get_body_size()));
	    mp4w.close_box(box_avcC);


#ifdef CENC
#ifdef FRAGMENTEDSTYPFALSE
		   write_protected_sinf(mp4w);
           mp4w.write_child_box(box_btrt, _v.get_btrt());
#endif
#endif
		

#ifdef CENC
		if(has_cenc_id())
		{
			mp4w.close_box(box_encv);
		}
		else
		{
			mp4w.close_box(box_avc1);
		}
#else
		mp4w.close_box(box_avc1);
#endif
		
	
	}
	virtual bool IsVisual(){return true;}
	
public:
	CMP4VisualStream(const CMP4VisualEntry &e):_v(e)
	{
	}

	uint32_t get_width(){return _v.get_entry().width;}
	uint32_t get_height(){return _v.get_entry().height;}
protected:
	void fill_track_header(TrackHeaderBox &box) 
	{
		box.set_height(get_height());
		box.set_width(get_width());
		box.set_volume(0);
	}
	
};

class CMP4AudioStream:public CMP4Stream
{
	CMP4AudioEntry _a;
protected:

#ifdef CENC
	virtual uint32_t get_original_format_type() const {return box_mp4a;}
#endif

	virtual bool IsVisual(){return false;}
	virtual void write_stream_entry(CMP4W &mp4w)
	{
#ifdef CENC
		if(has_cenc_id())
		{
			mp4w.open_box(box_enca);
		}
		else
		{
			mp4w.open_box(box_mp4a);
		}
#else
		mp4w.open_box(box_mp4a);
#endif
		
		mp4w.write_box(const_cast<AudioSampleEntry &>(_a.get_entry()));
		mp4w.write_bytes(_a.get_body(), static_cast<uint32_t>(_a.get_body_size()));

#ifdef CENC
		if(has_cenc_id())
		{
			write_protected_sinf(mp4w);
			mp4w.close_box(box_enca);
		}
		else
		{
			mp4w.close_box(box_mp4a);
		}
#else
		mp4w.close_box(box_mp4a);
#endif
	}
public:
	CMP4AudioStream(const CMP4AudioEntry &e):_a(e)
	{
	}
protected:
	void fill_track_header(TrackHeaderBox &box) 
	{
		box.set_volume(0x0100);
	}
	//virtual void fill_media_header(MediaHeaderBox &box)
	//{
	//	//box.set_timescale(_a.get_entry().samplerate); //sample * seconds
	//	//box.set_duration(get_duration() / 10000ULL / _a.get_entry().samplerate);
	//}
};


class CMP4LTCStream:public CMP4Stream
{
	timecode_head _head;
	TC _base;
	uint64_t _next;

protected:
	virtual bool IsVisual(){return false;}
	
	virtual void write_stream_entry(CMP4W &mp4w)
	{

		mp4w.open_box(box_uuid);

		Box ltc;

		SET_LTC_HANDLER(ltc);

		mp4w.write_bytes(ltc.guid(), 16);

		mp4w.write_box(_head);

		mp4w.close_box(box_uuid);

	}

	virtual void write_stbl(CMP4W &mp4w)
	{
		if(get_chunks_size())
			CMP4Stream::write_stbl(mp4w);
	}
public:
	virtual bool IsLTC() const {return true;}
	CMP4LTCStream(uint32_t avg_frame_rate = 400000)	
		: _next(0)
	{
		_head.avg_frame_rate = avg_frame_rate;
	}
protected:

#ifdef CENC
	virtual uint32_t get_original_format_type() const {return 0;}
#endif

	virtual void queue_sample(
		  const BYTE * body
		, const uint32_t body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		)
	{
		uint64_t offset = _head.avg_frame_rate;
		
		bool write = true;

		const uint32_t *ptc  = reinterpret_cast<const uint32_t*>(body + sizeof(uint16_t));
		const uint32_t & wtc = *ptc;

		if(_next > offset && (_next - offset) >= composition_time) //avoid duplicated timecode
		{
			_next = composition_time + offset;
			return;
		}
		
		if((_next/10000) == (composition_time/10000))
		{
			if(_base == wtc){

				write = false;

				_base.operator++();
			    _next = composition_time + offset;
			}
		}

		
		if(write){
		
		    CMP4Stream::queue_sample(body, body_size, IFrame, composition_time, decoding_time);

			TC t(static_cast<uint32_t>(wtc));

			_base = t;

			_base.operator++();
			_next = composition_time + offset;
		}
		
	}

	void fill_track_header(TrackHeaderBox &box) 
	{

	}

	virtual void write_open_trak_box(CMP4W &mp4w)
	{
		mp4w.open_box(box_uuid);

		Box ltc;

		SET_LTC_BOX(ltc);

		mp4w.write_bytes(ltc.guid(), 16);

	}

	virtual void write_close_trak_box(CMP4W &mp4w){mp4w.close_box(box_uuid);}
	
};

#define MAX_STREAM 64

class MP4Write
{
#ifdef _DEBUG
public:
#endif
	FileTypeBox    _ftyp;
	MovieHeaderBox _mvhd;

	uint64_t _max_composition_time;
	uint64_t _max_distance;
	uint64_t _max_composition_time_used;
	uint64_t _max_composition_time_increment;
	bool             _use_composition_in_distance;

	int     _first_track_id;

	std::vector<CMP4Stream*> _streams;
	void clean()
	{
		for(uint32_t i = 0; i < _streams.size(); i++)
			delete _streams[i];

		_streams.clear();

	}

	uint64_t min_duration()
	{
		uint64_t min = UINT64_MAX;

		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			if(!_streams[i]->IsLTC())
			{
				uint64_t t = _streams[i]->get_duration();
				if(t < min)
					min = t;
			}
		}

		return min;
	}

	uint64_t min_stream()
	{
		uint64_t min = UINT64_MAX - _max_distance;

		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			if(!_streams[i]->IsLTC())
			{
				if(_streams[i]->get_sample_count())
				{
					uint64_t t = _streams[i]->get_sample_max();//_streams[i]->get_max();
					if(t < min)
						min = t;
				}
			}
		}

		return min;
	}

	uint32_t min_stream_start_number()
	{
		uint64_t min = UINT64_MAX;
		uint32_t   n = UINT32_MAX;

		if(!_streams.size())
			THROW_MP4EX(_T("no streams availables."), E_UNKNOWN);

		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			uint64_t t =  _streams[i]->get_start_time();
			
			if(t < min)
			{
				min = t;
				n = i;
			}
		}

		//return min;

		if(UINT32_MAX == n)
			THROW_MP4EX(_T("no streams found!"), E_UNKNOWN);


		return n;
	}


	uint64_t max_stream_ctts_offset()
	{
		uint64_t max = 0;
		
		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			uint64_t t =  _streams[i]->get_ctts_offset();
			
			if(t > max)
			{
				max = t;
			}
		}

		return max;
	}


	//void set_ctts_offset(const int stream_id, const uint64_t ctts_offset_hnano)
	//{
	//	_ASSERTE(stream_id < _streams.size());
	//	_streams[stream_id]->set_ctts_offset(ctts_offset_hnano);
	//}

	//virtual void rebase_composition_time()
	//{
	//	uint64_t s_min = min_stream_start();

	//	if(s_min > 0)
	//	{
	//		for(int i = 0; i < _streams.size(); i++)
	//		{
	//			uint64_t diff = _streams[i]->get_start_time() - s_min;

	//			if(diff)
	//				_streams[i]->set_ctts_offset(diff);
	//		}
	//	}
	//}


public:

	MP4Write(const uint64_t max_distance = 6000000llu):
	  _max_distance(max_distance)
	, _max_composition_time(UINT64_MAX - _max_distance)
	, _max_composition_time_used(0)
	, _max_composition_time_increment(100000)
	, _use_composition_in_distance(true)
	, _first_track_id(0)
	//, _mvhd.
	{

		_mvhd.set_version(0);
		_mvhd.set_timescale(1000);

		_max_composition_time = UINT64_MAX - _max_distance;
//#ifdef _CONSOLE
//			fprintf(stdout, "MP4Write %S\t%I64u\t%I64u\t%I64u\n\r"
//				, HNS(_max_composition_time)
//				, _max_composition_time
//				, UINT64_MAX
//				, _max_distance
//				);
//#endif
		_ftyp.set_major_brand(ftyp_isom); 
		_ftyp.set_minor_version(512);
        _ftyp.add_brand(ftyp_isom); 
		_ftyp.add_brand(ftyp_iso2);
		_ftyp.add_brand(ftyp_avc1);
		_ftyp.add_brand(ftyp_mp41);


	}

    void set_ftyp(int major_brand, uint32_t minor_version)
	{
		_ftyp.clear_brands();

		_ftyp.set_major_brand(major_brand); 
		_ftyp.set_minor_version(minor_version);
	}

	void add_brand(int brand)
	{
		_ftyp.add_brand(brand);
	}

	void set_mvhd_version(int version)
	{
		_mvhd.set_version(version);
		
	}

	void set_mvhd_timescale(int time_scale)
	{
		_mvhd.set_timescale(time_scale);

		for(uint32_t i = 0; i < _streams.size(); i++)
			_streams[i]->set_mvhd_time_scale(time_scale);
	}
	
	virtual ~MP4Write(){clean();}

	virtual int add_visual_stream(const CMP4VisualEntry & v
		, const uint64_t time_scale = 0)
	{
		CMP4Stream* ps = new CMP4VisualStream(v);
		
		if(time_scale)
			ps->set_time_scale(time_scale);

		ps->set_mvhd_time_scale(_mvhd.get_timescale());
		
		_streams.push_back(ps);

		ps->set_stream_id(_streams.size() - 1);
		return ps->get_stream_id();
	}

	template<typename T>
	inline static void AVCDecoderConfigurationRecordFromSPSandPPS(
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, T &var_bs
		, uint32_t & width_i
        , uint32_t & height_i
		)
	{
		 H264Sequence sequence;
         H264Nal nal;
		 nal.decode_nal(  sps_nal_size
						, sps_nal_source);

        _ASSERTE(NALTYPE::SEQUENCE == nal.get_decoded_nal_unit_type());

		FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
						  , nal.decoded_rbsp_size() -1
							);

		 sequence.get(mem);

         AVCDecoderConfigurationRecord avcc;
		                               avcc.AVCProfileIndication  = sequence.profile_idc;
									   avcc.profile_compatibility = 
											  sequence.constraint_set0_flag << 7 
										   || sequence.constraint_set1_flag << 6
										   || sequence.constraint_set2_flag << 5
										   || sequence.constraint_set3_flag << 4
										   || sequence.reserved_zero_bits;
									   avcc.AVCLevelIndication = sequence.level_idc;
									   avcc.lengthSizeMinusOne = 3;

									   avcc.set_sps(sps_nal_source
										    , sps_nal_size);

									   avcc.set_pps(pps_nal_source
										    , pps_nal_size);
						


		 width_i  = (16 * (sequence.pic_width_in_mbs_minus1 + 1));
    	 height_i = (16 * (sequence.pic_height_in_map_units_minus1 + 1));

		 avcc.put(var_bs);

		 var_bs.flush();

	}

	virtual int add_visual_stream(
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, const uint64_t time_scale = 0
		, const uint32_t width  = 0
        , const uint32_t height = 0
		)
	{
		 uint32_t width_i  = width;
         uint32_t height_i = height;


		 /*

		 H264Sequence sequence;
         H264Nal nal;
		 nal.decode_nal(  sps_nal_size
						, sps_nal_source);

        _ASSERTE(NALTYPE::SEQUENCE == nal.get_decoded_nal_unit_type());

		MemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
						  , nal.decoded_rbsp_size() -1
							);

		 sequence.get(mem);

         AVCDecoderConfigurationRecord avcc;
		                               avcc.AVCProfileIndication  = sequence.profile_idc;
									   avcc.profile_compatibility = 
											  sequence.constraint_set0_flag << 7 
										   || sequence.constraint_set1_flag << 6
										   || sequence.constraint_set2_flag << 5
										   || sequence.constraint_set3_flag << 4
										   || sequence.reserved_zero_bits;
									   avcc.AVCLevelIndication = sequence.level_idc;
									   avcc.lengthSizeMinusOne = 3;

									   avcc.set_sps(sps_nal_source
										    , sps_nal_size);

									   avcc.set_pps(pps_nal_source
										    , pps_nal_size);
						

		 MemoryBitstream  m2(1024);

		 //m2.putbits(0xFFFFFFFF, 32);//box_size
		 //m2.putbits(box_avcC, 32);
		 //m2.putbits(0, 32);//flags //version
		 
		 avcc.put(m2);

		 m2.flushbits();

		 //m2.set_position(0);

		 //m2.putbits(m2.get_size(), 32);

		 //m2.flushbits();

		 */



		 ////////////////////////////////////////
		 DBGC0("--------WriteMemoryBitstream-------------");
			 
			 WriteMemoryBitstream  m2(1024);
			 uint32_t width_k(0);
			 uint32_t height_k(0);

			 AVCDecoderConfigurationRecordFromSPSandPPS(
				  sps_nal_source
				 , sps_nal_size
				 , pps_nal_source
				 , pps_nal_size
				 , m2
				 , width_k
				 , height_k
			 );


			 m2.flush();


			 //m2.flushbits();
			 ////////////////////////////////////////

			 if (0 == width_i)
				 width_i = width_k;

			 if (0 == height_i)
				 height_i = height_k;


			 CMP4VisualEntry v(
				 width_i
				 , height_i
				 , 72
				 , 72
				 , m2.get_buffer()
				 , static_cast<uint32_t>(m2.get_size()));

		 


		 return add_visual_stream(v, time_scale);
	}


	

	virtual int add_visual_stream(
		  const BYTE*    p_codec_private_data
        , const unsigned codec_private_size
		, const uint64_t time_scale = 0
		)
	{

		unsigned sps_nal_size         = (p_codec_private_data[0] << 8 & 0xFF00) | (p_codec_private_data[1] & 0x00FF);
		const BYTE*    sps_nal_source = &p_codec_private_data[2];
		unsigned pps_nal_size         = (p_codec_private_data[2 + sps_nal_size] << 8 & 0xFF00) | (p_codec_private_data[3 + sps_nal_size] & 0x00FF);
		const BYTE*	 pps_nal_source   = &p_codec_private_data[4 + sps_nal_size];
		 
		return
		add_visual_stream(
		  sps_nal_source
        , sps_nal_size
		, pps_nal_source
        , pps_nal_size
		, time_scale);

		
	}

	virtual int add_audio_stream(const CMP4AudioEntry & a
		, const uint64_t time_scale = 0)
	{
		CMP4Stream* ps = new CMP4AudioStream(a);
		_streams.push_back(ps);

		if(time_scale)
			ps->set_time_scale(time_scale);

		ps->set_mvhd_time_scale(_mvhd.get_timescale());
	
		ps->set_stream_id(_streams.size() -1);
		return ps->get_stream_id();
	}

	virtual int add_audio_stream(
	  const uint32_t object_type
	, const uint32_t sample_rate
    , const uint32_t channels
	, const uint32_t target_bit_rate
	, const uint64_t time_scale = 0)
	{
		aac_info_mp4 aac;
		             aac.object_type = object_type;
		             aac.sample_rate = sample_rate;
		             aac.channels	 = channels;

	   WriteMemoryBitstream  mem(1024);

	   aac.put(mem);

	   mem.flush();

	   ES_Descriptor es;
	   es.set_extension(mem.get_buffer()
                     , static_cast<uint32_t>(mem.get_size())
					 , 0x40//(1 == aac.object_type)?0x40:0x41//, 0x40
					 , 5
					 , 0
					 , 850//, 750
					 , target_bit_rate + 100000//, 206136 
					 , target_bit_rate//, 159991
		      );

	 WriteMemoryBitstream  m2(1024);

	 m2.putbits(0xFFFFFFFF, 32);//box_size
	 m2.putbits(box_esds, 32);
	 m2.putbits(0, 32);//flags //version
	 
	 es.put(m2);

	 m2.set_position(0);

	 uint64_t m2size = m2.get_size();

	 m2.putbits(static_cast<uint32_t>(m2size), 32);

	 m2.flush();

	  CMP4AudioEntry a(sample_rate
					 , channels
					 , m2.get_buffer()
                     , static_cast<uint32_t>(m2.get_size()));

	  return add_audio_stream(a, time_scale);
	}



	virtual int add_audio_stream(
		  const BYTE*    p_codec_private_data
        , const unsigned codec_private_size
		, const uint32_t target_bit_rate
		, const uint64_t time_scale = 0
		)
	{
		aac_info_mp4 aac;
		FixedMemoryBitstream  mem(p_codec_private_data, codec_private_size);

		aac.get(mem);

		return add_audio_stream(aac.object_type, aac.sample_rate, aac.channels, target_bit_rate, time_scale);
	}
	
	virtual void set_lang(const uint32_t stream_id, const TCHAR * lang)
	{
		_ASSERTE(stream_id < _streams.size());
		_streams[stream_id]->set_lang(lang);
	}

	virtual int add_extension_ltc_stream(
		  const uint32_t avg_frame_rate = 400000
		, const uint64_t time_scale = 0)
	{
		CMP4Stream* ps = new CMP4LTCStream(avg_frame_rate);
		_streams.push_back(ps);

		if(time_scale)
			ps->set_time_scale(time_scale);
	
		ps->set_stream_id(_streams.size() -1);
		return ps->get_stream_id();
	}		
	
	virtual void add_sample(uint32_t stream_id
		, const BYTE * body
		, const uint32_t body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, CMP4W & mp4w
		)
	{
		_ASSERTE(stream_id < _streams.size());
		_streams[stream_id]->queue_sample(body, body_size, IFrame, composition_time, decoding_time);
        
		uint64_t w(0);

		bool all_over;

		_max_composition_time_used = _max_composition_time;

		while(w != NOWRITE)
		{

//#ifdef _CONSOLE
//			fprintf(stdout, "max %S\t%I64u\n\r"
//				, HNS(_max_composition_time)
//				, _max_composition_time
//				);
//#endif

			w = NOWRITE;
			
			bool stream_over;
				 all_over = true;

			for(uint32_t i = 0; i < _streams.size(); i++)
			{
				uint64_t result = _streams[i]->write_samples(_max_composition_time_used + _max_distance, mp4w, stream_over, _use_composition_in_distance);
				
				if(result < NOWRITE)
				{ 
				  w = result;
				  _max_composition_time = min_stream();
				  _max_composition_time_used = _max_composition_time;

				}
				else
					all_over &= stream_over;
			}

			if((NOWRITE == w) && all_over)
			{
				
					//_ASSERTE(false);

					//retry
					_max_composition_time_used += _max_composition_time_increment;
					
					w = 0;

					DBGC5(_T("MAX DISTANCE FORCED INCREASE\t%d\t%s\t%s\t%s\t%s")
						, stream_id
						, HNS(composition_time)
						, HNS(decoding_time)
						, HNS(_max_composition_time)
						, HNS(_max_composition_time_used)
						);
				
				
				
			}
		}

	}

	virtual void write_ftyp(CMP4W &mp4w, uint32_t type = box_ftyp)
	{
		mp4w.write_child_box(box_ftyp, _ftyp);
	}
	
	void set_first_track_id(int rhs){_first_track_id = rhs;}
	void set_stream_track_id(int track_id, int stream_id = 0)
	{
		_streams[stream_id]->set_stream_id(track_id);
	}

#ifdef CENC
	void set_stream_key_id(unsigned char * p_key_id, int stream_id = 0)
	{
		_streams[stream_id]->set_key_id(p_key_id);
	}
#endif

	void set_allow_empty_stream(bool rhs, int stream_id = 0)
	{
		_streams[stream_id]->set_allow_empty_stream(rhs);
	}

	virtual void close_moov(CMP4W &mp4w)
	{
		mp4w.close_box(box_moov);
	}

	virtual void write_moov(CMP4W &mp4w, bool close = true)
	{
		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			_streams[i]->flush();
		}

        //rebase_composition_time();

		 mp4w.open_box(box_moov);

		uint64_t time_scale = _mvhd.get_timescale();

		//_mvhd.set_duration(min_duration() / time_scale );
		//_mvhd.set_timescale(1000);

		_mvhd.set_duration(CMP4Stream::time_scale(min_duration(), time_scale));

		_mvhd.set_next_track_ID(_first_track_id + _streams.size() + 1);
		_mvhd.set_creation_time(0);
		_mvhd.set_modification_time(0);
		_mvhd.set_creation_time(MP4NOW());
		_mvhd.set_modification_time(MP4NOW());

		mp4w.write_child_box(box_mvhd, _mvhd);

		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			_streams[i]->write_stream(mp4w);
		}

		if(close)
			close_moov(mp4w);
			
	}

	//virtual void write_streams(CMP4W &mp4w)
	//{		
	//	mp4w.open_box(box_moov);
	//	mp4w.write_child_box(box_mvhd, _mvhd);
	//}

	virtual void rebase_streams(CMP4W &mp4w, uint64_t headers_size)
	{
		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			_streams[i]->write_stco_body(mp4w, headers_size);
		}
	}

	void set_auto_decoding_time(const uint32_t stream_id, const bool rhs)
	{
		_ASSERTE(stream_id < _streams.size());
		_streams[stream_id]->set_auto_decoding_time(rhs);
	}

	virtual void compute_auto_decoding_time()
	{
		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			if(_streams[i]->get_auto_decoding_time())
				_streams[i]->compute_auto_decoding_time();
		}
	}
		
	
	//bool get_auto_decoding_time() const {return _auto_decoding_time;}

    /*
	here we must move stream back and forward in order to have the starting time leveled 
	to the max_ctts arriving from the auto decoding. The ctts_offset is used internally.
	and to have the startime aligned with the offset used in the index
	*/
	virtual void level_start_time(bool use_stream_offset_instead_of_ctts = DEFAULT_NOCTTS)
	{
		uint32_t stream     = min_stream_start_number();
		uint64_t max_ctts   = max_stream_ctts_offset();

		uint64_t start_less = _streams[stream]->get_decoding_start_time();

		uint64_t min_diff = 0;//100000ui64;

		for(uint32_t i = 0; i < _streams.size(); i++)
		{
			uint64_t start  = _streams[i]->get_start_time() - _streams[i]->get_decoding_start_time();
			uint64_t target = _streams[i]->get_start_time();
			uint64_t ctts   = _streams[i]->get_ctts_offset();
			
			//_ASSERTE(start >= target);

			uint64_t offset = 0;
			
			if((target - start_less) > start )
				offset = (target - start_less) - start;

			if(max_ctts > ctts)
				offset += (max_ctts - ctts);

			if(offset)
			{
				if(use_stream_offset_instead_of_ctts)
					_streams[i]->set_stream_offset(offset);
				else
					_streams[i]->set_ctts_offset(offset + ctts);
			}

		}
		
	}

	virtual void open_mdat(CMP4W &mp4w, uint64_t size)
	{
		_ASSERTE(size < UINT32_MAX);
		mp4w.write_uint(static_cast<uint32_t>(size + 8));
		mp4w.write_uint(box_mdat);
	}

	//void set_stts_offset(const int stream_id, const uint64_t stts_offset_hnano)
	//{
	//	_ASSERTE(stream_id < _streams.size());

	//	_streams[stream_id]->set_stts_offset(stts_offset_hnano);
	//}

	virtual void end()
	{
		
	}


	virtual void write_xml_meta(
		  CMP4W &mp4w
		, const TCHAR* pszXml
		, uint32_t handler   = BOX('x','m','l','h')
		, uint32_t fill_size = 0
		)
	{
		uint64_t s = mp4w.get_position();

		XmlBox xmlbox(pszXml);

		FullBox fb;
		
		mp4w.open_box(box_meta);
		mp4w.write_box(fb);
		
			HandlerBox hbox;
			           hbox.handler_type = handler;

					   mp4w.write_child_box(box_hdlr, hbox);
					   mp4w.write_child_box(box_xml , xmlbox);

		   uint64_t e = mp4w.get_position();

		   if(fill_size > 0 && fill_size > (e - s)
			   && fill_size > (e - s) + 8)
		   {
			   uint64_t free = fill_size - (e - s) - 8;
			   mp4w.open_box(box_free);
			   BYTE z = 0;
			      
			   for(uint64_t x = 0; x < free; x++)
					  mp4w.write_bytes(&z, 1);

			   mp4w.close_box(box_free);
		   }

		mp4w.close_box(box_meta);
	}


	void set_max_distance(uint64_t max_distance){_max_distance = max_distance;}
	void set_use_composition_in_distance(bool rhd){_use_composition_in_distance = rhd;}
};

//typedef file_media_bitstream<CMP4> MP4File;
typedef memory_write_media_bitstream<CMP4W> CMP4WriteMemory;
typedef file_media_bitstream<CMP4W, O_CREAT | O_WRONLY | O_TRUNC> CMP4WriteFile;
typedef sync_file_media_bitstream<CMP4W, false> SYNCMP4WriteFile;

typedef base_file_midia_bitstream<CMP4W> BMP4W;

/*
class CMP4WriteMemory: public CMP4W
{
	MemoryBitstream *_p_m;
public:
	CMP4WriteMemory():
	  _p_m(NULL)
	{}

    void Close()
	{
		if(_p_m)
			delete _p_m;
		_p_m = NULL;
	}

	virtual ~CMP4WriteMemory(){Close();}
	
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

	void Open(ULONG size)
	{	   
	   _p_m = new MemoryBitstream(size, 1024);
	   _p_f = _p_m;
	}

	uint64_t get_size() const {return _p_m->get_size();}
	const BYTE * get_buffer() const {return _p_m->get_buffer();}

	void flush()
	{
		_ASSERTE(NULL != _p_m); //do not call flush if the MemoryBitstream was supplied externally
		if(_p_m)
			_p_m->flushbits();
	}
};


class CMP4WriteFile: public CMP4W
{
	FileBitstream * _p_ff;
public:
	CMP4WriteFile():
	  _p_ff(NULL)
	{}
    void Close()
	{
		if(_p_ff)
			delete _p_ff;
		_p_ff = NULL;
	}
	virtual ~CMP4WriteFile(){Close();}

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




__ALX_END_NAMESPACE

