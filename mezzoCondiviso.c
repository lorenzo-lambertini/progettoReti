#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>		/* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>		/* inet(3) functions */
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "types.h"
#define BUFMAX 2041
#define PORTNUM 5000
#define SOCKET_ERROR -1
/*#define TEST*/
#define RUN
typedef struct sockaddr SA;
struct timeval *timeoutCRC;
extern char *macAddressSta1;
extern char *macAddressSta2;
extern char *macAddressSta3;

int ModificaCrc();
int spedisci(int fd, const void *buffer, int buffer_length);
int calcolaTempo(struct timeval* end, struct timeval* start);
char* leggiBuffer(int lengthBuffer, char* bufferDaLeggere, int fd);
void inizializzaTempo(struct timeval *tempo);

static int contaclient = 0;
static int tempoCRC = 0;
void *mezzoCondiviso(void* p) {
    struct sockaddr_in Local, Cli;
    int socketfd, newsocketfd, OptVal, ris;
    unsigned int cliLen;
    int i, nready, maxfd, times = 0, client[3], contaSta,staCorrente,
        sizeBufferToRead, lunPachettoNoCRC, sizeToPacketLength = 0;

    struct timeval *istantePartenza, *istanteAttuale;
    fd_set Rset;
    struct frame *revert, fr, *scartato;
    char *bufFinale;
    char bufVuoto[BUFMAX];


    timeoutCRC = NULL; /* TIMEOUTCRC = NULL per fare attendere la select fino alla scrittura di qualcuno */
    istantePartenza = (struct timeval*) malloc(sizeof (struct timeval));
    istanteAttuale = (struct timeval*) malloc(sizeof (struct timeval));
    if ((!istanteAttuale) || (!istantePartenza)) {
        perror("malloc failed: ");
        fflush(stderr);
        exit(0);
    }
    /* creazione del socketfd */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == SOCKET_ERROR) {
        printf("socket() failed, \n");
        fflush(stdout);
        exit(1);
    }
    OptVal = 1;
    ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *) &OptVal, sizeof (OptVal));
    if (ris == SOCKET_ERROR) {
        printf("setsockopt() SO_REUSEADDR failed, \n");
        fflush(stdout);
        exit(1);
    }
    /* settiamo i parametri del nuovo socket */
    memset(&Local, 0, sizeof (Local));
    Local.sin_family = AF_INET;
    Local.sin_addr.s_addr = htonl(INADDR_ANY);
    Local.sin_port = htons(PORTNUM);
    ris = bind(socketfd, (struct sockaddr*) &Local, sizeof (Local));
    if (ris == SOCKET_ERROR) {
        printf("bind() failed\n");
        fflush(stdout);
        exit(1);
    }


    ris = listen(socketfd, 100);
    if (ris == SOCKET_ERROR) {
        printf("listen() failed\n");
        fflush(stdout);
        exit(1);
    }

    do { /*accetto le connessioni dalle 3 sta e le aggiungo ai client per la select*/
        memset(&Cli, 0, sizeof (Cli));
        cliLen = sizeof (Cli);
        newsocketfd = accept(socketfd, (struct sockaddr*) &Cli, &cliLen);
        if (newsocketfd >= 0) {
            client[contaclient] = newsocketfd;
            contaclient++;

#ifdef TEST
            printf("aggiunto client[%d] con valore: %d\n", contaclient, newsocketfd);
            fflush(stdout);
#endif
        }
    } while (((newsocketfd < 0)&&(errno == EINTR)) || (contaclient < 3));

    if (newsocketfd == SOCKET_ERROR) {
        printf("accept() failed\n");
        fflush(stdout);
        exit(1);
    }




    while (1) {

        FD_ZERO(&Rset);
        for (i = 0; i < 3; i++) {
            FD_SET(client[i], &Rset);
        }
        maxfd = client[0];
        for (i = 0; i < 3; i++) {
            if (maxfd < client[i]) {
                maxfd = client[i];
            }
        }

        nready = select(maxfd + 1, &Rset, NULL, NULL, timeoutCRC); /*select*/
        if (timeoutCRC == NULL) {
            timeoutCRC = (struct timeval*) malloc(sizeof (struct timeval));
        }
        timeoutCRC->tv_sec = 0;
        timeoutCRC->tv_usec = 0;
        if (nready == -1) {
            printf("Errore nella select mc \n");
            fflush(stdout);
            exit(1);
        }
        if (nready > 0 && tempoCRC == 0) {/*qualcuno sulla select*/
#ifdef TEST
            printf(RED"valore nready %d\n"DEFAULTCOLOR,nready);
            fflush(stdout);
#endif
            for (i=0; i<FD_SETSIZE; i++)
            {
                if ( FD_ISSET( i, &Rset) ) {
                    staCorrente = i;
                }
            }
            times = gettimeofday(istantePartenza, NULL);
            if (times == -1) {
                printf("Errore nella gettimeofday /n");
                fflush(stdout);
                exit(1);
            }

            if (FD_ISSET(staCorrente, &Rset)) { /* leggo il pacchetto ricevuto dalla STA corrente */
                sizeToPacketLength = (sizeof (fr.ctrl) + sizeof (fr.duration) + sizeof (fr.packet_length));
                bufFinale = malloc(2041 * sizeof (char)); /* buffer packet */
                leggiBuffer(sizeToPacketLength, bufFinale, staCorrente);/*leggo la grandezza della frame*/
                revert = (frame *) bufFinale;
                sizeBufferToRead = (revert->packet_length - (sizeToPacketLength)); /*byte da leggere ancora*/
                leggiBuffer(sizeBufferToRead, bufFinale + sizeToPacketLength, staCorrente);/*leggo la frame*/

#ifdef TEST
                printf(ORANGE "received from socket %d %s\n" DEFAULTCOLOR, staCorrente, revert->payload);
                fflush(stdout);
                printf(ORANGE"CRC : %i\n"DEFAULTCOLOR, revert->CRC);
                fflush(stdout);
                printf(ORANGE"invio senza CRC\n"DEFAULTCOLOR);
                fflush(stdout);
#endif
#ifdef RUN
                printf(ORANGE "il mezzo condiviso ha ricevuto da %d il messaggio %s\n" DEFAULTCOLOR, staCorrente, revert->payload);
                fflush(stdout);
#endif

                lunPachettoNoCRC = revert->packet_length - 1; /*tolgo il CRC*/

                if(strcmp(revert->addr.addr1,macAddressSta1) == 0){/*se sono sta1 spedisco la frame senza CRC a sta2*/
                    spedisci(client[1], (revert), lunPachettoNoCRC);
                } else if(strcmp(revert->addr.addr1,macAddressSta2) == 0){/*se sono sta2 spedisco la frame senza CRC*/
                    if (strcmp(revert->addr.addr2, macAddressSta1) == 0) {
                        spedisci(client[0], (revert), lunPachettoNoCRC);/*se il mac destinatario è 00aa21b9c6a1 la spedisco a sta1*/
                    }
                    if (strcmp(revert->addr.addr2, macAddressSta3) == 0) {
                        spedisci(client[2], (revert), lunPachettoNoCRC);/*se il mac destinatario è 00aa21b9c6a2 la spedisco a sta3*/
                    }
                } else {
                    spedisci(client[1], (revert), lunPachettoNoCRC);/*se sono sta3 spedisco la frame senza CRC a sta2*/
                }
                tempoCRC = 1;

            } /*fine FD_ISSET*/
            inizializzaTempo(istanteAttuale);/*salvo l'istante attuale per l'attesa del NO CRC*/
            timeoutCRC->tv_usec = 2000000 - calcolaTempo(istanteAttuale, istantePartenza); /*calcolo il tempo passato*/
        } else if (nready == 0) {/*se il timeoutCRC della select è scaduto*/
            if (revert->CRC != 0) {
                revert->CRC = ModificaCrc(); /*cambio il CRC al 10%*/
            }
            if (strcmp(revert->addr.addr1, macAddressSta1) == 0) {
                spedisci(client[1], ((char*) &(revert->CRC)), 1);/*spedisco il CRC*/
#ifdef TEST
                printf("invio CRC da sta1 a sta2\n");
                fflush(stdout);
#endif
            } else {
                if (strcmp(revert->addr.addr1, macAddressSta2) == 0) {
                    if (strcmp(revert->addr.addr2, macAddressSta1) == 0) {
                        spedisci(client[0], ((char*) &(revert->CRC)), 1);/*spedisco il CRC*/
#ifdef TEST
                        printf("invio CRC da sta2 a sta1\n");
                        fflush(stdout);
#endif
                    }
                    if (strcmp(revert->addr.addr2, macAddressSta3) == 0) {
                        spedisci(client[2], ((char*) &(revert->CRC)), 1);/*spedisco il CRC*/
#ifdef TEST
                        printf("invio CRC da sta2 a sta3\n");
                        fflush(stdout);
#endif
                    }
                } else {
                    spedisci(client[1], ((char*) &(revert->CRC)), 1);/*spedisco il CRC*/
#ifdef TEST
                    printf("invio CRC da sta3 a sta2\n");
                    fflush(stdout);
#endif
                }
            }
            timeoutCRC = NULL;
            tempoCRC = 0;

        } else if(nready > 0 && tempoCRC == 1) { /*qualcuno sulla select o il tempo CRC non è passato*/
            if (revert->CRC == 1) {
                revert->CRC = 0;
                printf(RED"pacchetto CORROTTO CRC = 0: %s\n"DEFAULTCOLOR, revert->payload);
                fflush(stdout);
            }
            for (contaSta = 0; contaSta < 3; contaSta++) {
                if (FD_ISSET(client [contaSta], &Rset)) {/*se qualcuno è sulla select lo scarto come corrotto*/
                    if (revert->CRC == 1) {
                        revert->CRC = 0;
                    }
                    sizeToPacketLength = (sizeof (fr.ctrl) + sizeof (fr.duration) + sizeof (fr.packet_length));
                    leggiBuffer(sizeToPacketLength, bufVuoto, client[contaSta]);/*leggo la size della frame da scartare*/
                    scartato = (frame *) bufVuoto;
                    sizeBufferToRead = (scartato->packet_length - sizeToPacketLength);
                    leggiBuffer(sizeBufferToRead, bufVuoto + sizeToPacketLength, client[contaSta]);/*leggo la frame da scartare*/
                    printf(ORANGE"pacchetto scartato per collisione dal MC : %s\n"DEFAULTCOLOR, scartato->payload);
                    fflush(stdout);
                }
            }
        }
        if(tempoCRC == 1){/*se ho inviato un pacchetto senza CRC inizio a calcolare il tempo di attesa prima dell' invio del CRC*/
            inizializzaTempo(istanteAttuale);
            if (2000000 - calcolaTempo(istanteAttuale, istantePartenza) > 0) {/*se non è ancora passato il tempo del CRC*/
                timeoutCRC->tv_usec = 2000000 - calcolaTempo(istanteAttuale, istantePartenza);
            } else {
                timeoutCRC->tv_sec = 0;
                timeoutCRC->tv_usec = 0;
                tempoCRC = 0;
            }
        }
    }


}

/**
 *
 *modifica il crc al 10%
 *#return CRC modificato
 */
int ModificaCrc() {
    int n = rand() % 100;
    if (n < 10) {
        return 0;
    } else {
        return 1;
    }
}

/**
 *
 * @param buffer messaggio da spedire
 * @param buffer_length dimensione del buffer
 * @param fd file descriptor che rappresenta il socket
 * @return numero di byte letti
 */
int spedisci(int fd, const void *buffer, int buffer_length) {
    int ris;
    do {
        ris = send(fd, buffer, buffer_length, MSG_NOSIGNAL);

    } while ((ris < 0)&&(errno == EINTR));
    if (ris < 0) {
        printf("errore send(spedisci) ris < 0  \n");
        fflush(stdout);
        exit(1);
    } else if (ris == 0) {
        printf("errore send(spedisci) ris = 0  \n");
        fflush(stdout);
        exit(1);
    } else {
#ifdef TEST
        printf(BLU"abbiamo spedito : %d\n"DEFAULTCOLOR, ris);
        fflush(stdout);
#endif
    }
    return ris;
}
