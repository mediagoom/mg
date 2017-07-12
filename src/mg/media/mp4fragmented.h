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

#include "mp4write.h"
#include "mp4/mp4_fragments.h"

#ifdef CENC
#if defined( __GNUC__ )

    static inline volatile unsigned long long read_tsc(void)
    {
        unsigned int cyl, cyh;
        __asm__ __volatile__("movl $0, %%eax; cpuid; rdtsc":"=a"(cyl),"=d"(cyh)::"ebx","ecx");
        return ((unsigned long long)cyh << 32) | cyl;
    }

#elif defined( _WIN32 ) || defined( _WIN64 )

#   include <intrin.h>
#   pragma intrinsic( __rdtsc )

    __inline volatile unsigned long long read_tsc(void)
    {
        return __rdtsc();
    }

#else
#   error A high resolution timer is not available
#endif

#define RAND(a,b) (((a = 36969 * (a & 65535) + (a >> 16)) << 16) + \
                    (b = 18000 * (b & 65535) + (b >> 16))  )


#endif

#ifndef STREAM_TYPE_AUDIO_AAC
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_H264      0x1b
#endif

__ALX_BEGIN_NAMESPACE

class IMP4ReaderCallback
{
public:
	virtual void start_sample_in_fragment(int stream_id, uint64_t total_sample) = 0;
	virtual void using_sample_in_fragment(sample_stream_info & sample
		, uint64_t fragment_composition_time
		, uint64_t fragment_decoding_time) = 0;
	virtual void end_sample_in_fragment(int stream_id, uint64_t total_sample) = 0;
};

class CMP4Fragment
{
	MovieFragmentHeaderBox _mfhd;
	TrackFragmentHeaderBox _tfhd;
	TrackRunBox            _trun;
	//uint64_t       _last_composition;

	CBuffer<unsigned char> _mdat;

	//CMP4WriteMemory       _headers;

	uint64_t _baseMediaDecodeTime;

	CBuffer<unsigned char> _avcn;

	bool _do_avcn;

#ifdef CENC
	unsigned char _key[KEY_LEN];
	unsigned char _iv[KEY_LEN];

	CBuffer<unsigned char> _encrypted_buffer;

	bool _encrypted;

	SampleEncryptionBox                  _senc;
	SampleAuxiliaryInformationSizesBox   _saiz;
	SampleAuxiliaryInformationOffsetsBox _saio;

#define CLEARINITSIZE 96

	void fillrand(unsigned char *buf, const int len)
	{   static unsigned long a[2], mt = 1, count = 4;
		static unsigned char r[4];
		int                  i;

		if(mt) { mt = 0; *(unsigned long long*)a = read_tsc(); }

		for(i = 0; i < len; ++i)
		{
			if(count == 4)
			{
				*(unsigned long*)r = RAND(a[0], a[1]);
				count = 0;
			}

			buf[i] = r[count++];
		}
	}



	void encrypt(unsigned char * buf, unsigned int clear, unsigned int protected_bytes)
	{

#ifdef HAVE_LIBGYPAES

		SingleEncryptionBox  * psence = new SingleEncryptionBox;
		
		psence->init_vector = new SampleEncryptionInitializationVector64;

		bool sub_sample_encryption = (_senc.get_flags() & 0x000002)?true:false;

		if(sub_sample_encryption)
		{
			SubSampleEncryptionBox * psub = new SubSampleEncryptionBox;

									 psub->BytesOfClearData = clear;
									 psub->BytesOfProtectedData = protected_bytes;
			                         
									 
									 
									 psence->subsample_count = 1;
									 psence->_sub_samples.push_back(psub);
		}
		else
		{
			_ASSERTE(0 == clear);
		}
		 
		unsigned char iv[BLOCK_LEN];
	    ::memset(iv, 0, BLOCK_LEN);

		::memcpy(iv, &_iv[0], 8);

		counter8(_iv); //set for next sample

		::memcpy(&psence->init_vector->InitializationVector_0, iv, 8); //TODO: size of iv

		if(protected_bytes)
		{		
			aes_encrypt_ctx ctx[1];

			::memset(ctx, 0, sizeof(aes_encrypt_ctx));

			AES_RETURN ret = aes_encrypt_key(_key, BLOCK_LEN, ctx);
				_ASSERTE(0 == ret);

			_encrypted_buffer.prepare(protected_bytes);

				ret = aes_ctr_encrypt(buf + clear, _encrypted_buffer.get()
						, protected_bytes
						, iv, counter, ctx);
				_ASSERTE(0 == ret);

			::memcpy(buf + clear, _encrypted_buffer.get(), protected_bytes);
		}

		_senc._samples.push_back(psence);

		_senc.sample_count++;

#endif //AES

	}

#endif //CENC

public:

