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
/*
#include <alxbase.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
//#include <dsound.h>
#include <crtdbg.h>
#include <tchar.h>

#ifdef _DEBUG
#include <bitset>
#include <string>
using namespace std;
#endif

#pragma warning( disable : 4244 ) //conversion from 'unsigned long' to 'unsigned short', possible loss of data
*/

__ALX_BEGIN_NAMESPACE

#ifdef _WIN32

class CWaveParser
{
protected:
	WAVEFORMATEX m_wfx;
	int64_t      m_PulseTime;

protected:
	virtual void SetWaveFormat(WAVEFORMATEX* pwfx)
	{
		__m = 32767.0 / 8388607.0;

		memcpy(&m_wfx, pwfx, sizeof(WAVEFORMATEX));
		//m_PulseTime = (1000 * 10000)/static_cast<double>(m_wfx.nSamplesPerSec); //UndredNMilleseconds
		m_PulseTime = (1000 * 10000)/m_wfx.nSamplesPerSec; //UndredNMilleseconds

		_RPT1(_CRT_WARN,"wBitsPerSample %d\n", m_wfx.wBitsPerSample);
		_RPT1(_CRT_WARN,"\nPulseTime %e\n", m_PulseTime);
		_RPT1(_CRT_WARN,"nSamplesPerSec %d\n", m_wfx.nSamplesPerSec);
		_RPT1(_CRT_WARN,"nAvgBytesPerSec %e\n", m_wfx.nAvgBytesPerSec);
		//_RPT1(_CRT_WARN,"wBitsPerSample %d\n", m_wfx.wBitsPerSample);
		_RPT1(_CRT_WARN,"nBlockAlign %d\n", m_wfx.nBlockAlign);
	}

	CWaveParser(): m_PulseTime(0)
	{
		
	}
protected:
	virtual void BytesInSample(int  nSamplesPerChan)
	{
	}

public:

	static int Read24(unsigned char * b)
    {
            int b1 = 0xFF & b[0];
            int b2 = b[1] << 8;
            int b3 = b[2] << 16;
            int b4 = 0;


            if (0 != (0x80 & b[2]))
            {
                b4 = 0xFF << 24;
            }

            return b1 | b2 | b3 | b4;
    }    

	double __m;

	CWaveParser(WAVEFORMATEX* pwfx)
	{
		SetWaveFormat(pwfx);
	}

	int64_t Parse(BYTE* pBuffer, DWORD dwBufferSize, int64_t startTime) 
	{
		_ASSERTE(0 != m_PulseTime);
	
		int64_t UNPulseTime = startTime;

		int nSamplesPerChan = dwBufferSize / m_wfx.nBlockAlign;
		//_ASSERTE((dwBufferSize / 2) == nSamplesPerChan);

		BytesInSample(nSamplesPerChan);
		UINT i = 0;
		switch(m_wfx.wBitsPerSample + m_wfx.nChannels)
		{
			
			case 9:
			{   // Mono, 8-bit
				BYTE *pshortBuffer = reinterpret_cast<BYTE*>(pBuffer);
				
				while(nSamplesPerChan--)
				{
					channelpulse(0, pshortBuffer[i++], UNPulseTime);
					//channelpulse(1, pshortBuffer[i++], UNPulseTime);
					
					UNPulseTime += m_PulseTime;
				}
				break;
			}

			case 10:
			{   // Stereo, 8-bit
				BYTE *pshortBuffer = reinterpret_cast<BYTE*>(pBuffer);
				
				while(nSamplesPerChan--)
				{
					channelpulse(0, pshortBuffer[i++], UNPulseTime);
					channelpulse(1, pshortBuffer[i++], UNPulseTime);
						
					UNPulseTime += m_PulseTime;
				}
				break;
			}

			case 17:
			{ // Mono, 16-bit
				short *pshortBuffer = reinterpret_cast<short*>(pBuffer);
				
				while(nSamplesPerChan--)
				{
					channelpulse(0, pshortBuffer[i++], UNPulseTime);
					UNPulseTime += m_PulseTime;
				}
				
				break;
			}

			case 18:
			{ // Stereo, 16-bit
				short *pshortBuffer = reinterpret_cast<short*>(pBuffer);
				
				while(nSamplesPerChan--)
				{
					channelpulse(0, pshortBuffer[i++], UNPulseTime);
					channelpulse(1, pshortBuffer[i++], UNPulseTime);
					
					UNPulseTime += m_PulseTime;
				}
				break;
			}

			case 26:
			{ // Stereo, 24-bit
				unsigned char *pshortBuffer = reinterpret_cast<unsigned char*>(pBuffer);
				
				while(nSamplesPerChan--)
				{
					channelpulse(0, Read24(&pshortBuffer[i]), UNPulseTime);
					i += 3;
					channelpulse(1, Read24(&pshortBuffer[i]), UNPulseTime);
					i += 3;
					
					UNPulseTime += m_PulseTime;
				}
				break;
			}

			default:
				_ASSERTE(FALSE);
				break;

		} // End of format switch

	
		return UNPulseTime;
	}

