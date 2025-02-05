#!/bin/sh

PID_FILE="/var/run/aesdsocket.pid"

case "$1" in
    start)
        echo "Starting aesdsocket"
        start-stop-daemon -S -n aesdsocket -p "$PID_FILE" -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket"
        start-stop-daemon -K -n aesdsocket -p "$PID_FILE"
        [ -f "$PID_FILE" ] && rm "$PID_FILE"
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0