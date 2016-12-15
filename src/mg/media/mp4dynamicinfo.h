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
#include "mp4dynamic.h"
#include <map>



#define CROSS_POINT_TIME INT64_MIN

enum stream_type{stream_video, stream_audio};

class IDynamicRenderer
{
public:
	virtual void begin(uint64_t duration) = 0;
	virtual void begin_stream(const TCHAR * psz_name, int points, int bitrates, stream_type stype) = 0;
	virtual void add_video_bitrate(int bit_rate, const TCHAR * psz_type, int Width, int Height, BYTE* codec_privet_data, int size) = 0;
	virtual void add_audio_bitrate(int bit_rate
		, int AudioTag, int SamplingRate, int Channels, int BitsPerSample, int PacketSize
		, BYTE* codec_privet_data, int size) = 0;
	virtual void add_point(uint64_t computed_time, uint64_t duration) = 0;
    virtual void add_point_info(const TCHAR * psz_path, uint64_t composition, uint64_t computed) = 0;
	virtual void add_point_info_cross(const TCHAR * psz_path, uint64_t composition, uint64_t computed) = 0;
	virtual void end_point() = 0;
	virtual void end_stream() = 0;
	virtual void end() = 0;
};

class CCodecPrivateData
{
	CBuffer<unsigned char> _codec_private_data;
public:

	void AddCodecPrivateData(const unsigned char* p_data, int size)
	{
		_codec_private_data.add(p_data, size);
	}

	unsigned char * CodecPrivateData(){return _codec_private_data.get();}
	uint32_t CodecPrivateDataSize() {return _codec_private_data.size();}

	CCodecPrivateData():_codec_private_data(1024)
	{}
};

class VideoInfo
{
public:
	int Width, Height;
};

class DynamicBitrate: public CCodecPrivateData
{
	std::map<int64_t, Cstring> _paths;

public:
	
	void Add(int64_t start_computed_time, const Cstring & path)
	{
		if(0 == _paths.size() && start_computed_time > 0)
			_paths[0] = path;

		
		if(_paths.end() == _paths.find(start_computed_time))
			_paths[start_computed_time] = path;
	}

	void rebase(int64_t offset)
	{
		std::map<int64_t, Cstring> paths;

		std::map<int64_t, Cstring>::iterator iter;

		for(iter = _paths.begin(); iter != _paths.end(); iter++)
		{
			paths[iter->first + offset] = iter->second;
		}

		_paths.clear();


		for(iter = paths.begin(); iter != paths.end(); iter++)
		{
			_paths[iter->first] = iter->second;
		}

	}

	const Cstring & get_path(uint64_t computed_time) const
	{
		/*
		std::map<int64_t, Cstring>::const_iterator it = _paths.lower_bound(computed_time);

		if(it == _paths.end())
		   it--;

		 */

		std::map<int64_t, Cstring>::const_iterator it = _paths.lower_bound(computed_time);
		
		if(it != _paths.begin() && (it == _paths.end() || it->first != computed_time))
		   it--;

		return it->second;
	}

	const std::map<int64_t, Cstring>::const_iterator begin() const
	{return _paths.begin();}

	const std::map<int64_t, Cstring>::const_iterator end() const
	{return _paths.end();}

	DynamicBitrate()
	{}
};


class DynamicBitrateVideo: public DynamicBitrate, public VideoInfo
{
	
};

//class DynamicStream;

struct CrossPoint
{
	uint64_t computed_time;
	uint64_t composition;
	uint64_t computed_time_2;
	uint64_t composition_2;
};

class DynamicPoints
{
	std::map<int64_t, int64_t> _computed;

	std::map<int64_t, CrossPoint> _computed_cross;
	

public:
	DynamicPoints()
	{}

	void Add(int64_t computed_time, int64_t composition)
	{
		_computed[computed_time] = composition;
	}

	void AddCross(uint64_t computed_time
		, uint64_t composition
		, uint64_t computed_time_2
		, uint64_t composition_2)
	{
		_computed[computed_time] = CROSS_POINT_TIME;
		
		CrossPoint cp = {computed_time, composition, computed_time_2, composition_2};
		
		_computed_cross[computed_time] = cp;
	}

	const CrossPoint & get_cross_point(uint64_t computed_time) const
	{
		//return _computed_cross[computed_time];

		std::map<int64_t, CrossPoint>::const_iterator iter = _computed_cross.find(computed_time);

		if(_computed_cross.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}

		return iter->second;
	}

