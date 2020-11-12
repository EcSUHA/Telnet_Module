// SCDE-Module Telnet (cmd interface to the SCDE)

#ifndef TELNET_MODULE_H
#define TELNET_MODULE_H

// -------------------------------------------------------------------------------------------------

// this Module is made for the Smart-Connected-Device-Engine
#include "SCDE_s.h"

// this Module provides functions for other Modules:
// -- no ---

// this Module uses an 1st stage:
// -- no ---

// -------------------------------------------------------------------------------------------------

// stores the Root Data of the Smart Connected Devices Engine - at/for this Module
SCDERoot_t* SCDERoot_at_Telnet_M;

// stores SCDEFn (Functions / callbacks) provided for operation - at/for this Module
SCDEFn_t* SCDEFn_at_Telnet_M;

// -------------------------------------------------------------------------------------------------
















#define RECV_BUF_SIZE 2048











// Data 'Provided By Module' for the telnet module - we need this extern
//ProvidedByModule_t Telnet_Module;


//Module_t Module;




//typedef sint8 err_t;

typedef void *espconn_handle;
/** A callback prototype to inform about events for a espconn */
typedef void (* espconn_connect_callback)(void *arg);
typedef void (* espconn_reconnect_callback)(void *arg, int8_t err); //sint8 err);
typedef void (* espconn_recv_callback)(void *arg, char *pdata, unsigned short len);
typedef void (* espconn_sent_callback)(void *arg);



/* Definitions for error constants. */
#define ESPCONN_OK          0    /* No error, everything OK. */
#define ESPCONN_MEM        -1    /* Out of memory error.     */
#define ESPCONN_TIMEOUT    -3    /* Timeout.                 */
#define ESPCONN_RTE        -4    /* Routing problem.         */
#define ESPCONN_INPROGRESS  -5    /* Operation in progress    */
#define ESPCONN_MAXNUM		-7	 /* Total number exceeds the set maximum*/
#define ESPCONN_ABRT       -8    /* Connection aborted.      */
#define ESPCONN_RST        -9    /* Connection reset.        */
#define ESPCONN_CLSD       -10   /* Connection closed.       */
#define ESPCONN_CONN       -11   /* Not connected.           */
#define ESPCONN_ARG        -12   /* Illegal argument.        */
#define ESPCONN_IF		   -14	 /* UDP send error			 */
#define ESPCONN_ISCONN     -15   /* Already connected.       */
#define ESPCONN_HANDSHAKE  -28   /* ssl handshake failed	 */
#define ESPCONN_SSL_INVALID_DATA  -61   /* ssl application invalid	 */


/** Protocol family and type of the espconn */
enum espconn_type {
    ESPCONN_INVALID    = 0,
    /* ESPCONN_TCP Group */
    ESPCONN_TCP        = 0x10,
    /* ESPCONN_UDP Group */
    ESPCONN_UDP        = 0x20,
};

/** Current state of the espconn. Non-TCP espconn are always in state ESPCONN_NONE! */
enum espconn_state {
    ESPCONN_NONE,
    ESPCONN_WAIT,
    ESPCONN_LISTEN,
    ESPCONN_CONNECT,
    ESPCONN_WRITE,
    ESPCONN_READ,
    ESPCONN_CLOSE
};



typedef struct _esp_tcp {
  int remote_port;
  int local_port;
  uint8_t local_ip[4];
  uint8_t remote_ip[4];
  espconn_connect_callback	connect_callback;
  espconn_reconnect_callback	reconnect_callback;
  espconn_connect_callback	disconnect_callback;
//  espconn_connect_callback write_finish_fn;
} esp_tcp;



typedef struct _esp_udp {
  int remote_port;
  int local_port;
  uint8_t local_ip[4];
  uint8_t remote_ip[4];
} esp_udp;


typedef struct _remot_info {
	enum espconn_state state;
	int remote_port;
	uint8_t remote_ip[4];
} remot_info;





enum espconn_option {
	ESPCONN_START = 0x00,
	ESPCONN_REUSEADDR = 0x01,
	ESPCONN_NODELAY = 0x02,
	ESPCONN_COPY = 0x04,
	ESPCONN_KEEPALIVE = 0x08,
	ESPCONN_END
};

enum espconn_level {
	ESPCONN_KEEPIDLE,
	ESPCONN_KEEPINTVL,
	ESPCONN_KEEPCNT
};

enum {
	ESPCONN_IDLE = 0,
	ESPCONN_CLIENT,
	ESPCONN_SERVER,
	ESPCONN_BOTH,
	ESPCONN_MAX
};





/*
 * Telnet Daemon Instance Configuration
 */
