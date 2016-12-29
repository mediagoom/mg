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

#include <mgcore.h>


#include "TBitstream.h"

#include "mp4/mpeg2ts.h"
#include "mp4/mpeg2au.h"

__ALX_BEGIN_NAMESPACE

//0xC0 - 0xDF MPEG-1 or MPEG-2 audio stream 
//0xE0 - 0xEF 


#define PACK_HEADER 0xBA
#define SEQUENCE_END 0xB7

#define PS_MAP_TABLE 0x000001BC
#define PS_DIRECTORY_TABLE 0x000001FF

#define MPEG2_START_VIDEO_STREAM 0xE0
#define MPEG2_END_VIDEO_STREAM 0xEF

#define MPEG2_START_AUDIO_STREAM 0xC0
#define MPEG2_END_AUDIO_STREAM 0xDF

#define MPEG2_SEQUENCE_HEADER 0xB3
#define MPEG2_GOP_HEADER 0xB8

#define MPEG2_PIC_HEADER 0x00
#define MPEG2_SYS_HEADER 0xBB

/*
 * Various start code
 */
#define SEQ_START_CODE 0x000001b3
#define SEQ_END_CODE   0x000001b7
#define ISO_11172_END_CODE 0x000001b9
#define PACK_START_CODE 0x000001ba
#define SYS_START_CODE 0x000001bb
#define PIC_START_CODE 0x00000100
#define GOP_START_CODE 0x000001b8
#define EXT_START_CODE 0x000001b5
#define USER_START_CODE 0x000001b2
#define SLICE_MIN_START_CODE 0x00000101
#define SLICE_MAX_START_CODE 0x000001af
#define PACKET_MIN_START_CODE 0x0000001bc
#define PACKET_MAX_START_CODE 0x0000001f0

/*
 * Return Code
 */

#define DVM_MPEG_OK 0
#define DVM_MPEG_NOT_FOUND -1
#define DVM_MPEG_INVALID_START_CODE -2
#define DVM_MPEG_INDEX_FULL 1

#define I_FRAME 1
#define P_FRAME 2
#define B_FRAME 3
#define D_FRAME 4

/*
 *  PROGRAM STREAM
 */

#define PESPACKSIZE 2048



extern int mpegbitmask[];

//inline bool operator==(AudioHeader &lhs, AudioHeader &rhs)
//{
//	return  lhs.syncword == rhs.syncword &&
//			lhs.id == rhs.id &&
//			lhs.layer == rhs.layer &&
//			lhs.protection_bit == rhs.protection_bit &&
//			lhs.bitrate_index == rhs.bitrate_index &&
//			lhs.sampling_frequency == rhs.sampling_frequency &&
//			lhs.padding_bit == rhs.padding_bit &&
//			lhs.private_bit == rhs.private_bit &&
//			lhs.mode == rhs.mode &&
//			lhs.mode_extension == rhs.mode_extension &&
//			lhs.copyright == rhs.copyright &&
//			lhs.original_copy == rhs.original_copy &&
//			lhs.emphasis == rhs.emphasis;
//}



class CBufferRead: public CBuffer<unsigned char>
{
		
    short        _availablebits;
	unsigned int _bits;
	



	uint64_t _position;
	ResetBuffer<BYTE> _r;
	

public:
	CBufferRead(size_t size = 1024):
	  CBuffer(size),
		_availablebits(0),
		_bits(0)
		, _position(0)
		, _r(*this)
	{}



	virtual void Read(/*[out]*/  void* pv, /*[in]*/ULONG cb, /*[out]*/ULONG* pcbRead)
	{
		bool b = CBuffer<BYTE>::ReadBuffer(reinterpret_cast<BYTE*>(pv), cb, *pcbRead, _position);
		_ASSERTE(b);
		_position += *pcbRead;
	}

	virtual void Write( /*[in]*/ void const* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG* pcbWritten)
	{
		if (cb == 0)
		{
			*pcbWritten = 0;
			return;
		}

		add(reinterpret_cast<const BYTE*>(pv), cb);
		*pcbWritten = cb;
	}

	

	void Reset()
	{
		_r.Reset();
		_position = 0;
	}

	uint64_t ByteRead()
	{
		return _position;
	}

	uint64_t ByteToRead()
	{
		return this->getFull() - _position;
	}

	virtual void Seek(uint64_t cb)
	{
		_ASSERTE((_position + cb) <= this->getSize());
		_position += cb;
	}

	virtual void SetPosition(uint64_t cb) { _position = cb; }
	virtual uint64_t GetPosition() { return _position; }

	virtual void SeekBack(ULONG cb)
	{
		_ASSERTE(_position >= cb);
		_position -= cb;
	}

