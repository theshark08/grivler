#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/termios.h> /* POSIX terminal control definitions */
#include <sys/socket.h> /* POSIX terminal control definitions */
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "defines.h"

FILE * logfd;
int verbose;


int readport(int fd, char *result) {
        int iIn = read(fd, result, 1024);
        result[iIn-1] = 0x00;
        if (iIn < 0) {
                if (errno == EAGAIN) {
                        fprintf(logfd, "SERIAL EAGAIN ERROR\n");
                        return 0;
                } else {
                        fprintf(logfd, "SERIAL read error %d %s\n", errno, strerror(errno));
                        return 0;
                }
        }
        return 1;
}

int initport(int fd) {
        struct termios options;
        // Get the current options for the port...
        tcgetattr(fd, &options);
        // Set the baud rates to 19200...
        cfsetispeed(&options, B115200);
        //cfsetospeed(&options, B9600);
        // Enable the receiver and set local mode...
        options.c_cflag |= (CLOCAL | CREAD);

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        // Set the new options for the port...
        tcsetattr(fd, TCSANOW, &options);

    fcntl(fd, F_SETFL, O_SYNC); //blocking read
        return 1;
}


int do_write(int fd, const void * ptr, int n) {
    if (verbose) {
        fprintf(logfd, "write %d bytes to %d: ", n, fd);
        int i;
        for (i = 0; i < n; i++) {
            fprintf(logfd, "%.2x ",((unsigned char *)ptr)[i]);
        }
        fprintf(logfd, "\n");
    }
    return write(fd, ptr, n);
}

int synchronous_read(int fd, void * ptr, int n, int timeout_seconds  ) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    int n_read = 0;
    int retval;

    struct timeval timeout;

    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;

    while (n_read <  n) {
        retval = select(fd + 1,&rfds, NULL, NULL, &timeout);
        if (retval) {
            if (FD_ISSET(fd,&rfds)) {
                int this_read = read(fd, ptr + n_read, n - n_read);
                n_read+=  this_read;
                if (this_read == 0) {
                    break;
                }
            }
        } else {
            return -1;
        }
    }

    if (verbose) {
        fprintf(logfd, "read %d/%d bytes from %d: ", n_read, n, fd);

        int i;
        for (i = 0; i < n_read; i++) {
            fprintf(logfd, "%x ",((unsigned char *)ptr)[i]);
        }
        fprintf(logfd, "\n");
    }

    return n_read;
}

int send_command(int fd, unsigned short command, unsigned char * data, int data_len) {
    unsigned short length = 2 + 2 + 1 + data_len;
    unsigned char header0 = HEADER_FIRST;
    unsigned char header1 = HEADER_SECOND;
    unsigned char zero = 0;

    unsigned char checksum = 0x00;
    unsigned char * payload;

    do_write(fd, &header0, 1);
    do_write(fd, &header1, 1);

    do_write(fd, &length, 2);

    do_write(fd, &zero, 1);
    do_write(fd, &zero, 1);
    checksum ^= 0;

    do_write(fd, &command, 2);
    checksum ^= hibyte(command);
    checksum ^= lobyte(command);

    int i ;
    for (i = 0; i < data_len; i++) {
        do_write(fd, data + i, 1);
        checksum ^= data[i];
        if (data[i] == 0xAA) {
            do_write(fd, &zero, 1);
        }
    }

    do_write(fd, &checksum, 1);
    fdatasync(fd);
}

// returns count bytes recieved, -1 otherwise
int recieve_data(int fd, unsigned short expected_cmd, unsigned char * status, char * data) {
    unsigned char  prev_byte = 0;
    unsigned char  current_byte = 0;
    unsigned short cmd;

    while ( 1 ) { //expected_cmd != cmd
        while ( 1 ) {
            if (synchronous_read(fd, &current_byte, 1, TIMEOUT) >= 0)  {
                if (( prev_byte  == HEADER_FIRST ) && ( current_byte  == HEADER_SECOND )) {
                    // header found, stop
                    if (verbose) fprintf(logfd, "header found stop\n");
                    break;
                }
                prev_byte = current_byte;
            } else {
                fprintf(logfd, "failed to get header!\n");

                return -2;
            }
        }

        unsigned short length;
        unsigned short reserved;

        unsigned char checksum = 0;
        unsigned char original_checksum;

        if ( synchronous_read(fd, &length, 2, TIMEOUT) < 1) return -2;
        if (verbose) fprintf(logfd, "length: %d\n", length);

        if (length == 0x000a && expected_cmd == 0x0212) {
            length = 0x0d;
        }

        if ( synchronous_read(fd, &reserved, 2, TIMEOUT) < 1) return -2;
        checksum ^= hibyte(reserved);
        checksum ^= lobyte(reserved);

        if ( synchronous_read(fd, &cmd, 2, TIMEOUT) < 1) return -2;

        if (cmd != expected_cmd)  continue ;

        checksum ^= lobyte(cmd);     checksum ^= hibyte(cmd);

        if ( synchronous_read(fd, status, 1, TIMEOUT) < 1) return -2;

        checksum ^= *status;

        int i;
        //reading data

        if ( synchronous_read(fd, data, length - 6, TIMEOUT) < length - 6) return -2;

        for (i=0; i < length -6; i++  ) {
            checksum ^= data[i];
        }

        synchronous_read(fd, &original_checksum, 1, TIMEOUT);

        if (original_checksum == checksum || 1) {
            return length - 6;
        } else {
            if (*status == 0) {
                fprintf(logfd, "checksum %d should be %d\n", original_checksum, checksum);
            }
            return -1;
        }
    }
}
// returns count bytes recieved, -1 otherwise, -2 if error is occured
int send_recieve(int fd, unsigned short command, unsigned char * request, int request_len, unsigned char * response) {

    send_command(fd, command, request, request_len);

    unsigned char status;
    int count_recieved = recieve_data(fd, command, &status, response);

    if (count_recieved > -1) {
        if (status != 0 ) {
            return -3;
        }
    }

    return count_recieved;
}