	CMP4Fragment():_mdat(1024)
		, _baseMediaDecodeTime(UINT64_MAX)
		, _do_avcn(false)
		, _avcn(1024)
#ifdef CENC
	    , _encrypted(false)
		, _encrypted_buffer(10240)
#endif
	{}

	static void counter8(unsigned char *cbuf)
	{
		uint64_t val;
		uint64_t tmp;

		tmp = ( (cbuf[0] << 24) & 0xFF000000 )
			| ( (cbuf[1] << 16) & 0x00FF0000 )
			| ( (cbuf[2] <<  8) & 0x0000FF00 )
			| ( (cbuf[3]      ) & 0x000000FF );

		val = ( (cbuf[4] << 24) & 0x00000000FF000000 )
			| ( (cbuf[5] << 16) & 0x0000000000FF0000 )
			| ( (cbuf[6] <<  8) & 0x000000000000FF00 )
			| ( (cbuf[7]      ) & 0x00000000000000FF );

		val |= ( ( tmp << 32 ) & 0xFFFFFFFF00000000 );

		val++;

		cbuf[0] = ( ( val >> 56 ) & 0x00000000000000FF );
		cbuf[1] = ( ( val >> 48 ) & 0x00000000000000FF );
		cbuf[2] = ( ( val >> 40 ) & 0x00000000000000FF );
		cbuf[3] = ( ( val >> 32 ) & 0x00000000000000FF );
		cbuf[4] = ( ( val >> 24 ) & 0x00000000000000FF );
		cbuf[5] = ( ( val >> 16 ) & 0x00000000000000FF );
		cbuf[6] = ( ( val >>  8 ) & 0x00000000000000FF );
		cbuf[7] = ( ( val       ) & 0x00000000000000FF );
	}

	static void counter(unsigned char *cbuf)
	{
		uint64_t * pval = reinterpret_cast<uint64_t *>(&(cbuf[8]));

		if(0xFFFFFFFFFFFFFFFF == (*pval)) //if we reached max val reset it
		{
			_ASSERTE(false);

			cbuf[ 8] = 0x0;
			cbuf[ 9] = 0x0;
			cbuf[10] = 0x0;
			cbuf[11] = 0x0;
			cbuf[12] = 0x0;
			cbuf[13] = 0x0;
			cbuf[14] = 0x0;
			cbuf[15] = 0x0;

			counter8(cbuf);
		}
		else
		{
			counter8(&(cbuf[8]));
		}
	}



	
	void set_baseMediaDecodeTime(uint64_t baseMediaDecodeTime)
	{
		_baseMediaDecodeTime = baseMediaDecodeTime;
	}

	void add_avcn_box(const unsigned char * body, int size)
	{
		_ASSERTE(0 == _avcn.size());
		_avcn.add(body, size);
		_do_avcn = true;

		_ASSERTE(size == _avcn.size());
	}

	uint64_t get_baseMediaDecodeTime(){return _baseMediaDecodeTime;}
	bool has_baseMediaDecodeTime(){return (UINT64_MAX != _baseMediaDecodeTime);}

#ifdef CENC
	void set_encryption(unsigned char * p_key, unsigned int flags = 0x000002)
	{
		//force sub samples 
		//flags = 0x000002;

		::memcpy(&_key[0], p_key, KEY_LEN);
		::memset(&_iv[0], 0, KEY_LEN);
		
		fillrand(&_iv[0], 8);

		_senc.sample_count = 0;
		_senc.set_flags(flags);
		_saiz.sample_count = 0;
		_saio.entry_count  = 0;

			
		_encrypted = true;
	}
#endif