	inline bool is_cross_point(uint64_t computed_time) const
	{
		//return (_computed[computed_time] == CROSS_POINT_TIME);

		std::map<int64_t, int64_t>::const_iterator iter = _computed.find(computed_time);

		if(_computed.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}

		return (iter->second == CROSS_POINT_TIME);
	}
	
	int64_t get_grater_or_equal(uint64_t computed_time) const
	{
		std::map<int64_t, int64_t>::const_iterator iter = _computed.lower_bound(computed_time);

		if(_computed.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}

		_ASSERTE(iter->first >= static_cast<int64_t>(computed_time));

		return iter->first;
	}

	int64_t get_closest(uint64_t computed_time) const
	{
		std::map<int64_t, int64_t>::const_iterator iter = _computed.lower_bound(computed_time);

		if(_computed.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}

		_ASSERTE(iter->first >= static_cast<int64_t>(computed_time));
		_ASSERTE(_computed.end() != iter);

		uint64_t target = iter->first;
		         int64_t dist   = target - computed_time;

		if(_computed.begin() != iter)
		{
			iter--;

			if(iter->first > static_cast<int64_t>(computed_time) && ((iter->first - static_cast<int64_t>(computed_time)) < dist))
			{
				target = iter->first;
				dist   = target - computed_time;
			}

			if(iter->first <= static_cast<int64_t>(computed_time) &&((static_cast<int64_t>(computed_time) - iter->first) < dist))
			{
				target = iter->first;
				dist = computed_time - target;

			}
		}

		iter++;
		iter++;

		if(_computed.end() != iter)
		{
			if(iter->first > static_cast<int64_t>(computed_time) && ((iter->first - static_cast<int64_t>(computed_time)) < dist))
			{
				target = iter->first;
				dist   = target - computed_time;
			}

			if(iter->first <= static_cast<int64_t>(computed_time) &&((static_cast<int64_t>(computed_time) - iter->first) < dist))
			{
				target = iter->first;
				dist = computed_time - target;

			}
		}		

		return target;
	}
	
	int64_t get(uint64_t computed_time) const 
	{
		std::map<int64_t, int64_t>::const_iterator iter = _computed.find(computed_time);

		if(_computed.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}

		return iter->second;
	}

	int64_t get_end(uint64_t computed_time) const
	{
		std::map<int64_t, int64_t>::const_iterator iter = _computed.find(computed_time);
		
		int64_t orig = INT64_MAX;
		int64_t comp = INT64_MAX;

		_ASSERTE(_computed.end() != iter);
		
		//if(_computed.end() != iter)
		//{
			orig = iter->second;
			comp = iter->first;
			iter++;
		//}
		

		if(orig > iter->second //changed file
			) 
		{
			//return INT64_MAX; //get all file to the end

			//_ASSERTE(_computed.end() != iter);
			_ASSERTE(comp != iter->first);
			_ASSERTE(comp < iter->first);

			return orig + (iter->first - comp);

		}

		return iter->second;
	}

	int64_t get_end_computed(uint64_t computed_time) const
	{
		std::map<int64_t, int64_t>::const_iterator iter = _computed.find(computed_time);

		int64_t orig = INT64_MAX;

		if (_computed.end() != iter)
		{
			orig = iter->first;
			iter++;
		}

		return iter->first;
	}

	int Count() const {return _computed.size();}

	int64_t mint() const
	{
		if(!_computed.size())
			return 0;

		return _computed.begin()->first;
	}
	int64_t maxt() const 
	{
		if(!_computed.size()) return 0;

		std::map<int64_t, int64_t>::const_iterator iter = _computed.end();
				
		iter--;
		
		return iter->first;
	}

	void rebase(int64_t offset)
	{
		std::map<int64_t, int64_t> computed;

		std::map<int64_t, int64_t>::iterator iter;

		for( iter = _computed.begin(); iter != _computed.end(); iter++ ) 
		{
			computed[iter->first + offset] = iter->second;
		}

		_computed.clear();

		for( iter = computed.begin(); iter != computed.end(); iter++ ) 
		{
			_computed[iter->first] = iter->second;
		}


		///////////////////////////////////////////////
		std::map<int64_t, CrossPoint> cross;

		std::map<int64_t, CrossPoint>::iterator icross;

		for(icross = _computed_cross.begin(); icross != _computed_cross.end(); icross++ )
		{
			cross[icross->first + offset] = icross->second;
		}

		_computed_cross.clear();

		for( icross = cross.begin(); icross != cross.end(); icross++)
		{
			_computed_cross[icross->first] = icross->second;
		}

		
	}

