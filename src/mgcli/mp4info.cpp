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
#include "stdafx.h"

#include "auto_test.h"
#include "fragment_validation.h"
#ifdef CENC
#include "cenc_provider.h"
#endif

#include <string>

#include "version.h"

using namespace MGUV;
using namespace MGCORE;



unsigned int _fragmented_default_duration;

class MP4ReaderConsole: public MP4Reader
{
protected:

	virtual void unknown_box(CMP4 &mp4)
	{
		//_ASSERTE(false);

		std::wcout << _T("unknown-box\t") 
			<< mp4.get_box().get_type_string()
			<< _T("\t")
			<< mp4.get_box_position()
			<< _T("\t")
			<< mp4.get_box_position_end()
			<< std::endl;

		mp4.skip_current();
	}
};


class MOOFReaderConsole: public MOOFReader
{
protected:

	virtual void unknown_box(CMP4 &mp4)
	{
		//_ASSERTE(false);

		std::wcout << _T("unknown-box\t") 
			<< mp4.get_box().get_type_string()
			<< _T("\t")
			<< mp4.get_box_position()
			<< _T("\t")
			<< mp4.get_box_position_end()
			<< std::endl;

		mp4.skip_current();
	}
};


enum BOXTYPE {
	  box_container
	, box_extended
	, box_simple
	, box_command
} ;



struct box_handle
{
	STDTSTRING boxname;
	int (*mp4command)(CMP4 &mp4, bool extended);
	BOXTYPE boxtype;
};


std::map<STDTSTRING , box_handle> g_box_map;

#define BOX_FUNCTION(T) int handle##T(CMP4 &mp4, bool extended)

BOX_FUNCTION(container)
{
	return 0;
}

#define BOX_TYPE_CONTAINER(T) {_T( #T ), &handlecontainer, box_container},
#define BOX_TYPE(T, B) {_T( #T ), &handle##T, B},
#define BEGIN_BOX_TYPE box_handle g_box_all_handler[] = {
#define END_BOX_TYPE };int g_box_size = sizeof(g_box_all_handler) / sizeof(g_box_all_handler[0]);


int dobox(CMP4 &mp4, const TCHAR * target = _T("xxxx"), bool recursive = true, bool extended = false);
void output_sequence(const BYTE* sequence_bytes, int sequence_size, int idx = 0)
{

	H264Sequence sequence;

											H264Nal nal;
											        nal.decode_nal(sequence_size
														         , sequence_bytes);

												_ASSERTE(NALTYPE::SEQUENCE == nal.get_decoded_nal_unit_type());
												//_ASSERTE(avcC.get_sequence_size(idx) == nal.decoded_rbsp_size());

												FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
													              , nal.decoded_rbsp_size() -1
													);

											sequence.get(mem);

											_ASSERTE(mem.get_position() == (nal.decoded_rbsp_size() - 1));




	std::wcout 
				<< _T(" [H264 SEQUENCE: ")
				<< idx
				
				<< " Profile: "
				<< sequence.profile_idc
												
				<< " Level: "
				<< (sequence.level_idc / 10.0)

				<< " constraint_set0_flag: "
				<< sequence.constraint_set0_flag

				<< " constraint_set1_flag: "
				<< sequence.constraint_set1_flag

				<< " constraint_set2_flag: "
				<< sequence.constraint_set2_flag

				<< " seq_parameter_set_id: "
				<< sequence.seq_parameter_set_id
											    
				<< " num_ref_frames: "
				<< sequence.num_ref_frames

				<< " MaxFrameNum: "
				<< pow(2.0, (int)(sequence.log2_max_frame_num_minus4 + 4))

	            << " pic_order_cnt_type: "
				<< sequence.pic_order_cnt_type

				<< " max_pic_order_cnt: "
				<< pow(2.0, (int)(sequence.log2_max_pic_order_cnt_lsb_minus4 + 4))
				
				;//std:wout


				if(1 == sequence.pic_order_cnt_type)
				{
					std::wcout
					<< " delta_pic_order_always_zero_flag: "
					<< sequence.delta_pic_order_always_zero_flag
					<< " offset_for_non_ref_pic: "
                    << sequence.offset_for_non_ref_pic
					<< " offset_for_top_to_bottom_field: "
                    << sequence.offset_for_top_to_bottom_field
					<< " num_ref_frames_in_pic_order_cnt_cycle: "
                    << sequence.num_ref_frames_in_pic_order_cnt_cycle

					;//std:wout
            
				}

				std::wcout
				<< " frame_mbs_only_flag: "
                << sequence.frame_mbs_only_flag

				<< " mb_adaptive_frame_field_flag: "
                << sequence.mb_adaptive_frame_field_flag
				;				//std:wout

												if(sequence.vui_parameters_present_flag)
												{
													std::wcout
													<< "VUI> "
													<< "pic_struct_present_flag: "
													<< sequence.VUI->pic_struct_present_flag
													;

													if(sequence.VUI->aspect_ratio_info_present_flag)
													{
														if(255 == sequence.VUI->aspect_ratio_idc)
														{
															std::wcout 
															<< " Ratio: "
															<< (16*sequence.VUI->sar_width)
														    << "x"
															<< (16*sequence.VUI->sar_height) 
															;
														}
														else
														{
															std::wcout 
															<< " Ratio IDC: "
															<< sequence.VUI->aspect_ratio_idc;
														}

														///has_b_frames = num_reorder_frames;
														std::wcout
														<< " num_reorder_frames: "
												        << sequence.VUI->num_reorder_frames;
													}
												}

												    std::wcout 
													<< " width: "
												    << (16 * (sequence.pic_width_in_mbs_minus1 + 1))
													<< " height: "
													<< (16 * (sequence.pic_height_in_map_units_minus1 + 1))
												    << _T("]");
}

void outputAVCDecoderConfigurationRecord(CMP4 &mp4)
{
			
				AVCDecoderConfigurationRecord avcC;
				mp4.parse_avc_decoder_configuration_record(avcC);
							  
							  std::wcout 
								  << _T(" AVCProfileIndication: ")
									<< avcC.AVCProfileIndication
									<< _T(" profile_compatibility: ")
									<< avcC.profile_compatibility
									<< _T(" AVCLevelIndication: ")
									<< avcC.AVCLevelIndication
									<< _T(" lengthSizeMinusOne: ")
									<< avcC.lengthSizeMinusOne
									<< _T(" numOfSequenceParameterSets: ")
									<< avcC.numOfSequenceParameterSets
									<< _T(" numOfPictureParameterSets: ")
									<< avcC.numOfPictureParameterSets;
									

									if(avcC.get_sequence_count())
									{
										for(uint32_t idx = 0; idx < avcC.get_sequence_count(); idx++)
										{
											output_sequence(avcC.get_sequence(idx), avcC.get_sequence_size(idx), idx);
										}
									}

									if(avcC.get_picture_count())
									{
										for(uint32_t idx = 0; idx < avcC.get_picture_count(); idx++)
										{
											

											H264Nal nal;
											        nal.decode_nal(avcC.get_picture_size(idx)
														         , avcC.get_picture(idx));

												_ASSERTE(NALTYPE::PICTURE == nal.get_decoded_nal_unit_type());
												//_ASSERTE(avcC.get_picture_size(idx) == nal.decoded_rbsp_size());

												uint32_t nal_size = nal.decoded_rbsp_size();

												FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
													              , nal.decoded_rbsp_size() -1
													);

											pic_parameter_set_rbsp picture(nal.decoded_rbsp_size() - 1);
											//pic_parameter_set_rbsp picture;
											
											picture.get(mem);

											
											//picture.skip

											_ASSERTE(mem.get_position() == nal_size -1);

											if(mem.get_position() > ( nal_size -1 ) )
												mem.skipbits( static_cast<uint16_t>( (mem.get_position() - ( nal_size -1 )  ) * 8 ) );

											std::wcout 
												<< _T(" [H264 picture: ")
												<< idx
												<< " pic id: "
												<< picture.pic_parameter_set_id
												<< _T("]");
										}
									}
									
									
									std::wcout << std::endl;

									if(mp4.get_position() < mp4.get_box_position_end())
									{
										std::wcout << "MORE DATA IN stbl" << std::endl;
									}

									return;

								if(mp4.get_position() < mp4.get_box_position_end())
								{
									uint64_t pos = mp4.get_position();
									uint64_t to_be_skipped = mp4.get_box_position_end() - mp4.get_position();
									unsigned int next_size = 0;
									char tt[5];
									bool extra = false;
									
									if(8 <= to_be_skipped)
									{
										extra = true;
										next_size = mp4.read_uint();
										mp4.read_bytes(reinterpret_cast<BYTE*>(tt), 4);

										mp4.skipbytes(to_be_skipped - 8);
										
									}
									else
										mp4.skipbytes(to_be_skipped );

									tt[4] = 0x0;

									

									std::cout << _T(" SKIPPING AVC END BYTES ") << to_be_skipped 
										<< _T("\t") << next_size
										<< _T("\t") << tt
										<< _T("\t[") << extra << _T("]")
										<< _T("\t[") << pos << _T("]")
										<< std::endl;
								}

}



struct moofinfo
{
	unsigned int     sample_count;
	uint64_t baseMediaDecodeTime;

} g_moof_info;

struct moovinfo
{	
	uint64_t mvhd_time_scale;

} g_moov_info;

BOX_FUNCTION(moof)
{
	g_moof_info.sample_count        = 0;
	g_moof_info.baseMediaDecodeTime = 0;

	return 0;
}

BOX_FUNCTION(tfdt)
{
	TrackFragmentBaseMediaDecodeTimeBox box;
	mp4.read_box(box);

	std::wcout << _T("Track Fragment Base Media Decode Time Box ") << std::endl
		<< HNS(box.get_baseMediaDecodeTime()) 
		<< _T("\t")
		<< box.get_baseMediaDecodeTime()
		<< _T("\t")
		
		<< std::endl;

	g_moof_info.baseMediaDecodeTime = box.get_baseMediaDecodeTime();

	return 0;

	
}


BOX_FUNCTION(trik)
{
	
	mp4.do_full_box();

	TrickPlayBoxRecord trik;

	std::wcout << _T("Trick Play Box [trik] sample:") << g_moof_info.sample_count << std::endl;

	for(uint32_t i = 0; i < g_moof_info.sample_count; i++)
	{
		mp4.read_box(trik);


		std::cout << _T("\t") << trik.pic_type << _T(" [");
		
		switch(trik.pic_type) 
		{
		case 0:
			std::wcout << _T("The	type of	this sample is unknown");
			break;
		case 1:
			std::wcout << _T("This sample	is an IDR	picture");
			break;
		case 2:
			std::wcout << _T("This sample	is a Random	Access	(RA)	I-picture");
			break;
		case 3:
			std::wcout << _T("This sample	is an unconstrained	I-picture");
			break;
		}
		
		std::cout << _T("] ") << _T("\t") << trik.dependency_level << std::endl;
	}	
	

	return 0;

	
}

BOX_FUNCTION(sidx)
{
	SegmentIndexBox box;

	mp4.read_box(box);

	std::wcout << _T("Segment Index Box [sidx] sample:") << box.reference_count 
		 << _T("\t") << box.get_earliest_presentation_time()
		  << _T("\t") << box.get_first_offset()
		   << _T("\t") << box.reference_ID
		    << _T("\t") << box.timescale
		<< std::endl;

	std::wcout << _T("\t") 
		<< _T("reference_type")
				   << _T("\t") 
		<< _T("referenced_size")
				   << _T("\t") 
		<< _T("subsegment_duration")
				   << _T("\t") 
		<< _T("starts_with_SAP")
				   << _T("\t") 
		<< _T("SAP_type")
				   << _T("\t") 
		<< _T("SAP_delta_time")
				   << std::endl;

	for (uint32_t i=0; i < box.reference_count; i++) 
	{
		std::wcout << _T("\t") << box.reference_type[i]
				   << _T("\t") << box.referenced_size[i]
				   << _T("\t") << box.subsegment_duration[i]
				   << _T("\t") << box.starts_with_SAP[i]
				   << _T("\t") << box.SAP_type[i]
				   << _T("\t") << box.SAP_delta_time[i]
				   << std::endl;

	}


	return 0;
}



BOX_FUNCTION(avcn)
{
	outputAVCDecoderConfigurationRecord(mp4);

	return 0;
}

BOX_FUNCTION(frma)
{
	OriginalFormatBox frma;
	mp4.read_box(frma);

	std::cout << _T("OriginalFormatBox: ") << _T("\t")
		<< frma.data_format
		<< _T("\t")
		<< frma._stype
		<< _T("\t")
	    << std::endl;

	return 0;
}

BOX_FUNCTION(schm)
{
	SchemeTypeBox schm;
	mp4.read_box(schm);

	std::wcout << _T("SchemeTypeBox: ") 
		<< schm.get_version() << _T("\t") 
		<< schm.get_flags() << std::endl;
	std::wcout << schm._stype
			   << _T("\tscheme_version:\t")
			   << schm.scheme_version
			   << _T("\t")
			   << schm.scheme_type
			   << _T("\t")
			   << std::endl;

	return 0;
}

BOX_FUNCTION(tenc)
{
	TrackEncryptionBox tenc;
	mp4.read_box(tenc);

	std::wcout << _T("TrackEncryptionBox: ") << std::endl
		<< _T("\tversion: ") << tenc.get_version() << std::endl
        << _T("\tdefault_isProtected: ") << tenc.default_isProtected << std::endl
		<< _T("\tdefault_constant_IV_size: ") << tenc.default_constant_IV_size << std::endl
		<< _T("\tdefault_crypt_byte_block: ") << tenc.default_crypt_byte_block << std::endl
		<< _T("\tdefault_Per_Sample_IV_Size: ") << tenc.default_Per_Sample_IV_Size << std::endl
		<< _T("\tdefault_skip_byte_block: ") << tenc.default_skip_byte_block << std::endl
		;

	if (tenc.default_isProtected ==1 && tenc.default_Per_Sample_IV_Size == 0) {
		
		Cstring hex = ALX::hexformat(tenc.default_constant_IV, tenc.default_constant_IV_size);

		std::wcout << _T("default_constant_IV: ") << static_cast<const TCHAR*>(hex) << std::endl;

	}

	Cstring kid = ALX::hexformat(&tenc.default_KID, 16);

	std::wcout << _T("default_KID: ") << static_cast<const TCHAR*>(kid) << std::endl;
   
	return 0;
		
}

BOX_FUNCTION(senc)
{
	SampleEncryptionBox senc;
	mp4.read_box(senc);

	std::wcout << _T("SampleEncryptionBox samples: ") << senc.sample_count << std::endl;

			for(uint32_t i = 0; i < senc.sample_count; i++)
			{
				Cstring hex = ALX::hexformat(&(senc._samples[i]->init_vector->InitializationVector_0), 8);
				
				std::wcout << i << _T("\t\t") << static_cast<const TCHAR*>(hex) << std::endl;

				uint32_t size = 0;
				
				for(uint32_t k = 0; k < senc._samples[i]->subsample_count; k++)
				{
					std::wcout << _T("\t\t\t") 
						<< k << _T("\tBytesOfClearData\t") 
						<< senc._samples[i]->_sub_samples[k]->BytesOfClearData
						<< _T("\tBytesOfProtectedData\t")
						<< senc._samples[i]->_sub_samples[k]->BytesOfProtectedData
						<< std::endl;

					size += senc._samples[i]->_sub_samples[k]->BytesOfClearData;
					size += senc._samples[i]->_sub_samples[k]->BytesOfProtectedData;
				}
				
				std::wcout << i << _T("\tsize:\t") << size << std::endl;

			}


	return 0;
}



BOX_FUNCTION(saiz)
{

	SampleAuxiliaryInformationSizesBox saiz;
	mp4.read_box(saiz);

	std::wcout << _T("SampleAuxiliaryInformationSizesBox:\t") << saiz.sample_count 
		<< _T("\tflags:\t") << saiz.get_flags()
		<< _T("\tversion:\t") << saiz.get_version()
		<< _T("\tdefault_sample_info_size\t") << saiz.default_sample_info_size
		<< std::endl;

	if(0 == saiz.default_sample_info_size)
	{
		for(uint32_t i = 0; i < saiz.sample_count; i++)
			std::wcout << _T("\t\t") << i << _T("\t") << saiz._samples_info_size[i] << std::endl;
	}


	return 0;
		
}

BOX_FUNCTION(saio)
{
	SampleAuxiliaryInformationOffsetsBox saio;
	mp4.read_box(saio);

	std::wcout << _T("SampleAuxiliaryInformationOffsetsBox:\t") << saio.entry_count
		<< _T("\tflags:\t") << saio.get_flags()
		<< _T("\tversion:\t") << saio.get_version()
		<< std::endl;

	for(uint32_t i = 0; i < saio.entry_count; i++)
		std::wcout << _T("\t\t") 
		<< i << _T("\t") 
		<< saio._samples_offset[i] << std::endl;

	return 0;
}

