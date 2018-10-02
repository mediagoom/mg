---
layout: default
title: Encode Files for mg
tags: [help]
---



In the beginning you should have a file you want to play in a web page or in a mobile application using adaptive streaming instead of progressive download.

The first thing you need is [*ffmpeg*](https://ffmpeg.org/download.html). A free tool.

The basic command to create each of the needed file should be something similar to:

# Encode with Bash
```bash
ffmpeg -i [input file] -vf "scale=w=1280:h=720" \
-codec:v libx264 -profile:v high -level 31 -b:v 750k \
-r 25 -g 50 -sc_threshold 0 -x264opts ratetol=0.1 \
-minrate 750k -maxrate 750k -bufsize 750k \
-b:a 96k -codec:a aac -profile:a aac_low -ar 44100 -ac 2 mg_750.mp4
```
# Encode with Powershell
```powershell
ffmpeg -i [input file] -vf "scale=w=1280:h=720" `
-codec:v libx264 -profile:v high -level 31 -b:v 750k `
-r 25 -g 50 -sc_threshold 0 -x264opts ratetol=0.1 `
-minrate 750k -maxrate 750k -bufsize 750k `
-b:a 96k -codec:a aac -profile:a aac_low -ar 44100 -ac 2 mg_750.mp4
```
In the above command this are the parameter used and their explanation:

 * *-i* This is the input file you want to convert. 
 * *-vf* with this parameter you define the output size of your file. You should decrease the output size while you decrease the bitrate.
 * *-codec* this the codec. You should mainly use **libx264** and **aac**.
 * *-profile* and *-level* this is the h264 profile. For maximum compatibility use **high**.
 * *-b:v* this is the video bit rate. You should change this parameter together with the video size for each output file. 

# Sample Encoding Session

You can use this function to simplify the encoding and setting the file path of the source:
## Bash Function
```bash
function encode()
{

ffmpeg -i "$2" -vf "scale=w=$4:h=$5" -codec:v libx264 \
-profile:v high -level 31 -b:v "${3}k" \
-r 25 -g 50 -sc_threshold 0 -minrate "${3}k" \
-maxrate "${3}k" -bufsize "${3}k" -x264opts ratetol=0.1 \
-b:a 96k -codec:a aac -profile:a aac_low -ar 44100 -ac 2 -strict -2 \
 "$1_$6x$7_h264-$5Kb_aac-lc.mp4"
}

f=[path to file to encode] 
```

## Powershell Function
```powershell

function encode($n, $f, $k, $w=1280, $h=720)
{

ffmpeg -i "$f" -vf "scale=w=$($w):h=$($h)" -codec:v libx264 `
-profile:v high -level 31 -b:v "$($k)k" `
-r 25 -g 50 -sc_threshold 0 -minrate "$($k)k" -maxrate "$($k)k" `
-bufsize "$($k * 1)k" `
 -x264opts ratetol=0.1 -b:a 96k -codec:a aac `
-profile:a aac_low -ar 44100 -ac 2 -strict -2 `
"$($n)_$($w)x$($h)_h264-$($k)Kb_aac-lc.mp4"
}

```

With the function defined to produce 6 bitrates you just run:
```powershell
$file=[path to file to encode] 
$name="test"

encode $name $f 120 256 144
encode $name $f 320 512 288
encode $name $f 750 1024 576
encode $name $f 1200 1280 720
encode $name $f 2000 1280 720
encode $name $f 3500 1024 720
```

Once you have your 6 files you can [generate the dash segments using **mg**](./package).