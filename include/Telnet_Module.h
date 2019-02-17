// SCDE-Module Telnet

#ifndef TELNET_MODULE_H
#define TELNET_MODULE_H



#include "SCDE_s.h"


#define RECV_BUF_SIZE 2048











// Data 'Provided By Module' for the telnet module - we need this extern
//ProvidedByModule_t Telnet_Module;


Module_t Module;




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



typedef struct _esp_tcp
  {
  int remote_port;
  int local_port;
  uint8_t local_ip[4];
  uint8_t remote_ip[4];
  espconn_connect_callback connect_callback;
  espconn_reconnect_callback reconnect_callback;
  espconn_connect_callback disconnect_callback;
//  espconn_connect_callback write_finish_fn;
  } esp_tcp;

typedef struct _esp_udp
  {
  int remote_port;
  int local_port;
  uint8_t local_ip[4];
  uint8_t remote_ip[4];
  } esp_udp;


typedef struct _remot_info{
	enum espconn_state state;
	int remote_port;
	uint8_t remote_ip[4];
}remot_info;





enum espconn_option{
	ESPCONN_START = 0x00,
	ESPCONN_REUSEADDR = 0x01,
	ESPCONN_NODELAY = 0x02,
	ESPCONN_COPY = 0x04,
	ESPCONN_KEEPALIVE = 0x08,
	ESPCONN_END
};

