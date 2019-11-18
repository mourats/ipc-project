# gcc write.c -o write
# gcc read.c -o read
# gcc api.c -o api -lpthread
gcc api.c chat.c -o chat -lpthread