BOX_FUNCTION(pssh)
{
	ProtectionSystemSpecificHeaderBox pssh;
	mp4.read_box(pssh);

	std::wcout << _T("ProtectionSystemSpecificHeaderBox:\t")
	   << std::hex
	   << pssh.SystemID 
	   << pssh.SystemID_1
       << pssh.SystemID_2
       << pssh.SystemID_3
	   << _T("-")
       << pssh.SystemID_4
       << pssh.SystemID_5
	   << _T("-")
       << pssh.SystemID_6
       << pssh.SystemID_7
	   << _T("-")
       << pssh.SystemID_8
       << pssh.SystemID_9
	   << _T("-")
       << pssh.SystemID_10
       << pssh.SystemID_11
       << pssh.SystemID_12
       << pssh.SystemID_13
       << pssh.SystemID_14
       << pssh.SystemID_15
	   << std::dec
	   << std::endl
	   << _T("\t") << pssh.DataSize << std::endl
	   ;

	   Cstring systemid;
	           systemid.append_hex_buffer(&pssh.SystemID, 16);

	   std::wcout
		   << static_cast<const TCHAR*>(systemid)
		   << std::endl;
	   
	   if(ALX::Equals(_T("9A04F07998404286AB92E65BE0885F95"), systemid))
	   {
		    std::wcout
				<< _T("PlayReady drm system") 
				<< std::endl;
			
				FixedMemoryBitstream  playready(pssh._p_data, pssh.DataSize);

				uint32_t size = playready.little_getbits(32);

				_ASSERTE(size == pssh.DataSize);

				uint32_t count = playready.little_getbits(16);

				std::wcout << _T("\t\t size:\t") 
					<< size 
					<< _T("\tfor-objects:\t") << count << std::endl;

				uint16_t tot = 6;

				for(uint32_t p = 0; p < count; p++)
				{
					uint32_t kind = playready.little_getbits(16);
					uint32_t size = playready.little_getbits(16);

					tot += 4;

					std::wcout << _T("\t\t\tHEADER:\t") 
						<< p 
						<< _T("\tkind:\t") 
						<< kind 
						<< _T("\tsize:\t") << size  << std::endl;
					
					_ASSERTE(4 > kind && 0 < kind);

					if(1 == kind)
					{
						CResource<CBuffer<wchar_t> > b(new CBuffer<wchar_t>( (size / 2) + 2));
						                                    b->add(reinterpret_cast<const wchar_t *>( (pssh._p_data + tot) ), (size / 2));

						CstringT<wchar_t> wmheader(b);


						std::wcout << _T("\t\t\t") << static_cast<const wchar_t*>(wmheader) << std::endl;


						        
					}
					else if(2 == kind)
					{
						std::wcout << _T("Reserved Record.") << std::endl;
					}
					else if(3 == kind)
					{
						std::wcout << _T("Indicates an embedded license store.") << std::endl;
					}
					else
					{
						std::wcout << _T("unknown kind") << std::endl;
					}

					int skip = size << 3;

					playready.skipbits(skip);
					
					
				}


	   }
	   else
	   {
		   uint32_t size = 1000;

			if(pssh.DataSize < size)
				size = pssh.DataSize;

			Cstring hex = ALX::hexformat(pssh._p_data, size, 29);
			//Cstring guid = ALX::hexformat(&pssh.SystemID, 16, 7);

			if(size < pssh.DataSize)
				hex.append(_T("........"));

			std::wcout
			<< _T("unknown system") 
			<< std::endl
			<< static_cast<const TCHAR*>(hex) 
			<< std::endl;
	   }

	   //<< static_cast<const TCHAR *>(guid)
		
		
			//<< pssh._p_data << std::endl;
		


	   if(0 < pssh.get_version() )
	   {
		   Cstring kid = ALX::hexformat(&pssh.KID, 16, 16);

		   std::wcout << _T("\tKID COUNT:\t") << pssh.KID_count
			   << std::endl
			   << _T("LAST KID: ") 
			   << std::endl
			   << static_cast<const TCHAR*>(kid)
			   << std::endl;
	   }

	return 0;

}



BOX_FUNCTION(sbgp)
{
	SampleToGroupBox sbgp;
	mp4.read_box(sbgp);

	TCHAR type[5];

	type[0] = ((sbgp.grouping_type >> 24) & 0x000000FF);
	type[1] = ((sbgp.grouping_type >> 16) & 0x000000FF);
	type[2] = ((sbgp.grouping_type >> 8) & 0x000000FF);
	type[3] = ((sbgp.grouping_type) & 0x000000FF);
	type[4] = 0x0;

	std::wcout << _T("version:\t") 
		<< sbgp.get_version()
		<< _T("\tentry:\t")
		<< sbgp.entry_count
		<< _T("\tgrouping_type\t")
		<< sbgp.grouping_type
		<< _T("\t")
		<< type;

	if(0 < sbgp.get_version())
		std::wcout << _T("\tgrouping_type_parameter\t") << sbgp.grouping_type_parameter;

	std::wcout << std::endl;

	for(unsigned int i = 0; i < sbgp.entry_count; i++)
	{
		std::wcout << _T("\t\t\t")
		<< sbgp._samples[i]->sample_count
		<< _T("\t")
		<< sbgp._samples[i]->group_description_index
		<< std::endl;
	}

	return 0;

}



BOX_FUNCTION(sgpd)
{
	SampleGroupDescriptionBox  sgpd;
	mp4.read_box(sgpd);

	_ASSERTE(0x73656967 == sgpd.grouping_type);
	_ASSERTE(1 == sgpd.entry_count);

	TCHAR type[5];

	type[0] = ((sgpd.grouping_type >> 24) & 0x000000FF);
	type[1] = ((sgpd.grouping_type >> 16) & 0x000000FF);
	type[2] = ((sgpd.grouping_type >> 8) & 0x000000FF);
	type[3] = ((sgpd.grouping_type) & 0x000000FF);
	type[4] = 0x0;


	std::wcout << _T("version:\t") 
		<< sgpd.get_version()
		<< _T("\tentry:\t")
		<< sgpd.entry_count
		<< _T("\tgrouping_type\t")
		<< sgpd.grouping_type
		<< _T("\t")
		<< type;

	if(0 < sgpd.get_version())
		std::wcout << _T("\tdefault_length\t") << sgpd.default_length;

	std::wcout << std::endl;


	for (uint32_t i = 1 ; i <= sgpd.entry_count ; i++)
	{
		if (0 < sgpd.get_version())
		{
			if (sgpd.default_length==0) 
			{
				std::wcout << _T("----> description_length\t") << sgpd.description_length << std::endl;
			}
		}

		Cstring kid = ALX::hexformat(&sgpd._seig->KID, 16, 16);
		
		std::wcout << _T("constant_IV_size:\t")
					<< sgpd._seig->constant_IV_size
					<< _T("\tcrypt_byte_block:\t")
					<< sgpd._seig->crypt_byte_block
					<< _T("\tisProtected:\t")
					<< sgpd._seig->isProtected
					<< _T("\tKID:\t")
					<< static_cast<const TCHAR*>(kid)
					<< _T("\tPer_Sample_IV_Size:\t")
					<< sgpd._seig->Per_Sample_IV_Size
					<< _T("\tskip_byte_block:\t")
					<< sgpd._seig->skip_byte_block
					<< std::endl;

		
	}

	return 0;
}

BEGIN_BOX_TYPE

BOX_TYPE(moof, box_simple)
BOX_TYPE(trik, box_simple)
BOX_TYPE(tfdt, box_simple)
BOX_TYPE(sidx, box_simple)
BOX_TYPE(avcn, box_simple)
BOX_TYPE_CONTAINER(sinf)
BOX_TYPE(frma, box_simple)
BOX_TYPE(schm, box_simple)
BOX_TYPE_CONTAINER(schi)
BOX_TYPE(tenc, box_simple)
BOX_TYPE(senc, box_simple)
BOX_TYPE(saiz, box_simple)
BOX_TYPE(saio, box_simple)
BOX_TYPE(pssh, box_simple)
BOX_TYPE(sbgp, box_simple)
BOX_TYPE(sgpd, box_simple)


END_BOX_TYPE


void fill_box_map()
{
	if(g_box_map.size())
		return;

	for(int i = 0; i < g_box_size; i++)
		g_box_map[g_box_all_handler[i].boxname] = g_box_all_handler[i];

}



int doimeta(CMP4 &mp4)
{
	std::wcout << "iMeta: " ;

	switch(mp4.get_box().get_type())
	{
		case box_iNAM:
		   std::wcout << _T("-NAM"); break;
		case box_iCMT:
		   std::wcout << _T("-CMT"); break;
		case box_iDAY:
		   std::wcout << _T("-DAY"); break;
		case box_iART:
		   std::wcout << _T("-ART"); break;
		case box_iTRK:
		   std::wcout << _T("-TRK"); break;
		case box_iALB:
		   std::wcout << _T("-ALB"); break;
		case box_iCOM:
		   std::wcout << _T("-COM"); break;
		case box_iWRT:
		   std::wcout << _T("-WRT"); break;
		case box_iTOO:
		   std::wcout << _T("-TOO"); break;
		case box_iCPY:
		   std::wcout << _T("-CPY"); break;
		case box_iDES:
		   std::wcout << _T("-DES"); break;
		case box_iGEN:
		   std::wcout << _T("-GEN"); break;
		case box_iGRP:
		   std::wcout << _T("-GRP"); break;
		case box_iENC:
		   std::wcout << _T("-ENC"); break;
		case box_aART:
		   std::wcout << _T("aART"); break;
		case box_PGAP:
		   std::wcout << _T("PGAP"); break;
		case box_GNRE:
		   std::wcout << _T("GNRE"); break;
		case box_DISK:
		   std::wcout << _T("DISK"); break;
		case box_TRKN:
		   std::wcout << _T("TRKN"); break;
		case box_TMPO:
		   std::wcout << _T("TMPO"); break;
		case box_CPIL:
		   std::wcout << _T("CPIL"); break;
		case box_COVR:
		   std::wcout << _T("COVR"); break;
		case box_iSpecificInfo:
		   std::wcout << _T("iTunesSpecificInfo"); break;
		case box_DATA:
		   std::wcout << _T("DATA"); break;
		case box_CHAP:
		   std::wcout << _T("CHAP"); break;


	}

	std::wcout << _T(": ");

	mp4.do_box();//data	

	data_box dbox;
	mp4.parse_data_box(dbox);

	std::wcout <<
		mp4.parse_char(static_cast<uint32_t>(mp4.get_box().get_size() - 16))
	<< std::endl;	       

	return 0;
}


