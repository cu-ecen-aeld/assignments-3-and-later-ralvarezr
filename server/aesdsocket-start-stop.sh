#!/bin/sh

DAEMON=/usr/bin/aesdsocket
PIDFILE=/var/run/aesdsocket.pid
NAME=aesdsocket

case "$1" in
    start)
        echo "Starting $NAME..."
        start-stop-daemon --start --background \
            --exec $DAEMON -- -d \
            --make-pidfile --pidfile $PIDFILE
        ;;
    stop)
        echo "Stopping $NAME..."
        start-stop-daemon --stop --pidfile $PIDFILE --retry TERM/5/KILL/1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0
