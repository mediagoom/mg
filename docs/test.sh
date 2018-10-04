#!/bin/bash

DIRECTION=0

if [ -z "$1" ]; then
    DIRECTION=1
fi

function myfa(){ 
    echo "$1"

if [ -d "$SCRIPTPATH/$1" ]; then
  rm "$SCRIPTPATH/$1" -fr
fi

SOURCE="$TARGET/$1"

echo $SOURCE

if [ -d "$SOURCE" ]; then
  cp "$SOURCE" "$SCRIPTPATH" -r
  #ls "$TARGET"
fi
}

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
echo $SCRIPTPATH;

TARGET="$SCRIPTPATH/../../mgdoc-jekyll-theme";
echo $TARGET;

echo $DIRECTION

if [ 0 == $DIRECTION ]; then
    TMP="$SCRIPTPATH";
    SCRIPTPATH=$TARGET;
    TARGET=$TMP
fi

echo $SCRIPTPATH;
echo $TARGET;

myfa "_layouts"
myfa "assets"
myfa "_sass"
myfa "_includes"