typedef struct Telnet_DInstanceCfg_s {

  // Slot Control Register Bitfield, if a bit 1-32 is set, a slot 1-32 is in use!
  uint32_t SlotCtrlRegBF;

  // Pointer to BuildInURls set at init time.
} Telnet_DInstanceCfg_t;



/* 
 * Entry Telnet Definition (struct) stores values for operation valid only for the defined instance of an
 * loaded module. Values are initialized by SCDE an the loaded module itself.
 */
typedef struct Entry_Telnet_Definition_s {

  Entry_Definition_t common;		// ... the common part of the definition

//---

  enum espconn_type type;		// type of the espconn (TCP, UDP)
  enum espconn_state state;		// current state of the espconn
  union {
	esp_tcp *tcp;			// connection details IP,Ports, ...
	esp_udp *udp;
  } proto;
    
  espconn_recv_callback recv_callback;	// A callback function for event: receive-data
  espconn_sent_callback send_callback;	// a callback function for event: send-data

  uint8_t link_cnt;

  void* reverse;			// the reverse link to application-conn-slot-data
  int Telnet_CtrlRegA;			// Telnet Control-Reg-A (enum Telnet_CtrlRegA from WEBIF.h)

  Telnet_DInstanceCfg_t* Telnet_DInstanceCfg;	// link to configuration of this HTTPD-Instance

  uint8_t slot_no;			// slot number in this instance

} Entry_Telnet_Definition_t;



/* 
 * Definition typedef stores values for operation valid only for the defined instance of an
 * loaded module. Values are initialized by HCTRLD an the loaded module itself.
 */
typedef struct Telnet_DConnSlotData_s {

  Entry_Telnet_Definition_t *conn;	// Link to lower level connection management

//- V V V V V V V V V V V V V V V V V V // TX-Helper

  char* send_buffer;//uint8_t*		// Pointer to current allocated send buffer w. size [MAX_SB_LEN] (NULL=NO Send-Buffer)
  int send_buffer_write_pos; //uint16_t	// Send buffer, current write position (offset), !> 0 = its allocated!
  char* trailing_buffer;//uint8_t*	// An 'trailing_buffer' in case of 'send_buffer' overflow. Its prio for next transmission!
  int trailing_buffer_len; //uint16_t	// The 'trailing_buffer' length of (allocated & stored) data, !> 0 = its allocated!


//- V V V V V V V V V V V V V V V V V V // Libtelnet

  telnet_t* tnHandle;			// the clients Libtelnet tnHandle

//- V V V V V V V V V V V V V V V V V V // Helper


//  HTTPDConnSlotPrivData *priv;		// Opaque pointer to data for internal httpd housekeeping (currently unused)


//- V V V V V V V V V V V V V V V V V V // General Connection Control

  uint16_t ConnCtrlFlags;		// Connection Control Register (enum ConnCtrlFlags from httpD.h)
  int16_t InactivityTimer;		// 10HZ Timer counting up after any Callback to detect broken connections

//- V V V V V V V V V V V V V V V V V V // ?




//- union 32Bit connspecific possible 

  uint8_t slot_no;			// helper to show slot number
  unsigned int SlotCgiState : 4;	// enum SlotCgiState
  unsigned int SlotParserState : 4;	// enum SlotParserState
  uint16_t LP10HzCounter;		// LongPoll 100ms/ 10Hz Counter (0.1-6553.6 Sec) for this conn

//-

  // adds telnet data (till new line / line feed)
  char* p_telnet_rcv_buff;		// current telnet received buffer ... adds during parsing
  int telnet_rcv_len;			// current telnet received buffer length ... during parsing

  } Telnet_DConnSlotData_t;


// Information Flags - for Connection Control
enum ConnCtrlFlags
  { F_GENERATE_IDLE_CALLBACK	= 1 << 0	// Force one further callback (may be by generating one idle callback)
  , F_CONN_IS_AUTHENTICATED	= 1 << 1	// this connection is authenticated
  , F_TXED_CALLBACK_PENDING	= 1 << 2	// something was sent. Sent Callback is pending. Do not send again.  
  , F_CALLED_BY_RXED_CALLBACK	= 1 << 3	// processing was invoked by Received Callback
  , F_RESA			= 1 << 4
  , F_RESB			= 1 << 5
  , F_RESC			= 1 << 6
  , F_RESD			= 1 << 7
  , F_RESE			= 1 << 8
  , F_RESF			= 1 << 9
  , F_RESG			= 1 << 10
  , F_RESH			= 1 << 11
  , F_RESI			= 1 << 12
  , F_RESJ			= 1 << 13
  , F_RESK			= 1 << 14
  , F_RESL			= 1 << 15
  };



