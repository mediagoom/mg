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
#include "alxstring.h"
#include <vector>

#ifndef __ALX_CSPLITTER__
#define __ALX_CSPLITTER__

__ALX_BEGIN_NAMESPACE

class Csplitter{

struct locator
{
     size_t nBegin;
	 size_t nLen;
};
protected:
	

	std::vector<locator> m_locator;
	Cstring m_str;
public:
	
	Csplitter(){}

	Csplitter(Cstring scontent, Cstring sseparator)
	{
		split(scontent, sseparator);
	}

	void split(Cstring scontent, Cstring sseparator)
	{

		if(!scontent || !sseparator)
			return;

		m_str = scontent;

		TCHAR *pszbegin = scontent;
		size_t nLen = _tcslen(sseparator);
			
		while(true)
		{
			locator loc;
			
				loc.nBegin = pszbegin - static_cast<TCHAR*>(scontent);
			
			pszbegin = _tcsstr(pszbegin, sseparator);
			if(!pszbegin){
				//nothing found
					loc.nLen = _tcslen(scontent) - loc.nBegin;
					m_locator.push_back(loc);
					break;
					
			}
			
				loc.nLen = pszbegin - (loc.nBegin + static_cast<TCHAR*>(scontent));
					m_locator.push_back(loc);
				 
			//move the pointer at the beginning of the 
			//next element
				pszbegin += nLen;

			//check if we 
			//are at the end
			if(!pszbegin)
				break;
		}

	}

	size_t getCount()
	{
		return m_locator.size();
	}

	Cstring operator[](int index)
	{
		CResource<CBuffer<TCHAR> > res = new CBuffer<TCHAR>(m_locator[index].nLen + 1);

		res->add(static_cast<TCHAR*>(m_str) + m_locator[index].nBegin, m_locator[index].nLen);

		return res;
	}

	Cstring getInternalString() const
	{
		return m_str;
	}


};

inline Cstring replace(const TCHAR *str, const TCHAR *in, const TCHAR *out)
{
	Csplitter split(str, out);
	Cstring retVal(split[0]);

		
		
		for(size_t i = 1; i < split.getCount(); ++i)
		{
			retVal += in;
			retVal += split[i];
		}

	return retVal;
}
__ALX_END_NAMESPACE

#endif //__ALX_CSPLITTER__

