#include <linux/input.h>                                                                                                                                                                                     
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#define INPUT_DEV "/dev/input/event0"

int main(int argc, char * const argv[])
{
    int fd = 0;

    struct input_event event;

    int ret = 0;

    fd = open(INPUT_DEV, O_RDONLY);

    while(1){
        ret = read(fd, &event, sizeof(event));
        if(ret == -1) {
            perror("Failed to read.\n");
            exit(1);
        }

        if(event.type != EV_SYN) {
            printf("type:%d, code:%d, value:%d\n", event.type, event.code, event.value);
        }
    }   

    return 0;
}

