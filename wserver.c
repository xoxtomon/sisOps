#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "request.h"
#include "io_helper.h"

struct estMensaje
{
	long int msg_type;
	char *incial;
	char *final;	
};

char default_root[] = ".";

//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
    //inicializar my estructura
    struct estMensaje mensaje; 
    int c, msg_id;
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

    // run out of this directory
    chdir_or_die(root_dir);

    // now, get to work

    int listen_fd = open_listen_fd_or_die(port);

    //crear la cola de mensajes
    //lA LLAVE ES ´12345´ PERMISOS DE ESCRIBIR Y LEER
    msg_id = msgget((key_t)12345,0666|IPC_CREAT);
    if(msg_id ==-1){
    	printf("La cola no pudo ser creada con exito.\n");
    }
    
    //Fork que se levanta con el servidor
    int init_fork = fork_or_die();
    if (init_fork > 0){
    	//listener parent >0
    	while (1) {
			struct sockaddr_in client_addr;
			int client_len = sizeof(client_addr);
			int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
	
			//Fork que maneja los requests
			int request_fork = fork_or_die();
			if (request_fork == 0){
				time_t initialTime;
				char *initialTimeString;
				initialTime = time(NULL);
				initialTimeString = ctime(&initialTime);

				time_t finalTime;
				char *finalTimeString;
				//printf("[son] pid %lu from [parent] pid %lu\n", getpid(), getppid());
		
				request_handle(conn_fd);
				close_or_die(conn_fd);

				finalTime = time(NULL);
				finalTimeString = ctime(&finalTime);

				//sender
				mensaje.msg_type = 1;
				mensaje.incial = initialTimeString;
				mensaje.final = finalTimeString;
				//printf("%d\n",mensaje.msg_type);
				printf("%s %s\n",mensaje.incial, mensaje.final );

				//enviar el mensaje
				msgsnd(msg_id, &mensaje, sizeof(mensaje),0);
				exit(0);
			}
    	}
    }
    else if(init_fork == 0){
    	//listener child == 0
    	//reader
    	printf("listening\n");

    	while(1){
    		msgrcv(msg_id, &mensaje, sizeof(mensaje), 1, 0);

    		printf("desde listener: %s %s\n",mensaje.incial, mensaje.final);

    		msgctl(msg_id, IPC_RMID, NULL);
    	}	
    }
    return 0;
}