	virtual void channelpulse(int channel, short pulse, int64_t startTime) 
	{

	}

	
	virtual void channelpulse(int channel, BYTE pulse, int64_t startTime)
	{
		channelpulse(channel, static_cast<short>(pulse), startTime);
	}

	virtual void channelpulse(int channel, int pulse, int64_t startTime)
	{
		//SCALE TO 16 BIT
		short p = static_cast<short>(pulse * __m);
        channelpulse(channel, p, startTime);
	}

	virtual ~CWaveParser(void)
	{
	}

	
};

class CWaveParserAutoTime: public  CWaveParser
{
	int64_t m_time;
public:
	CWaveParserAutoTime(WAVEFORMATEX* pwfx):CWaveParser(pwfx), m_time(0){};

	int64_t Parse(BYTE* pBuffer, DWORD dwBufferSize)
	{
		m_time = CWaveParser::Parse(pBuffer, dwBufferSize, m_time);
		return m_time;
	}

	int64_t Parse(BYTE* pBuffer, DWORD dwBufferSize, int64_t time)
	{
		m_time = CWaveParser::Parse(pBuffer, dwBufferSize, time);
		return m_time;
	}
};

#endif

class TC
{

public:
	DWORD _nFrames;
	DWORD _dwFrames;
	DWORD _dwSeconds;
	DWORD _dwMinutes;
	DWORD _dwHours;
public:
	TC(TC &rhs):_nFrames(25)
	{
		*this = rhs;
	}

	TC(DWORD dwFrames, DWORD dwSeconds, DWORD dwMinutes, DWORD dwHours, DWORD frames = 25):
	_dwFrames(dwFrames),
	_dwSeconds(dwSeconds),
	_dwMinutes(dwMinutes),
	_dwHours(dwHours),
	_nFrames(frames)
	{

		
		
	}

	TC():
	_dwFrames(0),
	_dwSeconds(0),
	_dwMinutes(0),
	_dwHours(0),
	_nFrames(25)
	{

		
		
	}

	TC(uint32_t dwTimecode):_nFrames(25)
	{
		_dwFrames   = ((((0x000000F0 & dwTimecode)>> 4) %16) * 10 ) + (0x0000000F & dwTimecode)%10;		
        _dwSeconds  = ((((0x0000F000 & dwTimecode)>> 12)  % 16 ) * 10 )  + ((0x00000F00 & dwTimecode) >> 8)%10;
		_dwMinutes  = ((((0x00F00000 & dwTimecode) >> 20 )% 16 ) * 10 )  + ((0x000F0000 & dwTimecode) >> 16 )%10;
		_dwHours    = ((((0xF0000000 & dwTimecode) >> 28 )% 16 ) * 10 )  + ((0x0F000000 & dwTimecode) >> 24)%10;
        //dwTimecode |= ((_dwMinutes / 10 ) << 20 )  | ((_dwMinutes % 10) << 16);
        //dwTimecode |= ((_dwHours / 10 ) << 28 )    | ((_dwHours % 10) << 24);
		
	}

