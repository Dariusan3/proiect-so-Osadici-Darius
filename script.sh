!/bin/bash

filePath="$1"

chmod 777 "$filePath"

result="SAFE"

if grep -q -P '[^\x00-\x7F]' "$filePath"
then 
    result="UNSAFE"
fi
if grep -q -E 'malware|dangerous|risk|attack' "$filePath"
then
    result="UNSAFE"
fi

lineCount=$(wc -l < "$filePath")
wordCount=$(wc -w < "$filePath")
caracterCount=$(wc -c < "$filePath")

if ((lineCount < 3 && wordCount > 1000 && caracterCount > 2000))
then
    result="UNSAFE"
fi

if [ "UNSAFE" == "$result" ];
then
    result="$filePath"
fi

echo "$result"
