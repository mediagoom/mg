# mg

[![Build Status](https://travis-ci.org/mediagoom/mg.svg?branch=master)](https://travis-ci.org/mediagoom/mg)
[![Win Build Status](https://ci.appveyor.com/api/projects/status/github/mediagoom/mg?branch=dev-win&svg=true)](https://ci.appveyor.com/project/aseduto/mg)

## Essential Open Source Web Media Streaming

**Use MPEG-DASH and HLS to effectively serve your video.**

mg packages MP4 files in order to be streammed on web.
Once they are packaged you may deploy your video to any web server to stream it.

Add or improve your video communication.
MediaGoom let you use video on the web effectively since using adaptive streaming let you:
- improve media startup time
- use your bandwidth evenly
- give each user the best possible experience based on their bandwidth
				
Essential since does not require any infrastructure in your site in order to achieve this. It just uses plain files.
				
In order to produce mp4 file to be consumed by mg you can follow this guide:  [How to encode files to use with mg](../../wiki/encode)


### Packaging

mg  allows you to package your MP4 files. In this way you they can be effectively streamed on the Internet.

Packaging features:


[comment]: <> (https://en.wikipedia.org/wiki/HTTP_Live_Streaming">HTTP Live Streaming also known as HLS)
 * Statically package your mp4 files as:
 1. [Dynamic Adaptive Streaming over HTTP (DASH)](https://en.wikipedia.org/wiki/Dynamic_Adaptive_Streaming_over_HTTP)
 2. [Http Live Stream (HLS)](https://en.wikipedia.org/wiki/HTTP_Live_Streaming)
 2. [Smooth Streaming](https://en.wikipedia.org/wiki/Adaptive_bitrate_streaming#Microsoft_Smooth_Streaming)


In order to generate the dash segments using mg you should have available one or more MP4 files.
All this files should be *gop aligned*.

[Read here](../../wiki/encode) to know how to produce your files in case you do not have them.

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

For instance if you followed the [explanation to encode a media file](../../wiki/encode) you could have the following files:
 *  test_1024x576_h264-750Kb_aac-lc.mp4
 *  test_1280x720_h264-1200Kb_aac-lc.mp4
 *  test_1280x720_h264-2000Kb_aac-lc.mp4
 *  test_1280x720_h264-3500Kb_aac-lc.mp4
 *  test_256x144_h264-120Kb_aac-lc.mp4
 *  test_512x288_h264-320Kb_aac-lc.mp4

So your command line should be:

**Bash**
```bash
mkdir dash
mg -k:dash -o:./dash \
-i:./test_1024x576_h264-750Kb_aac-lc.mp4 -b:750 -s:0 -e:0 \
-j:./test_1280x720_h264-1200Kb_aac-lc.mp4 -b:1200 \
-j:./test_1280x720_h264-2000Kb_aac-lc.mp4 -b:2000 \
-j:./test_1280x720_h264-3500Kb_aac-lc.mp4 -b:3500 \
-j:./test_256x144_h264-120Kb_aac-lc.mp4 -b:120 \
-j:./test_512x288_h264-320Kb_aac-lc.mp4 -b:320
```

**Powershell**
```powershell
mkdir dash
mg -k:dash -o:./dash \
-i:./test_1024x576_h264-750Kb_aac-lc.mp4 -b:750 -s:0 -e:0 `
-j:./test_1280x720_h264-1200Kb_aac-lc.mp4 -b:1200 `
-j:./test_1280x720_h264-2000Kb_aac-lc.mp4 -b:2000 `
-j:./test_1280x720_h264-3500Kb_aac-lc.mp4 -b:3500 `
-j:./test_256x144_h264-120Kb_aac-lc.mp4 -b:120 `
-j:./test_512x288_h264-320Kb_aac-lc.mp4 -b:320
```

Once you have your directory with *mpeg-dash* segment you can [publish](../../wiki/serve) then in a Web Server.


