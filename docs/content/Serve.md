---
layout: default
title: Server your media Content
tags: [help]
---

Once you have created your directory containing [mpeg-dash segments](./package) you can publish it in a Web Server.

Most of the time it should just work when you point your player to that folder but in order to be sure you should check whether your server has the correct content types.

This is a list of the correct content types:

* .mpd : application/dash+xml
* .m4a : audio/mp4
* .m4v : video/mp4

In case you want to access your content from a different domain from your player you should add a cross domain header as well:

* Access-Control-Allow-Origin: *

Instead of an * you can but the exact domain of your player.

This is a list of how to accomplish the above operations in some web server.

# Nginx

In a file in the nginx configuration directory, usually /etc/nginx/conf.d create a new file containing

```
add_header 'Access-Control-Allow-Origin' '*';
          types {
	  		application/dash+xml     mpd;
	  }
```

# Apache

In your htaccess file add the following:

```
AddType audio/mp4 .m4a
AddType video/mp4 .m4v
AddType application/vnd.ms-sstr+xml .ism
AddType application/dash+xml .mpd
AddType video/mp2t .ts
AddType application/vnd.apple.mpegurl .m3u8
Header set Access-Control-Allow-Origin *
```

# Microsoft IIS

In the web.config add the following:

```xml
<configuration>
	<system.webServer>
        <handlers>
            <remove name="LiveStreamingHandler" />
            <remove name="SmoothHandler" />
            <remove name="PippoSmoothHandler" />
        </handlers>
        <staticContent>
		<remove fileExtension=".m4f" />
		<remove fileExtension=".m4v" />
		<remove fileExtension=".mp4v" />
		<remove fileExtension=".m4a" />
		<remove fileExtension=".ts" />
		<remove fileExtension=".m3u8" />
		<remove fileExtension=".json" />
		<remove fileExtension=".key" />
		<remove fileExtension=".mpd" />
		
			<mimeMap fileExtension=".mpd"  mimeType="application/dash+xml" />
            		<mimeMap fileExtension=".m4a"  mimeType="audio/mp4" />
			<mimeMap fileExtension=".m4v"  mimeType="video/mp4" />
			<mimeMap fileExtension=".mp4v" mimeType="video/mp4" />
			
			<mimeMap fileExtension=".ism"  mimeType="application/vnd.ms-sstr+xml" />
			<mimeMap fileExtension=".ts"   mimeType="video/mp2t" />
			<mimeMap fileExtension=".m3u8" mimeType="application/vnd.apple.mpegurl" />
			<mimeMap fileExtension=".json" mimeType="application/javascript" />
			<mimeMap fileExtension=".key"  mimeType="application/javascript" />
			
        </staticContent>
        <httpProtocol>
            <customHeaders>
                <remove name="X-Powered-By" />
                <add name="Cache-Control" value="max-age=120000" />
                <add name="Access-Control-Allow-Origin" value="*" />
            </customHeaders>
        </httpProtocol>
    </system.webServer>
</configuration>
```