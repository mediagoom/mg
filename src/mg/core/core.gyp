{

   'includes': ['../../../mg.gypi']
 , 'targets': [
    {
      'target_name': 'mgcore',
      'type': 'static_library'
      ,'conditions': [
       		 ['OS == "win"', {
	
			        'dependencies': [
                  '../../../deps/libuv/uv.gyp:libuv'
              ]
              , 'sources':[
                   'win/win-util.h'
                 , 'win/util.cpp'
                 , 'win/ctime.cpp' 
                ]
           }
           ,{
              'sources':[
                   'nx/util.cpp'
                 , 'nx/ctime.cpp'
                ]
             }
           
           ]
      ],
      'defines': [
        
      ],
      'include_dirs': [
      ],
      'sources': [
          'core.h'
	, 'char.h'
	, '../cresource.h'
	, 'bitstream.cpp'
	, 'bitstream.h'
	, 'thread.h'
	, 'thread.cpp'
	, '../../mgcore.h'
	, 'core.gyp'
  , 'file.cpp'
  , 'file.h'
  , '../alxstring.h'
  , '../cbuffer.h'
  , '../exception.h'
  , '../promises.h'
  , '../cmdline.h'
  , '../splitter.h'
  , 'uvbase.h'
  , 'uvbase.cpp'
  , 'util.h'
  , 'ctime.h'
      ]              
    }
  ]
}

