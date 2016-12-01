param($arch='ia32')

$ErrorActionPreference = "Stop";


$my_dir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

#check whether we have gyp installed and in our path
if( (get-command gyp -ea SilentlyContinue) -eq $null)
{
	#gyp is not in our path
	#test whether we have defined a function to load gyp
	if( (get-command use-gyp -ea SilentlyContinue) -ne $null)
	{
		use-gyp
	}
	else
	{
		Write-Error 'gyp is not available.'
	}

}

gyp mgtest.gyp --depth 0 -Duv_library=static_library "-Dtarget_arch=$arch" -I../deps/libuv/common.gypi

$k = $LASTEXITCODE;


return $k;
