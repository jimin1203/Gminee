#!/bin/sh

if [ -n "$1" ] 
then
    c=$1
    while [ $c -ne 0 ]
    do
        echo "hello world"
        c=$(($c - 1))
    done
else
    while [ 1 ]
    do
        echo "Hello World"
    done
fi

exit 0
