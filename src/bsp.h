/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h

#include "xaxidma.h"
#define INTC_HANDLER	XIntc_InterruptHandler

#define INTC_GPIO_INTERRUPT_ID	XPAR_INTC_0_GPIO_0_VEC_ID

#define INTC_DEVICE_ID	XPAR_INTC_0_DEVICE_ID

#define TIMER_CNTR_0	 0

#define TIMER_CNTR_1	 1

#define RESET_VALUE_0	 200000000 //2x10^8, Timer triggers every 2s //100000 - 1ms //Disappearing part

#define RESET_VALUE_1	 10000 //Timer triggers every 100us //100000 - 1ms // Button De-bouncing

#define INTC		XIntc

#define XIN_REAL_MODE 1

#define INTC_DEVICE_ID	XPAR_INTC_0_DEVICE_ID

#define INTC_HANDLER	XIntc_InterruptHandler

#define GPIO_CHANNEL1 1

#define VERBOSE 0

#define DDR_BASE_ADDR		XPAR_MIG7SERIES_0_BASEADDR
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)

#define BSP_showState(prio_, state_) ((void)0)

typedef enum DemoMode {
	DEMO_MODE_SW_TONE_GEN,
	DEMO_MODE_PAUSED = 0
} DemoMode;

typedef struct Demo {
	XAxiDma dma_inst;
	DemoMode mode;
} Demo;

                                              


/* bsp functions ..........................................................*/

void BSP_init(void);
void GpioHandler(void *CallbackRef);
void ButtonFSM(u32);
void timer0_handler(void);
void timer1_handler(void);
void TimerInt0_Enable(void);
void TimerInt0_Disable(void);
void TimerInt1_Enable(void);
void TimerInt1_Disable(void);
void EncoderBtnInt_Enable(void);
void EncoderBtnInt_Disable(void);
XStatus init_call_dma(Demo *p_demo_inst);
XStatus initialize_dma (XAxiDma *p_dma_inst, int dma_device_id);





#endif                                                             /* bsp_h */


