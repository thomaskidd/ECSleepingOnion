#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

int main(int argc, char **argv) {

    //scan connecttion structs and variables
    inquiry_info *info = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char address[19] = { 0 };
    char name[248] = { 0 };

    //variables and structs from connecting to selected bluetooth device
    struct sockaddr_rc addr = { 0 };
    int s, status;
    char dest[18] = "1234";


    //get socket for scanning for bluetooth devices
    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    //scan for 1.28 * len seconds
    len  = 8;
    max_rsp = 255; //find max of 255 bluetooth devices
    flags = IREQ_CACHE_FLUSH;
    info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &info, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    //testing
    printf("num_rsp: ");
    printf("%d\n",num_rsp);

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(info+i)->bdaddr, address);
        memset(name, 0, sizeof(name));

        if (hci_read_remote_name(sock, &(info+i)->bdaddr, sizeof(name), name, 0) < 0) {
            strcpy(name, "[unknown]");
        }

        //check if this is the desired device to connect to
                int test = strcmp(name, "Xperia M4 Aqua");
        printf("test: %d\n", test);
        if (test > -3 && test < 3) {
            printf("Confirmed.\n");
            ba2str(&(info+i)->bdaddr, dest);
        }

        printf("%s  [%s]\n", address, name);
    }

    free( info );
    close( sock );


    //connect to selected device

    if (strcmp(dest,"1234") == 0) {
        printf("Copying error\n");
        return 0;
    }

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );
    
    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    printf("Got past connect\n");
    // send a message
    if( status == 0 ) {
        status = write(s, "hello!", 6);
    }

    if( status < 0 ) {
        perror("uh oh\n");
        return 0;
    }



    //main loop for testing
    int done = 0;
    int input = -1;

    while (!done) {
        int input = getchar();

        if (input == 1) {
            status = write(s, "1", 1);
        }
        if (input == 0) {
            status = write(s, "0", 1);
        }

        if (input == 2) {
            done = 1;
        }

    }

    close(s);

    return 0;
}

