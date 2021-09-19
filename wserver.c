#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <uuid/uuid.h>
#include "request.h"
#include "io_helper.h"

typedef struct estMensaje
{
	long int msg_type;
	char incial[50];
	char final[50];
	char uuid[50];	
} estMensaje;

typedef struct estMensaje2{
	long int msg_type;
	time_t tiempo1;
	time_t tiempo2;
}estMensaje2;

char default_root[] = ".";

//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
	int c;
    char *root_dir = default_root;
    int port = 10000;
    
    while ((c = getopt(argc, argv, "d:p:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port]\n");
	    exit(1);
	}

	//variables para comonicacion
	int msg_id = 0;
	estMensaje mensaje;
	estMensaje2 mensaje2;

	msg_id = msgget((key_t *)123, 0666 | IPC_CREAT);
	long int msgtype;

    // run out of this directory
    chdir_or_die(root_dir);

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    
    
    //Fork que se levanta con el servidor
    int init_fork = fork_or_die();
    if (init_fork == 0){
    	//listener parent
    	//reader
    	printf("listening\n");
    	while(1){

    		int aux = msgrcv(msg_id, (void *)&mensaje2, sizeof(estMensaje2), msgtype, 0);
    		if(-1 == aux){
    			printf("msgrcv fail\n");
    			exit(-1);
    		}

    		if(mensaje2.msg_type == -1){
    			break;
    		}

    		uuid_t out;
    		uuid_generate_random(&out);

    		printf("recibi: %s %s %s\n",ctime(&mensaje2.tiempo1), ctime(&mensaje2.tiempo2),out);
    	}

    	if(msgctl(msg_id,IPC_RMID,0)==1){
    		printf("MSGCTL(IPC_RMID) failed\n");
    	}
    }
    else if(init_fork > 0){
    	//hijo hace requests
    	while (1) {
			struct sockaddr_in client_addr;
			int client_len = sizeof(client_addr);
			int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
	
			//Fork que maneja los requests
			int request_fork = fork_or_die();
			if (request_fork == 0){

				//Toma de tiempo cuando se inicial la peticion
				time_t initialTime;
				initialTime = time(NULL);

				//printf("[son] pid %lu from [parent] pid %lu\n", getpid(), getppid());
				
				request_handle(conn_fd);
				close_or_die(conn_fd);


				//Toma de timepo cuando se finaliza la peticion
				time_t finalTime;
				finalTime = time(NULL);

				mensaje2.msg_type = 1;
				mensaje2.tiempo1 = initialTime;
				mensaje2.tiempo2 = finalTime;

				//mandar info
				int tmp = msgsnd(msg_id, (void *)&mensaje2, sizeof(estMensaje2), 0);
				if(-1 == tmp){
					printf("msgsnd() failed\n");
				}

				if(mensaje2.msg_type == -1){
					break;
				}
				
				printf("le mande: %s %s \n",ctime(&mensaje2.tiempo1), ctime(&mensaje2.tiempo2));

				exit(0);
			}
    	}
    }

    return 0;
}
