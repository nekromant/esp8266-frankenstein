
if [ "$2" = "" ]; then
	echo "need echo server: host port"
	echo "need nc (netcat) and pv (pipe monitor)"
	exit 1
fi

host="$1"
port="$2"

stop ()
{
	kill $flood
	echo ""
	echo "done"
	exit 0
}

trap stop INT

fifo1=/tmp/echo1$$
fifo2=/tmp/echo2$$
rm -f $fifo1 $fifo2
mkfifo $fifo1 $fifo2

(while true; do cat /etc/services; done) \
	| tee -a $fifo1 >> $fifo2 & flood=$!

nc $host $port < $fifo1 \
	| pv \
	| cmp - $fifo2