	BYTE * GetCurrentPosition()
	{
		return get() + ByteRead();
	}

	//BYTE &operator[](ULONG index){return CBuffer<BYTE>::operator[](index + _position);};


	/*
	virtual void Read( void* pv, ULONG cb, ULONG* pcbRead)
	{

		_ASSERTE((cb > 1 && 0 == _availablebits) || cb == 1);

		CBufferPersist::Read(pv, cb, pcbRead);
	}
	*/

	BYTE &getAt(size_t index){return *(GetCurrentPosition() + index);};
	

	BYTE  PeekByte() { return (*GetCurrentPosition()); }
	BYTE  GetByte() { 
		BYTE b = PeekByte(); 
		_position++;
		return b;
	}
	short GetShort() {
		short b = GetByte();
		      b |= (GetByte() << 8);
	    return b;
	}
	int GetReversedInt()   
	{
		int i  = GetByte() << 24;
		    i |= GetByte() << 16;
			i |= GetByte() << 8;
			i |= GetByte();

		return i;
	}
	short GetReversedShort()
	{
		short s  = GetByte() << 8;
		      s |= GetByte();
		return s;
	}

	void RestoreInt(){SeekBack(sizeof(int));}

	int PeekBits(int num)                     
	{
		_ASSERTE(16 >= num);
		int size    = sizeof(unsigned short)*8;
		int sizeall = sizeof(_bits)*8;
		_ASSERTE((size + sizeof(BYTE)*8) <= sizeall);

		while(_availablebits < num)
		{
			_ASSERTE(0 < ((sizeall - _availablebits) - (sizeof(BYTE)*8)));
			unsigned int b = GetByte();
			_bits |= b << ((sizeall - _availablebits) - (sizeof(BYTE)*8));
			_availablebits += (sizeof(BYTE)*8);
		}

		int bitCount = (size - num);
		int ret = (_bits >> (sizeall - size)) & 0x0000ffff;
		return ret >> bitCount & mpegbitmask[num]; 
	}

	int GetBits(int num)
	{
		int ret = PeekBits(num);
		_availablebits -= num;
		_bits <<= num;

		return ret;
		
	}

	void ByteAlign()
	{
		GetBits(_availablebits);
	}


	operator CBufferRead*(){return this;}
};

