
#undef	PHOTO_HACK

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>

#define	D1	10
#define	D2	11
#define	D3	12
#define	D4	13




static const uint8_t segmentDigits [] =
{
// a  b  c  d  e  f  g  p
// 0  1  2  3  4  5  6  7
   1, 1, 1, 1, 1, 1, 0, 0,	// 0
   0, 1, 1, 0, 0, 0, 0, 0,	// 1
   1, 1, 0, 1, 1, 0, 1, 0,	// 2
   1, 1, 1, 1, 0, 0, 1, 0,	// 3
   0, 1, 1, 0, 0, 1, 1, 0,	// 4
   1, 0, 1, 1, 0, 1, 1, 0,	// 5
   1, 0, 1, 1, 1, 1, 1, 0,	// 6
   1, 1, 1, 0, 0, 0, 0, 0,	// 7
   1, 1, 1, 1, 1, 1, 1, 0,	// 8
   1, 1, 1, 1, 0, 1, 1, 0,	// 9
   1, 1, 1, 0, 1, 1, 1, 0,	// A
   0, 0, 1, 1, 1, 1, 1, 0,	// b
   1, 0, 0, 1, 1, 1, 0, 0,	// C
   0, 1, 1, 1, 1, 0, 1, 0,	// d
   1, 0, 0, 1, 1, 1, 1, 0,	// E
   1, 0, 0, 0, 1, 1, 1, 0,	// F
   1, 0, 1, 1, 1, 1, 0, 0,      // G
   0, 1, 1, 0, 1, 1, 1, 0,      // H
   0, 0, 0, 0, 1, 1, 0, 0,      // I
   0, 1, 1, 1, 1, 0, 0, 0,      // J
   0, 0, 0, 1, 1, 0, 1, 0,      // K
   0, 0, 0, 1, 1, 1, 0, 0,      // L
   1, 1, 1, 0, 1, 1, 0, 0,      // M
   0, 0, 1, 0, 1, 0, 1, 0,      // N
   1, 1, 1, 1, 1, 1, 0, 0,      // O
   1, 1, 0, 0, 1, 1, 1, 0,      // P
   1, 1, 1, 0, 0, 1, 1, 0,      // Q
   0, 0, 0 ,0, 1, 0, 1, 0,      // R
   1, 0, 1, 1, 0, 1, 1, 0,      // S
   0, 0, 0, 1, 1, 1, 1, 0,      // T
   0, 1, 1, 1, 1, 1, 0, 0,      // U
   0, 0, 0, 0, 0, 0, 1, 0,      // V!!!
   0, 0, 0, 0, 0, 0, 1, 0,     	// W!!!
   0, 0, 0, 0, 0, 0, 1, 0,	// X!!!
   0, 1, 1, 0, 0, 1, 1, 0,	// Y
   1, 1, 0, 1, 1, 0, 1, 0,	// Z
   0, 0, 0, 0, 0, 0, 0, 0,	// blank
} ;
 


char digits [8] ;



void *displayDigits (void *dummy)
{
  uint8_t digit, segment ;
  uint8_t index, d, segVal ;

  for (;;)
  {
    for (digit = 0 ; digit < 4 ; ++digit)
    {
      digitalWrite (D1 + digit, 1) ;
      for (segment = 0 ; segment < 8 ; ++segment)
      {
	d = toupper (digits [digit]) ;
	 if ((d >= '0') && (d <= '9'))
	  index = d - '0' ;
	else if ((d >= 'A') && (d <= 'Z'))
	  index = d - 'A' + 10 ;
	else
	  index = 32 ;

	segVal = segmentDigits [index * 8 + segment] ;

	digitalWrite (segment, !segVal) ;
	delayMicroseconds (100) ;
	digitalWrite (segment, 1) ;
      }
      digitalWrite (D1 + digit, 0) ;
    }
  }
}



void setHighPri (void)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  sched.sched_priority = 10 ;
  if (sched_setscheduler (0, SCHED_RR, &sched))
    printf ("Warning: Unable to set high priority\n") ;
}


void setup (void)
{
  int i, c ;
  pthread_t displayThread ;


  if (wiringPiSetup () == -1)
    exit (1) ;

  setHighPri () ;



  for (i = 0 ; i < 8 ; ++i)
  {
    digitalWrite (i, 0) ;
    pinMode      (i, OUTPUT) ;
  }



  for (i = 10 ; i <= 13 ; ++i)
  {
    digitalWrite (i, 0) ;
    pinMode      (i, OUTPUT) ;
  }

  strcpy (digits, "    ") ;

  if (pthread_create (&displayThread, NULL, displayDigits, NULL) != 0)
  {
    printf ("thread create failed: %s\n", strerror (errno)) ;
    exit (1) ;
  }
  delay (10) ;




#ifdef PHOTO_HACK
  sprintf (digits, "%s", "1234") ;
  for (;;)
    delay (1000) ;
#endif

}



void show (char *message)
{
  int i ;

  for (i = 0 ; i < strlen (message) - 3 ; ++i)
  {
    strncpy (digits, &message [i], 4) ;
    delay (500) ;
  }
  delay (1000) ;
  for (i = 0 ; i < 3 ; ++i)
  {
  strcpy (digits, "    ") ;
    delay (150) ;
    strcpy (digits, "  ") ;
    delay (250) ;
    }
  delay (1000) ;
  strcpy (digits, "    ") ;
  delay (1000) ;
}


int main (int argc, char *argv[])
{

  if(argc == 1 ) return 0;

  struct tm *t ;
  time_t     tim ;

  setup    () ;
  if (strlen(argv[1]) <=4) {
    strcpy (digits,argv[1]) ;
    while(1)  delay (2000) ;
    strcpy (digits, "    ") ;
    }
//  if (strlen(argv[1] > 4)){
//	show(argv[1]);
//	}
  show (argv[1]) ;

  strcpy(digits, "");
  strcpy(digits, "");
    strcpy(digits, "");
  return 0 ;
}