// returns cardtype, fill serial
int mifare_select(int fd,unsigned short * cardtype, unsigned char * serial) {
    unsigned char buffer;
    unsigned char response_buffer[16];
    buffer = 0x52;
    int serial_length;

    int bytes_recieved = send_recieve(fd, CMD_MIFARE_REQUEST, &buffer, 1, (unsigned char * ) cardtype);
    if (bytes_recieved > 0) {
        int serial_length = send_recieve(fd, CMD_MIFARE_ANTICOLISION, 0, 0, serial);
        if (serial_length > 0) {
            if (verbose) fprintf(logfd, "cardtype: %d\n", *cardtype);
            if (*cardtype == TYPE_MIFARE_UL) {
                if (verbose) fprintf(logfd,"TYPE_UL!\n");

                //fprintf(logfd, "try...\n");

                serial_length = send_recieve(fd, CMD_MIFARE_UL_SELECT, &buffer, 0, serial);
             } else {
                int bytes_recieved2 = send_recieve(fd, CMD_MIFARE_SELECT, serial, serial_length, response_buffer);
            }
        }
        return  serial_length;
    } else {
        return bytes_recieved;
    }
}

char * tohex(unsigned char * buffer, int length) {
    int i;
    char * result = (char *) malloc ( length * 2  + 1);
    for (i = 0; i < length; i++) {
        sprintf(result + i * 2, "%.2X", buffer[i]);
    }
    return result;
}
int serial_to_matrix_iii(unsigned short cardtype, unsigned char * serial, unsigned char serial_length, char * output) {
    char * hex;
    char postfix[4] = "XX";

    if (cardtype == TYPE_MIFARE_UL) {
        char * left = tohex(serial, serial_length / 2);
        char * right = tohex(serial+ serial_length / 2 , serial_length /2 + 1);

        right = realloc(right, strlen(left) + strlen(right) + 1);
        strcat(right, left);
        hex = right;
        strcpy(postfix,"UL");

    } else {
        hex =  tohex(serial, serial_length);

        if (cardtype == TYPE_MIFARE_1K) {
            strcpy(postfix, "1K");
        } else if (cardtype == TYPE_MIFARE_4K) {
            strcpy(postfix, "3K");
        } else if (cardtype == TYPE_MIFARE_DESFIRE) {
            strcpy(postfix,"DF");
        } else if (cardtype == TYPE_MIFARE_PRO) {
            strcpy(postfix,"PR");
        }
    }

    strcpy(output,"Mifare[");
    strcat(output, hex);
    strcat(output, "] ");
    strcat(output, postfix);
    strcat(output, " X");

    return 0;
}

void set_led(int fd, unsigned char led) {
    unsigned char response_buffer[255];
    unsigned char arg = 0;
    if (led != 0 ) {
        send_recieve(fd, CMD_LED , &arg, 1, response_buffer); //reset
    }
    arg = led;
    send_recieve(fd, CMD_LED , &arg, 1, response_buffer); //set
}

void beep(int fd, unsigned char delay) {
    unsigned char response_buffer[255];
    unsigned char arg = delay;
    send_recieve(fd, CMD_BEEP, &arg, 1, response_buffer); //reset
}

unsigned char auth(int fd, unsigned char type, unsigned char sector_id, const char* key) {
  unsigned char ibuff[8], obuff[1];
  memset( ibuff,   type, 1 );
  memset( ibuff+1, sector_id*4, 1 );
  memset( ibuff+2, 0xFF, 6 );

  if(key) memcpy(ibuff+2, key, 6);

  int rlen = send_recieve(fd, CMD_MIFARE_AUTH2, ibuff, 8, obuff);

  return rlen == 0;
}

int read_sector(int fd, unsigned char sector_id, unsigned char key_type, const char* key, char* data) {

  if( !auth(fd, key_type, sector_id, key) ) return -1;

  unsigned char block = sector_id*4;
  int rlen = 0, size, l;

  for( l = 0 ; l < 2 + 2 * (key_type == 0x61); l ++, block ++ ){
    size = send_recieve(fd, CMD_MIFARE_READ_BLOCK, &block, 1, data+rlen);
    if( size < 0 ){
      printf( " block %d \n", block);
      return -2;
    }
    rlen += size;
  }

  return rlen;
}

int write_sector(int fd, unsigned char sector_id, unsigned char key_type, const char* key, char* data) {

  if( !auth(fd, key_type, sector_id, key) ) return -1;

  unsigned char block = sector_id*4;
  int rlen = 0, size, l;
  char obuffer[16];
  char buffer[17];


  for( l = 0 ; l < 2 + 1 * (key_type == 0x61); l ++, block ++ ){
    buffer[0] = sector_id*4 + l;
    memcpy(buffer+1, data + 16*l, 16);
    size = send_recieve(fd, CMD_MIFARE_WRITE_BLOCK, buffer, 17, obuffer);
    if( size < 0 ) return -1;
  }

  return 0;
}




int dump(int fd, unsigned char key_type, const char* keys, char* data) {
  int sector, ret = 0;
  for( sector = 0 ; sector < 16 ; sector++ ){
    int r = 0;
    if( keys )
       r = read_sector(fd, sector, key_type, keys + sector * 6, data + ret);
    else
       r = read_sector(fd, sector, key_type, 0, data+ret);
    if( r > 0 ) ret += r;
  }
  return ret;
}
