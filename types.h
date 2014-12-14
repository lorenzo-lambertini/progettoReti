#define BUFSIZE 2000
#define DEFAULTCOLOR "\033[0m"
#define ROSSO  "\033[22;31m"
#define VERDE  "\033[22;32m"
#define GREEN "\033[0;0;32m"
#define WHITE   "\033[0m"
#define RED "\033[0;0;31m"
#define BLU "\033[0;0;34m"
#define ORANGE "\033[0;0;33m"

	 
	 
	 typedef struct control {
		uint8_t data;
		uint8_t data_type;
		uint8_t toDS;
		uint8_t fromDS;
		uint8_t RTS;
		uint8_t CTS;
		uint8_t scan;
		 
	}control;

	typedef struct address {
		char addr1[13]; /*mittente*/
		char addr2[13];	/*destinatario*/	
	}address;



	typedef struct frame {
		struct control ctrl;
		uint8_t duration;
		uint16_t packet_length;
		struct address addr;
		uint32_t seqctrl;				
		char payload[BUFSIZE];	
		uint8_t CRC;	
	} __attribute__((__packed__))  frame;
	 
	typedef struct frameRecv {
		uint16_t packet_length;
		int  corrotto;
		char payload [BUFSIZE];
		char addrMittente[13];
		char addrDestinatario[13];
	} __attribute__((__packed__))  frameRecv;

typedef struct {
    int  fd;
    char *macDest;
    int  fdMezzoCondiviso;
} PARAMETRI;	 


