#!/bin/bash

# usage: ./loop_thumbnailer QUEUENAME VIEWPORTWIDTH VIEWPORTHEIGHT SLEEPCHECK SCALEDWIDTH MAXREQUESTS
# Note: CutyCapt must be in the same directory as this script

PRIMARYQUEUENAME=$1
SECONDARYQUEUENAME=$2
SLEEPCHECK=$3
MAXREQUESTS=$4
INSTANCE=$5
MAXREQUESTTIME=$6

abspath="$(cd "${0%/*}" 2>/dev/null; echo "$PWD"/"${0##*/}")"
path_only=`dirname "$abspath"`
cd $path_only

while [ 0 == 0 ]; do
    su www-data -c "xvfb-run --error-file=/tmp/xvfb-run.$INSTANCE.out -a -w 1 --server-args='-screen 0, 1024x768x24' \
    ./CutyCapt --primary-queue-name=$PRIMARYQUEUENAME --secondary-queue-name=$SECONDARYQUEUENAME \
    --sleep-check=$SLEEPCHECK --max-runs=$MAXREQUESTS \
    --max-wait=$MAXREQUESTTIME \
    --user-agent='Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_5_7; de-de) AppleWebKit/525.28.3 (KHTML, like Gecko) Version/3.2.3 Safari/525.28.3' \
    --instance=$INSTANCE"
    sleep 1
done
