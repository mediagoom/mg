#!/bin/bash

function myfa(){
    echo "$1"

if [ -d "$SCRIPTPATH/$1" ]; then
  rm "$SCRIPTPATH/$1" -fr
fi

TARGET="$SCRIPTPATH/../../mgdoc-jekyll-theme/$1"

echo $TARGET

if [ -d "$TARGET" ]; then
  cp "$TARGET" "$SCRIPTPATH" -r
  #ls "$TARGET"
fi
}

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
echo $SCRIPTPATH;

myfa "_layouts"
myfa "assets"
myfa "_sass"
myfa "_includes"