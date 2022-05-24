#!/bin/sh

set -e

run_client() {
    ./bin/sdstore "README.md" "/tmp/$1_0" bcompress gcompress
    ./bin/sdstore "/tmp/$1_0" "/tmp/$1_1" encrypt nop decrypt
    ./bin/sdstore "/tmp/$1_1" "/tmp/$1_2" gdecompress bdecompress
    echo "finished for $1"
}

for i in {0..10}
do
    run_client $i &
    sleep 0.00001
done


./bin/sdstore status