	TC(int64_t time):_nFrames(25)
	{
		_dwFrames   = ( (DWORD)((time / 10000) % 1000) ) / (1000/_nFrames);
        _dwSeconds  = (DWORD)(time / (10000 * 1000) ) % 60;
		_dwMinutes  = (DWORD)(time / (10000 * 1000 * 60)) % 60;
		_dwHours    = (DWORD)(time / (10000 * 1000 * 60)) / 60;
	}

	TC & operator=(TC &rhs)
	{
		_dwFrames  = rhs._dwFrames;
	    _dwSeconds = rhs._dwSeconds;
	    _dwMinutes = rhs._dwMinutes;
	    _dwHours   = rhs._dwHours;
		return *this;
	}

	bool isValid()
	{
		return 
				(_nFrames > _dwFrames)  &
				(60 > _dwSeconds) &
				(60 > _dwMinutes) &
				(24 > _dwHours);
	}

	TC operator+(TC &rhs) const
	{
		TC tc;

		tc._dwFrames  = (_dwFrames + rhs._dwFrames)%_nFrames;
		DWORD dwFR = (_dwFrames + rhs._dwFrames)/_nFrames;

		tc._dwSeconds  = (_dwSeconds + rhs._dwSeconds + dwFR)%60;
		DWORD dwSR = (_dwSeconds + rhs._dwSeconds + dwFR)/60;

		tc._dwMinutes  = (_dwMinutes + rhs._dwMinutes + dwSR)%60;
		DWORD dwMR = (_dwMinutes + rhs._dwMinutes + dwSR)/60;
		
		tc._dwHours = (_dwHours + rhs._dwHours + dwMR)%24;
		//DWORD dwMR = (_dwMinutes + rhs._dwMinutes + dwSR)/60;
				
		return tc;
	}

	TC operator-(TC &rhs) const
	{
		TC tc;
		
		DWORD dwFR = 0;
		long lF  = (_dwFrames - rhs._dwFrames);
		if(0 > lF)
		{
			lF = (-1)*lF;

			dwFR = lF/25 + 1;
			tc._dwFrames = _nFrames - lF%_nFrames;
		}
		else
			tc._dwFrames = lF;

		DWORD dwSR = 0;
		long lS  = (_dwSeconds - rhs._dwSeconds - dwFR);
		if(0 > lS)
		{
			lS = (-1)*lS;

			dwSR = lS/60 + 1;
			tc._dwSeconds = 60 - lS%60;
		}
		else
			tc._dwSeconds = lS;


		DWORD dwMR = 0;
		long lM  = (_dwMinutes - rhs._dwMinutes - dwSR);
		if(0 > lM)
		{
			lM = (-1)*lM;

			dwMR = lM/60 + 1;
			tc._dwMinutes = 60 - lM%60;
		}
		else
			tc._dwMinutes = lM;

		long lH  = (_dwHours - rhs._dwHours - dwMR);
		if(0 > lH)
		{
			if(-23 < lH)
				tc._dwHours = 0;
			else
				tc._dwHours = 24 + lH;
		}
		else
			tc._dwHours = lH;
		
		return tc;
	}

	TC operator++()
	{
		TC tc(1,0,0,0);

		TC tmp  = operator+(tc);

		return operator=(tmp);
	}

	operator uint32_t()
	{

		uint32_t 
		dwTimecode  = ((_dwFrames/10) << 4 )       | (_dwFrames % 10);
        dwTimecode |= ((_dwSeconds / 10 ) << 12 )  | ((_dwSeconds % 10) << 8);
        dwTimecode |= ((_dwMinutes / 10 ) << 20 )  | ((_dwMinutes % 10) << 16);
        dwTimecode |= ((_dwHours / 10 ) << 28 )    | ((_dwHours % 10) << 24);

		return dwTimecode;

	}
	
