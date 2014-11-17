#ifndef PWD_RS232_H 
#define PWD_RS232_H 
 
/*--------------------------------------------------------------------------- 
                Copyright (C) 2009 Philips Medical Systems 
                All Rights Reserved.  Proprietary and Confidential. 
 
                File:           pwmrs232.h   
                
                Description:    Definition for RS232 interface object
 
--------------------------------------------------------------------------*/ 
 
//------------------------------------------------------------------------- 
// Compiler Specific Includes                                               
//------------------------------------------------------------------------- 
#include <termios.h>

//------------------------------------------------------------------------- 
// Philips Specific Includes                                                 
//------------------------------------------------------------------------- 
#include "pwmdef.h"
//--------------------------------------------------------------------------- 
//                              Defines 
//--------------------------------------------------------------------------- 
 
//--------------------------------------------------------------------------- 
//                              Typedefs / Enums 
//--------------------------------------------------------------------------- 
typedef enum
{
    PWM_RS232_BAUD_1200     = B1200,
    PWM_RS232_BAUD_2400     = B2400,
    PWM_RS232_BAUD_4800     = B4800,
    PWM_RS232_BAUD_9600     = B9600,
    PWM_RS232_BAUD_19200    = B19200,
    PWM_RS232_BAUD_38400    = B38400,
    PWM_RS232_BAUD_57600    = B57600,
    PWM_RS232_BAUD_115200   = B115200
} PWM_RS232_BAUD;

typedef enum
{
    PWM_RS232_DATABIT_7     = CS7,
    PWM_RS232_DATABIT_8     = CS8
} PWM_RS232_DATABIT;

typedef enum
{
    PWM_RS232_PARITY_NONE   = 0,
    PWM_RS232_PARITY_ODD    = PARODD | PARENB,
    PWM_RS232_PARITY_EVEN   = PARENB
} PWM_RS232_PARITY;

typedef enum
{
    PWM_RS232_STOPBIT_1     = 0,
    PWM_RS232_STOPBIT_2     = CSTOPB
} PWM_RS232_STOPBIT;

typedef struct PwmRs232ConfigStruct
{
    u8                  nPort;  // 1 = ttyS1, 2 = ttyS2
    PWM_RS232_BAUD      baud;
    PWM_RS232_DATABIT   data;
    PWM_RS232_PARITY    parity;
    PWM_RS232_STOPBIT   stop;
    boolean             bNonBlock;
} PWM_RS232_CONFIG, *PWM_RS232_CONFIG_PTR;

//--------------------------------------------------------------------------- 
//                              Class Definition 
//--------------------------------------------------------------------------- 
 
class CPwmRs232 
{ 
public: 
 
    CPwmRs232();
    ~CPwmRs232();
    
    boolean Start( PWM_RS232_CONFIG_PTR pConfig );
    boolean Stop( void );
    
    u32 Read( u8 *pOutBuf, u32 nBufSize );
    u32 Write( u8 *pInBuf, u32 nBufSize );

    void ChangeBaudRate(PWM_RS232_BAUD baudRate);

protected: 
 
private: 
        HANDLE m_hPort;
        struct termios m_oldtio;
        struct termios m_newtio;
	PWM_RS232_CONFIG mConfig;
};   
//--------------------------------------------------------------------------- 
//                              C/C++ Functions Prototypes 
//--------------------------------------------------------------------------- 
#ifdef __cplusplus 
extern "C" { 
#endif 
 
 
#ifdef __cplusplus 
} 
#endif 
 
#endif		// End PWD_RS232_H 
 
