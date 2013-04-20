// https://code.google.com/p/sl500-linux-api/source/browse/trunk/src/sl500.c?r=6

#define HEADER_FIRST  0xAA
#define HEADER_SECOND 0xBB

#define TIMEOUT 1 //seconds

#define CMD_ERROR        -3
#define READ_ERROR       -2
#define CHECKSUM_ERROR   -1


#define CMD_SET_BAUDRATE     0x0101
#define CMD_SET_NODE_NUMBER  0x0102
#define CMD_READ_NODE_NUMBER 0x0103
#define CMD_READ_FW_VERSION  0x0104
#define CMD_BEEP             0x0106
#define CMD_LED              0x0107
#define CMD_WORKING_STATUS   0x0108  // #not used?         # data = 0x41

#define CMD_ANTENNA_POWER    0x010C
#define CMD_RFU              0x0108

#define CMD_MIFARE_REQUEST   0x0201
                            // #  request a type of card
                            // 0x52: request all Type A card In field,
                            // 0x26: request idle card


#define TYPE_MIFARE_UL        0x0044
#define TYPE_MIFARE_1K        0x0004
#define TYPE_MIFARE_4K        0x0002
#define TYPE_MIFARE_DESFIRE   0x0344
#define TYPE_MIFARE_PRO       0x0008


#define CMD_MIFARE_ANTICOLISION  0x0202 // 0x04 -> <NUL> (00)      [4cd90080]-cardnumber
#define CMD_MIFARE_SELECT        0x0203 //#  [4cd90080]  -> 0008
#define CMD_MIFARE_HALT          0x0204

#define CMD_MIFARE_AUTH2         0x0207
//# 60 [sector*4] [key]
//#Auth_mode:  Authenticate mode, 0x60: KEY A, 0x61: KEY B

#define CMD_MIFARE_READ_BLOCK   0x0208 // # [block_number]
#define CMD_MIFARE_WRITE_BLOCK  0x0209 // # [block_number] [*data - 16B]

#define CMD_MIFARE_UL_SELECT   0x0212

#define LED_RED   1
#define LED_GREEN 2
#define LED_BOTH  3



#define hibyte(x)       ( (unsigned char)(x>>8) )
#define lobyte(x)       ( (unsigned char)(x & 0xFF) )

