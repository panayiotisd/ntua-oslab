#!/bin/bash

# clean directory
rm -f .hello_there
rm -f magic_mirror
rm -f pipe*
rm -f .hey_there
rm -f bf*
# delete savegame
rm -f riddle.savegame
# enable/disable game/tech hints
#export I_HATE_GAME_HINTS=YES
export I_NEED_TECH_HINTS=YES

# challenge no.0
touch .hello_there
# challenge no.1
chmod -w .hello_there
# challenge no.3
export ANSWER=42
# challenge no.4
mkfifo magic_mirror
# challenge no.5
exec 99>&1
# challenge no.6
mkfifo pipeA pipeB
exec 33<>pipeA
exec 34<>pipeA
exec 53<>pipeB
exec 54<>pipeB
# challenge no.7
ln .hello_there .hey_there
# challenge no.8
for i in {0..9}
do
	truncate -s 1G bf0$i
	echo HelloWorld! >> bf0$i
done

# run riddle in background
./riddle &

# challenge no.9
echo $(($(pidof riddle)+1)) | nc -lp 49842 &
# challenge no.2
killall -SIGSTOP riddle
killall -SIGCONT riddle

echo
sleep 1
