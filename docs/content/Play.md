---
layout: default
title: Play
tags: [help]
---

Once your file are [published  in a Web Server](./serve) you can play them.

Look at the [Media goom sample player project](https://github.com/mediagoom/Play) to see how to play adapting on the device.

For mpeg-dash you can just use this snipet on your page:

```html
<script src="http://cdn.dashjs.org/latest/dash.all.min.js"></script>
...
<style>
    video {
       width: 640px;
       height: 360px;
    }
</style>
...
<body>
   <div>
       <video data-dashjs-player autoplay src="[url of your source]" controls>
       </video>
   </div>
</body>
```