	void GetTCHAR(TCHAR * pszTimeCode, DWORD *dwsize, TCHAR * separator)
	{
	
	   	memset(pszTimeCode, 0, *dwsize);
		//_i64tot(operator DWORD(), pszTimeCode, 16);

		//_sntprintf(
#ifdef _WIN32
		_sntprintf_s(
		pszTimeCode, *dwsize 
		, _TRUNCATE
#else
		_snprintf(pszTimeCode, *dwsize
#endif
		,_T("%2u%s%2u%s%2u%s%2u")
		, 
										_dwHours,
										separator,
										_dwMinutes,
										separator,
										_dwSeconds,
										separator,
										_dwFrames);


		
	}

	TC SumHnano(int64_t rhs)
	{
		DWORD Hours   = static_cast<uint32_t>((rhs / (10000 * 1000 * 60)) / 60); 
		DWORD Minutes = static_cast<uint32_t>((rhs / (10000 * 1000 * 60)) % 60) ;
		DWORD Seconds = static_cast<uint32_t>((rhs / (10000 * 1000) ) % 60);
		DWORD Frames  = static_cast<uint32_t>(( ((rhs / 10000) % 1000) ) / (1000/25));

		TC tc(Frames, Seconds, Minutes, Hours);

		TC tc2 = operator+(tc);
			
		return tc2;
	}

	bool IsZero()
	{
		return (0 == _dwFrames && 0 == _dwSeconds && 0 == _dwMinutes && 0 == _dwHours);
	}

	bool operator==(TC& rhs)
	{
		return (_dwFrames == rhs._dwFrames) &&
			   (_dwSeconds == rhs._dwSeconds ) &&
			   (_dwMinutes == rhs._dwMinutes ) && 
			   (_dwHours == rhs._dwHours);
	}

	bool operator!=(TC& rhs)
	{
		return !operator==(rhs);
	}

	void set_n_frames(DWORD n){_nFrames = n;}

};

struct __shift_t_code
{
	unsigned long one;
	unsigned long two;
	unsigned short check;
};


class CTimeCode
{
private:
	__shift_t_code m_time_code;
	void init()
	{
		m_time_code.one    = 0;
		m_time_code.two    = 0;
		m_time_code.check  = 0;
	}
public:
	CTimeCode()
	{
		init();
	}

	void ReSet()
	{
		init();
	}

	bool IsValid()
	{
		return (0x3FFD == m_time_code.check);
	}

	unsigned short FrameUnit()
	{
		_ASSERTE(IsValid());
		return ((0xF0000000 & m_time_code.one) >> 28);
	}

	unsigned short FrameTens()
	{
		_ASSERTE(IsValid());
		return ((0x00C00000 & m_time_code.one) >> 22);
	}

	unsigned short Frame()
	{		
		return FrameTens() + FrameUnit();
	}

	unsigned short HoursUnit()
	{
		_ASSERTE(IsValid());
		return ((0x0000F000 & m_time_code.two) >> 12);
	}

	unsigned short HoursTens()
	{
		_ASSERTE(IsValid());
		return ((0x000000C0 & m_time_code.two) >> 6);
	}

	unsigned short Hours()
	{		
		return HoursTens() + HoursUnit();
	}

	unsigned short SecsUnit()
	{
		_ASSERTE(IsValid());
		return ((0x0000F000 & m_time_code.one) >> 12);
	}

	unsigned short SecsTens()
	{
		_ASSERTE(IsValid());
		return ((0x000000E0 & m_time_code.one) >> 5);
	}

	unsigned short Seconds()
	{		
		return SecsTens() + SecsUnit();
	}

	unsigned short MinsUnit()
	{
		_ASSERTE(IsValid());
		return ((0xF0000000 & m_time_code.two) >> 28);
	}

	unsigned short MinsTens()
	{
		_ASSERTE(IsValid());
		return ((0x00E00000 & m_time_code.two) >> 21);
	}

	unsigned short Minutes()
	{		
		return MinsTens() + MinsUnit();
	}