// Information Flags - for Connection Control
enum WebIF_CtrlRegA
  { F_THIS_IS_SERVERSOCKET	= 1 << 0	// indicates this is Server Socket (listens for new conn)
  , F_NEEDS_CLOSE		= 1 << 1	// 
  , F_RESXX			= 1 << 2	//   
  , F_RESXZ			= 1 << 3	// 
  , F_RESXA			= 1 << 4
  , F_RESXB			= 1 << 5
  , F_RESXC			= 1 << 6
  , F_RESXD			= 1 << 7
  , F_RESXE			= 1 << 8
  , F_RESXF			= 1 << 9
  , F_RESXG			= 1 << 10
  , F_RESXH			= 1 << 11
  , F_RESXI			= 1 << 12
  , F_RESXJ			= 1 << 13
  , F_RESXK			= 1 << 14
  , F_RESXL			= 1 << 15
  };




//----------------- old stuff ------------------------





// Max allowed connections to SCDED. (Limit is 31+1)
// +1 is the SCDE-Event-TX connection, its conn[0]
#define MAX_SCDED_CONN 10 +1



/*
// get current state hlper macro for stage 2 processing FSM
#define S2_CURRENT_STATE() S2_State

// update current state hlper macro for stage 2 processing FSM
#define S2_UPDATE_STATE(V) S2_State = (enum state) (V)

// set error code helper macro for stage 2 processing FSM
#define SET_S2_ERRNO(e)							\
do									\
	{								\
	conn->parser_http_errno = (e);					\
	} while(0)
*/
/*// set error code helper macro for stage 2 processing FSM
#define SET_S2_ERRNO(e)							\
do									\
	{								\
	stage2_scde_errno = (e);					\
	} while(0)
*/



























// Platform dependent code should call these
//void Telnet_IdleCb(void *arg);
void Telnet_SentCb(void *arg);
void Telnet_RecvCb(void *arg, char *recvdata, unsigned short recvlen);
void Telnet_ReconCb(void *arg, int8_t err);
void Telnet_DisconCb(void *arg);
void Telnet_ConnCb(void *arg);

/*
// move to right place
int Telnet_DirectRead(Common_Definition_t* Def);
int Telnet_DirectWrite(Common_Definition_t* Def);
int Telnet_Initialize(SCDERoot_t* SCDERoot);
char* Telnet_Define(Entry_Telnet_Definition_t *Telnet_Definition, const char *Definition);
char* Telnet_Set(Common_Definition_t* Common_Definition, char *args);
char* Telnet_Undefine(Common_Definition_t* Common_Definition);
*/

// platform additional ...
int Telnet_sent(Entry_Telnet_Definition_t *Telnet_Definition, uint8_t *Buff, uint Len);
void Telnet_disconnect(Entry_Telnet_Definition_t *Telnet_Definition);
void Telnet_espconn_regist_recvcb(Entry_Telnet_Definition_t *conn, espconn_recv_callback RecvCb);
void Telnet_espconn_regist_connectcb(Entry_Telnet_Definition_t *conn, espconn_connect_callback connectCb);
void Telnet_espconn_regist_reconcb(Entry_Telnet_Definition_t *conn, espconn_reconnect_callback ReconCb);
void Telnet_espconn_regist_disconcb(Entry_Telnet_Definition_t *conn, espconn_connect_callback DisconCb);
void Telnet_espconn_regist_sentcb(Entry_Telnet_Definition_t *conn, espconn_sent_callback SentCb);



// --------------------------------------------------------------------------------------------------



/*
 *  Functions provided to SCDE by Module - for type operation (A-Z)
 */

//
strTextMultiple_t* Telnet_Define(Common_Definition_t *Common_Definition);//, const char *Definition);

//
int Telnet_Direct_Read(Common_Definition_t*  p_entry_definition);

//
int Telnet_Direct_Write(Common_Definition_t*  p_entry_definition);

//
int Telnet_Initialize(SCDERoot_t* SCDERoot);

//
strTextMultiple_t* Telnet_Set(Common_Definition_t* Common_Definition, uint8_t *setArgs, size_t setArgsLen);

//
strTextMultiple_t* Telnet_Undefine(Common_Definition_t* Common_Definition);



// --------------------------------------------------------------------------------------------------



/*
 *  telnet
 */
int Telnet_Send_To_Send_Buff(Telnet_DConnSlotData_t *conn, const char *data, int len);
void Telnet_TransmitSendBuff(Telnet_DConnSlotData_t *conn); 
void Telnet_RespToOpenConn(Telnet_DConnSlotData_t* conn);


/*
 *  helpers
 */
static char* eventToString(telnet_event_type_t type);
static void Telnet_LibtelnetEventHandler(telnet_t *thisTelnet, telnet_event_t *event, void *userData);






#endif /*TELNET_MODULE_H*/
