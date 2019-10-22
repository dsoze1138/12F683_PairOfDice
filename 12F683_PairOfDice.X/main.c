/*
 * file: main.c
 * target: PIC12F683
 * IDE: MPLABX v4.05
 * compiler: XC8 v1.45 (free mode)
 *
 * Description:
 *
 *  This application uses Charlieplexing to turn on LEDs attached
 *  to GPIO pins GP1, GP2, GP4 & GP5 in all possible combinations.
 *
 *  The LEDs are arranged to show the faces of a pair of dice:
 *
 *      (D1.2a)        (D1.3b)    (D2.2a)        (D2.3b)
 *
 *      (D1.4a) (D1.1) (D1.4b)    (D2.4a) (D2.1) (D2.4b)
 *
 *      (D1.3a)        (D1.2b)    (D2.3a)        (D2.2b)
 *
 *
 *                       PIC12F683
 *              +-----------:_:-----------+
 *       5v0 -> : 1 VDD             VSS 8 : <- GND
 *      DRV5 <> : 2 GP5         PGD/GP0 7 : <> ICD_PGD/SW1
 *      DRV4 <> : 3 GP4         PGC/GP1 6 : <> ICD_PGC/DRV1
 *  ICD_MCLR -> : 4 GP3/MCLR        GP2 5 : <> DRV2
 *              +-------------------------+
 *                         DIP-8
 *
 *           100 OHM
 *  DRV1 ----/\/\/\-------+-----------+
 *                        :           :
 *                       ---         ---
 *                 D1.1  / \         \ / D2.1
 *                       ---         ---
 *            75 OHM      :           :
 *  DRV2 ----/\/\/\-------+-----------+-------------+-----------+
 *                        :           :             :           :
 *                       ---         ---            :           :
 *                 D1.2a / \         \ / D1.3a      :           :
 *                       ---         ---           ---         ---
 *                        :           :       D2.3a/ \         \ / D2.4a
 *                       ---         ---           ---         ---
 *                 D1.2b / \         \ / D1.3b      :           :
 *                       ---         ---            :           :
 *            75 OHM      :           :             :           :
 *  DRV4 ----/\/\/\-------+-----------+             :           :
 *                        :           :             :           :
 *                       ---         ---            :           :
 *                 D1.4a / \         \ / D2.2a      :           :
 *                       ---         ---           ---         ---
 *                        :           :       D2.3b/ \         \ / D2.4b
 *                       ---         ---           ---         ---
 *                 D1.4b / \         \ / D2.2b      :           :
 *                       ---         ---            :           :
 *            75 OHM      :           :             :           :
 *  DRV5 ----/\/\/\-------+-----------+-------------+-----------+
 *
 *                   ____    470 OHM
 *  SW1  -------+----o  o----/\/\/\-------VDD(5v0)
 *              :   BUTTON
 *              \
 *              /
 *              \ 10K OHM
 *              /
 *              \
 *              :
 *              +------------------------VSS(GND)
 *
 * Notes:
 *  Charlieplexing, see https://en.wikipedia.org/wiki/Charlieplexing
 */
    