int docommand(CMP4 &mp4, const STDTSTRING command, bool extended = false)
{

		   fill_box_map();

		   if(g_box_map.end() != g_box_map.find(command))
		   {
			   return g_box_map[command].mp4command(mp4, extended);
		   }

	       if(command == _T("box") 
			   || command == _T("b"))
		   {
			  if(!mp4.do_box())
				  return ERROR_HANDLE_EOF;

			  std::wcout 
				<< mp4.get_box().get_type_string() << _T(" ")
				<< mp4.get_box_position() << _T(" ")
				<< mp4.get_box().get_size()
				<< _T(" ")
				<< mp4.get_box_position() + mp4.get_box().get_size();
			    

				if(box_uuid == mp4.get_box().get_type())
				{
					std::wcout
					<< _T(" ");
					
					//::StringFromGUID2(

					const unsigned char * p_c = mp4.get_box().guid();

					for(int i = 0; i < 16; i++)
					{
						std::wcout << std::hex
							       << _T(" %x")
								   << p_c[i]
								   << std::dec;
							
					}
				}

				std::wcout
				<< std::endl;

		   }else if(command == _T("fullbox") 
			   || command == _T("f"))
		   {
			  mp4.do_full_box();
			  std::wcout 
				<< mp4.get_box().get_type_string() << _T(" ")
				<< mp4.get_box_position() << _T(" ")
				<< mp4.get_box().get_size()
				<< _T(" version: ")
				<< mp4.get_full_box().get_version()
				<< _T(" flags: ")
				<< mp4.get_full_box().get_flags()
				<< std::endl;

		   }
		   else if(command == _T("skip") 
			   || command == _T("s"))
		   {  
			   mp4.skip_current();
		       std::wcout << _T("skipped") << std::endl;
		   }
		   else if(command == _T("hex") 
			   || command == _T("h"))
		   {
			   uint32_t size = static_cast<uint32_t>(mp4.get_box().get_size());
			       size -= 8;

			   CBuffer<unsigned char> body(size);
			   mp4.read_bytes(body.get(), body.getFree());


			   ALX::Cstring hex = ALX::hexformat(body.get(), size);

			   std::wcout << static_cast<const TCHAR*>(hex) << std::endl;

		   }
		   else if(command == _T("mvhd"))
		   {
			   MovieHeaderBox box;
			   mp4.parse_movie_header_box(box);

			   g_moov_info.mvhd_time_scale = box.get_timescale();

			   std::wcout 
				<< _T("version: ")
				<< box.get_version() 
				<< _T(" timescale: ")
				<< box.get_timescale() 
				<< _T(" duration: ")
				<< box.get_duration()
				<< _T("\tcomputed duration hns:")
				<< HNS(box.get_duration_hns())
				<< _T("\trate: ")
				<< box.get_rate()
				<< _T(" next track id") 
				<< box.get_next_track_ID()
				<< std::endl
				;

		   }
		   else if(command == _T("tkhd"))
		   {
			   TrackHeaderBox box;
			   mp4.parse_track_header_box(box);

			   uint64_t duration = box.get_duration();
                                    duration = (1000UL * 10000UL * duration) / g_moov_info.mvhd_time_scale;

			   std::wcout 
				   << _T("version: ")
				   << box.get_version() 
				   << _T(" track id: ") 
				   << box.get_track_ID()
                   << _T(" duration: ")
				   << box.get_duration() 
				   << _T("\t")
				   << HNS(duration)
				   << _T(" width: ")
				   << box.get_width()  
				   << _T(" height: ")
				   << box.get_height()

				<< std::endl
				;

		   }else if(command == _T("mdhd"))
		   {
			   MediaHeaderBox box;
			   mp4.parse_media_header_box(box);

			   std::wcout << _T("version: ")
				<< box.get_version() 
				<< _T(" timescale: ")
				<< box.get_timescale() 
				<< _T(" duration: ")
				<< box.get_duration()
				<< _T(" computed duration hns:")
				<< HNS(box.get_duration_hns())
				<< _T(" ISO Lang: ")
				<< box.lang_1
				<< _T("-")
			    << box.lang_2
				<< _T("-")
				<< box.lang_3
				<< _T(" (")
			    << box.get_lang()
                << _T(") ")
				<< std::endl
				;

		   }
		   else if(command == _T("hdlr"))
		   {
			   HandlerBox box(static_cast<uint32_t>(mp4.get_box().get_size()));
			   mp4.parse_handler_box(box);

			   std::wcout << _T("desc: ")
				<< box.desc
				<< _T(" handler type: ")
				<< box.htype
				<< std::endl
				;

		   }
		   else if(command == _T("dref"))
		   {
			   DataReferenceBox box;
			   mp4.parse_data_reference_box(box);

			   std::wcout << _T("entry count: ")
				<< box.entry_count
				<< std::endl
				;
		   }
		   else if(command == _T("stsd"))
		   {
			   
			   SampleDescriptionBox box;
			   mp4.parse_sample_description_box(box);

			   std::wcout << _T("Sample description #: ") 
				    << box.entry_count
					<< std::endl;

			   for(uint32_t i = 0; i < box.entry_count; i++)
			   {
				   uint64_t sample_pos = mp4.get_position();

				   Box sample_box;
				   mp4.parse_box(sample_box);

				   std::wcout 
					   << _T("\t\tsample: ") 
					   << sample_box.get_type_string()
					   << _T("\t")
					   << sample_pos
					   << _T("\t")
					   << sample_box.get_size()
					   << _T("\t")
					   << (sample_pos + sample_box.get_size())
					   << std::endl
					   ;

				   if(handler_vide == mp4.get_last_handler_type())
				   {
					   VisualSampleEntry box;
					   mp4.parse_visual_sample_entry(box);

					   Cstring hex = ALX::hexformat(reinterpret_cast<const unsigned char*>(box.compressorname), box.size, 16);
					   

					   std::wcout 
						   << _T("\t\t\tvideo-sample ")
						   << _T("\tdata-reference:\t")
						   << box.data_reference_index
						   << _T("\thorizresolution:")
						   << box.horizresolution
						   << _T("\tvertresolution:")
						   << box.vertresolution
						   << std::endl
						   << _T("\t\t\t\tframe_count:\t")
						   << box.frame_count
						   << _T("\tsize:\t")
						   << box.size
						   << _T("\tdepth:\t")
						   << box.depth
						   << _T("\tcompressorename:\t")
						   << box.compressorname
						   << _T("\tframe-count:\t")
						   << box.frame_count
						   << _T("\theight:\t")
						   << box.height
						   << _T("\twidth:\t")
						   << box.width

						   
						   << std::endl

						   << static_cast<const TCHAR*>(hex)

						   << std::endl

						   ;//<< std::endl;

					   if(mp4.get_position() < mp4.get_box_position_end())
					   {
					      Box config_box;


						  uint64_t config_pos = mp4.get_position();

				          mp4.parse_box(config_box);

						  if(box_clap == config_box.get_type())
						  {
							  CleanApertureBox clap;
							  mp4.parse_clap_box(clap);
							  mp4.parse_box(config_box);

							  std::wcout 
								  << _T("CLAP BOX") 
								  << std::endl;
						  }

						  if(box_pasp == config_box.get_type())
						  {
							  PixelAspectRatioBox pasp;
							  mp4.parse_pasp_box(pasp);
							  mp4.parse_box(config_box);

							  std::wcout 
								  << _T("PASP BOX ")
								  << pasp.vSpacing
								  << _T("x")
								  << pasp.hSpacing
								  << std::endl;

						  }

						  if((box_avc1 == sample_box.get_type() || box_encv == sample_box.get_type())
							  && box_avcC == config_box.get_type()
							  )
						  {
							  							
							  std::wcout 
								  << std::endl
								  << _T("PARSING avcC [") 
								  << config_box.get_type_string() 
								  << _T("] ")
								  << _T("\t")
								  << config_pos
								  << _T("\t")
								  << config_box.get_size()
								  << _T("\t")
								  << (config_pos + config_box.get_size())
								  << std::endl;

							  //////////////////avcn///////////////////////
								outputAVCDecoderConfigurationRecord(mp4);

						  }
						  else
						  {
							  std::wcout 
								  << std::endl
								  << _T("Unknown Video config box: ") 
								  << config_box.get_type_string() 
								  << _T(" config_box position: ")
								  << mp4.get_position()
								  << _T(" size: ")
								  << config_box.get_size()
								  << std::endl;
							  
							  mp4.skipbytes(config_box.get_size() - 8);
					          
						  }
					   }
				   
				   else
				   {
					   std::wcout << _T("Unknown handler type ") << sample_box.get_type_string() << std::endl;
					   return 12;
				   }
			   }
			   else if(handler_soun == mp4.get_last_handler_type())
		       {
					   AudioSampleEntry box;
					   mp4.parse_audio_sample_entry(box);
                        
					   std::wcout
					   << _T(" channelcount: ")
					   << box.channelcount
					   << _T(" samplesize: ")
					   << box.samplesize
					   << _T(" samplerate: ")
					   << box.samplerate
					   << _T(" (")
					   << (box.samplerate >> 16)
					   << _T(") ")
					   << std::endl;

					   if(mp4.get_position() < mp4.get_box_position_end())
					   {
					      Box config_box;
				          mp4.parse_box(config_box);

						  if(box_mp4a == sample_box.get_type()
							  && box_esds == config_box.get_type()
							  )
						  {
							  //version //flags
							  mp4.read_uint();

							  ES_Descriptor es;
							  mp4.parse_es_configuration_record(es);

							  std::wcout 
								  << _T(" ES_ID: ")
									<< es.ES_ID
									<< _T(" streamDependenceFlag: ")
									<< es.streamDependenceFlag
									<< _T(" URL_Flag: ")
									<< es.URL_Flag
									<< _T(" OCR_Flag: ")
									<< es.OCR_Flag
									<< _T(" streamPriority: ")
									<< es.streamPriority
									<< _T(" decoder_config_objectProfileIndication: ")
									<< es.p_decoder_config_descriptor->objectProfileIndication
									<< _T(" decoder_config_streamType: ")
									<< es.p_decoder_config_descriptor->streamType
									<< _T(" decoder_config_upStream: ")
									<< es.p_decoder_config_descriptor->upStream
									<< _T(" decoder_config_bufferSizeDB: ")
									<< es.p_decoder_config_descriptor->bufferSizeDB
									<< _T(" decoder_config_maxBitrate: ")
									<< es.p_decoder_config_descriptor->maxBitrate
									<< _T(" decoder_config_avgBitrate: ")
									<< es.p_decoder_config_descriptor->avgBitrate
									<< _T(" decoder_config_descriptor_size: ")
									<< es.p_decoder_config_descriptor->p_ext->get_length();

									if(es.p_decoder_config_descriptor->p_ext->get_length())
									{
										FixedMemoryBitstream  mem(
																  es.p_decoder_config_descriptor->p_ext->get_descriptor()
																, es.p_decoder_config_descriptor->p_ext->get_length()
																);

										aac_info_mp4 aac;
										aac.get(mem);

										std::wcout 
											<< _T(" [aac channels: ")
											<< aac.channels
											<< _T(" aac sample_rate: ")
											<< aac.sample_rate
											<< _T(" aac object type: ")
											<< aac.object_type
											<< _T("] {")
											<< _T(" frame_length_flag:")
											<< aac.frame_length_flag
											<< _T(" depends_core_coder:")
											<< aac.depends_core_coder
											<< _T(" core_coder_delay:")
											<< aac.core_coder_delay
											<< _T(" extension_flag:")
											<< aac.extension_flag
											<< _T(" }")
											;

									}


									std::wcout 
									<< std::endl;
						  }
						  else
						  {
							  std::wcout 
								  << std::endl
								  << _T("Unknown Audio config box: ") 
								  << config_box.get_type_string() 
								  << _T(" config_box position: ")
								  << mp4.get_position()
								  << _T(" size: ")
								  << config_box.get_size()
								  << std::endl;
							  
							  mp4.skipbytes(config_box.get_size() - 8);
					          
						  }
						}//if position

				}//else if(handler_soun
		   }//for
		   }//command
		   else if(command == _T("btrt"))
		   {
				   BitRateBox box;
				   mp4.parse_bitrate_box(box);

				   std::wcout
					    << _T(" bufferSizeDB: ")
						<< box.bufferSizeDB
						<< _T(" maxBitrate: ")
						<< box.maxBitrate
						<< _T(" avgBitrate: ")
						<< box.avgBitrate
						<< std::endl;
		   
		   }else if(command == _T("stts"))
		   {
		   
			   std::wcout << _T("stts (decoding) time-to-sample") << std::endl;

			   TimeToSampleBox box;
			   mp4.parse_time_to_sample_box(box);

			   uint64_t total_samples(0);

			   std::wcout << _T("stts entries ")
				   << box.entry_count
				   << std::endl;

			   for(unsigned int x = 0; x < box.entry_count; x++)
			   {
				   unsigned int sample_count = mp4.read_uint();
	               unsigned int sample_delta = mp4.read_uint();

				   std::wcout << _T("\tstts entries ")
				   << sample_count 
				   << _T("\t")
				   << sample_delta
				   << std::endl;

					total_samples += sample_count;
			   }

			   std::wcout << _T("\tsample total count:")
				   << total_samples << std::endl;

		   }else if(command == _T("stsc"))
		   {

			   mp4.do_full_box();


			   std::wcout << _T("stsc sample-to-chunk, partial data-offset information") << std::endl;

			   unsigned int entry_count = mp4.read_uint();

			   std::wcout << _T("stsc entries ")
				   << entry_count
				   << std::endl;

			   
			   std::wcout << _T("           \t")
							   << _T("first_chunk")
							   << _T("\t")
							   << _T("samples_per_chunk")
							   << _T("\t")
							   << _T("sample_description_index")
							   << std::endl;

			   for(unsigned int x = 0; x < entry_count; x++)
			   {
				    unsigned int first_chunk = mp4.read_uint();
					unsigned int samples_per_chunk = mp4.read_uint();
					unsigned int sample_description_index = mp4.read_uint();

					std::wcout << _T("stsc entry:\t")
							   << first_chunk
							   << _T("\t")
							   << samples_per_chunk
							   << _T("\t")
							   << sample_description_index
							   << std::endl;
			   }


           }else if(command == _T("ilst"))
		   {
			   ilst_box box;
			   mp4.parse_ilst_box(box);

			   std::wcout << _T("ilst subtype: ") 
				   << box.subtype
				   << std::endl;
		   
		   }else if(command == _T("elst"))
		   {
			   EditListBox box;
			   mp4.parse_edl_box(box);

			   std::wcout << _T("elst entry: ")
				   << box.count()
				   << std::endl;

			   if(box.count())
			   {
				   std::wcout << _T("segment_duration\t[segment_duration]\tmedia_time\tmedia_rate_integer\tmedia_rate_fraction") << std::endl;
			   }

			   for(uint32_t i = 0; i < box.count(); i++)
			   {

				   uint64_t duration = box.entry(i)._segment_duration;
                                    duration = (1000UL * 10000UL * duration) / g_moov_info.mvhd_time_scale;

				   std::wcout 
					   << box.entry(i)._segment_duration
					   << _T("\t")
					   << HNS(duration)
					   << _T("\t")
					   << box.entry(i)._media_time
					   << _T("\t")
					   << box.entry(i)._media_rate_integer
					   << _T("\t")
					   << box.entry(i)._media_rate_fraction
				       << std::endl;
			   }
		   }else if(command == _T("ftyp"))
		   {
			   FileTypeBox box(static_cast<uint32_t>(mp4.get_box().get_size()));
			   mp4.parse_file_type_box(box);
			
			   char brand[5];
			   brand[0] = box.get_major_brand() >> 24;
			   brand[1] = box.get_major_brand() >> 16;
			   brand[2] = box.get_major_brand() >> 8;
			   brand[3] = box.get_major_brand();
			   brand[4] = '\0';

			   std::wcout << _T("File Type: ") 
				   << brand
				   << _T(".")
				   << box.get_minor_version()
				   << _T("\tBrands: ")
				   << box.count()
				   << std::endl;
					
				   for(unsigned int i = 0; i < box.count(); i++)
				   {

				   brand[0] = box.get_brand(i) >> 24;
				   brand[1] = box.get_brand(i) >> 16;
				   brand[2] = box.get_brand(i) >> 8;
				   brand[3] = box.get_brand(i);
					   
					   std::wcout << _T("\t")
					   << brand
					   << std::endl;
				  }		   
		   }else if(command == _T("smhd"))
		   {
			   SoundMediaHeaderBox box;
			   mp4.parse_sound_header_box(box);

			   std::wcout 
				   << _T("balance: ")
				   << box.balance
				   << std::endl;

		   }else if(command == _T("vmhd"))
		   {
			   VideoMediaHeaderBox box;
			   mp4.parse_video_header_box(box);

			   std::wcout 
				   << box.graphicsmode
				   << std::endl;

		   }else if(command == _T("xml ") 
			   || command == _T("xml"))
		   {
			   XmlBox box(static_cast<uint32_t>(mp4.get_box().get_size()));
			   mp4.parse_xml_box(box);

			       std::wcout 
				   << _T("XML: ")
				   << box._p_xml
				   << std::endl;

		   }else if(command == _T("mfhd"))
		   {
			   MovieFragmentHeaderBox box;
			   mp4.parse_movie_fragment_header_box(box);

				   std::wcout 
				   << _T("fragment ")
				   << box.sequence_number
				   << std::endl;

		   }else if(command == _T("tfhd"))
		   {
			    _fragmented_default_duration = 0;
			    TrackFragmentHeaderBox box;
				mp4.parse_track_fragment_header_box(box);

                   std::wcout 
				   << _T("fragment header")
				   << _T(" track_ID: ")
				   << box.track_ID
				   ;

					if(0x000001 & box.get_flags())
						std::wcout << _T(" offset: ") << box.get_base_data_offset();
					if(0x000002 & box.get_flags()) 
						std::wcout << _T(" description index: ") << box.sample_description_index; 
					if(0x000008 & box.get_flags()) 
					{
						std::wcout << _T(" sample duration: ") << box.default_sample_duration; 
						_fragmented_default_duration = box.default_sample_duration;
					}
					if(0x000010 & box.get_flags()) 
						std::wcout << _T(" sample size: ") << box.default_sample_size; 
                    if(0x000020 & box.get_flags()) 
						std::wcout << _T(" sample flags: ") << box.default_sample_flags;
				   
					std::wcout << std::endl;
	
		   }else if(command == _T("trun") 
			   || command == _T("trunvalidate"))
		   {
				TrackRunBox box;
				mp4.parse_track_run_box(box);

				int64_t dts = 0;
				int64_t cts = 0;

				std::wcout << _T("trun version ") << box.get_version();

				std::wcout << _T(" trun flags ") << box.get_flags();

				std::wcout << _T(" framgment samples: ") << box.get_sample_count();

				g_moof_info.sample_count = box.get_sample_count();
				
				if(0x000001 & box.get_flags()) 
						std::wcout << _T(" data offset: ") << box.data_offset; 
	            if(0x000004 & box.get_flags()) 
						std::wcout << _T(" first sample flags: ") << box.first_sample_flags;

				std::wcout << std::endl;


				for(uint32_t i = 0; i < box.get_sample_count(); i++)
				{
					if(0x000100 & box.get_flags()) 
						std::wcout << _T(" sample duration: ") << box.get_item(i).sample_duration; 
                    if(0x000200 & box.get_flags()) 
						std::wcout << _T(" sample size: ") << box.get_item(i).sample_size; 
                    if(0x000400 & box.get_flags()) 
						std::wcout << _T(" sample flags: ") << box.get_item(i).sample_flags;
                    if(0x000800 & box.get_flags()) 
					{
						int c = box.get_item(i).sample_composition_time_offset;
						std::wcout 
						<< _T(" sample composition: ") 
						<< box.get_item(i).sample_composition_time_offset 
					    << _T(" ") 
						<< HNS(c)
					    << _T(" ") 
						<< HNS(dts)
					    << _T(" ") 
						<< HNS(dts + c);
					}
   
					if(0x000100 & box.get_flags())
					   dts += box.get_item(i).sample_duration;
				    else
					{
						if(!_fragmented_default_duration)
							dts += 400000;
						else
							dts += _fragmented_default_duration;
					}

                        std::wcout << std::endl;
				}

				if(command == _T("trunvalidate"))
				{
					/*if(!mp4.do_box())
					{
						std::wcout << _T("trunvalidate failde to find mdat") << std:endl;
						return 2;
					}

					if(mp4.)
					{
						std::wcout << _T("trunvalidate failde to find mdat") << std:endl;
						return 2;
					}*/

					

					//if(mp4.get_cu


					dobox(mp4, _T("mdat"), true);

					std::wcout << _T("VALIDATING TRUN") << std::endl;

					uint64_t startposition = mp4.get_position();

					for(uint32_t i = 0; i < box.get_sample_count(); i++)
					{
						


						uint64_t actual_size = mp4.read_uint() + 4;
						
						if(actual_size != box.get_item(i).sample_size)
						{
							std::wcout 
								<< _T("trunvalidate failed to validete size ") 
								<< actual_size
								<< _T("\t")
								<< box.get_item(i).sample_size
								<< _T("\t")
								<< startposition
								<< _T("\t")
								<< i
								<< std::endl;
							
							return 2;
						}
						else
						{
							std::wcout 
								<< _T("trunvalidate validated size ") 
								<< actual_size
								<< _T("\t")
								<< box.get_item(i).sample_size
								<< _T("\t")
								<< startposition
								<< _T("\t")
								<< i
								<< std::endl;
						}

						mp4.skipbytes(box.get_item(i).sample_size - 4);

						startposition += box.get_item(i).sample_size;
					}


				}


		   }else if(command == _T("mehd"))
		   {
			   MovieExtendsHeaderBox box;
			   mp4.parse_movie_extends_header_box(box);

			   std::wcout << box.get_fragment_duration()
				   << std::endl;

		   }else if(command == _T("trex"))
		   {
			   TrackExtendsBox box;
			   mp4.parse_track_extends_header_box(box);

			   std::wcout 
				    << _T(" track_ID: ") << box.track_ID
					<< _T(" default_sample_description_index: ") << box.default_sample_description_index
					<< _T(" default_sample_duration: ") << box.default_sample_duration
					<< _T(" default_sample_size: ") << box.default_sample_size
					<< _T(" default_sample_flags: ") << box.default_sample_flags
				<< std::endl;

		   }else if(command == _T("tfra"))
		   {
			   TrackFragmentRandomAccessBox box;
			   mp4.parse_track_fragment_random_access_box(box);

			   std::wcout << _T("TFRA: ") << box.get_count() << std::endl;

				   for(int i = 0; i < box.get_count(); i++)
				   {
					   RandomAccessPoint & a = box.get_point(i);
					   std::wcout 
						    << _T("\t")
							<< _T(" time: ") << a.time
							<< _T(" offset: ") << a.offset
							<< _T(" traf_number: ") << a.traf_number
							<< _T(" trun_number: ") << a.trun_number
							<< _T(" sample_number: ") << a.sample_number
							<< std::endl;
				   }

			     
		   }else if(command == _T("flush"))
		   {
			   for(int i = 0; i < 100; i++)
			   {
				   std::wcerr << i;

				   for(int x = 0; x < i; x++)
				   {
					   std::wcerr << _T("=");
				   }

				   std::wcerr << std::endl;

				   std::flush(std::wcerr);
			   }
		   
		   }else if(command == _T("help"))
		   {
			   std::wcout << L"box \
					fullbox \
					skip \
					mvhd \
					tkhd \
					mdhd \
					hdlr \
					dref \
					stsd \
					btrt \
					stts \
					ilst \
					elst \
					ftyp \
					smhd \
					vmhd \
					xml \
					mfhd \
					tfhd \
					trun \
					tfra \
					analyze \
					move \
					forward \
					position \
					rint \
					target " << std::endl;

		   }else
		   {
			   if(command != _T("quit") 
				   && command != _T("q"))
			      std::wcerr << _T("command not found ") 
				   << command.c_str() << " type help for command list" << std::endl;
		   }

	   return 0;
}

int dobox(CMP4 &mp4, const TCHAR * target, bool recursive, bool extended)
{
	if(mp4.eof())
		return 0;	
	
	int res = docommand(mp4, _T("box"));
	if(0 != res)
		return res;

	const TCHAR * type = mp4.get_box().get_type_string();

	if(ALX::Equals(target, type))
		return 0;

	fill_box_map();

	bool do_box_simple    = false;
	//bool do_box_complex   = false;
	bool do_box_container = false;
	bool do_box_skip      = false;

	if(g_box_map.end() != g_box_map.find(type))
	{
		if(BOXTYPE::box_simple == g_box_map[type].boxtype)
		{
			do_box_simple    = true;
		}

		if(BOXTYPE::box_container == g_box_map[type].boxtype)
		{
			do_box_container    = true;
		}

		if(BOXTYPE::box_extended == g_box_map[type].boxtype)
		{			
			do_box_simple    = true;
		}
	}

    const unsigned int box_type = mp4.get_box().get_type();
	if(
		   ALX::Equals(_T("moov"), type)
		|| ALX::Equals(_T("trak"), type)
		|| ALX::Equals(_T("mdia"), type)
		|| ALX::Equals(_T("minf"), type)
		|| ALX::Equals(_T("dinf"), type)
		|| ALX::Equals(_T("stbl"), type)  
		|| ALX::Equals(_T("udta"), type)
		|| ALX::Equals(_T("edts"), type)
		|| ALX::Equals(_T("mvex"), type)
		|| ALX::Equals(_T("mfra"), type)
        || ALX::Equals(_T("traf"), type)
		|| do_box_container
		)
	{
		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}
	else if(
		    ALX::Equals(_T("meta"), type)
			)
	{
		res = docommand(mp4, _T("fullbox"));
		if(0 != res)
			return res;
		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}
	else if(
		   ALX::Equals(_T("ftyp"), type)
		|| ALX::Equals(_T("mvhd"), type)
		|| ALX::Equals(_T("tkhd"), type)
		|| ALX::Equals(_T("mdhd"), type)
		|| ALX::Equals(_T("hdlr"), type)
		|| ALX::Equals(_T("dref"), type)
		|| ALX::Equals(_T("stsd"), type)
		|| ALX::Equals(_T("btrt"), type)
        || ALX::Equals(_T("ilst"), type)
		|| ALX::Equals(_T("elst"), type)
		|| ALX::Equals(_T("vmhd"), type)
		|| ALX::Equals(_T("smhd"), type)  
		|| ALX::Equals(_T("xml "), type) 
		|| ALX::Equals(_T("mfhd"), type)
		|| ALX::Equals(_T("tfhd"), type)
		|| ALX::Equals(_T("mehd"), type)
		|| ALX::Equals(_T("trex"), type)
        || ALX::Equals(_T("trun"), type)
		|| do_box_simple
		)
	{
		res = docommand(mp4, type, extended);
		if(0 != res)
			return res;
		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}
	else if(ALX::Equals(_T("iods"), type)
		 || ALX::Equals(_T("ctts"), type)
		 || ALX::Equals(_T("stss"), type)
		 || ALX::Equals(_T("stsc"), type)
		 || ALX::Equals(_T("stsz"), type)
		 || ALX::Equals(_T("stco"), type)
		 || ALX::Equals(_T("free"), type)
		 || ALX::Equals(_T("mdat"), type)
		 || ALX::Equals(_T("stts"), type)
		)
	{
		res = docommand(mp4, _T("skip"));
		if(0 != res)
			return res;
		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}
	else if(
		       box_iNAM == box_type
			|| box_iCMT == box_type
			|| box_iDAY == box_type
			|| box_iART == box_type
			|| box_iTRK == box_type
			|| box_iALB == box_type
			|| box_iCOM == box_type
			|| box_iWRT == box_type
			|| box_iTOO == box_type
			|| box_iCPY == box_type
			|| box_iDES == box_type
			|| box_iGEN == box_type
			|| box_iGRP == box_type
			|| box_iENC == box_type
			|| box_aART == box_type
			|| box_PGAP == box_type
			|| box_GNRE == box_type
			|| box_DISK == box_type
			|| box_TRKN == box_type
			|| box_TMPO == box_type
			|| box_CPIL == box_type
			|| box_COVR == box_type
			|| box_iSpecificInfo == box_type
			|| box_CHAP == box_type
		)
	{
		int res = doimeta(mp4);
		if(0 != res)
			return res;
		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}
	else
	{
		std::wcout << _T("Unknown box ")
			<< mp4.get_box_position()
			<< _T(" ")
			<< mp4.get_box().get_type()
			<< _T(" ")
			<< mp4.get_box().get_type_string()
			<< std::endl;

		mp4.skip_current();

		if(recursive)
		{
			res = dobox(mp4, target);
			if(0 != res)
				return res;
		}
	}

	return 0;

}


