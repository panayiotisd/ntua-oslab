#!/bin/bash

# clean directory
rm -f .hello_there
rm -f magic_mirror
rm -f .hey_there
rm -f bf00
# delete savegame
#rm -f riddle.savegame

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
# challenge no.7
ln .hello_there .hey_there
# challenge no.8
truncate -s 1025M bf00
# challenge no.9
#nc -lp 49842


# enable/disable game and tech hints
#export I_HATE_GAME_HINTS=1
export I_NEED_TECH_HINTS=1
# run riddle in background
./riddle &

# challenge no.2
kill -SIGSTOP $(pidof riddle)
kill -SIGCONT $(pidof riddle)
# challenge no.6
#synchro
# challenge no.9
#response to client
