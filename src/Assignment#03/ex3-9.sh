#!/bin/sh

database="DB.txt"

if [ ! -e "$database" ]; then
    echo "DB 파일이 없습니다."
    exit 1
fi

search_name="$1"


info=$(grep "^$search_name:" "$database")

if [ -n "$info" ]; then
    echo "$info"
else
    echo "검색 결과가 없습니다."
fi

exit 0