int doboxroot(CMP4 &mp4, const TCHAR * target = _T("xxxx"), bool recursive = true)
{
	int res = dobox(mp4, target, false);
	if(0 != res)
		return res;

	const TCHAR * type = mp4.get_box().get_type_string();

	while(!ALX::Equals(target, type))
	{
		res = dobox(mp4, target, false);
		if(0 != res)
			return res;
		type = mp4.get_box().get_type_string();
	}

	return 0;
}

int interactive(CMP4 &mp4)
{
	//unsigned int last_handler_type;

	    STDTSTRING command;
		STDTSTRING last_command;
		
		while(command != _T("quit") 
			&& command != _T("q"))
		{
		   std::wcout << _T("(mp4) ");
#ifdef UNICODE
		   std::wcin >> command;
#else
		   std::cin >> command;
#endif

		   if(command == _T("")) //on enter repeat last
			   command = last_command;

		   last_command = command;

		   if(command == _T("analyze") 
			   || command == _T("analize") 
			   || command == _T("a"))
		   {
			   dobox(mp4);
		   }
		   else if(command == _T("move") 
			   || command == _T("m"))
		   {
			   uint64_t position(0);
			   std::wcout << _T("(enter position) ");
		       std::wcin >> position;

			   mp4.set_position(position);
               
			   std::wcout 
				   << _T("Moved to ") 
				   << position
				   << std::endl;
			   
		   }
		   else if(command == _T("forward") 
			   || command == _T("f"))
		   {
			   uint64_t position(0);
			   std::wcout << _T("(enter farward bytes) ");
		       std::wcin >> position;

			   mp4.skipbytes(position);
               
			   std::wcout 
				   << _T("Jumped forward ") 
				   << position
				   << _T(" Bytes")
				   << std::endl;
			   
		   }
		   else if(command == _T("position") 
			   || command == _T("p"))
		   {
			   uint64_t position(0);
			                  
			   std::wcout 
				   << _T("Current position ") 
				   << mp4.get_position()
				   << std::endl;
			   
		   }
		   else if(command == _T("rint") 
			   || command == _T("ri"))
		   {			   
			                  
			   std::wcout 
				   << _T("int val ") 
				   << mp4.read_uint()
				   << std::endl;
			   
		   }
		   else if(command == _T("target") 
			   || command == _T("t"))
		   {
			   STDTSTRING target;
			                  
			   std::wcout 
				   << _T("enter target "); 

#ifdef UNICODE
			   std::wcin >> target;
#else
			   std::cin >> target;
#endif

			   dobox(mp4, target.c_str());
			   
		   }
		   else
		   {
			   int ret = docommand(mp4, command);
			   if(0 != ret)
				   return ret;

		   }


		}

			return 0;
}

int testvideoindex(CMP4 &mp4)
{
	MP4Reader reader;

	reader.parse(mp4);

	int max_error_count = 45;
	int errors(0);

	uint32_t video_stream(0);

	uint64_t cn = reader.get_sample_count(video_stream);

	std::wcout << _T("video index check of stream ") << video_stream << std::endl;

	for(uint64_t i = 0; i < cn; i++)
	{
		uint64_t offset = reader.get_sample_offset(i, video_stream);
		uint64_t size   = reader.get_sample_size(i, video_stream);

		mp4.set_position(offset);

		uint64_t actual_size = mp4.read_uint() + 4;

		if(size != actual_size)
		{
			std::wcout << _T("video index error - sample: ")
				<< i
				<< " offset: "
                << offset
				<< " expected: " 
				<< size
				<< " found: "
				<< actual_size
				<< std::endl;

			errors++;
		}

		if(errors > max_error_count)
			break;
	}

	std::wcout << _T("video index found ") 
		<< errors
		<< _T(" errors")
		<< std::endl;

	//while(errors < max_error_count

	return 0;
}

int do_edit_header(  CMP4 &mp4
				   , CMP4W & mem
				   , MP4Write & mp4w
				   )
{
	MP4Reader reader;

	std::wcout << _T("Parse input headers") << std::endl;

	reader.parse(mp4);

	mp4w.write_ftyp(mem);

	for(int i = 0; i < reader.stream_count(); i++)
	{
		_ASSERTE(1 == reader.entry_count(i));

		if(reader.IsVisual(i))
		{
			std::wcout << _T("Add Visual Stream") << std::endl;

			_ASSERTE(!reader.IsSound(i));
			mp4w.add_visual_stream(reader.get_visual_entry(0, i)
				, reader.get_media_time_scale(i));

			//mp4w.set_ctts_offset(i, 50000000ULL);
		}
		else
		{
			_ASSERTE(!reader.IsVisual(i));

			std::wcout << _T("Add Audio Stream") << std::endl;

			mp4w.add_audio_stream(reader.get_audio_entry(0, i)
				, reader.get_media_time_scale(i));

			//mp4w.set_ctts_offset(i, 5000000ULL);
		}
	}

	return 0;
}

