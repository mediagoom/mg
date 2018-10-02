---
layout: default
title: Package media file for play
tags: [help]
---

In order to generate the dash segments using mg you should have available one or more MP4 files.
All this files should be *gop aligned*.

[Read here](./Encode.html) to know how to produce your files in case you do not have them.

In order to verify that your files are *gop aligned* you can use this command:
```bash
mg -k:gop -i:<mp4 file>
```
Running the same command on all file should result in the same gop list. 

Once you have verified the input file are correct you can use mg to produce the mpeg dash segments:

```bash
mg -k:dash -o:<output directory> \
-i:<first file> -b:<first bitrate> -s:0 -e:0 \
-j:<second file> -b:<second bitrate> \
-j:<third file> -b:<therd bitrate> \
-j:<nth file> -b<nth bitrate>
```

For instance if you followed the [explanation to encode a media file](./encode) you could have the following files:
 *  test_1024x576_h264-750Kb_aac-lc.mp4
 *  test_1280x720_h264-1200Kb_aac-lc.mp4
 *  test_1280x720_h264-2000Kb_aac-lc.mp4
 *  test_1280x720_h264-3500Kb_aac-lc.mp4
 *  test_256x144_h264-120Kb_aac-lc.mp4
 *  test_512x288_h264-320Kb_aac-lc.mp4

So your command line should be:

# Bash
```bash
mkdir adaptive
mg -k:adaptive -o:./dash \
-i:./test_1024x576_h264-750Kb_aac-lc.mp4 -b:750 -s:0 -e:0 \
-j:./test_1280x720_h264-1200Kb_aac-lc.mp4 -b:1200 \
-j:./test_1280x720_h264-2000Kb_aac-lc.mp4 -b:2000 \
-j:./test_1280x720_h264-3500Kb_aac-lc.mp4 -b:3500 \
-j:./test_256x144_h264-120Kb_aac-lc.mp4 -b:120 \
-j:./test_512x288_h264-320Kb_aac-lc.mp4 -b:320
```

# Powershell
```powershell

$dir=[path to the output directory] 
$name="test"

mkdir $dir
mg -k:adaptive "-o:$dir" `
"-i:./$($name)_1024x576_h264-750Kb_aac-lc.mp4" -b:750 -s:0 -e:0 `
"-j:./$($name)_256x144_h264-120Kb_aac-lc.mp4" -b:120 `
"-j:./$($name)_512x288_h264-320Kb_aac-lc.mp4" -b:320 `
"-j:./$($name)_1280x720_h264-1200Kb_aac-lc.mp4" -b:1200 `
"-j:./$($name)_1280x720_h264-2000Kb_aac-lc.mp4" -b:2000 `
"-j:./$($name)_1280x720_h264-3500Kb_aac-lc.mp4" -b:3500 

```

Once you have your directory with *mpeg-dash* segment you can [publish](./Serve.html) then in a Web Server.




