**This file is for my own convenience**

----------------------------------------------------------------------------------------

1) Alternatively, if you prefer to run the command inside WSL, do this:
cp /mnt/c/Users/Sid/Documents/OS-MiniProject/* .

2) Want to copy an entire folder with its contents? Add -r:
cp -r /mnt/c/Users/Sid/Documents/OS-MiniProject .

3) If you want to copy everything from the current WSL directory to C:\Users\YourName\Documents\myfiles, run:
cp * /mnt/c/Users/Sid/Documents/OS-MiniProject/

4) To copy directories and files recursively:
cp -r * /mnt/c/Users/Sid/Documents/OS-MiniProject/

---------------------------------------------------------------------------------------

gcc server.c admin.c faculty.c student.c utils.c -o server -pthread
gcc client.c -o client

./server
./client

---------------------------------------------------------------------------------------

Author: Siddharth Goswami
Rollno: IMT2023542