	void render(IDynamicRenderer * renderer, DynamicBitrate ** pp_bit_rate = NULL, int bit_rate_count = 1)
	{
		 
		std::map<int64_t, int64_t>::iterator iter;

		int64_t t  = _computed.begin()->first;
		int64_t te = _computed.begin()->second;

		for( iter = _computed.begin(); iter != _computed.end(); iter++ ) 
		{
			if(iter != _computed.begin())
			{
				renderer->add_point(t, iter->first - t);
				if(NULL != pp_bit_rate)
				{
					//must loop into the array
					//renderer->add_point_info(p_bit_rate->get_path(iter->first), te);
					
					for(int j = 0; j < bit_rate_count; j++)
					{
						if(is_cross_point(t))
						{
							CrossPoint cp = get_cross_point(t);

							renderer->add_point_info_cross(pp_bit_rate[j]->get_path(cp.computed_time), cp.composition, cp.computed_time);
							renderer->add_point_info_cross(pp_bit_rate[j]->get_path(cp.computed_time_2), cp.composition_2, cp.computed_time_2);
						}
						else
						{
							renderer->add_point_info(pp_bit_rate[j]->get_path(t), te, t);
						}
					}


				}
				
				renderer->end_point();

				t  = iter->first;
				te = iter->second;

			}
		}

	        	

	}

	uint64_t duration() const 
	{
		/*
		std::map<int64_t, int64_t>::iterator iter = _computed.end();
		iter--;

		return iter->first - _computed.begin()->first;
		*/

		return maxt() - mint();
	}

	const std::map<int64_t, int64_t>::const_iterator begin() const {return _computed.begin();}
	const std::map<int64_t, int64_t>::const_iterator end() const {return _computed.end();}
	
};



class DynamicStream: public CCodecPrivateData
{
	
	DynamicPoints       _points;

	uint64_t    _current_end;

protected:

	std::map<uint64_t, DynamicBitrate *> _bitrates;
	

	virtual void render_type(IDynamicRenderer * renderer)
	{
	}


	virtual DynamicBitrate * CreateBitRate() 
	{
		//DynamicBitrateVideo

		return new DynamicBitrate();
	}
public:
	int id;

	Cstring name;

	DynamicStream(): _current_end(0)
	{}

	virtual ~DynamicStream()
	{
		std::map<uint64_t, DynamicBitrate *>::iterator iter;   
		for( iter = _bitrates.begin(); iter != _bitrates.end(); iter++ ) 
		{
			   delete iter->second;
		}

		_bitrates.clear();
	}

	DynamicBitrate * AddBitRate(uint64_t bit_rate)
	{
		DynamicBitrate * p_bitrate = CreateBitRate();

		_bitrates[bit_rate] = p_bitrate;

		return p_bitrate;
	}

	void AddPoint(int64_t computed_time, int64_t composition)
	{
		_points.Add(computed_time, composition);
	}

	void AddPointCross(int64_t computed_time, int64_t composition, int64_t computed_time_2, int64_t composition_2)
	{
		_points.AddCross(computed_time, composition, computed_time_2, composition_2);
	}

	inline bool is_cross_point(uint64_t computed_time) const
	{
		return _points.is_cross_point(computed_time);
	}

	const CrossPoint & get_cross_point(uint64_t computed_time) const
	{
		return _points.get_cross_point(computed_time);
	}

	int64_t mint() const {return _points.mint();}
	int64_t maxt() const {return _points.maxt();}

	void set_current_end(uint64_t rhs){_current_end = rhs;}
	uint64_t get_current_end() const 
	{

		if(static_cast<int64_t>(_current_end) < maxt())
			return maxt();

		return _current_end;
	}

	void rebase(int64_t offset)
	{
		_points.rebase(offset);

		std::map<uint64_t, DynamicBitrate *>::iterator iter;   
		for( iter = _bitrates.begin(); iter != _bitrates.end(); iter++ ) 
		{
			iter->second->rebase(offset);
		}
	}

		
	DynamicBitrate * get_bit_rate(uint64_t bit_rate)
	{
		return _bitrates[bit_rate];
	}

