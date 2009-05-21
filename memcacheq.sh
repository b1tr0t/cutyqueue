#! /bin/bash
# set -x
### BEGIN INIT INFO
# Provides:          Start memcacheq server
# Required-Start:    networking
# Required-Stop:     networking
# Default-Start:     2 3 4 5
# Default-Stop:      S 0 1 6
# Short-Description: Memcacheq provides a memcache interface to a queue
# Description:       Memcacheq provides a memcache interface to a queue
### END INIT INFO
#
# Author:  Guillermo Fernandez Castellanos
#          <guillermo.fernandez.castellanos AT gmail.com>.
#
# Version: @(#)fastcgi 0.1 11-Jan-2007 guillermo.fernandez.castellanos AT gmail.com
#

#### MEMCACHE CONFIG OPTIONS
RUNAS_USER="peter"
MEMCACHE_DIR="/usr/local/memcacheq"
DATA_DIR="$MEMCACHE_DIR/data"
TCP_PORT=22201
MEM_CACHE=16 # memory cache in MB
MESSAGE_LENGTH=2048



set -e

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="Memcache queue"
NAME=$0
SCRIPTNAME=/etc/init.d/$NAME

#
#       Function that starts the daemon/service.
#
d_start()
{
    $MEMCACHE_DIR/memcacheq -d -p $TCP_PORT -u $RUNAS_USER -r -H $DATA_DIR -N -L 512 -B $MESSAGE_LENGTH -m $MEM_CACHE > $MEMCACHE_DIR/error.log 2>&1
}

#
#       Function that stops the daemon/service.
#
d_stop() {
    GREP=`ps auxwww | grep memcacheq | grep -v grep | head -1`
    if [[ -n $GREP  ]]; then
            PID=$GREP
            echo "-- PID: $PID"
            kill $PID
    else
            echo "-- No process to kill for memcacheq"
    fi
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