int do_edit_header2( CMP4 &mp4
				   , CMP4W & mem
				   , MP4Write & mp4w)
{
	MP4Reader reader;

	std::wcout << _T("Parse input headers") << std::endl;

	reader.parse(mp4);

	mp4w.write_ftyp(mem);

	for(int i = 0; i < reader.stream_count(); i++)
	{
		_ASSERTE(1 == reader.entry_count(i));

		if(reader.IsVisual(i))
		{
			std::wcout << _T("Add Visual Stream") << std::endl;

			_ASSERTE(!reader.IsSound(i));
			/*mp4w.add_visual_stream(reader.get_visual_entry(0, i)
				, reader.get_media_time_scale(i));*/

			mp4w.add_visual_stream(
				  reader.get_visual_entry(0, i).get_avcc_header().get_nal_sequence(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_nal_picture(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_picture_size(0)
				, reader.get_media_time_scale(i)
				);



			//mp4w.set_ctts_offset(i, 50000000ULL);
		}
		else
		{
			_ASSERTE(!reader.IsVisual(i));

			std::wcout << _T("Add Audio Stream") << std::endl;

			FixedMemoryBitstream  mem(
				reader.get_audio_entry(0, i).get_body()
				, static_cast<uint32_t>(reader.get_audio_entry(0, i).get_body_size()));

			unsigned int box_size = mem.getbits(32);
			unsigned int box_type = mem.getbits(32);
			
			_ASSERTE(box_esds == box_type);

			mem.skipbits(32); //version flags

			ES_Descriptor es;
			es.get(mem);

			if(!es.p_decoder_config_descriptor->p_ext->get_length())
				ALXTHROW_T(_T("no audio decoder configuration"));
			
			FixedMemoryBitstream  maac(es.p_decoder_config_descriptor->p_ext->get_descriptor()
								, es.p_decoder_config_descriptor->p_ext->get_length()
								  );

			aac_info_mp4 aac;
			aac.get(maac);

			mp4w.add_audio_stream(
				  aac.object_type
				, aac.sample_rate
                , aac.channels
	            , static_cast<uint32_t>(reader.get_stream_bit_rate(i))
				, reader.get_media_time_scale(i)
				);

			//mp4w.set_ctts_offset(i, 5000000ULL);
		}
	}

	return 0;
}



int do_edit(uint64_t start, uint64_t end
			, CMP4 &mp4, CMP4W & mp4w, MP4Write & body)
{
	sample_stream_info ms;

	ms.composition_time = 0;
	
	MP4Reader reader;
	reader.parse(mp4);

	std::vector<bool> streams(reader.stream_count());
	
	for(uint32_t i = 0; i < streams.size(); i++)
		streams[i] = true;

	reader.move(start);

	bool work = true;

	CBuffer<BYTE> sample(1024);

	while(work)
	{
		if(!reader.next_file_chunks(ms))
			break;

		sample.prepare(static_cast<uint32_t>(ms.size));

		mp4.set_position(ms.offset);
		mp4.read_bytes(sample.get(), ms.size);

		body.add_sample(ms.stream
			, sample.get()
			, static_cast<uint32_t>(ms.size)
			, ms.bIsSyncPoint
			, ms.composition_time
			, ms.decoding_time
			, mp4w);

		if(0 < end)
		{
			streams[ms.stream] = (ms.decoding_time < static_cast<int64_t>(end));

			for(uint32_t i = 0; i < streams.size(); i++)
				work &= streams[i];
		}

	}

	return 0;

}


int simulate_auto_decoding(uint64_t start, uint64_t end
			, CMP4 &mp4
			, int stream = -1)
{

	         int64_t max_distance      = 0;
	uint64_t max_distance_time = 0;


	if(-1 == stream)
		stream = 0;
	
	sample_stream_info ms;

	ms.composition_time = 0;

	MP4Reader reader;
	reader.parse(mp4);

	
	std::vector<uint64_t> auto_decoding_vector;
	std::vector<uint64_t> composition_vector;


	//if(0 == end)
	//	    end = reader.get_duration();

	std::vector<bool> streams(reader.stream_count());
	std::vector<uint64_t> samples(reader.stream_count());
	
	
	for(uint32_t i = 0; i < streams.size(); i++)
	{
		

		streams[i] = true;
	}

	reader.move(start);

	

	bool work = true;

	while(work)
	{
		work = reader.next_file_chunks(ms);

		if(-1 < stream 
			&& stream != ms.stream
			&& work)
			continue;

		auto_decoding_vector.push_back(ms.composition_time);
		composition_vector.push_back(ms.composition_time);
				
		streams[ms.stream] = (ms.decoding_time < static_cast<int64_t>(end)) || (0 == end);

		for(uint32_t i = 0; i < streams.size(); i++)
			work &= streams[i];

		
	
	}

	std::sort(auto_decoding_vector.begin()
			, auto_decoding_vector.end());


	_ASSERTE(auto_decoding_vector.size() == composition_vector.size());

	uint64_t duration = 0;
	uint64_t tot     = 0;

	for(uint32_t i = 0; i < auto_decoding_vector.size(); i++)
	{

		if(i > 0)
		{
			duration = CMP4Stream::get_auto_decoding_duration(auto_decoding_vector, composition_vector[i]);
			tot     += duration;
		}
		else
		{
			tot = auto_decoding_vector[0];
		}

		int64_t distance = tot - composition_vector[i];
		if(max_distance < distance)
		{
			max_distance      = distance;
			max_distance_time = composition_vector[i];
		}

		std::wcout 
			<< HNS(auto_decoding_vector[i])
			<< _T("\t")
			<< HNS(composition_vector[i])
			<< _T("\t")
			<< HNS(duration)
			<< _T("\t")
			<< HNS(tot)
			<< _T("\t")
			<< HNS(distance)
			<< _T("\t")
			<< HNS(max_distance)
			<< _T("\t")
			<< HNS(max_distance_time)
			<< std::endl;


	}


	return 0;
}


int frames(   const TCHAR * in_file
		    , uint64_t start, uint64_t end
			, CMP4 &mp4
			, const TCHAR * output_dir
			, int stream = -1
			, int max = 0)
{
	
	/*
	sample_stream_info ms;

	ms.composition_time = 0;

	MP4Reader reader;
	reader.parse(mp4);

	std::vector<bool> streams(reader.stream_count());

	for(int i = 0; i < streams.size(); i++)
	{
		
		streams[i] = true;
	}

	ALX::SHStream    binary_file;
	binary_file.Open(in_file);

	reader.move(start);
	
	bool work = true;

	int count(0);

	while(work)
	{
		work = reader.next_file_chunks(ms);

		if(-1 < stream 
			&& stream != ms.stream
			&& work)
			continue;

		Cstring file(output_dir);

		        file += _T("\\s");
				file += ms.stream;
				file += _T("_");
				file += ms.sample_number;
				file += _T(".bin");

	  
	  CFileOut f(file);

	  binary_file.SetPosition(ms.offset);
	  binary_file.ReadInBuffer(f, ms.size);


	  streams[ms.stream] = (ms.decoding_time < end) || !end;

		for(int i = 0; i < streams.size(); i++)
			work &= streams[i];

		work &= ((count++ < max) || 0 == max);
		
	}

	*/
	return 0;
}


int pick_samples(uint64_t start, uint64_t end
			, CMP4 &mp4
			, int stream = -1)
{
	
	sample_stream_info ms;

	ms.composition_time = 0;

	MP4Reader reader;
	reader.parse(mp4);

	uint64_t max_distance      = 0;
	uint64_t max_distance_time = 0;


	//if(0 == end)
	//	    end = reader.get_duration();

	std::vector<bool> streams(reader.stream_count());
	std::vector<uint64_t> samples(reader.stream_count());
	
	std::wcout << _T(" duration: ") 
			<< HNS(reader.get_duration())
			<< std::endl;

	for(uint32_t i = 0; i < streams.size(); i++)
	{
		std::wcout << i
			<< _T(") offset: ")
			<< HNS(reader.get_stream_offset(i))
			<< _T(" duration: ") 
			<< HNS(reader.get_duration(i))
			<< _T(" sample duration: ")
			<< HNS(reader.get_sample_duration(i))
			<< _T(" [")
			<< reader.get_sample_duration(i)
			<< _T("]")
			<< _T(" samples count: ")
			<< reader.get_sample_count(i)
			<< _T(" start time: ")
			<< HNS(reader.get_start_time(i))
			<< _T(" end time: ")
			<< HNS(reader.get_end_time_plus_duration(i))
			/*
			<< _T(" {")
			<< HNS(  (reader.get_end_time_plus_duration(i) - reader.get_start_time(i)) / reader.get_sample_count(i) )
			<< _T("}")
			*/
			<< std::endl;

		streams[i] = true;
	}

	if(0 < start)
		reader.move(start);

	std::wcout 
		    << _T("stream")
			<< _T("\t")
			<< _T("sample")
			<< _T("\t")
			<< _T("composition_time")
			<< _T("\t")
			<< _T("decoding_time")
			<< _T("\t")
			<< _T("duration")
			<< _T("\t\t")
			<< _T("bIsSyncPoint")
			<< _T("\t")
			<< _T("offset")
			<< _T("\t")
			<< _T("size")
			<< std::endl;

	bool work = true;

	while(work)
	{
		work = reader.next_file_chunks(ms);

		if(!work)
			continue;

		if(-1 < stream 
			&& stream != ms.stream
			&& work)
			continue;

		std::wcout << ms.stream
			<< _T("\t")
			<< ms.sample_number
		    << _T("\t")
			<< HNS(ms.composition_time + reader.get_stream_offset(ms.stream))
			<< _T("\t")
			<< HNS(ms.decoding_time)
			<< _T("\t")
			<< HNS(ms.duration)
			<< _T("\t")
			<< ms.bIsSyncPoint
			<< _T("\t")
			<< ms.offset
			<< _T("\t")
			<< ms.size
			<< std::endl;
		
		streams[ms.stream] = (ms.decoding_time < static_cast<int64_t>(end)) || (0 == end);

		for(uint32_t i = 0; i < streams.size(); i++)
			work &= streams[i];

		uint64_t d = ms.composition_time - ms.decoding_time;

		if(max_distance < d)
		{
			max_distance = d;
			max_distance_time = ms.composition_time;
		}
	
	}

	std::wcout << _T("MAX DISTANCE: ") 
		<< HNS(max_distance) << _T(" AT ") << HNS(max_distance_time) << std::endl;

	return 0;
}

int gop_list( uint64_t start
	        , uint64_t end
			, CMP4 &mp4
)
{
	
	MP4Reader reader;
	reader.parse(mp4);

	std::wcout << _T("Duration ") << HNS(reader.get_duration()) << std::endl;

	if(0 == end)
		end = reader.get_duration();

	unsigned int video(UINT32_MAX);
	
	/*
	for(int i = 0; i < reader.stream_count(); i++)
	{
		if(reader.IsVisual(i))
		{
			video = i;
			break;
		}
	}*/

	if(!reader.HasVisual())
	{
		std::wcerr << _T("Cannot find video stream") << std::endl;
		return 2;
	}

	video = reader.VisualStream();
		

	uint64_t offset   = reader.get_stream_offset(video);
	uint64_t sample   = reader.get_IFrame_number(start, video);
	uint64_t time     = reader.get_composition_time(sample, video);
    uint64_t decoding = reader.get_decoding_time(sample, video);

	uint64_t next_time = UINT64_MAX;

	std::wcout << video << _T("\t") 
		<< sample << _T("\t") << HNS(time) 
			<< _T("\t")
			<< time
	        << _T("\t")
			<< HNS(decoding)

			<< std::endl;

	while(next_time > time && time < end)
	{
		if(UINT64_MAX > next_time)
		   time = next_time;

		next_time = reader.get_next_IFrame_time(video);
		sample    = reader.get_IFrame_number(next_time, video);
		decoding = reader.get_decoding_time(sample, video);
                

		std::wcout << video << _T("\t") 
			<< sample << _T("\t") << HNS(next_time) 
			<< _T("\t")
			<< next_time
			<< _T("\t")
			<< HNS(decoding)
			<< std::endl;
	}

	return 0;

}



int parse_h264_header(  const CAVCCHeader & h
					  , const sample_stream_info ms
					  , const unsigned char * psample
					  )
{

		unsigned int consumed(0);



		while(consumed < ms.size)
		{


			unsigned int nal_size(0);

			for(uint32_t x = 0; x < h.get_nal_length_size(); x++)
			{
				nal_size = (nal_size << 8) + psample[x + consumed];
			}

			H264Nal nal;
			sei_message sei;
			nal.decode_nal(nal_size
						 , psample + consumed + h.get_nal_length_size());

						

			FixedMemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
							  , nal.decoded_rbsp_size() -1
				);

			switch(nal.get_decoded_nal_unit_type())
			{
				case NALTYPE::NONIDR_SLICE:
				case NALTYPE::A_SLICE:
				case NALTYPE::B_SLICE:
				case NALTYPE::C_SLICE:
				case NALTYPE::IDR_SLICE:
					{
						slice_header sh(
						h.get_sequence(0)
						, h.get_picture(0)
						, nal.get_decoded_nal_unit_type()
						, nal.get_decoded_nal_ref_idc());


						uint32_t pos = static_cast<uint32_t>(mem.getpos());

						sh.get(mem);

						uint32_t hsize = static_cast<uint32_t>(mem.getpos()) - pos;


						std::wcout
						<< hsize
						<< _T("\t") << ms.sample_number
						<< _T("\t") << nal.get_decoded_nal_unit_type() 
						<< _T("\t") << nal.get_decoded_nal_ref_idc() 
						<< _T("\t") << nal.decoded_rbsp_size()
						<< _T("\t") << HNS(ms.composition_time)
						<< _T("\t") << HNS(ms.decoding_time)
						<< _T("\t") << sh.frame_num
						<< _T("\t") << sh.pic_order_cnt_lsb 
						<< _T("\t") << ((sh._p_mark && sh._p_mark->memory_management_control_operation)?1:0)
						<< _T("\t") << sh.frame_mbs_only_flag 
						<< _T("\t") << sh.first_mb_in_slice 
						<< _T("\t") << sh.slice_type 
						<< _T("\t") << sh.pic_parameter_set_id 
						<< _T("\t") << sh.idr_pic_id 
						<< _T("\t") << sh.delta_pic_order_cnt_bottom 
						<< _T("\t") << sh.redundant_pic_cnt 
						<< _T("\t") << sh.num_ref_idx_l0_active_minus1 
						<< _T("\t") << sh.num_ref_idx_l1_active_minus1 
						<< _T("\t") << sh.cabac_init_idc 
						<< _T("\t") << sh.slice_qp_delta 
						<< _T("\t") << sh.slice_qs_delta 
						<< _T("\t") << sh.disable_deblocking_filter_idc 
						<< _T("\t") << sh.slice_alpha_c0_offset_div2 
						<< _T("\t") << sh.slice_beta_offset_div2 
						<< _T("\t") << sh.slice_group_change_cycle 
						<< _T("\t") << sh.field_pic_flag 
						<< _T("\t") << sh.bottom_field_flag 
						<< _T("\t") << sh.direct_spatial_mv_pred_flag 
						<< _T("\t") << sh.num_ref_idx_active_override_flag 
						<< _T("\t") << sh.sp_for_switch_flag 
						<< std::endl
						;

					}
					break;

				case NALTYPE::SEINAL:
						sei.get(mem);

						std::wcout 
							<< "SEI: "
							<< sei.payloadType
							<< " "
							<< sei.payloadSize
						    << std::endl;

							if(1 == sei.payloadType)
							{
								pic_timing pic;
								pic.get(mem, &h.get_sequence(0));
								
								std::wcout 
							    << "pic_type: "
								<< pic.pic_struct
								<< std::endl;

								for(uint32_t i = 0; i < pic.NumClockTS; i++)
									if(pic.clock_timestamp_flag[i])
										   std::wcout 
										   << "[" << i << "] "
										   << pic.ct_type[i]
										   << " "
										   << std::endl;
								

							}

						   
						break;

				default:
					std::wcout 
						<< "Unparsed Nal: "
						<< nal.get_decoded_nal_unit_type()
						<< std::endl;
			}

            //nal size prefix
			consumed += (nal_size + 4);

		}//while

		return 0;

}

int h264_frame_list(uint64_t start, uint64_t end
			, CMP4 &mp4)
{
	
	MP4Reader reader;
	reader.parse(mp4);

	std::wcout << _T("Duration ") << HNS(reader.get_duration()) << std::endl;

	if(0 == end)
		end = reader.get_duration();

		unsigned int video(UINT32_MAX);
	
	for(int i = 0; i < reader.stream_count(); i++)
	{
		if(reader.IsVisual(i))
		{
			video = i;
			break;
		}
	}

	if(UINT32_MAX == video)
	{
		std::wcerr << _T("Cannot find video stream") << std::endl;
		return 2;
	}

	if(reader.get_visual_entry(0, video).get_entry_type() != box_avc1)
	{
		std::wcerr << _T("video stream is not avcC") << std::endl;
		return 2;
	}

	const CAVCCHeader & h = reader.get_visual_entry(0, video).get_avcc_header();

	sample_stream_info ms;
	ms.composition_time = 0;


	for(uint32_t idx = 0; idx < h.get_sequence_count(); idx++)
		output_sequence(h.get_nal_sequence(idx), h.get_sequence_size(idx), idx);


	std::wcout << std::endl;

	reader.move(start);


   	std::wcout
			            << _T("size")
						<< _T("\tsample") 
						<< _T("\tNAL") 
						<< _T("\tdecoded_nal_ref_idc") 
						<< _T("\tsize") 
						<< _T("\tcomposition")
						<< _T("\tdecoding")
						<< _T("\tframe_num") 
						<< _T("\tpic_order_cnt_lsb")
						<< _T("\tmemory_management_control_operation")  
						<< _T("\tframe_mbs_only_flag: ") 
						<< _T("\tfirst_mb_in_slice: ")
						<< _T("\tslice_type: ") 
						<< _T("\tpic_parameter_set_id: ") 
						<< _T("\tidr_pic_id: ") 
						<< _T("\tdelta_pic_order_cnt_bottom: ") 
						<< _T("\tredundant_pic_cnt: ") 
						<< _T("\tnum_ref_idx_l0_active_minus1: ") 
						<< _T("\tnum_ref_idx_l1_active_minus1: ") 
						<< _T("\tcabac_init_idc: ") 
						<< _T("\tslice_qp_delta: ")
						<< _T("\tslice_qs_delta: ") 
						<< _T("\tdisable_deblocking_filter_idc: ") 
						<< _T("\tslice_alpha_c0_offset_div2: ") 
						<< _T("\tslice_beta_offset_div2: ")  
						<< _T("\tslice_group_change_cycle: ")  
						<< _T("\tfield_pic_flag: ") 
						<< _T("\tbottom_field_flag: ") 
						<< _T("\tdirect_spatial_mv_pred_flag: ") 
						<< _T("\tnum_ref_idx_active_override_flag: ") 
						<< _T("\tsp_for_switch_flag: ") 
						<< std::endl
						;
	

	while(ms.composition_time < static_cast<int64_t>(end))
	{
		if(!reader.next_file_chunks(ms))
			return 0;

		if(video != ms.stream)
			continue;
		
		CBuffer<BYTE>  b(static_cast<uint32_t>(ms.size));
			
		mp4.set_position(ms.offset);
		mp4.read_bytes(b.get(), ms.size);

		int ret = parse_h264_header( h
					  , ms
					  , b.get()
					  );

/*
		unsigned int consumed(0);

		while(consumed < ms.size)
		{


			unsigned int nal_size(0);

			for(int x = 0; x < h.get_nal_length_size(); x++)
			{
				nal_size = (nal_size << 8) + b.getAt(x + consumed);
			}

			H264Nal nal;
			sei_message sei;
			nal.decode_nal(nal_size
						 , b.get() + consumed + h.get_nal_length_size());

			//std::wcout << _T("NAL: ") << nal.get_decoded_nal_unit_type() <<
			//	_T(" " << nal.get_decoded_nal_ref_idc() << _T(" size: ")) << nal.decoded_rbsp_size() << std::endl;

			

			MemoryBitstream mem(nal.decoded_rbsp()     + 1 //nal header 
							  , nal.decoded_rbsp_size() -1
				);

			switch(nal.get_decoded_nal_unit_type())
			{
				case NALTYPE::NONIDR_SLICE:
				case NALTYPE::A_SLICE:
				case NALTYPE::B_SLICE:
				case NALTYPE::C_SLICE:
				case NALTYPE::IDR_SLICE:
					{
						slice_header sh(
						h.get_sequence(0)
						, h.get_picture(0)
						, nal.get_decoded_nal_unit_type()
						, nal.get_decoded_nal_ref_idc());

						sh.get(mem);

						std::wcout
						<< _T("NAL\t") << nal.get_decoded_nal_unit_type() 
						<< _T("\t") << nal.get_decoded_nal_ref_idc() 
						<< _T("\tsize\t") 
						<< nal.decoded_rbsp_size()
						<< _T("\t")
						<< HNS(ms.composition_time)
						<< _T("\t")
						<< HNS(ms.decoding_time)
						<< _T("\tframe_num\t") << sh.frame_num
						<< _T("\tpic_order_cnt_lsb\t") << sh.pic_order_cnt_lsb 
						<< _T("\tmemory_management_control_operation\t")  
						<< ((sh._p_mark && sh._p_mark->memory_management_control_operation)?1:0)
						<< _T("\tframe_mbs_only_flag: ") << sh.frame_mbs_only_flag 
						<< _T("\tfirst_mb_in_slice: ") << sh.first_mb_in_slice 
						<< _T("\tslice_type: ") << sh.slice_type 
						<< _T("\tpic_parameter_set_id: ") << sh.pic_parameter_set_id 
						<< _T("\tidr_pic_id: ") << sh.idr_pic_id 
						<< _T("\tdelta_pic_order_cnt_bottom: ") << sh.delta_pic_order_cnt_bottom 
						<< _T("\tredundant_pic_cnt: ") << sh.redundant_pic_cnt 
						<< _T("\tnum_ref_idx_l0_active_minus1: ") << sh.num_ref_idx_l0_active_minus1 
						<< _T("\tnum_ref_idx_l1_active_minus1: ") << sh.num_ref_idx_l1_active_minus1 
						<< _T("\tcabac_init_idc: ") << sh.cabac_init_idc 
						<< _T("\tslice_qp_delta: ") << sh.slice_qp_delta 
						<< _T("\tslice_qs_delta: ") << sh.slice_qs_delta 
						<< _T("\tdisable_deblocking_filter_idc: ") << sh.disable_deblocking_filter_idc 
						<< _T("\tslice_alpha_c0_offset_div2: ") << sh.slice_alpha_c0_offset_div2 
						<< _T("\tslice_beta_offset_div2: ") << sh.slice_beta_offset_div2 
						<< _T("\tslice_group_change_cycle: ") << sh.slice_group_change_cycle 
						<< _T("\tfield_pic_flag: ") << sh.field_pic_flag 
						<< _T("\tbottom_field_flag: ") << sh.bottom_field_flag 
						<< _T("\tdirect_spatial_mv_pred_flag: ") << sh.direct_spatial_mv_pred_flag 
						<< _T("\tnum_ref_idx_active_override_flag: ") << sh.num_ref_idx_active_override_flag 
						<< _T("\tsp_for_switch_flag: ") << sh.sp_for_switch_flag 
						<< std::endl
						;

					}
					break;

				case NALTYPE::SEINAL:
						sei.get(mem);

						std::wcout 
							<< "SEI: "
							<< sei.payloadType
							<< " "
							<< sei.payloadSize
						    << std::endl;

							if(1 == sei.payloadType)
							{
								pic_timing pic;
								pic.get(mem, &h.get_sequence(0));
								
								std::wcout 
							    << "pic_type: "
								<< pic.pic_struct
								<< std::endl;

								for(int i = 0; i < pic.NumClockTS; i++)
									if(pic.clock_timestamp_flag[i])
										   std::wcout 
										   << "[" << i << "] "
										   << pic.ct_type[i]
										   << " "
										   << std::endl;
								

							}

						   
						break;

				default:
					std::wcout 
						<< "Unparsed Nal: "
						<< nal.get_decoded_nal_unit_type()
						<< std::endl;
			}

            //nal size prefix
			consumed += (nal_size + 4);

		}//while

		
		*/
		

		
	}

	return 0;
	
}

int timecode(uint64_t start, uint64_t end
			, CMP4 &mp4)
{
	MP4Reader reader;
	reader.parse(mp4);

	WMT_TIMECODE_EXTENSION_DATA TD;

	std::wcout << _T("Duration ") << HNS(reader.get_duration()) << std::endl;

	int ltc_stream = -1;

	for(int i = 0; i < reader.stream_count(); i++)
	{
		if(reader.IsLTC(i))
		{
			ltc_stream = i;
			break;
		}
	}

	if(0 > ltc_stream)
	{
		std::wcout << _T("Cannot Find a LTC custom stream") << std::endl;
		return 2;
	}

	std::wcout << _T("Found a LTC custom stream: ") 
		    << ltc_stream 
			<< _T(") offset: ")
			<< HNS(reader.get_stream_offset(ltc_stream))
			<< _T(" duratltc_streamon: ") 
			<< HNS(reader.get_duration(ltc_stream))
			<< _T(" sample duratltc_streamon: ")
			<< HNS(reader.get_sample_duration(ltc_stream))
			<< _T(" [")
			<< reader.get_sample_duration(ltc_stream)
			<< _T("]")
			<< _T(" samples count: ")
			<< reader.get_sample_count(ltc_stream)
			<< _T(" start tltc_streamme: ")
			<< HNS(reader.get_start_time(ltc_stream))
			<< _T(" end tltc_streamme: ")
			<< HNS(reader.get_end_time(ltc_stream))
			<< std::endl;


	bool work = true;

	sample_stream_info ms;

	while(work)
	{
		work = reader.next_file_chunks(ms);

		if(ltc_stream != ms.stream)
			continue;

		mp4.set_position(ms.offset);
		mp4.read_bytes(reinterpret_cast<BYTE*>(&TD), sizeof(WMT_TIMECODE_EXTENSION_DATA));

		//0x%x", wmdata->dwTimecode

		std::wcout 
			<< std::hex
			<< TD.dwTimecode
			<< std::dec
			<< _T("\t")
			<< ms.stream
			<< _T("\t")
			<< ms.sample_number
		    << _T("\t")
			<< HNS(ms.composition_time + reader.get_stream_offset(ms.stream))
			<< _T("\t")
			<< HNS(ms.decoding_time)
			<< _T("\t")
			<< HNS(ms.duration)
			<< _T("\t")
			<< ms.offset
			<< _T("\t")
			<< ms.size
			<< std::endl;

		if(work)
			work = (ms.decoding_time < static_cast<int64_t>(end)) || (0 == end);

	}

	
	
	return 0;

}



int timecode2(uint64_t start, uint64_t end
			, CMP4 &mp4)
{
	MP4Reader reader;

	Ctime begin;

	reader.parse(mp4);

	Ctime headers;

	int64_t ms = headers.TotalHNano() - begin.TotalHNano();
			std::wcout 
				<< _T("MP4 HEADER READING: ") 
				<< HNS(ms)
				<< std::endl;

	wmt_timecode tc;

	std::wcout << _T("Duration ") << HNS(reader.get_duration()) << std::endl;

	int ltc_stream = -1;

	for(int i = 0; i < reader.stream_count(); i++)
	{
		if(reader.IsLTC(i))
		{
			ltc_stream = i;
			break;
		}
	}

	if(0 > ltc_stream)
	{
		std::wcout << _T("Cannot Find a LTC custom stream") << std::endl;
		return 2;
	}

	std::wcout << _T("Found a LTC custom stream: ") 
		       << ltc_stream 
               << _T(" samples count: ")
			   << reader.get_sample_count(ltc_stream);

    if(reader.get_sample_count(ltc_stream))
	{

	         std::wcout 
			<< _T(") offset: ")
			<< HNS(reader.get_stream_offset(ltc_stream))
			<< _T(" stream duration: ") 
			<< HNS(reader.get_duration(ltc_stream))
			<< _T(" sample duration: ")
			<< HNS(reader.get_sample_duration(ltc_stream))
			<< _T(" [")
			<< reader.get_sample_duration(ltc_stream)
			<< _T("]")
			
			<< _T(" start time: ")
			<< HNS(reader.get_start_time(ltc_stream))
			<< _T(" end time: ")
			<< HNS(reader.get_end_time(ltc_stream))
			;
	}
	
	std::wcout << std::endl;

	tc = reader.get_interpolated_time_code(start, mp4);

	std::wcout 
		    << _T("STARTING:\t")
			<< std::hex
			<< tc.dwTimecode
			<< std::dec
			<< _T("\t")
			<< HNS(start)
			<< _T("\t")
			<< std::hex
			<< tc.dwUserbits
			<< std::dec
			<< std::endl
			<< std::endl;


	if(!reader.get_sample_count(ltc_stream))
	{
		return 3;
	}


	//bool work = true;

	//sample_stream_info ms;

	uint64_t s = reader.get_composition_sample_number(start, ltc_stream);
	uint64_t e = reader.get_composition_sample_number(end,   ltc_stream);

	std::wcout 
			<< _T("\tFirst Sample: ")
			<< s
		    << _T("\t")
			<< HNS(start)
			<< _T("\t")
			<< HNS(reader.get_composition_time(s, ltc_stream))
			<< _T("\tLast Sample: ")
			<< e
		    << _T("\t")
			<< HNS(end)
			<< _T("\t")
			<< HNS(reader.get_composition_time(e, ltc_stream))
			<< std::endl;

	for(uint64_t x = s; x <= e; x++)
	{

		uint64_t p = reader.get_sample_offset(x, ltc_stream);
		uint64_t t = reader.get_composition_time(x, ltc_stream);
		//uint64_t t = reader.get
			//(x, ltc_stream);
	

		mp4.set_position(p);
		//mp4.read_bytes(reinterpret_cast<BYTE*>(&TD), sizeof(WMT_TIMECODE_EXTENSION_DATA));

		mp4.read_box(tc);


		//0x%x", wmdata->dwTimecode

		std::wcout 
			<< std::hex
			<< tc.dwTimecode
			<< std::dec
			<< _T("\t")
			<< x
		    << _T("\t")
		    << _T("\t")
			<< HNS(t + reader.get_stream_offset(ltc_stream))
			<< _T("\t")
			<< std::hex
			<< tc.dwUserbits
			<< _T("\t")
			<< tc.dwAmFlags
            << _T("\t")
			<< tc.wRange
			<< std::dec
			<< std::endl;

		//if(t > e)
			//break;

	}

	
	
	return 0;

}



int auto_test(const STDTSTRING & test_name, console_command & c)
{

	MP4AutoTest t;

	if(test_name == _T("RangeMap"))
	{
		int ret = t.TestRangeMap();
		
		if(0 != ret)
			return ret;

		ret = t.TestRangeMap2();
		
		if(0 != ret)
			return ret;


		return ret;

	}

	if(test_name == _T("parse_date_time"))
	{
		Cstring d = c.get_value(_T("time"));

		Ctime base(d, false);

		std::wcout << static_cast<const TCHAR*>(base.ToDateString())
			       << _T(" ")
				   << static_cast<const TCHAR*>(base.ToTimeString())
				   << std::endl;

		return 0;
	}

	std::wcout << _T("invalid test name") << std::endl;

	return 12;

}


int do_mp4_mux(console_command &c, CMP4Edit & mp4edit)
{
	unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}
			

			
			if(c.command_specified(_T("maxdistance")))
			{
				uint64_t max_distance = c.get_integer64_value(_T("maxdistance"));
				mp4edit.set_max_distance(max_distance);

				std::wcout << _T("SETTING MAX DISTANCE ") << HNS(max_distance) << std::endl;
			}

			if(c.command_specified(_T("usedecodingdistance")))
			{
				bool usedecodingdistance = c.get_value(_T("usedecodingdistance"));
				mp4edit.set_use_composition_in_distance(!usedecodingdistance);

				std::wcout << _T("SETTING DECODING DISTANCE ") << usedecodingdistance << std::endl;
			}

			if(c.command_specified(_T("audiotimescale")))
			{
				uint64_t audiotimescale = c.get_value(_T("audiotimescale"));
				mp4edit.set_stream_time_scale(audiotimescale);

				std::wcout << _T("SETTING AUDIO TIMESCALE ") << audiotimescale << std::endl;
			}
			

			if(c.command_specified(_T("ctts")))
			{
				bool use_ctts = false;
				use_ctts = c.get_value(_T("ctts"));

				mp4edit.set_ctts_offset(use_ctts);
			}

			mp4edit.start(c.get_value(_T("output")));
 			
			std::wcout << _T("Process Body") << std::endl;
	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				std::wcout << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< std::endl;

				Ctime start_time;

				mp4edit.Add(c.get_value(_T("input"), idx)
				    , c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					); 

				Ctime now;

				uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

				std::wcout << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();	

			return 0;
}