/*
 * PIC12F683 specific configuration words
 */
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select bit (MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Detect (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
/*
 * Include PIC12F683 specific symbols
 */
#include <xc.h>
/*
 * Application specific constants
 */
#define FSYS (4000000ul)                    /* syetem oscillator frequency in Hz */
#define FCYC (FSYS/4ul)                     /* number of inctruction clocks in one second */
#define TIMER0_COUNTS_UNTIL_ASSERT (128ul)
#define TIMER0_PRESCALE (64ul)
#define MAX_LED_STATES (8)
#define TIMER0_ASSERTS_IN_ONE_SECOND (2ul)
#define POR_DELAY (FCYC/(TIMER0_ASSERTS_IN_ONE_SECOND * TIMER0_COUNTS_UNTIL_ASSERT * TIMER0_PRESCALE))
/*
 * Check that the Power On Reset delay is in range for this implementation.
 */
#if (POR_DELAY >= 256)
#undef POR_DELAY
#define POR_DELAY (255)
#warning POR Delay too long for this implememnattion
#elif (POR_DELAY < 1)
#undef POR_DELAY
#define POR_DELAY (1)
#warning POR Delay too short for this implememnattion
#endif
    
/*
 * Global data
 */
unsigned char gLEDs;
unsigned char gTRISIO;
unsigned char gTMR0_MSB;
unsigned char gPause;
/*
 * LED_refresh
 *
 * This function must be called regularly from the application loop.
 *
 * We use 1 of 8 timing to multiplex (charlieplex) and drive eight LEDs.
 *
 * This function needs to be called often enough so all the LED
 * can appear to be on at the same time.
 *
 * Notes:
 *  For base line PICs like the PIC12F508 the TRISIO register
 *  cannot be read. To deal with this a shadow register, gTRISIO,
 *  is used in this implementation to remember the state of
 *  the GPIO in or out setting.
 *
 */
void LED_refresh(void)
{
    static unsigned char State = MAX_LED_STATES;
    
    unsigned char OutBits, HighBits;
    
    CLRWDT();
    OutBits  =  0b00000000;
    HighBits =  0b00000000;
    
    switch (--State)
    {
    case 7:
        if (gLEDs & 0x80)
        {
            HighBits |= (1 << 5); /* Drive D2.4, GP5=H GP2=L */
            OutBits = ~((1<<5)|(1<<2));
        }
        break;
        
    case 6:
        if (gLEDs & 0x40)
        {
            HighBits |= (1 << 2); /* Drive D2.3, GP5=L GP2=H */
            OutBits = ~((1<<5)|(1<<2));
        }
        break;
        
    case 5:
        if (gLEDs & 0x20)
        {
            HighBits |= (1 << 4); /* Drive D2.2, GP5=L GP4=H */
            OutBits = ~((1<<5)|(1<<4));
        }
        break;
        
    case 4:
        if (gLEDs & 0x10)
        {
            HighBits |= (1 << 1); /* Drive D2.1, GP1=H GP2=1 */
            OutBits = ~((1<<1)|(1<<2));
        }
        break;
        
    case 3:
        if (gLEDs & 0x08)
        {
            HighBits |= (1 << 5); /* Drive D1.4, GP5=H GP4=L */
            OutBits = ~((1<<5)|(1<<4));
        }
        break;
        
    case 2:
        if (gLEDs & 0x04)
        {
            HighBits |= (1 << 2); /* Drive D1.3, GP4=L GP2=H */
            OutBits = ~((1<<4)|(1<<2));
        }
        break;
        
    case 1:
        if (gLEDs & 0x02)
        {
            HighBits |= (1 << 4); /* Drive D1.2, GP4=H GP2=L */
            OutBits = ~((1<<4)|(1<<2));
        }
        break;
        
    default:
        if (gLEDs & 0x01)
        {
            HighBits |= (1 << 2); /* Drive D1.1, GP1=L GP2=H */
            OutBits = ~((1<<1)|(1<<2));
        }
        State = MAX_LED_STATES;
    }
    
    gTRISIO |= ((1<<5)|(1<<4)|(1<<2)|(1<<1)); /* Turn off all LED output drivers */
    TRISIO   = gTRISIO;
    if (OutBits)
    {
        GPIO    &= OutBits;      /* Set both LED drivers to low */
        gTRISIO &= OutBits;      /* Turn on LED output drivers */
        TRISIO   = gTRISIO;
        GPIO    |= HighBits;     /* Turn on just one of the two LEDs  */
    }
}
/*
 * Initialize PIC hardware for this application
 */
void PIC_Init( void )
{
    INTCON = 0; /* Disable interrupts */
    OPTION_REG = 0b11010101; /* TIMER0 prescale 1:64, clock source FCYC */
    /*
     * Wait for about 1/2 second after reset to
     * give MPLAB/ICD time to connect.
     */
    gTMR0_MSB = TMR0;
    for( gPause = POR_DELAY; gPause != 0; )
    {
        CLRWDT();
        if((TMR0 ^ gTMR0_MSB ) & 0x80)
        {
            gTMR0_MSB = TMR0;
            gPause -= 1;
        }
    }
    /*
     * PIC12F683 specific initialization
     */
    ANSEL   = 0;
    CMCON0  = 0x07;
    GPIO    = 0;
    gTRISIO = 0b11111111;
    TRISIO  = gTRISIO;
}
/*
 * Application
 */
#define LED_STEP_DELAY (64u)
void main(void)
{
    PIC_Init();
    /*
     * Process loop
     */
    gTMR0_MSB = TMR0;
    gPause = LED_STEP_DELAY;
    for(;;)
    {
        LED_refresh();
        if((TMR0 ^ gTMR0_MSB ) & 0x80)
        {
            gTMR0_MSB = TMR0;
            if(--gPause == 0)
            {
                gLEDs <<= 1;
                if(!gLEDs)
                {
                    gLEDs = 1;
                }
                gPause = LED_STEP_DELAY;
            }
        }
    }
}
