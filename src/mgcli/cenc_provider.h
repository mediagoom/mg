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

inline Cstring kid_to_string(unsigned char * pKid)
{
	Cstring kid;
	kid.append_hex_buffer(pKid, 4);
	kid.append(_T("-"));

	kid.append_hex_buffer(pKid + 4, 2);
	kid.append(_T("-"));

	kid.append_hex_buffer(pKid + 6, 2);
	kid.append(_T("-"));

	kid.append_hex_buffer(pKid + 8, 2);
	kid.append(_T("-"));

	kid.append_hex_buffer(pKid + 10, 6);

	return kid;
}

class ICencProvider
{
public:

	virtual void add_pssh(std::vector<ProtectionSystemSpecificHeaderBox *> &  vpssh) = 0;
	virtual void add_ContentProtection(std::vector<Cstring> & vContentProtection) = 0;
};

class PlayReadyCencProvider: public ICencProvider
{

	CstringT<wchar_t> _wmheader;

	Cstring _url;

	unsigned char _key[BLOCK_LEN];
	unsigned char _kid[BLOCK_LEN];
	unsigned char _bekid[BLOCK_LEN];

	unsigned char _system_id[BLOCK_LEN];

	//CBuffer<unsigned char> _body;

	WriteMemoryBitstream _body;

	const void binary_encode_kid(unsigned char * pKid, unsigned char * pbEnc )
	{
			 unsigned char * retVal = pbEnc;
			 unsigned char * guid   = pKid;

		     int pos = 0; 
             
             // 3..0 -> 0..3
             for (int j = 4; j > 0; j--) 
                 retVal[pos++] = guid[j-1]; 
             // 5..4 -> 4..5
             for (int j = 6; j > 4; j--) 
                 retVal[pos++] = guid[j-1]; 
             // 7 .. 6 -> 6..7
             for (int j = 8; j > 6; j--) 
                 retVal[pos++] = guid[j-1];
             // 8 .. 15 -> 8 .. 15
             for (int j = 8; j < 16; j++) 
                 retVal[pos++] = guid[j]; 
	}

	

	
	CstringT<wchar_t> Checksum(unsigned char * pKey, unsigned char * pKid)
	{
#ifdef HAVE_LIBGYPAES

		aes_encrypt_ctx ctx[1];

		::memset(ctx, 0, sizeof(aes_encrypt_ctx));

		AES_RETURN ret = aes_encrypt_key(pKey, BLOCK_LEN, ctx);
			_ASSERTE(0 == ret);

		unsigned char enc[BLOCK_LEN];

		ret = aes_ecb_encrypt(pKid, enc, BLOCK_LEN, ctx);
			_ASSERTE(0 == ret);

		return Base64(enc, 8);
#else
		return CstringT<wchar_t>();
#endif

		/*int size = BLOCK_LEN * 2 + 4;

		Cstring str64;
		        

		BOOL check64 = Base64Encode(
			   enc
			   , BLOCK_LEN
			   , str64.GetBuffer(size)
			   , &size);

		if(!check64)
		{
			check64 = Base64Encode(
			     enc
			   , BLOCK_LEN
			   , str64.GetBuffer(size)
			   , &size);

			if(!check64)
				ALXTHROW_T(_T("CANNOT BASE64 ENCODE"));
		}

		str64.CommitBuffer();


		return str64;*/

	}
	

public:

