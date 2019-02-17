/* #################################################################################################
 *
 *  Function: Telnet Module for SCDE (Smart Connected Device Engine)
 *
 *  ESP 8266EX & ESP32 SOC Activities ...
 *  Copyright by EcSUHA
 *
 *  Created by Maik Schulze, Sandfuhren 4, 38448 Wolfsburg, Germany for EcSUHA.de 
 *
 *  MSchulze780@GMAIL.COM
 *  EcSUHA - ECONOMIC SURVEILLANCE AND HOME AUTOMATION - WWW.EcSUHA.DE
 * #################################################################################################
 */

//https://github.com/nkolban/esp32-snippets/blob/master/networking/telnet/main/telnet.h
//https://github.com/seanmiddleditch/libtelnet

#include "ProjectConfig.h"

#if defined(ESP_PLATFORM)


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
//#include <spiffs.h>

#include "lwip/err.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

//#include "duktape_spiffs.h"
//#include "esp32_specific.h"


//#include <esp8266.h>
//#include "Platform.h"


#include "lwip/ip.h"

#include <stdint.h>
#include "esp_attr.h"
#include "esp_deep_sleep.h"
//#include "esp_err.h" bug mit assert
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_heap_alloc_caps.h"
#include "esp_intr.h"
#include "esp_ipc.h"
#include "esp_ssc.h"
#include "esp_system.h"
#include "esp_task.h"
#include "esp_wifi.h"
#include "esp_types.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
//#include "heap_alloc_caps.h"

#include "sdkconfig.h"
#else // LINUX_PLATFORM

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#endif // END PLATFORM



#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

//#include "scde_task.h"
#include "logging.h"
#include "SCDE_s.h"
#include "SCDE_Main.h"
//#include "SCDE.h"


#include "libtelnet.h"
#include "Telnet_Module.h"

LOG_TAG("Telnet_Module");




/*
#include <ProjectConfig.h>
#include <esp8266.h>
#include <Platform.h>

#include "libtelnet.h"
#include <SCDE_s.h>

#include "Telnet_Module.h"

//#include "SCDE_Main.h"


#include "lwip/sockets.h"

*/




// Max send buffer len
#define MAX_SENDBUFF_LEN 2048

// -------------------------------------------------------------------------------------------------

// ammount of debug information 0 = off 5 max?
#ifndef SCDED_DBG
#define SCDED_DBG 0
#endif

// ammount of debug information 0 = off 5 max?
#ifndef SCDEH_DBG
#define SCDEH_DBG 0
#endif

// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------

// set default build verbose - if no external override
#ifndef ESP32_Telnet_Module_DBG  
#define ESP32_Telnet_Module_DBG  5	// 5 is default
#endif 

// -------------------------------------------------------------------------------------------------


// make data root locally available
static SCDERoot_t* SCDERoot;

// make locally available from data-root: the SCDEFn (Functions / callbacks) for operation
static SCDEFn_t* SCDEFn;

// --------------------------------------------------------------------------------------------------



/**
 * --------------------------------------------------------------------------------------------------
 *  DName: Telnet_Module
 *  Desc: Data 'Provided By Module' for the Telnet module (functions + infos this module provides
 *        to SCDE)
 *  Data: 
 * --------------------------------------------------------------------------------------------------
 */
ProvidedByModule_t Telnet_ProvidedByModule =   { // A-Z order
  "Telnet"				// Type-Name of module -> should be same name as libfilename.so !
  ,6					// size of Type-Name

  ,NULL					// Add
  ,NULL					// Attribute
  ,Telnet_Define			// Define
  ,NULL					// Delete
  ,NULL					// Except
  ,NULL					// Get
  ,NULL					// IdleCb
  ,Telnet_Initialize			// Initialize
  ,NULL					// Notify
  ,NULL					// Parse
  ,NULL					// Read
  ,NULL					// Ready
  ,NULL					// Rename
  ,Telnet_Set				// Set
  ,NULL					// Shutdown
  ,NULL					// State
  ,NULL					// Sub
  ,Telnet_Undefine			// Undefine
  ,Telnet_DirectRead			// DirectRead
  ,Telnet_DirectWrite			// DirectWrite
 // ,NULL				// FnProvided
  ,sizeof(Telnet_Definition_t)		// Size of modul specific definition structure (Common_Definition_t + X)
};



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_Initialize
 *  Desc: Initializion of SCDE Function Callbacks of an new loaded module
 *  Info: Stores Module-Information (Function Callbacks) to SCDE-Root
 *  Para: SCDERoot_t* SCDERootptr -> ptr to SCDE Data Root
 *  Rets: ? unused
 *--------------------------------------------------------------------------------------------------
 */
int 
Telnet_Initialize(SCDERoot_t* SCDERootptr)
{

  // make data root locally available
  SCDERoot = SCDERootptr;

  // make locally available from data-root: SCDEFn (Functions / callbacks) for faster operation
  SCDEFn = SCDERootptr->SCDEFn;

  SCDEFn->Log3Fn(Telnet_ProvidedByModule.typeName
		  ,Telnet_ProvidedByModule.typeNameLen
		  ,3
		  ,"InitializeFn called. Type '%.*s' now useable.\n"
		  ,Telnet_ProvidedByModule.typeNameLen
		  ,Telnet_ProvidedByModule.typeName);

  return 0;
}



/* --------------------------------------------------------------------------------------------------
 *  FName: eventToString
 *  Desc: Convert libtelnet event-type to string - for user readability
 *  Info: 
 *  Para: telnet_event_type_t type -> event type from libtelnet
 *  Rets: char * -> to string TXT
 *--------------------------------------------------------------------------------------------------
 */
static char*
eventToString(telnet_event_type_t type)
{
  switch (type)

	{

	case TELNET_EV_COMPRESS:
		return "TELNET_EV_COMPRESS";

	case TELNET_EV_DATA:
		return "TELNET_EV_DATA";

	case TELNET_EV_DO:
		return "TELNET_EV_DO";

	case TELNET_EV_DONT:
		return "TELNET_EV_DONT";

	case TELNET_EV_ENVIRON:
		return "TELNET_EV_ENVIRON";

	case TELNET_EV_ERROR:
		return "TELNET_EV_ERROR";

	case TELNET_EV_IAC:
		return "TELNET_EV_IAC";

	case TELNET_EV_MSSP:
		return "TELNET_EV_MSSP";

	case TELNET_EV_SEND:
		return "TELNET_EV_SEND";

	case TELNET_EV_SUBNEGOTIATION:
		return "TELNET_EV_SUBNEGOTIATION";

	case TELNET_EV_TTYPE:
		return "TELNET_EV_TTYPE";

	case TELNET_EV_WARNING:
		return "TELNET_EV_WARNING";

	case TELNET_EV_WILL:
		return "TELNET_EV_WILL";

	case TELNET_EV_WONT:
		return "TELNET_EV_WONT";

	case TELNET_EV_ZMP:
		return "TELNET_EV_ZMP";

	}

  return "Unknown type";
}



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_LibtelnetEventHandler
 *  Desc: Libtelnet event handler function - gets incoming events from libtelnet
 *  Info: Initializion of available Module Function Callbacks for Home Control
 *  Para: telnet_t *thisTelnet -> libtelnet conn info?
 *        telnet_event_t *event -> libtelnet event info
 *        void *userData -> in this case Telnet_DConnSlotData_t *conn
 *  Rets: ? unused
 *--------------------------------------------------------------------------------------------------
 */
