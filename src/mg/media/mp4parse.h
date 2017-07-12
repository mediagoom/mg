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

#include "TBitstream.h"
#include "media_parser.h"
#include "mp4/mp4i_ext.h"

#include "mp4/aac.h"

#include "mp4/ltcextension.h"

#include "mp4/mp4_fragments.h"


#include <map>
#include <vector>
#include <algorithm>
#include <stack>

#include "media_queue.h"

#include "h264nal.h"

#include "WaveParser.h"

//#include <WebBitstream.h>

#define LOOK_UP_SAMPLES 100

#define XVEC

__ALX_BEGIN_NAMESPACE

//#pragma optimize( "g", off )

struct sample_info: public media_sample
{
	uint64_t offset;
	uint64_t size;
	uint64_t sample_number;
};

struct sample_stream_info: public sample_info
{
	unsigned int stream;
};

#define MP4ERROR(msg) ALXTHROW_T(msg)
#define MP4CHECK(box) _ASSERTE(box == mp4.get_box().get_type()); if(box != mp4.get_box().get_type()) MP4ERROR(_T(#box " expected"));
#define MP4CHECK2(box1, box2) _ASSERTE(box1 == mp4.get_box().get_type() || box2 == mp4.get_box().get_type()); if(box1 != mp4.get_box().get_type() && box2 != mp4.get_box().get_type()) MP4ERROR(_T(#box1 " expected or " #box2));
template <typename K, typename T>
class CRangeMap
{
#ifndef XVEC
	std::map<K, T> _map;
#else

	struct CRangeMapData
	{
		K k;
		T t;
	};

	std::vector<CRangeMapData> _vector_data;

#endif

	K _last_start_range;

public:

	void add(const K &k, const T &t)
	{
#ifndef XVEC
#if DEBUG
		std::map<K, T>::const_iterator it = _map.end();
		if(0 < _map.size())
		{
			it--;
			_ASSERTE(it->first < k);
		}
#endif
		_map[k] = t;
#else

		//_ASSERTE(!_vector_data.size() 
		//	|| _vector_data[_vector_data.size() - 1].k < k
		//	);
#if _DEBUG
			if(_vector_data.size() &&  
			   _vector_data[_vector_data.size() - 1].k >= k
			)
			{
				DBGC2(_T("INVALID MAPRANGE k1 %s k2 %s \n\r")
					, HNS(_vector_data[_vector_data.size() - 1].k)
					, HNS(k)
					);
			}
#endif

		CRangeMapData d = {k , t};

		_vector_data.push_back(d);
#endif
	}

	int get(const K &k, T &t) const  
	{
#ifndef XVEC
		_ASSERTE(0 < _map.size());

		std::map<K, T>::const_iterator it = _map.lower_bound(k);

		int ret = 0;

		if(it == _map.end())
		{
			if(it != _map.begin())
			{
				it--;
			}

			ret = +1;
		}
		else if(k != it->first)
		{
			if(it != _map.begin())
			{
				//it--;
			}
		}

		if(k < _map.begin()->first)
			ret = -1;

		const_cast<CRangeMap<K,T>*>(this)->_last_start_range = it->first;
		
		t = it->second;

		return ret;
#else

		_ASSERTE(_vector_data.size());

		if(_vector_data[0].k > k)
		{
			t = _vector_data[0].t;
			const_cast<CRangeMap<K,T>*>(this)->_last_start_range = _vector_data[0].k;
			return -1;
		}

		if(_vector_data[_vector_data.size() - 1].k < k)
		{
			const_cast<CRangeMap<K,T>*>(this)->_last_start_range = _vector_data[_vector_data.size() - 1].k;
			t = _vector_data[_vector_data.size() - 1].t;
			return 1;
		}

		unsigned int idx   = _vector_data.size() / 2;
		unsigned int lower = 0;
		unsigned int upper = _vector_data.size() - 1;

		while(idx < (_vector_data.size())
			    //&& idx > 0
				&& (
				        idx > 0 
						&& (
						_vector_data[idx - 1].k >= k ||
						_vector_data[idx].k < k
						) 
						||
						(
						   _vector_data[idx].k < k
						)
				  )

		)
		{
			if(idx > 0 && _vector_data[idx - 1].k >= k)
				upper = idx - 1;

			if(_vector_data[idx].k < k)
				if(lower == idx)
					lower++;
				else
					lower = idx;

			_ASSERTE(upper >= lower);

			if(upper > lower)
				idx = ((upper - lower) / 2) + lower;
			else
				idx = lower;

		}

		_ASSERTE(_vector_data[idx].k >= k && (0 == idx || _vector_data[idx -1].k < k));
			 
		const_cast<CRangeMap<K,T>*>(this)->_last_start_range = _vector_data[idx].k;

		t = _vector_data[idx].t;

		return 0;
#endif

	}

	
	const K & get_upper_limit() const{return _last_start_range;}

	uint64_t size() const
	{
#ifndef XVEC
		return _map.size();
#else
		return _vector_data.size();
#endif
	}

};


class CMP4SampleManager;

class CMP4
{
	Box              _current_box;
	uint64_t _box_position;
	unsigned int     _last_handler_type;
	FullBox          _current_full_box;

	friend CMP4SampleManager;

protected:
	IBitstream3 *    _p_f;
	CMP4():_p_f(NULL){}


#if _DEBUG
	TCHAR _last_box_type[5];
#else
#ifndef _WIN32
	TCHAR _last_box_type[5];
#endif
#endif

private:
	CMP4(CMP4 &rhs);
	int operator=(CMP4 &rhs);
protected:
	
	void skip(short bits)
	{
		_ASSERTE(NULL != _p_f);
		_p_f->skipbits(bits);
	}
	
//#ifdef MP4INFO
public:
//#else
//protected:
//#endif

	uint64_t get_position()
	{
		_ASSERTE(NULL != _p_f);
		return _p_f->get_position() /*/ 8ULL */;
	}

	void skipbytes(uint64_t bits)
	{
		uint64_t to_do_bits = bits * 8ULL;
		unsigned int max = 	INT16_MAX / 1024;
		while((to_do_bits ) > max )
		{
			skip(max);
			to_do_bits -= max;
		}
		
		skip(static_cast<short>(to_do_bits));
	}

	void set_position(uint64_t position)
	{
		_ASSERTE(NULL != _p_f);
		return _p_f->set_position(position);
	}

	void parse_box(Box &box)
	{

		box.get(*_p_f);
#if _DEBUG
		I2C(box.get_type(), _last_box_type);	
#endif
	}

	void parse_movie_header_box(MovieHeaderBox &box)
	{
		_ASSERTE(ALX::Equals(_T("mvhd"), &_last_box_type[0]));
		box.get(*_p_f);
	}