	virtual void start(int track_id, int fragment_id, unsigned int first_sample_flags = 0)
	{
		_mfhd.sequence_number = fragment_id;
		_tfhd.track_ID = track_id;

		_tfhd.set_base_data_offset(0);
		
		//_last_composition = UINT64_MAX;


		unsigned int first_sample_flags_present = 0;
		unsigned int sample_composition_flags   = 0;

		if(0 < first_sample_flags)
		{
			first_sample_flags_present = 0x000004;
			sample_composition_flags   = 0x000800;
		}


        _trun.set_version(1);
		_trun.set_flags(0x000100 //duration
			| first_sample_flags_present  //first-sample-flags-present
		    | 0x000200 //sample size
			  //| 0x000400 //sample flag
			    | sample_composition_flags //sample composition offset
				| 0x000001 //data-offset-present.
				);

		_trun.first_sample_flags = first_sample_flags;
		
	}

	
	virtual void add_sample(
		  int stream_id
		, BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, uint64_t duration
		)
	{

		uint64_t comp_diff = composition_time - decoding_time;
		unsigned int     comp_diff_flag = 0x000800 & _trun.get_flags();

		//_ASSERTE( ! ( comp_diff != 0 && comp_diff_flag == 0) );

		_trun.add(static_cast<uint32_t>(duration)
			, body_size
		  , 0x000100 //duration
		    | 0x000200 //sample size
			  |  comp_diff_flag //sample composition offset
				, static_cast<uint32_t>(comp_diff)
				);


#define AUDIOF2 

#ifdef CENC
		if(_encrypted)
		{
			bool sub_sample_encryption = (_senc.get_flags() & 0x000002)?true:false;

				 
			if(sub_sample_encryption)
			{

				if((CLEARINITSIZE + 16) >= body_size)
				{
					unsigned int clear           = body_size;
					unsigned int protected_bytes = 0;

					encrypt(body, clear, protected_bytes);

				}
				else
				{

					unsigned int clear           = CLEARINITSIZE + (body_size % BLOCK_LEN);
					unsigned int protected_bytes = body_size - clear;

					_ASSERTE(0 == (protected_bytes % BLOCK_LEN));

					encrypt(body, clear, protected_bytes);

				}

			}
			else
			{
				
#ifdef AUDIOF2
				DWORD orig = _senc.get_flags();
			    _senc.set_flags(0x000002);
#endif
				encrypt(body, 0, body_size);
#ifdef AUDIOF2
			    _senc.set_flags(orig);
#endif
			}
		}
#endif

		_mdat.add(body, body_size);

		//_last_composition = composition_time;
	}
	