	/*void AddFile(const Cstring & path)
	{
		
		std::map<uint64_t, DynamicBitrate *>::iterator iter;   
		for( iter = _bitrates.begin(); iter != _bitrates.end(); iter++ ) 
		{
			iter->second->Add(_max_computed, path);
		}
	}
	*/

	const Cstring & get_path(uint64_t bitrate, uint64_t computed_time) const
	{
		std::map<uint64_t, DynamicBitrate *>::const_iterator iter = _bitrates.find(bitrate);

		if(_bitrates.end() == iter)
		{
			ALXTHROW_LASTERR1(E_INVALIDARG );
		}
		
		return iter->second->get_path(computed_time);
	}

	uint64_t get_original_time(uint64_t computed_time) const
	{
		return _points.get(computed_time);
	}

	uint64_t get_end_original_time(uint64_t computed_time) const
	{
		return _points.get_end(computed_time);
	}

	uint64_t get_end_computed_time(uint64_t computed_time) const
	{
		return _points.get_end_computed(computed_time);
	}

	/*int64_t get_grater_or_equal(uint64_t computed_time) const
	{
		return _points.get_grater_or_equal(computed_time);
	}*/

	int64_t get_closest(uint64_t computed_time) const
	{
		return _points.get_closest(computed_time);
	}
	
	void render(IDynamicRenderer * renderer)
	{

		stream_type stype = stream_type::stream_video;

		if(this->is_audio())
			stype = stream_type::stream_audio;

		renderer->begin_stream(name, _points.Count(), _bitrates.size(), stype);
		render_type(renderer);

		CBuffer<DynamicBitrate*> dinamic_bitrates(_bitrates.size());

		std::map<uint64_t, DynamicBitrate *>::const_iterator iter = begin();

		while(end() != iter)
		{
			DynamicBitrate * db = iter->second;
			dinamic_bitrates.add(&db, 1);

			iter++;
		}

		_points.render(renderer, dinamic_bitrates.get(), dinamic_bitrates.size());

		renderer->end_stream();
	}

	uint64_t duration() const {return _points.duration();}

	int get_point_count() const {return _points.Count();}

	const std::map<int64_t, int64_t>::const_iterator get_point_begin() const
	{return _points.begin();}

	const std::map<int64_t, int64_t>::const_iterator get_point_end() const 
	{return _points.end();}

	const std::map<uint64_t, DynamicBitrate *>::const_iterator begin() const {return _bitrates.begin();}
	const std::map<uint64_t, DynamicBitrate *>::const_iterator end() const {return _bitrates.end();}

	int get_bitrate_count() const { return _bitrates.size();}
	
	virtual bool is_video() const{return false;}
	virtual bool is_audio() const{return false;}

	
};

class DynamicStreamAudio: public DynamicStream
{
public:
    int AudioTag;
	int SamplingRate;
	int Channels;
	int BitsPerSample;
	int	PacketSize;


	DynamicStreamAudio()
	: AudioTag(255)
	, BitsPerSample(16)
	, PacketSize(4)
	{
	}

	virtual void render_type(IDynamicRenderer * renderer)
	{
		std::map<uint64_t, DynamicBitrate *>::iterator iter;   
		for( iter = _bitrates.begin(); iter != _bitrates.end(); iter++ ) 
		{
			renderer->add_audio_bitrate(static_cast<int32_t>(iter->first)
				, 255, SamplingRate, Channels, BitsPerSample, 4, iter->second->CodecPrivateData()
				, iter->second->CodecPrivateDataSize());
		}

	}

	virtual bool is_audio() const {return true;}

	virtual ~DynamicStreamAudio(){}

};

class DynamicStreamVideo: public DynamicStream, public VideoInfo
{
protected:

	virtual DynamicBitrate * CreateBitRate() 
	{
		return new DynamicBitrateVideo();
	}
public:
	 
	

	virtual void render_type(IDynamicRenderer * renderer)
	{
		std::map<uint64_t, DynamicBitrate *>::iterator iter;   
		for( iter = _bitrates.begin(); iter != _bitrates.end(); iter++ ) 
		{
			DynamicBitrateVideo * p_dynamic_video = static_cast<DynamicBitrateVideo *>(iter->second);

			renderer->add_video_bitrate(static_cast<int32_t>(iter->first)
				, _T("AVC1")
				, p_dynamic_video->Width
				, p_dynamic_video->Height
				, p_dynamic_video->CodecPrivateData()
				, p_dynamic_video->CodecPrivateDataSize()
				);
		}
	
	}

	virtual bool is_video() const {return true;}

