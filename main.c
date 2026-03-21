#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
//  /dev/uinput


void emit(int fd, int type, int code, int val)
{
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;

    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

int main()
{   
    int fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    struct uinput_setup usetup;

    ioctl(fd_uinput);
    
    //  /dev/input/event6
    int fd_input = open("/dev/input/event6", O_RDONLY);
    //printf("%d\n",fd);
    struct input_event ie;
    /*
    __u16 type;
	__u16 code;
	__s32 value;
    */
    while(1){
        read(fd_input, &ie, sizeof(ie));

        //printf("type: %d\n", ie.value);
        printf("code: %d\n", ie.code);
        //printf("value: %d\n", ie.value);
    }
    
}