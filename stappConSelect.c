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
#include <sys/time.h>
#define SIZE_PAYLOAD 27
#define MAXSIZEMAC 12
#define PORTNUM 5000
#define SOCKET_ERROR -1
#define max(x,y) ((x) > (y) ? (x) : (y))
/*#define TEST*/



char *strerror_r(int errnum);
int Send(char* macDest, char* bufferMessaggio, int sizeBufferToReaduf);
int Recv(char* macRecv, char* bufferMessaggio, int *length);
char* leggiBuffer(int lengthBuffer, char* bufferMessaggio, int parsta);
frame package_construction(int parsta);
int selectStazione(int socketPair, int listenFd);
int spedisci(int fd, const void *buffer, int buffer_length);
frameRecv creaFrameRicevuta(int packet_length, char* payload, char* addrMittente, char* addrDestinatario, int isCorrotto);
struct frame creaFrameAck(frame frameAck, frame *revert);
void inizializzaTempo(struct timeval *tempo);
int calcolaTempo(struct timeval* end, struct timeval* start);

extern char *macAddressSta1;
extern char *macAddressSta2;
extern char *macAddressSta3;
extern pthread_t sta_thread1;
extern pthread_t sta_thread2;
extern pthread_t sta_thread3;
extern pthread_t app_thread1;
extern pthread_t app_thread2;
extern pthread_t app_thread3;
extern struct timeval *timeoutCRC;
PARAMETRI ParApp1;
PARAMETRI ParApp2;
PARAMETRI ParApp3;
PARAMETRI ParSta1;
PARAMETRI ParSta2;
PARAMETRI ParSta3;
int sizepkg = 0;
struct frame fr;
char *sizeMSG = "28";



/**
 *
 *questo metodo viene richiamato dal thread della Stazione con la quale gestisco le trasmissioni dei pacchetti con l'Applicazione e il MC
 *@param *p struttura passata nella creazione del thread
 *
 */
void *Sta(void *p) {

    int ris, mezzoCondivisoFd, OptVal, staFD;
    struct sockaddr_in Serv;

    mezzoCondivisoFd = socket(AF_INET, SOCK_STREAM, 0);
    OptVal = 1;
    ris = setsockopt(mezzoCondivisoFd, SOL_SOCKET, SO_REUSEADDR, (void *) &OptVal, sizeof (OptVal));

    memset(&Serv, 0, sizeof (Serv));
    Serv.sin_family = AF_INET;
    Serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    Serv.sin_port = htons(PORTNUM);

    ris = connect(mezzoCondivisoFd, (struct sockaddr*) &Serv, sizeof (Serv));/*connetto la sta al MC*/
    if (ris == SOCKET_ERROR) {
        printf("connect() failed, Err: %d \"%s\"\n", errno, strerror(errno));
        fflush(stdout);
        exit(1);
    }

    if (pthread_self() == sta_thread1) {
        ParSta1 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        staFD = ParSta1.fd;/*salvo il file descriptor della sta1*/
    }

    if (pthread_self() == sta_thread2) {
        ParSta2 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        staFD = ParSta2.fd;/*salvo il file descriptor della sta2*/
    }

    if (pthread_self() == sta_thread3) {
        ParSta3 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        staFD = ParSta3.fd;/*salvo il file descriptor della sta3*/
    }

    selectStazione(staFD, mezzoCondivisoFd);/*gestisco la select*/

    if (close(mezzoCondivisoFd) < 0) {
        printf("errore close nella STA\n");
        fflush(stdout);
        exit(1);
    }
    return (NULL);
}


/**
 *
 *questo metodo viene richiamato dal thread dell' Applicazione con la quale creo i messaggi
 *@param *p struttura passata nella creazione del thread
 *
 */
