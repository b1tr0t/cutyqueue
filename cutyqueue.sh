#! /bin/bash
# set -x
### BEGIN INIT INFO
# Provides:          CutyCapt queue service
# Required-Start:    networking
# Required-Stop:     networking
# Default-Start:     2 3 4 5
# Default-Stop:      S 0 1 6
# Short-Description: Start FastCGI servers with Django.
# Description:       Django, in order to operate with FastCGI, must be started
#                    in a very specific way with manage.py. This must be done
#                    for each DJango web server that has to run.
### END INIT INFO
#
# Author:  Guillermo Fernandez Castellanos
#          <guillermo.fernandez.castellanos AT gmail.com>.
#
# Version: @(#)fastcgi 0.1 11-Jan-2007 guillermo.fernandez.castellanos AT gmail.com
#

#### SERVER SPECIFIC CONFIGURATION
QUEUE_NAME="queue"
CUTYCAPT_DIR="/services/cutyqueue"

## Cutycapt options: 
PRIMARYQUEUENAME="queue_fast"
SECONDARYQUEUENAME="queue_slow"
VIEWPORTWIDTH="980"
VIEWPORTHEIGHT="1470"
SLEEPCHECK="100"
SCALEDWIDTH="160"
MAXREQUESTS="50"

INSTANCES="CC_1 CC_2 CC_3"

set -e

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="CutyCapt queue processors"
NAME=$0
SCRIPTNAME=/etc/init.d/$NAME

SELECTED_INSTANCE=$2

#
#       Function that starts the daemon/service.
#
d_start()
{
    # Killing cutycapt instance(s)
    for INSTANCE in $INSTANCES
    do
        if [[ -z $SELECTED_INSTANCE || $SELECTED_INSTANCE -eq $INSTANCE ]]
	then 
		echo -n "Starting CutyCapt instance : $INSTANCE . . . "
        	if [[ -n `ps auxwww | grep loop_thumbnailer | grep $INSTANCE | head -1` ]]; then
            		echo -n " already running!"
        	else
            		$CUTYCAPT_DIR/loop_thumbnailer.sh $PRIMARYQUEUENAME $SECONDARYQUEUENAME $VIEWPORTWIDTH $VIEWPORTHEIGHT $SLEEPCHECK $SCALEDWIDTH $MAXREQUESTS $INSTANCE &
            		echo -n " . . . started!"
        	fi
    	fi
    done
}

#
#       Function that stops the daemon/service.
#
d_stop() {
    # Killing CutyCapt instance(s)
    for INSTANCE in $INSTANCES
    do
        if [[ -z $SELECTED_INSTANCE || $SELECTED_INSTANCE -eq $INSTANCE ]]
	    then 
    	    if [[ -n `ps auxwww | grep loop_thumbnailer | grep $INSTANCE | head -1` ]]; then
                    PID=`ps auxwww | grep loop_thumbnailer | grep $INSTANCE | head -1 | awk '{{ print $2 }}'`
                    echo "-- PID: $PID"
                    kill $PID
                    PID=`ps auxwww | grep CutyCapt| grep -v xvfb | grep -- "--instance=$INSTANCE" | head -1 | awk '{{ print $2 }}'`
                    echo "-- CutyCapt PID: $PID" 
                    kill $PID
            else
                    echo "-- No process to kill for $INSTANCE"
            fi
        fi
    done
}

ACTION="$1"
case "$ACTION" in
    start)
        echo "Starting $DESC: $NAME"
        d_start
        echo "."
        ;;

    stop)
        echo "Stopping $DESC: $NAME"
        d_stop
        echo "."
        ;;

    restart|force-reload)
        echo "Restarting $DESC: $NAME"
        d_stop
        sleep 2
        d_start
        echo "-- DONE"
        ;;

    *)
        echo "Usage: $NAME {start|stop|restart|force-reload}" >&2
        exit 3
        ;;
esac


exit 0