	static CstringT<wchar_t> Base64(const unsigned char * pbuffer, uint32_t buffer_size, uint32_t flags = ATL_BASE64_FLAG_NOCRLF)
	{
		CstringT<char> str64;
		
		int size = buffer_size * 2 + 4;

#ifdef BASE64

		base64_encodestate state_in;

		base64_init_encodestate(&state_in);

		int written = base64_encode_block(reinterpret_cast<const char*>(pbuffer)
			, buffer_size
			, str64.GetBuffer(size, false)
			, &state_in);

		str64.CommitBuffer(written);

		written = base64_encode_blockend(str64.GetBuffer(4, false), &state_in);

		str64.CommitBuffer(written);
#else

#ifdef _WIN32
		
		BOOL check64 = Base64Encode(
			     pbuffer
			   , buffer_size
			   , str64.GetBuffer(size, false)
			   , &size
			   , flags
			   );

		if(!check64)
		{
			check64 = Base64Encode(
			     pbuffer
			   , buffer_size
			   , str64.GetBuffer(size, false)
			   , &size
			   , flags);

			if(!check64)
				ALXTHROW_T(_T("CANNOT BASE64 ENCODE"));
		}

		str64.CommitBuffer(size);
#else

#error NO BASE 64 ENCODING

#endif

#endif

		CstringT<wchar_t> W64;
				W64 += static_cast<const char*>(str64);

		return W64;
	}


	PlayReadyCencProvider(unsigned char * pKey
		, unsigned char * pKid
		, const TCHAR * pszurl = NULL):_body(1024)
	{

		unsigned char system_id[] = {0x9A, 0x04, 0xF0, 0x79, 0x98, 0x40, 0x42, 0x86, 0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95};
        
		::memcpy(_system_id, system_id, BLOCK_LEN);
		::memcpy(_key, pKey, BLOCK_LEN);
		::memcpy(_kid, pKid, BLOCK_LEN);

		binary_encode_kid(_kid, _bekid);
		
		if(NULL != pszurl)
			_url = pszurl;
		else
			_url = _T("http://playready.directtaps.net/pr/svc/rightsmanager.asmx?PlayRight=1&amp;UseSimpleNonPersistentLicense=1");

		
		_wmheader =  _T("<WRMHEADER xmlns=\"http://schemas.microsoft.com/DRM/2007/03/PlayReadyHeader\" version=\"4.0.0.0\">");
		_wmheader += _T("<DATA><PROTECTINFO><KEYLEN>16</KEYLEN><ALGID>AESCTR</ALGID></PROTECTINFO><KID>");
		
		//_wmheader += _T("AAAAEAAQABAQABAAAAAAAQ==");
		_wmheader += Base64(_bekid, BLOCK_LEN);
		
		_wmheader += _T("</KID><CHECKSUM>");
		
		//_wmheader += _T("5TzIYQ2hrOY=");
		_wmheader += Checksum(pKey, _bekid);

		_wmheader += _T("</CHECKSUM><LA_URL>");
		_wmheader += _url;
		_wmheader += _T("</LA_URL><CUSTOMATTRIBUTES><IIS_DRM_VERSION>7.1.1565.4</IIS_DRM_VERSION></CUSTOMATTRIBUTES></DATA></WRMHEADER>");
				
		uint32_t  size = 10 + (sizeof(wchar_t) *_wmheader.size());

		/*
		_body.prepare(size);

		WriteMemoryBitstream bit_stream(_body, size);// , DEFAULT_TBS_BUF_LEN, false);
		
		bit_stream.little_putbits(size, 32);
		bit_stream.little_putbits(1, 16);

		bit_stream.little_putbits(1, 16);
		bit_stream.little_putbits((sizeof(wchar_t) * _wmheader.size()), 16);

		const wchar_t* pchar = _wmheader;

		for(uint32_t i = 0; i < _wmheader.size(); i++)
		{
			bit_stream.little_putbits(pchar[i], 16);
		}

		_body.updatePosition(size);
		*/

		//_body.write(reinterpret_cast<const unsigned char*>(_wmheader.operator wchar_t*()), size);

		_body.little_putbits(size, 32);
		_body.little_putbits(1, 16);

		_body.little_putbits(1, 16);
		_body.little_putbits((sizeof(wchar_t) * _wmheader.size()), 16);

		const wchar_t* pchar = _wmheader;

		for (uint32_t i = 0; i < _wmheader.size(); i++)
		{
			_body.little_putbits(pchar[i], 16);
		}

		_body.flush();

		_ASSERTE(_body.get_size() == size);

	}

