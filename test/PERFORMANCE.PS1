

function configure_instrumentation($assembly_bin, $perf_dir, $includerx = "\.dll$", $coverage = $false)
{

    if($null -eq $perf_dir){return;}

	if(-Not($env:path.Contains($perf_dir)))
	{
		"Adding perf_dir $perf_dir" | oh
		$env:path = "$env:path;$perf_dir;"
	}

	"Getting DLLS From $assembly_bin" | oh
	$dlls = gi "$assembly_bin\*.*" | where {$_ -match $includerx}
	
	foreach($dll in $dlls)
	{
	    $instr = $true;
	    $orig = "$($dll.FullName).orig"
		
		if([IO.File]::Exists("$($dll.FullName).orig"))
		{
			$or = ls $orig

			
			if($or.LastWriteTime -ge $dll.LastWriteTime)
			{
				$instr = $false;
			}

			"CHECKING $($dll.Name) [$instr] $($dll.LastWriteTime) $($or.LastWriteTime)" | oh
		}

		if($instr)
		{
			"INSTRUMENTING $dll" | oh

			if($coverage)
			{
			   vsinstr -coverage "$($dll.FullName)"
			}
			else
			{
			   vsinstr "$($dll.FullName)"
			}
			$e = $LASTEXITCODE 

			if(0 -ne $e)
			{
				Write-Error "Instrumentation Failed"
			}

			$or = ls $orig
			$d  = ls $dll

			$or.LastWriteTime = $d.LastWriteTime


		}
		else
		{
			"SKIPPING INSTRUMENTING $dll" | oh
		}
	}

	#$perf_dir

	#VSPerfCmd.exe /Start:COVERAGE "/Output:$assembly_bin\$([DateTime]::Now.ToString('yyyyMMddHHmmssfff')).VSP" #"/LAUNCH:$($p.path)" "/ARGS:-command $args"

}

function end_performance_session()
{

    #if($null -eq $perf_dir){return;}

	vsperfcmd -shutdown

}

function start_performance($assembly_bin, $type = "TRACE")
{
    
	VSPerfCmd.exe "/Start:$type" "/Output:$assembly_bin\$([DateTime]::Now.ToString('yyyyMMddHHmmssfff')).VSP" #"/LAUNCH:$($p.path)" "/ARGS:-command $args"
}

function configure_coverage($assembly_bin, $perf_dir, $includerx = "\.dll$")
{
	configure_instrumentation $assembly_bin $perf_dir $includerx $true
}

function start_coverage($assembly_bin)
{
	start_performance $assembly_bin "COVERAGE";
}
