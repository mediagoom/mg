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

class MOOFReader
{

	MovieFragmentHeaderBox _fbox;
	TrackFragmentHeaderBox _tbox;
	TrackFragmentBaseMediaDecodeTimeBox _bbox;
	TrackRunBox _trunbox;

	SSFLIVE_BODY _TfxdBox;
	std::vector<SSFLIVE_BODY*> _TfrfBox;

	uint64_t _mdat_position;
	uint64_t _moof_position;

	bool _has_bbox;
	bool _has_tfxd;

#ifdef CENC
	SampleAuxiliaryInformationOffsetsBox _saio;
	SampleAuxiliaryInformationSizesBox   _saiz;
	TrackEncryptionBox                   _tenc;
	SampleEncryptionBox                  _senc;

	bool _has_saio;
	bool _has_saiz;
	bool _has_tenc;
	bool _has_senc;
#endif

protected:

	virtual void unknown_box(CMP4 &mp4)
	{
		_ASSERTE(false);
		mp4.skip_current();
	}

private:
	void parse_boxes(CMP4 &mp4, bool strict = false)
	{
		Cstring g;

		switch(mp4.get_box().get_type())
		{
			case box_free:
				mp4.set_position(mp4.get_box_position() + mp4.get_box().get_size());
				break;
			case box_MOOF:
				_moof_position = mp4.get_box_position();
				break;
			case box_TRAF:
				break;
			case box_TFHD:
				mp4.parse_track_fragment_header_box(_tbox);
				break;
			case box_MFHD:
				mp4.parse_movie_fragment_header_box(_fbox);
				break;
			case box_TFDT:
				mp4.read_box(_bbox);
				_has_bbox = true;
				break;
			case box_TRUN:
				_trunbox.cleanup();
				mp4.parse_track_run_box(_trunbox);
				break;
			case box_mdat:
				_mdat_position = mp4.get_position();
				mp4.skip_current();
				break;
			case box_AVCN:
				mp4.skip_current();
				break;
			case box_SDTP:
				mp4.skip_current();
				break;
#ifdef CENC
			case box_saio:
				mp4.read_box(_saio);
				_has_saio = true;
				break;
			case box_saiz:
				mp4.read_box(_saiz);
				_has_saiz = true;
				break;
			case box_tenc:
				mp4.read_box(_tenc);
				_has_tenc = true;
				break;
			case box_senc:
				_senc.cleanup();
				mp4.read_box(_senc);
				_has_senc = true;
				break;
#endif
			case box_uuid:

				//TCHAR g[200];

				//int r = StringFromGUID2(reinterpret_cast<const GUID *>(mp4.get_box().guid()), g, 200);

				
						g.append_hex_buffer(mp4.get_box().guid(), 16);

						//TODO: make faster

						if(ALX::Equals(_T("6D1D9B0542D544E680E2141DAFF757B2"), g))
						{
							mp4.do_full_box();

							mp4.read_box(_TfxdBox, mp4.get_full_box().get_version());

							_has_tfxd = true;
							
						}

						if(ALX::Equals(_T("D4807EF2CA3946958E5426CB9E46A79F"), g))
						{
							mp4.do_full_box();
							
							unsigned char sample_count(0);

							mp4.read_bytes(&sample_count, 1);

							for(unsigned char i = 0; i < sample_count; i++)
							{
								SSFLIVE_BODY* pTfrfBox = new SSFLIVE_BODY;

								mp4.read_box(*pTfrfBox, mp4.get_full_box().get_version());

								_TfrfBox.push_back(pTfrfBox);
							}

						}

				//mp4.skip_current();
				break;
			default:
				unknown_box(mp4);
				break;
		}

		if(!mp4.eof() && (!_mdat_position || strict))
		{
		  mp4.do_box();
		  parse_boxes(mp4, strict);
		}
	}


public:

	MOOFReader():_has_bbox(false)
		, _has_tfxd(false)
#ifdef CENC
		, _has_saio(false)
		, _has_saiz(false)
		, _has_tenc(false)
		, _has_senc(false)
#endif
	{
	}

	~MOOFReader()
	{
		for(unsigned int i = 0; i < _TfrfBox.size(); i++)
			delete _TfrfBox[i];

		_TfrfBox.clear();
	}

	void parse(CMP4 &mp4, bool strict = true)
	{
		if(strict)
		{
			if(0 != mp4.get_position())
				mp4.set_position(0);
		}

		bool box = mp4.do_box();

		if(strict)
		{
			MP4CHECK(box_MOOF);
		}
		else
		{
			_mdat_position = 0;
		}

		if(box)
			parse_boxes(mp4, strict);
	}

	
	bool has_TrackFragmentBaseMediaDecodeTimeBox() const {return _has_bbox;}
	bool has_tfxd() const {return _has_tfxd;}
	bool has_tfrf() const {return 0 < _TfrfBox.size();}

	int get_tfrf_size() const {return _TfrfBox.size();}
	int64_t get_tfrf_time(const int idx) const {return _TfrfBox[idx]->get_FragmentAbsoluteTime();}
	int64_t get_tfrf_duration(const int idx) const {return _TfrfBox[idx]->get_FragmentDuration();}

	int64_t get_tfxd_time() const {return _TfxdBox.get_FragmentAbsoluteTime();}

	uint64_t TrackFragmentBaseMediaDecode() const
	{
		_ASSERTE(_has_bbox);

		if(!has_TrackFragmentBaseMediaDecodeTimeBox())
			return UINT64_MAX;

		return _bbox.get_baseMediaDecodeTime();
	}

	uint64_t get_base_offset(bool ignore_flags = false) const
	{
		
		if(0x000001 & _trunbox.get_flags() & (!ignore_flags))
		{
			_ASSERTE((_trunbox.data_offset + _moof_position) == _mdat_position);
			return (_trunbox.data_offset + _moof_position);
		}
		

		return _mdat_position;
	}

	unsigned int get_trun_sample_count() const {return _trunbox.get_sample_count();}

	int64_t get_sample_composition_time_offset(const int idx, bool as_signed = false ) const 
	{
		if(0x000800 & _trunbox.get_flags())
			if(_trunbox.get_version() == 0 && (!as_signed))
				return _trunbox.get_item(idx).sample_composition_time_offset;
			else
			{
				unsigned int t = _trunbox.get_item(idx).sample_composition_time_offset;
				return (int)t;
			}
			

		return 0; //composition = decoding
	}

	int64_t get_sample_duration(const int idx) const 
	{

				if(0x000100 & _trunbox.get_flags())
				{
					//return _trunbox.get_item(idx).sample_duration;
					unsigned int t = _trunbox.get_item(idx).sample_duration;
					return  (int)t;
				}
				else
					return _tbox.default_sample_duration;
	}

	int64_t get_sample_size(const int idx) const
	{
		if(0x000200 & _trunbox.get_flags())
			return _trunbox.get_item(idx).sample_size;
		else
			return _tbox.default_sample_size;
	}

	uint64_t get_mdat_position() const
	{
		return _mdat_position;
	}

#ifdef CENC
	const SampleAuxiliaryInformationOffsetsBox & get_saio()  const {return _saio;}
	const SampleAuxiliaryInformationSizesBox   & get_saiz()  const {return _saiz;}
	const TrackEncryptionBox                   & get_tenc()  const {return _tenc;}
	const SampleEncryptionBox                  & get_senc() const 
	{
		_ASSERTE(_senc.sample_count == get_trun_sample_count());
		return _senc;
	}

	bool has_tenc() const {return _has_tenc;}
	bool has_senc() const {return _has_senc;}
#endif
};

