#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>

int str2uuid( const char *uuid_str, uuid_t *uuid ) 
{
    uint32_t uuid_int[4];
    char *endpointer;

    if( strlen( uuid_str ) == 36 ) {
    
        // Parse uuid128 standard format: 12345678-9012-3456-7890-123456789012
        char buf[9] = { 0 };
		
		//check if dashes are in valid indexes in the string
        if( uuid_str[8] != '-' && uuid_str[13] != '-' &&
            uuid_str[18] != '-'  && uuid_str[23] != '-' ) {
            return 0;
        }
        
        // first 8-bytes
        strncpy(buf, uuid_str, 8);
        uuid_int[0] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
		}
		
        // second 8-bytes
        strncpy(buf, uuid_str+9, 4);
        strncpy(buf+4, uuid_str+14, 4);
        uuid_int[1] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
        }

        // third 8-bytes
        strncpy(buf, uuid_str+19, 4);
        strncpy(buf+4, uuid_str+24, 4);
        uuid_int[2] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
		}
		
        // fourth 8-bytes
        strncpy(buf, uuid_str+28, 8);
        uuid_int[3] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
        }

        if( uuid != NULL ) {
        	sdp_uuid128_create( uuid, uuid_int );
        }
        
    } 
    
    else if ( strlen( uuid_str ) == 8 ) {
        // 32-bit reserved UUID
        uint32_t i = strtoul( uuid_str, &endpointer, 16 );
        if( endpointer != uuid_str + 8 ) return 0;
        if( uuid != NULL ) sdp_uuid32_create( uuid, i );
    } 
    
    else if( strlen( uuid_str ) == 4 ) {
        // 16-bit reserved UUID
        int i = strtol( uuid_str, &endpointer, 16 );
        if( endpointer != uuid_str + 4 ) return 0;
        if( uuid != NULL ) sdp_uuid16_create( uuid, i );
    } 
    
    else {
        return 0;
    }

    return 1;
}




