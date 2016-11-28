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

#define STREAM_MAX 16

class CMP4FragmentValidation : public IMP4ReaderCallback
{

	uint64_t _last_sample_number[STREAM_MAX];
	uint64_t _total_samples[STREAM_MAX];

public:

	CMP4FragmentValidation()
	{
		for(int i = 0; i < STREAM_MAX; i++)
		{
			_last_sample_number[i] = UINT64_MAX;
			_total_samples[i]      = UINT64_MAX;
		}
	}
	
	virtual void start_sample_in_fragment(int stream_id, uint64_t total_sample)
	{
		
	}

	virtual void using_sample_in_fragment(sample_stream_info & sample
		, uint64_t fragment_composition_time
		, uint64_t fragment_decoding_time)
	{
		//sample.stream
		//sample.sample_number

		//if this is the first sample everthing is fine
		if(UINT64_MAX != _last_sample_number[sample.stream])
		{
			if(0 == sample.sample_number) //this is the first sample in the file
			{
				if((_total_samples[sample.stream] - 1) != _last_sample_number[sample.stream])
				{
					_ASSERTE((_total_samples[sample.stream] - 1) == _last_sample_number[sample.stream]);
					std::wcout << L"MISSING-STREAM-SAMPLES-ATTHEEND\t"
						      << _total_samples[sample.stream]
							  << L"\t"
							  << _last_sample_number[sample.stream]
							  << std::endl;
				}
			}
			else
			{
				if((sample.sample_number - 1) != _last_sample_number[sample.stream])
				{
					_ASSERTE((sample.sample_number - 1) == _last_sample_number[sample.stream]);
					std::wcout << L"MISSING-STREAM-SAMPLES-INTHEMIDDLE\t"
						      << (sample.sample_number - 1)
							  << L"\t"
							  << _last_sample_number[sample.stream]
							  << std::endl;
				}
			}
		}

		_last_sample_number[sample.stream] = sample.sample_number;
	}

	
	
	virtual void end_sample_in_fragment(int stream_id, uint64_t total_sample)
	{
		_total_samples[stream_id] = total_sample;
	}
};

