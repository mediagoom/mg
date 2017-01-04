
if [ -z "$DBOX" ]; then
    echo "DBOX NOT FOUND"
        exit 1
fi 

which curl > /dev/null
if [[ $? != 0 ]]; then
	echo "CURL NOT FOUND"
	exit 1
fi

curl "https://raw.githubusercontent.com/andreafabrizi/Dropbox-Uploader/master/dropbox_uploader.sh" -o ./dropbox_uploader.sh
chmod 777 dropbox_uploader.sh

echo "OAUTH_ACCESS_TOKEN=${DBOX}" > ~/.dropbox_uploader

#ls -a
#ls ~ -a

./dropbox_uploader.sh upload ./out/Release/mg mg
