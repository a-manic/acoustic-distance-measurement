/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xtmrctr.h"
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"
#include "xil_printf.h"  // from here copied from helloworld
#include "xaxidma.h"
#include "xparameters.h"
#include "xstatus.h"
#include "sleep.h"
#include "xgpio.h"
#include "xil_cache.h"
#include <stdio.h>
#include <mb_interface.h>
#include <xil_types.h>
#include <xil_assert.h>
#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "stream_grabber.h"


/*****************************/

/* Define all variables and Gpio objects here  */

XIntc Intc; /* The Instance of the Interrupt Controller Driver */
XGpio axi_gpio_Encoder_Gpio; /* The Instance of the Encoder Button GPIO Driver */
XGpio Btn; /* The Instance of the Button GPIO Driver */
XTmrCtr TimerCounter0;   /* The instance of the Timer Counter 0 */
XTmrCtr TimerCounter1;   /* The instance of the Timer Counter 0 */
XGpio dc;
XSpi spi;
XSpi_Config *spiConfig;	/* Pointer to Configuration data */
enum State {S0,S1,S2,S3,S4,S5,S6,S7};
enum State activeState = S0;
int blackbox;
int background;
int timer0_done;



XStatus initialize_dma (XAxiDma *p_dma_inst, int dma_device_id) {
// Local variables
	XStatus dmastatus = 0;
	XAxiDma_Config* cfg_ptr;

	// Find the hardware config of device first, if this order messed then error so stick to it
	cfg_ptr = XAxiDma_LookupConfig(dma_device_id);
	if (!cfg_ptr)
	{
		xil_printf("ERROR! No hardware config %d.\r\n", dma_device_id);
		return XST_FAILURE;
	}

	// Initializing the DMA driver here
	dmastatus = XAxiDma_CfgInitialize(p_dma_inst, cfg_ptr);
	if (dmastatus != XST_SUCCESS)
	{
		xil_printf("ERROR! Init DMA failed due to %d\r\n", dmastatus);
		return XST_FAILURE;
	}

	// Test for Scatter Gather mode, this should be checked in Hardware Block design first,I changed it in BD, otherwise kept throwing error
	if (XAxiDma_HasSg(p_dma_inst))
	{
		xil_printf("ERROR! Device configured as SG mode.\r\n");
		return XST_FAILURE;
	}

	// Disable all interrupts for both channels: Necessary
	XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	// Reset DMA
	XAxiDma_Reset(p_dma_inst);
	while (!XAxiDma_ResetIsDone(p_dma_inst));
	return XST_SUCCESS;
}

// DMA Audio and Microphone functions
XStatus init_call_dma(Demo *p_demo_inst) {
	XStatus status;

	status = initialize_dma(&(p_demo_inst->dma_inst), XPAR_AXI_DMA_0_DEVICE_ID);
	if (status != XST_SUCCESS) return XST_FAILURE;
	return XST_SUCCESS;
}

void BSP_init(void) {
//Our code starts here//
	int status;
	u32 controlReg;
	Demo device;
	XStatus DMA_status;


//Encoder
	status = XGpio_Initialize(&axi_gpio_Encoder_Gpio, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	if (status != XST_SUCCESS) {
		status =  XST_FAILURE;
		print("Encoder initialization failed!");
	}
	else
	{
		print("Encoder Success");
	}
	XGpio_SetDataDirection(&axi_gpio_Encoder_Gpio, GPIO_CHANNEL1, 0xFFFFFFFF); //0xFFFFFFFF is for setting it to input

//PushButtons
	status = XGpio_Initialize(&Btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if (status != XST_SUCCESS) {
	    xil_printf("Failed to initialize Button GPIO driver\n\r");
	}
	else
	{
		print("PushButton Success");
	}
	XGpio_SetDataDirection(&Btn,GPIO_CHANNEL1,0xFFFF);



// LCD SPI
	status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO dc fail!\n");
	}
	else
	{
	  print("GPIO dc Success");
	}

	XGpio_SetDataDirection(&dc, 1, 0x0);

	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);  // Initialize the SPI driver so that it is  ready to use.
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");

	}
	else
	{
	  print("SPI device Success");
	}

	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");

	}
	else
	{
	  print("SPI initialize Success");
	}

	XSpi_Reset(&spi); //Reset the SPI device to leave it in a known good state.
	controlReg = XSpi_GetControlReg(&spi); // Setup the control register to enable master mode
	XSpi_SetControlReg(&spi,(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) & (~XSP_CR_TRANS_INHIBIT_MASK));
	XSpi_SetSlaveSelectReg(&spi, ~0x01);
	initLCD();
