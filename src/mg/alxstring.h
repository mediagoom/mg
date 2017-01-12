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

#include "cbuffer.h"
#include "cresource.h"



#include <iostream>
//#include <tstring>


#ifdef _WIN32

#include <tchar.h>
#include "strsafe.h"

#else

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>

#define __STDC_FORMAT_MACROS


#include "core/char.h"

#endif

#ifndef PRId64
//#warning "PRId64 not defined"
#define PRId64       "ld"
#define PRIu64       "lu"
#define SCNd64       "ld"
#define SCNu64       "lu"
#define SCNx64       "lx"
#endif




#ifndef __ALX__CSTRING_
#define __ALX__CSTRING_

__ALX_BEGIN_NAMESPACE

#define DECLARE_CH_HEX static const char s_chHexChars[16] = \
		{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', \
		 'A', 'B', 'C', 'D', 'E', 'F'};

		

typedef int SSIZE_T;

template <typename T>
class CstringT;

typedef CstringT<TCHAR> Cstring;

template <typename T>
class CstringT{

	friend std::ostream& operator<< ( std::ostream &os, const Cstring &str );
	friend std::wostream& operator<< ( std::wostream &os, const Cstring &str );

	friend bool operator<(const Cstring &greater, const Cstring &less);
	friend bool /*operator==*/Equals(const Cstring &s1, const Cstring &s2);
	//friend bool operator==(const Cstring &s1, const Cstring &s2);
	friend bool operator==(const TCHAR *ch, const Cstring &s2);
	friend bool operator!=(const TCHAR *ch, const Cstring &s2);
	friend Cstring operator+(const TCHAR *ch, const Cstring &str);

	//delegate all the work to CResource
	CResource<CBuffer<T, size_t> > m_str;


	//conversions

	template<typename K> CstringT<T> convert(const TCHAR * pszFormat, K val)
	{
		
		CstringT<T> tmp(1024);
		TCHAR * p = tmp.m_str.get();

                if(sizeof(T) > 1)
		{
		  MGBASECHECK(-1);
		}
		else
		{
			

			int r = _stprintf(p, pszFormat, val );
			MGBASECHECK(r);

		}
	
	}

	template <typename K> void to_type(const TCHAR * pszFormat, K * p_val)
	{

		if(sizeof(T) > 1)
		{
			MGBASECHECK(-1);
		}
		else
		{
		    int ret = _stscanf_s(CstringT<T>::operator char*(), pszFormat, p_val);
		    if(1 != ret)
		      MGBASECHECK(-1);

		}
	}


public:	
	enum RESULT{ npos = -1};
	CstringT()
	{
		  m_str = new CBuffer<T>(1024);
	}

 	CstringT(CResource<CBuffer<T> > str)
		:m_str(str)
	{
	}

 	CstringT(uint32_t size)
	{
		  m_str = new CBuffer<T>(size);
	}

	CstringT(const char *str)
	{
		if(str)
		  operator=(str);
	}

	CstringT(const wchar_t *str)
	{
		if (str)
			operator=(str);
	}

 	CstringT(const CstringT &str):
		m_str(str.m_str)
	{		
	}

	CstringT operator=(const CstringT &str)
	{
		m_str = str.m_str;
		return *this;
	}

	CstringT operator=(const char *str)
	{
		size_t size = strlen(str); 
		
		m_str = new CBuffer<T>(/*null terminated*/static_cast<uint32_t>(++size));
		
		return operator+=(str);


		/*
		if(1 < size)
		  m_str->add(str, --size);
		*/

	    
	}

	CstringT operator=(const wchar_t *str)
	{
		size_t size = wcslen(str);

		m_str = new CBuffer<T>(/*null terminated*/static_cast<uint32_t>(++size));

		return operator+=(str);


		/*
		if(1 < size)
		m_str->add(str, --size);
		*/

		
	}

	CstringT append(const T *str) 
	{
		
		if(!m_str)
			return operator=(str);			
	
		uint32_t size = _tcslen(str);		
		m_str->add(str, size);
			return *this;
	}


	CstringT & operator+=(const char *str)
	{
		if (NULL == str)
			return *this;

		uint32_t size = static_cast<uint32_t>(strlen(str));

		if (1 < sizeof(T))
		{

			wchar_t * pw = reinterpret_cast<wchar_t*>(GetBuffer(size + 1, false));

#ifdef _WIN32
			//CP_ACP
			int c_res = ::MultiByteToWideChar(0, NULL, str, size, pw, size + 1);
			if (0 == c_res) //{ALXTHROW("cannot convert char * to WCHAR*");}
				MGBASECHECK(E_BASE_CUSTOM);

			_ASSERTE(0 != c_res);
#else
			
			size_t c_res = mbstowcs(pw, str, size + 1);
			if( (size_t) -1 == c_res)
				MGBASECHECK(E_BASE_CUSTOM);

			_ASSERTE(c_res == size);
			
#endif

			CommitBuffer(size);
			return *this;
		}
		else
		{
			m_str->add(reinterpret_cast<const T *>(str), size);
			return *this;
		}

	}

	CstringT & operator+=(const wchar_t *str)
	{
		if (NULL == str)
			return *this;

		size_t size = wcslen(str);

		if (1 == sizeof(T))
		{
			char * pw = reinterpret_cast<char*>(GetBuffer(size + 1, false));

#ifdef _WIN32
			//CP_ACP
			int c_res = ::WideCharToMultiByte(0, NULL, str, ST_U32(size), pw, size + 1, NULL, NULL);
			if (0 == c_res) //{ALXTHROW("cannot convert WCHAR* to char*");}
				MGBASECHECK(E_BASE_CUSTOM);

			_ASSERTE(0 != c_res);
#else
			/*
			#include <stdlib.h>

			*/

       			size_t c_res = wcstombs(pw, str, size + 1);
			if( (size_t) -1 == c_res)
				MGBASECHECK(E_BASE_CUSTOM);
			
			_ASSERTE(c_res == size);
#endif
			CommitBuffer(size);

		}
		else
		{
			m_str->add(reinterpret_cast<const T *>(str), static_cast<uint32_t>(size));
			
		}
		
		return *this;

	}


#ifdef _XWIN32
		
	static CstringT Load(uint32_t  uID , HINSTANCE H = NULL)
	{
		if(NULL == H)
		{
			H = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
			ASSERT(NULL != H);
		}

		CstringT<T> tmp;

		int res = LoadString(H, uID, tmp.m_str->get(), tmp.m_str->getFree());

		

		ASSERT(NULL != res);

		tmp.m_str->updatePosition(res);

		return tmp;		
	}
	
#endif


	CstringT & operator+=(const CstringT<wchar_t> &str)
	{
		//if (1 < sizeof(T))
		{
			//CstringT<wchar_t> &s = const_cast<CstringT<wchar_t>&>(str);
			return operator+=(str.operator wchar_t*());
		}
		/*else
		{
			return operator += (str.m_str->get());
		}*/
	}

	CstringT & operator+=(const CstringT<char> &str)
	{
		
		//if (1 == sizeof(T))
		{
			//CstringT<char> &s = const_cast<CstringT<char>&>(str);
			return operator+= ( str.operator char*() );
				//(static_cast<const char>(str.to_type*));
		}
		/*else
		
		{
			return operator += (str.m_str->get());
		}*/
		
	}


//#ifdef _WIN32

	operator T*()
	{
		if(!m_str)
			return NULL;

		return m_str->get();
	}

	operator  T*() const
	{
		CstringT &s = const_cast<CstringT&>(*this);
		   T* tmp = s.operator T *();
				return tmp;
	}

	operator CResource<CBuffer<T> >()
	{
		return m_str;
	}


/*
#else

	operator TCHAR*()
	{
#ifdef _UNICODE
		if(sizeof(T) > 1)
		{
			return m_str->get();
		}
		else
		{
         		//compile unicode cstring char
			MGBASECHECK(-1);		
		}
#else
	        if(sizeof(T) > 1)
		{
			//compile char cstring wchar
			MGBASECHECK(-1);
		}
		else
		{
			return m_str->get();
		}
#endif
	}

	operator TCHAR*() const
	{
	 	CstringT &s = const_cast<CstringT&>(*this);
		   TCHAR* tmp = s.operator TCHAR*();

	    return tmp;
		 
	}
#endif
*/


  	bool operator!()
	{
		return !m_str;
	}

	bool operator==(const TCHAR* str)
	{
		TCHAR * p = CstringT<T>::operator TCHAR*();

		return !_tcscmp(p, str);
	}

	bool operator!=(const TCHAR* str)
	{
		return !operator==(str);
	}
	
	//return a Cstring with the same content but pointing
	//to a differnet buffer
	CstringT clone() const
	{
		CstringT &str1 = const_cast<CstringT&>(*this);
		CstringT tmp(str1.operator TCHAR *());
			return tmp;
	}
	CstringT operator+(const T *str) const
	{
		CstringT tmp(clone());
			tmp += str;

			return tmp;
	}

	CstringT operator+(const Cstring str) const
	{
		CstringT tmp(clone());
			tmp += str;

			return tmp;
	}
/*	
	CstringT operator+=(const long l)
	{
		
		TCHAR sz[16]; //really 8 should be enouth
		_itot(l, sz, 10);
		
		uint32_t size = _tcslen(sz);		
		m_str->add(sz, size);
			return *this;
	}
*/


	CstringT & operator+=(const int64_t l)
	{
		
		TCHAR sz[22]; 
#ifdef _WIN32
		_i64tot_s(l, sz, 22, 10);
#else
                if(sizeof(T) > 1)
		{
		  MGBASECHECK(-1);
		}
		else
		{

			int r = sprintf(sz, _T( "%" PRId64 ), l);
			//PRId64 "" ), l);
			MGBASECHECK(r);

		}
#endif
		
		uint32_t size = static_cast<uint32_t>(_tcslen(sz));
		m_str->add(sz, size);
			return *this;
	}

	CstringT & operator+=(const float f)
	{
		char buffer[1024];
		memset(buffer, 0, 1024);
#ifdef _WIN32
	
	//double value = -1234567890.123;
        errno_t e = _gcvt_s(buffer, 1024, f, 24); 

		_ASSERTE(0 == e);
#else
                if(sizeof(T) > 1)
		{
		  MGBASECHECK(-1);
		}
		else
		{

			int r = sprintf(buffer, _T( "%.6f" ), f);
			//PRId64 "" ), l);
			MGBASECHECK(r);

		}
#endif
	
		return operator+=(buffer);

	}



	CstringT & operator+=(const uint64_t l)
	{
		
		TCHAR sz[22]; 
#ifdef _WIN32
		_ui64tot_s(l, sz, 22, 10);
#else
                if(sizeof(T) > 1)
		{
		  MGBASECHECK(-1);
		}
		else
		{

			int r = sprintf(sz, _T( "%" PRIu64 ), l);
			//PRId64 "" ), l);
			MGBASECHECK(r);

		}
#endif
		
		uint32_t size = _tcslen(sz);		
		m_str->add(sz, size);
			return *this;
	}

	CstringT & operator+=(const unsigned int ui)
	{
		uint64_t  ui64 = ui;
		return operator+=(ui64);
	}	
	
	CstringT & operator+=(const int i)
	{
		int64_t i64 = i;
		return operator+=(i64);
	}


#ifdef _WIN32
	CstringT & operator+=(const DWORD dw)
	{
		return operator+=((unsigned int)dw);
	}

	/*
	CstringT & operator+=(const GUID & guid)
	{
		LPOLESTR g(0);
		HRESULT hr = StringFromCLSID(guid, &g);
		if(FAILED(hr))
		{
			_ASSERTE(hr == S_OK);
			operator+=(_T("GUID FAILED ["));
			operator+=(hr);
			return operator+=(_T("]"));
		}

		CoTaskMemFree(g);

		return operator+=(g);

	}
	*/
#endif
	
	//Return the position from start of the searched string
	SSIZE_T find(const TCHAR *str, uint32_t nBegin = 0) const
	{
		const TCHAR *sz = CstringT<T>::operator TCHAR*();
		const TCHAR *f = _tcsstr(&sz[nBegin], str);
			if(f) return ST_U32(f - sz);

			return npos;
	}

	//Return the position from end of the searched string
	SSIZE_T rfind(const TCHAR *str, uint32_t nBegin = 0) const
	{
		uint32_t result = npos;

		while(true){
			nBegin = find(str, nBegin);
			if(npos == nBegin)
				return result;
			result = nBegin;
			++nBegin;
		}
	}

	//CstringT operator[](uint32_t index) const
	//{
	//	const TCHAR *sz = *this;
	//	CstringT tmp(sz + index /** sizeof(TCHAR)*/);
	//		return tmp;
	//}

	//Return a sub string
	CstringT subString(const size_t nbegin, size_t nLen) const
	{
		CstringT &s = const_cast<CstringT&>(*this);
		//let see if the source buffer is big enough
		nLen = ST_U32((s.m_str->getSize() - nbegin) < nLen?s.m_str->getSize():nLen);
			CResource<CBuffer<TCHAR> > tmp(new CBuffer<TCHAR>(nLen + 1));
			const TCHAR *sz = s;
			tmp->add(sz + nbegin, nLen);

			return tmp;
	}

	//return the size of the string
	const uint32_t size() const
	{
		CstringT &s = const_cast<CstringT&>(*this);
		return s.m_str->getFull();
	}

	const size_t len() const
	{
		CstringT &s = const_cast<CstringT&>(*this);
		return _tcslen(s.m_str->get());
	}

#ifdef ALXSTRING_LEGACY_CONVERSION

	operator int()
	{
		return _ttoi(operator TCHAR*());
	}

	operator int64_t()
	{
		return _tstoi64(operator TCHAR*());
	}

	operator uint64_t()
	{
		return _tcstoui64(operator TCHAR*(), NULL, 10);
	}

	operator double()
	{
		return _tcstod(operator TCHAR*(), NULL);
	}

#else
	operator int()
	{
		int k(0);
		to_type(_T("%d"), &k);

		return k;
	}

	operator int64_t()
	{
		int64_t k(0);
		to_type(_T("%" 	SCNd64), &k);

		return k;
	}

	operator uint64_t()
	{
		uint64_t k(0);
		to_type(_T("%" SCNu64), &k);

		//to_type(_T("%llu"), &k);

		return k;
			
	}	

	operator double()
	{
		double k(0);
		to_type(_T("%lf"), &k);

		return k;
	}


#endif


	operator bool()
	{
		if(   !_tcscmp(CstringT<T>::operator TCHAR*(), _T("1")) 
		   || !_tcscmp(CstringT<T>::operator TCHAR*(), _T("true")) 
		   || !_tcscmp(CstringT<T>::operator TCHAR*(), _T("yes")) 
		   || !_tcscmp(CstringT<T>::operator TCHAR*(), _T("TRUE")) 
		   || !_tcscmp(CstringT<T>::operator TCHAR*(), _T("True")) 
		   )
         return true;
    
        return false;
  	}

	T* GetBuffer(size_t size, bool AutoCommit = true)
	{
		m_str->prepare(size);
		T* t = m_str->getCurrent();
		
		if(AutoCommit)
			CommitBuffer(size);

		return t;
	}

	void CommitBuffer(size_t size)
	{
		//NEVER COMMIT A NULL SIZE
		if(0 >= size)
			return;

		m_str->updatePosition(size);
	}

	void Clear()
	{
		ResetBuffer<T> reset(m_str);

		reset.Reset();
	}


	void append_format(const TCHAR *pszFmtString, ...)
	{	
		TCHAR szLine[2048];
                int retVal = 0;
        
		va_list args;
		va_start( args, pszFmtString );
#ifdef _WIN32
		retVal = StringCbVPrintf(szLine, sizeof(szLine), pszFmtString, args );
#else
		retVal = sprintf(szLine, pszFmtString, args);
#endif
		
		MGBASECHECK(retVal);

		va_end( args );

		operator+=(szLine);
	}



	void append_hex_buffer(const unsigned char * pb, uint32_t size)
	{
		
		DECLARE_CH_HEX;

		for(uint32_t x = 0; x < size; x++)
		{
			char ch     = pb[x];
			char out[4] = {0x0, 0x0, 0x0, 0x0};

			out[0] = s_chHexChars[(ch >> 4) & 0x0F];
			(*this) += reinterpret_cast<const TCHAR *>(out);

		        out[0] = s_chHexChars[ch & 0x0F];
			(*this) += reinterpret_cast<const TCHAR *>(out);

		}
	}


	void extract_binary_hex(unsigned char * pb, uint32_t size, uint32_t start_at = 0)
	{		
		_ASSERTE(size <= ((this->size() - start_at)/2) );
		
		uint32_t k = start_at;
		
		for(uint32_t x = 0; x < size; x++)
		{			
			CstringT t = this->subString(start_at, 2);
			start_at += 2;

			uint64_t b;// = _tcstoui64(t.operator TCHAR*(), NULL, 16);
			
				t.to_type(_T("%" SCNx64), &b);

			pb[x] = (unsigned char)b;
		}
	}


#ifdef _XXWIN32

	CstringT toLower()
	{
		 T* p = m_str->get();

		 Cstring ret;

		 wint_t t;

		 for(uint32_t i = 0; i < this->size(); i++)
		 {
			t = p[i];
			t = _totlower(t);
			ret.m_str->add(reinterpret_cast<const T*>(&t), 1);
		 }

		return ret;
			 
	}


#endif
};

inline std::ostream& operator<< ( std::ostream& os, const Cstring& str)
{
	const TCHAR * pp = str; //.operator TCHAR*();
        
	//os << static_cast<const TCHAR*>(str);
          os << pp;

		return os;

}

inline std::wostream& operator<< ( std::wostream &os, const Cstring &str )
{
		os << static_cast<const TCHAR*>(str);
				return os;
}


inline bool operator<(const Cstring &greater, const Cstring &less)
{
	Cstring& tmp = const_cast<Cstring&>(greater);
	Cstring& tmp2 = const_cast<Cstring&>(less);
		return 0  > _tcscmp(tmp, tmp2) ;
}

inline bool /*operator==*/Equals(const Cstring &s1, const Cstring &s2)
{
	Cstring &str1 = const_cast<Cstring&>(s1);
	Cstring &str2 = const_cast<Cstring&>(s2);
	return str2.operator==(str1.m_str->get());
}

//inline bool operator==(const Cstring &s1, const Cstring &s2)
//{return Equals(s1, s2);}

inline bool operator==(const TCHAR *ch, const Cstring &s2)
{
	return Equals(Cstring(ch), s2);
};

inline bool operator!=(const TCHAR *ch, const Cstring &s2)
{
	return !operator==(ch, s2);
};

inline Cstring operator+(const TCHAR *ch, const Cstring &str)
{
	Cstring tmp(ch);
		tmp += str;

	return tmp;
}

inline Cstring hexformat(const unsigned char *buffer
	, unsigned long buffer_size
	, unsigned long line_width_minus1 = 9
	, int byte_format = 1
)
{

	if (!buffer_size)
		return Cstring();
	
	unsigned long lines = buffer_size / (line_width_minus1 + 1);

	Cstring out(lines * (line_width_minus1 + 5) * 2);

	

	for (unsigned long i = 0; i <= lines; i++)
	{
		for (int x = 0; x < 2; x++)
		{
			int t = (0 == x) ? 0 : byte_format;

			for (unsigned long bidx = 0; bidx <= line_width_minus1; bidx++)
			{
				unsigned long byte_index = bidx + i * (line_width_minus1 + 1);
				unsigned char c = 0x20;

				if (byte_index < buffer_size)
					c = buffer[byte_index];


				switch (t)
				{
				case 0:
					if (c <= 0xF)
						out.append_format(_T("0%x "), c);
					else
						out.append_format(_T("%x "), c);

					break;

				case 1:
					out.append_format(_T("%c"), (0 == c) ? '\0' : c);
					break;
				default:

					out.append_format(_T("%c"), '?');
				}

			}

			out += _T("\t");
		}

		out += _T("\r\n");
	}

	return out;
}

__ALX_END_NAMESPACE

#endif //_ALX__CSTRING


