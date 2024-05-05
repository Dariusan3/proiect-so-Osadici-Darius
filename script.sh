!/bin/bash

filePath = "$1"
destPath = "$2"

chmod 777 "$filePath"

if grep -q -P '[^\x00-\x7F]' "$filePath"
then 
    mv "$filePath" "$destPath"
    exit 1
fi
if grep -q -E 'malware|dangerous|risk|attack' "$filePath"
then
    mv "$filePath" "$destPath"
    exit 1
else
    exit 0
fi