#!/bin/bash

# use the above with -x to debug...

# This script pulls the build version from BuildVersion.h.
# The version string is used to create the file names for
# the build artifacts.
#
# Copyright (C) 2009 Diomede Coorporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNEC-
# TION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

versionString="VERINFO_VERSIONNAME"
startDeliminter="\""
endDeliminter="\)"
versionNameValue=

echo "Looking for " $versionString

#cat ./Include/BuildVersion.h | while read line; do

while read line; do

# This also finds the substring within the string but hardcodes
# the start and end postions.
#subString=`expr substr "${line}" 9 19`
#if [ "$subString" = "$versionString" ] 

if [[ "$line" =~ "$versionString" ]];
    then
        echo "Found it!: "

        # get the lengths of the line and substrings
        var=$line
        len=${#var} 
        lenVersion=${#versionString}
                
        posStart=`expr index "$line" "$startDeliminter"`
        posEnd=`expr index "$line" "$endDeliminter"`
        echo "Start pos: " $posStart
        echo "End pos: " $posEnd

        versionNameValue=$(echo "$var"|cut -c"$((posStart+1))"-"$((posEnd-2))")
        echo "$versionNameValue"
fi
done < ./Include/BuildVersion.h

cd LinuxPack/
cp ./makeReadMe.dat ./README
sed -i "s/{VERINFO_VERSIONNAME}/${versionNameValue}/g" README
cat ./README

cd ..
./version.sh
echo "Version from script: " ${versionBuildValue}




