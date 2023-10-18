#!/bin/sh

echo "리눅스가 재밌나요? (yes / no)"

read answer

case $answer in
    *yes* | *y* | *Y* | *Yes* | *네* | 예 | 넵 | 옙 | *좋아* )
        echo "yes";;
    *no* | *n* | *N* | *No* | *아니* | *아니요* | *싫어* )
        echo "no";;
    * )
        echo "yes or no로 입력해 주세요."
        exit 1;;
    esac

    exit 0
