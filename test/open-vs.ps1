param($arch="ia32")

$ErrorActionPreference = "Stop";

#if(-Not (test-path "$dp\flavor.exe"))
#{
#   Write-Error 'flavor not found';  
#}

#if(-Not ($env:Path.Contains($dp)))
#{
#	$env:Path="$env:Path;$dp";
#}

#$ErrorActionPreference = "Stop";

$my_dir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

$k = & "$my_dir\compile-gyp.ps1" $arch
$j = $LASTEXITCODE
$k | oh
"LASTEXITCODE: $j" | oh
if(0 -eq $j)
{
  & "$my_dir\mgtest.sln"
}
