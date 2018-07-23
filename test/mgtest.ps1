#
# Link Options: /PERFORMANCE
#


param($g = "*"
		, $t = "*"
		, $Configuration="Debug"
		, $perf = $false
		, $perf_dir	     = "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Team Tools\Performance Tools"
		, $repeat = 1
		, $cov = $false
		, $arch = "Win32"
)

$ErrorActionPreference = "Stop";
$my_dir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

. "$my_dir\PERFORMANCE.PS1"

$env:srcdir=$my_dir;
$env:MGTEST_REPEAT_TIME=$repeat;

$path = "$my_dir\bin\$arch\$Configuration";

$path | oh

if($perf -or $cov)
{
  configure_instrumentation $path $perf_dir "\.exe$" $cov
  if($perf)
  {start_performance $path}
  else
  {start_coverage $path}
}


& "$path\mgtest.exe" $g $t > mgtest.log
$result = $LASTEXITCODE

if($perf -or $cov)
{
	end_performance_session
}

if(0 -ne $result)
{
	write-error "test failed"
}
