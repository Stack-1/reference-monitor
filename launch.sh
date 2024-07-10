cd syscall-table-discoverer/
make
cd ..
cd log-filesystem
rm -r mount/
make
cd ..
cd reference-monitor
make
cd user/
gcc switch_reconfigure.c -o switch
sudo ./switch 2
gcc user.c
sudo ./a.out