void *App(void *p) {

    int risSendGrande = 0, sizePacchetto, risRead;
    char macRecv[13], payload[3000];
    char bufferMesaggio1[] = "Messaggio spedito da STA1!\0";
    char bufferMesaggio2[] = "Messaggio spedito da STA2!\0";
    char bufferMesaggio3[] = "Messaggio spedito da STA3!\0";
    if (pthread_self() == app_thread1) {
        ParApp1 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        sleep(12);
        risSendGrande = Send(macAddressSta2, bufferMesaggio1, SIZE_PAYLOAD);/*spedisco il messaggio da app1*/
        printf("App1 ha spedito il messaggio: %s\n", bufferMesaggio1);
        fflush(stdout);
        if (risSendGrande == -1) {
            printf("errore Send Grande");
            fflush(stdout);
            exit(1);
        }
    }
    if (pthread_self() == app_thread2) {
        ParApp2 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        /*sleep(7);*/
        risSendGrande = Send(macAddressSta3, bufferMesaggio2, SIZE_PAYLOAD);/*spedisco il messaggio da app2*/
        printf("App2 ha spedito il messaggio: %s\n", bufferMesaggio2);
        fflush(stdout);
        if (risSendGrande == -1) {
            printf("errore Send Grande");
            fflush(stdout);
            exit(1);
        }
    }

    if (pthread_self() == app_thread3) {
        ParApp3 = *((PARAMETRI*) p);
        free(p);
        p = NULL;
        /*sleep(12);*/
        risSendGrande = Send(macAddressSta2, bufferMesaggio3, SIZE_PAYLOAD);/*spedisco il messaggio da app3*/
        printf("App3 ha spedito il messaggio: %s\n", bufferMesaggio3);
        fflush(stdout);
        if (risSendGrande == -1) {
            printf("errore Send Grande");
            fflush(stdout);
            exit(1);
        }
    }


    sleep(15);

    if (pthread_self() == app_thread1) {
        risRead = Recv(macRecv, payload, &sizePacchetto);/*app1 riceve un messaggio*/
        if (risRead == 1) {
            printf(BLU"%s ha ricevuto: %s \n"DEFAULTCOLOR,macRecv, payload);
            fflush(stdout);
        } else {
            printf("reinvio il messaggio %s \n",payload);
            fflush(stdout);
            sleep(5);
            risSendGrande = Send(macRecv, payload, SIZE_PAYLOAD);
            if (risSendGrande == -1) {
                printf("errore Send Grande");
                fflush(stdout);
                exit(1);
            }
        }
    }
    if (pthread_self() == app_thread2) {
        risRead = Recv(macRecv, payload, &sizePacchetto);/*app2 riceve un messaggio*/
        if (risRead == 1) {
            printf(BLU"%s ha ricevuto: %s \n"DEFAULTCOLOR,macRecv, payload);
            fflush(stdout);
        } else {
            printf("reinvio il messaggio %s \n",payload);
            fflush(stdout);
            sleep(5);
            risSendGrande = Send(macRecv, payload, SIZE_PAYLOAD);
            if (risSendGrande == -1) {
                printf("errore Send Grande");
                fflush(stdout);
                exit(1);
            }
        }
    }
    if (pthread_self() == app_thread3) {
        risRead = Recv(macRecv, payload, &sizePacchetto);/*app3 riceve un messaggio*/
        if (risRead == 1) {
            printf(BLU"%s ha ricevuto: %s \n"DEFAULTCOLOR,macRecv, payload);
            fflush(stdout);
        } else {
            printf("reinvio il messaggio %s \n",payload);
            fflush(stdout);
            sleep(5);
            risSendGrande = Send(macRecv, payload, SIZE_PAYLOAD);
            if (risSendGrande == -1) {
                printf("errore Send Grande");
                fflush(stdout);
                exit(1);
            }
        }
    }

    return (NULL);

}

/**
 * spedisco il messaggio Dall'applicazione alla stazione
 * 
 * @param macDest mac address del destinatario
 * @param bufferMessaggio buffer che contiene il messaggio
 * @param sizebuffer grandezza del buffer
 * 
 * @return  -1 se qualcosa va storto, 1 se tutto va bene
 * 
 * */

int Send(char* macDest, char* bufferMessaggio, int sizeBuffer) {

    int ris_length_pkg, ris_macDest, returnValue, risPayload = 0;
    int fdCurrent = 0;
    char size_buf_c[2];

    snprintf(size_buf_c, 10, "%d", sizeBuffer);

    if (pthread_self() == app_thread1) {/*se sono app1 setto il mio fd della socket pair*/
        fdCurrent = ParApp1.fd;
    }
    if (pthread_self() == app_thread2) {/*se sono app2 setto il mio fd della socket pair*/
        fdCurrent = ParApp2.fd;
    }
    if (pthread_self() == app_thread3) {/*se sono app3 setto il mio fd della socket pair*/
        fdCurrent = ParApp3.fd;
    }

    ris_length_pkg = spedisci(fdCurrent, size_buf_c, 2); /*spediamo size  payload */
    ris_macDest = spedisci(fdCurrent, macDest, 13); /*spediamo il mac address del destinatario */
    risPayload = spedisci(fdCurrent, bufferMessaggio, 27); /*spediamo il payload */

    if (ris_length_pkg < 0 || ris_macDest < 0 || risPayload < 0) {
        returnValue = -1;
    } else {
        returnValue = 1;
    }
    return returnValue;
}