//XIntc initialize
	status = XIntc_Initialize(&Intc, INTC_DEVICE_ID);
		if (status != XST_SUCCESS) {
			xil_printf("Failed to initialize Interrupt Controller\n\r");
		}
		else {
			xil_printf("Successfully initialized Interrupt Controller\n\r");
		}


// Hook up interrupt service routine
	//XIntc_Connect(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR,(XInterruptHandler)debounceInterrupt, &axi_gpio_Encoder_Gpio);  // Encoder Button
	XIntc_Connect(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,(XInterruptHandler)timer0_handler,(void *)&TimerCounter0); //Timer 0
	XIntc_Connect(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_1_INTERRUPT_INTR,(XInterruptHandler)timer1_handler,(void *)&TimerCounter1); //Timer 1
	XIntc_Connect(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,(XInterruptHandler)GpioHandler, &Btn); //Push Button

	XIntc_Enable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
	XIntc_Enable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_1_INTERRUPT_INTR);

	// Timer 0: Used for monitoring the disappearing of Volume bar only
		status = XTmrCtr_Initialize(&TimerCounter0, XPAR_AXI_TIMER_0_DEVICE_ID);
		if (status != XST_SUCCESS) {
			xil_printf("Failed to initialize Timer0 Counter\n\r");
		}
		else
		{
			print("Timer counter 0 Success");
		}
		XTmrCtr_SetOptions(&TimerCounter0, TIMER_CNTR_0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
		XTmrCtr_SetResetValue(&TimerCounter0, TIMER_CNTR_0, 0xFFFFFFFF-RESET_VALUE_0);

	// Timer 1: Used for de-bouncing of encoder buttons only
		status = XTmrCtr_Initialize(&TimerCounter1, XPAR_AXI_TIMER_1_DEVICE_ID);
		if (status != XST_SUCCESS) {
			xil_printf("Failed to initialize Timer1 Counter\n\r");
		}
		else
		{
		  print("Timer counter 1 Success");
		}

		XTmrCtr_SetOptions(&TimerCounter1, TIMER_CNTR_1, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
		XTmrCtr_SetResetValue(&TimerCounter1, TIMER_CNTR_1, 0xFFFFFFFF-RESET_VALUE_1);

//Start the Interrupt controller
   status = XIntc_Start(&Intc, XIN_REAL_MODE);
	if (status != XST_SUCCESS) {
			xil_printf("Failed to start Interrupt Controller\n\r");
	}

// DMA Initialization
	DMA_status = init_call_dma(&device);
	if (DMA_status != XST_SUCCESS) {
		xil_printf("ERROR: Demo not initialized correctly\r\n");
		sleep(1);
	} else
		xil_printf("Demo started\r\n");


	print("exiting bsp init");
	clrScr();
		
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

//Our code starts here
//Enable interrupts here because you don't know whether the interrupt queue initialized correctly
//Permanently enable interrupts from the Encoder Push Button (Click) and Push Buttons (5 of them)

	XIntc_Enable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);

	XGpio_InterruptEnable(&axi_gpio_Encoder_Gpio, GPIO_CHANNEL1);
	XGpio_InterruptGlobalEnable(&axi_gpio_Encoder_Gpio);

	XGpio_InterruptEnable(&Btn, GPIO_CHANNEL1);
	XGpio_InterruptGlobalEnable(&Btn);

	microblaze_enable_interrupts();



//Handle all the exceptions
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)INTC_HANDLER, &Intc);
	Xil_ExceptionEnable();

}


void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();                       /* unlock interrupts */


}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/