double normalize_time(double t, double time_base)
{
		return 10000000UL / time_base * t;
}

uint64_t normalize_time(uint64_t t, uint64_t time_base)
{
	return static_cast<uint64_t>(normalize_time(static_cast<double>(t), static_cast<double>(time_base)));
}

void counter(unsigned char *cbuf)
{
	CMP4Fragment::counter(cbuf);
}

int decrypt(CBuffer<unsigned char> & buffer, CBuffer<unsigned char> & decrypt, unsigned int size
			, int sample_number
			, const TrackEncryptionBox & tenc
			, const SampleEncryptionBox & senc
			, const unsigned char * key)
			//, aes_encrypt_ctx ctx[1])
{

#ifdef HAVE_LIBGYPAES

	aes_encrypt_ctx ctx[1];

	::memset(ctx, 0, sizeof(aes_encrypt_ctx));

	AES_RETURN ret = aes_encrypt_key(key, BLOCK_LEN, ctx);
		_ASSERTE(0 == ret);

	decrypt.prepare(size);

	unsigned int written(0);
	unsigned int read(0);

	unsigned char * pDecrypt = decrypt.get();
	unsigned char * pSource  = buffer.get();

	unsigned char iv[BLOCK_LEN];

	::memset(iv, 0, BLOCK_LEN);
	::memcpy(iv, &senc._samples[sample_number]->init_vector->InitializationVector_0, 8); //TODO: size of iv

	if(senc._samples[sample_number]->subsample_count)
	{
		for(uint32_t i = 0; i < senc._samples[sample_number]->subsample_count; i++)
		{
			if(senc._samples[sample_number]->_sub_samples[i]->BytesOfClearData)
			{
				::memcpy(pDecrypt + written, pSource + read
					, senc._samples[sample_number]->_sub_samples[i]->BytesOfClearData);

				written += senc._samples[sample_number]->_sub_samples[i]->BytesOfClearData;
				read    += senc._samples[sample_number]->_sub_samples[i]->BytesOfClearData;
			}

			if(senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData)
			{
				//_ASSERTE(0 == (senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData % 16));

				/*
				AES_RETURN ret = aes_ctr_decrypt(pSource + read, pDecrypt + written
					, senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData
					, iv, ctx);
				*/

				//aes_ctr_crypt(const unsigned char *ibuf, unsigned char *obuf,
				//int len, unsigned char *cbuf, cbuf_inc ctr_inc, aes_encrypt_ctx ctx[1])
				AES_RETURN ret = aes_ctr_decrypt(pSource + read, pDecrypt + written
					, senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData
					, iv, counter, ctx);

				_ASSERTE(0 == ret);

				written += senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData;
				read    += senc._samples[sample_number]->_sub_samples[i]->BytesOfProtectedData;

			}
		}

		_ASSERTE(written == read && read == size);
	}
	else
	{
		//decrypt full sample

		AES_RETURN ret = aes_ctr_decrypt(pSource, pDecrypt
					, size
					, iv, counter, ctx);

		_ASSERTE(0 == ret);

	}

#endif

	return 0;
}


int do_moof_mux(console_command &c)
{
	
	std::vector<unsigned int> scount;
	std::vector<bool>        astream;

	std::vector<uint64_t>  time_base;

	//std::vector<uint64_t> time_base;

	Cstring n = _T("S");

	bool encrypted = false;

	unsigned char key[BLOCK_LEN];

	//aes_decrypt_ctx ctx[1];

#ifdef HAVE_LIBGYPAES

	if(c.command_specified(_T("key")))
	{
		Cstring k = c.get_value(_T("key"));

		_ASSERTE(k.size() == (2*BLOCK_LEN));

		k.extract_binary_hex(key, BLOCK_LEN);

		aes_init();
	
		//AES_RETURN ret = aes_decrypt_key(key, BLOCK_LEN, ctx);
		
		//_ASSERTE(0 == ret);
	}

#endif

	unsigned int count = 0;

	for(int i = 1; i < 4; i++)
	{
		Cstring nn = n.clone();

		nn += i;

		unsigned int cc = c.get_command_count(nn);

		if(cc)
		{
			scount.push_back(cc);
			if(cc > count)
				count = cc;
		}
		else
		{
			break;
		}
	}	
	
	MP4Mux mp4mux;

	//add stream description
	for(uint32_t i = 0; i < scount.size(); i++)
	{
		Cstring nn = n.clone();
		nn += (i + 1);

		Cstring file = c.get_value(nn, 0);

		std::wcout << _T("Adding Stream ") 
			   << static_cast<const TCHAR*>(nn)	
			   << _T(":\t")
			   << static_cast<const TCHAR*>(file) 
			   << std::endl;
		
		SYNCMP4File mp4;
		mp4.open(file);

		MP4ReaderConsole reader;
		reader.parse(mp4);
		
		for(int k = 0 ; k < reader.stream_count(); k++)
		{
			if(reader.IsVisual(k))
			{				
				const ALX::CMP4VisualEntry & ve = reader.get_visual_entry(0, k);

				_ASSERTE(1 == ve.get_avcc_header().get_picture_count());
				_ASSERTE(1 == ve.get_avcc_header().get_sequence_count());

				int idx =
				mp4mux.add_visual_stream(ve.get_avcc_header().get_nal_sequence()
					, ve.get_avcc_header().get_sequence_size()
					, ve.get_avcc_header().get_nal_picture()
					, ve.get_avcc_header().get_picture_size()
					, reader.get_media_time_scale(k)
					, 0
					, 0
					);

				_ASSERTE(idx == k);

				mp4mux.set_auto_decoding_time(idx, true);

				astream.push_back(false);

				time_base.push_back( reader.get_media_time_scale(k) );
			}

			if(reader.IsSound(k))
			{
				const ALX::CMP4AudioEntry & ae = reader.get_audio_entry(0, k);

				//_ASSERTE(1 == ae.get

				int idx =
				mp4mux.add_audio_stream(ae.get_aac_info().object_type
					, ae.get_aac_info().sample_rate
					, ae.get_aac_info().channels
					, 128000
					, 0); //ae.get_aac_info().

				_ASSERTE(idx == k);

				astream.push_back(true);

				time_base.push_back( reader.get_media_time_scale(k) );
			}
		}

	}

	
	mp4mux.start(c.get_value(_T("output")));

	CBuffer<unsigned char> buffer(1024);
	CBuffer<unsigned char> decrypted(1024);

	for(unsigned int i = 0; i < count; i++)
	{
		for(unsigned int k = 0; k < scount.size(); k++)
		{
			Cstring nn  = n.clone();
	                nn += (k + 1);

			if(scount[k] > i)
			{
				SYNCMP4File mp4;
		                 mp4.open(c.get_value(nn, i));

				MOOFReaderConsole reader;
				           reader.parse(mp4, false);

			   uint64_t mp4pos = mp4.get_position();

			   while(reader.get_mdat_position())
			   {

			   if(c.command_specified(_T("key")) && reader.has_senc())
				   encrypted = true;
			   else
				   encrypted = false;


			   if(!reader.get_mdat_position())
				   continue;


				uint64_t position		= reader.get_base_offset();
						 int64_t presentation	= reader.TrackFragmentBaseMediaDecode();
						 int64_t decoding		= reader.TrackFragmentBaseMediaDecode();
						 int64_t duration		= 0;
				uint32_t size           = 0;


				 std::wcout << _T("Adding Samples ") 
					   << static_cast<const TCHAR*>(nn)	
					   << _T(":\t")
					   << static_cast<const TCHAR*>(c.get_value(nn, i)) 
					   << _T("\tposition:\t")
					   << position
					   << _T("\tpresentation:\t")
					   << presentation
					   << std::endl;

				 bool audio = astream[k];

				 uint64_t tbase = time_base[k];

				  media_sample sample;

				  for(unsigned int s = 0; s < reader.get_trun_sample_count(); s++)
				  {
					 size = static_cast<uint32_t>(reader.get_sample_size(s));

					 int64_t comp = reader.get_sample_composition_time_offset(s, true);

					 sample.bIsSyncPoint = (s == 0 || audio);
					 sample.composition_time = presentation + comp; //ssf has always used signed in spite of specification
					 sample.decoding_time = decoding;
					 duration = reader.get_sample_duration(s);
					 sample.duration = duration;
					 sample.discontinuity = 0;

					 if(tbase != 10000000UL)
					 {
						 sample.composition_time = normalize_time(sample.composition_time, tbase);
						 sample.decoding_time    = normalize_time(sample.decoding_time, tbase);
						 sample.duration         = normalize_time(sample.duration, tbase);
					 }

					 /*
					 ALX_TRACE8(_T("SAMPLE\t%hd\t%s\t%s\t[%s]\t%s\t%s\t%I64\t%d")
						 , pin_idx
						 , HNS(comp)
						 , HNS(presentation_time)
						 , HNS(sample.composition_time)
						 , HNS(sample.decoding_time)
						 , HNS(sample.duration)
						 , size
						 , sample.bIsSyncPoint);
					 */

					 mp4.set_position(position);

					 buffer.prepare(size);

					 mp4.read_bytes(buffer.get(), size);

					 
					 if(!encrypted)
						mp4mux.add_sample(k, buffer.get(), size, sample.bIsSyncPoint, sample.composition_time, sample.decoding_time, sample.duration);
					 else
					 {
						 decrypted.prepare(size);
						 decrypt(buffer, decrypted, size, s, reader.get_tenc(), reader.get_senc(), key);

						 mp4mux.add_sample(k, decrypted.get(), size, sample.bIsSyncPoint, sample.composition_time, sample.decoding_time, sample.duration);
						 
					 }
					 
					 position += size;
					 decoding += duration;

					 presentation = decoding;
				  }

				  mp4.set_position(mp4pos);
				  reader.parse(mp4, false);

				  mp4pos = mp4.get_position();

			   }


			}
		}
	}

	mp4mux.end();

	return 0;

}

int produce_m4f_initialization
(
	  int track_id
    , unsigned int codec_type
	, const TCHAR* mp4_init_file_out
	, unsigned char * codec_private_data
	, unsigned int size
	, const unsigned int target_bit_rate
	, uint64_t duration   = 0
	, uint64_t time_scale = 10000000
		, unsigned char	* pKid        = NULL
		, const unsigned char * ppssh       = NULL
		, unsigned int     pssh_size  = 0
)
{

	SYNCMP4WriteFile   body;
	                body.open(mp4_init_file_out, false);

	int ret = CMP4Fragment::produce_m4f_initialization(body
						, track_id
						, codec_type
						, codec_private_data
						, size
						, target_bit_rate
						, duration
						, time_scale
						, pKid
						, ppssh
						, pssh_size
						);

	body.close();

	return ret;

	/*
	MP4Write        write;

	int stream_id(-1);

	if(STREAM_TYPE_VIDEO_H264 == codec_type)
			stream_id = write.add_visual_stream(codec_private_data, size, time_scale);

	
	if(STREAM_TYPE_AUDIO_AAC == codec_type)
		stream_id = write.add_audio_stream(codec_private_data, size, target_bit_rate, time_scale);

	write.set_first_track_id(track_id - 1);
	write.set_stream_track_id(track_id -1 , stream_id);
	write.set_allow_empty_stream(true);

	//write.set_

	_ASSERTE(0 == stream_id);
	

	//write.set_ftyp(ftyp_DASH, 0);

	write.set_ftyp(BOX( 'c', 'c', 'f', 'f' ), 1);
	write.add_brand(ftyp_ISO6);
	write.set_mvhd_timescale(time_scale);
	write.set_mvhd_version(1);
	//write.add_brand(ftyp_avc1);
	//write.add_brand(ftyp_mp41);


	write.write_ftyp(body);
		
		write.write_moov(body, false);
				body.open_box(box_MVEX);

					MovieExtendsHeaderBox mehd;
					                      mehd.set_fragment_duration(duration);

										  body.write_child_box(box_MEHD, mehd);

					TrackExtendsBox trex;
					                trex.set_flags(0);
									trex.set_version(0);
									trex.track_ID = track_id;
									trex.default_sample_description_index = 1;
									trex.default_sample_duration = 0;
									trex.default_sample_size = 0;
									trex.default_sample_flags= 0;

									body.write_child_box(box_TREX, trex);

				body.close_box(box_MVEX);
		write.close_moov(body);

	write.end();

	body.Close();

	return 0;
	*/
	
}

int produce_m4f_chunk_cross(  int stream_id
					  , int track_id
					  , const TCHAR* mp4_input_file
					  , uint64_t start_time
					  , uint64_t end_time
					  , const TCHAR* mp4_input_file_2
					  , uint64_t start_time_2
					  , uint64_t end_time_2
					  , const TCHAR* m4f_output_file
					  , int sequence
					  , bool AVCNBOX
					  , uint64_t base_media_decode_time
					  , IMP4ReaderCallback * p_callback
					  , unsigned char * p_key = NULL
					  , DWORD senc_flags = 0
					  )
{
	std::wcout << track_id
		       << _T("\t")
			   << mp4_input_file
			   << _T("\t")
			   << HNS(start_time)
			   << _T("\t")
			   << HNS(end_time)
			   << _T("\t")
			   << mp4_input_file_2
			   << _T("\t")
			   << HNS(start_time_2)
			   << _T("\t")
			   << HNS(end_time_2)
			   << _T("\t")
			   << m4f_output_file
			   << std::endl;

	SYNCMP4WriteFile file;
	              file.open(m4f_output_file, false);


	SYNCMP4File mp4;
	        mp4.open(mp4_input_file);

	SYNCMP4File mp4_2;
	        mp4_2.open(mp4_input_file_2);


	int ret = CMP4Fragment::produce_m4f_chunk_cross(file
				, mp4
				, mp4_2
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
				, senc_flags
				);

	file.close();

	return ret;
}