/**
 * 
 * Ricevo il messaggio dalla Stazione per spedirlo all'applicazione
 * @param macRecv mac address destinatario
 * @param buffer messaggio da spedire
 * @param length lunghezza del messaggio
 * 
 * @return 1 se il pacchetto è da reinviare, 0 se il pacchetto è andato a buon fine
 *   
 * */

int Recv(char* macRecv, char* buffer, int *length) {
    frameRecv *buf;
    int risR = 0, toRead, pkt_length = sizeof (buf->packet_length);
    char tmp[2041];
    int fdRecv;

    if (pthread_self() == app_thread1) {/*se sono app1*/

        fdRecv = ParApp1.fd;

    }
    if (pthread_self() == app_thread2) {/*se sono app2*/

        fdRecv = ParApp2.fd;
    }
    if (pthread_self() == app_thread3) {/*se sono app3*/

        fdRecv = ParApp3.fd;
    }


    leggiBuffer(pkt_length, tmp, fdRecv);/*leggo la lunghezza della frame*/
    buf = (frameRecv *) tmp;
    toRead = buf->packet_length - pkt_length;
    leggiBuffer(toRead, tmp + pkt_length, fdRecv);/*leggo la frame*/
    strcpy(buffer, buf->payload);
    strcpy(macRecv, buf->addrDestinatario);
    *length = strlen(buffer);
    if (buf->corrotto == 0) {/*se è da reinviare*/
        risR = 1;
    } else {
        risR = 0;
    }
    return risR;
}

/**
 * select per ricevere/spedire pacchetti dalla app e dal mezzo condiviso
 * @param socketPair socket che collega la sta con l'app
 * @param listenFd socket che collega la sta con il Mezzo condiviso
 *
 *
 */
