cd ../reference-monitor/user/
gcc switch_reconfigure.c -o switch
./switch 2 1234
gcc user.c
./a.out
gcc remove.c
./a.out