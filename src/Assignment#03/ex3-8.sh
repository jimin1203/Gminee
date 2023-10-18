#!/bin/sh

database="DB.txt"

if [ ! -e "$database" ]; then
    touch "$database"
fi

name="$1"
info="$2"

echo "$name: $info" >> "$database"

echo "정보가 DB.txt 파일에 추가되었습니다."

exit 0