	virtual void add_pssh(std::vector<ProtectionSystemSpecificHeaderBox *> &  vpssh)
	{
		
		//unsigned char(8) SystemID

		ProtectionSystemSpecificHeaderBox * ppssh = new ProtectionSystemSpecificHeaderBox;
		 
		                                    ppssh->set_version(0);
											ppssh->KID_count = 0;
											
											//::memcpy(ppssh->KID, _kid, BLOCK_LEN);

											::memcpy(&ppssh->SystemID, _system_id, BLOCK_LEN);

											ppssh->_p_data = new unsigned char[static_cast<uint32_t>(_body.get_size())];
											ppssh->DataSize = static_cast<uint32_t>(_body.get_size());

											::memcpy(ppssh->_p_data, _body.get_buffer(), ppssh->DataSize);

		vpssh.push_back(ppssh);
	}

	virtual void add_ContentProtection(std::vector<Cstring> & vContentProtection)
	{
		//CLSIDFromString

		Cstring kid = kid_to_string(_kid);
			
		Cstring cp  = _T("<ContentProtection schemeIdUri=\"urn:mpeg:dash:mp4protection:2011\" value=\"cenc\" cenc:default_KID=\"");
		               
				//cp += _T("10000000-1000-1000-1000-100000000001");
			    
				cp += kid;
				cp += _T("\"/>");
				cp += _T("<ContentProtection schemeIdUri=\"urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95\" value=\"2.0\" cenc:default_KID=\"");
				//cp += _T("10000000-1000-1000-1000-100000000001");
				cp += kid;

				cp += _T("\">");
				cp += _T("<mspr:pro>");
				//cp += _T("ngMAAAEAAQCUAzwAVwBSAE0ASABFAEEARABFAFIAIAB4AG0AbABuAHMAPQAiAGgAdAB0AHAAOgAvAC8AcwBjAGgAZQBtAGEAcwAuAG0AaQBjAHIAbwBzAG8AZgB0AC4AYwBvAG0ALwBEAFIATQAvADIAMAAwADcALwAwADMALwBQAGwAYQB5AFIAZQBhAGQAeQBIAGUAYQBkAGUAcgAiACAAdgBlAHIAcwBpAG8AbgA9ACIANAAuADAALgAwAC4AMAAiAD4APABEAEEAVABBAD4APABQAFIATwBUAEUAQwBUAEkATgBGAE8APgA8AEsARQBZAEwARQBOAD4AMQA2ADwALwBLAEUAWQBMAEUATgA+ADwAQQBMAEcASQBEAD4AQQBFAFMAQwBUAFIAPAAvAEEATABHAEkARAA+ADwALwBQAFIATwBUAEUAQwBUAEkATgBGAE8APgA8AEsASQBEAD4AQQBBAEEAQQBFAEEAQQBRAEEAQgBBAFEAQQBCAEEAQQBBAEEAQQBBAEEAUQA9AD0APAAvAEsASQBEAD4APABDAEgARQBDAEsAUwBVAE0APgA1AFQAegBJAFkAUQAyAGgAcgBPAFkAPQA8AC8AQwBIAEUAQwBLAFMAVQBNAD4APABMAEEAXwBVAFIATAA+AGgAdAB0AHAAOgAvAC8AcABsAGEAeQByAGUAYQBkAHkALgBkAGkAcgBlAGMAdAB0AGEAcABzAC4AbgBlAHQALwBwAHIALwBzAHYAYwAvAHIAaQBnAGgAdABzAG0AYQBuAGEAZwBlAHIALgBhAHMAbQB4AD8AUABsAGEAeQBSAGkAZwBoAHQAPQAxACYAYQBtAHAAOwBVAHMAZQBTAGkAbQBwAGwAZQBOAG8AbgBQAGUAcgBzAGkAcwB0AGUAbgB0AEwAaQBjAGUAbgBzAGUAPQAxADwALwBMAEEAXwBVAFIATAA+ADwAQwBVAFMAVABPAE0AQQBUAFQAUgBJAEIAVQBUAEUAUwA+ADwASQBJAFMAXwBEAFIATQBfAFYARQBSAFMASQBPAE4APgA3AC4AMQAuADEANQA2ADUALgA0ADwALwBJAEkAUwBfAEQAUgBNAF8AVgBFAFIAUwBJAE8ATgA+ADwALwBDAFUAUwBUAE8ATQBBAFQAVABSAEkAQgBVAFQARQBTAD4APAAvAEQAQQBUAEEAPgA8AC8AVwBSAE0ASABFAEEARABFAFIAPgA=");
				
				cp += Base64(_body.get_buffer(), static_cast<uint32_t>(_body.get_size()));
				
				cp += _T("</mspr:pro>");
				cp += _T("</ContentProtection>");

		vContentProtection.push_back(cp);
	}
};