	static unsigned short DecodeUnits(unsigned  short n)
	{
		//switch(u)
		//{
		//	case 0:
		//		return 0;
		//	case 8:
		//		return 1;
		//	case 4:
		//		return 2;
		//	case 12:
		//		return 3;
		//	case 2:
		//		return 4;
		//	case 10:
		//		return 5;
		//	case 6:
		//		return 6;
		//	case 14:
		//		return 7;
		//	case 1:
		//		return 8;
		//	case 9:
		//		return 9;

		//}

		//return 200000 + u;

		  unsigned  short one   = (0x01 & n) << 3;
          unsigned  short two   = (0x02 & n) << 1;
          unsigned  short three = (0x04 & n) >> 1;
          unsigned  short four  = (0x08 & n) >> 3;

         return one | two | three | four;
	}

	static unsigned short Decode3Tens(unsigned  short n)
	{
		//switch(t)
		//{
		//	case 0:
		//		return 0;
		//	case 2:
		//		return 1;
		//	case 1:
		//		return 2;

		//	
		//	
		//}

		//return 100000 + t;

		unsigned short one = (0x01 & n) << 1;
        unsigned short two = (0x02 & n) >> 1;

        return one | two ;
	}

	static unsigned short Decode6Tens(unsigned  short n)
	{
		//switch(t)
		//{
		//	case 0:
		//		return 0;
		//	case 4:
		//		return 1;
		//	case 2:
		//		return 2;
		//	case 6:
		//		return 3;
		//	case 1:
		//		return 4;
		//	case 5:
		//		return 5;

		//	
		//}
		//
		//return 300000 + t;

		 unsigned  short one   = (0x01 & n) << 2;
         unsigned  short two   = (0x02 & n);
         unsigned  short three = (0x04 & n) >> 2;

         return one | two | three ;
	}

	TC GetTimeCode()
	{
		TC t(
				(Decode3Tens(FrameTens()) * 10 + 
				 DecodeUnits(FrameUnit())),

			    (Decode6Tens(SecsTens())  * 10 +
				 DecodeUnits(SecsUnit())),

				(Decode6Tens(MinsTens())  * 10 +
				 DecodeUnits(MinsUnit())),
			
				(Decode3Tens(HoursTens()) * 10 +
			     DecodeUnits(HoursUnit()))
			);
		
		return t;
	}
	

	void push(bool bit)
	{
		//Read the exiting bit in two
		bool two = (0x80000000 & m_time_code.two)?true:false;
		//shift one
		m_time_code.one <<= 1;
		//add the exiting bit from two in one
		m_time_code.one |= (two)?1:0;

		//same for two
		bool sh = (0x8000 & m_time_code.check)?true:false;
		//shift two
		m_time_code.two <<= 1;
		//add the exiting bit from two in one
		m_time_code.two |= (sh)?1:0;

		//add the bit to shift
		m_time_code.check <<= 1;
		//add the exiting bit from two in one
		m_time_code.check |= (bit)?1:0;
/*
#ifdef _DEBUG
		Dump();
#endif
*/
	}

	unsigned short last(){return m_time_code.check & 0x0001;}

#ifdef _DEBUG
#ifdef _WIN32

	void P(unsigned long l)
	{
		for(unsigned int i = 0; i < 32; ++i)
		{
			_RPT1(_CRT_WARN, "%d", ((0x80000000 >> i) & l)?1:0);
		}
		_RPT0(_CRT_WARN, "\t");
	}

	void P(unsigned short s)
	{
		for(unsigned short i = 0; i < 16; ++i)
		{
			_RPT1(_CRT_WARN, "%d", ((0x8000 >> i) & s)?1:0);
		}
		_RPT0(_CRT_WARN, "\t");
	}

	void Dump()
	{
		/*
		_RPT3(_CRT_WARN, "ROW TIME CODE: %x %x %x \n", 
			  m_time_code.one,
			  m_time_code.two,
			  m_time_code.check);
		*/
    	P(m_time_code.one);
		P(m_time_code.two);
		P(m_time_code.check);

		_RPT0(_CRT_WARN, "\n");
		
	}
#endif
#endif
};


#ifdef _WIN32

class CBiPhaseMonoDecoder: public CWaveParser
{
	CTimeCode    _tc;
    
	int _soundToBiphasePeriod;
    int _soundToBiphaseLimit;
    int _soundToBiphaseCnt;
    int _soundToBiphaseMin;
	int _soundToBiphaseMax;
	int _biphaseToBinaryPrev;
    int _biphaseToBinaryState;

