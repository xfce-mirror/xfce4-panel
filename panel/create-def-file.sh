#!/bin/sh

echo "IMPORTS" > panel.def
grep -h "EXPORT:" $* | \
    sed -e 's/.*\/\* EXPORT:\([^ ]*\) *\*\//\1 = xfce4-panel.exe.\1/' \
    >> panel.def
