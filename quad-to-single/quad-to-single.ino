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
#define PULSE_LEN_US      75U

/* The number of clock counts that sets the output pulse width. */
#define PULSE_LEN_CPU_CLOCKS    (PULSE_LEN_US * CPU_CLOCK_PER_US)

/* We're using the 16 bit timer in overlflow, so we need to set it up so that 
   the starting count is such that it will overflow after PULSE_LEN_CPU_CLOCKS */
#define PULSE_LEN_TIMER_VAL     (0xFFFFU - PULSE_LEN_CPU_CLOCKS)

/* Forward function definitions */
static void QuadChange();


/* The bit rate at which the Arduino serial port runs */
#define SERIAL_BIT_RATE 115200

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
  attachInterrupt(0, QuadChange, CHANGE);
  attachInterrupt(1, QuadChange, CHANGE);

  /* Disable timer 0 interrupts: this disables the millis timer. */
  TIMSK0 &= ~_BV(TOIE0);
  
  /* Set up timer 1 . */
  TCCR1A = 0;
  
  TCCR1B = 0;
  TCCR1B = _BV(CS10);    // prescaler of 1 

   Serial.print("* ");
  // Serial.print("PULSE_LEN_TIMER_VAL ");
  // Serial.println(PULSE_LEN_TIMER_VAL);

}

/* The main function called by the Arduino framework. */
void loop() 
{
}


/* ISR for change in state of either quadrature input */
void QuadChange()
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


/* ISR for the timer 1 overflow */
ISR(TIMER1_OVF_vect)
{
  digitalToggleFast(PULSE_OUT);

  /* Stop the timer - disable the overflow interrupt */
  TIMSK1 &= ~_BV(TOIE1);
}

    