static void
Telnet_LibtelnetEventHandler(telnet_t *thisTelnet,
		telnet_event_t *event,
		void *userData)
{

  int rc;

  # if SCDED_DBG >= 3
  printf("|telnet event: %s>", eventToString(event->type));
  #endif


  LOGD("telnet event: %s", eventToString(event->type));

  Telnet_DConnSlotData_t *conn = (Telnet_DConnSlotData_t *) userData;

  switch (event->type)

	{

// -----------------------------------------------------------------------

/* TELNET_EV_SEND:
   This event is sent whenever libtelnet has generated data that must
   be sent over the wire to the remove end.  Generally that means
   calling send() or adding the data to your application's output
   buffer.

   The event->data.buffer value will contain the bytes to send and the
   event->data.size value will contain the number of bytes to send.
   Note that event->data.buffer is not NUL terminated, and may include
   NUL characters in its data, so always use event->data.size!

   NOTE: Your SEND event handler must send or buffer the data in
   its raw form as provided by libtelnet.  If you wish to perform
   any kind of preprocessing on data you want to send to the other*/

	case TELNET_EV_SEND:

		# if SCDED_DBG >= 5
 		 SCDEFn->HexDumpOutFn ("\nTelnet req to send:",
			(char *) event->data.buffer,
			event->data.size);
		# endif

		// transfer the data from libtelnet to Send-Buffer 
		rc = Telnet_SendToSendBuff(conn
			,event->data.buffer
			,event->data.size);

		// report error
		if (rc < 0) {

			printf("free / err: %d", rc);
		}

		break;

// -----------------------------------------------------------------------

/* TELNET_EV_DATA:
   The DATA event is triggered whenever regular data (not part of any
   special TELNET command) is received.  For a client, this will be
   process output from the server.  For a server, this will be input
   typed by the user.

   The event->data.buffer value will contain the bytes received and the
   event->data.size value will contain the number of bytes received.
   Note that event->data.buffer is not NUL terminated!

   NOTE: there is no guarantee that user input or server output
   will be received in whole lines.  If you wish to process data
   a line at a time, you are responsible for buffering the data and
   checking for line terminators yourself!*/

	case TELNET_EV_DATA:
{

// to do: Line Buffering

		# if SCDED_DBG >= 5
 		 SCDEFn->HexDumpOutFn ("\nTelnet received:"
			,(char *) event->data.buffer
			,event->data.size);
		# endif

		// analyze, execute cmd and get retMsgMultiple from Fn
/*use?		struct headRetMsgMultiple_s headRetMsgMultipleFromFn =
			 SCDEFn->AnalyzeCommandFn((uint8_t *) event->data.buffer,
			(size_t) event->data.size);*/

		struct headRetMsgMultiple_s headRetMsgMultipleFromFn =
			 SCDEFn->AnalyzeCommandChainFn((uint8_t *) event->data.buffer,
			(size_t) event->data.size);

		// get the entries from retMsgMultiple till empty, if any
		while (!STAILQ_EMPTY(&headRetMsgMultipleFromFn)) {

			// for the retMsg elements
			strTextMultiple_t *retMsg =
				STAILQ_FIRST(&headRetMsgMultipleFromFn);

			// contains a msg?
			if (retMsg->strTextLen) {

				// and send it via telnet
				telnet_send(conn->tnHandle
					,(char *) retMsg->strText
					,retMsg->strTextLen);

				// add \r\n (CR+LF)
				telnet_send(conn->tnHandle
					,"\r\n"
					,2);
			}

			// done, remove this entry
			STAILQ_REMOVE(&headRetMsgMultipleFromFn, retMsg, strTextMultiple_s, entries);

			// free the msg-string
			free(retMsg->strText);

			// and the strTextMultiple_t
			free(retMsg);
		}
}
		break;



// -----------------------------------------------------------------------
/* TELNET_EV_IAC:
   The IAC event is triggered whenever a simple IAC command is
   received, such as the IAC EOR (end of record, also called go ahead
   or GA) command.

   The command received is in the event->iac.cmd value.

   The necessary processing depends on the specific commands; see
   the TELNET RFC for more information.*/ 

	case TELNET_EV_IAC:

	break;



// -----------------------------------------------------------------------
/* TELNET_EV_WILL:
   TELNET_EV_DO:
   The WILL and DO events are sent when a TELNET negotiation command
   of the same name is received.

   WILL events are sent by the remote end when they wish to be
   allowed to turn an option on on their end, or in confirmation
   after you have sent a DO command to them.

   DO events are sent by the remote end when they wish for you to
   turn on an option on your end, or in confirmation after you have
   sent a WILL command to them.

   In either case, the TELNET option under negotiation will be in
   event->neg.telopt field.

   libtelnet manages most of the pecularities of negotiation for you.
   For information on libtelnet's negotiation method, see:

    http://www.faqs.org/rfcs/rfc1143.html

   Note that in PROXY mode libtelnet will do no processing of its
   own for you.*/

	case TELNET_EV_WILL:

	break;

	case TELNET_EV_DO:

	break;



// -----------------------------------------------------------------------
/* TELNET_EV_WONT:
   TELNET_EV_DONT:
   The WONT and DONT events are sent when the remote end of the
   connection wishes to disable an option, when they are refusing to
   a support an option that you have asked for, or in confirmation of
   an option you have asked to be disabled.

   Most commonly WONT and DONT events are sent as rejections of
   features you requested by sending DO or WILL events.  Receiving
   these events means the TELNET option is not or will not be
   supported by the remote end, so give up.

   Sometimes WONT or DONT will be sent for TELNET options that are
   already enabled, but the remote end wishes to stop using.  You
   cannot decline.  These events are demands that must be complied
   with.  libtelnet will always send the appropriate response back
   without consulting your application.  These events are sent to
   allow your application to disable its own use of the features.

   In either case, the TELNET option under negotiation will be in
   event->neg.telopt field.

   Note that in PROXY mode libtelnet will do no processing of its
   own for you.*/

	case TELNET_EV_WONT:

	break;

	case TELNET_EV_DONT:

	break;



// -----------------------------------------------------------------------
/* TELNET_EV_SUBNEGOTIATION:
   Triggered whenever a TELNET sub-negotiation has been received.
   Sub-negotiations include the NAWS option for communicating
   terminal size to a server, the NEW-ENVIRON and TTYPE options for
   negotiating terminal features, and MUD-centric protocols such as
   ZMP, MSSP, and MCCP2.

   The event->sub->telopt value is the option under sub-negotiation.
   The remaining data (if any) is passed in event->sub.buffer and
   event->sub.size.  Note that most subnegotiation commands can include
   embedded NUL bytes in the subnegotiation data, and the data
   event->sub.buffer is not NUL terminated, so always use the
   event->sub.size value!

   The meaning and necessary processing for subnegotiations are
   defined in various TELNET RFCs and other informal specifications.
   A subnegotiation should never be sent unless the specific option
   has been enabled through the use of the telnet negotiation
   feature.*/

	case TELNET_EV_SUBNEGOTIATION:

	break;



// -----------------------------------------------------------------------
/* TTYPE/ENVIRON/NEW-ENVIRON/MSSP/ZMP SUPPORT:
   libtelnet parses these subnegotiation commands.  A special
   event will be sent for each, after the SUBNEGOTIATION event is
   sent.  Except in special circumstances, the SUBNEGOTIATION event
   should be ignored for these options and the special events should
   be handled explicitly.

// -----------------------------------------------------------------------
 TELNET_EV_COMPRESS:
   The COMPRESS event notifies the app that COMPRESS2/MCCP2
   compression has begun or ended.  Only servers can send compressed
   data, and hence only clients will receive compressed data.

   The event->command value will be 1 if compression has started and
   will be 0 if compression has ended.

// -----------------------------------------------------------------------
 TELNET_EV_ZMP:
   The event->zmp.argc field is the number of ZMP parameters, including
   the command name, that have been received.  The event->zmp.argv
   field is an array of strings, one for each ZMP parameter.  The
   command name will be in event->zmp.argv[0].

// -----------------------------------------------------------------------
 TELNET_EV_TTYPE:
   The event->ttype.cmd field will be either TELNET_TTYPE_SEND,
   TELNET_TTYPE_IS, TELNET_TTYPE_INFO.

   The actual terminal type will be in event->ttype.name.

// -----------------------------------------------------------------------
 TELNET_EV_ENVIRON:
   The event->environ.cmd field will be either TELNET_ENVIRON_IS,
   TELNET_ENVIRON_SEND, or TELNET_ENVIRON_INFO.

   The actual environment variable sent or requested will be sent
   in the event->environ.values field.  This is an array of
   structures with the following format:
     
     struct telnet_environ_t {
       unsigned char type;
       const char *var;
       const char *value;
     };

   The number of entries in the event->environ.values array is
   stored in event->environ.count.

   Note that libtelnet does not support the ESC byte for ENVIRON/
   NEW-ENVIRON.  Data using escaped bytes will not be parsed
   correctly.

// -----------------------------------------------------------------------
 TELNET_EV_MSSP:
   The event->mssp.values field is an array of telnet_environ_t
   structures.  The cmd field in each entry will have an
   unspecified value, while the var and value fields will always
   be set to the MSSP variable and value being set.  For multi-value
   MSSP variables, there will be multiple entries in the values
   array for each value, each with the same variable name set.

   The number of entries in the event->mssp.values array is
   stored in event->mssp.count.
 
// -----------------------------------------------------------------------
 TELNET_EV_WARNING:
   The WARNING event is sent whenever something has gone wrong inside
   of libtelnet (possibly due to malformed data sent by the other
   end) but which recovery is (likely) possible.  It may be safe to
   continue using the connection, but some data may have been lost or
   incorrectly interpreted.

   The event->error.msg field will contain a NUL terminated string
   explaining the error.*/



// -----------------------------------------------------------------------
/* TELNET_EV_ERROR:
   Similar to the WARNING event, the ERROR event is sent whenever
   something has gone wrong.  ERROR events are non-recoverable,
   however, and the application should immediately close the
   connection.  Whatever has happened is likely going only to result
   in garbage from libtelnet.  This is most likely to happen when a
   COMPRESS2 stream fails, but other problems can occur.

   The event->error.msg field will contain a NUL terminated string
   explaining the error.*/

	case TELNET_EV_ERROR:

		sprintf("TELNET error: %s"
			,(char*) event->error.msg);

		break;

// -----------------------------------------------------------------------

	default:

		break;

	}
}



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_SendToSendBuff
 *  Desc: Sends / adds data to the Send-Buffer, without doing the real transmission.
 *        May create (alloc) an send buffer. May also create an manage an Trailing Buffer if SB is full.
 *        If len is -1 the data is seen as a C-string. Len will be determined.
 *  Info: CAN SEND FROM FLASH. DO NOT USE LENGTH AUTO-DETECTION IF SENDING DATA DIRECT FROM FLASH !!!
 *  Para: Telnet_DConnSlotData_t *conn -> ptr to the connection slot
 *        const char *data -> ptr to the data to send 
 *        int len -> length of data. If -1 the data is seen as a C-string and len will be determined.
 *  Rets: int bytes free in sendbuff
 * --------------------------------------------------------------------------------------------------
 */
