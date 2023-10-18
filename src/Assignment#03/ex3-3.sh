#!/bin/sh
weight=$1
height=$2

total_weight=$(( $weight * 100000 ))
total_height=$(( $height * $height ))

bmi=$(( $total_weight / $total_height ))

if [ $bmi -le 185 ]; then
    echo "저체중입니다."
elif [ $bmi -lt 230 ]; then
    echo "정상체중입니다."
else
    echo "과체중입니다."
fi

exit 0