class ClearKeyCencProvider: public ICencProvider
{
	unsigned char _kid[BLOCK_LEN];
	//WriteMemoryBitstream _bit_stream;
	CMP4WriteMemory _bit_stream;

	void fill_pssh(ProtectionSystemSpecificHeaderBox * ppssh)
	{
		unsigned char system_id[] = {
									  0x10
									, 0x77
									, 0xEF
									, 0xEC
									, 0xC0
									, 0xB2
									, 0x4D
									, 0x02
									, 0xAC
									, 0xE3
									, 0x3C
									, 0x1E
									, 0x52
									, 0xE2
									, 0xFB
									, 0x4B
		};
        
		 
		                                    ppssh->set_version(1);
											ppssh->KID_count = 1;
											
											::memcpy(&ppssh->KID, _kid, BLOCK_LEN);
											::memcpy(&ppssh->SystemID, system_id, BLOCK_LEN);
											
											ppssh->_p_data  = 0;
											ppssh->DataSize = 0;
	}
public:

	ClearKeyCencProvider(unsigned char * pKid)//:_bit_stream(1024)
	{		
		::memcpy(_kid, pKid, BLOCK_LEN);

		ProtectionSystemSpecificHeaderBox  pssh;

		fill_pssh(&pssh);
		
		CMP4WriteMemory & m = _bit_stream;
					    //m.Open(&_bit_stream);
						m.open(1024);

		m.open_box(box_pssh);

		m.write_box(pssh);

		m.close_box(box_pssh);
				
		//_bit_stream.flush();

		m.flush();

	}

	virtual void add_pssh(std::vector<ProtectionSystemSpecificHeaderBox *> &  vpssh)
	{
		
		
		//1077EFECC0B24D02ACE33C1E52E2FB4B

		ProtectionSystemSpecificHeaderBox * ppssh = new ProtectionSystemSpecificHeaderBox;
		 /*
		                                    ppssh->set_version(1);
											ppssh->KID_count = 1;
											
											::memcpy(&ppssh->KID, _kid, BLOCK_LEN);
											::memcpy(&ppssh->SystemID, system_id, BLOCK_LEN);
											
											ppssh->_p_data  = 0;
											ppssh->DataSize = 0;

											//::memcpy(ppssh->_p_data, _body.get(), ppssh->DataSize);
		*/

		fill_pssh(ppssh);

		vpssh.push_back(ppssh);
	}


	//1077EFECC0B24D02ACE33C1E52E2FB4B
	virtual void add_ContentProtection(std::vector<Cstring> & vContentProtection)
	{
		   Cstring cp  = _T("<ContentProtection schemeIdUri=\"urn:uuid:1077efec-c0b2-4d02-ace3-3c1e52e2fb4b\">");
				   cp += _T("<cenc:pssh>");
					   //AAAANHBzc2gBAAAAEHfv7MCyTQKs4zweUuL7SwAAAAEfcltX3dBXeY80EpA43ZS1AAAAAA==
					
					   cp+= PlayReadyCencProvider::Base64(_bit_stream.get_buffer(), 
						   static_cast<uint32_t>(_bit_stream.get_size()));
						   
				   cp += "</cenc:pssh>";
				   cp += _T("</ContentProtection>");

		   vContentProtection.push_back(cp);
	}