int main(void) {

	//scanning variables and structs
    int i = 0;
    int j = 0;
    int err = 0;
    int sock = 0;
    int dev_id = -1;
    int foundName = 0;

    struct hci_dev_info dev_info;
    inquiry_info *info = NULL;
    int num_rsp= 0;
    int length = 8; //scan for 1.28 * 8s
    int flags = 0;
    bdaddr_t ba;
    char addr[19] = { 0 };
    char name[248] = { 0 };
    char targetAddr[19] = { 0 };
    char targetName[248] = { 0 };
    uuid_t uuid = { 0 };


    //sdp query and connection variables and structs
    int infoCounter = 0;
    char *uuid_str="00001101-0000-1000-8000-00805F9B34FB";
    uint32_t range = 0x0000ffff;
    sdp_list_t *response_list = NULL;
    sdp_list_t *search_list;
    sdp_list_t *attrid_list;
    int s, loco_channel = -1, status;
    struct sockaddr_rc loc_addr = { 0 };
    bdaddr_t tmpAddr;
    int retries = 0;
    int found = 0;
    int responses = 0;
    sdp_session_t *session;


    (void) signal(SIGINT, SIG_DFL);

    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        perror("No Bluetooth Adapter Available");
        return -1;
    }

    if (hci_devinfo(dev_id, &dev_info) < 0) {
        perror("Can't get device info");
        return -1;
    }



    sock = hci_open_dev( dev_id );
    if (sock < 0) {
        perror("HCI device open failed");
        free(info);
        return -1;
    }

    if( !str2uuid( uuid_str, &uuid ) ) {
        perror("Invalid UUID");
        free(info);
        return -1;
  	}

  	while(!foundName) {

  		printf("Scanning ...\n");
        info = NULL;
        num_rsp = 0;
        flags = 0;
        length = 8; /* ~10 seconds */
        num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
        if (num_rsp < 0) {
            perror("Inquiry failed");
            exit(1);
        }

        //check info structs returned by hci_inquiry
        for (int i = 0; i < num_rsp; i++) {


        	ba2str(&(info+i)->bdaddr, addr);
            memset(name, 0, sizeof(name));

            if (hci_read_remote_name(sock, &(info+i)->bdaddr, sizeof(name), name, 0) < 0) {
            	strcpy(name, "[unknown]");
        	}

            printf("Found %s  %s\n", addr, name);

            if (strcmp(name, "Yusef's G5") == 0) {
            	printf("Success\n");
            	infoCounter = i;
            	foundName = 1;
            	strcpy(targetName, name);
            	strcpy(targetAddr, addr);
            }
        }
  	}

  	//attempt to connect to the app's sdp service
  	session = 0;
  	responses = 0;
    while(!session) {

        session = sdp_connect( BDADDR_ANY, &(info+infoCounter)->bdaddr, SDP_RETRY_IF_BUSY );

        if(session) {
        	printf("Connected to sdp\n");
    		break;
		}

        if(errno == EALREADY && retries < 5) {
            perror("Retrying");
            retries++;
            sleep(1);
            continue;
        }
        break;
    }

    //call error if session is null
    if ( session == NULL ) {
        perror("Can't open session with the device");
        free(info);
    }

    //set up further sdp search variables
    search_list = sdp_list_append( 0, &uuid );
    attrid_list = sdp_list_append( 0, &range );
    err = 0;
    err = sdp_service_search_attr_req( session, search_list, SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
    sdp_list_t *r = response_list;
    sdp_record_t *rec;

    //check sdp service records
    for (; r; r = r->next ) {
        responses++;
        rec = (sdp_record_t*) r->data;
        sdp_list_t *proto_list;
        
        // get a list of the protocol sequences
        if( sdp_get_access_protos( rec, &proto_list ) == 0 ) {
        sdp_list_t *p = proto_list;

            // go through each protocol sequence
            for( ; p ; p = p->next ) {
                    sdp_list_t *pds = (sdp_list_t*)p->data;

                    // go through each protocol list of the protocol sequence
                    for( ; pds ; pds = pds->next ) {

                            // check the protocol attributes
                            sdp_data_t *d = (sdp_data_t*)pds->data;
                            int proto = 0;
                            for( ; d; d = d->next ) {
                                    switch( d->dtd ) { 
                                            case SDP_UUID16:
                                            case SDP_UUID32:
                                            case SDP_UUID128:
                                                    proto = sdp_uuid_to_proto( &d->val.uuid );
                                                    break;
                                            case SDP_UINT8:
                                                    if( proto == RFCOMM_UUID ) {
                                                            printf("rfcomm channel: %d\n",d->val.int8);
                                                            loco_channel = d->val.int8;
                                                            found = 1;
                                                    }
                                                    break;
                                    }
                            }
                    }
                    sdp_list_free( (sdp_list_t*)p->data, 0 );
            }
            sdp_list_free( proto_list, 0 );

        }
        if (loco_channel > 0)
            break;

	}
	int input = -1;
	char send = '1';
    printf("No of Responses %d\n", responses);
    if ( loco_channel > 0 && found == 1 ) {
        printf("Found service on this device, now gonna blast it with dummy data\n");
        s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        loc_addr.rc_family = AF_BLUETOOTH;
        loc_addr.rc_channel = loco_channel;
        loc_addr.rc_bdaddr = *(&(info+infoCounter)->bdaddr);
        status = connect(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
        if( status < 0 ) {
            perror("uh oh");
        }
        do {
			scanf ("%d", &input); 
			input+=48;
			send = input;
			printf("Wrote %c\n", input);
            status = write(s, &send, 1);
            printf ("Wrote %d bytes\n", status);
            sleep(1);
        } while (status > 0);
        close(s);
        sdp_record_free( rec );
    }

    sdp_close(session);
    if (loco_channel > 0) {
        //goto sdpconnect;
        //break;
    }
}