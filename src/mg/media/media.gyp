{

   'includes': ['../../../mg.gypi']
 , 'targets': [
    {
        'target_name': 'mgmedia'
      , 'type': 'static_library'
	  , 'defines' : ['MG_ERRROR_LOG']
      , 'dependencies': [
                  '../../../deps/AES/aes.gyp:gypaes'
                , '../../../deps/libuv/uv.gyp:libuv'
                , '../core/core.gyp:mgcore'
                , '../../../deps/flavor/flavor.gyp:flavor'
              ]
	, 'direct_dependent_settings': {
            'include_dirs': [ './' ]
		    , 'defines' : ['MG_ERRROR_LOG']
	    }
      ,'conditions': [
       		 ['OS == "win"', {
              'variables'	: {'flavordir': '../../../deps/flavor/<(CONFIGURATION_NAME)/'}
           }
           ,{
	   	'variables' : { 'flavordir': '../../../test/out/<(CONFIGURATION_NAME)/'}
            }
           
           ]
      ]
      
      
    , 'rules': [
       {
         'rule_name': 'flavor',
         'extension': 'fl',
         'inputs': [
         ]
         , 'outputs': [
           'mp4/<(RULE_INPUT_ROOT).cpp'
           , 'mp4/<(RULE_INPUT_ROOT).h'
         ]
	, 'conditions': [
		['OS == "win"', {
			  'action': [
			   '<(flavordir)flavor', '-x', '-l', '-oh', 'mp4/<@(RULE_INPUT_ROOT).h'
			   , '-oc', 'mp4/<@(RULE_INPUT_ROOT).cpp', '<@(RULE_INPUT_PATH)',
			 ]}
		,{
			  'action': [
			   '<(flavordir)flavor', '-x', '-l', '-oh', 'mp4/<@(RULE_INPUT_ROOT).h'
			   , '-oc', 'mp4/<@(RULE_INPUT_ROOT).cpp', 'mp4/<@(RULE_INPUT_ROOT).fl',
			 ]}
		]
	]
         , 'process_outputs_as_sources': 1,
       },
     ]
      , 'include_dirs': [
          './'
        , '../..'
      ],
      'sources': [
          'stdafx.h'
        , 'TBitstream.h'
        , 'mp4/aac.fl'
        , 'mp4/cenc.fl'
        , 'mp4/h264.fl'
        , 'mp4/ltcextension.fl'
        , 'mp4/mp4.fl'
        , 'mp4/mp4i_ext.fl'
        , 'mp4/mp4_fragments.fl'
        , 'mp4/mpeg4_odf.fl'
        , 'mp4/mpeg2au.fl'
        , 'mp4/mpeg2ts.fl'
        , 'mp4fragmented.h'
        , 'media_parser.h'
        , 'MP4Mux.h'
        , 'mp4parse.h'
        , 'media_queue.h'
        , 'WaveParser.h'
        , 'fixed_queue.h'
        , 'h264nal.h'
        , 'hnano.h'
        , 'MOOFReader.h'
        , 'mp4write.h'
        , 'mp4fragmented.h'
        , 'MediaJoin.h'
        , 'mp4edit.h'
        , 'mp4edit.cpp'
        , '../../mgmedia.h'
        , 'mp4dynamic.h'
        , 'mp4dynamic.cpp'
        , 'mp4dynamicinfo.h'
        , 'mp4dynamicinfo.cpp'
        , 'hls_renderer.h'
        , 'mpd_renderer.h'
        , 'wincrc.h'
        , 'wincrc.cpp'
        , 'mpeg.h'
        , 'ts.h'
        , 'tsmux.h'
        , 'tswrite.h'
        , 'tsparse.h'
        , 'tsinfo.h'
        , 'tsinfo.cpp'
      ]              
    }
  ]
}

