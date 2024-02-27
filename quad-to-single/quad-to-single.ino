/*  This sketch uses a Rotalink incremental encoder interface and Arduino relay 
    shields to make measurements of the quadrature edge to edge periods.
    

    Outline of operation
    ---------------------
    
    The sketch operates by enabling interrupts on the both the rising and 
    falling edges of the digital inputs to which the encoder quadrature inputs
    are connected.
    
    On each rising edge the pulse output pin state is toggled. This doubles the 
    the frequncy of the output pin compoared with either of the quadrature 
    input pulse trains.

    The final step is that a timer is triggered on each edge. When the timer 
    expires the pulse output pin is toggled.

    This produces a pulse on every input edge on either quadrature pin so 
    4 times the input frequency appears on the pulse output pin, although the 
    output pulse train is just that.wit a fixed pulse length.

    The maximum input frequcency is 1kHz, so the output maximum pulse frequency
    is 4kHz, i.e. 1 pulse every 250us. The pulse length is set to half this, i.e.
    125us.

    The arduino CPU clock rate is 16mHz, i.e so we have a period of the pulse feeding
    the timer is 62.5ns.
    
    The number of CPU clock counts we need for 125us is:
      
      Timeout count = 125us / 62.5ns  = 125,000 / 62.5 = 2,0000   
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <digitalWriteFast.h>

/* Definition of quadrature digital inputs */
#define DI_QUAD_A  2
#define DI_QUAD_B  3

/* The output for monitoring ISR activity */
#define PULSE_OUT 4

/* Definitions to work out the timer loading values */

/* The number of CPU clocks per micro-second. */
#define CPU_CLOCK_PER_US  16U

/* The output pulse length in micro-seconds. */
#define PULSE_LEN_US      50U

/* The number of clock counts that sets the output pulse width. */
#define PULSE_LEN_CPU_CLOCKS    (PULSE_LEN_US * CPU_CLOCK_PER_US)

/* We're using the 16 bit timer in overlflow, so we need to set it up so that 
   the starting count is such that it will overflow after PULSE_LEN_CPU_CLOCKS */
#define PULSE_LEN_TIMER_VAL     (0xFFFFU - PULSE_LEN_CPU_CLOCKS)

/* Forward function definitions */
static void QuadChange();


/* The bit rate at which the Arduino serial port runs */
#define SERIAL_BIT_RATE 115200

/* The number of possible state changes, this is just the 16 values a 4 bit
   nibble can take. */
#define NUM_POSSIBLE_QUAD_STATE_CHANGES     16U

/* Masks to pick out the old and new states of the quadrature input pins */
#define MSK_PREV_STATE  0x03U
#define MSK_NEW_STATE   0x0CU

/* Number of bits to right shft the new bits to the previous bits. */
#define RIGHT_SHIFT_NEW_TO_PREV 2


static uint8_t QuadStatePrevious = 0U;
static uint8_t QuadState = 0U; 
static int8_t NewMovement = 0;
static int8_t PrevMovement = 0;

/* The port on which the qadraute inputs are received. */
#define QUAD_PORT PIND


    /* The look-up table for the movement associated with a change in state of
       the quadrature inputs. */
static const int8_t QuadLookup[NUM_POSSIBLE_QUAD_STATE_CHANGES] =
{
/* Aprev    Bprev   Acurr   Bcurr   Nibble(hex)     Position change */
/*  0       0       0       0       0   */          0,
/*  0       0       0       1       1   */          -1,
/*  0       0       1       0       2   */          1,
/*  0       0       1       1       3   */          0,
/*  0       1       0       0       4   */          1,
/*  0       1       0       1       5   */          0,
/*  0       1       1       0       6   */          0,
/*  0       1       1       1       7   */          -1,
/*  1       0       0       0       8   */          -1,
/*  1       0       0       1       9   */          0,
/*  1       0       1       0       A   */          0,
/*  1       0       1       1       B   */          1,
/*  1       1       0       0       C   */          0,
/*  1       1       0       1       D   */          1,
/*  1       1       1       0       E   */          -1,
/*  1       1       1       1       F   */          0,
};

/* The function called by the Arduino framework to initialise the system */
void setup() 
{
  /* Open the serial port */
  Serial.begin(SERIAL_BIT_RATE);

  /* Set-up the built-in LED */
  pinMode(LED_BUILTIN, OUTPUT);

  /* Set up the ISR indicator output */
  pinMode(PULSE_OUT, OUTPUT);
    
  /* Set=up quadrature monitoring */
  pinMode(DI_QUAD_B, INPUT_PULLUP);
  pinMode(DI_QUAD_A, INPUT_PULLUP);

  /* Set up the previous quad state for the first interrupt */
  QuadState = QUAD_PORT & MSK_NEW_STATE;
  QuadStatePrevious = QuadState >> RIGHT_SHIFT_NEW_TO_PREV;

  attachInterrupt(0, QuadChange, CHANGE);
  attachInterrupt(1, QuadChange, CHANGE);

  /* Disable timer 0 interrupts: this disables the millis timer. */
  TIMSK0 &= ~_BV(TOIE0);
  
  /* Set up timer 1 . */
  TCCR1A = 0;
  
  TCCR1B = 0;
  TCCR1B = _BV(CS10);    // prescaler of 1 

   Serial.println("quad to single v1.0");

}

/* The main function called by the Arduino framework. */
void loop() 
{
}

/* ISR for change in state of either quadrature input */
void QuadChange()
{
  QuadState = (QUAD_PORT & MSK_NEW_STATE) | QuadStatePrevious;

  NewMovement = QuadLookup[QuadState];

  if (NewMovement == PrevMovement)
  {
    digitalToggleFast(PULSE_OUT);

    /* Clear the timer 1 overflow flag to prevent an immmediate overflow
      ISR running when the interrupt is enabled. */
    TIFR1 |= _BV(TOV1);

    /* Set the timer to expire a pulse width period from now. */
    /* The ATMega 328 requires that the high register is loaded first. */
    TCNT1 = PULSE_LEN_TIMER_VAL;

    /* Trigger the timer - enable the overflow interrupt */
    TIMSK1 |= _BV(TOIE1);  
  }

  PrevMovement = NewMovement;

  QuadStatePrevious = QuadState >> RIGHT_SHIFT_NEW_TO_PREV;

}


/* ISR for the timer 1 overflow */
ISR(TIMER1_OVF_vect)
{
  digitalToggleFast(PULSE_OUT);

  /* Stop the timer - disable the overflow interrupt */
  TIMSK1 &= ~_BV(TOIE1);
}

    
