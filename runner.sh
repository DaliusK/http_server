#!/bin/bash -x
#lines: 1 for grep, 1 for the process
cd /home/pi/src/http_server
LOGFILE='http_server.log';
lines=`ps x | grep -i "./bin/http_server 80 >> logs/server.log &" | wc -l`;

if [[ $lines -ne 2 ]]
then
    #means the process stopped, re-run it
    echo "\nHTTP server not found in process list, running again\n" >> $LOGFILE
    ./bin/http_server 80 >> logs/$LOGFILE &
fi