int selectStazione(int socketPair, int listenFd) {
    fd_set readset;
    frame frameToSend;
    char* bufFinale;
    frame* revert;
    static frame *message;
    struct frame frameAck;
    int nready, maxi, sizeBufferToRead, flagScartato = 1,isAck = 0;
    int sizeToPacketLength = 0;
    frameRecv buffRecv;
    struct timeval *attesaCorrotto, *istanteAttuale, *timeAck;
    istanteAttuale = (struct timeval*) malloc(sizeof (struct timeval));
    attesaCorrotto = (struct timeval*) malloc(sizeof (struct timeval));
    timeAck = (struct timeval*) malloc(sizeof (struct timeval));
    /*costruzione frame ACK*/
    frameAck.ctrl.data = 1;
    frameAck.ctrl.data_type = 3;
    frameAck.ctrl.toDS = 0;
    frameAck.ctrl.fromDS = 0;
    frameAck.ctrl.RTS = 0;
    frameAck.ctrl.CTS = 0;
    frameAck.ctrl.scan = 0;
    frameAck.duration = 20;
    frameAck.packet_length = sizeof (frame);
    frameAck.seqctrl = 0;
    frameAck.CRC = 1;
    strcpy(frameAck.payload, "payload ACK\0");
    /*fine costruzione frame ACK*/

    inizializzaTempo(attesaCorrotto);
    inizializzaTempo(timeAck);

    while (1) {
        FD_ZERO(&readset);
        FD_SET(listenFd, &readset);
        FD_SET(socketPair, &readset);
        maxi = max(listenFd, socketPair);
        nready = select(maxi + 1, &readset, NULL, NULL, NULL); /*    SELECT  */

        if (nready < 0) {
            printf("select failed\n");
            fflush(stdout);
            exit(1);
        } else {
            inizializzaTempo(istanteAttuale);/*salvo l'istante attuale*/

            if (((FD_ISSET(socketPair, &readset) && flagScartato) || 
                        (FD_ISSET(socketPair, &readset) && calcolaTempo(istanteAttuale, attesaCorrotto) > 6000000)) && /*se sto ricevendo dall'app e non è stato scartato || se sto ricevendo ed è passato il tempo corroto*/
                    ( !isAck && calcolaTempo(istanteAttuale,timeAck)>1000000)) {/* && se non è un 'ack e ho dato la precedenza se c'era un ack, allora invio il messaggio al MC*/

                flagScartato = 1;
                frameToSend = package_construction(socketPair); /*costruiamo tutto il pacchetto*/
                spedisci(listenFd, &frameToSend, frameToSend.packet_length);/*spedisco il messaggio al MC*/
            }
            if ((FD_ISSET(listenFd, &readset)) && (timeoutCRC == NULL)) { /*ricevo il messaggio dal MEZZO CONDIVISO*/
                sizeToPacketLength = (sizeof (fr.ctrl) + sizeof (fr.duration) + sizeof (fr.packet_length));
                bufFinale = malloc(2041 * sizeof (char)); /* buffer packet */
                leggiBuffer(sizeToPacketLength, bufFinale, listenFd);/*leggo la grandezza della frame*/
                revert = (frame *) bufFinale;
                sizeBufferToRead = (revert->packet_length - (sizeToPacketLength)); /*byte da leggere ancora*/
                leggiBuffer(sizeBufferToRead, bufFinale + sizeToPacketLength, listenFd);/*leggo il resto della frame*/

#ifdef TEST
                printf(GREEN "STA received from socket %d %s\n" DEFAULTCOLOR, listenFd, revert->payload);
                printf(GREEN"CRC : %i\n"DEFAULTCOLOR, revert->CRC);
                fflush(stdout);
#endif

                if (revert->CRC == 1) {
                    if (revert->ctrl.data_type == 1) { /*messaggio normale*/
                        message = (frame *) bufFinale;/*conservo il messaggio, nel caso mi arriva l'ack lo mando all'app*/
                        frameAck = creaFrameAck(frameAck, revert);
                        spedisci(listenFd, &frameAck, frameAck.packet_length);/*spedisco l'ack*/
                        inizializzaTempo(timeAck);/*faccio partire 1 sec per dare la precedenza all'ack*/
                        isAck = 1;                        
                    } else {/*ack*/
                        isAck = 0;
                        printf("ack ricevuto da : %s e mandato da : %s\n", revert->addr.addr1, revert->addr.addr2);
                        buffRecv = creaFrameRicevuta(message->packet_length, message->payload, message->addr.addr1, message->addr.addr2, 0);
                        spedisci(socketPair, &buffRecv, buffRecv.packet_length);/*ack ricevuto, spedisco il messaggio all'app*/
                        printf(GREEN"messaggio spedito all'app\n"DEFAULTCOLOR);
                        fflush(stdout);
                    }
                }
                if (revert->CRC == 0) {/*scarto il pacchetto CRC = 0*/
                    printf("scarto il pacchetto CRC == 0\n");
                    fflush(stdout);
                    flagScartato = 0;
                    buffRecv = creaFrameRicevuta(revert->packet_length, revert->payload, revert->addr.addr1, revert->addr.addr2, 1);
                    spedisci(socketPair, &buffRecv, buffRecv.packet_length);/*spedisco all'app il messaggio corrotto per reinviarlo*/
                    inizializzaTempo(attesaCorrotto);/*faccio partire l'attesa del pacchetto corrotto*/
                }
                inizializzaTempo(istanteAttuale);/*salvo l'istanteattuale*/
            }

        }

    }/*fine while(1)    */
}

/**
 * costruisco il pacchetto da spedire al Mezzo Condiviso
 *@param fd della stazione
 *@return frame costruita
 *
 */