	virtual ~DynamicStreamVideo(){}
};

class DynamicPresentation
{
	std::map<Cstring, DynamicStream *> _streams;

public:

	~DynamicPresentation()
	{
		std::map<Cstring, DynamicStream *>::iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			   delete iter->second;
		}

		_streams.clear();
	}

	DynamicStreamVideo * AddVideoStream(const TCHAR * pszname, int Width, int Height, int id)
	{
		DynamicStreamVideo * pv = new DynamicStreamVideo();

		pv->Width  = Width;
		pv->Height = Height;

		pv->name = pszname;

		pv->id   = id;

		Cstring name = pszname;

		_ASSERTE( _streams.end() == _streams.find(name));

		_streams.insert(std::pair<Cstring, DynamicStream *>(name, pv) );

		//_streams[name] = pv;

		return pv;
	}

	DynamicStreamAudio * AddAudioStream(const TCHAR * pszname, int SamplingRate, int Channels, int BitsPerSample, int id)
	{
		DynamicStreamAudio * pv = new DynamicStreamAudio();

		pv->SamplingRate  = SamplingRate;
		pv->Channels      = Channels;
		pv->BitsPerSample = BitsPerSample;

		pv->name = pszname;
		pv->id   = id;

		Cstring name = pszname;

		_ASSERTE( _streams.end() == _streams.find(name));

		_streams.insert(std::pair<Cstring, DynamicStream *>(name, pv) );

		//_streams[name] = pv;

		return pv;
	}

	DynamicStream      * get(const TCHAR * pszname) const 
	{
		std::map<Cstring, DynamicStream *>::const_iterator iter = _streams.find(pszname);

		if (_streams.end() == iter)
			return NULL;
		
		return iter->second;
	}
	DynamicStream      * get(int id) const{

		std::map<Cstring, DynamicStream *>::const_iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			   if(id == iter->second->id)
				   return iter->second;
		}

		return NULL;
	}

	
	int Count() const
	{
		return _streams.size();
	}

	int audio_count() const
	{
		int c(0);
		std::map<Cstring, DynamicStream *>::const_iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			    if(iter->second->is_audio())
					c++;
		}

		return c;
	}

	int video_count() const
	{
		int c(0);
		std::map<Cstring, DynamicStream *>::const_iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			    if(iter->second->is_video())
					c++;
		}

		return c;
	}

	DynamicStream      * get_by_index(int id) {
		int c(0);
		std::map<Cstring, DynamicStream *>::iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			   //if(c++ == idx)

			    if(id == iter->second->id)
				  return iter->second;
		}

		return NULL;
	}

	const DynamicStream      * get_by_index(int id) const {
		return const_cast<DynamicPresentation*>(this)->get_by_index(id);
	}

	/*
	void AddFile(const Cstring & path)
	{
		std::map<Cstring, DynamicStream *>::iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			   iter->second->AddFile(path);
		}
	}
	*/

	void rebase()
	{
		int64_t min = INT64_MAX;
		for(int i = 0; i < Count(); i++)
		{
			int64_t mint = get_by_index(i)->mint();

			if(mint < min)
				min = mint;
		}

		if(0 > min)
		{
			for(int i = 0; i < Count(); i++)
			{
				get_by_index(i)->rebase(min * -1);
			}
			
		}
	}
	void render(IDynamicRenderer * renderer)
	{
		renderer->begin(get_by_index(0)->duration());
		
		std::map<Cstring, DynamicStream *>::iterator iter;   
		for( iter = _streams.begin(); iter != _streams.end(); iter++ ) 
		{
			   iter->second->render(renderer);
		}
		

		renderer->end();
	}
};