void ButtonFSM (u32 input) {                   //New function
	//XGpio_DiscreteRead( &twist_Gpio, 1);


	switch(activeState){
	    		case(S0):{
	    		//S0 = '11'
	    			//print("base case");
	    			switch(input){
	    				case(1):{
	    					QActive_postISR((QActive *)&AO_Lab2A,FIVE_FT_CALIB);
	    					activeState = S1;
	    					break;
	    				}
	    				case(2):{
	    				    QActive_postISR((QActive *)&AO_Lab2A,GO_TEST);
	    				    activeState = S6;
	    				    break;
	    				}


	    			}
					break;
	    		}
	    		case(S1):{
	    		//S1 = 01
				switch(input){
	    				case(1):{
	    					 QActive_postISR((QActive *)&AO_Lab2A,DO_CALIB);
	    					 activeState = S2;
	    					break;
	    				}

	    			}
					break;
	    		}

	    		case(S2):{
	    		 //S2 = 10
				 switch(input){
	    				case(3):{
	    					 QActive_postISR((QActive *)&AO_Lab2A,TEN_FT_CALIB);
	    					 activeState = S3;
	    					break;
	    				}
	    			}
					break;

	    		}

	    		case(S3):{
	    		 //S3 = 00
				 switch(input){
	    				case(1):{
	    					QActive_postISR((QActive *)&AO_Lab2A,DO_CALIB);
	    					 activeState = S4;
	    					break;
	    				}

	    			}
					break;
	    		}
	    		case(S4):{
	    		 //S4 = 00
				 switch(input){
	    				case(3):{
	    					QActive_postISR((QActive *)&AO_Lab2A,GO_TEST);
	    					activeState = S5;
	    					break;
	    				}

	    			}
					break;
	    		}
	    		case(S5):{
	    		 //S5 = 10

				 switch(input){
	    				case(2):{
	    					QActive_postISR((QActive *)&AO_Lab2A,DO_TEST);
	    					activeState = S6;
	    					break;
	    				}
	    				case(1):{
	    			    	QActive_postISR((QActive *)&AO_Lab2A,GO_CALIB);
	    			    	activeState = S0;
	    			    	break;
	    			    }

	    			}
					break;
	    		}

	    		case(S6):{


				 switch(input){
	    				case(2):{
	    					 QActive_postISR((QActive *)&AO_Lab2A,CALC_DIST);
	    					 activeState = S7;
	    					break;
	    				}
	    				case(1):{
	    					QActive_postISR((QActive *)&AO_Lab2A,GO_CALIB);
	    					 activeState = S0;
	    					 break;
	    				}

	    			}
					break;
	    		}

	      		case(S7):{

	    				 switch(input){
	    					case(0):{
	    				    	QActive_postISR((QActive *)&AO_Lab2A,DO_TEST);
	    				    	activeState = S6;
	    				    	break;
	    				    }

	    	    		}
	    				break;
	    	    }

	    	}

}



void GpioHandler(void *CallbackRef) {         //Handler for push button press
// Increment A counter
	XGpio *GpioPtr = (XGpio *)CallbackRef;
// Clear the Interrupt from the push buttons
	XGpio_InterruptClear(GpioPtr, GPIO_CHANNEL1);

	u32 data = XGpio_DiscreteRead(GpioPtr, GPIO_CHANNEL1);
	//print("button");
	switch(data){

			case 0x04:  //Right button
				ButtonFSM(2);
				break;

			case 0x02:  //Left Button
				ButtonFSM(1);
				break;

		}

}


void timer0_handler(){
	u32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(TimerCounter0.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(TimerCounter0.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);

}


void timer1_handler(){

	u32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(TimerCounter1.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(TimerCounter1.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);

}

void TimerInt0_Enable(void)
{

	XTmrCtr_Start(&TimerCounter0, TIMER_CNTR_0);

}

void TimerInt0_Disable(void)
{
	XIntc_Disable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
}

void TimerInt1_Enable(void)
{
	XTmrCtr_Start(&TimerCounter1, TIMER_CNTR_1);

}

void TimerInt1_Disable(void)
{
	XIntc_Disable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_1_INTERRUPT_INTR);
}