	virtual void end(CMP4W & mp4w)
	{
		mp4w.open_box(box_MOOF);
		mp4w.write_child_box(box_MFHD, _mfhd);
		mp4w.open_box(box_TRAF);
		mp4w.write_child_box(box_TFHD, _tfhd);
		

		//tfdt
		if(has_baseMediaDecodeTime())
		{			

			TrackFragmentBaseMediaDecodeTimeBox box;
			box.set_version(1);
			box.set_flags(0);
			box.set_baseMediaDecodeTime(_baseMediaDecodeTime);

			mp4w.write_child_box(box_TFDT, box);

		}

		uint64_t data_offset_position  = mp4w.get_position();
		             data_offset_position += (12 + 4);
		
		mp4w.write_child_box(box_TRUN, _trun);
		




#ifdef CENC
		if(_encrypted)
		{

#ifdef AUDIOF2
			    _senc.set_flags(0x000002);
#endif

			_ASSERTE(_trun.sample_count == _senc.sample_count);

			uint64_t senc_offset_position = mp4w.get_position() + 16;

			mp4w.write_child_box(box_senc, _senc);

			_saio.entry_count = 1;
			_saio._samples_offset.push_back(senc_offset_position);

			_saiz.default_sample_info_size = 16;
			_saiz.sample_count = _senc.sample_count;

			mp4w.write_child_box(box_saiz, _saiz);
			mp4w.write_child_box(box_saio, _saio);
			
		}
#endif

		if(_do_avcn)
		{
			mp4w.open_box(box_AVCN);
			   mp4w.write_bytes(_avcn.get(), _avcn.size());
			mp4w.close_box(box_AVCN);
		}

		mp4w.close_box(box_TRAF);
		mp4w.close_box(box_MOOF);

		mp4w.write_uint(_mdat.size() + 8);
		mp4w.write_uint(box_mdat);

		uint64_t data_offset = mp4w.get_position();
		
		mp4w.write_bytes(_mdat.get(), _mdat.size());

		uint64_t end_position = mp4w.get_position();

		if(_trun.get_flags() & 0x000001)
		{
			mp4w.set_position(data_offset_position);
			mp4w.write_uint(static_cast<uint32_t>(data_offset));
			mp4w.set_position(end_position);
		}
	
	}




//////////////////////////////////////////////////////////////////////////////////


    static void add_frames_to_m4f(
		  CMP4Fragment & m4f
		, MP4Reader    & reader
		, CMP4         & input_stream
		, int stream_id
		, uint64_t start_time
		, uint64_t end_time
		, uint64_t base_media_decode_time /* = UINT64_MAX*/
		, IMP4ReaderCallback * p_callback = NULL
		, unsigned char * p_key = NULL)
	{

		sample_stream_info sample;

	    bool is_audio = reader.IsSound(stream_id);

		uint64_t stream_offset = reader.get_stream_offset(stream_id);

		////////we work on presetation time	////////////////////////////////	
		_ASSERTE(stream_offset <= start_time && stream_offset < end_time);

		/*
		start_time -= stream_offset;
		end_time   -= stream_offset;
		*/

		//////////////////////////////////////////////////////////////////////

		reader.move(start_time, stream_id);

#ifdef _DEBUG
		
							 if(NULL != p_callback)
								 p_callback->start_sample_in_fragment(stream_id, reader.get_sample_count(stream_id));
#endif

		CBuffer<unsigned char> body(1024);

		 while(reader.next_file_chunks(sample))
				 {
					 if(sample.stream == stream_id)
					 {
						 if( (sample.composition_time + stream_offset) >= end_time)
							 break;

						 body.prepare(static_cast<uint32_t>(sample.size));

						 input_stream.set_position(sample.offset);
						 input_stream.read_bytes(body.get(), sample.size);

						 uint64_t composition_time = sample.composition_time;
						          int64_t decoding_time    = sample.decoding_time;

					    
						 if(reader.stream_ctts_offset(sample.stream))
						 {
							decoding_time += reader.ctts_stream_offset(sample.stream);
						 }
						 
						 if( (composition_time + stream_offset) < start_time)
						 {
							 _ASSERTE( (composition_time + stream_offset) < start_time );
							 /*
							 std::wcout << L"INVALID SEQUENCE: "
								        << mp4_input_file
										<< L"\t"
										<< HNS(start_time)
										<< L"\t"
										<< HNS(composition_time)
										<< L"\t"
										<< sample.sample_number
										<< L"\t"
										<< stream_id
										<< L"\t"
										<< track_id
										<< std::endl;
							*/

							 
						 }
						 else
						 {

#ifdef _DEBUG
							 if(NULL != p_callback)
								 p_callback->using_sample_in_fragment(sample, composition_time, decoding_time);
#endif

							  m4f.add_sample(sample.stream
								 , body.get()
								 , static_cast<uint32_t>(sample.size)
								 , sample.bIsSyncPoint
								 , composition_time
								 , decoding_time
								 , sample.duration
								 );

							 if(!m4f.has_baseMediaDecodeTime())
							 {
								 if(UINT64_MAX == base_media_decode_time)
								 {
									 _ASSERTE(start_time >= (composition_time - decoding_time));
									 m4f.set_baseMediaDecodeTime(start_time - (composition_time - decoding_time));
								 }
								 else
								 {
									 m4f.set_baseMediaDecodeTime(base_media_decode_time - (composition_time - decoding_time));
								 }
							 }
							 
						 }//if(composition_time < start_time)
					 }
				 }

#ifdef _DEBUG
							 if(NULL != p_callback)
								 p_callback->end_sample_in_fragment(stream_id, reader.get_sample_count(stream_id));
#endif
		
	}