/*

class CMPEG2FILE_PROGRAMSTREAM: protected SHStream
{
private:
	CBufferRead       _buffer; //Current Buffer used for Retaining data
	uint64_t  _read;   //Bytes Read in the file
	uint64_t  _duration;

	CResource<SequenceHeader> _sequence;
	CResource<AudioHeader>    _audioHeader;

	uint64_t _audio_stream_id;
	uint64_t _video_stream_id;

	PackHeader   _packHeader;
	SystemHeader _systemHeader;
	PesData      _pesHeader;

	uint64_t ToReadInFile()
	{
		return size() - _read;
	}

	uint64_t ReverseToReadInFile()
	{
		return _read;
	}

	bool _IsAtHeader()
	{
		 return (!_buffer.getAt(0) && !_buffer.getAt(1) && 1 == _buffer.getAt(2));
	}

	bool _IsAtAudioHeader()
	{
		//return (_buffer.getAt(0) == 0xFF && (_buffer.getAt(1) & 0xE0) == 0xE0);


		// sync bytes found?
		// for performance reasons check already that it is not data within an empty frame (all bits set)
		// therefore check wether the bits for bitrate are all set -> means that this is no header!
		
		bool H = (_buffer.getAt(0) == 0xFF && (_buffer.getAt(1) & 0xE0) == 0xE0);
		bool E = ((_buffer.getAt(2) & 0xF0) == 0xF0);
		
		//_ASSERTE(!H || ( H && !E) );

		return  H && !E;

		
	}
	
	bool _PositionToHeaderByte(ULONG &lookMax)
	{
		ULONG l(0);
		while((_buffer.getAt(0) || _buffer.getAt(1) || 1 != _buffer.getAt(2)) && _buffer.ByteToRead())
		{
			if(l >= lookMax && 0 < lookMax)
			{return false;}

			_buffer.Seek(1);
			l++;
		}

		_ASSERTE(lookMax >= l || 0 == lookMax);
		lookMax -= l;
		return (_buffer.ByteToRead())?true:false;
	}

	bool _PositionToHeader(ULONG &lookMax)
	{
		bool read = false;
		while(!(read = _PositionToHeaderByte(lookMax)) && ToReadInFile())
		{
			if(_buffer.ByteToRead() && 0 < lookMax)//we do not want to read more
			{
				return false;
			}
			ReadBytes();
		}

		return read;
	}

	bool _PositionToAudioHeader(ULONG &lookMax, bool StopAtMPEGHeader = false)
	{
		ULONG l(0);

		while(  
			   (!_IsAtAudioHeader()) 
			   && (_buffer.ByteToRead() >= 4)
			)
		{
			//Do not go over other mpeg2 header
			if(StopAtMPEGHeader && 0 == _buffer.getAt(0) && 0 == _buffer.getAt(1) && 1 == _buffer.getAt(2))
				return false;

			if(l >= lookMax && 0 < lookMax)
				return false;

			_buffer.Seek(1);
			l++;

			
		}
        _ASSERTE(lookMax >= l);
		lookMax -= l;

		return (_buffer.ByteToRead() >= 4)?true:false;
	}

	void MoveToEnd()
	{
		_read = this->size();
		_buffer.Reset();

		ReverseReadBytes();
	}

	bool _ReversePositionToHeader()
	{
		while((4 <= _buffer.ByteRead()) && (_buffer.getAt(-4) || _buffer.getAt(-3) || 1 != _buffer.getAt(-2)))
		{
			_buffer.SeekBack(1);
		}

		return (4 <= _buffer.ByteRead())?true:false;
	}

	

public:

	uint64_t size(){return SHStream::size();}
	
	bool PositionToAudioHeader(ULONG &lookMax, bool StopAtMPEGHeader = false){return _PositionToAudioHeader(lookMax, StopAtMPEGHeader);}
	
	bool PositionToAudioHeaderReload()
	{
		ULONG max = ToReadInFile();
		while(!PositionToAudioHeader(max) && ToReadInFile() > 3)
		{
			ReadBytes();
		}

		return (3 < ToReadInFile())?true:false;
	}
	
	__int64 FirstCheckedAudioOnlyHeaderPosition(int checks = 10)
	{
		MoveToBeginning();

		int valid = 0;
		__int64 p = -1;

		
		ULONG max = 0;
        
		uint64_t pos  = 0;
		if(PositionToAudioHeaderReload())
		{
			pos  = CurrentPosition();
			ReadStruct(_audioHeader);
		}
		else 
			return -1;


				
		uint64_t next = pos + _audioHeader->getFrameLength();
		p = pos;

		while(valid < checks)
		{
			if(p == -1)
				p = pos;

			if(PositionToAudioHeaderReload())
			{
				_ASSERTE(CurrentPosition() > pos);
				pos  = CurrentPosition();
				_ASSERTE(_buffer.ByteToRead() >= 4);
				ReadStruct(_audioHeader);
			}
			else 
				return -1;

			_ASSERTE(CurrentPosition() > pos);
					
			if(next == pos)
			{
				valid++;
				next = pos + _audioHeader->getFrameLength();
			}
			else
			{
				if(next < pos)
				{
					valid =  0;
					p     = -1;
					next = pos + _audioHeader->getFrameLength();
				}
			}			

			
		}

		return p;
	}
	
	bool PositionToAnyHeader(ULONG &lookMax){return _PositionToHeader(lookMax);}
	//bool PositionToHeader(ULONG lookMax){return _PositionToHeader(lookMax);}

	bool PositionToHeader()
	{
		ULONG M(0);
		return _PositionToHeader(M);
	}
	//<summary>
	//	Extract bytes from the MPEG File and make them available for Processing.
	//</summary>
	void ReadBytes(ULONG size = 0)
	{
		//_RPTW3(_CRT_WARN, L"READING BYTES\t%d\t%d\t%d\t", size, _buffer.getSize(), _read);
		
//#if _DEBUG
//		uint64_t xpos = CurrentPosition();
//#endif
		if(0 == size)
			size = _buffer.getSize();

		    uint64_t toread(0);
			toread = _buffer.ByteToRead();
					
			ULONG read = 0;
			_buffer.Reset();
			_buffer.prepare(size - 1);

			_ASSERTE(_buffer.getFree() == _buffer.getSize());

			_ASSERTE(toread <= _read);

			uint64_t position = _read - toread;
			 this->SetPosition(position);

				this->Read(_buffer.get(), _buffer.getFree(), &read);
				_buffer.updatePosition(read);
			
			_ASSERTE(_buffer.getFull() == read);
				
			//_read += _buffer.getFull() - toread;

			_read = position + _buffer.getFull();

			_ASSERTE(_read <= this->size());

			//_RPTW3(_CRT_WARN, L"READED  BYTES\t%d\t%d\t%d\t", size, read, _read);
			//_RPTW3(_CRT_WARN, L"%d\t%d\t%d\n", position, toread, _buffer.getSize());

//#if _DEBUG
//		_ASSERTE(xpos == CurrentPosition());
//#endif
		
	}

	void ReverseReadBytes(ULONG size = 0)
	{
//#if _DEBUG
//		uint64_t xpos = CurrentPosition();
//#endif
		if(0 == size)
			size = _buffer.getSize();

		    uint64_t toread = _buffer.ByteRead();
					
			ULONG read = 0;
			_buffer.Reset();
			_buffer.prepare(size - 1);

			_ASSERTE(_buffer.getFree() == _buffer.getSize());
			_ASSERTE((toread + _read) >= size);
			_ASSERTE((toread + _read - size) <= this->size());

			//uint64_t position = _read + toread - size;

			//this->SetPosition(position);

			//	this->Read(_buffer.get(), _buffer.getFree(), &read);
			//	_buffer.updatePosition(read);
			//
			//_ASSERTE(_buffer.getFull() == read);
			//	
			//_read = position - read;

			//_buffer.SetPosition(_buffer.size());

			uint64_t position = CurrentPosition() - size;

			this->SetPosition(position);

			this->Read(_buffer.get(), _buffer.getFree(), &read);
			_buffer.updatePosition(read);
			
			_ASSERTE(_buffer.getFull() == read);
				
			_read = position + _buffer.getFull();

			_buffer.SetPosition(_buffer.size());
//#if _DEBUG
//		_ASSERTE(xpos == CurrentPosition());
//#endif
		
	}


private:
	bool PositionToHeader(BYTE type, ULONG lookMax = 0)
	{
		bool found = false;

		ULONG M = size();
		if(0 < lookMax)
			M = lookMax;

		while(_PositionToHeader(M) && M > 0)
		{
			if(type == HeaderType())
			{
				found = true;

				//_ASSERTE(PESPACKSIZE >= _buffer.ByteRead());

				if(PESPACKSIZE >= _buffer.ByteToRead() && PACK_HEADER == type )
				{
					ULONG toread = PESPACKSIZE - _buffer.ByteToRead();
					//_ASSERTE(toread <= ToReadInFile());
					ReadBytes(toread);
				}

				break;
			}
			
			_buffer.Seek(1);
		}

		return found;
	}

	
	
	
public:
    BYTE HeaderType()
	{
		return _buffer.getAt(3);
	}
	
	Cstring HeaderDefinition()
	{
		BYTE b = HeaderType();
		Cstring tmp;
		        tmp = b;

		switch(b)
		{
			case 0xB9: tmp = _T("Program end (terminates a program stream)");break;
			case 0xBA: tmp = _T("Pack header");break;
			case 0xBB: tmp = _T("System Header");break;
			case 0xBC: tmp = _T("Program Stream Map");break;
			case 0xBD: tmp = _T("Private stream 1");break;
			case 0xBE: tmp = _T("Padding stream");break;
			case 0xBF: tmp = _T("Private stream 2");break;
			
			case 0xC0: 
			case 0xC1: 
			case 0xC2: 
			case 0xC3: 
			case 0xC4: 
			case 0xC5: 
			case 0xC6: 
			case 0xC7: 
			case 0xC8: 
			case 0xC9: 
			case 0xCA: 
			case 0xCB: 
			case 0xCC: 
			case 0xCD: 
			case 0xCE: 
			case 0xCF: 
			case 0xD0: 
			case 0xD1: 
			case 0xD2: 
			case 0xD3: 
			case 0xD4: 
			case 0xD5: 
			case 0xD6: 
			case 0xD7: 
			case 0xD8: 
			case 0xD9: 
			case 0xDA: 
			case 0xDB: 
			case 0xDC: 
			case 0xDD: 
			case 0xDE: 
			case 0xDF: 
			 tmp = _T("MPEG-1 or MPEG-2 audio stream");
			 break;
			
			case 0xE0: 
			case 0xE1: 
			case 0xE2: 
			case 0xE3: 
			case 0xE4: 
			case 0xE5: 
			case 0xE6: 
			case 0xE7: 
			case 0xE8: 
			case 0xE9: 
			case 0xEA: 
			case 0xEB: 
			case 0xEC: 
			case 0xED: 
			case 0xEE: 
			case 0xEF: 
			 tmp = _T("MPEG-1 or MPEG-2 video stream");
			 break;
			
			case 0xF0: tmp = _T("ECM Stream");break;
			case 0xF1: tmp = _T("EMM Stream");break;
			case 0xF2: tmp = _T("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A or ISO/IEC 13818-6_DSMCC_stream");break;
			case 0xF3: tmp = _T("ISO/IEC_13522_stream");break;
			case 0xF4: tmp = _T("ITU-T Rec. H.222.1 type A");break;
			case 0xF5: tmp = _T("ITU-T Rec. H.222.1 type B");break;
			case 0xF6: tmp = _T("ITU-T Rec. H.222.1 type C");break;
			case 0xF7: tmp = _T("ITU-T Rec. H.222.1 type D");break;
			case 0xF8: tmp = _T("ITU-T Rec. H.222.1 type E");break;
			case 0xF9: tmp = _T("ancillary_stream");break;
			
			case 0xFA: 
			case 0xFB: 
			case 0xFC: 
			case 0xFD: 
			case 0xFE: 
			 tmp = _T("reserved");
			 break;

			case 0xFF: tmp = _T("Program Stream Directory");break;
			case 0x00: tmp = _T("Picture");break;
			
			case 0x1: 
			case 0x2: 
			case 0x3: 
			case 0x4: 
			case 0x5: 
			case 0x6: 
			case 0x7: 
			case 0x8: 
			case 0x9: 
			case 0xA: 
			case 0xB: 
			case 0xC: 
			case 0xD: 
			case 0xE: 
			case 0xF: 
			case 0x10: 
			case 0x11: 
			case 0x12: 
			case 0x13: 
			case 0x14: 
			case 0x15: 
			case 0x16: 
			case 0x17: 
			case 0x18: 
			case 0x19: 
			case 0x1A: 
			case 0x1B: 
			case 0x1C: 
			case 0x1D: 
			case 0x1E: 
			case 0x1F: 
			case 0x20: 
			case 0x21: 
			case 0x22: 
			case 0x23: 
			case 0x24: 
			case 0x25: 
			case 0x26: 
			case 0x27: 
			case 0x28: 
			case 0x29: 
			case 0x2A: 
			case 0x2B: 
			case 0x2C: 
			case 0x2D: 
			case 0x2E: 
			case 0x2F: 
			case 0x30: 
			case 0x31: 
			case 0x32: 
			case 0x33: 
			case 0x34: 
			case 0x35: 
			case 0x36: 
			case 0x37: 
			case 0x38: 
			case 0x39: 
			case 0x3A: 
			case 0x3B: 
			case 0x3C: 
			case 0x3D: 
			case 0x3E: 
			case 0x3F: 
			case 0x40: 
			case 0x41: 
			case 0x42: 
			case 0x43: 
			case 0x44: 
			case 0x45: 
			case 0x46: 
			case 0x47: 
			case 0x48: 
			case 0x49: 
			case 0x4A: 
			case 0x4B: 
			case 0x4C: 
			case 0x4D: 
			case 0x4E: 
			case 0x4F: 
			case 0x50: 
			case 0x51: 
			case 0x52: 
			case 0x53: 
			case 0x54: 
			case 0x55: 
			case 0x56: 
			case 0x57: 
			case 0x58: 
			case 0x59: 
			case 0x5A: 
			case 0x5B: 
			case 0x5C: 
			case 0x5D: 
			case 0x5E: 
			case 0x5F: 
			case 0x60: 
			case 0x61: 
			case 0x62: 
			case 0x63: 
			case 0x64: 
			case 0x65: 
			case 0x66: 
			case 0x67: 
			case 0x68: 
			case 0x69: 
			case 0x6A: 
			case 0x6B: 
			case 0x6C: 
			case 0x6D: 
			case 0x6E: 
			case 0x6F: 
			case 0x70: 
			case 0x71: 
			case 0x72: 
			case 0x73: 
			case 0x74: 
			case 0x75: 
			case 0x76: 
			case 0x77: 
			case 0x78: 
			case 0x79: 
			case 0x7A: 
			case 0x7B: 
			case 0x7C: 
			case 0x7D: 
			case 0x7E: 
			case 0x7F: 
			case 0x80: 
			case 0x81: 
			case 0x82: 
			case 0x83: 
			case 0x84: 
			case 0x85: 
			case 0x86: 
			case 0x87: 
			case 0x88: 
			case 0x89: 
			case 0x8A: 
			case 0x8B: 
			case 0x8C: 
			case 0x8D: 
			case 0x8E: 
			case 0x8F: 
			case 0x90: 
			case 0x91: 
			case 0x92: 
			case 0x93: 
			case 0x94: 
			case 0x95: 
			case 0x96: 
			case 0x97: 
			case 0x98: 
			case 0x99: 
			case 0x9A: 
			case 0x9B: 
			case 0x9C: 
			case 0x9D: 
			case 0x9E: 
			case 0x9F: 
			case 0xA0: 
			case 0xA1: 
			case 0xA2: 
			case 0xA3: 
			case 0xA4: 
			case 0xA5: 
			case 0xA6: 
			case 0xA7: 
			case 0xA8: 
			case 0xA9: 
			case 0xAA: 
			case 0xAB: 
			case 0xAC: 
			case 0xAD: 
			case 0xAE: 
			case 0xAF: 
 
				tmp = _T("slice");
				break;

			case 0xB0: tmp = _T("reserved");break;
			case 0xB1: tmp = _T("reserved");break;
			case 0xB2: tmp = _T("user data");break;
			case 0xB3: tmp = _T("Sequence header");break;
			case 0xB4: tmp = _T("sequence error");break;
			case 0xB5: tmp = _T("extension");break;
			case 0xB6: tmp = _T("reserved");break;
			case 0xB7: tmp = _T("sequence end");break;
			case 0xB8: tmp = _T("Group of Pictures");break;


		}

		return tmp;
	}
    

	bool ReversePositionToHeader()
	{
		bool read = false;
		while((0 != ReverseToReadInFile()) && (!(read = _ReversePositionToHeader())) )
		{
			ReverseReadBytes();
		}

		return read;
	}

	bool ReversePositionToHeader(BYTE type)
	{
		bool found = false;

		while(ReversePositionToHeader())
		{
			if(type == _buffer.getAt(-1))
			{
				found = true;
				break;
			}
			
			_buffer.SeekBack(1);
		}

		//Move at the beginning of the header.
		//While reverse looking we are positioned at the end of the header
		if(found)
			_buffer.SeekBack(4);

		return found;
	}


	 	

	bool PositionToGopHeader(ULONG lookMax = 0)
	{
		return PositionToHeader(MPEG2_GOP_HEADER, lookMax);
	}

	bool PositionToPicHeader(ULONG lookMax = 0)
	{
		return PositionToHeader(MPEG2_PIC_HEADER, lookMax);
	}

	bool PositionToSequenceHeader(ULONG lookMax = 0)
	{
		return PositionToHeader(MPEG2_SEQUENCE_HEADER, lookMax);
	}

	bool PositionToPackHeader(ULONG lookMax = 0)
	{
		return PositionToHeader(PACK_HEADER, lookMax);
	}

	bool PositionToSysHeader(ULONG lookMax = 0)
	{
		return PositionToHeader(MPEG2_SYS_HEADER, lookMax);
	}


	//<summary>
	//	Return the compleate size of the pes Header
	//</summary>
	UINT ReadPesHeaderSize()
	{
    
		//_ASSERTE(MPEG2_VIDEO_STREAM == HeaderType());
		//_ASSERTE((0xC0 & _buffer.getAt(6)) == 0x80); //MPEG2
		
		UINT len = 9 + _buffer.getAt(8);
		    

		  return len;
	}
	
	
	
	

	bool PositionOverHeader()
	{		
		if(_IsAtHeader())
		{
			if(_buffer.ByteToRead() >= 4)
				_buffer.Seek(4);
			else
			{
				ReadBytes();
				bool ret = PositionOverHeader();
				_ASSERTE(true == ret);

				return ret;
			}

			return true;
		}
		else
		{
			if(!_IsAtAudioHeader())
				return false;

			if(_buffer.ByteToRead() >= 2)
				_buffer.Seek(2);
			else
			{
				ReadBytes();
				bool ret = PositionOverHeader();
				_ASSERTE(true == ret);

				return ret;
			}
		}

		return true;
	}

	BYTE * GetBuffer()
	{
		return _buffer.GetCurrentPosition();
	}

	uint64_t CurrentPosition()
	{
		if(0 == _read)
			return 0;

		_ASSERTE(_read <= size());
		//_ASSERTE((_read + _buffer.ByteRead()) <= size());
		_ASSERTE(_read + _buffer.ByteRead() >=  _buffer.getSize());
		return _read + _buffer.ByteRead() - _buffer.getSize();
	}

	uint64_t AvailableBuffer()
	{
		return _buffer.ByteToRead();
	}

	
	
	void MoveToBeginning()
	{
		_read = 0;
		_buffer.Reset();
	    ReadBytes();
	}

	uint64_t GetPositionForTime(uint64_t duration, uint64_t time)
	{
		uint64_t packsize = PESPACKSIZE;
		uint64_t size = this->size();
		
		double r = (double)size*(double)time;
		       r = r/(duration);

		return (r/packsize)*packsize;
	}

	void MoveToPosition(uint64_t position)
	{
		//_ASSERTE(position >= _read);

		_read = position;
		_buffer.Reset();
		
	    ReadBytes();
	}

	void MovePositionBack(uint64_t bytes)
	{
		if(bytes <= _buffer.ByteRead())
			_buffer.SeekBack(bytes);
		else
		{
			_ASSERTE(CurrentPosition() >= bytes);
			MoveToPosition(CurrentPosition() - bytes);
		}
	}
	void MovePositionForward(uint64_t bytes)
	{
		if(bytes <= _buffer.ByteToRead())
			_buffer.Seek(bytes);
		else
		{
			_ASSERTE(CurrentPosition() + bytes <= size());
			MoveToPosition(CurrentPosition() + bytes);
		}
	}
	template <typename T>
	void ReadStruct(CResource<T> &s)
	{
		s = new T;
		ReadStruct(*s);
	}

	template <typename T>
	void ReadStruct(T &s)
	{
		TBitstream<CBufferRead*> tb(&_buffer);
		s.get(tb);

		if(int d = tb.getUnreadBufferSize())
		{
			_buffer.SeekBack(d);
		}
	}

	bool MoveBack()
	{
		if(!ReversePositionToHeader(PACK_HEADER))
			return false;

		return MoveNext();//We are at pack_header read it.
	}

	bool MoveNext(bool BackWard = false)
	{
		if(!this->PositionToPackHeader())
			return false;

		_ASSERTE(_IsAtHeader());
		_ASSERTE(PACK_HEADER == this->HeaderType());

		try{
				ReadStruct(_packHeader);
				_ASSERTE(_IsAtHeader());

				if(MPEG2_SYS_HEADER == this->HeaderType())
				{
					ReadStruct(_systemHeader);
					_ASSERTE(_IsAtHeader());
				}

#if _DEBUG
		uint64_t xpos = CurrentPosition();
		xpos += ReadPesHeaderSize();
#endif
		        
				_pesHeader.PTS_flags = 0;
				ReadStruct(_pesHeader);

		//#if _DEBUG
		//		_ASSERTE(xpos == CurrentPosition());
		//		         xpos =  CurrentPosition();
		//#endif
		}catch(Cexception &ex)//Should have an invalid pack specific exception
		{
			_ASSERTE(FALSE);
#if _DEBUG
			uint64_t xpos = CurrentPosition();
			Cstring  err(_T("FOUNDATION: INVALID FILE STRUCT AT "));
			         err += xpos;
			 
			//ALXTHROW_T(err);
#endif
			//throw;

		    if(!BackWard)
				return MoveNext();
			else
				return false;
		}

		return true;

	}

	SequenceHeader * GetSequence()
	{
		if(_sequence)
			return _sequence;

		uint64_t p = _buffer.GetPosition();

		if((_pesHeader.stream_id >= MPEG2_START_VIDEO_STREAM && _pesHeader.stream_id <= MPEG2_END_VIDEO_STREAM) &&
				!_sequence)
		{
			if(this->PositionToSequenceHeader(_pesHeader.getBodySize()))
			{
				ReadStruct(_sequence);
			}
		}
		else
			return NULL;


		_buffer.SetPosition(p);

		return _sequence;
	}

	const PesData * GetPesHeader()
	{
		if(0 == _pesHeader.stream_id)
			return NULL;

		return &_pesHeader;
	}
	 

	const SystemHeader * GetSystemHeader()	
	{
		if(0 == _systemHeader.id)
			return NULL;

		return &_systemHeader;
	}

	const PackHeader * GetPackHeader()
	{
		if(0 == _packHeader.id)
			return NULL;

		return &_packHeader;
	}

	bool Analize()
	{
		while(MoveNext())
		{
			if((_pesHeader.stream_id >= MPEG2_START_VIDEO_STREAM && _pesHeader.stream_id <= MPEG2_END_VIDEO_STREAM) &&
				!_sequence)
			{
				SequenceHeader * s = GetSequence();
				if(NULL != s)
				{
					_video_stream_id = _pesHeader.stream_id;
				}
			}

			if((_pesHeader.stream_id >= MPEG2_START_AUDIO_STREAM && _pesHeader.stream_id <= MPEG2_END_AUDIO_STREAM) &&
				! _audioHeader)
			{
				AudioHeader * a = GetAudioHeader();
				if(NULL != a)
				{
					_audio_stream_id = _pesHeader.stream_id;
				}
			}

			if(_audioHeader && _sequence)
			{
				_ASSERTE(224 == _video_stream_id);
				_ASSERTE(192 == _audio_stream_id);

				return true;
			}
		}

		return false;
	}

	AudioHeader * GetAudioHeader()
	{
		if(_audioHeader)
			return _audioHeader;


		uint64_t p = _buffer.GetPosition();

		if((_pesHeader.stream_id >= MPEG2_START_AUDIO_STREAM && _pesHeader.stream_id <= MPEG2_END_AUDIO_STREAM) &&
				! _audioHeader)
			{
				ULONG max = _pesHeader.getBodySize();
				if(this->PositionToAudioHeader(max))
				{
					ReadStruct(_audioHeader);
				}
			}
		else
			return NULL;


		_buffer.SetPosition(p);

		return _audioHeader;

	}
    uint64_t GetDurationEx()
	{
		this->MoveToBeginning();
		
		MoveNext();
		
		while(!_pesHeader.PTS_flags || 
			  !(_pesHeader.stream_id >= MPEG2_START_VIDEO_STREAM && _pesHeader.stream_id <= MPEG2_END_VIDEO_STREAM) 
			)
		{
			
			if((_pesHeader.stream_id >= MPEG2_START_VIDEO_STREAM && _pesHeader.stream_id <= MPEG2_END_VIDEO_STREAM) &&
				!_sequence)
			{
				SequenceHeader * s = GetSequence();
				_ASSERTE(NULL != s);
			}

			if((_pesHeader.stream_id >= MPEG2_START_AUDIO_STREAM && _pesHeader.stream_id <= MPEG2_END_AUDIO_STREAM) &&
				! _audioHeader)
			{
				AudioHeader * a = GetAudioHeader();
				_ASSERTE(NULL != a);
			}


			if(!_pesHeader.PTS_flags)
			{
				MoveNext();
			}
		
		}

		_ASSERTE(_pesHeader.PTS_flags);

		UINT id = _pesHeader.stream_id;

		uint64_t start = MpegTime::Time(
			_pesHeader.PTS_32_30,
			_pesHeader.PTS_29_15,
			_pesHeader.PTS_14_0);

		MoveToEnd();   //GO TO THE END OF THE FILE

		double videoessize = 0;

		while(ReversePositionToHeader(PACK_HEADER))
		{
		   MoveNext();
   

		   if(id == _pesHeader.stream_id && _pesHeader.PTS_flags)
		   {
			   break;
		   }

		   

		   if(id == _pesHeader.stream_id)
		   {
				videoessize += _pesHeader.getBodySize();
		   }

		   ReversePositionToHeader(PACK_HEADER);//GO BACK TO THE HEADER AT THE BEGINNING OF THE WHILE
		}

		_ASSERTE(_pesHeader.PTS_flags);

		uint64_t end = MpegTime::Time(
			_pesHeader.PTS_32_30,
			_pesHeader.PTS_29_15,
			_pesHeader.PTS_14_0);

		if(_sequence)
		{
			double sbr = _sequence->getByteRate();
			double br = videoessize / sbr;
			uint64_t q = (10000 * 1000)*br;

			end += q;
		}

		return end - start;
	}

	uint64_t GetDuration()
	{
		if(0 == _duration)
		{
			uint64_t r = 0;
	
			if(0 < _read)
			{
				_ASSERTE(_read >= _buffer.ByteToRead());
				r = _read - _buffer.ByteToRead();
			}

			MoveToEnd();   //GO TO THE END OF THE FILE
			ReversePositionToHeader(PACK_HEADER);

			if((size() - CurrentPosition()) < PESPACKSIZE)//FILE NOT WELL ENDED
			{
				ReversePositionToHeader(PACK_HEADER);
			}


			while(!MoveNext(true))//Read Header
			{
				ReversePositionToHeader(PACK_HEADER);//the one checked
				ReversePositionToHeader(PACK_HEADER);//go to the previus one
			}

			_duration = GetPackHeader()->GetTime();

			if(0 < r)
				MoveToPosition(r);
		}

		return _duration;
	}

	bool MoveToTime(uint64_t time)
	{
		if(0 == _duration)
		{
			GetDuration();
		}

		MoveToPosition(GetPositionForTime(_duration, time));

		if(!MoveNext())
		{
			//ALXTHROW("MPEG EOF");
			return false;
		}

		uint64_t t = this->GetPackHeader()->GetTime();
		//_ASSERTE(t <= time);
		
		//while(t < time)
		//{
		//	if(!MoveNext())
		//	{
		//		ALXTHROW("MPEG EOF");
		//	}

		//	t = this->GetPackHeader()->GetTime();
		//}

	}
	
	CMPEG2FILE_PROGRAMSTREAM(Cstring filename, bool PreLoad = false, size_t buffer_size = 4096):
	  SHStream(filename),
	  _buffer(buffer_size),
	  _read(0),
	  _duration(0),
	  _sequence(ALXEMPTY),
	  _audioHeader(ALXEMPTY)
	{
		if(PreLoad)
			MoveToBeginning();

		_systemHeader.id = 0;
		_systemHeader.header_length = 0;

		_packHeader.id = 0;
		_pesHeader.stream_id = 0;
	}


	CBufferRead * GetBufferPointer(){return &_buffer;}

	uint64_t AudioId(){return _audio_stream_id;}
	uint64_t VideoId(){return _video_stream_id;}

};
*/
__ALX_END_NAMESPACE