frame package_construction(int ParSta) {
    struct frame f;
    char* macAddress;
    char* payload;
    char buf_payload[BUFSIZE];
    char* msg_length;
    char msg_packet_length[SIZE_PAYLOAD];
    char bufMac[MAXSIZEMAC];
    msg_length = leggiBuffer(2, msg_packet_length, ParSta);

    msg_length[2] = 0; /* VIC - METTO UN TERMINATORE IN FONDO */
    sizepkg = atoi(msg_length);

    /*ADDRESSES*/

    if (pthread_self() == sta_thread1) {
        strncpy(f.addr.addr1, macAddressSta1, MAXSIZEMAC); /*setto mac address sta1*/
    }
    if (pthread_self() == sta_thread2) {
        strncpy(f.addr.addr1, macAddressSta2, MAXSIZEMAC); /*setto mac address sta2*/
    }
    if (pthread_self() == sta_thread3) {
        strncpy(f.addr.addr1, macAddressSta3, MAXSIZEMAC); /*setto mac address sta3*/
    }

    macAddress = leggiBuffer(13, bufMac, ParSta); /*leggo il mac address -> f.addr->addr2 -> destinatario*/
    strncpy(f.addr.addr2, macAddress, MAXSIZEMAC);


    payload = leggiBuffer(sizepkg, buf_payload, ParSta);

    strncpy(f.payload, payload, SIZE_PAYLOAD); /*PAYLOAD*/
    f.ctrl.data_type = 1;
    f.duration = 20; /*DURATION*/
    f.CRC = 1;
    f.ctrl.RTS = 0;
    f.ctrl.CTS = 0;
    f.ctrl.scan = 0;
    f.ctrl.fromDS = 0;
    f.ctrl.toDS = 0;
    f.seqctrl = 0;
    if (strlen(payload) > 0) {
        f.ctrl.data = 1;
    } else {
        f.ctrl.data = 0;
    }

    /*PACKET LENGTH*/
    f.packet_length = sizeof (frame);
    return f;
}

/**
 * costruisco la frame ack partendo dal pacchetto spedito
 *
 * @param frameAck frame costruita per spedire l'ack
 * @param revert pacchetto ricevuto dalle sta
 * @return frameAck
 */
struct frame creaFrameAck(frame frameAck, frame *revert) {
    strcpy(frameAck.addr.addr1, revert->addr.addr2);
    strcpy(frameAck.addr.addr2, revert->addr.addr1);
    return frameAck;
}

/**
 * costruisco la frame da spedire all'app
 *
 * @param packet_length lunghezza del pacchetto
 * @param payload messaggio spedito
 * @param addrMittente indirizzo del mittente
 * @param addrDestinatario indirizzo del destinatario
 * @param isCorrotto flag per sapere se il pachetto è corrotto : 1 -> Corrotto; 0 -> Non corrotto
 * @return frameRecv frame per l'app
 *
 */
frameRecv creaFrameRicevuta(int packet_length, char* payload, char* addrMittente, char* addrDestinatario, int isCorrotto) {
    frameRecv buffRecv;
    buffRecv.packet_length = packet_length;
    strcpy(buffRecv.payload, payload);
    strcpy(buffRecv.addrMittente, addrMittente);
    strcpy(buffRecv.addrDestinatario, addrDestinatario);
    buffRecv.corrotto = isCorrotto;
    return buffRecv;
}

/**
 * calcola le differenze fra due strutture di tipo timeval (end-start)
 *
 * @param end second timeval structure
 * @param start first timeval structure
 * @return differenza di tempo
 *
 */
int calcolaTempo(struct timeval* end, struct timeval* start) {
    long int timeout_usec;
    timeout_usec = (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_usec - start->tv_usec); /*Tempo in microsecondi*/
    return (timeout_usec);
}

/**
 * leggo il buffer da un fd
 *
 * @param lengthBuffer lunghezza del buffer da leggere
 * @param bufferDaLeggere buffer
 * @param fd file descriptor del socket
 * @return buffer letto
 *
 */
char* leggiBuffer(int lengthBuffer, char* bufferDaLeggere, int fd) {
    int risRead, i;

    for (i = 0; i < lengthBuffer; i++) {
        do {
            risRead = read(fd, bufferDaLeggere + i, 1); /*legge il messaggio da mettere nella frame     */

        } while ((risRead < 0)&&(errno == EINTR));
        if (risRead < 0) {
            printf("errore read : leggiBuffer < 0 , cazzo succede?\n");
            fflush(stdout);
            exit(1);
        } else if (risRead == 0) {
            printf("pipe chiusa nella read: ho letto 0 mentre componevo il frame\n");
            fflush(stdout);
            exit(1);
        } else {

        }

    }
#ifdef TEST
    printf("bufferDaLeggere : %s\n", bufferDaLeggere);
    fflush(stdout);
#endif

    return bufferDaLeggere;
}
/**
 *  inizializzo il tempo con gettimeofday
 *  @param struct* timeval tempo struttura da inizializzare
 */
void inizializzaTempo(struct timeval *tempo){
    int time;
    time = gettimeofday(tempo, NULL);
    if (time == -1) {
        printf("errore time\n");
        fflush(stdout);
        exit(1);
    }

}