enum espconn_level{
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
typedef struct Telnet_DInstanceCfg_s
  {
  // Slot Control Register Bitfield, if a bit 1-32 is set, a slot 1-32 is in use!
  uint32_t SlotCtrlRegBF;
  // Pointer to BuildInURls set at init time.
  } Telnet_DInstanceCfg_t;



/* 
 * Telnet_Definition stores values for operation valid only for the defined instance of an
 * loaded module. Values are initialized by HCTRLD an the loaded module itself.
 */
typedef struct Telnet_Definition_s
  {
  Common_Definition_t common;		// ... the common part of the definition
//---
  enum espconn_type type;		// type of the espconn (TCP, UDP)
  enum espconn_state state;		// current state of the espconn
  union
	{
	esp_tcp *tcp;			// connection details IP,Ports, ...
	esp_udp *udp;
	} proto;
    
  espconn_recv_callback recv_callback;	// A callback function for event: receive-data
  espconn_sent_callback send_callback;	// a callback function for event: send-data
  uint8_t link_cnt;
  void *reverse;			// the reverse link to application-conn-slot-data
  int Telnet_CtrlRegA;			// Telnet Control-Reg-A (enum Telnet_CtrlRegA from WEBIF.h)
  Telnet_DInstanceCfg_t* Telnet_DInstanceCfg;	// link to configuration of this HTTPD-Instance
  uint8_t SlotNo;			// slot number in this instance
  } Telnet_Definition_t;



/* 
 * Definition typedef stores values for operation valid only for the defined instance of an
 * loaded module. Values are initialized by HCTRLD an the loaded module itself.
 */
typedef struct Telnet_DConnSlotData_s
  {

  Telnet_Definition_t *conn;	// Link to lower level connection management

//- V V V V V V V V V V V V V V V V V V // TX-Helper

  char *sendBuff;			// Pointer to current allocated send buffer w. size [MAX_SB_LEN] (NULL=NO Send-Buffer)
  int sendBuffLen; //SendBuffWritePos	// Send buffer, current write position (offset)
  char *TrailingBuff;			// Pointer to a Trailing-Buffer (in case of an Send-Buffer overflow). Its prio for next transmission!
  int TrailingBuffLen;			// Trailing-Buffer Length of (allocated) data, if any

//- V V V V V V V V V V V V V V V V V V // Libtelnet

  telnet_t *tnHandle;			// the clients Libtelnet tnHandle

//- V V V V V V V V V V V V V V V V V V // Helper


//  HTTPDConnSlotPrivData *priv;		// Opaque pointer to data for internal httpd housekeeping (currently unused)

//- V V V V V V V V V V V V V V V V V V // Procedure Callback Data

 // PCallback cgi;			// Assigned Procedure Callback for Processing
  const void *PCArg;			// Argument for Procedure-Callback-Processing
  void *PCData;				// Data for PC Procedure-Callback-Operations (counting...)

//- V V V V V V V V V V V V V V V V V V // General Connection Control

  uint16_t ConnCtrlFlags;		// Connection Control Register (enum ConnCtrlFlags from httpD.h)
  int16_t InactivityTimer;		// 10HZ Timer counting up after any Callback to detect broken connections

//- V V V V V V V V V V V V V V V V V V // ?




//- union 32Bit connspecific possible 

  uint8_t SlotNo;			// helper to show slot number
  unsigned int SlotCgiState : 4;	// enum SlotCgiState
  unsigned int SlotParserState : 4;	// enum SlotParserState
  uint16_t LP10HzCounter;		// LongPoll 100ms/ 10Hz Counter (0.1-6553.6 Sec) for this conn

//-

//union
//	{ routine verschieben!
//	RPCCallback cgi; //soll rpc //alt:cgiSendCallback cgi;	// Assigned RPC Processing Callback
//	HdrFldProcCb HdrFldFnCb;// Header Field processing Function Callback
// int32_t HdrFldId;


//tempörär. Idee fehlt
// union je nach prozesschritt?
char *HdrFldNameBuff;	// Pointer to Header Field Name Buffer
int HdrFldNameLen;	// current HdrFldNameBuff length
char *HdrFldValueBuff;	// Pointer to Header Field Value Buffer
int HdrFldValueLen;	// current HdrFldValueBuff length

//	};




//- V V V V V V V V V V V V V V V V V V // SCD-Engine STAGE 1 processing -> parsing HTTP

  uint32_t parser_nread;          	// # bytes read in various scenarios
  uint64_t parser_content_length;	// Content-Length of Body from Headerfield ; -1 = not specified ; (counts down while parsed?)

//-

  unsigned int parser_type : 2;         // Parser type cfg  AND result after parse (enum http_parser_type from httpD.h)
  unsigned int parser_flags : 8;        // Extracted information Flags F_* (enum flags from httpD.h)
					// BIT 0 = F_CHUNKED
					// BIT 1 = F_CONNECTION_KEEP_ALIVE
					// BIT 2 = F_CONNECTION_CLOSE
					// BIT 3 = F_CONNECTION_UPGRADE
					// BIT 4 = F_TRAILING
					// BIT 5 = F_UPGRADE
					// BIT 6 = F_SKIPBODY
					// BIT 7 = F_CONTENTLENGTH -> Content Length is set
  unsigned int parser_state : 7;        // State of parser finite state machine (enum state from http_parser.c)
  unsigned int parser_header_state : 7; // State of parser_header finite state machine (enum header_state from http_parser.c)
  unsigned int parser_index : 7;        // index into current matcher
  unsigned int parser_lenient_http_headers : 1;

//-

  uint16_t parser_http_major;		// FROM RESPONSE ONLY - extracted major version code (0-999 valid)
  uint16_t parser_http_minor;		// FROM RESPONSE ONLY - extracted minor version code (0-999 valid)

//-	

  uint16_t parser_status_code;		// FROM RESPONSE ONLY - extracted status code (0-999 valid)		// unsigned int parser_status_code : 16;
  unsigned int parser_method : 8;	// FROM REQUEST ONLY - extracted method (enum http_method from httpD.h)
  unsigned int parser_http_errno : 7;	// internal error number in various operations
  unsigned int parser_upgrade : 1;	// FROM REQUEST ONLY - upgrade header was present. Check when http_parser_execute() returns

//- Extracted values in case of an request ...

  unsigned int parser_scheme : 4;	// FROM REQUEST ONLY - scheme extracted by uri-parsing (AvailSchemes[] from ps.h)
  unsigned int parser_mime : 4;		// FROM REQUEST ONLY - mime extracted by uri-parsing (AvailContentTypes[] from ps.h)
  unsigned int freereserved1 : 8;	// 
  int16_t ActiveDirID;			// FROM REQUEST ONLY - Active Directory ID (ADID) extracted by uri-parsing (0-32767 valid,-1,-2,-3 indicate special cases)

//-

  STAILQ_HEAD (stailhead, HeaderFieldInfo_s) head; // REQUEST & RESPONSE - Selected Header-Fields extracted & cached (AvailHdrFlds[])

//-

  char *url;				// FROM REQUEST ONLY - Ptr to allocated memory filled with PATH extracted from URI
  char *AltFSFile;			// FROM REQUEST ONLY - (NULL = Filename from Path!) else Ptr. to alternative filename in FS
  char *DestUrl;//MatchedUrl		// FROM REQUEST ONLY - Ptr to BuildInURL that matches (Active URL with Tokens for reconstruction)
  char *getArgs;//Query			// FROM REQUEST ONLY - Ptr to allocated memory filled with QUERY (GETARGS) extracted from URI, if any
  char *hostName;			// FROM REQUEST ONLY - content of host name field

//-

  char *postBuff; //BodyData		// REQUEST & RESPONSE - Pointer to BdyData Buffer
  int postLen;	//BodyLen		// REQUEST & RESPONSE - length of stored body data
//int postPos;	//BodyPos benötigt?	// counter for post position (contains whole post data len, not only buffer!)

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
char* Telnet_Define(Telnet_Definition_t *Telnet_Definition, const char *Definition);
char* Telnet_Set(Common_Definition_t* Common_Definition, char *args);
char* Telnet_Undefine(Common_Definition_t* Common_Definition);
*/

// platform additional ...
int Telnet_sent(Telnet_Definition_t *Telnet_Definition, uint8_t *Buff, uint Len);
void Telnet_disconnect(Telnet_Definition_t *Telnet_Definition);
void Telnet_espconn_regist_recvcb(Telnet_Definition_t *conn, espconn_recv_callback RecvCb);
void Telnet_espconn_regist_connectcb(Telnet_Definition_t *conn, espconn_connect_callback connectCb);
void Telnet_espconn_regist_reconcb(Telnet_Definition_t *conn, espconn_reconnect_callback ReconCb);
void Telnet_espconn_regist_disconcb(Telnet_Definition_t *conn, espconn_connect_callback DisconCb);
void Telnet_espconn_regist_sentcb(Telnet_Definition_t *conn, espconn_sent_callback SentCb);



/*
 *  Functions provided to SCDE by Module - for type operation (A-Z)
 */
strTextMultiple_t* Telnet_Define(Common_Definition_t *Common_Definition);//, const char *Definition);

int Telnet_DirectRead(Common_Definition_t* Common_Definition);

int Telnet_DirectWrite(Common_Definition_t* Common_Definition);

int Telnet_Initialize(SCDERoot_t* SCDERoot);

strTextMultiple_t* Telnet_Set(Common_Definition_t* Common_Definition, uint8_t *setArgs, size_t setArgsLen);

strTextMultiple_t* Telnet_Undefine(Common_Definition_t* Common_Definition);



/*
 *  telnet
 */
int Telnet_SendToSendBuff(Telnet_DConnSlotData_t *conn, const char *data, int len);
void Telnet_TransmitSendBuff(Telnet_DConnSlotData_t *conn); 



/*
 *  helpers
 */
static char* eventToString(telnet_event_type_t type);
static void Telnet_LibtelnetEventHandler(telnet_t *thisTelnet, telnet_event_t *event, void *userData);






#endif /*TELNET_MODULE_H*/
