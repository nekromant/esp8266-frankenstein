
port=10102

trap 'kill $pid' INT

x=/tmp/tex$$
y=/tmp/tey$$
rm -f $x $y
mkfifo $x
mkfifo $y

nc -l 10102 < $x > $y & pid=$1
cat > $x < $y
