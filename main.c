#include "stapp.e"
#include "mezzoCondiviso.e"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "types.h"
#define MAXSIZE 30
#define PORTNUM 5000
#define SOCKET_ERROR -1




void *ptr;
int error;
int rc,i;
pthread_t sta_thread1;
pthread_t sta_thread2;
pthread_t sta_thread3;
pthread_t app_thread1;
pthread_t app_thread2;
pthread_t app_thread3;
int staConnesse = 0;
char *macAddressSta1 = "00aa21b9c6a1\0";
char *macAddressSta2 = "00aa21b9c6b2\0";
char *macAddressSta3 = "00aa21b9c6c3\0";



/*-----------------------------------------------INIT SOCKET PAIR---------------------------------------------------------------------*/

void init_socketPair(int *fdsockpair){
    int ris;


    ris=socketpair(AF_UNIX,SOCK_STREAM,0,fdsockpair);
    if (ris < 0)
    {
        perror("socketpair failed: ");
        fflush(stderr);
        exit(1);
    }
    else
    {
        /*printf("socketpair fds succeed: \n");*/
        fflush(stdout);
    }


}


/*-----------------------------------------------INIT APP---------------------------------------------------------------------*/

void init_app(PARAMETRI *pPar, char* macDest, pthread_t *app_t, int *fdsockpair,pthread_t *app_thread){


    pPar = (PARAMETRI*)malloc(sizeof(PARAMETRI));
    if(!pPar)
    {
        perror("malloc failed: ");
        exit(0);
    }

    pPar->macDest = macDest;
    pPar->fd=fdsockpair[1];

    rc = pthread_create(&(app_t[0]), NULL, App, pPar);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n",rc);
        exit(-1);
    }
    else
    {
        *app_thread = app_t[0];
    }

}




/*-----------------------------------------------INIT STA---------------------------------------------------------------------*/

void init_sta(PARAMETRI *pPar, char* macDest, pthread_t *app_t, int *fdsockpair, pthread_t *sta_t){

    pPar = (PARAMETRI*)malloc(sizeof(PARAMETRI));
    if(!pPar)
    {
        perror("malloc failed: ");
        exit(0);
    }
    pPar->fd=fdsockpair[0];
    pPar->macDest = macDest;

    rc = pthread_create(&(app_t[1]), NULL, Sta, pPar);
    if (rc)
    {
        printf("ERROR; return code from pthread_create() is %d\n",rc);
        exit(-1);
    }
    else
    {
        staConnesse++;
        *sta_t = app_t[1];  /*assegniamo il thread alla sta per riconoscerlo dal MC*/


    }
}


/*-----------------------------------------------JOIN PTHREAD---------------------------------------------------------------------*/


void joinPthread(pthread_t *app_t, void *ptr){

    for(i=0;i<2;i++)
    {
        error=pthread_join( app_t[i] , (void*) &ptr );
        if(error!=0){
            printf("pthread_join() failed: error=%d\n", error );
            exit(-1);
        }
        else {

        }
    }

    fflush(stdout);

}


/*-----------------------------------------------MAIN---------------------------------------------------------------------*/

int main(int argc, char const *argv[])
{
    pthread_t mc;
    PARAMETRI *pPar1 = NULL;
    pthread_t app_t1[2];
    int fdAppSta1[2];
    void *ptr1 = NULL;
    PARAMETRI *pPar2 = NULL;
    pthread_t app_t2[2];
    int fdAppSta2[2];
    void *ptr2 = NULL;
    PARAMETRI *pPar3 = NULL;
    pthread_t app_t3[2];
    int fdAppSta3[2];
    void *ptr3 = NULL;

    rc = pthread_create(&mc, NULL, mezzoCondiviso, NULL);

    sleep(2);/*mi assicuro che il thread del MC sia partito*/

    /*socketpair per comunicare tra app e sta*/
    init_socketPair(fdAppSta1);
    init_socketPair(fdAppSta2);
    init_socketPair(fdAppSta3);



    /*creo i thread per le STA */

    init_sta(pPar1, macAddressSta1, app_t1, fdAppSta1, &sta_thread1);
    init_sta(pPar2, macAddressSta2, app_t2, fdAppSta2, &sta_thread2);
    init_sta(pPar3, macAddressSta3, app_t3, fdAppSta3, &sta_thread3);
    printf("THREADS STA CREATI\n");
    fflush(stdout);

    /*creo i thread per le APP*/
    while(staConnesse < 2);
    init_app(pPar1,macAddressSta1, app_t1, fdAppSta1, &app_thread1);
    init_app(pPar2,macAddressSta2, app_t2, fdAppSta2, &app_thread2);
    init_app(pPar3,macAddressSta3, app_t3, fdAppSta3, &app_thread3);
    printf("THREADS APP CREATI\n");
    fflush(stdout);
    /*faccio il join tra i thread delle APP e delle STA*/
    joinPthread(app_t1, ptr1);
    joinPthread(app_t2, ptr2);
    joinPthread(app_t3, ptr3);


    return 0;
}