class CMP4DynamicInfo :
	public CMP4DynamicDiscreate
{
	DynamicPresentation _presentation;
	bool _bit_rates;
	
public:
	CMP4DynamicInfo(void);
	virtual ~CMP4DynamicInfo(void);


	   virtual void publish_segmented_composition(uint64_t composition_time,
						uint64_t computed_time,
						int stream)
		{
			_presentation.get_by_index(stream)->AddPoint(computed_time, composition_time);
		}

		virtual void publish_segmented_composition_cross(uint64_t composition_time,
			uint64_t computed_time,
			uint64_t composition_time_2,
			uint64_t computed_time_2,
			int stream)
		{
			_presentation.get_by_index(stream)->AddPointCross(computed_time
				, composition_time
				, computed_time_2 
				, composition_time_2);
		}

		
		virtual void begin_processing(dynamic_item * p_dynamic_items, int dynamic_items_size
			, int streams, MP4Reader & reader)
		{

			//TODO: HANDLE MULTILANGUAGE
			
			if(!_bit_rates)
			{
				for(int i = 0; i < streams; i++)
				{
					for(int k = 0; k < dynamic_items_size; k++)
					{
						uint64_t bitrate = p_dynamic_items[k].bitrate;

						DynamicStream * pStream = _presentation.get_by_index(i);

						if(pStream)
						{
						
							if(pStream->is_audio())
							{
								bitrate = p_dynamic_items[k].audio_bitrate;
							}

						DynamicBitrate * p_dynamic_bitrate = NULL;//_presentation.get_by_index(i)->AddBitRate(bitrate);

						//--------------------/////////////////----------------//
						 						
						if(0 == k)
						{
							p_dynamic_bitrate = _presentation.get_by_index(i)->AddBitRate(bitrate); 

							p_dynamic_bitrate->AddCodecPrivateData(
								 _presentation.get_by_index(i)->CodecPrivateData()
								 ,  _presentation.get_by_index(i)->CodecPrivateDataSize()
								 );

							if(_presentation.get_by_index(i)->is_video())
							{
								DynamicStreamVideo  * p_stream_video  = static_cast<DynamicStreamVideo  *>(_presentation.get_by_index(i));
								DynamicBitrateVideo * p_dynamic_video = static_cast<DynamicBitrateVideo *>(p_dynamic_bitrate);
								p_dynamic_video->Width  = p_stream_video->Width;
								p_dynamic_video->Height = p_stream_video->Height;
							}
						}
						else
						{

							//MP4File mp4;
							SYNCMP4File mp4;
							mp4.open(p_dynamic_items[k].psz_path);

							MP4Reader reader;
							reader.parse(mp4);


							if(_presentation.get_by_index(i)->is_video())
							{
								p_dynamic_bitrate = _presentation.get_by_index(i)->AddBitRate(bitrate);
								DynamicBitrateVideo * p_dynamic_video = static_cast<DynamicBitrateVideo *>(p_dynamic_bitrate);

								if (reader.stream_count() <= i)
									ALXTHROW("invalid file stream count");

								if (!reader.IsVisual(i))
									ALXTHROW("invalid file video stream");
								
								const BYTE*    sps_nal_source     = reader.get_visual_entry(0, i).get_avcc_header().get_nal_sequence(0);
								const unsigned int sps_nal_size   = reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size(0);
								const BYTE*	 pps_nal_source       = reader.get_visual_entry(0, i).get_avcc_header().get_nal_picture(0);
								const unsigned int pps_nal_size   = reader.get_visual_entry(0, i).get_avcc_header().get_picture_size(0);
								const unsigned int width          = reader.get_visual_entry(0, i).get_entry().width;
								const unsigned int height         = reader.get_visual_entry(0, i).get_entry().height;  
									//reader.get_media_time_scale(i)
								

								DWORD extrasize = sps_nal_size
												+ pps_nal_size
												+ 4;
										

								ALX::CBuffer<BYTE> extra(extrasize);
								BYTE *out = extra.get();
									
								compute_video_private_data(sps_nal_source
											, sps_nal_size
											, pps_nal_source
											, pps_nal_size
											, out);

								p_dynamic_video->AddCodecPrivateData(out, extrasize);
								p_dynamic_video->Width  = width;
								p_dynamic_video->Height = height;

							}//if video stream
						}//if k = 0
					}//if(pStream)
					}//for k
				}//for i
				_bit_rates = true;
			}// !_bit_rates

			
			
			for(int i = 0; i < streams; i++)
			{
				
					for(int k = 0; k < dynamic_items_size; k++)
					{
						uint64_t bitrate = p_dynamic_items[k].bitrate;
					
						DynamicStream * pStream = _presentation.get_by_index(i);

						if(pStream)
						{
							//_presentation.get_by_index(i)->AddBitRate(reader.get_stream_bit_rate(i)/1000*1000);
							if(pStream->is_audio())
							{
						
								bitrate = p_dynamic_items[k].audio_bitrate;
							}

							DynamicBitrate * p_dynamic_bitrate = _presentation.get_by_index(i)->get_bit_rate(bitrate);

							if (NULL == p_dynamic_bitrate)
								ALXTHROW("invalid bitrate specified");
						
							//if (pStream->is_video() || 0 == k)
								p_dynamic_bitrate->Add(/*_presentation.get_by_index(i)->maxt()*/
									  _presentation.get_by_index(i)->get_current_end()
									, p_dynamic_items[k].psz_path);


						}//if(pStream)

						
					}
				
			}
			
		}

		virtual void video_private_data(const unsigned char* p_private_data, long size, int stream
			, const unsigned int width
			, const unsigned int height)
		{
			Cstring v(_T("video"));
					//v += stream;

			if(0 < _presentation.video_count())
				v += _presentation.video_count();					

			_presentation.AddVideoStream(v, width, height, stream)->AddCodecPrivateData(p_private_data, size);
		}

		virtual void audio_private_data(const unsigned char* p_private_data, long size, int stream
			, const unsigned int sample_rate
			, const unsigned int channels
			, const unsigned int target_bit_rate)
		{
			Cstring v = _T("audio");
					//v += stream;

			if(0 < _presentation.audio_count())
				v += _presentation.audio_count();

			_presentation.AddAudioStream(v, sample_rate, channels, 16, stream)->AddCodecPrivateData(p_private_data, size);
		}


	void render(IDynamicRenderer * renderer)
	{
		_presentation.rebase();
		_presentation.render(renderer);
	}

	const DynamicPresentation & get_presentation() const {return _presentation;}

	virtual void end_processing()
	{
	
		    CMP4DynamicDiscreate::end_processing();

			//let inform each stream of its end time		
			for(int i = 0; i < _presentation.Count(); i++)
			{				
					_presentation.get_by_index(i)->set_current_end(
							get_stream_end(i) 
								+ this->_stream_start_offset[i]
						);
			}

	}

	virtual void End()
	{
		CMP4DynamicDiscreate::End();

		//WIP: foreach stream => _presentation.get_by_index(stream)->AddPoint(computed_time, composition_time);
		//
		for(int i = 0; i < _presentation.Count(); i++)
		{		
			_presentation.get_by_index(i)->AddPoint(
					  get_stream_end(i)
					, /*INT64_MAX //this is a closing point. it should never be used.
						     //it's just there to allow computing the duration of
						     //the real last one
					 */
					 get_stream_composition_end(i)
					);
		}
	}

};