	Cstring json_key(unsigned char * pKey)
	{
			Cstring json = _T("{ \"keys\":[{\"kid\" : \"");
					json += PlayReadyCencProvider::Base64(_kid, BLOCK_LEN, ATL_BASE64_FLAG_NOPAD);
					json += _T("\" , \"k\" : \"");
					json += PlayReadyCencProvider::Base64(pKey, BLOCK_LEN, ATL_BASE64_FLAG_NOPAD);
                    json += _T("\"}]}");

			return json;
	}
};

class WidevineBoyProvider : public ICencProvider
{

	unsigned char _key[BLOCK_LEN];
	unsigned char _kid[BLOCK_LEN];
	
	unsigned char _system_id[BLOCK_LEN];

	//CBuffer<unsigned char> _body;

	WriteMemoryBitstream _body;

public:
	
	WidevineBoyProvider(unsigned char * pKey
		, unsigned char * pKid
		, const char * body64
		, uint32_t body_size) :_body(body_size)
	{

		unsigned char system_id[] = { 
			  0xED
			, 0xEF
			, 0x8B
			, 0xA9
			, 0x79
			, 0xD6
			, 0x4A
			, 0xCE
			, 0xA3
			, 0xC8
			, 0x27
			, 0xDC
			, 0xD5
			, 0x1D
			, 0x21
			, 0xED
		};

		::memcpy(_system_id, system_id, BLOCK_LEN);
		::memcpy(_key, pKey, BLOCK_LEN);
		::memcpy(_kid, pKid, BLOCK_LEN);

		CBuffer<char>  strClear(body_size);

		base64_decodestate state_in;

		base64_init_decodestate(&state_in);

		int written = base64_decode_block(
			  body64
			, body_size
			, strClear.get()
			, &state_in);

		strClear.updatePosition(written);
		
		_body.write(reinterpret_cast<unsigned char*>(strClear.get()), ST_U32(strClear.size()));
		

	}

	void fill(ProtectionSystemSpecificHeaderBox & pssh)
	{
		pssh.set_version(0);
		pssh.KID_count = 0;

		
		::memcpy(&pssh.SystemID, _system_id, BLOCK_LEN);

		pssh._p_data = new unsigned char[static_cast<uint32_t>(_body.get_size())];
		pssh.DataSize = static_cast<uint32_t>(_body.get_size());

		::memcpy(pssh._p_data, _body.get_buffer(), pssh.DataSize);
	}

	virtual void add_pssh(std::vector<ProtectionSystemSpecificHeaderBox *> &  vpssh)
	{
		
		ProtectionSystemSpecificHeaderBox * ppssh = new ProtectionSystemSpecificHeaderBox;

		fill(*ppssh);

		vpssh.push_back(ppssh);
	}

	virtual void add_ContentProtection(std::vector<Cstring> & vContentProtection)
	{
		

		Cstring kid = kid_to_string(_kid);

		Cstring cp = _T("<ContentProtection value=\"Widevine\" schemeIdUri=\"urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed\" >");

		ProtectionSystemSpecificHeaderBox pssh;
		fill(pssh);

		/*

		CMP4WriteMemory & m = _bit_stream;
		//m.Open(&_bit_stream);
		m.open(1024);

		m.open_box(box_pssh);

		m.write_box(pssh);

		m.close_box(box_pssh);

		//_bit_stream.flush();

		m.flush();
		*/

		//WriteMemoryBitstream stream(1024);

		CMP4WriteMemory m;
		m.open(1024);
		m.open_box(box_pssh);
		m.write_box(pssh);
		m.close_box(box_pssh);

		//pssh.put(stream);

		//stream.flush();

		m.flush();
		
		cp += _T("<cenc:pssh>");
		
		cp += PlayReadyCencProvider::Base64(m.get_buffer(), U64_U32(m.get_size()));

		cp += _T("</cenc:pssh>");
		cp += _T("</ContentProtection>");

		vContentProtection.push_back(cp);
	}
};

