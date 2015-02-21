#include <msp430.h>


int TXByteCtr = 1;
unsigned char PRxData;                     // Pointer to RX data
int Rx = 0;
unsigned char WHO_AM_I = 0x00;

void init_I2C(void);
void Transmit(void);
void Receive(void);
int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  init_I2C();
  while(1){
  //Transmit process
  Rx = 0;
  Transmit();
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent

 // Receive process
  Rx = 1;
  Receive();
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  }
}

//-------------------------------------------------------------------------------
// The USCI_B0 data ISR is used to move received data from the I2C slave
// to the MSP430 memory. It is structured such that it can be used to receive
//-------------------------------------------------------------------------------
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
  if(Rx == 1){                              // Master Recieve?
  PRxData = UCB0RXBUF;                       // Get RX data
  }

  else{                                     // Master Transmit
  if (TXByteCtr)                            // Check TX byte counter
  {
      UCB0TXBUF = WHO_AM_I;                     // Load TX buffer
      TXByteCtr--;                            // Decrement TX byte counter
  }
  else
  {
      UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
      IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
  }
 }

}

//interrupt(USCIAB0RX_VECTOR) state change to trap the no_Ack from slave case
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{
    if(UCNACKIFG & UCB0STAT) {
        UCB0STAT &= ~UCNACKIFG;             // Clear flag so that not to come here again
        i2c_packet.isr_mode=NOACK;          // The main function needs to act based on noack
        UCB0CTL1 |= UCTXSTP;                // I2C stop condition
    }
}

void init_I2C(void) {
    P1SEL |= BIT6 + BIT7;                   //Set I2C pins
    P1SEL2|= BIT6 + BIT7;
    UCB0CTL1 |= UCSWRST;                    //Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;   //I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;          //Use SMCLK, keep SW reset
    UCB0BR0 = 10;                           //fSCL = SMCLK/11 = ~100kHz
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;                   //Clear SW reset, resume operation
    IE2 |= UCB0TXIE;                        //Enable TX interrupt
    IE2 |= UCB0RXIE;                        //Enable RX interrupt
    UCB0I2CIE |= UCNACKIE;                  //Need to enable the status change interrupt
    __enable_interrupt();                   //Enable global interrupt
}

void Transmit(void){
    while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
    UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
   // __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
}
void Receive(void){
   UCB0CTL1 &= ~UCTR ;                  // Clear UCTR
   while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
    UCB0CTL1 |= UCTXSTT;                    // I2C start condition
    while (UCB0CTL1 & UCTXSTT);             // Start condition sent?
    UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
}