	bool _soundToBiphaseState;
        
    double       _fr;

	unsigned short _channel_target;



        uint64_t _total_pcm;

        static const short SAMPLE_CENTER = 128;

        void init()
        {
            _soundToBiphasePeriod = (int)(m_wfx.nSamplesPerSec / _fr / 80);
            _soundToBiphaseLimit = (_soundToBiphasePeriod * 14) / 16;

            //_check_interval = _soundToBiphasePeriod / 4;

            //if (m_wfx.wBitsPerSample > 8)
            //    SAMPLE_CENTER = 16384;
        }
        

        bool push_biphase(bool biphase, uint64_t startTime)
        {
            bool ret = false;

            if (biphase == ((_biphaseToBinaryPrev)?true:false) ) {
			        _biphaseToBinaryState = 1;
			        _tc.push(false);
                    ret = true;
		        } else {
			        _biphaseToBinaryState = 1 - _biphaseToBinaryState;
			        if (_biphaseToBinaryState == 1) 
                    {
                        _tc.push(true);
                        ret = true;
                    }                 
		        }

            if (ret)
            {
#ifdef _DEBUG
                //_RPT4(_CRT_WARN, 
                //            "Bit\t%d\t%I64u\t[%d %d]\r\n"  //\t%d"
                //            , _tc.last()
                //            , _total_pcm
                //            , _biphaseToBinaryPrev
                //            , biphase
                //            //, _biphaseToBinaryState
                //            );
#endif
                if (_tc.IsValid())
                {
					Time_Code(_tc, startTime);
			        //_tc.ReSet();
                }

            }
            else
            {
                        //_RPT4(_CRT_WARN,
                        //    "XXX\t%d\t%I64u\t[%d %d]\r\n" //\t{4}"
                        //    , biphase
                        //    , _total_pcm
                        //    , _biphaseToBinaryPrev
                        //    , biphase
                        //    //, _biphaseToBinaryState
                        //    );

            }

		     _biphaseToBinaryPrev = biphase;

             return ret;
        }


protected:
	//CBiPhaseMonoDecoder(): 
	//	  _soundToBiphasePeriod(0)
	//	, _soundToBiphaseLimit(0)
	//	, _soundToBiphaseCnt(0)
	//	, _soundToBiphaseMin(0)
	//	, _soundToBiphaseMax(0)
	//	, _biphaseToBinaryPrev(0)
	//	, _biphaseToBinaryState(0)
	//	, _soundToBiphaseState(0)
	//	, _soundToBiphaseState(false)
	//{
	//	init();
	//}

protected:
	virtual void SetWaveFormat(WAVEFORMATEX* pwfx)
	{
		CWaveParser::SetWaveFormat(pwfx);
		init();
	}

public:
	CBiPhaseMonoDecoder(): 
		  _soundToBiphasePeriod(0)
		, _soundToBiphaseLimit(0)
		, _soundToBiphaseCnt(0)
		, _soundToBiphaseMin(0)
		, _soundToBiphaseMax(0)
		, _biphaseToBinaryPrev(0)
		, _biphaseToBinaryState(0)
		, _soundToBiphaseState(0)
		, _fr(25)
		, _total_pcm(0)
		, _channel_target(0)
	{
		//init();
		_RPT0(_CRT_WARN, "HOURS TENS\tHOURS UNIT\tMINS TENS\tMINS UNIT\tSECS TENS\tSECS UNIT\tFRAME TENS\tFRAME UNIT\n"); 
	}

	
	virtual void channelpulse(int channel, short pulse_x, int64_t startTime) 
	{

		if(_channel_target != channel)
			return;
        
		int pulse = pulse_x;


            bool ret = false;


            if (pulse_x < 0)
                if (m_wfx.wBitsPerSample < 16)
                {
                    pulse &= 0x7FFFFF7F;
                    
                    pulse = ~pulse;
                    pulse += 1;

                    pulse &= 0x000000FF;
                }

			

            if (m_wfx.wBitsPerSample < 16)
            {
                _soundToBiphaseMin = (short)(SAMPLE_CENTER - (((SAMPLE_CENTER - _soundToBiphaseMin) * (short)15) / (short)16));
                _soundToBiphaseMax = (short)(SAMPLE_CENTER + (((_soundToBiphaseMax - SAMPLE_CENTER) * (short)15) / (short)16));
            }
            else
            {
                _soundToBiphaseMin = _soundToBiphaseMin * 15 / 16;
                _soundToBiphaseMax = _soundToBiphaseMax * 15 / 16;
            }


            if (pulse < _soundToBiphaseMin)
                _soundToBiphaseMin = pulse;
            if (pulse > _soundToBiphaseMax)
                _soundToBiphaseMax = pulse;

            int min_threshold = 0;
            int max_threshold = 0;

            if (m_wfx.wBitsPerSample < 16)
            {
                min_threshold = SAMPLE_CENTER - (((SAMPLE_CENTER - _soundToBiphaseMin) * 8) / 16);
                max_threshold = SAMPLE_CENTER + (((_soundToBiphaseMax - SAMPLE_CENTER) * 8) / 16);
            }
            else
            {
                min_threshold = _soundToBiphaseMin * 8 / 16;
                max_threshold = _soundToBiphaseMax * 8 / 16;
            }

            //_RPT4(_CRT_WARN, "pulse\t%d\t%I64u\t%d\t%d\r\n"
            //    , pulse_x
            //    , _total_pcm
            //    , min_threshold
            //    , max_threshold
            // );


            if (
               (_soundToBiphaseState && (pulse > max_threshold)) 
            || (!_soundToBiphaseState && (pulse < min_threshold)) 
           )
            {
                if (_soundToBiphaseCnt > _soundToBiphaseLimit)
                {
                    ret = push_biphase(_soundToBiphaseState, startTime);
                    ret = push_biphase(_soundToBiphaseState, startTime);
                }
                else
                {
                    ret = push_biphase(_soundToBiphaseState, startTime);
                 
                    _soundToBiphaseCnt *= 2;
                }

                _soundToBiphasePeriod = (_soundToBiphasePeriod * 3 + _soundToBiphaseCnt) / 4;
                _soundToBiphaseLimit  = (_soundToBiphasePeriod * 14) / 16;

                _soundToBiphaseCnt   = 0;
                _soundToBiphaseState = !_soundToBiphaseState;
            }

            _soundToBiphaseCnt++;
            _total_pcm++;


	}