	void parse_track_header_box(TrackHeaderBox &box)
	{
		_ASSERTE(ALX::Equals(_T("tkhd"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_media_header_box(MediaHeaderBox &box)
	{
		_ASSERTE(ALX::Equals(_T("mdhd"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_handler_box(HandlerBox &box)
	{
		_ASSERTE(ALX::Equals(_T("hdlr"), _last_box_type));
		box.get(*_p_f);

		_last_handler_type = box.handler_type;
	}

	void parse_data_reference_box(DataReferenceBox &box)
	{
		_ASSERTE(ALX::Equals(_T("dref"), _last_box_type));
		box.get(*_p_f);

		_ASSERTE(1 == box.entry_count);

		Box b;
		b.get(*_p_f);
		_ASSERTE(ALX::Equals(_T("url "), b.get_type_string()));

		FullBox fb;
		fb.get(*_p_f);

		_ASSERTE(1 == fb.get_flags());//self contained mp4
	}

	void parse_sample_description_box(SampleDescriptionBox &box)
	{
		_ASSERTE(ALX::Equals(_T("stsd"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_visual_sample_entry(VisualSampleEntry &box)
	{
		_ASSERTE(ALX::Equals(_T("avc1"), _last_box_type)
			|| ALX::Equals(_T("encv"), _last_box_type)
			);
		box.get(*_p_f);
	}

	void parse_audio_sample_entry(AudioSampleEntry &box)
	{
		_ASSERTE(ALX::Equals(_T("mp4a"), _last_box_type)
			|| ALX::Equals(_T("enca"), _last_box_type)
			);
		box.get(*_p_f);
	}

	void parse_avc_decoder_configuration_record(AVCDecoderConfigurationRecord & box)
	{
		_ASSERTE(ALX::Equals(_T("avcC"), _last_box_type) || ALX::Equals(_T("avcn"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_es_configuration_record(ES_Descriptor & box)
	{
		_ASSERTE(ALX::Equals(_T("esds"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_bitrate_box(BitRateBox & box)
	{
		_ASSERTE(ALX::Equals(_T("btrt"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_time_to_sample_box(TimeToSampleBox &box)
	{
		_ASSERTE(ALX::Equals(_T("stts"), _last_box_type)
			  || ALX::Equals(_T("ctts"), _last_box_type)
			  || ALX::Equals(_T("stss"), _last_box_type)
			);
		box.get(*_p_f);
	}

	void parse_data_box(data_box &box)
	{
		_ASSERTE(ALX::Equals(_T("data"), _last_box_type)
			);
		box.get(*_p_f);
	}

	void parse_ilst_box(ilst_box &box)
	{
		_ASSERTE(ALX::Equals(_T("ilst"), _last_box_type)
			);
		box.get(*_p_f);
	}

	void parse_edl_box(EditListBox &box)
	{
		_ASSERTE(ALX::Equals(_T("elst"), _last_box_type));
		box.get(*_p_f);
	}

	void parse_file_type_box(FileTypeBox &box)
	{
        _ASSERTE(ALX::Equals(_T("ftyp"), _last_box_type) || ALX::Equals(_T("styp"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_sound_header_box(SoundMediaHeaderBox &box)
	{
		 _ASSERTE(ALX::Equals(_T("smhd"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_video_header_box(VideoMediaHeaderBox &box)
	{
		 _ASSERTE(ALX::Equals(_T("vmhd"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_iods_box(InitialObjectDescriptor &box)
	{
        _ASSERTE(ALX::Equals(_T("iods"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_xml_box(XmlBox &box)
	{
        _ASSERTE(ALX::Equals(_T("xml "), _last_box_type));
		box.get(*_p_f);
	}
	void parse_clap_box(CleanApertureBox &box)
	{
        box.get(*_p_f);
	}
	void parse_pasp_box(PixelAspectRatioBox &box)
	{
        box.get(*_p_f);
	}
	void parse_movie_fragment_header_box(MovieFragmentHeaderBox &box)
	{
        _ASSERTE(ALX::Equals(_T("mfhd"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_track_fragment_header_box(TrackFragmentHeaderBox &box)
	{
        _ASSERTE(ALX::Equals(_T("tfhd"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_track_run_box(TrackRunBox &box)
	{
        _ASSERTE(ALX::Equals(_T("trun"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_movie_extends_header_box(MovieExtendsHeaderBox &box)
	{
        _ASSERTE(ALX::Equals(_T("mehd"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_track_extends_header_box(TrackExtendsBox &box)
	{
		_ASSERTE(ALX::Equals(_T("trex"), _last_box_type));
		box.get(*_p_f);
	}
	void parse_track_fragment_random_access_box(TrackFragmentRandomAccessBox &box)
	{
        _ASSERTE(ALX::Equals(_T("tfra"), _last_box_type));
		box.get(*_p_f);
	}

	unsigned int read_uint(unsigned int bits = 32)
	{
		_ASSERTE(32 >= bits);
		return _p_f->getbits(bits);
	}

	void read_bytes(BYTE * pbyte, const uint64_t size)
	{
		/*
		for(uint64_t i = 0; i < size; i++)
		{
			pbyte[i] = _p_f->sgetbits(8);
		}
		*/
        
		uint32_t cb_read(0);
		uint32_t s = static_cast<uint32_t>(size);
		_p_f->read(static_cast<unsigned char*>(pbyte), s, &cb_read);
		

		_ASSERTE(cb_read == s);
		
	}


	Cstring parse_char(unsigned int size)
	{
		unsigned int tsize = sizeof(TCHAR);
		
		CResource<CBuffer<TCHAR> >  b(new CBuffer<TCHAR>(size + 1));
		

		for(unsigned int i = 0; i < size; i++)
		{
			TCHAR c = _p_f->sgetbits(8);
			b->add(&c, 1);
		}

		Cstring t(b);

		return t;
	}

public:
	
	bool do_box()
	{	
		if(_p_f->eof())
			return false;

		_box_position = get_position();
		parse_box(_current_box);
		

		//if(_box_position + _current_box.size < 

		return true;
	}

	bool move_box_start()
	{
		set_position(_box_position);

		return true;
	}

	bool do_full_box()
	{
		//bool ret = do_box();
		_current_full_box.get(*_p_f);

		return true;
	}

	void skip_current()
	{
		_ASSERTE(_current_box.get_size() > 8);
		skipbytes(_current_box.get_size() - 8);
	}

	const Box & get_box() const {return _current_box;}
	const FullBox & get_full_box() const {return _current_full_box;}
	const uint64_t get_box_position() const {return _box_position;}
	const uint64_t get_box_position_end() const {return _box_position + _current_box.get_size();}

	const unsigned int get_last_handler_type() const{ return _last_handler_type;}
	
	bool eof() const {return _p_f->eof();}

	template<typename BOX> int read_box(BOX &box)
	{
		return box.get(*_p_f);
	};

	template<typename BOX, typename PARAM> int read_box(BOX &box, PARAM param)
	{
		return box.get(*_p_f, param);
	};


};


typedef file_media_bitstream<CMP4> MP4File;
//typedef sync_file_media_bitstream<CMP4> MP4File;
typedef sync_file_media_bitstream<CMP4> SYNCMP4File;

class CMP4SampleManager
{
	bool _has_stsc;//sample size
	bool _has_stsz;//chunk offset
	bool _has_stco;//sample offset
	

	unsigned int        _sample_size;
    unsigned int        _sample_count;
	uint64_t   *_p_psample_size;
	unsigned int        _chunk_count;
	uint64_t   *_p_chunk_offset;
	uint64_t   _total_size;

	struct Chunk {
		unsigned int     samples_per_chunk; 
		unsigned int     sample_description_index;
		uint64_t first_sample;
		uint64_t first_chunk;
	};

	CRangeMap<uint64_t, Chunk> _sample_chunk;
public:
    //stsc 244067 2800 246867
    //stsz 246867 61320 308187
    //stco 308187 6376 314563

	CMP4SampleManager():
	  _has_stsc(false)
	, _has_stsz(false)
	, _has_stco(false)
	, _sample_count(0) //default
	, _sample_size(0)
	, _p_psample_size(NULL)
	, _p_chunk_offset(NULL)
	, _chunk_count(0)
	, _total_size(0)
	  {
	  }

   virtual ~CMP4SampleManager()
   {
	   if(_p_psample_size)
		   delete[] _p_psample_size;

	   _p_psample_size = NULL;

	   if(_p_chunk_offset)
		   delete[] _p_chunk_offset;

	   _p_chunk_offset = NULL;
   }

   void parse_stsc(CMP4 &mp4)
   {
       _ASSERTE(box_stsc == mp4.get_box().get_type());
	   _ASSERTE(!_has_stsc);

	   mp4.do_full_box();	   

       unsigned int entry_count = mp4.read_uint();
       unsigned int previus_first_chunk = 0;
	   unsigned int previus_samples_per_chunk = 0;
       uint64_t samples = 0;

	   unsigned int first_chunk              = 0;
	   unsigned int samples_per_chunk        = 0;
	   unsigned int sample_description_index = 0;
	   //Chunk chunk;
      
       for(unsigned int i = 0; i < entry_count; i++)
       {	
		   Chunk chunk = 
		   {
			     samples_per_chunk
			   , sample_description_index
		       , samples //+ 1
		       , first_chunk
		   };

	       first_chunk              = mp4.read_uint();
	       samples_per_chunk        = mp4.read_uint();
	       sample_description_index = mp4.read_uint();

	       _ASSERTE(i == 0 && first_chunk == 1 || i > 0);
     
		  //chunk = chunkt;

		   _ASSERTE(
			   (0 < (first_chunk - previus_first_chunk) * previus_samples_per_chunk)
				|| (0 == previus_samples_per_chunk && i == 0)
				);
  
		   if(0 < i)
		   {
		     samples += (first_chunk - previus_first_chunk) * previus_samples_per_chunk;
			 
			 if(0 < chunk.samples_per_chunk)
				_sample_chunk.add(samples, chunk);
		   }
	       
		   previus_first_chunk       = first_chunk;
		   previus_samples_per_chunk = samples_per_chunk;

       }

	   Chunk chunk = {
			     samples_per_chunk
			   , sample_description_index
		       , samples
		       , first_chunk};

	   samples += samples_per_chunk;

	   if(0 < chunk.samples_per_chunk)
	      _sample_chunk.add(samples, chunk);

	   _has_stsc = true;
   }

   int get_chunk_number(
	     const uint64_t sample_number
	   , uint64_t &chunk_number
	   , uint64_t &remains
	   , uint64_t &sample_description_index)  
   {
	   _ASSERTE(sample_number >= 0);
	   _ASSERTE(_has_stsc);

	   Chunk chunk; 
	   int ret = _sample_chunk.get(sample_number, chunk);
	   
	   _ASSERTE(chunk.first_sample <= sample_number);

	   uint64_t samples = (sample_number) - chunk.first_sample;
	
	   uint64_t chunkn = (samples) / chunk.samples_per_chunk;

	   remains = samples % chunk.samples_per_chunk;
       sample_description_index = chunk.sample_description_index;

	   //if(chunkn > 0 && 0 == remains)
	   //   chunkn--;

	   chunk_number = chunkn + chunk.first_chunk;
		   //( (samples < chunk.samples_per_chunk)?chunk.first_chunk:(chunk.first_chunk-1) );
        
		return ret;
   }

   uint64_t get_chunk_number(
	     const uint64_t sample_number
	   ) 
   {
	   uint64_t chunk_number;
	   uint64_t remains;
	   uint64_t sample_description_index;

	   get_chunk_number(sample_number
		   , chunk_number
	       , remains
		   , sample_description_index);

	   return chunk_number;
   
   }

   void parse_stsz(CMP4 &mp4)
   {
	   _ASSERTE(box_stsz == mp4.get_box().get_type());
	   _ASSERTE(!_has_stsz);

	   mp4.do_full_box();

	   _sample_size  = mp4.read_uint();
	   _sample_count = mp4.read_uint();

	   if(0 == _sample_size)
	   {
		   _total_size     = 0;
		   _p_psample_size = new uint64_t[_sample_count];

		   for(unsigned int i = 0; i < _sample_count; i++)
		   {
			   _p_psample_size[i] = mp4.read_uint();
			   _total_size       += _p_psample_size[i];
		   }
	   }
	   else
	   {
		   _total_size = _sample_size * _sample_count;
	   }
	   
	   _has_stsz = true;
   }

   uint64_t get_sample_size (const uint64_t sample_number) const
   {
	   _ASSERTE(sample_number >= 0);
	   _ASSERTE(_has_stsz);
	   _ASSERTE(sample_number < _sample_count);

	   if(0 == _sample_size)
	   {
		   return _p_psample_size[sample_number];//sample number 1 based
	   }
	   
	   return _sample_size;
   }

   unsigned int get_sample_count() const {_ASSERTE(_has_stsz || _sample_count == 0); return _sample_count;}

   void parse_stco(CMP4 &mp4)
   {
	   _ASSERTE(box_stco == mp4.get_box().get_type());
	   _ASSERTE(!_has_stco);

	   mp4.do_full_box();

       _chunk_count = mp4.read_uint();

	   _p_chunk_offset = new uint64_t[_chunk_count];

	   for(unsigned int i = 0; i < _chunk_count; i++)
	   {
	         _p_chunk_offset[i] = mp4.read_uint();
	   }

	   _has_stco = true;
   }

   uint64_t get_chunk_offset(const unsigned int chunk_number) const
   {
	   _ASSERTE(_has_stco);
	   _ASSERTE(chunk_number > 0);
	   _ASSERTE(chunk_number <= _chunk_count);

       return _p_chunk_offset[chunk_number - 1];
   }

   uint64_t get_sample_offset(const uint64_t sample_number
	   , uint64_t &sample_description_index)
   {
	   _ASSERTE(sample_number >= 0);
	   _ASSERTE(_has_stsz);
	   _ASSERTE(_has_stco);
	   _ASSERTE(_has_stsc);
	   _ASSERTE(sample_number < _sample_count);

	   uint64_t chunk_number;
	   uint64_t remains;
	   
	   get_chunk_number(sample_number
		   , chunk_number
	       , remains
		   , sample_description_index);

	   uint64_t chunk_offset = get_chunk_offset(static_cast<uint32_t>(chunk_number));

	   for(uint64_t i = 0; i < remains; i++)
	   {
		   chunk_offset += get_sample_size(sample_number - (remains) + i);
	   }
		
	   return chunk_offset;

   }

   uint64_t get_sample_offset(const uint64_t sample_number)
   {
	  uint64_t sample_description_index;

	  uint64_t sample_offset = get_sample_offset(sample_number, sample_description_index);

	  _ASSERTE(1 == sample_description_index);

	  return sample_offset;

   }

   uint64_t get_total_size() const {return _total_size;}
   
};

class CMP4TimeManager
{
	bool _has_stts;//time to sample
	bool _has_ctts;//composition time to sample
	bool _has_stss;//sync sample table (I-FRAMES)

	bool _has_elst;//edit list

	uint64_t _sample_count;

	unsigned int     _ctts_entry_count; //number of entry in a ctts box
	uint64_t _ctts_stream_offset;      //if there is only one entry in the ctts box is equivalent to a stream offset

	struct decoding {
		uint64_t samples;
		uint64_t duration;
		unsigned int sample_delta;
	};

	struct composition {
		uint64_t decoding_time;
		uint64_t composition_offset;
		uint64_t duration;
	};

	struct composition_sample_number 
	{
		uint64_t composition_time;
		uint64_t sample_number;

		composition_sample_number operator=(const composition_sample_number rhs)
		{
			composition_time = rhs.composition_time;
			sample_number    = rhs.sample_number;

			return *this;
		}
	};

	
	struct composition_sample_number_comparer {
       bool operator() (composition_sample_number a, composition_sample_number b) 
	   {return a.composition_time < b.composition_time;}
    } comp_sample_comp;
    

	CRangeMap<uint64_t, decoding>         _time_2decoding_sample;
	CRangeMap<uint64_t, uint64_t> _sample_2I_frame;

	std::vector<composition> * _p_sample_2composition;

	//ordered composition vector
	std::vector<composition_sample_number> _composition_2sample;

	EditListBox _elst;
	uint64_t _stream_offset;

	//uint64_t _max_composition_offset;

    void allocate_sample_2composition()
	{
		if(!_p_sample_2composition)
			_p_sample_2composition = new std::vector<composition>();//(_sample_count);
	}

	void add_ordered(std::stack<composition_sample_number> & stack
			, std::vector<composition_sample_number> &comp
			, composition_sample_number n)
		{
			_ASSERTE(stack.empty());

			//a.composition_time < b.composition_time

			unsigned int idx = comp.size();

			if(0 == idx) //first element
			{
				comp.push_back(n);
				return;
			}

			while(n.composition_time < comp[idx - 1].composition_time)
			{
				stack.push(comp[--idx]);
			}

			if(stack.empty()) //insert at the end
			{
				comp.push_back(n);
				return;
			}

			comp[idx] = n;

			while(++idx < comp.size())
			{
				_ASSERTE(!stack.empty());
				comp[idx] = stack.top();
				stack.pop();
			}

			_ASSERTE(!stack.empty());
			comp.push_back(stack.top());
			stack.pop();

			_ASSERTE(stack.empty());
		}


public:

   CMP4TimeManager():
	  _has_stts(false)
	, _has_ctts(false)
	, _has_stss(false)
    , _has_elst(false)
	, _sample_count(0) //default
	, _stream_offset(0)
	, _p_sample_2composition(NULL)
	, _ctts_entry_count(0)
	, _ctts_stream_offset(0)
	//, _max_composition_offset(0)
	  {
	  }

   void set_sample_count(const uint64_t sample_count) 
   {
	   _sample_count = sample_count;
	   allocate_sample_2composition();
   }

   virtual ~CMP4TimeManager()
   {
	   if(_p_sample_2composition)
		   delete _p_sample_2composition;

	   _p_sample_2composition = NULL;
   }

	  
      void parse_stts(CMP4 &mp4)
	  {
	     MP4CHECK(box_stts);
		 _ASSERTE(!_has_stts);

		 allocate_sample_2composition();

		 mp4.do_full_box();

		unsigned int entry_count = mp4.read_uint();

		uint64_t total_time(0);
		uint64_t total_entry(0);

		decoding dec     = {0, 0, 0};
		composition comp = {0, 0, 0};

		for (unsigned int i = 0; i < entry_count; i++) 
		{
		   unsigned int sample_count = mp4.read_uint();
		   dec.sample_delta = mp4.read_uint();

		   for(unsigned int x = 0; x < sample_count; x++)
		   {
			   total_entry++;
			   comp.duration = dec.sample_delta;
			   _p_sample_2composition->push_back(comp);
			   _ASSERTE(_p_sample_2composition->size() == total_entry);//(x+1));
			   
			   total_time += dec.sample_delta;
			   comp.decoding_time = total_time;
			   //comp.duration = dec.sample_delta;
		   } 

		   _time_2decoding_sample.add(total_time, dec);

		   dec.samples += sample_count;
		   dec.duration = total_time;
		}

		//_time_2decoding_sample.add(total_time, dec);

		 _has_stts = true;
	  }


	  void parse_ctts(CMP4 &mp4)
	  {
		  MP4CHECK(box_ctts);
		 _ASSERTE(!_has_ctts);
		 _ASSERTE(_has_stts);

		_ASSERTE(_p_sample_2composition);

		mp4.do_full_box();

		_ctts_entry_count = mp4.read_uint();


		std::stack<composition_sample_number> reorder_stack;

		
		uint64_t total_samples(0);

		for (unsigned int i = 0; i < _ctts_entry_count; i++) 
		{
		   unsigned int sample_count  = mp4.read_uint();
		   unsigned int sample_offset = mp4.read_uint();

		   if(1 == _ctts_entry_count)
			   _ctts_stream_offset = sample_offset;

		   //if(sample_offset > _max_composition_offset)
			  // _max_composition_offset = sample_offset;

		   for(unsigned int x = 0; x < sample_count; x++)
		   {
			   _ASSERTE(total_samples < _p_sample_2composition->size());
			  
			   if(total_samples < _p_sample_2composition->size())
			   {
				   composition_sample_number n = {
					  _p_sample_2composition->at(static_cast<uint32_t>(total_samples)).decoding_time + sample_offset
					  , total_samples
				  };
				   
				   _p_sample_2composition->at(static_cast<uint32_t>(total_samples++)).composition_offset = sample_offset;

				   //_composition_2sample.push_back(n);

				   add_ordered(reorder_stack, _composition_2sample, n);
			   }
			   else
			   {
				   fprintf(stdout
					   , ">invalid composition sample: decoding samples: %lu vs %" PRIu64 "\r\n"
								, _p_sample_2composition->size()
								, total_samples);

				   total_samples++;

			   }

			 
		   }
		}

		/*
		std::vector<composition_sample_number>::iterator begin = _composition_2sample.begin();
		std::vector<composition_sample_number>::iterator end   = _composition_2sample.end();


		std::sort(
			  begin
			, end
			, comp_sample_comp
			);
		*/
		
  
        _has_ctts = true;
	  }
	  void parse_stss(CMP4 &mp4)
	  {
		   MP4CHECK(box_stss);
		   _ASSERTE(!_has_stss);
		
		   mp4.do_full_box();

		  unsigned int entry_count = mp4.read_uint();
		  //uint64_t total_samples(0);

		  unsigned int previus_sample_number(0);

		for (unsigned int i = 0; i < entry_count; i++) 
		{		   		   
		   unsigned int sample_number  = (mp4.read_uint() - 1);

		   if(0 < i)
			  _sample_2I_frame.add(sample_number - 1, previus_sample_number);

		   previus_sample_number = sample_number;
		}

		_sample_2I_frame.add(previus_sample_number + 1, previus_sample_number);

        _has_stss = true;
	  }
	  void parse_elst(CMP4 &mp4)
	  {
		  MP4CHECK(box_elst);
		  _ASSERTE(!_has_elst);

		  mp4.parse_edl_box(_elst);

		  if(2 == _elst.count())
		  {
			  if(-1 == _elst.entry(0)._media_time)
			  {
				  _stream_offset = _elst.entry(0)._segment_duration;
			  }
		  }

		  _has_elst = true;
	  }

	  uint64_t get_decoding_sample_number(uint64_t time)const 
	  {
		  //_ASSERTE(!_has_elst);

		  
		  decoding dec = {0, 0, 0};
         _time_2decoding_sample.get(time, dec);

		 uint64_t reftime = dec.duration;
		 _ASSERTE(time >= reftime);
		 reftime = time - reftime;

		 _ASSERTE(dec.sample_delta);

		 //if(!dec.sample_delta)
		 // return 0;

		 uint64_t sample_diff = (dec.sample_delta)?(reftime / dec.sample_delta):0;

		 return dec.samples + sample_diff;
	  }

	  uint64_t get_composition_sample_number(uint64_t time)const 
	  {
		  if(!_has_ctts)
			  return get_decoding_sample_number(time);

		  //uint64_t target_time = time;

		  //if(time <= _max_composition_offset)
			 // time = 0;
		  //else
			 // time -= _max_composition_offset;

		  //uint64_t sample_number = get_decoding_sample_number(time);

		  //while(get_composition_time(sample_number) < target_time && sample_number < _p_sample_2composition->size())
		  //{
			 // sample_number++;
		  //}

		  //return sample_number;


		  composition_sample_number target = {time, 0};

		  std::vector<composition_sample_number>::const_iterator it = 
			std::lower_bound(_composition_2sample.begin()
			, _composition_2sample.end()
			, target
			, comp_sample_comp);


		bool found = (
			    it != _composition_2sample.end()
			 && time <= it->composition_time);

		if(!found)
			return 0;

		if(it == _composition_2sample.begin())
			return 0;

		_ASSERTE(it != _composition_2sample.begin());

		if(time < it->composition_time)
		   it--;

		return it->sample_number;
		  
	  }
     

	  //return the decoding time + the composition offset
	  uint64_t get_composition_time(const uint64_t sample_number) const
	  {
		  //_ASSERTE(!_has_elst);

		  if(!_p_sample_2composition)
			  ALXTHROW_T(_T("_p_sample_2composition IS NULL -get_composition_time- "));

		  _ASSERTE(0 <= sample_number && sample_number < _p_sample_2composition->size());
		  
		  if(sample_number >= _p_sample_2composition->size())
		  {
			  Cstring e(_T("INVALID SAMPLE NUMBER -get_composition_time- "));
			          e += sample_number;
					  e += _T(" [");
					  e += _p_sample_2composition->size();
					  e += _T("].");
			  ALXTHROW_T(static_cast<const TCHAR*>(e));
		  }
			  

		  composition comp = (*_p_sample_2composition)[static_cast<uint32_t>(sample_number)];

		  return comp.decoding_time + comp.composition_offset;
	  }

	      
     uint64_t get_IFrame_number(uint64_t time)
	 {	  
		 if(!_has_stss)
			 return get_composition_sample_number(time);

		  uint64_t IFrame_number(0);

		 _sample_2I_frame.get(get_decoding_sample_number(time), IFrame_number);
		 //_sample_2I_frame.get(get_composition_sample_number(time), IFrame_number);

		 return IFrame_number;
	  }

	 uint64_t get_next_IFrame_number() 
	 {	  
		  uint64_t IFrame_number(0);
		  uint64_t time = _sample_2I_frame.get_upper_limit() + 1;
		  
		  _sample_2I_frame.get(time, IFrame_number);

		  //return _sample_2I_frame.get_upper_limit();

		  return IFrame_number;
	  }

	 uint64_t get_decoding_time(const uint64_t sample_number) const
	 {
		  //_ASSERTE(!_has_elst);

		  _ASSERTE(0 <= sample_number && sample_number < _p_sample_2composition->size());

		  composition comp = (*_p_sample_2composition)[static_cast<uint32_t>(sample_number)];

		  return comp.decoding_time;
	 }

	 uint64_t get_duration(const uint64_t sample_number) const
	 {
		  //_ASSERTE(!_has_elst);

		  _ASSERTE(0 <= sample_number && sample_number < _p_sample_2composition->size());

		  composition comp = (*_p_sample_2composition)[static_cast<uint32_t>(sample_number)];

		  return comp.duration;
	 }

	 bool is_IFrame(const uint64_t sample_number)
	 {
		 uint64_t kframe(0);

		 if(!_sample_2I_frame.size())
			 return true;

		_sample_2I_frame.get(sample_number, kframe);

		_ASSERTE(kframe <= sample_number);

		return (kframe == sample_number);
	 }

	 uint64_t get_offset() const {return _stream_offset;}

	 bool has_composition_time() const {return _has_ctts;}
	 bool has_random_access_point() const {return _has_stss;}

	 uint64_t get_start_time() const
	 {
		 if(!has_composition_time())
		 {
			 uint64_t start_time = get_composition_time(0);
			 uint64_t sample_count = LOOK_UP_SAMPLES;
			 if(sample_count > _sample_count)
				 sample_count = _sample_count;

			 for(uint64_t idx = 1; idx < sample_count; idx++)
			 {
				 //TODO: we do not have composition. This should not be happening.
				 _ASSERTE(get_composition_time(idx) > start_time);

				 if(get_composition_time(idx) < start_time)
				 {
					 start_time = get_composition_time(idx);
				 }
			 }
		 
			 _ASSERTE(get_composition_time(0) == start_time);
			 return start_time;
		 }


		 _ASSERTE(_composition_2sample.size());
		 return _composition_2sample[0].composition_time;
	 }

	 uint64_t get_end_time() const
	 {
		 if(!has_composition_time())
		 {
			 uint64_t end_time = get_composition_time(_sample_count -1);
			 if(_sample_count > 1)
			 {
				 uint64_t sample_count = LOOK_UP_SAMPLES;
				 if(sample_count > _sample_count)
					 sample_count = _sample_count;

				 for(uint64_t idx = _sample_count - 2; idx > (_sample_count - sample_count)  ; idx--)
				 {
					 //TODO: we do not have composition. This should not be happening.
					 _ASSERTE(get_composition_time(idx) < end_time);
					 if(get_composition_time(idx) > end_time)
					 {
						 end_time = get_composition_time(idx);
					 }
				 }
			 }
		 
			 _ASSERTE(end_time == get_composition_time(_sample_count - 1));
			 return end_time;
		 }

		 _ASSERTE(_composition_2sample.size());
		 return _composition_2sample[static_cast<uint32_t>(_sample_count -1)].composition_time;
	 }

	 uint64_t get_end_time_plus_duration() const
	 {
		 uint64_t end = get_end_time();

		 if(!has_composition_time())
			return end + get_duration(_sample_count -1);

		 uint64_t duration = get_duration(_composition_2sample[static_cast<uint32_t>(_sample_count -1)].sample_number);

		 return duration + end;
	 }

	 uint64_t get_sample_count() const {return _sample_count;}

	 unsigned int get_ctts_entry_count() const {return _ctts_entry_count;}
	 uint64_t get_ctts_stream_offset() const {return _ctts_stream_offset;}

};

class CMP4Handler
{
protected:
    uint64_t _minf_position;
	uint64_t _minf_size;

	uint64_t _dinf_position;
	uint64_t _dinf_size;

	DataReferenceBox     _dref;
	SampleDescriptionBox _stsd;

	bool _has_sample_description;

	virtual bool parse_header(CMP4& mp4) = 0;
	virtual void parse_sample_description(CMP4& mp4) = 0;

	virtual void not_sample_description_found(CMP4 &mp4)
	{
		if(mp4.get_position() >= (_minf_position + _minf_size))
				MP4ERROR(_T("sample description not found"));

		  mp4.do_box();
		  parse_boxes(mp4);
	}
	
	void parse_boxes(CMP4 &mp4)
	{
		switch(mp4.get_box().get_type())
		{
		case box_stbl:
			break;
		case box_dinf:
			_dinf_position = mp4.get_box_position();
		    _dinf_size     = mp4.get_box().get_size();
			break;
		case box_dref:
			mp4.parse_data_reference_box(_dref);
			break;
		case box_stsd:
			mp4.parse_sample_description_box(_stsd);
			for(unsigned int i = 0; i < _stsd.entry_count; i++)
			{
				mp4.do_box();
				parse_sample_description(mp4);
				_has_sample_description = true;
			}
			break;
			
		default:
			_ASSERTE(false);
			mp4.skip_current();
			break;
		}

		if(!_has_sample_description)
		{
			not_sample_description_found(mp4);
		}
	
	}

	
public:
	void parse(CMP4 &mp4)
	{
		MP4CHECK(box_minf);

		_minf_position = mp4.get_box_position();
		_minf_size     = mp4.get_box().get_size();

		mp4.do_box();
		if(parse_header(mp4))
			mp4.do_box();

		parse_boxes(mp4);
	}

	CMP4Handler():
	_has_sample_description(false)
	{}

	int entry_count() const {return _stsd.entry_count;}

	virtual ~CMP4Handler(){}

};


class CAVCCHeader: public AVCDecoderConfigurationRecord 
{
	std::vector<H264Sequence*>           _sequences;
	std::vector<pic_parameter_set_rbsp*> _pictures;
protected:
    
	virtual void add_sequence_parameter_set(const unsigned char * body, unsigned int size)
	{
		H264Sequence *psequence = new H264Sequence;

		H264Nal nal;
		nal.decode_nal(size
			, body);
		
		_ASSERTE(NALTYPE::SEQUENCE == nal.get_decoded_nal_unit_type());
		
		FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
			              , nal.decoded_rbsp_size() -1
			);

		psequence->get(mem);

		_sequences.push_back(psequence);


	}
	virtual void add_picture_parameter_set (const unsigned char * body, unsigned int size)
	{
		pic_parameter_set_rbsp * ppic = new pic_parameter_set_rbsp;
	
		H264Nal nal;
		nal.decode_nal(size
			, body);
		
		_ASSERTE(NALTYPE::PICTURE == nal.get_decoded_nal_unit_type());
		
		FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
			              , nal.decoded_rbsp_size() -1
			);

		ppic->get(mem);

		_pictures.push_back(ppic);
	}

public:
	virtual ~CAVCCHeader()
	{
		_ASSERTE(_sequences.size() == _pictures.size());

		for(unsigned int i = 0; i < _sequences.size(); ++i)
		{
			delete _pictures[i];
			delete _sequences[i];
		}

		_pictures.clear();
		_sequences.clear();
	}

	
	const H264Sequence           & get_sequence(unsigned int idx) const {_ASSERTE(idx < _sequences.size()); return *_sequences[idx];}
	const pic_parameter_set_rbsp & get_picture (unsigned int idx) const {_ASSERTE(idx < _pictures.size());  return *_pictures[idx];}

	const BYTE* get_nal_sequence(unsigned int idx = 0) const {return AVCDecoderConfigurationRecord::get_sequence(idx);}
	const BYTE* get_nal_picture(unsigned int idx = 0) const { return AVCDecoderConfigurationRecord::get_picture(idx);}
      
};


class CMP4VisualEntry
{
	VisualSampleEntry _visual;
	uint64_t _visual_end;

	CAVCCHeader      _avc_header;

	BYTE * _p_body;
	uint64_t _body_size;

	BitRateBox _btrt;
	PixelAspectRatioBox _pasp;

	unsigned int     _entry_type;

	void parse_boxes(CMP4 &mp4)
	{
		while(mp4.get_position() < _visual_end)
		{
			mp4.do_box();
			switch(mp4.get_box().get_type())
			{
			case box_avcC:
			   _body_size = mp4.get_box().get_size() - 8;
			   _p_body = new BYTE[static_cast<uint32_t>(_body_size)];

			   mp4.read_bytes(_p_body, _body_size);
			   
			   {
				   FixedMemoryBitstream mem(_p_body, static_cast<uint32_t>(_body_size));
				   _avc_header.get(mem);
			   }

			   break;
			case box_btrt:
				mp4.parse_bitrate_box(_btrt);
				break;
			case box_pasp:
				mp4.parse_pasp_box(_pasp);
				break;
			default:
				_ASSERTE(false);
				mp4.skip_current();
			}
		}
	}

public:
	void parse(CMP4 &mp4)
	{
		_entry_type = mp4.get_box().get_type();

#ifdef CENC
		_ASSERTE(box_avc1 == mp4.get_box().get_type()
			|| box_encv == mp4.get_box().get_type() 
			);
#else
		_ASSERTE(box_avc1 == mp4.get_box().get_type()
			);
#endif	

		_visual_end = mp4.get_box_position_end();
		mp4.parse_visual_sample_entry(_visual);

		parse_boxes(mp4);
	}

	virtual void close()
	{
		if(_p_body)
			delete[] _p_body;

		_p_body = NULL;
	}

	CMP4VisualEntry():
		_p_body(NULL)
	{}
	CMP4VisualEntry(const CMP4VisualEntry &rhs)
	{
		_visual = rhs._visual;
	    _visual_end = 0;

		_btrt = rhs._btrt;

		_body_size = rhs._body_size;

		if(_body_size > 0)
		{
			_p_body = new BYTE[static_cast<uint32_t>(_body_size)];

			::memcpy(_p_body, rhs._p_body, static_cast<uint32_t>(_body_size));
		}
	
	}

	CMP4VisualEntry(
		           const unsigned int width
                 , const unsigned int height
				 , const unsigned int horizresolution
				 , const unsigned int vertresolution
		         , const BYTE* source
                 , const unsigned size)
	{
		
		_visual.width  = width;
        _visual.height = height;
        _visual.horizresolution = horizresolution;
        _visual.vertresolution = vertresolution;
        _visual.frame_count = 1;
        _visual.size = 0;

	/////////////////////////IOS 7////////////////////
		_visual.depth = 0;
	    _btrt.bufferSizeDB = 1024 * 1000 * 32;
		_btrt.maxBitrate   = 1024 * 1000 * 32;
		_btrt.avgBitrate   = 1024 * 1000 * 32;
	//////////////////////////////////////////////////
    
		_visual.data_reference_index = 0x01;

		_body_size = size;

		if(size > 0)
		{
			_p_body = new BYTE[size];
			::memcpy(_p_body, source, size);
		}
	
	
	}

	virtual ~CMP4VisualEntry(){close();}

	const VisualSampleEntry & get_entry() const {return _visual;}

	const BYTE * get_body() const {return _p_body;}
	const uint64_t get_body_size() const {return _body_size;}

	BitRateBox & get_btrt() {return _btrt;}

	const CAVCCHeader & get_avcc_header() const {return _avc_header;}

	unsigned int get_entry_type() const {return _entry_type;}
};

class CMP4AudioEntry
{
	AudioSampleEntry _audio;
	uint64_t _audio_end;

	aac_info_mp4 _aac;

	BYTE * _p_body;
	uint64_t _body_size;

	unsigned int     _entry_type;
	
public:
	void parse(CMP4 &mp4)
	{
		_entry_type = mp4.get_box().get_type();
		_ASSERTE(box_mp4a == mp4.get_box().get_type());
		_audio_end = mp4.get_box_position_end();
		mp4.parse_audio_sample_entry(_audio);

		if(_audio_end > mp4.get_position())
		{
			_body_size = _audio_end - mp4.get_position();
			_p_body = new BYTE[static_cast<uint32_t>(_body_size)];

			mp4.read_bytes(_p_body, _body_size);
#ifdef CENC
			if(box_mp4a == _entry_type
				|| box_enca == _entry_type)
			{
#else
			if(box_mp4a == _entry_type)
			{
#endif
				FixedMemoryBitstream  mem(
				  get_body()
				, static_cast<uint32_t>(get_body_size()));

			unsigned int box_size = mem.getbits(32);
			unsigned int box_type = mem.getbits(32);
			
			_ASSERTE(box_esds == box_type);

			mem.skipbits(32); //version flags

			ES_Descriptor es;
			es.get(mem);

			if(!es.p_decoder_config_descriptor->p_ext->get_length())
				ALXTHROW_T(_T("no audio decoder configuration"));
			
			FixedMemoryBitstream  maac(es.p_decoder_config_descriptor->p_ext->get_descriptor()
								, es.p_decoder_config_descriptor->p_ext->get_length()
								  );

			_aac;
			_aac.get(maac);

			}
		}
	}

	virtual void close()
	{
		if(_p_body)
			delete[] _p_body;

		_p_body = NULL;
	}

	CMP4AudioEntry():
		_p_body(NULL)
	{}

	CMP4AudioEntry(const unsigned int sample_rate
				 , const unsigned int channels
				 , const BYTE* source
                 , const unsigned size)
	{
		_audio.set_channelcount(channels);
		_audio.set_samplesize(16);
		_audio.set_samplerate(sample_rate);
		_audio.data_reference_index = 0x01;

		_body_size = size;

		if(size > 0)
		{
			_p_body = new BYTE[size];
			::memcpy(_p_body, source, size);
		}
	}
	
	CMP4AudioEntry(const CMP4AudioEntry &rhs)
	{
		_audio = rhs._audio;
	    _audio_end = 0;

		_body_size = rhs._body_size;

		if(_body_size > 0)
		{
			_p_body = new BYTE[static_cast<uint32_t>(_body_size)];

			::memcpy(_p_body, rhs._p_body, static_cast<uint32_t>(_body_size));
		}
	}
	virtual ~CMP4AudioEntry(){close();}

	const AudioSampleEntry & get_entry() const {return _audio;}

	const BYTE * get_body() const {return _p_body;}
	const uint64_t get_body_size() const {return _body_size;}

	unsigned int get_entry_type() const {return _entry_type;}

	const aac_info_mp4 & get_aac_info() const {return _aac;}
};

class CMP4HandlerVide: public CMP4Handler
{
	VideoMediaHeaderBox _vmhd;
	std::vector<CMP4VisualEntry*> _entries;
	
protected:
	virtual bool parse_header(CMP4& mp4)
	{
		//MP4CHECK(box_vmhd);
		if (box_vmhd == mp4.get_box().get_type())
		{
			mp4.parse_video_header_box(_vmhd);
			return true;
		}

		return false;
	}

	virtual void parse_sample_description(CMP4& mp4)
	{
#ifdef CENC
		_ASSERTE(box_avc1 == mp4.get_box().get_type()
			|| box_encv == mp4.get_box().get_type() 
			);
#else
		_ASSERTE(box_avc1 == mp4.get_box().get_type()
			);
#endif		
		CMP4VisualEntry *pe = new CMP4VisualEntry;

		pe->parse(mp4);

		_entries.push_back(pe);
		
	}
public:
	virtual ~CMP4HandlerVide()
	{
		for(unsigned int i = 0; i < _entries.size(); i++)
			delete _entries[i];

		_entries.clear();
	}

	const CMP4VisualEntry & get_entry(const uint32_t idx) const
	{
		_ASSERTE(idx < _entries.size());
		return *_entries[idx];
	}
};


class CMP4HandlerSoun: public CMP4Handler
{
	SoundMediaHeaderBox _smhd;
	std::vector<CMP4AudioEntry*> _entries;
protected:
	virtual bool parse_header(CMP4& mp4)
	{
		//MP4CHECK(box_smhd);
		if (box_smhd == mp4.get_box().get_type())
		{
			mp4.parse_sound_header_box(_smhd);
			return true;
		}
		
		return false;
		
	}

	virtual void parse_sample_description(CMP4& mp4)
	{
		_ASSERTE(box_mp4a == mp4.get_box().get_type());
	
	    CMP4AudioEntry * pe = new CMP4AudioEntry;

		pe->parse(mp4);

		_entries.push_back(pe);
		
	}

	virtual ~CMP4HandlerSoun()
	{
		for(unsigned int i = 0; i < _entries.size(); i++)
			delete _entries[i];

		_entries.clear();
	}
public:
	const CMP4AudioEntry & get_entry(const uint32_t idx) const
	{
		_ASSERTE(idx < _entries.size());
		return *_entries[idx];
	}
};



class CMP4HandlerLTC: public CMP4Handler
{
	SoundMediaHeaderBox _smhd;
	timecode_head _head;

	bool _has_one;

protected:
	virtual bool parse_header(CMP4& mp4)
	{
		MP4CHECK(box_smhd);
		mp4.parse_sound_header_box(_smhd);

		_has_one = true;

		return true;
	}

	virtual void not_sample_description_found(CMP4 &mp4)
	{
		if(mp4.get_position() >= (_minf_position + _minf_size))
		{
			_has_one = false;
			return;
		}

		mp4.do_box();
		parse_boxes(mp4);
		
	}

	virtual void parse_sample_description(CMP4& mp4)
	{
		if(IS_LTC_HANDLER(mp4.get_box()))
		{			
			mp4.read_box(_head);

		}else
			mp4.move_box_start();
	}

	

	virtual ~CMP4HandlerLTC()
	{

	}
public:
	const timecode_head & get_head() const {return _head;}
	virtual bool has_one() const {return _has_one;}
};


class CPM4Track
{
	CMP4SampleManager _sample_manager;
	CMP4TimeManager   _time_manager;

	TrackHeaderBox    _tkhd;
	MediaHeaderBox    _mdhd;
	HandlerBox      * _p_hdlr;
	CMP4Handler     * _p_media;

	uint64_t  _trak_position;
	uint64_t  _trak_size;

	uint64_t  _current_sample;
	uint64_t  _mvhd_time_scale;

protected:
	
	const CMP4Handler * get_handler() const {return _p_media;}
	virtual void parse_boxes(CMP4 &mp4)
	{
		switch(mp4.get_box().get_type())
		{
		case box_trak:
		case box_mdia:
		case box_edts:
			break;
		case box_tkhd:
			mp4.parse_track_header_box(_tkhd);
			break;
		case box_mdhd:
			mp4.parse_media_header_box(_mdhd);
			break;
		case box_hdlr:
			_p_hdlr = new HandlerBox(static_cast<uint32_t>(mp4.get_box().get_size()));
			mp4.parse_handler_box(*_p_hdlr);
			break;
		case box_minf:
			_ASSERTE(_p_hdlr);

			if(!IsLTC())
			{
				switch(_p_hdlr->handler_type)
				{
				case BOX('v','i','d','e'):
					_p_media = new CMP4HandlerVide;
					break;
				case BOX('s', 'o','u','n'):
					_p_media = new CMP4HandlerSoun;
					break;
				default:
					MP4ERROR(_T("unknowed handler type"));
					break;
				}

				_p_media->parse(mp4);

			}else
			{
				_p_media = new CMP4HandlerLTC;
				_p_media->parse(mp4);
			}

			break;
		case box_stts:
			_time_manager.parse_stts(mp4);
			break;
		case box_ctts:
			_time_manager.parse_ctts(mp4);
			break;
		case box_elst:
			_time_manager.parse_elst(mp4);
			break;
		case box_stss:
			_time_manager.parse_stss(mp4);
			break;
		case box_stco:
			_sample_manager.parse_stco(mp4);
			break;
		case box_stsc:
			_sample_manager.parse_stsc(mp4);
			break;
		case box_stsz:
			_sample_manager.parse_stsz(mp4);
			_time_manager.set_sample_count(_sample_manager.get_sample_count());
			break;
		default:
			_ASSERTE(false);
			mp4.skip_current();
			break;
		}

		if(mp4.get_position() < (_trak_position + _trak_size))
		{
		  mp4.do_box();
		  parse_boxes(mp4);
		}
	
	}

	virtual void read_position(CMP4 &mp4)
	{
		_trak_size = mp4.get_box().get_size();
		_trak_position = mp4.get_box_position();

	}
public:
	virtual void parse(CMP4 &mp4)
	{
		MP4CHECK(box_trak);
		
		read_position(mp4);

		parse_boxes(mp4);
	}

	CPM4Track(uint64_t mvhd_time_scale):
	   _p_hdlr(NULL)
     , _p_media(NULL)
	 , _current_sample(0)
	 , _mvhd_time_scale(mvhd_time_scale)
	{}

    void close()
	{
		if(_p_hdlr)
			delete _p_hdlr;

		_p_hdlr = NULL;

		if(_p_media)
			delete _p_media;

		_p_media = NULL;

	}

	virtual ~CPM4Track(){close();}

	uint64_t get_sample_count() const 
	{
		return _sample_manager.get_sample_count();
	}

	uint64_t get_sample_offset (const uint64_t frame_number)
	{
		_ASSERTE(frame_number >= 0 && frame_number < get_sample_count());

		return _sample_manager.get_sample_offset(frame_number);
	}

	uint64_t get_sample_size (const uint64_t frame_number)
	{
		_ASSERTE(frame_number >= 0 && frame_number < get_sample_count());

		return _sample_manager.get_sample_size(frame_number);
	}

	uint64_t get_trak_total_size() const
	{
		return _sample_manager.get_total_size();
	}
	uint64_t get_trak_duration() const
	{
		return _mdhd.get_duration_hns();
	}
	uint64_t get_sample_duration() const
	{

		if(2 > _sample_manager.get_sample_count())
			return _mdhd.get_duration_hns();

		/*

		return _mdhd.get_duration_hns() / (_sample_manager.get_sample_count() - 1);

		*/

		return (get_end_time_plus_duration() - get_start_time()) / get_sample_count();
	}
	uint64_t get_trak_bit_rate() const
	{
		uint64_t d = get_trak_duration();

		_ASSERTE(d);
		
		if(0 == d)
			return 0;

		d = d/10000000lu;

		if(0 == d)
			return 0;

		return get_trak_total_size()*8lu/(d);
	}

	int entry_count() const {return _p_media->entry_count();}

	bool IsVisual() const 
	{
		_ASSERTE(_p_hdlr);
		return _p_hdlr->handler_type == BOX('v','i','d','e');
	}

	const CMP4VisualEntry & get_visual_entry(const int idx) const
	{		
		_ASSERTE(IsVisual());
		return static_cast<ALX::CMP4HandlerVide*>(_p_media)->get_entry(idx);
	}

	bool IsSound() const
	{
		_ASSERTE(_p_hdlr);
		return _p_hdlr->handler_type == BOX('s', 'o','u','n');
	}

	virtual bool IsLTC() const
	{
		return false;
	}

	const CMP4AudioEntry & get_audio_entry(const int idx) const
	{		
		_ASSERTE(IsSound());
		return static_cast<ALX::CMP4HandlerSoun*>(_p_media)->get_entry(idx);
	}

	
	uint64_t get_composition_time(const uint64_t sample_number) const
	{
		uint64_t composition_time = _time_manager.get_composition_time(sample_number);

		return (1000UL * 10000UL * composition_time) / _mdhd.get_timescale() ;

	}

	uint64_t get_decoding_time(const uint64_t sample_number) const
	{
		uint64_t decoding_time = _time_manager.get_decoding_time(sample_number);

		return (1000UL * 10000UL * decoding_time) / _mdhd.get_timescale() ;

	}

	uint64_t get_sample_duration(const uint64_t sample_number) const
	{
		uint64_t duration = _time_manager.get_duration(sample_number);

		return (1000UL * 10000UL * duration) / _mdhd.get_timescale() ;

	}
	uint64_t get_IFrame_number(uint64_t time)
	{	  

		 uint64_t composition_time = time * _mdhd.get_timescale() / (1000UL * 10000UL);

		 return _time_manager.get_IFrame_number(composition_time);
	}

	uint64_t get_next_IFrame_time()
	{
		uint64_t sample = _time_manager.get_next_IFrame_number();
		
		//return (1000UL * 10000UL * time) / _mdhd.get_timescale() ;

		return get_composition_time(sample);
	}

	uint64_t get_decoding_sample_number(const uint64_t time) const
	{
		uint64_t dtime = time * _mdhd.get_timescale() / (1000UL * 10000UL);
		return _time_manager.get_decoding_sample_number(dtime);
	}

	uint64_t get_composition_sample_number(const uint64_t time) const
	{
		uint64_t dtime  = time * _mdhd.get_timescale() / (1000UL * 10000UL);
		//uint64_t sample = _time_manager.get_decoding_sample_number(dtime);
		uint64_t sample = _time_manager.get_composition_sample_number(dtime);

		if(!sample)
			return 0;

		//while(dtime > _time_manager.get_composition_time(sample)
		//	&& 0 < sample )
		//{
		//	sample--;
		//}

		if(sample >= _time_manager.get_sample_count())
		{
			return _time_manager.get_sample_count() - 1;
		}

		if(_time_manager.get_composition_time(sample) < dtime)
			sample++;

		if(sample >= _time_manager.get_sample_count())
		{
			return _time_manager.get_sample_count() - 1;
		}

		return sample;
	}

	uint64_t get_stream_offset() const
	{
		uint64_t time = _time_manager.get_offset();
		return (1000UL * 10000UL * time) / _mvhd_time_scale;
	}

	uint64_t get_start_time() const
	{
		uint64_t time = _time_manager.get_start_time();
		return (1000UL * 10000UL * time) / _mdhd.get_timescale() ;
	}

	uint64_t get_end_time() const
	{
		uint64_t time = _time_manager.get_end_time();
		return (1000UL * 10000UL * time) / _mdhd.get_timescale() ;
	}

	uint64_t get_end_time_plus_duration() const
	{
		uint64_t time = _time_manager.get_end_time_plus_duration();
		return (1000UL * 10000UL * time) / _mdhd.get_timescale() ;
	}
	
	void move(const uint64_t time)
	{
		_current_sample = get_decoding_sample_number(time);
	}

	void move_to_sample(const uint64_t sample_number)
	{
		_current_sample = sample_number;
	}

	bool pick_sample(sample_info &ms, uint64_t sample_number)
	{
		if(sample_number >= get_sample_count())
			return false;

		ms.offset           = get_sample_offset(sample_number);
		ms.size             = get_sample_size(sample_number);
		ms.composition_time = get_composition_time(sample_number);
		ms.decoding_time    = get_decoding_time(sample_number);
		ms.duration         = get_sample_duration(sample_number);

		ms.bIsSyncPoint     = _time_manager.is_IFrame(sample_number);
        ms.discontinuity    = false;
		//ms.duration = 0;

		ms.sample_number    = sample_number;

		return true;
	}

	bool pick_sample(sample_info &ms)
	{
		return pick_sample(ms, _current_sample);
	}

	bool pick_haed(sample_info &ms, uint64_t head_samples = 1)
	{
         return pick_sample(ms, _current_sample + head_samples);
	}

	bool move_next()
	{
		if(_current_sample < _sample_manager.get_sample_count())
		{
			_current_sample++;
			return true;
		}

		return false;
	}

	uint64_t get_media_time_scale() const {return _mdhd.get_timescale();}

	const TCHAR * get_language() {return _mdhd.get_lang();}

	bool has_composition_time() const    {return _time_manager.has_composition_time();}
	bool has_random_access_point() const {return _time_manager.has_random_access_point();}

	unsigned int ctts_entry_count() const {return _time_manager.get_ctts_entry_count();}
	uint64_t ctts_stream_offset() const 
	{
		
		uint64_t time = _time_manager.get_ctts_stream_offset();
		return (1000UL * 10000UL * time) / _mdhd.get_timescale() ;
	}

};


class CPM4LTCTrack: public CPM4Track
{
protected:
	
public:

	CPM4LTCTrack(uint64_t mvhd_time_scale):CPM4Track(mvhd_time_scale){}
	
	virtual void parse(CMP4 &mp4)
	{
		MP4CHECK(box_uuid);

		if(!IS_LTC_BOX(mp4.get_box()))
		{
			MP4ERROR(_T("This is not a valid LTC custom extension stream."));
		}

		read_position(mp4);

		mp4.do_box();

		parse_boxes(mp4);
	}

	virtual bool IsLTC() const
	{
		return true;
	}

	virtual uint64_t frame_avg_time() const
	{
		return static_cast<const CMP4HandlerLTC*>(get_handler())->get_head().avg_frame_rate;
	}

	virtual bool has_one() const
	{
		return static_cast<const CMP4HandlerLTC*>(get_handler())->has_one();
	}
};

#ifndef WEBSTREAM
/*
class CMP4File: public CMP4
{
	FileBitstream * _p_ff;
public:
	CMP4File():
	  _p_ff(NULL)
	{}
    void Close()
	{
		if(_p_ff)
			delete _p_ff;
		_p_ff = NULL;
	}
	virtual ~CMP4File(){Close();}
	
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
#endif



class CMP4Memory: public CMP4
{
	FixedMemoryBitstream *_p_m;
public:
	CMP4Memory():
	  _p_m(NULL)
	{}

    void Close()
	{
		if(_p_m)
			delete _p_m;
		_p_m = NULL;
	}
	virtual ~CMP4Memory(){Close();}
	
	void Open(const BYTE * p, uint64_t size)
	{
		_ASSERTE(NULL == _p_f);
		_ASSERTE(NULL == _p_m);
		_p_m = new FixedMemoryBitstream(p, static_cast<uint32_t>(size));
		_p_f = _p_m;
	}

	/*
	void Open(MemoryBitstream * p)
	{
		_ASSERTE(NULL == _p_f);
		_ASSERTE(NULL == _p_m);

		_p_f = p;
	}
	*/
};


class MP4Reader
{
	FileTypeBox *           _p_ftyp;
	InitialObjectDescriptor _iods;
	MovieHeaderBox          _mvhd;
	uint64_t        _mdat_position;
	uint64_t        _moov_position;
	std::vector<uint64_t> _meta_position;
	uint64_t        _mdat_end;

	uint64_t        _last_offset;

	std::vector<CPM4Track*> _streams;

protected:

	virtual void unknown_box(CMP4 &mp4)
	{
		//_ASSERTE(false);
		mp4.skip_current();
	}

private:

	void parse_boxes(CMP4 &mp4)
	{
		CPM4Track * ptrack(NULL);

		uint64_t meta_position(0);

		switch(mp4.get_box().get_type())
		{
        case box_ftyp:
        case box_STYP:
			_p_ftyp = new FileTypeBox(static_cast<uint32_t>(mp4.get_box().get_size()));
			mp4.parse_file_type_box(*_p_ftyp);
			break;
		case box_iods:
			//mp4.do_full_box();
			//mp4.parse_iods_box(_iods);
			mp4.skip_current();
			break;
		case box_mvhd:
			mp4.parse_movie_header_box(_mvhd);
			break;
		case box_mdat:
			_mdat_position = mp4.get_box_position();
			_mdat_end = mp4.get_box_position() + mp4.get_box().get_size();
		case box_free:
			mp4.set_position(mp4.get_box_position() + mp4.get_box().get_size());
			break;
		case box_moov:
			_moov_position = mp4.get_box_position();
			break;
		case box_trak:
			ptrack = new CPM4Track(_mvhd.get_timescale());
			ptrack->parse(mp4);
			_streams.push_back(ptrack);
			break;
		case box_meta:
			 meta_position = mp4.get_box_position();
			_meta_position.push_back(meta_position);
			mp4.skip_current();
			break;
		case box_uuid:
			if(IS_LTC_BOX(mp4.get_box()))
			{
				ptrack = new CPM4LTCTrack(_mvhd.get_timescale());
							ptrack->parse(mp4);
			    _streams.push_back(ptrack);
			}
			else
			{
				_ASSERTE(false);
				mp4.skip_current();
			}
			break;
		default:
			unknown_box(mp4);
			break;
		}

		if(!mp4.eof())
		{
		  mp4.do_box();
		  parse_boxes(mp4);
		}
	}
	
public:	

	MP4Reader():
	  _mdat_position(0)
    , _mdat_end(0)
	, _p_ftyp(NULL)
	, _last_offset(0)
	{}

    void parse(CMP4 &mp4)
	{
		if(0 != mp4.get_position())
			mp4.set_position(0);

		mp4.do_box();

		MP4CHECK2(box_ftyp, box_STYP);

		parse_boxes(mp4);

		
	}

	void close()
	{
		for(unsigned int i = 0; i < _streams.size(); i++)
			delete _streams[i];

		_streams.clear();

		if(_p_ftyp)
			delete _p_ftyp;

		_p_ftyp = NULL;
	}

	virtual ~MP4Reader(){close();}

	uint64_t get_duration() const
	{
		return _mvhd.get_duration_hns();
	}

	uint64_t get_duration(unsigned int stream_number) const
	{
				_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_trak_duration();
	}

	uint64_t get_sample_duration(unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_sample_duration();
	}

	uint64_t get_sample_offset(const uint64_t sample, unsigned int stream_number)
	{
		_ASSERTE(stream_number < _streams.size());
		CPM4Track* pt = _streams[stream_number];

		return pt->get_sample_offset(sample);
	}

	uint64_t get_sample_size(const uint64_t sample, unsigned int stream_number)
	{
		_ASSERTE(stream_number < _streams.size());
		CPM4Track* pt = _streams[stream_number];

		return pt->get_sample_size(sample);
	}

	int stream_count() const{return _streams.size();}

	const CMP4VisualEntry & get_visual_entry(const int idx, unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_visual_entry(idx);
	}

	const CMP4AudioEntry & get_audio_entry(const int idx, unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_audio_entry(idx);
	}

	int entry_count(unsigned int stream_number) const 
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->entry_count();
	}

	uint64_t get_composition_time(const uint64_t sample_number, const unsigned int stream_number) const
	{	
	   _ASSERTE(stream_number < _streams.size());
	   return _streams[stream_number]->get_composition_time(sample_number);
	}

	uint64_t get_decoding_time(const uint64_t sample_number, const unsigned int stream_number) const
	{	
	   _ASSERTE(stream_number < _streams.size());
	   return _streams[stream_number]->get_decoding_time(sample_number);
	}

	uint64_t get_composition_sample_number(const uint64_t time, const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
	   return _streams[stream_number]->get_composition_sample_number(time);
	}

    uint64_t get_IFrame_number(const uint64_t time, const unsigned int stream_number) const
	{	
	   _ASSERTE(stream_number < _streams.size());
	   return _streams[stream_number]->get_IFrame_number(time);
	}

	uint64_t get_next_IFrame_time(const unsigned int stream_number)
	{
	   _ASSERTE(stream_number < _streams.size());
	   return _streams[stream_number]->get_next_IFrame_time();
	}

	uint64_t get_sample_count(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_sample_count();
	}

	uint64_t get_media_time_scale(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_media_time_scale();
	}

	uint64_t get_total_stream_size(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_trak_total_size();
	}

	uint64_t get_stream_bit_rate(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_trak_bit_rate();
	}
	uint64_t get_stream_offset(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_stream_offset();
	}
	uint64_t get_start_time(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_start_time();
	}
	uint64_t get_end_time(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_end_time();
	}

	uint64_t get_end_time_plus_duration(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_end_time_plus_duration();
	}
    
	uint64_t get_decoding_start_time(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_decoding_time(0);
	}

	uint64_t get_decoding_end_time(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_decoding_time(_streams.size() - 1);
	}

	uint64_t get_decoding_end_time_plus_duration(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return get_decoding_end_time(_streams.size() - 1) + get_sample_duration(_streams.size() - 1);
	}


	
	bool IsVisual(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->IsVisual();
	}

	int VisualStream() const
	{
		int v_stream = -1;

		for(int i = 0; i < stream_count(); i++)
		{
			if(IsVisual(i))
			{
				v_stream = i;
				break;
			}
		}

		return v_stream;
	}

	
	bool HasVisual() const
	{
		return (-1 != VisualStream());
	}

	bool IsSound(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->IsSound();
	}

	bool IsLTC(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->IsLTC();
	}

	int LTCStream() const
	{
		int ltc_stream = -1;

		for(int i = 0; i < stream_count(); i++)
		{
			if(IsLTC(i))
			{
				ltc_stream = i;
				break;
			}
		}

		return ltc_stream;
	}

	bool HasLTC() const
	{
		return (-1 != LTCStream());
	}


	uint64_t LTC_avg_time(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		_ASSERTE(_streams[stream_number]->IsLTC());

		return static_cast<CPM4LTCTrack*>(_streams[stream_number])->frame_avg_time();
	}

	bool LTC_has_one(const unsigned int stream_number) const
	{
		_ASSERTE(stream_number < _streams.size());
		_ASSERTE(_streams[stream_number]->IsLTC());

		return static_cast<CPM4LTCTrack*>(_streams[stream_number])->has_one();
	}

	bool has_composition_time(const unsigned int stream_number) const    
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->has_composition_time();
	}
	bool has_random_access_point(const unsigned int stream_number) const 
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->has_random_access_point();
	}

	unsigned int ctts_entry_count(const unsigned int stream_number) const 
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->ctts_entry_count();
	}

	bool stream_ctts_offset(const unsigned int stream_number) const 
	{
		return (1 == ctts_entry_count(stream_number));
	}

	uint64_t ctts_stream_offset(const unsigned int stream_number) const 
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->ctts_stream_offset();
	}

	void move(const uint64_t time, unsigned int stream = UINT32_MAX)
	{
		if(UINT32_MAX == stream) //use first video stream
		{
			for(unsigned int i = 0; i < _streams.size(); i++)
			{
				if(_streams[i]->IsVisual())
				{
					stream = i;
					break;
				}
			}
		}

		if(UINT32_MAX == stream) //we do not have video :-( //use first audio
		{
			stream = 0;
		}

		_ASSERTE(stream < _streams.size());

		uint64_t sample_number    = _streams[stream]->get_IFrame_number(time);

		if(!_streams[stream]->has_random_access_point())
		{
			sample_number = _streams[stream]->get_composition_sample_number(time - 
						_streams[stream]->get_stream_offset());

					if(time < _streams[stream]->get_stream_offset())
						sample_number = 0;
		}

		uint64_t decoding_time    = _streams[stream]->get_decoding_time(sample_number);
		uint64_t composition_time = _streams[stream]->get_composition_time(sample_number);

		composition_time += _streams[stream]->get_stream_offset();//should always be zero for video

		for(unsigned int i = 0; i < _streams.size(); i++)
		{
			if(_streams[i]->get_sample_count())
			{
				if(!_streams[i]->has_random_access_point()
					&& !_streams[i]->IsLTC())
				{
					/*
					uint64_t sn = _streams[i]->get_composition_sample_number(composition_time);
					*/

					uint64_t sn = 0;

					if(composition_time > _streams[i]->get_stream_offset())
						sn = _streams[i]->get_composition_sample_number(composition_time - 
						_streams[i]->get_stream_offset());


					_streams[i]->move_to_sample(sn);
					
				}
				else
				{
					_streams[i]->move(decoding_time);
				}
				
			}
		}

        _last_offset = 0;

	}
	///move keeping sample in the same order as they are stored
	///in the file
	bool next_file_chunks(sample_stream_info & ms)
	{
		int s(-1);

		int try_count(0);

		while(0 > s)
		{
			try_count++;

			//fail safe
			if(try_count > 15)
				return false;

			if(!_last_offset)
			{
				_last_offset = UINT64_MAX;

				for(unsigned int i = 0; i < _streams.size(); i++)
				{
					if(_streams[i]->get_sample_count())
					{
							if(_streams[i]->pick_sample(ms))
							{
								
								//fprintf(stdout
								//		, ">offset %I64u %I64u %I64u %d\r\n"
								//		, _last_offset
								//		, ms.offset
								//		, _UI64_MAX
								//		, s);

								if(ms.offset < _last_offset)
								{
									s = i;
									_last_offset = ms.offset;

									//fprintf(stdout
									//	, ">>>>last offset %I64u %d\r\n"
									//	, _last_offset
									//	, s);
								}
							}
							else
							{
								/*
								fprintf(stdout
										, ">cannot pick %d %d\r\n"
										, i
										, s);
								*/

								if(i == (_streams.size() -1) && _last_offset == UINT64_MAX)
								{
									return false;
								}
							}
					}//if sample_count
				}//for
			}//if
			else
			{
				for(unsigned int i = 0; i < _streams.size(); i++)
				{
					if(_streams[i]->get_sample_count())
					{
						if(_streams[i]->pick_sample(ms))
						{
							//fprintf(stdout
							//		, ">>last offset %I64u %d %I64u\r\n"
							//		, _last_offset
							//		, s
							//		, ms.offset);

							if(ms.offset == _last_offset)
							{
								s = i;
								break;
							}
						}//if sample_count
					}
				}//for

				if(0 > s)
				  _last_offset = 0;
			}//else

			//fprintf(stdout
			//					, ">>>last offset %I64u %d\r\n"
			//					, _last_offset
			//					, s);

		}//while

		_ASSERTE(-1 < s);

		if(-1 == s)
			ALXTHROW_T(_T("INVALID FILE MOVE"));

		_streams[s]->pick_sample(ms);
		_streams[s]->move_next();

		ms.stream = s;

		_last_offset += ms.size;

		return _last_offset <= _mdat_end;

	}

	int get_root_meta_count() const {return _meta_position.size();}
	uint64_t get_meta_position(const unsigned int idx) const {return _meta_position[idx];}

	const TCHAR * get_language(const unsigned int stream_number) 
	{
		_ASSERTE(stream_number < _streams.size());
		return _streams[stream_number]->get_language();
	}

	wmt_timecode get_interpolated_time_code(
		const uint64_t presentation_time
		, CMP4 &mp4
		)
	{
		_ASSERTE(HasLTC());
		int ltc_stream = LTCStream();
		_ASSERTE(-1 < ltc_stream);

		

		wmt_timecode tc;

		if(!LTC_has_one(ltc_stream))
		{
			tc.dwAmFlags  = TIMECODE_MISSING;
			tc.dwTimecode = TIMECODE_MISSING_VALUE;
			tc.dwUserbits = TIMECODE_MISSING;
			tc.wRange     = TIMECODE_MISSING;

			return tc;
		}

		uint64_t so = get_stream_offset(ltc_stream);
        uint64_t s  = 0;

		uint64_t pt = presentation_time;

		if(so < pt)
		{
			pt -= so;
			s = get_composition_sample_number(pt, ltc_stream);
		}
		else
			pt = 0;
				
		uint64_t t  = get_composition_time(s, ltc_stream);
		uint64_t p  = get_sample_offset(s, ltc_stream);
        
		uint64_t av = static_cast<CPM4LTCTrack*>(_streams[ltc_stream])->frame_avg_time();

		//_ASSERTE(s >= t);

		mp4.set_position(p);
		mp4.read_box(tc);

		if(TIMECODE_MISSING == tc.dwUserbits)
			return tc;

		if( (t-av) < pt && pt < (t + av) )
		{
			tc.dwUserbits = TIMECODE_READ;
		}
		else
		{
			tc.dwUserbits = TIMECODE_READ_INTERPOLATED;
			TC m(static_cast<uint32_t>(tc.dwTimecode));
			
			m.set_n_frames(
				static_cast<uint32_t>(10000000/ av)
				);

			if(pt > t)
			{

				TC bp(static_cast<int64_t>(pt - t));
				TC interpolated = m + bp;
				tc.dwTimecode = interpolated;

			}else
			{
				TC bp(static_cast<int64_t>(t - pt));
				TC interpolated = m - bp;
				tc.dwTimecode = interpolated;
			}
		}

		return tc;

	}

	
};

//#pragma optimize( "", on )


#ifdef WEBSTREAM

class MP4File: public CMP4
{
	FileBitstream * _p_ff;
	WebBitstream  * _p_wb;
public:
	MP4File():
	  _p_ff(NULL)
		  , _p_wb(NULL)
	{}
    void Close()
	{
		if(_p_ff)
			delete _p_ff;
		_p_ff = NULL;

		if(_p_wb)
			delete _p_wb;
		_p_wb = NULL;

		
	}
	virtual ~MP4File(){Close();}
	
	void Open(const TCHAR *pFileName)
	{
		_ASSERTE(NULL == _p_f);
		_ASSERTE(NULL == _p_ff);
		_ASSERTE(NULL == _p_wb);

		Cstring path(pFileName);

		if(Equals(path.subString(0, 7).toLower(), L"http://")
			|| Equals(path.subString(0, 7).toLower(), L"https:/")
			)
		{
			_p_wb = new WebBitstream();
			_p_wb->Open(pFileName);

			_p_f = _p_wb;
		}
		else
		{
			_p_ff = new FileBitstream(pFileName
				, BS_INPUT
				, 1024
				);
			
			_p_f = _p_ff;
		}
	}

};





typedef MP4File CMP4File;
#endif


__ALX_END_NAMESPACE