	static int produce_m4f_initialization
	(	  CMP4W & output_stream
		, int track_id
		, unsigned int codec_type
		, unsigned char * codec_private_data
		, unsigned int size
		, const unsigned int target_bit_rate
		, uint64_t duration   = 0
		, uint64_t time_scale = 10000000
		, unsigned char * pKid        = NULL
		, const unsigned char * ppssh       = NULL
		, unsigned int     pssh_size  = 0
	)
	{
		MP4Write        write;
		/*
		CMP4WriteFile   body;
						body.Open(mp4_init_file_out);
		*/

		int stream_id(-1);

		if(STREAM_TYPE_VIDEO_H264 == codec_type)
				stream_id = write.add_visual_stream(codec_private_data, size, time_scale);

		
		if(STREAM_TYPE_AUDIO_AAC == codec_type)
			stream_id = write.add_audio_stream(codec_private_data, size, target_bit_rate, time_scale);

#ifdef CENC
		if(NULL != pKid)
		{
			write.set_stream_key_id(pKid, stream_id);
		}
#endif

		write.set_first_track_id(track_id - 1);
		write.set_stream_track_id(track_id -1 , stream_id);
		write.set_allow_empty_stream(true);

		//write.set_

		_ASSERTE(0 == stream_id);
		

		//write.set_ftyp(ftyp_DASH, 0);
        //TODO: styp
		write.set_ftyp(BOX( 'c', 'c', 'f', 'f' ), 1);
		write.add_brand(ftyp_ISO6);
		write.set_mvhd_timescale(static_cast<uint32_t>(time_scale));
		write.set_mvhd_version(1);
		//write.add_brand(ftyp_avc1);
		//write.add_brand(ftyp_mp41);


		write.write_ftyp(output_stream);
			
			write.write_moov(output_stream, false);
					output_stream.open_box(box_MVEX);

						MovieExtendsHeaderBox mehd;
											  mehd.set_fragment_duration(duration);

											  output_stream.write_child_box(box_MEHD, mehd);

						TrackExtendsBox trex;
										trex.set_flags(0);
										trex.set_version(0);
										trex.track_ID = track_id;
										trex.default_sample_description_index = 1;
										trex.default_sample_duration = 0;
										trex.default_sample_size = 0;
										trex.default_sample_flags= 0;

										output_stream.write_child_box(box_TREX, trex);

					output_stream.close_box(box_MVEX);

					
#ifdef CENC
		if(NULL != ppssh)
		{
					output_stream.write_bytes(ppssh, pssh_size);
		}
#endif

			write.close_moov(output_stream);

		write.end();

		

		return 0;

		
	}


	static int produce_m4f_chunk(
		                CMP4W & output_stream
					  , MP4Reader & reader
					  , CMP4  & input_stream
		              , int stream_id
					  , int track_id
					  , uint64_t start_time
					  , uint64_t end_time
					  , int sequence
					  , bool AVCNBOX
					  , uint64_t base_media_decode_time
					  , IMP4ReaderCallback * p_callback = NULL
					  , unsigned char * p_key = NULL
					  , DWORD senc_flags = 0
					  )
{
	
			CMP4Fragment m4f;

			m4f.start(track_id, sequence, ((AVCNBOX)?0xa600000:0));

#ifdef CENC
			 if(NULL != p_key)
			 {
				 m4f.set_encryption(p_key, senc_flags );
			 }
#endif

				
					//->  
						add_frames_to_m4f(
							  m4f
							, reader
							, input_stream
							, stream_id
							, start_time
							, end_time
							, base_media_decode_time
							, p_callback);
					//<-


				 if(AVCNBOX)
				 {
					 const CMP4VisualEntry & ve = reader.get_visual_entry(0, stream_id);
					 m4f.add_avcn_box(ve.get_body(), static_cast<uint32_t>(ve.get_body_size()));
				 }

				 m4f.end(output_stream);

	return 0;
}

