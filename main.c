#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>

#define TERMINATE_KEYMAP 119    //if KEY_PAUSE is pressed

void emit(int fd, int type, int code, int val)
{
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val; //can be 0, 1, 2

    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}


int main()
{
    //SETUP ACCESS TO /INPUT/EVENT6
    int fd_input = open("/dev/input/event6", O_RDONLY);
    if(fd_input < 0){
        perror("Error opening /input");
        return fd_input;
    }  
    //SETUP ACCESS TO /UINPUT
    int fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if(fd_uinput < 0){
            perror("Error opening /uinput");
            printf("error error");
            return fd_uinput;
        }

    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    for(int i=0; i<255; i++){
        ioctl(fd_uinput, UI_SET_KEYBIT, i);
    }
    
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 0;
    strcpy(usetup.name, "keyswap_device");
    
    ioctl(fd_uinput, UI_DEV_SETUP, &usetup);
    ioctl(fd_uinput, UI_DEV_CREATE);
    sleep(1);

    struct input_event ie;
    memset(&ie, 0, sizeof(ie));

    read(fd_input, &ie, sizeof(ie));
    if(ie.value == 0){
        emit(fd_uinput, EV_KEY, ie.code, 0);
        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
    }
    ioctl(fd_input, EVIOCGRAB, 1);

    while(1){  
        read(fd_input, &ie, sizeof(ie));

        //DELETE -> RIGHT_CTRL
        if(ie.code == KEY_DELETE && ie.value == 1){
            
            /*  THIS LEADS TO WEIRD BEHAVIOUR; still unclear why
            ie.code = KEY_B;
            write(fd_uinput, &ie, sizeof(ie));
            */
           
            emit(fd_uinput, EV_KEY, KEY_RIGHTCTRL, 1);
            emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
            emit(fd_uinput, EV_KEY, KEY_RIGHTCTRL, 0);
            emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
        }
        else if(ie.code == TERMINATE_KEYMAP){
            break;
        }
        else{
            write(fd_uinput, &ie, sizeof(ie));
            //printf("code: %d\n", ie.code);
        }            
    }    
    ioctl(fd_input, EVIOCGRAB, 0);
    ioctl(fd_uinput, UI_DEV_DESTROY);
    close(fd_input);
    close(fd_uinput);
    return 0;
}


/*
<  86
leftalt 56
rightalt 100
end 107
del 111
*/