class SSFRenderer: public IDynamicRenderer
{
	 Cstring _ssf_xml;

	 Cstring _tmp_str_name;
	 int _points;
	 int _bitrates;
	 int _index;

	 bool _begin_stream;

	 bool _use_stream_name;

public:
	SSFRenderer() :_ssf_xml(10480), _index(-1), _use_stream_name(true)
	{
	}

	virtual void begin(uint64_t duration)
	{
		_ssf_xml += _T("<SmoothStreamingMedia MajorVersion=\"2\" MinorVersion=\"1\" Duration=\"");
		_ssf_xml += duration;
		_ssf_xml += _T("\">\r\n");

	}

	virtual void begin_stream(const TCHAR * psz_name, int points, int bitrates, stream_type stype)
	{
		_tmp_str_name = psz_name;
		_points       = points;
		_bitrates     = bitrates;
		_begin_stream = true;

		Cstring psz_type     = _T("video");
		Cstring psz_type_ext = _T(".m4v");

		if(stream_type::stream_audio == stype)
		{
			psz_type = _T("audio");
			psz_type_ext = _T(".m4a");
		}		

		_ssf_xml += _T("<StreamIndex Type=\"");
		_ssf_xml += psz_type;
		_ssf_xml += _T("\" Url=\"");
		_ssf_xml += (_use_stream_name)?_tmp_str_name:psz_type;
		_ssf_xml += _T("_{bitrate}_{start time}");
		_ssf_xml += psz_type_ext;
		_ssf_xml += "\" Name=\"";
		_ssf_xml += _tmp_str_name;
		_ssf_xml += "\" Chunks=\"";
		_ssf_xml += _points;
		_ssf_xml += "\" QualityLevels=\"";
		_ssf_xml += _bitrates;
		_ssf_xml += "\">\r\n";

		
	}

