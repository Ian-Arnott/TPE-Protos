// this is empty

#include "auth.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

bool parse_user_and_password(const char * string){
    int idx; // current char
    int length = 0;
    bool flag = false; // found :
    for (idx = 0; string[idx] != 0 ; idx++){
        if (!flag && string[idx] == ':'){
            flag = true;
            length = 0; //reset the length counter
        }else{
            if (length >= MAX_USER_LENGTH || !isalnum(string[idx])){
                return false;
            }
        }
        length++;
    }
    if (idx < 1 || flag == false){ // idx < 2 for password length // flag to check : input
        return false;
    }
    return true;
}

int store_connection(int socket_fd, connection * clients){
    int idx = get_user_buffer_idx(clients);
    if (idx == -1){
        //TODO: Logger
        printf("ERROR. No more connections are allowed right now. Try again later\n");
        return 1;
    }

    clients[idx].socket = socket_fd;
    clients[idx].active = true;
    printf("Estoy por hacer store del socket_fd: %d que se va a guardar en el client[%d]", clients[idx].socket, idx);

    return 0;

}

int get_user_buffer_idx(connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (!clients[i].active)
            return i;
    }
    return -1;
}

// static bool checkUserAndPasswordFormat(const char * str){
//     bool separatorSeen = false;
//     for ( int i = 0; str[i] != 0; i++){
//         if (str[i] == ':' && !separatorSeen){
//             separatorSeen = true;
//             if ( str[i + 1] == 0){
//                 return false;
//             }
//         } else if ( str[i] == ':' && separatorSeen){
//             return false;
//         }
//     }
//     return true;
// }