{
  'includes': ['../mg.gypi'],
    

  'targets': [
    {
        'target_name': 'mgtest'
      , 'type': 'executable'
      ,  'conditions': [
       		 ['OS == "win"', {
      
                  'dependencies': [
                      '../deps/libuv/uv.gyp:libuv'
                    ]
              }
              , 
              {
                'libraries': ['uv']
              }
              ]
      	]
      , 'dependencies': [
               '../src/mg/core/core.gyp:mgcore' 	
             , '../src/mg/media/media.gyp:mgmedia'
             , '../src/mgcli/mgcli.gyp:mg'
      		]
      , 'defines': [
        
      ]
      , 'include_dirs': [
      	 '..\src'
      ]
      , 'sources': [
	  'test.cpp'
  , 'test_base.h'
  , 'test_util.h'
  , 'test_util.cpp'
  , 'test_bitstream.h'
	, 'test_bitstream.cpp'
	, 'test_thread.h'
	, 'test_thread.cpp'
  , 'test_resource.h' 
  , 'test_resource.cpp' 
  , 'test_buffer.cpp'
  , 'test_uv.h'
  , 'test_uv.cpp'
  , 'test_mp4_parse.h'
  , 'test_mp4_parse.cpp'
      ]              
    }
  ]
}

