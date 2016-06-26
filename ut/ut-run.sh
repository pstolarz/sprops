#!/bin/sh

chk_diff() {
    echo -n "$1: ";
    if [ `./$1 | diff - $2 | wc -c` -gt 0 ];
    then
        echo "FAILED: Differences found between test's output and $2";
    else
        echo "OK";
    fi
}

chk_diff t01-get t01.out;

chk_diff t02-iter t02.out;

./t03-tknstr 2>&1 1>/dev/null
if [ $? -eq 0 ];
then
    echo "t03-tknstr: OK";
else
    echo "t03-tknstr: FAILED";
fi

chk_diff t04-add t04.out;

chk_diff t05-rm t05.out;

chk_diff t06-set t06.out;

chk_diff t07-mv t07.out;