int produce_m4f_chunk(  int stream_id
					  , int track_id
					  , const TCHAR* mp4_input_file
					  , uint64_t start_time
					  , uint64_t end_time
					  , const TCHAR* m4f_output_file
					  , int sequence
					  , bool AVCNBOX
					  , uint64_t base_media_decode_time
					  , IMP4ReaderCallback * p_callback
					  , unsigned char * p_key
					  , DWORD senc_flags = 0
					  )
{

	std::wcout << track_id
		       << _T("\t")
			   << mp4_input_file
			   << _T("\t")
			   << HNS(start_time)
			   << _T("\t")
			   << HNS(end_time)
			   << _T("\t")
			   << m4f_output_file
			   << std::endl;

	SYNCMP4WriteFile file;
	              file.open(m4f_output_file, false);


	SYNCMP4File mp4;
	        mp4.open(mp4_input_file);

	int ret = CMP4Fragment::produce_m4f_chunk(file
				, mp4
				, stream_id
				, track_id
				, start_time
				, end_time
				, sequence
				, AVCNBOX
				, base_media_decode_time
				, p_callback
				, p_key
				, senc_flags
				);

	file.close();

	return ret;
/*	
	MP4Reader reader;
		      reader.parse(mp4);

			  reader.move(start_time, stream_id);

			  sample_stream_info sample;

			  bool is_audio = reader.IsSound(stream_id);
		  

	CBuffer<unsigned char> body(1024);

	CMP4Fragment m4f;

			m4f.start(track_id, sequence, ((AVCNBOX)?0xa600000:0));

				 //uint64_t first_difference = UINT64_MAX;

				 //uint64_t ctts_difference

				 while(reader.next_file_chunks(sample))
				 {
					 if(sample.stream == stream_id)
					 {
						 if(sample.composition_time >= end_time)
							 break;

						 body.prepare(sample.size);

						 mp4.set_position(sample.offset);
						 mp4.read_bytes(body.get(), sample.size);

						 uint64_t composition_time = sample.composition_time;
						          int64_t decoding_time    = sample.decoding_time;

					    
						 if(reader.stream_ctts_offset(sample.stream))
						 {
							decoding_time += reader.ctts_stream_offset(sample.stream);
						 }

						 //if(is_audio)
						 //	 composition_time = sample.decoding_time;

						 //_ASSERTE(composition_time >= start_time);

						 if(composition_time < start_time)
						 {
							 //_ASSERTE(composition_time < start_time);
							 std::wcout << _T("INVALID SEQUENCE: ")
								        << mp4_input_file
										<< _T("\t")
										<< HNS(start_time)
										<< _T("\t")
										<< HNS(composition_time)
										<< _T("\t")
										<< sample.sample_number
										<< _T("\t")
										<< stream_id
										<< _T("\t")
										<< track_id
										<< std::endl;

							 //return 2;
						 }
						 else
						 {

						 //if(is_audio)
						 //	composition_time -= start_time;
						 //	decoding_time    -= start_time;

						 

							 m4f.add_sample(sample.stream
							 , body.get()
							 , sample.size
							 , sample.bIsSyncPoint
							 , composition_time
							 , decoding_time
							 , sample.duration);

							 if(!m4f.has_baseMediaDecodeTime())
							 {
								 _ASSERTE(start_time >= (composition_time - decoding_time));
								 m4f.set_baseMediaDecodeTime(start_time - (composition_time - decoding_time));
							 }
						 }//if(composition_time < start_time)
					 }
				 }


				 if(AVCNBOX)
				 {
					 const CMP4VisualEntry & ve = reader.get_visual_entry(0, stream_id);
					 m4f.add_avcn_box(ve.get_body(), ve.get_body_size());
				 }


				 m4f.end(file);

*/	 
				 

	return 0;
}