	virtual void Time_Code(CTimeCode &timeCode, int64_t startTime)
	{
#ifdef _DEBUG
		//timeCode.Dump();
#endif
		/*
		_RPT2(_CRT_WARN, "%d\t%d\t", timeCode.HoursTens(), timeCode.HoursUnit());
		_RPT2(_CRT_WARN, "%d\t%d\t", timeCode.MinsTens(),  timeCode.MinsUnit());
		_RPT2(_CRT_WARN, "%d\t%d\t", timeCode.SecsTens(),  timeCode.SecsUnit());
		_RPT2(_CRT_WARN, "%d\t%d\n", timeCode.FrameTens(), timeCode.FrameUnit());
		*/

		
		_RPT1(_CRT_WARN,"startTime %.I64\n", startTime);
		_RPT2(_CRT_WARN, "%d%d:", timeCode.Decode3Tens(timeCode.HoursTens()), timeCode.DecodeUnits(timeCode.HoursUnit()));
		_RPT2(_CRT_WARN, "%d%d:", timeCode.Decode6Tens(timeCode.MinsTens()), timeCode.DecodeUnits(timeCode.MinsUnit()));
		_RPT2(_CRT_WARN, "%d%d:", timeCode.Decode6Tens(timeCode.SecsTens()), timeCode.DecodeUnits(timeCode.SecsUnit()));
		_RPT2(_CRT_WARN, "%d%d\n", timeCode.Decode3Tens(timeCode.FrameTens()), timeCode.DecodeUnits(timeCode.FrameUnit()));
	}
};



#endif

__ALX_END_NAMESPACE

