import json, os
import urllib
import urllib2

appurl = os.environ.get('APPVEYOR_API_URL')

if not None == appurl:
    url = appurl + 'api/build/messages'
    values = {
        "message": "This is a test message",
        "category": "warning",
        "details": "Additional information for the message"
    }

    r = json.dumps(values)

    print('APIREQUEST', url, r)

    req = urllib2.Request(url, r , headers={'Content-type': 'application/json', 'Accept': 'application/json'})
    response = urllib2.urlopen(req)
    the_page = response.read()
    print(the_page)
else:
    print('NO API SERVER')