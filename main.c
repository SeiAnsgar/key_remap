#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>



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

int write_uinput()
{
//ioctl(fd_uinput);
    int fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd_uinput < 0){
        perror("Error opening /uinput");
        return fd_uinput;
    }
    struct uinput_setup usetup;
    /*
    struct input_id id;
	char name[UINPUT_MAX_NAME_SIZE];
	__u32 ff_effects_max;
    */

    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_B);
    
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
}

int read_input()
{
    //  /dev/input/event6
    int fd_input = open("/dev/input/event6", O_RDONLY);
    if(fd_input < 0){
        perror("Error opening /input");
        return fd_input;
    }
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

int main()
{   
    read_input();
}