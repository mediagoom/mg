{
    'includes': ['../../mg.gypi']
  , 'targets': [
    {
        'target_name': 'mg'
      , 'type': 'executable'
      ,  'conditions': [
       		    ['OS == "win"', {
      
                  'dependencies': [
                        '../../deps/libuv/uv.gyp:libuv'
                      , '../../deps/AES/aes.gyp:gypaes'
                    ]
              }
              , 
              {
                'libraries': ['uv', 'gypaes']
              }
              ]
      	]
      , 'dependencies': [
               '../mg/core/core.gyp:mgcore' 	
             , '../mg/media/media.gyp:mgmedia'
			       , '../b64/b64.gyp:base64'
      		]
      , 'defines': [
        
      ]
      , 'include_dirs': [
      	 '../'
      ]
      , 'sources': [
            'mp4info.cpp'
		      , 'stdafx.h'
      ]              
    }
  ]
}

