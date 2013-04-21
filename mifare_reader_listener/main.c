#include "mifare.h"
#include <sys/time.h>

#define RETRY_TIMER 5


unsigned long timestamp_sec() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec;
}

int main(int argc, char * argv[]) {


    int fd = 0;

    unsigned char response_buffer[255];
    int count_recieved;
    unsigned char delay = 100;
    verbose = 0;
    unsigned short cardtype;

    char *devicename = "/dev/ttyUSB0";
    char server_send_buffer[1024] = "";
    char server_recv_buffer[1024] = "";

    logfd = stdout;

    int argi;
    for (argi = 1; argi < argc; argi++) {
        if (strcmp("-v", argv[argi]) == 0 ) {
            verbose = 1;
        } else if (argv[argi][0] != '-') {
            devicename = argv[argi];
        }
    }

    fd = open(devicename, O_RDWR | O_NOCTTY    );
    int set_bits = 4;
    ioctl(fd, TIOCMSET, &set_bits);

    initport(fd);
    fprintf(logfd, "started ...\n");
    beep(fd, 10);
    set_led(fd, LED_GREEN);


    unsigned char version[16];
    count_recieved = send_recieve(fd, CMD_READ_FW_VERSION, 0, 0, version);
    fprintf(logfd, "version: %.*s\n", count_recieved, version);


    unsigned char serial[16];
    char matrix_serial[64];
    unsigned char prev_serial[16] = "";
    unsigned char prev_serial_len = 0;
    unsigned char prev_cardtype = 0;
    unsigned char prev_card[256];
    unsigned long prev_timestamp = 0;

    unsigned char card_status = 0, usb_status = 0;

    char slen, rlen;
    unsigned char ibuffer[1024], obuffer[1024], *in = ibuffer, *out;

    while (1) {
     count_recieved = mifare_select(fd, &cardtype, serial);

     if( access("/dev/sda1", R_OK) == 0){
       if( usb_status == 0 ){
          beep(fd, 5); beep(fd, 5); beep(fd, 5);
          usb_status = 1;
       }
     } else usb_status = 0;

     if (count_recieved > 0 && card_status == 0) {
       if ( (  memcmp(prev_serial, serial, count_recieved ) != 0 )  || (prev_serial_len != count_recieved) || (prev_cardtype != cardtype) ) {

         serial_to_matrix_iii(cardtype, serial, count_recieved, matrix_serial);

         // wait 5 seconds if it's the same card serial
         if( strcmp( prev_card, matrix_serial ) == 0 && timestamp_sec() - prev_timestamp < RETRY_TIMER ) {
           usleep( 1000 * 100 );
           continue;
         }
         set_led(fd, LED_RED);


         strcpy(prev_card, matrix_serial);

         fprintf(logfd, "\nserial:%s\n",matrix_serial);



         if( access("/dev/sda1", R_OK) == 0 ){
           system("mount /dev/sda1 usb");

           if( access("usb/.grivler_token", R_OK) == 0 ){
             printf(" STORE TOKEN TO CARD \n ");
             FILE *f = fopen("usb/.grivler_token", "r");
             fseek (f , 0 , SEEK_END);
             unsigned int size = ftell(f);
             rewind(f);
             memset( ibuffer, 0x00, 32 );
             if( size == fread(ibuffer, 1, size, f) ){
               printf(" TOKEN [%d] %s \n", size, ibuffer);
               ibuffer[size-1] = 0x00;
               int err = write_sector(fd, 8, 0x60, 0, ibuffer);
               fprintf(logfd, "write code [%d]\n", err);
               if( err >= 0 )
                 system("rm usb/.grivler_token");
             }
             fclose(f);
           }
           {
             printf(" SYNC USING CARD DATA \n ");
             int size = read_sector(fd, 8, 0x60, 0 , obuffer);
             if( size == 32 ){
               sprintf( ibuffer,
                 "rsync -avzru -e ssh theshark@192.168.0.5:/home/theshark/sync/%s/ ./usb ; "
                 "rsync -avzru -e ssh ./usb theshark@192.168.0.5:/home/theshark/sync/%s/", obuffer, obuffer);

               printf("%s", ibuffer);
//               system(ibuffer);
             }

             // save run sync script with folder = key
           }
           system("umount usb");
           prev_timestamp = timestamp_sec();
         }
         else
           printf(" NO USB DEVICE \n");

         card_status = 1;
       } else {
       //~ printf("select error\n");
           prev_cardtype = 0;
       }
     } else {
       if( card_status == 1 ) {
         card_status = 0;
         beep(fd, 5);
         usleep( 1000 * 500 );
       }
     }

     set_led(fd, LED_GREEN);

   }

}