	static int produce_m4f_chunk(
		CMP4W & output_stream
		, CMP4  & input_stream
		, int stream_id
		, int track_id
		, uint64_t start_time
		, uint64_t end_time
		, int sequence
		, bool AVCNBOX 
		, uint64_t base_media_decode_time
		, IMP4ReaderCallback * p_callback = NULL
		, unsigned char * p_key = NULL
		, DWORD senc_flags = 0
		)
	{
		MP4Reader reader;
		reader.parse(input_stream);

		return produce_m4f_chunk(output_stream
			, reader
			, input_stream
			, stream_id
			, track_id
			, start_time
			, end_time
			, sequence
			, AVCNBOX
			, base_media_decode_time
			, p_callback
			, p_key
			, senc_flags);
	}




	static int produce_m4f_chunk_cross(
		                CMP4W & output_stream
					  , MP4Reader & reader
					  , CMP4  & input_stream
					  , MP4Reader & reader_2
					  , CMP4  & input_stream_2
		              , int stream_id
					  , int track_id
					  , uint64_t start_time
					  , uint64_t end_time
					  , uint64_t start_time_2
					  , uint64_t end_time_2
					  , int sequence
					  , bool AVCNBOX 
					  , uint64_t base_media_decode_time
					  , IMP4ReaderCallback * p_callback = NULL
					  , unsigned char * p_key = NULL
					  , DWORD senc_flags = 0
					  )
{
	
			CMP4Fragment m4f;

			m4f.start(track_id, sequence, ((AVCNBOX)?0xa600000:0));

#ifdef CENC
			 if(NULL != p_key)
			 {
				 m4f.set_encryption(p_key, senc_flags );
			 }
#endif

				
					//->  
						add_frames_to_m4f(
							  m4f
							, reader
							, input_stream
							, stream_id
							, start_time
							, end_time
							, base_media_decode_time
							, p_callback
							, p_key);


						add_frames_to_m4f(
							  m4f
							, reader_2
							, input_stream_2
							, stream_id
							, start_time_2
							, end_time_2
							, base_media_decode_time
							, p_callback
							, p_key);
					//<-


				 if(AVCNBOX)
				 {
					 const CMP4VisualEntry & ve = reader.get_visual_entry(0, stream_id);
					 m4f.add_avcn_box(ve.get_body(), static_cast<uint32_t>(ve.get_body_size()));
				 }

				 m4f.end(output_stream);

	return 0;
}




	static int produce_m4f_chunk_cross(
		CMP4W & output_stream
		, CMP4  & input_stream
		, CMP4  & input_stream_2
		, int stream_id
		, int track_id
		, uint64_t start_time
		, uint64_t end_time
		, uint64_t start_time_2
		, uint64_t end_time_2
		, int sequence
		, bool AVCNBOX 
		, uint64_t base_media_decode_time
		, IMP4ReaderCallback * p_callback = NULL
		, unsigned char * p_key = NULL
		, DWORD senc_flags = 0
		)
	{
		MP4Reader reader;
		reader.parse(input_stream);

		MP4Reader reader2;
		reader2.parse(input_stream_2);

		return produce_m4f_chunk_cross(output_stream
			, reader
			, input_stream
			, reader2
			, input_stream_2
			, stream_id
			, track_id
			, start_time
			, end_time
			, start_time_2
			, end_time_2
			, sequence
			, AVCNBOX
			, base_media_decode_time
			, p_callback
			, p_key
			, senc_flags);
	}
};

__ALX_END_NAMESPACE

