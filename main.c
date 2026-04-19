#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>

#define TERMINATE_KEYMAP 119    //if KEY_PAUSE is pressed
#define KEYMAP_LENGTH 5


void emit(int fd, int type, int code, int val);
int setup_keymap();
void handle_keycheck(int fd_uinput, int *is_custom_key, struct input_event *ie);

/*#####################TODO:##########################
add key combo support
    -> read keycombo
    -> write keycombo   -ok
    ->how is info stored?

add macro support 
    ->send n keypresses at once
    ->how is macro saved?
#####################################################*/

typedef struct{
    int original;
    int custom;
}Keymap;
Keymap keymap_list[KEYMAP_LENGTH];


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
    read(fd_input, &ie, sizeof(ie));
    
    if(ie.value == 0){
        emit(fd_uinput, EV_KEY, ie.code, 0);
        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
    }
    ioctl(fd_input, EVIOCGRAB, 1);

    setup_keymap();

    int is_custom_key = 0;
    while(1){  
        read(fd_input, &ie, sizeof(ie));
        
        //debug messages
        printf("------------------------------------------\n");
        printf("ie type: %i\n", ie.type);
        printf("ie code: %i\n", ie.code);
        printf("ie value: %i\n", ie.value);
        printf("------------------------------------------\n");
        
        handle_keycheck(fd_uinput, &is_custom_key, &ie);
    }
        
    ioctl(fd_input, EVIOCGRAB, 0);
    ioctl(fd_uinput, UI_DEV_DESTROY);
    close(fd_input);
    close(fd_uinput);
    return 0;
}


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


int setup_keymap(){

    memset(keymap_list, -1, sizeof(keymap_list));

    //Keymap k0 = {.original=KEY_A, .custom=KEY_B};
    //keymap_list[0] = k0;

    //Keymap k1 = {.original=KEY_Y, .custom=KEY_Z};
    //keymap_list[1] = k1;

    Keymap k2 = {.original=KEY_1, .custom=KEY_3};
    keymap_list[2] = k2;

    for(int i=0; i<KEYMAP_LENGTH; i++)
    {
        printf("show keymap: %i\n",keymap_list[i].original);
    }
    return 1;
}


void handle_keycheck(int fd_uinput, int *is_custom_key, struct input_event *ie)
{
    *is_custom_key = 0;
    for(int i=0; i<KEYMAP_LENGTH; i++)
    {
        //send keycode ie is in keymap AND is pressed
        if(ie->code == keymap_list[i].original && ie->value == 1){
            emit(fd_uinput, EV_KEY, keymap_list[i].custom, 1);
            emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
            emit(fd_uinput, EV_KEY, keymap_list[i].custom, 0);
            emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
            *is_custom_key = 1;
            break;
        }
    }
    if(*is_custom_key == 0){ 
        write(fd_uinput, ie, sizeof(*ie));
    }
    //if(ie->code == TERMINATE_KEYMAP){
    //    break;
    //}
}



/*  
SEND KEY COMBO

    l -> ctrl + v

    emit(fd_uinput, EV_KEY, KEY_RIGHTCTRL, 1);
    emit(fd_uinput, EV_KEY, KEY_V, 1);

    emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
    emit(fd_uinput, EV_KEY, KEY_RIGHTCTRL, 0);
    emit(fd_uinput, EV_KEY, KEY_V, 0);
    emit(fd_uinput, EV_SYN, SYN_REPORT, 0);

*/



/*
ALTERNATIVE datastructure: linked list; currently used: array

typedef struct{
    Keymap km;
    Node *next;
}Node;

Node* setup_keymap(){
    Node *head = malloc(sizeof(Node));
    Node *n1 = malloc(sizeof(Node));

    Keymap k0{.original = 1, .custom = 2}
    head->km = k0;
    head->next = & 

    Keymap k1{.original = 10, .custom = 20}
    head->km = k1;
    head->next = NULL
}
*/