	virtual void add_video_bitrate(int bit_rate, const TCHAR * psz_type, int Width, int Height, BYTE* codec_privet_data, int size)
	{
		_index++;

		CBuffer<unsigned char> codec_private_data(size + 4);

		unsigned int sequence_size(0);
		             sequence_size = codec_privet_data[0] << 8 | codec_privet_data[1];

		unsigned char * p_data = codec_private_data.get();

		p_data[0] = 0x00;
		p_data[1] = 0x00;
		p_data[2] = 0x00;
		p_data[3] = 0x01;

		p_data[sequence_size + 4] = 0x00;
		p_data[sequence_size + 5] = 0x00;
		p_data[sequence_size + 6] = 0x00;
		p_data[sequence_size + 7] = 0x01;

		::memcpy(p_data + 4, codec_privet_data + 2, sequence_size);
		::memcpy(p_data + sequence_size + 8, codec_privet_data + 4 + sequence_size, size - (4 + sequence_size));

		_ssf_xml += _T("<QualityLevel Index=\"");
		_ssf_xml += _index;
		_ssf_xml += _T("\" Bitrate=\"");
		_ssf_xml += bit_rate;
		_ssf_xml += _T("\" CodecPrivateData=\"");
		_ssf_xml.append_hex_buffer(p_data, size + 4);
		_ssf_xml += _T("\" FourCC=\"AVC1\" MaxWidth=\"");
		_ssf_xml += Width;
		_ssf_xml += _T("\" MaxHeight=\"");
		_ssf_xml += Height;
		_ssf_xml += _T("\" />\r\n");

	}
	virtual void add_audio_bitrate(int bit_rate
		, int AudioTag, int SamplingRate, int Channels, int BitsPerSample, int PacketSize
		, BYTE* codec_privet_data, int size)
	{
		_index++;

		/*
		_ssf_xml += _T("<StreamIndex Type=\"audio\" Url=\"audio_{bitrate}_{start time}.m4f\" Name=\"");
		_ssf_xml += _tmp_str_name;
		_ssf_xml += "\" Chunks=\"";
		_ssf_xml += _points;
		_ssf_xml += "\" QualityLevels=\"";
		_ssf_xml += "1\">\r\n";
		*/

		_ssf_xml += _T("<QualityLevel Index=\"");
		_ssf_xml += _index;
		_ssf_xml += _T("\" Bitrate=\"");
		_ssf_xml += bit_rate;
		_ssf_xml += _T("\" FourCC=\"\" AudioTag=\"255\" CodecPrivateData=\"\"  Channels=\"");
		_ssf_xml += Channels;
		_ssf_xml += _T("\" SamplingRate=\"");
		_ssf_xml += SamplingRate;
		_ssf_xml += _T("\" BitsPerSample=\"");
		_ssf_xml += BitsPerSample;
		_ssf_xml += _T("\" PacketSize=\"4\"");
		_ssf_xml += _T("/>\r\n");
	}
	virtual void add_point(uint64_t computed_time, uint64_t duration)
	{
		_ssf_xml += _T("<c d=\"");
		_ssf_xml += duration;
		
		if(_begin_stream && 0 <= computed_time )
		{
			_begin_stream = false;
			
			_ssf_xml += "\" t=\"";
			_ssf_xml += computed_time;


		}

		_ssf_xml += _T("\">");
		
	}
    virtual void add_point_info(const TCHAR * psz_path, uint64_t composition, uint64_t computed)
	{
		_ssf_xml += _T("<h p=\"");
		_ssf_xml += psz_path;
		_ssf_xml += _T("\" t=\"");
		_ssf_xml += composition;
		_ssf_xml += _T("\" c=\"");
		_ssf_xml += computed;
		_ssf_xml += _T("\" k=\"");
		_ssf_xml += HNS(computed);
		_ssf_xml += _T("\" />");
	}
	virtual void add_point_info_cross(const TCHAR * psz_path, uint64_t composition, uint64_t computed)
	{
		_ssf_xml += _T("<cp p=\"");
		_ssf_xml += psz_path;
		_ssf_xml += _T("\" t=\"");
		_ssf_xml += composition;
		_ssf_xml += _T("\" c=\"");
		_ssf_xml += computed;
		_ssf_xml += _T("\" k=\"");
		_ssf_xml += HNS(computed);
		_ssf_xml += _T("\" />");
	}
	virtual void end_point()
	{
		_ssf_xml += _T("</c>\r\n");
	}
	virtual void end_stream()
	{
		_ssf_xml += _T("</StreamIndex>\r\n");
	}
	virtual void end()
	{
		_ssf_xml += _T("</SmoothStreamingMedia>");
	
	}

	Cstring & xml(){return _ssf_xml;}

	void set_use_stream_name(bool rhs){ _use_stream_name = rhs; }
};


