{
  'includes': ['../../mg.gypi'],
  'targets': [
    {
      'target_name': 'base64',
      'type': '<(component)'
	  , 'direct_dependent_settings': {
            'include_dirs': [ './include' ]
			, 'defines' : ['BASE64']
			}
    , 'dependencies': [
      ],
      'defines': [
      ],
      'sources': [
            'src/cdecode.cpp'
          , 'src/cencode.cpp'
        ],
      'conditions': [
      ]        
    }
  ]
}

