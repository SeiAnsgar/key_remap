#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>

#define TERMINATE_KEYMAP 119    //if KEY_PAUSE is pressed
#define KEYMAP_LENGTH 5
#define CUSTOM_SIZE 2

void emit(int fd, int type, int code, int val);
int setup_keymap();
void handle_keycheck(int fd_uinput, int *is_custom_key, struct input_event *ie);
void buffer_handler(struct input_event new_ie);

/*#####################TODO:##########################
detect key-combo
    -maybe buffer that holds all keys that are currently pressed.
    ->add key to buff if: key is pressed, key ist not in buff
    ->remove key from buff if key is released

#####################################################*/

typedef struct{
    int original;
    int custom[CUSTOM_SIZE];
    int is_shortcut;
}Keymap;
Keymap keymap_list[KEYMAP_LENGTH];

struct input_event input_buffer[2] = {0};


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

        buffer_handler(ie);
        printf("buffer[0] code: %i\n",input_buffer[0].code);
        printf("buffer[1] code: %i\n",input_buffer[1].code);

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

    //memset(keymap_list, -1, sizeof(keymap_list));   NO LONGER NEEDED

    //Keymap k0 = {.original=KEY_A, .custom=KEY_B};
    //keymap_list[0] = k0;

    Keymap k1 = {.original=KEY_2, .custom={KEY_102ND, KEY_3}};
    keymap_list[1] = k1;

    Keymap k2 = {.original=KEY_0, .custom={KEY_LEFTCTRL, KEY_V}, .is_shortcut=1};
    keymap_list[2] = k2;

    Keymap k2 = {.original=KEY_0, .custom={KEY_LEFTCTRL, KEY_V}, .is_shortcut=1};
    keymap_list[2] = k2;

    for(int i=0; i<KEYMAP_LENGTH; i++)
    {
        printf("show keymap: %i\n",keymap_list[i].original);
    }
    return 1;
}


void handle_keycheck(int fd_uinput, int *is_custom_key, struct input_event *ie)
{
    /*
    input event ie:
        type:
        code: KEY_CODE
        value:  1->pressed, 0->released
    */
    int buffer_flag = 0;
    *is_custom_key = 0;      
            
    //input buffer filled
    if (input_buffer[0].value == 1 && input_buffer[1].value == 1){
        for(int i=0; i<KEYMAP_LENGTH; i++)
        {
            if(input_buffer[0].code == keymap_list[i].original){
                buffer_flag++;
            }
            if(input_buffer[0].code == keymap_list[i].original){
                buffer_flag++;
            }
            if(buffer_flag == 2)
            {
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[0], 1);
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[1], 1);
                emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[0], 0);
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[1], 0);
                emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                
                *is_custom_key = 1;
                buffer_flag = 0;
                break;
            }
        
        }
    }
    
    
    else if(ie->value == 1){     //key_down event
        for(int i=0; i<KEYMAP_LENGTH; i++)
        {
            //keycode ie is in keymap
            if(ie->code == keymap_list[i].original && keymap_list[i].is_shortcut == 0){
                for(int k=0; k<CUSTOM_SIZE; k++) //send all keycoeds in custom if set
                {   
                    if(keymap_list[i].custom[k] != -1){
                        emit(fd_uinput, EV_KEY, keymap_list[i].custom[k], 1);
                        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                        emit(fd_uinput, EV_KEY, keymap_list[i].custom[k], 0);
                        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                    }
                }
                *is_custom_key = 1;
                break;
            }
            //for sending shortcuts e.g ctrl + v
            else if(ie->code == keymap_list[i].original && keymap_list[i].is_shortcut){
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[0], 1);
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[1], 1);
                emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[0], 0);
                emit(fd_uinput, EV_KEY, keymap_list[i].custom[1], 0);
                emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                *is_custom_key = 1;
                break;
            }
        }
    }
    if(*is_custom_key == 0){ 
        write(fd_uinput, ie, sizeof(*ie));
    }
    //if(ie->code == TERMINATE_KEYMAP){
    //    break;
    //}
}

void buffer_handler(struct input_event new_ie)
{
    if(new_ie.value == 1 && input_buffer[0].code == 0){
        //write to buff[0]
        input_buffer[0] = new_ie;
    }
    //else if(new_ie.value == 1 && new_ie.code != input_buffer[1].code){
    else if(new_ie.value == 1 && input_buffer[1].code == 0 ){
        //write to buff[1]
        input_buffer[1] = new_ie;
    }
    else if(new_ie.value == 0 && new_ie.code == input_buffer[0].code){
        //remove from buff[0]
        memset(&input_buffer[0],0,sizeof(struct input_event));
    }
    else if(new_ie.value == 0 && new_ie.code == input_buffer[1].code){
        //remove from buff[1]
        memset(&input_buffer[1],0,sizeof(struct input_event));
    }    
}




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