int _tmain(int argc, TCHAR* argv[])
{
	
	console_command c;

	c.add(_T("input"), console_command::type_string , false
		, _T("input file name"), 'i');
	c.add(_T("kind"),  console_command::type_string , false
		, _T("kind of execution could be: \n\r \
					pick  \n\r \
					gop  \n\r \
                    dash  \n\r \
	    "), 'k');
 
/*
			        edit \n\r \
					mux  \n\r \
					H264Header  \n\r \
					xml  \n\r \
					tfra  \n\r \
					ltc  \n\r \
					test  \n\r \
					ssf  \n\r \
					discrete  \n\r \
					analyze  \n\r \
					simul  \n\r \
					frame  \n\r \
*/


	c.add(_T("position"),  console_command::type_int , false
		, _T("file position"), 'p');
	c.add(_T("xml")     ,  console_command::type_string , false
		, _T("xml"), 'x');
	c.add(_T("handler") ,  console_command::type_string , false
		, _T("handler"), 'h');
	c.add(_T("size")    ,  console_command::type_int , false
		, _T("size"), 'z');
	c.add(_T("output"), console_command::type_string , false
		, _T("output file"), 'o');
	c.add(_T("starttime"),  console_command::type_int, false
		, _T("starttime"), 's');
	c.add(_T("endtime"), console_command::type_int, false
		, _T("end time"), 'e');
	c.add(_T("stream"), console_command::type_int , false
		, _T("target stream"));
	c.add(_T("test"), console_command::type_string , false
		, _T("target test name"), 't');
	c.add(_T("help"), console_command::type_string , false
		, _T("Display Help"), 'h');
	c.add(_T("-help"), console_command::type_string, false
		, _T("Display Help"));
	c.add(_T("segment"), console_command::type_int , false
		, _T("segment duration"));
	c.add(_T("bitrate"), console_command::type_int , false
		, _T("bitrate"), 'b');
	c.add(_T("path")   , console_command::type_string , false
		, _T("path"), 'j');
	c.add(_T("audiotimescale")   , console_command::type_int , false
		, _T("audio time scale"));
	c.add(_T("validate"), console_command::type_bool, false
		, _T("validate input in debug build"), 'v');

	bool r = c.process(argc, argv);

	if(!r)
	{
		std::wcerr << 
			static_cast<const TCHAR*>(c.get_error_message())
			<< std::endl;

		std::wcout << static_cast<const TCHAR*>(c.get_help())
			<< std::endl;

		return 1;
	}

	if(c.command_specified(_T("help"))
		|| c.command_specified(_T("-help"))
	)
	{
		std::cout << std::endl
			<< "USAGE mg -k:kind -i:[input file path] options"
			<< std::endl
			<< std::endl
			<< "kind is the subcommand should be adaptive, dash, hls, gop or analyze."
			<< std::endl
			<< std::endl
			<< "DASH/HLS/ADAPTIVE: the dash kind will produce a static mpeg-dash fragmented version of your mp4."
			<< std::endl
            << "\t\tthe hls kind wil produce a HTTP LIVE STREAM version of your mp4."
            << std::endl
            << "\t\tthe adaptive kind will produce both a mpeg-dash and hls version of your files."
            << std::endl
			<< "\tAdd the first mp4 with this sintax: -i:[path] -s:0 -e:0 -b:[bitrate]"
			<< std::endl
			<< "\t where -i:path is the path the the file with a -i: prefix. If you want the full file just use -s:0 -e:0."
			<< std::endl
			<< "\t use -b:bitrate to specify the video bitrate of the file in kb. For instance 700kb write -b:700."
			<< std::endl
			<< "\t use -j:path -b:bitrate to add more file with the same gop structure of your original file bat different bitrate."
			<< std::endl
			<< "\t finally add -o:directory to indicate in witch directory writing all fragments."
			<< std::endl
			<< std::endl
			<< "GOP: the gop kind let you see a list of all gop in your file."
			<< std::endl
			<< "\t use this command in each of your file to check whether they are gop aligned."
			<< std::endl
			<< std::endl;
		
		
		/*
		std::wcout << static_cast<const TCHAR*>(c.get_help())
			<< std::endl;
		*/

		return 0;
	}

	try{

		STDTSTRING kind = _T("interactive");

		if(c.command_specified(_T("kind")))
			kind = static_cast<const TCHAR*>(c.get_value(_T("kind")));

		if (kind == _T("version"))
		{
			version();
			return 0;
		}
		
		if(kind == _T("throw"))
		{
			MGCHECK(-1);
			//throw std::runtime_error("file not found");
		}

		if(kind == _T("test"))
		{
			STDTSTRING test = static_cast<const TCHAR*>(c.get_value(_T("test")));
			return auto_test(test, c);
		}

		if (kind == _T("hls") 
		 || kind == _T("PES")
		 || kind == _T("all")
         || kind == _T("adaptive")
		)
		{
            bool adaptive = false;
            
            if(kind == _T("adaptive"))
            {
                adaptive = true;
                c.set_value(_T("kind"), _T("hls"), 0);
            }
			int r = tsinfo(c, std::cout);

            if(adaptive)
                kind = _T("dash");
            else
                return r;
		}

		//CResource<uvloopthread> t; t.Create(); t->start();

		SYNCMP4File mp4;

		Ctime start_time;

		if(kind != _T("moof"))
		{
			if(!c.command_specified(_T("input")))
			{
				std::wcout << "Missing Input File " << std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
				<< std::endl;

				return 2;
			}

			std::wcout << _T("Opening File ") 
				<< c.get_value(_T("input")) << std::endl;

			mp4.open(c.get_value(_T("input")));
		}
		
        

		if(kind == _T("interactive"))
		{
			interactive(mp4);
		}
		else if(kind == _T("analyze") 
			|| kind == _T("analize"))
		{
			dobox(mp4);
		}
		else if(kind == _T("testvideoindex"))
		{
			testvideoindex(mp4);
		}
		/*
		else if(kind == _T("edit") 
			|| kind == _T("edit2"))
		{

			unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}

			std::wcout << _T("Edit Operation ") << std::endl;


			MP4Write        mp4w;

			CMP4WriteMemory headers;
			                headers.Open(3000000);

			if(kind == _T("edit"))
				do_edit_header(mp4, headers, mp4w); 
			else
				do_edit_header2(mp4, headers, mp4w);

            Cstring file_out  = c.get_value(_T("output")).clone();
			        file_out += _T(".body.tmp");

			CMP4WriteFile   body;
			body.Open(file_out);

			std::wcout << _T("Process Body") << std::endl;
	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
				MP4File *p4(0);
				MP4File mp4f;

				if(0 == idx)
				{
					p4 = &mp4;
				}
				else
				{							             
					mp4f.Open(c.get_value(_T("input"), idx));
					p4 = & mp4f;
				}

				std::wcout << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< std::endl;

				do_edit(
					  c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					, *p4
					, body
					, mp4w); 
			}

			body.Close();

			
            mp4w.write_moov(headers);

			SHStream body_read(file_out);
           
			mp4w.open_mdat(headers, body_read.size());

			headers.flush();
			
			uint64_t headers_size = headers.get_size();

			mp4w.rebase_streams(headers, headers_size);

			headers.flush();
            
			mp4w.end();

			SHStreamWrite full(c.get_value(_T("output")));

			ULONG uls(0);

			full.Write(headers.get_buffer(), headers.get_size(), &uls);

			_ASSERTE(uls == headers.get_size());

			unsigned int chunk_size = 102400;

			CBuffer<BYTE> chunk(chunk_size);

			for(uint64_t x = 0; x < body_read.size();x)
			{
				ULONG ucopy(0);
				ULONG uwrite(0);

				body_read.Read(chunk.get(), chunk_size, &ucopy);
				if(ucopy)
				{
					full.Write(chunk.get(), ucopy, &uwrite);
					_ASSERTE(uwrite == ucopy);
				}

				if(ucopy < chunk_size)
					break;

				x += ucopy;
			}

			body_read.Release();

			if(!::DeleteFile(file_out))
				ALXTHROW_LASTERR;
		}
		*/
		/////////////
		
		
		else if(kind == _T("mux"))
		{
			std::wcout << _T("Mux Operation ") << std::endl;
			
			CMP4EditConsole mp4edit;

			int r = do_mp4_mux(c, mp4edit);

			if(0 != r)
				return r;
			
			
		}
		else if(kind == _T("moof"))
		{
			std::wcout << _T("Moof Operation ") << std::endl;

			int r = do_moof_mux(c);

			if(0 != r)
				return r;
		}
		

		/*
		else if(kind == _T("aacrep"))
		{
			std::wcout << _T("AAC Replace Operation ") << std::endl;

			unsigned int cnt = c.get_command_count(_T("aac_input"));
			if(cnt != c.get_command_count(_T("aac_starttime")) ||
				cnt != c.get_command_count(_T("aac_endtime")) ||
				 cnt != c.get_command_count(_T("aac_stream"))  ||
				  cnt != c.get_command_count(_T("aac_duration")) 
				  )
			{
				std::wcerr << 
					_T("ee:  aac input start and end must be specified for every input")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}
			
			MP4EditAACReplace<CMP4EditConsole> mp4edit;

			for(int i = 0; i < cnt; i++)
			{
				mp4edit.add_aac_replace(
					  c.get_integer64_value((_T("aac_starttime")), i)
					, c.get_integer64_value((_T("aac_endtime")), i)
					, c.get_integer64_value((_T("aac_duration")), i)
					, c.get_value((_T("aac_input")), i)
					, c.get_integer64_value((_T("aac_stream")), i)
					);

			}

			int r = do_mp4_mux(c, mp4edit);

			if(0 != r)
				return r;
			
			
		}
		else if(kind == _T("discrete"))
		{
			unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}

			std::wcout << _T("Mux Operation ") << std::endl;


			class ConsoleMP4Dynamic: public CMP4DynamicDiscreate
			{
				virtual void publish_segmented_composition(uint64_t composition_time,
						uint64_t computed_time,
						int stream)
				{
					std::wcout << _T("Fragment ") 
								<< HNS(composition_time)
								<< _T("\t")
								<< HNS(computed_time)
								<< _T("\t")
								<< stream
								<< std::endl;
				}

				
				virtual void publish_no_random_stream(
				int stream, const MP4Reader &reader, uint64_t start, uint64_t end)
				{
					std::wcout << _T("no_random_stream ") 
								<< HNS(start)
								<< _T("\t")
								<< HNS(end)
								<< _T("\t")
								<< stream
								<< std::endl;

					CMP4DynamicDiscreate::publish_no_random_stream(
							stream, reader, start, end);
				}
				

				virtual void begin_processing(LPCTSTR pszfile
					, int streams, MP4Reader & reader)
				{
					std::wcout << _T("processing ") 
								<< pszfile
								<< _T("\t")
								<< streams
								<< std::endl;

					for(int i = 0; i < streams; i++)
					{
						std::wcout << _T("stream bit rate " << i << _T(" ")) << reader.get_stream_bit_rate(i) << std::endl;
					}
				}

				virtual void video_private_data(const unsigned char* p_private_data, long size, int stream
					, const unsigned int width
					, const unsigned int height)
				{
					std::wcout << _T("video_private_data ") 
								<< size
								<< _T("\t")
								<< width
								<< _T("\t")
								<< height
								<< _T("\t")
								<< stream
								<< std::endl;
				}

				virtual void audio_private_data(const unsigned char* p_private_data, long size, int stream
					, const unsigned int sample_rate
					, const unsigned int channels
					, const unsigned int target_bit_rate)
				{
					std::wcout << _T("audio_private_data ") 
								<< size
								<< _T("\t")
								<< sample_rate
								<< _T("\t")
								<< channels
								<< _T("\t")
								<< target_bit_rate
								<< _T("\t")
								<< stream
								<< std::endl;
				}

				
				virtual void publish_composition(uint64_t composition_time,
					uint64_t computed_time,
					int stream)
				{
					std::wcout << _T("sample ") 
								<< HNS(composition_time)
								<< _T("\t")
								<< HNS(computed_time)
								<< _T("\t")
								<< stream
								<< std::endl;

					CMP4DynamicDiscreate::publish_composition(composition_time,
					computed_time,
					stream);
				}

			} mp4edit;

			//mp4edit.start(c.get_value(_T("output")));
 			
			std::wcout << _T("Process Body") << std::endl;

			if(c.command_specified(_T("segment")))
				mp4edit.set_fragment_length(c.get_integer64_value(_T("segment")));
	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				std::wcout << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< std::endl;

				Ctime start_time;

				mp4edit.Add(c.get_value(_T("input"), idx)
				    , c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					); 

				Ctime now;

				uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

				std::wcout << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();			
			
		}*/
		else if(kind == _T("ssf") 
			|| kind == _T("ssfx")
			|| kind == _T("dash")
			)
		{
			unsigned int cnt = c.get_command_count(_T("input"));
			if(cnt != c.get_command_count(_T("starttime")) ||
				cnt != c.get_command_count(_T("endtime")))
			{
				std::wcerr << 
					_T("ee: input start and end must be specified for every input ")
					<< "input: " 
					<< cnt
					<< " start " << c.get_command_count(_T("starttime"))
					<< " end   " << c.get_command_count(_T("endtime"))
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2020;
			}

			unsigned int npaths    = c.get_command_count(_T("path"));
			unsigned int nbitrates = c.get_command_count(_T("bitrate"));

			if(npaths + cnt != nbitrates)
			{
				std::wcerr << 
					_T("ee: input path and bitrates do not match")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2021;
			}

			if(0 != (npaths % cnt))
			{
				std::wcerr << 
					_T("ee: input and path not in proportion")
					<< std::endl;

				std::wcout << static_cast<const TCHAR*>(c.get_help())
					<< std::endl;

				return 2022;
			}

			std::wcout << _T("SSF Operation ") << std::endl;

			CMP4DynamicInfo mp4edit;
			if(c.command_specified(_T("segment")))
				mp4edit.set_fragment_length(c.get_integer64_value(_T("segment")));
 			
			std::wcout << _T("Process Body") << std::endl;

			int path_for_input = npaths / cnt;

			int path_idx    = 0;
			int bitrate_idx = 0;

			unsigned short audio_streams = 1;
		        uint64_t audio_bitrate = 96000;

			if(c.command_specified(_T("audio_streams")))
				audio_streams = c.get_integer_value(_T("audio_streams"));

			if(c.command_specified(_T("audio_bitrate")))
				audio_bitrate = c.get_integer64_value(_T("audio_bitrate"));

	        			
			for(unsigned int idx = 0; idx < cnt; idx++)
			{
								

				std::wcout << _T("Add File ") 
					<< c.get_value(_T("input"), idx)
					<< _T(" ") 
					<< c.get_integer64_value(_T("starttime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("endtime"), idx)
					<< _T(" ")
					<< c.get_integer64_value(_T("bitrate"), bitrate_idx)
					<< std::endl;


				CBuffer<dynamic_item> ditem(npaths + 1);
				                      ditem.updatePosition(npaths + 1);

				for(unsigned int pp = 0; pp <= npaths; pp++)
				{
					ditem.getAt(pp).bitrate       = c.get_integer64_value(_T("bitrate"), bitrate_idx++) * 1000UL;
					ditem.getAt(pp).audio_bitrate = audio_bitrate;
					ditem.getAt(pp).lang_count    = audio_streams; 
					
					for(unsigned int ll = 0; ll < audio_streams; ll++)
					{
						if(c.command_specified(_T("lang"), ll))
						{
							Cstring lan = c.get_value(_T("lang"), ll);
							
							const TCHAR * plan = lan;

							unsigned int l0 = plan[0];
							unsigned int l1 = plan[1];
							unsigned int l2 = plan[2];


							ditem.getAt(pp).langs[ll] = 
								((l0 << 24) & 0xFF000000)
							    ||  ((l1 << 16) & 0x00FF0000)
							    ||	((l2 << 8)  & 0x0000FF00);

								
						}
						else
						{
							if(ll > 0)
							{
								std::wcout << _T("missing language for ") << ll << std::endl;
								return 2025;
							}

							TCHAR und[4];
							      und[0] = 'U';
							      und[1] = 'N';
							      und[2] = 'D';
							      und[3] = 0x0;
						
							unsigned int l0 = und[0];
							unsigned int l1 = und[1];
							unsigned int l2 = und[2];

							ditem.getAt(pp).langs[ll] = 
								((l0 << 24) & 0xFF000000)
							    ||  ((l1 << 16) & 0x00FF0000)
							    ||	((l2 << 8)  & 0x0000FF00);
						}
					}

					if(0 == pp)
						ditem.getAt(pp).psz_path = c.get_value(_T("input"), idx);
					else
						ditem.getAt(pp).psz_path = c.get_value(_T("path"), path_idx++);
						
				}


				Ctime start_time;

				mp4edit.Add(c.get_integer64_value(_T("starttime"), idx)
					, c.get_integer64_value(_T("endtime"), idx)
					, ditem.get()
					, ditem.size()
					); 

				Ctime now;

				uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

				std::wcout << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
			}

			mp4edit.End();	


			SSFRenderer r;
			MPDRenderer d;

			bool          encrypted = false;
			bool    audio_encrypted = true;
			uint32_t        senc_flags = 0x000002;
			uint32_t        video_senc_flags = 0x000002;
			unsigned char key[BLOCK_LEN];
			unsigned char kid[BLOCK_LEN];

			unsigned char * pKid       = NULL;
			unsigned char * pKey       = NULL;
			const unsigned char * ppssh      = NULL;
			unsigned int     pssh_size = 0;
			
			std::vector<ProtectionSystemSpecificHeaderBox *>  vpssh;
			std::vector<Cstring> vContentProtection;

			//WriteMemoryBitstream bit_stream(10240);

			if(c.command_specified(_T("key")) 
				&& c.command_specified(_T("kid")))
			{
				Cstring k = c.get_value(_T("key"));

				_ASSERTE(k.size() == (2*BLOCK_LEN));

				k.extract_binary_hex(key, BLOCK_LEN);

				Cstring id = c.get_value(_T("kid"));

				_ASSERTE(id.size() == (2*BLOCK_LEN));

				id.extract_binary_hex(kid, BLOCK_LEN);

				encrypted = true;

				bool clearkey = false;

				if(c.command_specified(_T("senc_flags")))
				{
					senc_flags = c.get_integer_value(_T("senc_flags"));
				}

				if(c.command_specified(_T("audio_encrypted")))
				{
					Cstring v = c.get_value(_T("audio_encrypted"));
					audio_encrypted = v;
				}

				if(c.command_specified(_T("clearkey")))
				{
					Cstring v = c.get_value(_T("clearkey"));

					clearkey = v;
				}
				
				if(c.command_specified(_T("playreadyurl")))
				{
					PlayReadyCencProvider prp(key, kid, c.get_value(_T("playreadyurl")));

					                      prp.add_pssh(vpssh);
										  prp.add_ContentProtection(vContentProtection);
				}

				if(clearkey)
				{
					ClearKeyCencProvider ckp(kid);
										 ckp.add_pssh(vpssh);
										 ckp.add_ContentProtection(vContentProtection);

				    if(c.command_specified(_T("output")))
					{
						CstringT<char> json;
									   json += static_cast<const TCHAR*>(ckp.json_key(key));

						Cstring out_dir;

						out_dir = c.get_value(_T("output"));
				        
						Cstring json_file = out_dir.clone();
						        json_file += _T("/json.key");
				    
						/*
						CFileOut outfile(json_file, 1024, false);

						         outfile.add(reinterpret_cast<const unsigned char *>(static_cast<const char *>(json)), json.size());
					    */
								sync_file_bitstream outfile; 
								outfile.open(json_file, false);

								outfile.write(reinterpret_cast<const unsigned char *>(static_cast<const char *>(json))
									, json.size()
								);

								outfile.close();
					}
				}



				pKid = kid;
				pKey = key;
			}

			CMP4WriteMemory m;
			m.open(1024);// bit_stream.get_size());

			if(encrypted)
			{
				for(uint32_t k = 0; k < vContentProtection.size(); k++)
				{
					d.add_content_protection(vContentProtection[k]);
				}

							

				for(uint32_t k = 0; k < vpssh.size(); k++)
				{
					m.open_box(box_pssh);

					m.write_box((*vpssh[k]));

					m.close_box(box_pssh);
				}

				//m.flush();

				//bit_stream.flush();

				m.flush();

				ppssh = m.get_buffer();
				pssh_size = static_cast<uint32_t>(m.get_size());

			}
			

			mp4edit.render(&d);
			mp4edit.render(&r);
			

			//std::wcout << static_cast<const TCHAR*>(r.xml());

			Cstring out_dir;

			if(c.command_specified(_T("output")))
			{
				 out_dir = c.get_value(_T("output"));
				 Cstring out_xml_file = out_dir.clone();

				 out_xml_file += _T("/index.ism");

				 if(kind != _T("dash")) //do not output ssf for dash
				 {
					
					 sync_file_bitstream outfile;
					 outfile.open(out_xml_file, false);

					 outfile.write(reinterpret_cast<const unsigned char *>(static_cast<const char *>(r.xml()))
						 , r.xml().size() * sizeof(TCHAR)
					 );

					 outfile.close();
				 }

				 out_xml_file = out_dir.clone();

				 out_xml_file += _T("/index.mpd");

				 {
				    
					 					 
					 CstringT<char> mpd;
					                mpd += static_cast<const TCHAR*>(d.xml());

					

						sync_file_bitstream outfile; 

									outfile.open(out_xml_file, false);

						outfile.write(reinterpret_cast<const unsigned char *>(static_cast<const char *>(mpd))
										, mpd.size() * sizeof(TCHAR)
									);

									outfile.close();

				 }

				 if(kind == _T("ssfx"))
					return 0;

				 //std::map<int, int> sequence_map;

				 const DynamicPresentation & p = mp4edit.get_presentation();

				 int mpd_track_id(1);



				 CMP4FragmentValidation * p_validation = NULL;
				 bool validate = false;


#ifdef _DEBUG
				 CMP4FragmentValidation vobj;
#endif

				 if(c.command_specified(_T("validate")))
					 validate = c.get_value(_T("validate"));
				 
				 if(validate)
				 {
#ifdef _DEBUG
					 p_validation = &vobj;
#else
					 std::wcout << _T("WARNING: validate is disabled in release!") << std::endl;
#endif

				 }

				 

				 for(int i = 0; i < p.Count(); i++)
				 {
					const DynamicStream      * s = p.get_by_index(i);
					std::map<uint64_t, DynamicBitrate *>::const_iterator bitrate = s->begin();

					int track_id(-1);

					while(s->end() != bitrate)
					{

						track_id++;
						int sequence(1);

						//*****Initialization Segment//

						Cstring init_chunk_file = out_dir.clone();

							init_chunk_file += _T("/");

								if(s->is_audio())
									init_chunk_file += s->name;//_T("\\audio_");

								if(s->is_video())
									init_chunk_file += s->name;//_T("\\video_");

								init_chunk_file += _T("_");
								init_chunk_file += bitrate->first;
								init_chunk_file += _T("_");
								init_chunk_file += _T("i");
								
								if(s->is_audio())
									init_chunk_file += _T(".m4a");

								if(s->is_video())
									init_chunk_file += _T(".m4v");


						produce_m4f_initialization(
							  (s->id + 1)//mpd_track_id
							, (s->is_video())?STREAM_TYPE_VIDEO_H264:STREAM_TYPE_AUDIO_AAC
							, init_chunk_file
							, bitrate->second->CodecPrivateData()
							, bitrate->second->CodecPrivateDataSize()
							, static_cast<uint32_t>(bitrate->first)
							, s->duration()
							, 10000000
							, pKid
							, (s->is_video())?ppssh:((audio_encrypted)?ppssh:NULL)
							, pssh_size
							);

						//bitrate->second->

						//**************************//


						std::map<int64_t, int64_t>::const_iterator point = s->get_point_begin();
						while(point != s->get_point_end())
						{
							uint64_t computed_time    = point->first;
							uint64_t composition_time = point->second;

							_ASSERTE(composition_time == s->get_original_time(computed_time));

							point++;


							if(point != s->get_point_end())
							{
								//uint64_t end_time = point->second; //the following point start time

								uint64_t end_time = s->get_end_original_time(computed_time);//(composition_time);
								_ASSERTE(end_time == point->second || cnt > 1); //only for single files
								
								Cstring out_chunk_file = out_dir.clone();

								out_chunk_file += _T("/");

								if(s->is_audio())
									out_chunk_file += s->name;//_T("\\audio_");

								if(s->is_video())
									out_chunk_file += s->name;//_T("\\video_");

								out_chunk_file += _T("_");
								out_chunk_file += bitrate->first;
								out_chunk_file += _T("_");
								out_chunk_file += computed_time;
								
								if(s->is_audio())
									out_chunk_file += _T(".m4a");

								if(s->is_video())
									out_chunk_file += _T(".m4v");
								
								if(s->is_cross_point(computed_time))
								{

									CrossPoint cp = s->get_cross_point(computed_time);

									produce_m4f_chunk_cross(s->id
										, (s->id + 1)//mpd_track_id
										, s->get_path(bitrate->first, computed_time) //bitrate->second->get_path(computed_time)
										, cp.composition //composition_time
										, (cp.composition + cp.computed_time_2 - cp.computed_time)
										, s->get_path(bitrate->first, cp.computed_time_2)
										, cp.composition_2
										, end_time
										, out_chunk_file
										, sequence++
										, s->is_video()
										, computed_time
										, p_validation
										, (s->is_video())?pKey:((audio_encrypted)?pKey:NULL)
										, (s->is_video())?video_senc_flags:senc_flags
										);
								}
								else
								{
									produce_m4f_chunk(s->id
										, (s->id + 1)//mpd_track_id
										, s->get_path(bitrate->first, computed_time) //bitrate->second->get_path(computed_time)
										, composition_time
										, end_time
										, out_chunk_file
										, sequence++
										, s->is_video()
										, computed_time
										, p_validation
										, (s->is_video())?pKey:((audio_encrypted)?pKey:NULL)
										, (s->is_video())?video_senc_flags:senc_flags
										);
								}

							}

						}

						bitrate++;
					}
					
					mpd_track_id += s->get_bitrate_count();

				 }

				 //free up vpssh
				 for(uint32_t v = 0; v < vpssh.size(); v++)
				 {
					 delete vpssh[v];
				 }

				 vpssh.clear();

			}
			
		}
		else if(kind == _T("pick"))
		{
			int stream = -1;

			if(c.command_specified(_T("stream")))
				stream = c.get_integer_value(_T("stream"));

			pick_samples(
				      c.get_integer64_value(_T("starttime"))
					, c.get_integer64_value(_T("endtime"))
					, mp4
					, stream
				);
		}
		else if(kind == _T("frame"))
		{
			int stream = -1;
			int max    =  0;
			Cstring output_dir = c.get_value(_T("output"));

			if(c.command_specified(_T("stream")))
				stream = c.get_integer_value(_T("stream"));

			if(c.command_specified(_T("max")))
				max = c.get_integer_value(_T("max"));

			frames(   c.get_value(_T("input"))
				    , c.get_integer64_value(_T("starttime"))
					, c.get_integer64_value(_T("endtime"))
					, mp4
					, output_dir
					, stream
					, max
				);
		}
		else if(kind == _T("simul"))
		{
			int stream = -1;

			if(c.command_specified(_T("stream")))
				stream = c.get_integer_value(_T("stream"));

			simulate_auto_decoding(
				      c.get_integer64_value(_T("starttime"))
					, c.get_integer64_value(_T("endtime"))
					, mp4
					, stream
				);
		}
		else if(kind == _T("ltc"))
		{
			timecode2( c.get_integer64_value(_T("starttime"))
					, c.get_integer64_value(_T("endtime"))
					, mp4
				);
		}
		else if(kind == _T("gop"))
		{
			uint64_t start = 0;
			uint64_t end = 0;

			if (c.command_specified(_T("starttime")))
				start = c.get_integer64_value(_T("starttime"));

			if (c.command_specified(_T("endtime")))
				end = c.get_integer64_value(_T("endtime"));


			gop_list(
				      start
					, end
					, mp4
				);
		}
		else if(kind == _T("H264Header"))
		{
			h264_frame_list(
				      c.get_integer64_value(_T("starttime"))
					, c.get_integer64_value(_T("endtime"))
					, mp4
				);
		}
		else if(kind == _T("xml"))
		{
			{
				MP4Reader reader;
				reader.parse(mp4);

				for(int i = 0; i < reader.get_root_meta_count(); i++)
				{
					uint64_t meta_position = reader.get_meta_position(i);
					
					mp4.set_position(meta_position);
					mp4.do_box();
					
					std::wcout 
						<< _T("META: ")
						<< mp4.get_box().get_type_string()
						<< _T(" position: ")
						<< meta_position
						<< _T(" size: ")
						<< mp4.get_box().get_size()	
						<< std::endl;
				}
			}

			/*
			if(c.command_specified(_T("xml")))
			{
				Cstring xml          = c.get_value(_T("xml"));
				unsigned int handler = BOX('r','i','f','m');
				unsigned int size    = 0;

				if(c.command_specified(_T("size")))
					size = c.get_integer64_value(_T("size"));

				if(c.command_specified(_T("handler")))
				{
					Cstring h = c.get_value(_T("handler"));
					const TCHAR * p = h;

					handler = BOX(p[0],p[1],p[2],p[3]);
				}
				
	

			//MP4Write        mp4w;
   //         CMP4WriteMemory mem;
			//                mem.Open(
			//					(size)?size:xml.size()*3
			//					);

		 //   mp4w.write_xml_meta(mem
			//	, xml
			//	, handler
			//	, size);

			//mem.flush();

				CMP4Meta meta;

				if(c.command_specified(_T("position")))
				{
					mp4.Close();

					meta.replace_meta(
							  c.get_integer64_value(_T("position"))
							, c.get_value(_T("input"))
							, xml
							, handler
							, size); 

					//CMemoryMappedFile mfile(
					//	c.get_value(_T("input"))
					//	, NULL
					//	, false);

					//mfile.Map(mem.get_size()
					//	, FILE_MAP_WRITE
					//	, c.get_integer64_value(_T("position"))
					//	);

					//ULONG written(0);
					//mfile.Write(mem.get_buffer()
					//	, mem.get_size()
					//	, &written);

					//_ASSERTE(written == mem.get_size());

				}
				else
				{

					//SHStreamWrite append;
					//append.OpenAppend(c.get_value(_T("input")));
					//ULONG w(0);
					//append.Write(mem.get_buffer()
					//		   , mem.get_size()
					//		   , &w);

					meta.append_meta(
							 c.get_value(_T("input"))
							, xml
							, handler
							, size); 
					
				}

			}//if(c.command_specified(_T("xml")))
			else
			{
				if(c.command_specified(_T("position")))
				{
					mp4.set_position(
						c.get_integer64_value(_T("position"))
						);

					mp4.do_box();

					if(box_meta == mp4.get_box().get_type())
					{
						mp4.do_full_box();
						mp4.do_box();

						if(box_hdlr == mp4.get_box().get_type())
						{
							HandlerBox h(mp4.get_box().get_size());
							mp4.parse_handler_box(h);
							mp4.do_box();

							if(box_xml == mp4.get_box().get_type())
							{
								XmlBox x(mp4.get_box().get_size());
								mp4.parse_xml_box(x);

								std::wcout 
								<< _T("HDLR: ")
								<< h.htype
								<< _T(" [")
								<< h.desc
								<< _T("]")
								<< std::endl
							    << x._p_xml
							    << std::endl;
							}
							else
							{
								std::wcout 
							    << _T("NO XML FOUND IN META")
							    << std::endl;
							}
						}
						else
						{
							std::wcout 
							<< _T("NO HDLR FOUND IN META")
							<< std::endl;
						}
					}
					else
					{
						std::wcout 
						<< _T("NO META FOUND")
						<< std::endl;
					}

					
				}
			}
			
			*/
		}
		else if(kind == _T("tfra"))
		{
			int r = doboxroot(mp4, _T("tfra"));
			if(0 != r)
				return r;

			r = docommand(mp4, _T("tfra"));
			if(0 != r)
				return r;
		}
		else
		{
			std::cout << _T("the specified kind is invalid: ")
				<< kind.c_str()
			<< std::endl;

			return 9;
		}

		Ctime now;

		uint64_t total_time = now.TotalHNano() - start_time.TotalHNano();

		std::wcout << _T("Edit add time ") 
					<< HNS(total_time)
					<< std::endl;
	
	/*}catch(ALX::Cexception &ex){
		std::wcerr << static_cast<LPCTSTR>(ex) << std::endl;
		return 1;
	}*/
	}catch ( std::exception & ex)
	{
	        std::cout << "EXCEPTION" << std::endl;
		std::cout << ex.what() << std::endl;
		return 1277;
	
	}
	catch (...)
	{
		std::cout << "...." << std::endl;
		return 127;
	}

	return 0;
}


