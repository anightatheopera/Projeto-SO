#!/bin/sh

set -e

run_client() {
    echo "started for $1"
    (./bin/sdstore "README.md" "/tmp/$1_0" bcompress gcompress \
    && ./bin/sdstore "/tmp/$1_0" "/tmp/$1_1" encrypt nop decrypt \
    && ./bin/sdstore "/tmp/$1_1" "/tmp/$1_2" gdecompress bdecompress) > /dev/null
    echo "finished for $1"
}

for i in {0..10}
do
    run_client $i &
done

sleep 0.2