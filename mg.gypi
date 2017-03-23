{
  'variables': {

    'target_arch%': 'x86',          
    'host_arch%': 'x86',            
    'library%': 'static_library',     # allow override to 'shared_library' for DLL/.so builds
    'component%': 'static_library',   # NB. these names match with what V8 expects
    'msvs_multi_core_compile': '0',   # we do enable multicore compiles, but not using the V8 way
    'python%': 'python',
  },

  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'variables': {

        },
          'defines': [ 'DEBUG', '_DEBUG' ]
        , 'cflags': [ '-g3', '-O0' , '-std=c++11']
        , 'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64',
          }],
         
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
            'Optimization': 0, # /Od, no optimization
            'MinimalRebuild': 'false',
            'OmitFramePointers': 'false',
            'BasicRuntimeChecks': 3, # /RTC1
          },
          'VCLinkerTool': {
            'LinkIncremental': 2, # enable incremental linking
          },
        },
      },
      'Release': {
          'variables': {}
        , 'cflags': [ '-O2', '-g', '-std=c++11', '-static-libstdc++', '-static-libgcc']
        , 'ldflags': ['-static', '-static-libstdc++', '-static-libgcc'] 
	, 'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64',
          }],
          
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
            'Optimization': 3, # /Ox, full optimization
            'FavorSizeOrSpeed': 1, # /Ot, favour speed over size
            'InlineFunctionExpansion': 2, # /Ob2, inline anything eligible
            'WholeProgramOptimization': 'true', # /GL, whole program optimization, needed for LTCG
            'OmitFramePointers': 'true',
            'EnableFunctionLevelLinking': 'true',
            'EnableIntrinsicFunctions': 'true',
            'RuntimeTypeInfo': 'false',
            'AdditionalOptions': [
              '/MP', # compile across multiple CPUs
            ],
          },
          'VCLibrarianTool': {
            'AdditionalOptions': [
              '/LTCG', # link time code generation
            ],
          },
          'VCLinkerTool': {
            'LinkTimeCodeGeneration': 1, # link-time code generation
            'OptimizeReferences': 2, # /OPT:REF
            'EnableCOMDATFolding': 2, # /OPT:ICF
            'LinkIncremental': 1, # disable incremental linking
          },
        },
      }
    }
    
    , 'cflags!': ['-Werror', '-fno-exceptions'
			, '-Wall', '-ansi', '-pthread'
			, '-O3'
			, '-fstrict-aliasing'
			, '-fomit-frame-pointer'
			, '-fdata-sections'
			, '-ffunction-sections'
		]
    , 'cflags_cc!': [ '-fno-rtti', '-fno-exceptions' ]
    , 'msvs_configuration_attributes': {
	  'OutputDirectory': '$(SolutionDir)bin\\$(PlatformName)\\$(ConfigurationName)\\',
	}
    , 'msvs_settings': {
      'VCCLCompilerTool': {
        'StringPooling': 'true', # pool string literals
        'DebugInformationFormat': 3, # Generate a PDB
        'WarningLevel': 3,
        'BufferSecurityCheck': 'true',
        'ExceptionHandling': 1, # /EHsc
        'SuppressStartupBanner': 'true',
        'WarnAsError': 'false',
      },
      'VCLibrarianTool': {
      	'OutputFile' : '$(OutDir)\$(ProjectName).lib'
      },
      'VCLinkerTool': {
        'conditions': [
          ['target_arch=="x86"', {
            'TargetMachine' : 1, # /MACHINE:X86
            'target_conditions': [
              ['_type=="executable"', {
                'AdditionalOptions': [ '/SubSystem:Console,"5.01"' ],
              }],
            ],
          }],
          ['target_arch=="x64"', {
            'TargetMachine' : 17, # /MACHINE:AMD64
            'target_conditions': [
              ['_type=="executable"', {
                'AdditionalOptions': [ '/SubSystem:Console,"5.02"' ],
              }],
            ],
          }],
        ],
        'GenerateDebugInformation': 'true',
        'GenerateMapFile': 'true', # /MAP
        'MapExports': 'true', # /MAPINFO:EXPORTS
        'RandomizedBaseAddress': 2, # enable ASLR
        'DataExecutionPrevention': 2, # enable DEP
        'AllowIsolation': 'true',
        'SuppressStartupBanner': 'true',
      },
    },
    
      
  }
}
