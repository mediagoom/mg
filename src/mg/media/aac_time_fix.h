#pragma once


class CAACFix;

class CAACFixStream
{
	
	double _previus;
	double _error;

	double _min_frame_duration;
	double _max_frame_duration;

	int _stream_id;

	friend CAACFix;

	CAACFixStream()
		: _stream_id(INT32_MAX)
	{
		_min_frame_duration = INT32_MAX;
	}

	void set_info(const unsigned int sample_rate, int stream_id)
	{
		if(_min_frame_duration == INT32_MAX && _stream_id == INT32_MAX)
		{
			_min_frame_duration = 1024.00 / sample_rate * 10000000.00;
			_max_frame_duration = 1025.00 /sample_rate * 10000000.00;

			
			_error = INT32_MAX;
			_previus = INT32_MAX;

			_stream_id = stream_id;
		}
		else
		{
			
			_ASSERTE(_min_frame_duration == 1024.00 / sample_rate * 10000000.00);
			_ASSERTE(_max_frame_duration == 1025.00 /sample_rate * 10000000.00);

			//_ASSERTE(_error < _min_frame_duration);
		}
	}

	double fix_audio_frequency_timing(double time_in, int stream_id)
	{
		
		_ASSERTE(stream_id == _stream_id);
		_ASSERTE(_min_frame_duration != INT32_MAX);

		double time = time_in;

		double & previus = _previus;
		double & error   = _error;

		if(INT32_MAX == previus)
		{
			previus = time;
			error = 0;

			return time;
		}

		
		int correction = 0;

		if( (time - previus) >= _max_frame_duration )
		{
			error += (time - previus) - _max_frame_duration;
			time = previus + _max_frame_duration;

			correction = 1;
		}
		else //if( (time - previus ) >= _min_frame_duration )
		{
			error += ( (time - previus) - _min_frame_duration );
			time = previus + _min_frame_duration;

			correction = 2;

			if(error >= (_max_frame_duration - _min_frame_duration) )
			{
				double correction = (_max_frame_duration - _min_frame_duration);
				time += correction;
				error -= correction;

				correction = 3;
			}
		}

		

		if(time_in - time > _max_frame_duration 
			|| time - time_in > _max_frame_duration)
		{
			
				//if(time_in - time > (2 * _min_frame_duration 
				//	|| time - time_in > (2 * _min_frame_duration)

			//LETS TRY A BRUTE FORCE RESET
			if(time < time_in)
				time = time_in;
		}
		
		//_ASSERTE(time > previus);

		

		previus = time;

		return time;
	}


};


class CAACFix{

private:

	//DECLARE_FILE_LOG;

	CAACFixStream * _streams[MAX_STREAM];

public:

	CAACFix()
	{
		//INIT_FILE_LOG_SUBCAT(FOUNDATION, MP4MUX, L"AACMUXFIX");
	
		for(int i = 0; i < MAX_STREAM; i++)
			_streams[i] = NULL;
		
	}

	~CAACFix()
	{
		for(int i = 0; i < MAX_STREAM; i++)
			if(_streams[i] != NULL)
				delete _streams[i];
	}

	void set_info(const unsigned int sample_rate, int stream_id)
	{
		_ASSERTE(stream_id < MAX_STREAM);

		if(NULL == _streams[stream_id])
			_streams[stream_id] = new CAACFixStream(/*g_filetracer*/);

		_streams[stream_id]->set_info(sample_rate, stream_id);
			
	}

	double fix_audio_frequency_timing(double time_in, int stream_id)
	{
		_ASSERTE(stream_id < MAX_STREAM);

        if(NULL == _streams[stream_id])
            return time_in;

		_ASSERTE(NULL != _streams[stream_id]);

		return _streams[stream_id]->fix_audio_frequency_timing(time_in, stream_id);
	}

};