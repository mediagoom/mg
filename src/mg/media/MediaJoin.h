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

#include <vector>
#include <algorithm>

class CMediaJoin
{
protected:
	std::vector<int64_t> _streams_end;
	std::vector<int64_t> _streams_start;

	std::vector<int64_t> _streams_diff;
	int64_t _min;

	unsigned long _streams_count;
public:

	void SetStreamCount(unsigned long s)
	{_streams_count = s;}

	unsigned long GetStreamCount()
	{return _streams_count;}

	void AddStreamEnd(uint64_t e)
	{_streams_end.push_back(e);}

	void AddStreamStart(uint64_t s)
	{_streams_start.push_back(s);}

	virtual void Compute()
	{
		_ASSERTE(_streams_start.size() == _streams_end.size());
		_ASSERTE(_streams_end.size() == _streams_count);
		_ASSERTE(0 < _streams_count);

		int64_t max_end(0);
		int64_t min_start(INT64_MAX);

		for(uint32_t i = 0; i < GetStreamCount(); ++i)
		{
			max_end   = (max_end   < _streams_end[i])?_streams_end[i]:max_end;
			min_start = (min_start > _streams_start[i])?_streams_start[i]:min_start; 
		}

		std::vector<int64_t> streams_end_diff;
	    std::vector<int64_t> streams_start_diff;

		

		for(uint32_t i = 0; i < GetStreamCount(); ++i)
		{
			streams_end_diff.push_back(_streams_end[i] - max_end );
			streams_start_diff.push_back(min_start - _streams_start[i]); 
            //_ASSERTE(streams_end_diff[i] >= streams_start_diff[i]);
			_streams_diff.push_back(streams_end_diff[i] + streams_start_diff[i]);
		}

		std::vector<int64_t>::iterator mini = max_element(_streams_diff.begin(), _streams_diff.end());
		
		_min = *mini;

		
	}

	virtual int64_t GetJoinTime(int stream)
	{
		int64_t less = _streams_diff[stream] - _min;
		_ASSERTE(less <= 0);
		return _streams_end[stream] + (-1*less);
	}

	 int64_t GetStreamDiff(int stream)
	{return _streams_diff[stream];}

	CMediaJoin(void):
	  _streams_count(0)
	{
	}

	virtual ~CMediaJoin(void)
	{
	}
};

