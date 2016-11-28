$dp = (gi ..\..\flavor\Win32\Release).FullName;

$ErrorActionPreference = "Stop";

if(-Not (test-path "$dp\flavor.exe"))
{
   Write-Error 'flavor not found';  
}

if(-Not ($env:Path.Contains($dp)))
{
	$env:Path="$env:Path;$dp";
}

$ErrorActionPreference = "Stop";

$my_dir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

use-gyp
gyp mgtest.gyp --depth 0 -Duv_library=static_library -Dtarget_arch=ia32 -I../deps/libuv/common.gypi

$k = $LASTEXITCODE;

"LASTEXITCODE: $k" | oh
if(0 -eq $k)
{
  & "$my_dir\mgtest.sln"
}