int
Telnet_SendToSendBuff(Telnet_DConnSlotData_t *conn
		,const char *data
		,int len)
  {

  // if len is -1, the data is seen as a C-string. Determine length ..
  if (len < 0) len = strlen(data);

  // in case we have nothing to send -> return free bytes in sendbuff
  if (!len) return (MAX_SENDBUFF_LEN - conn->sendBuffLen);

  // WE WILL DEFINITIVLY SEND DATA HERE - MAKE SEND REQUEST
  conn->conn->common.Common_CtrlRegA |= F_WANTS_WRITE;

  // alloc Send-Buffer, if not already done
  if (conn->sendBuff == NULL)
	conn->sendBuff = (char*) malloc(MAX_SENDBUFF_LEN);

  // will data fit in Send-Buffer? Then simply copy ...
  if (conn->sendBuffLen + len <= MAX_SENDBUFF_LEN)

	// data fits, copy to Send-Buffer ...
	{

	// copy to Send-Buffer
	SCDE_memcpy_plus(conn->sendBuff + conn->sendBuffLen, data, len);
	conn->sendBuffLen += len;

	// return free bytes in Send-Buffer
	return (MAX_SENDBUFF_LEN - conn->sendBuffLen);

	}

  else

	 // else data does NOT fit in Send-Buffer ...
	{

	// Step 1: copy data to Send-Buffer till full
	int SendBufFree = MAX_SENDBUFF_LEN - conn->sendBuffLen;

	if (SendBufFree)

		{

		// copy to Send-Buffer
		SCDE_memcpy_plus(conn->sendBuff + conn->sendBuffLen, data, SendBufFree);

		conn->sendBuffLen += SendBufFree;

		}

	// Step 2: create / add the rest of data to Trailing-Buffer
	int NewTrailingBuffLen;

	// if there is no Trailing-Buffer / empty ... -> get new length
	if (conn->TrailingBuff == NULL) NewTrailingBuffLen = len - SendBufFree;

	// else if there was a Trailing-Buffer ... -> get new length
	else NewTrailingBuffLen =
		conn->TrailingBuffLen + len - SendBufFree;

	// alloc new Trailing-Buffer
	char *NewTrailingBuff = (char*) malloc(NewTrailingBuffLen+1);

	// copy old Trailing-Buffer to new Trailing-Buffer, and dealloc old
	if (conn->TrailingBuff != NULL)

		{

		// copy old Trailing-Buffer to new Trailing-Buffer
		memcpy (NewTrailingBuff, conn->TrailingBuff, conn->TrailingBuffLen);

		// dealloc old Trailing-Buffer
		free(conn->TrailingBuff);

		}

	// add the rest of the new data to the Trailing Buffer and save
	SCDE_memcpy_plus(NewTrailingBuff + conn->TrailingBuffLen, data + SendBufFree, len - SendBufFree);

	# if SCDED_DBG >= 3
	printf("|note: adding %d bytes to TrailingBuff>",
		NewTrailingBuffLen);
	#endif

	// Store 
	conn->TrailingBuff = NewTrailingBuff;
	conn->TrailingBuffLen = NewTrailingBuffLen;

	}

  // here 0 bytes free in sendbuff
  return 0;

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_TransmitSendBuff
 *  Desc: Function to finally transmitts the data in the Send-Buffer (conn->sendBuff - if any)
 *
 *  Para: Telnet_DConnSlotData_t *conn -> ptr to connection slot
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_TransmitSendBuff(Telnet_DConnSlotData_t *conn) 

  {

  // do we have an allocated Send-Buffer -> there is something to send in the Send-Buffer !
  if (conn->sendBuff) //(conn->sendBuffLen)

	{

	# if SCDED_DBG >= 3
	printf("|TX SendBuff, slot %d, to remote:%d.%d.%d.%d:%d from local port:%d, len=%d. mem:%d>",
		conn->SlotNo,
		conn->conn->proto.tcp->remote_ip[0],
		conn->conn->proto.tcp->remote_ip[1],
		conn->conn->proto.tcp->remote_ip[2],
		conn->conn->proto.tcp->remote_ip[3],
		conn->conn->proto.tcp->remote_port,
		conn->conn->proto.tcp->local_port,
		conn->sendBuffLen,
		system_get_free_heap_size());
	# endif

	# if SCDED_DBG >= 5
 	 SCDEFn->HexDumpOutFn ("\nTX-SendBuff",
		conn->sendBuff,
		conn->sendBuffLen);
	# endif

	int8_t Result = Telnet_sent(conn->conn,
		(uint8_t*) conn->sendBuff,
		conn->sendBuffLen);

	// show error on debug term...
 	if (Result)

		{

		# if SCDED_DBG >= 1
		printf("\n|TX-Err:%d!>"
			,Result);
		# endif

		}

	// free Send-Buffer
	free(conn->sendBuff);

	// indicate -> no Send Buffer
	conn->sendBuff = NULL;

	// init length for next usage
	conn->sendBuffLen = 0;

			// alter code ?
			// We sent data. We are not allowed to send again till SentCb is fired.
			// Indicate this by F_TXED_CALLBACK_PENDING in ConCtrl
			conn->ConnCtrlFlags |= F_TXED_CALLBACK_PENDING;

	}

  else

	{

	# if SCDED_DBG >= 3
	printf("|no allocated SendBuff, no TX!>");
	# endif

	}

  }



/*
 * --------------------------------------------------------------------------------------------------
 *  FName: Telnet_SentCb
 *  Desc: Data was sent callback is triggered when data was sent to a client conn of Telnet
 *  Para: void* arg -> Telnet_Definition_t* Telnet_Definition
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_SentCb(void* arg)
  {

  // the arg is a ptr to the platform specific conn
  //struct espconn   *platform_conn = arg;	// ESP 8266 NonOS
  Telnet_Definition_t *platform_conn = arg;	// ESP 32 RTOS

  // get assigned TelnetD-Connection-Slot-Data
  Telnet_DConnSlotData_t *conn
	= platform_conn->reverse;

  # if SCDED_DBG >= 3
  printf("\nTelnet SentCb, slot:%d, remote:%d.%d.%d.%d:%d, local port:%d, mem:%d>"
	,conn->SlotNo
	,conn->conn->proto.tcp->remote_ip[0]
	,conn->conn->proto.tcp->remote_ip[1]
	,conn->conn->proto.tcp->remote_ip[2]
	,conn->conn->proto.tcp->remote_ip[3]
	,conn->conn->proto.tcp->remote_port
	,conn->conn->proto.tcp->local_port
	,system_get_free_heap_size());
  #endif

// --------------------------------------------------------------------------------------------------

  # if SCDED_DBG >= 1
  if (!(conn->ConnCtrlFlags & F_TXED_CALLBACK_PENDING))
	{
	printf("|Err! TXedCbFlag missing>");
	}
  #endif

  // speed up cpu
//  system_update_cpu_freq(SYS_CPU_160MHz);

  // Connection-Control Flags management for Sent-Callback
  // - CLEAR: F_TXED_CALLBACK_PENDING, because SentCb is fired. TXED_CALLBACK is no longer pending ...
  // - CLEAR: F_CALLED_BY_RXED_CALLBACK, because this is not RX-Callback ...
  // - CLEAR: F_GENERATE_IDLE_CALLBACK, because now we have a callback. No Idle Callback is needed ...
  // clr flags
  conn->ConnCtrlFlags &= ~(F_TXED_CALLBACK_PENDING + F_CALLED_BY_RXED_CALLBACK + F_GENERATE_IDLE_CALLBACK);

// --------------------------------------------------------------------------------------------------

  // Response to open connection...
//  SCDED_RespToOpenConn(conn);


  // transmit data - and free (if any)
  Telnet_TransmitSendBuff(conn); 

// --------------------------------------------------------------------------------------------------

  // slow down cpu
 // system_update_cpu_freq(SYS_CPU_80MHz);

  }



/*
 * --------------------------------------------------------------------------------------------------
 *  FName: Telnet_RecvCb
 *  Desc: Received callback is triggered when a data block is received from an client conn of Telnet
 *  Para: void* arg -> Telnet_Definition_t* Telnet_Definition
 *       char *recvdata -> ptr to received data
 *       unsigned short recvlen -> length of received data 
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_RecvCb(void *arg,
		char *recvdata,
		unsigned short recvlen) 
  {

  // the arg is a ptr to the platform specific conn
  //struct espconn   *platform_conn = arg;	// ESP 8266 NonOS
  Telnet_Definition_t *platform_conn = arg;	// ESP 32 RTOS

  // get assigned HTTPD-Connection-Slot-Data
  Telnet_DConnSlotData_t *conn
	= platform_conn->reverse;

  # if SCDED_DBG >= 3
  printf("\nTelnet RecvCb, slot %d, remote:%d.%d.%d.%d:%d,local port:%d, len:%d, mem:%d>"
	,platform_conn->SlotNo
	,platform_conn->proto.tcp->remote_ip[0]
	,platform_conn->proto.tcp->remote_ip[1]
	,platform_conn->proto.tcp->remote_ip[2]
	,platform_conn->proto.tcp->remote_ip[3]
	,platform_conn->proto.tcp->remote_port
	,platform_conn->proto.tcp->local_port
	,recvlen
	,system_get_free_heap_size());

  #endif

  # if SCDED_DBG >= 5
  SCDEFn->HexDumpOutFn ("RX-HEX", recvdata, recvlen);
  # endif

// --------------------------------------------------------------------------------------------------

  // speed up cpu
//  system_update_cpu_freq(SYS_CPU_160MHz);

  // Connection-Control Flags management for Received-Callback
  // - CLEAR: F_GENERATE_IDLE_CALLBACK, because now we have Idle Callback and it is needed once ...
  // - SET: F_CALLED_BY_RXED_CALLBACK, because this is RX-Callback ... ...
  // clr flags
  conn->ConnCtrlFlags &= ~(F_GENERATE_IDLE_CALLBACK);
  // set flags
  conn->ConnCtrlFlags |= (F_CALLED_BY_RXED_CALLBACK);

// --------------------------------------------------------------------------------------------------

  // Reset Inactivity Timer because we had an callback ...
  conn->InactivityTimer = 0;

// --------------------------------------------------------------------------------------------------

  // give the input to telnet lib for processing
  telnet_recv(conn->tnHandle, (char *) recvdata, recvlen); //(char *)buffer, len);

// --------------------------------------------------------------------------------------------------

  // slow down cpu
//  system_update_cpu_freq(SYS_CPU_80MHz);

  }



/*
 * --------------------------------------------------------------------------------------------------
 *  FName: Telnet_ReconCb
 *  Desc: Reconnect Info callback is triggered when the connection to client conn of SCDED is broken
 *        its unclear what to do in this cases ...
 *  Para: void* arg -> Telnet_Definition_t* Telnet_Definition
 *
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_ReconCb(void *arg
		,int8_t error)
  {

  // the arg is a ptr to the platform specific conn
  //struct espconn   *platform_conn = arg;	// ESP 8266 NonOS
  Telnet_Definition_t *platform_conn = arg;	// ESP 32 RTOS

  // get assigned TelnetD-Connection-Slot-Data
  Telnet_DConnSlotData_t *conn
	= platform_conn->reverse;

  # if SCDED_DBG >= 3
  printf("\nTelnet ReconCb, slot %d, remote:%d.%d.%d.%d:%d,local port:%d, error:%d, mem:%d>"
	,platform_conn->SlotNo
	,platform_conn->proto.tcp->remote_ip[0]
	,platform_conn->proto.tcp->remote_ip[1]
	,platform_conn->proto.tcp->remote_ip[2]
	,platform_conn->proto.tcp->remote_ip[3]
	,platform_conn->proto.tcp->remote_port
	,platform_conn->proto.tcp->local_port
	,error
	,system_get_free_heap_size());

  #endif

// ---------------------------------------------------------------------------------------------------

  // process error information here! 
/*
sint8 err : error code
ESCONN_TIMEOUT - Timeout
ESPCONN_ABRT - TCP connection aborted
ESPCONN_RST - TCP connection abort
ESPCONN_CLSD - TCP connection closed
ESPCONN_CONN - TCP connection
ESPCONN_HANDSHAKE - TCP SSL handshake fail
ESPCONN_PROTO_MSG - SSL application invalid
*/

  Telnet_DisconCb(arg);

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_DisconCb
 *  Desc: Disconnected callback -> conn is disconnected -> clean up, free memory
 *        its unclear what to do in this cases ...
 *  Para: void* arg -> Telnet_Definition_t* Telnet_Definition
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_DisconCb(void *arg)
  {

  // the arg is a ptr to the platform specific conn
//struct espconn     *platform_conn = arg;	// ESP 8266 NonOS
  Telnet_Definition_t *platform_conn = arg;	// ESP 32 RTOS

  // get assigned TelnetD-Connection-Slot-Data
  Telnet_DConnSlotData_t *conn
	= platform_conn->reverse;

 # if SCDED_DBG >= 3
  printf("\nWebIf DisconCb, slot %d, remote:%d.%d.%d.%d:%d, local port:%d, mem:%d>"
	,platform_conn->SlotNo
	,platform_conn->proto.tcp->remote_ip[0]
	,platform_conn->proto.tcp->remote_ip[1]
	,platform_conn->proto.tcp->remote_ip[2]
	,platform_conn->proto.tcp->remote_ip[3]
	,platform_conn->proto.tcp->remote_port
	,platform_conn->proto.tcp->local_port
	,system_get_free_heap_size());
  #endif

// ---------------------------------------------------------------------------------------------------

  // remove the conn / mark that the connection is gone
  conn->conn = NULL;

  // !!! CONN IS GONE FROM THIS POINT !!!

  // free allocated memory for the Send-Buffer, if any
  if (conn->sendBuff != NULL) free(conn->sendBuff);

  // free allocated memory for the Trailing-Buffer, if any
  if (conn->TrailingBuff != NULL) free(conn->TrailingBuff);


  // nicht schön
  // init http parser for new conn (goal is to free allocated memory!)
//  HTTPD_ParserInit(conn, HTTP_BOTH);


  // force libtelnet to free all allocated memory
  if (conn->tnHandle != NULL) telnet_free(conn->tnHandle);

  // finally free allocated memory for this Telnet_DConnSlotData_t struct
  free(conn);

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_ConnCb
 *  Desc: Connected callback is triggered in case of new connections to Telnet-Daemon
 *  Info:
 *  Para: void* arg -> Telnet_Definition_t* Telnet_Definition
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
void
Telnet_ConnCb(void *arg)
  {
  // the arg is a ptr to the platform specific conn
  //struct espconn   *platform_conn = arg;	// ESP 8266 NonOS
  Telnet_Definition_t *platform_conn = arg;	// ESP 32 RTOS

// --------------------------------------------------------------------------------------------------

// # if SCDED_DBG >= 3
  printf("\nTelnet ConCb, gets slot %d of %d, from remote:%d.%d.%d.%d:%d to local port:%d, mem:%d>"
	,platform_conn->SlotNo
	,MAX_SCDED_CONN
	,platform_conn->proto.tcp->remote_ip[0]
	,platform_conn->proto.tcp->remote_ip[1]
	,platform_conn->proto.tcp->remote_ip[2]
	,platform_conn->proto.tcp->remote_ip[3]
	,platform_conn->proto.tcp->remote_port
	,platform_conn->proto.tcp->local_port
	,system_get_free_heap_size());

//  #endif

// --------------------------------------------------------------------------------------------------

  // alloc conn-slot memory
  Telnet_DConnSlotData_t *conn 
	= (Telnet_DConnSlotData_t*) malloc(sizeof(Telnet_DConnSlotData_t));

  // zero conn-slot memory
  memset(conn, 0, sizeof(Telnet_DConnSlotData_t));

  // store current connection in created conn slot
  conn->conn = platform_conn;

 // Write slot number for identification
  conn->SlotNo = conn->conn->SlotNo;

  // store reverse reference
  platform_conn->reverse = conn;

  // Register data received callback
  Telnet_espconn_regist_recvcb(platform_conn, Telnet_RecvCb);

  // Register data reconnection / error info callback
  Telnet_espconn_regist_reconcb(platform_conn, Telnet_ReconCb);

  // Register disconnected callback
  Telnet_espconn_regist_disconcb(platform_conn, Telnet_DisconCb);

  // Register data sent callback
  Telnet_espconn_regist_sentcb(platform_conn, Telnet_SentCb);

// --------------------------------------------------------------------------------------------------

  // telnet options for this client
  static const telnet_telopt_t my_telopts[] =
  {
        //       telopt                 us          him
	{ TELNET_TELOPT_ECHO,      TELNET_DONT, TELNET_DONT }, //TELNET_WILL / TELNET_DONT
	{ TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT }, //TELNET_WILL / TELNET_DONT
	{ TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   }, //TELNET_WONT / TELNET_DO
	{ TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   }, //TELNET_WONT / TELNET_DO
	{ TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   }, //TELNET_WONT / TELNET_DO
	{ TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   }, //TELNET_WONT / TELNET_DO
	{ TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT }, //TELNET_WILL / TELNET_DONT
	{ -1, 0, 0 }
  };

  // init libtelnet - for this client
  conn->tnHandle = telnet_init(my_telopts
	,Telnet_LibtelnetEventHandler
	,0
	,conn);

  // TX welcome message to connected terminal
  char *welcomeMSG;
  size_t welcomeMSGLen = asprintf(&welcomeMSG
		,"--- Smart-Connected-Device-Engine, Telnet-Access, Welcome! ---\r\n"
		 "Service provided by Type-Name: %.*s, Def-Name: %.*s\r\n\r\n"
		 "Type <Help> for command overview.\r\n\r\n"
		,(int) conn->conn->common.module->ProvidedByModule->typeNameLen
		,conn->conn->common.module->ProvidedByModule->typeName
		,(int) conn->conn->common.nameLen
		,conn->conn->common.name);

  telnet_send(conn->tnHandle
	,welcomeMSG
	,welcomeMSGLen);

//			// add \r\n
//			telnet_send(conn->tnHandle
//				,"\r\n"
//				,2);

  free(welcomeMSG);

  // transmit data - and free (if any)
  Telnet_TransmitSendBuff(conn); 

  }



/*
 *--------------------------------------------------------------------------------------------------
 *FName: Telnet_sent (espconn_sent compatible Fn)
 * Desc: Platform conn - Send data Fn
 * Para: Telnet_Definition_t *Telnet_Definition -> Telnet Definition
 *       uint8_t *Buff -> buffer with data to send
 *       uint Len -> length of data to send
 * Rets: int -> written data
 *--------------------------------------------------------------------------------------------------
 */
int
Telnet_sent(Telnet_Definition_t *Telnet_Definition
	,uint8_t *Buff
	,uint Len)
  {

  // select for want writing (F_WANTS_WRITE), because maybe we have more data to send ...
  Telnet_Definition->common.Common_CtrlRegA |= F_WANTS_WRITE;

  return (write(Telnet_Definition->common.fd, Buff, Len) >= 0);

  }



/*
 *--------------------------------------------------------------------------------------------------
 *FName: Telnet_disconnect (espconn_disconnect compatible Fn)
 * Desc: Platform conn - Disconnect Fn
 * Info: 
 * Para: Telnet_Definition_t *Telnet_Definition -> Telnet_Definition
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_disconnect(Telnet_Definition_t *Telnet_Definition)
  {

  // select for disconnecting (F_NEEDS_CLOSE)
  Telnet_Definition->Telnet_CtrlRegA |= F_NEEDS_CLOSE;

  // select for want writing (F_WANTS_WRITE), because the real close is done in the write select of code
  Telnet_Definition->common.Common_CtrlRegA |= F_WANTS_WRITE;

  }


/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_UndefineRaw
 *  Desc: internal cleanup and Telnet_Definition delete
 *  Para: Telnet_Definition_t *Telnet_Definition -> Telnet Definition that should be deleted
 *  Rets: -/-
 * --------------------------------------------------------------------------------------------------
 */
int 
Telnet_UndefineRaw(Telnet_Definition_t* Telnet_Definition)
  {

  // connection close
  close(Telnet_Definition->common.fd);

  // FD is gone ...
//Telnet_Definition->common.FD = -1;

// --- dealloc? non master conns?


  // remove WebIF Definition
  STAILQ_REMOVE(&SCDERoot->HeadCommon_Definitions
	,(Common_Definition_t*) Telnet_Definition
	,Common_Definition_s
	,entries);

  // free Name
  free(Telnet_Definition->common.name);

  // free TCP struct
  free(Telnet_Definition->proto.tcp); 

  // free WebIF_Definition
  free(Telnet_Definition);

  return 0;

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_Define
 *  Desc: Finalizes the defines a new "device" of 'Telnet' type. Contains devicespecific init code.
 *  Info: 
 *  Para: Common_Definition_t *Common_Definition -> prefilled Telnet Definition
 *        char *Definition -> the last part of the CommandDefine arg* 
 *  Rets: strTextMultiple_t* -> response text NULL=no text
 * --------------------------------------------------------------------------------------------------
 */
strTextMultiple_t* 
Telnet_Define(Common_Definition_t *Common_Definition)//, const char *Definition)
  {

  // for Fn response msg
  strTextMultiple_t *retMsg = NULL;

  // make common ptr to modul specific ptr
  Telnet_Definition_t* Telnet_Definition =
		  (Telnet_Definition_t*) Common_Definition;

  #if SCDEH_DBG >= 5
  printf("\n|Telnet_Def, Def:%.*s>"
	,(int) Telnet_Definition->common.definitionLen
	,Telnet_Definition->common.definition);
  #endif

//---

  // alloc memory for the HTTPD-Instance-Configuration
  Telnet_Definition->Telnet_DInstanceCfg =
		  (Telnet_DInstanceCfg_t*) malloc(sizeof(Telnet_DInstanceCfg_t));

  // memclr the HTTPD-Instance-Configuration
  memset(Telnet_Definition->Telnet_DInstanceCfg, 0, sizeof (Telnet_DInstanceCfg_t));

  //reset Slot Control Register Bitfield -> no connections yet!
//Telnet_Definition->Telnet_DInstance->SlotCtrlRegBF = 0; cleared by memset ...

// -------------------------------------------------------------------------------------------------

  // mark this as the server-socket
  Telnet_Definition->Telnet_CtrlRegA |= F_THIS_IS_SERVERSOCKET;

 // later from definition
  int Port = 23;

  // ???
  int opt = true;

  int ret;

  // master socket or listening fd
  int listenfd;

  // server address structure
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(Port);
#if defined(ESP_PLATFORM)
  server_addr.sin_len = sizeof(server_addr);
#endif

  // Create socket for incoming connections
  do {

	listenfd = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);

	// socked created or error?
	if (listenfd < 0) {

		SCDEFn->Log3Fn(Telnet_Definition->common.name
				,Telnet_Definition->common.nameLen
				,1
				,"Telnet_Define ERROR: failed to create sock! retriing\n");
#if defined(ESP_PLATFORM)
		vTaskDelay(1000/portTICK_RATE_MS);
#endif

		}

	} while(listenfd < 0);


  // set master socket to allow multiple connections , this is just a good habit, it will work without this
  ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt) );
  if (ret < 0 ) {

	SCDEFn->Log3Fn(Telnet_Definition->common.name
			,Telnet_Definition->common.nameLen
			,1
			,"Telnet_Define ERROR: 'setsockopt' failed! error:%d\n"
			,ret);

	}

  // bind the socket to the local port
  do {

	ret = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	if (ret != 0) {

		SCDEFn->Log3Fn(Telnet_Definition->common.name
				,Telnet_Definition->common.nameLen
				,1
				,"Telnet_Define ERROR: 'bind' failed! retriing\n");
#if defined(ESP_PLATFORM)
		vTaskDelay(1000/portTICK_RATE_MS);
#endif
		}

	} while(ret != 0);


//  # if SCDED_DBG >= 3
 /* printf("|local at:%u.%u.%u.%u:%u,port:%u>"
	,(uint8_t*) &server_addr.sin_addr.s_addr
	,(uint8_t*) &server_addr.sin_addr.s_addr+1
	,(uint8_t*) &server_addr.sin_addr.s_addr+2
	,(uint8_t*) &server_addr.sin_addr.s_addr+3
	,(uint16_t*) &server_addr.sin_port);*/

//  #endif




#define HTTPD_MAX_CONNECTIONS 10

  // listen to the local port
  do	{

	ret = listen(listenfd, HTTPD_MAX_CONNECTIONS);

	if (ret != 0)

		{

		SCDEFn->Log3Fn(Telnet_Definition->common.name
				,Telnet_Definition->common.nameLen
				,1
				,"Telnet_Define ERROR: 'listen' failed! retriing\n");
#if defined(ESP_PLATFORM)
		vTaskDelay(1000/portTICK_RATE_MS);
#endif
		}

	} while(ret != 0);

  // store FD to Definition. Will than be processed in global loop ...
  Telnet_Definition->common.fd = listenfd;

  // using TCP, create, fill and store struct
  esp_tcp *tcp = malloc (sizeof(esp_tcp));
//  tcp->local_ip = server_addr.sin_addr.s_addr;
  tcp->local_port = server_addr.sin_port;
  Telnet_Definition->proto.tcp = tcp;

  Telnet_Definition->type = ESPCONN_TCP;
  Telnet_Definition->state = ESPCONN_NONE;

  // register the connect-callback - used ...
  Telnet_espconn_regist_connectcb(Telnet_Definition,
	Telnet_ConnCb);

  SCDEFn->Log3Fn(Telnet_Definition->common.name
		  ,Telnet_Definition->common.nameLen
		  ,1
		  ,"Defined a Telnet at X.X.X.X:YYYY, FD is:%d\n"
		  ,listenfd);

  return retMsg;

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_DirectRead
 *  Desc: Called by the global select-loop when FD is in read-set
 *  Info: 
 *  Para: Common_Definition* Common_Definition -> XXX_Definition of the FD owners definition
 *  Rets: ? - unused
 * --------------------------------------------------------------------------------------------------
 */
int 
Telnet_DirectRead(Common_Definition_t* Common_Definition)
  {

  #if SCDEH_DBG >= 5
  printf("\n|Telnet_DirectRead Entering..., Name:%.*s>"
	,(int) Common_Definition->nameLen
	,(char*)Common_Definition->name);
  #endif

  // make common ptr to modul specific ptr
  Telnet_Definition_t *Telnet_Definition =
	(Telnet_Definition_t*) Common_Definition;

  // ptr to receive buffer
  char *RecvBuf;

  int32_t len;

  // FD of the new client connection
  int32_t NewClientFD;

  // sockaddr_in, to get info about new client, contains port, ip-addr, family
  struct sockaddr_in remote_addr;
  struct sockaddr_in local_addr;
  
  // sockaddr, to get info about new client
  struct sockaddr name;

// -------------------------------------------------------------------------------------------------

  // Check Flag 'THIS_IS_SERVERSOCKET' in Telnet_CtrlRegA. This indicates a server-socket. 
  // -> Manage a new connection.
  if (Telnet_Definition->Telnet_CtrlRegA & F_THIS_IS_SERVERSOCKET) {

// -------------------------------------------------------------------------------------------------

	// check slot availiability, get a slot no. or RETURN, mark slot as 'in use'
	uint32_t SlotCtrlRegBF = Telnet_Definition->Telnet_DInstanceCfg->SlotCtrlRegBF;

	uint8_t NewSlotNo;

	// MAX_SLOTS_PER_INSTANCE -> uint32_t BF used in code -> 32 64?
	for (NewSlotNo = 0; NewSlotNo < 32; NewSlotNo++) {

		if (!(SlotCtrlRegBF & (0b1 <<NewSlotNo) ))
			break;
		}

	// Check for no slots free error
	if (NewSlotNo >=32) {

		SCDEFn->Log3Fn(Telnet_Definition->common.name
			,Telnet_Definition->common.nameLen
			,1
			,"Telnet_DirectRead Error no slots free...\n");

		// do not accept new connection
		return 0;//??

		}

	// mark found slot as used in Slot-Control-Register-Bitfield
	SlotCtrlRegBF |= (0b1 << NewSlotNo);

	// store Slot-Control-Register-Bitfield
	Telnet_Definition->Telnet_DInstanceCfg->SlotCtrlRegBF = SlotCtrlRegBF;

// --------------------------------------------------------------------------------------------------

	#if SCDEH_DBG >= 5
	printf("|accept a new conn>");
	#endif

	len = sizeof(struct sockaddr_in);

	// get FD from new connection and store remote address
	NewClientFD = accept(Telnet_Definition->common.fd
		,(struct sockaddr *) &remote_addr
		,(socklen_t *) &len);

	// check for error
	if (NewClientFD < 0) {

		SCDEFn->Log3Fn(Telnet_Definition->common.name
				,Telnet_Definition->common.nameLen
				,1
				,"Telnet_DirectRead Error! accept failed...\n");

		// error, process next slot?
		return 0;//??

		}

// --------------------------------------------------------------------------------------------------

	// create a new Telnet Definition
	Telnet_Definition_t* NewTelnet_Definition;

	// alloc mem for modul specific definition structure (Common_Definition_t + X)
	NewTelnet_Definition = 
		(Telnet_Definition_t*) malloc(sizeof(Telnet_Definition_t));

	// zero the struct
	memset(NewTelnet_Definition, 0, sizeof (Telnet_Definition_t));

	// store new Telnet Definition
	STAILQ_INSERT_HEAD(&SCDERoot->HeadCommon_Definitions
		,(Common_Definition_t*) NewTelnet_Definition
		, entries);

// --------------------------------------------------------------------------------------------------
				
	// set parameters for new connection 
	int keepAlive = 1;	//enable keepalive
 	int keepIdle = 60;	//60s
 	int keepInterval = 5;	//5s
 	int keepCount = 3;	//retry times

 	setsockopt(NewClientFD, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepAlive, sizeof(keepAlive));
  	setsockopt(NewClientFD, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &keepIdle, sizeof(keepIdle));
  	setsockopt(NewClientFD, IPPROTO_TCP, TCP_KEEPINTVL, (void *) &keepInterval, sizeof(keepInterval));
  	setsockopt(NewClientFD, IPPROTO_TCP, TCP_KEEPCNT, (void *) &keepCount, sizeof(keepCount));

//---
			
 	// store clients FD to new WebIF Definition
 	NewTelnet_Definition->common.fd = NewClientFD;

	// copy link to HTTPD-Instance-Configuration
	NewTelnet_Definition->Telnet_DInstanceCfg
		= Telnet_Definition->Telnet_DInstanceCfg;

	// copy ptr to associated module (this module)
	NewTelnet_Definition->common.module
		= Telnet_Definition->common.module;

	// store Slot-Number
	NewTelnet_Definition->SlotNo = NewSlotNo;

	// clear Flag 'WANT_WRITE' in new WebIF Definition
 	NewTelnet_Definition->common.Common_CtrlRegA
		&= ~F_WANTS_WRITE;

	// clear Flag 'NEEDS_CLOSE' in new WebIF Definition
 	NewTelnet_Definition->Telnet_CtrlRegA
		&= ~F_NEEDS_CLOSE;

	// get info about new client (port, ip, ...)	
 	socklen_t slen = sizeof(name);
  	getpeername(NewClientFD, &name, (socklen_t *) &slen);

  	struct sockaddr_in *piname = (struct sockaddr_in *) &name;

 	 // store port + ip info from new client to new WebIF Definition
//  	NewTelnet_Definition->port = piname->sin_port;
//  	memcpy(&NewTelnet_Definition->ip
//		, &piname->sin_addr.s_addr
//		, sizeof(NewTelnet_Definition->ip));


	// using TCP, create struct
	esp_tcp *tcp = (esp_tcp*) malloc (sizeof(esp_tcp));

	// using TCP, fill struct
	tcp->remote_port = piname->sin_port;	// port
	memcpy(&tcp->remote_ip	// dest-ip!?
		, &piname->sin_addr.s_addr
		, sizeof(tcp->remote_ip));

	// store TCP struct
	NewTelnet_Definition->proto.tcp = tcp;

	// give definition a new unique name
	NewTelnet_Definition->common.nameLen = 
		asprintf((char**)&NewTelnet_Definition->common.name
		,"%.*s.%d.%d.%d.%d.%u"
		,(int) Telnet_Definition->common.nameLen
		,Telnet_Definition->common.name
		,NewTelnet_Definition->proto.tcp->remote_ip[0]
		,NewTelnet_Definition->proto.tcp->remote_ip[1]
		,NewTelnet_Definition->proto.tcp->remote_ip[2]
		,NewTelnet_Definition->proto.tcp->remote_ip[3]
		,NewTelnet_Definition->proto.tcp->remote_port);

  // assign an unique number
  NewTelnet_Definition->common.nr = SCDERoot->DevCount++;

  // make this definition temporary
	NewTelnet_Definition->common.defCtrlRegA |= F_TEMPORARY;

	// official log entry
	SCDEFn->Log3Fn(NewTelnet_Definition->common.name
		,NewTelnet_Definition->common.nameLen
		,1
		,"Created a new Definition for conn - Name:%.*s TypeName:%.*s Slot:%d FD:%d\n"
		,NewTelnet_Definition->common.nameLen
		,NewTelnet_Definition->common.name
		,NewTelnet_Definition->common.module->ProvidedByModule->typeNameLen
		,NewTelnet_Definition->common.module->ProvidedByModule->typeName
		,NewTelnet_Definition->SlotNo
		,NewTelnet_Definition->common.fd);

 	// execute WebIF Connect Callback to init 			
	Telnet_ConnCb(NewTelnet_Definition);

	}

// --------------------------------------------------------------------------------------------------

  // Flag 'THIS_IS_SERVERSOCKET' cleared. This indicates that this is not the server-socket.
  // -> Manage old connection.
  else {

 #if SCDEH_DBG >= 5
 printf("recv>");
 #endif

	RecvBuf = (char*) malloc(RECV_BUF_SIZE);

	// error - got no buffer !
	if (RecvBuf == NULL) {

		printf("platHttpServerTask: memory exhausted!\n");

		Telnet_DisconCb(Telnet_Definition);

		close(Telnet_Definition->common.fd);

		Telnet_Definition->common.fd = -1;

		}

	// receive the data
	int32_t ret = recv(Telnet_Definition->common.fd
		,RecvBuf
		,RECV_BUF_SIZE
		,0);

	// process, if received data
	if (ret > 0) {

		// execute Received Callback
		Telnet_RecvCb(Telnet_Definition, RecvBuf, ret);

		}

	// or has remote closed the connection ?
	else if (ret == 0) {

		// execute Disconnect Callback
		Telnet_DisconCb(Telnet_Definition);

		// undefinde this Telnet_Definition
		Telnet_UndefineRaw(Telnet_Definition);

		}


	// else we got an error ...
	else 	{

		// execute Error Callback
		Telnet_ReconCb(Telnet_Definition, ret);

		// undefinde this Telnet_Definition
		Telnet_UndefineRaw(Telnet_Definition);

		}

	}

  // unused
  return 0;

  }




/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_DirectWrite
 *  Desc: Called by the global select-loop when fd is in write-set 
 *  Info: 
 *  Para: 
 *  Rets: 
 * --------------------------------------------------------------------------------------------------
 */
int 
Telnet_DirectWrite(Common_Definition_t* Common_Definition)
  {

  #if SCDEH_DBG >= 5
  printf("\n|Telnet_DirectWrite Entering..., Name:%.*s>"
	,(int) Common_Definition->nameLen
	,(char*)Common_Definition->name);
  #endif

  // make common ptr to modul specific ptr
  Telnet_Definition_t* Telnet_Definition = (Telnet_Definition_t*) Common_Definition;

  // clear Flag F_WANT_WRITE, will be set again when more data is send
//  Telnet_Definition->common.Common_CtrlRegA &= ~F_WANTS_WRITE;	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! in mainfn

// --------------------------------------------------------------------------------------------------

  // execute disconnection (indicated by NEEDS_CLOSE flag) or send more data ...
  if (Telnet_Definition->Telnet_CtrlRegA & F_NEEDS_CLOSE) {

	// execute Disconnect Callback
	Telnet_DisconCb(Telnet_Definition);

	// undefinde this Telnet_Definition
	Telnet_UndefineRaw(Telnet_Definition);

	}

// --------------------------------------------------------------------------------------------------

  // send more data ... by executing SendCb ...
  else	{

	// execute Sent Callback
	Telnet_SentCb(Telnet_Definition);

	}


  return 0;

  }



/**
 * --------------------------------------------------------------------------------------------------
 *  FName: Telnet_Set
 *  Desc: Processes the device-specific command line arguments from the set command
 *  Info: Invoked by cmd-line 'Set Telnet_Definition.common.Name setArgs'
 *  Para: Telnet_Definition_t *Telnet_Definition -> WebIF Definition that should get a set cmd
 *        uint8_t *setArgs -> the setArgs
 *        size_t setArgsLen -> length of the setArgs
 *  Rets: strTextMultiple_t* -> response text in allocated memory, NULL=no text
 * --------------------------------------------------------------------------------------------------
 */
strTextMultiple_t*
Telnet_Set(Common_Definition_t* Common_Definition
	,uint8_t *setArgs
	,size_t setArgsLen)
  {

  // for Fn response msg
  strTextMultiple_t *retMsg = NULL;

  // make common ptr to modul specific ptr
  Telnet_Definition_t* Telnet_Definition = (Telnet_Definition_t*) Common_Definition;

  #if SCDEH_DBG >= 5
  printf("\n|Telnet_Set, Name:%.*s, got args:%.*s>"
	,(int) Telnet_Definition->common.nameLen
	,Telnet_Definition->common.name
	,(int) setArgsLen
	,setArgs);
  #endif


  // response with error text
	// alloc mem for retMsg
  retMsg = malloc(sizeof(strTextMultiple_t));

  // response with error text
  retMsg->strTextLen = asprintf((char**) &retMsg->strText
	,"Telnet_Set, Name:%.*s, got args:%.*s"
	,(int) Telnet_Definition->common.nameLen
	,Telnet_Definition->common.name
	,(int) setArgsLen
	,setArgs);

  return retMsg;

  }



/* --------------------------------------------------------------------------------------------------
 *  FName: Telnet_Undefine
 *  Desc: Removes the define of an "device" of 'WebIF' type. Contains devicespecific init code.
 *  Info: Invoked by cmd-line 'Undefine Telnet_Definition.common.Name'
 *  Para: Telnet_Definition_t *Telnet_Definition -> WebIF Definition that should be removed
 *  Rets: strTextMultiple_t* -> response text NULL=no text
 * --------------------------------------------------------------------------------------------------
 */
strTextMultiple_t*
Telnet_Undefine(Common_Definition_t* Common_Definition)
  {

  // for Fn response msg
  strTextMultiple_t *retMsg = NULL;

  // make common ptr to modul specific ptr
  Telnet_Definition_t* Telnet_Definition = (Telnet_Definition_t*) Common_Definition;

  #if SCDEH_DBG >= 5
  printf("\n|Telnet_Undefine, Name:%.*s>"
	,(int) Telnet_Definition->common.nameLen
	,Telnet_Definition->common.name);
  #endif


  // response with error text
	// alloc mem for retMsg
  retMsg = malloc(sizeof(strTextMultiple_t));

  // response with error text
  retMsg->strTextLen = asprintf((char**) &retMsg->strText
	,"Telnet_Set, Name:%.*s"
	,(int) Telnet_Definition->common.nameLen
	,Telnet_Definition->common.name);

  return retMsg;

  }









/*
 *--------------------------------------------------------------------------------------------------
 *FName: espconn_regist_recvcb
 * Desc: Platform conn - Register data received callback
 * Para: 
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_espconn_regist_recvcb(Telnet_Definition_t *conn
			, espconn_recv_callback recv_callback)
  {

  conn->recv_callback 
	= recv_callback;

  }


/*
 *--------------------------------------------------------------------------------------------------
 *FName: espconn_regist_Connectcb
 * Desc: Platform conn - Register disconnected callback
 * Para: 
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_espconn_regist_connectcb(Telnet_Definition_t *conn
		, espconn_connect_callback connect_callback)

  {

 conn->proto.tcp->connect_callback
	= connect_callback;

  }



/*
 *--------------------------------------------------------------------------------------------------
 *FName: espconn_regist_reconcb
 * Desc: Platform conn - Register error info callback
 * Para: 
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_espconn_regist_reconcb(Telnet_Definition_t *conn
		, espconn_reconnect_callback reconnect_callback)
  {

  conn->proto.tcp->reconnect_callback
	= reconnect_callback;

  }



/*
 *--------------------------------------------------------------------------------------------------
 *FName: espconn_regist_disconcb
 * Desc: Platform conn - Register disconnected callback
 * Para: 
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_espconn_regist_disconcb(Telnet_Definition_t *conn
		, espconn_connect_callback disconnect_callback)
  {

 conn->proto.tcp->disconnect_callback
	= disconnect_callback;

  }



/*
 *--------------------------------------------------------------------------------------------------
 *FName: espconn_regist_sentcb
 * Desc: Platform conn - Register data sent callback
 * Para: 
 * Rets: -/-
 *--------------------------------------------------------------------------------------------------
 */
void
Telnet_espconn_regist_sentcb(Telnet_Definition_t *conn
		, espconn_sent_callback send_callback)
  {

  conn->send_callback
	= send_callback;

  }




















