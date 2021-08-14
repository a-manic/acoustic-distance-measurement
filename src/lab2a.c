/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "lcd.h"
#include "stream_grabber.h"
#include "fft.h"
#include "stdio.h"
#include "time.h"


#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
#define M 9 //2^m=samples
#define CLOCK 100000000.0 //clock speed


static float q[SAMPLES];
static float q1[SAMPLES];
static float q2[SAMPLES];
static float w[SAMPLES];

typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_stateA  (Lab2A *me);
static QState Lab2A_stateB  (Lab2A *me);


/**********************************************************************/


Lab2A AO_Lab2A;

int volume =0, volume_temp;



void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_stateA);
			}
	}
	
	return Q_SUPER(&QHsm_top);
}


/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState Lab2A_stateA(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			clrScr();
			setColor(255,0,127);
			setFont(BigFont);


			lcdPrint("IN CALIBRATION", 10, 30);
			lcdPrint("MODE", 90, 50);

			lcdPrint("Press Left (L)", 10, 100);
			lcdPrint("Button to", 40, 120);
			lcdPrint("Continue", 50, 140);

			lcdPrint("Press Right (R)", 3, 190);
			lcdPrint("Button for", 40, 210);
			lcdPrint("Test Mode", 50, 230);
			return Q_HANDLED();
		}
		
		case FIVE_FT_CALIB: {
			clrScr();
			setColor(255,0,127);
			setFont(BigFont);


			lcdPrint("Stand at 5 ft", 15, 70);
			lcdPrint("distance", 50, 95);

			lcdPrint("Press L to", 40, 145);
			lcdPrint("start the", 50, 170);
			lcdPrint("Calibration", 30, 195);

			ten_ft_flag = '0';
			return Q_HANDLED();
		}

		case DO_CALIB: {
			clrScr();
			setColor(255,0,127);
			setFont(BigFont);


			lcdPrint("Calibrating..", 20, 140);
		    dma_sw_tone_gen_calib(ten_ft_flag);

		     return Q_HANDLED();
		}

		case TEN_FT_CALIB:  {
			clrScr();
			setColor(255,0,127);
			setFont(BigFont);


			lcdPrint("Stand at 10 ft", 10, 70);
			lcdPrint("distance", 50, 95);

			lcdPrint("Press L to", 40, 145);
			lcdPrint("start the", 50, 170);
	        lcdPrint("Calibration", 30, 195);

			ten_ft_flag = '1';

			 return Q_HANDLED();
		}
		case GO_TEST:  {
			clrScr();
			setColor(255,0,127);
			setFont(BigFont);


			lcdPrint("Press R for", 35, 70);
			lcdPrint("Test Mode", 50, 95);


			lcdPrint("Press L to", 40, 155);
			lcdPrint("Calibrate", 50, 180);
			lcdPrint("Again", 82, 205);


			 return Q_TRAN(&Lab2A_stateB);
			}

	}

	return Q_SUPER(&Lab2A_on);

}

QState Lab2A_stateB(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {

			return Q_HANDLED();
		}
		
		case DO_TEST: {

			clrScr();
			setColor(255,0,127);
			setFont(BigFont);

			lcdPrint("IN TEST MODE", 30, 30);


			lcdPrint("Press R to", 45, 70);
			lcdPrint("Measure ", 70, 95);
			lcdPrint("Distance", 60, 120);


			lcdPrint("Press L for", 30, 175);
			lcdPrint("Calibration", 30, 200);
			lcdPrint("Mode", 90, 225);
			return Q_HANDLED();
		}

		case CALC_DIST:  {
			  dma_sw_tone_gen_test();
			  return Q_HANDLED();
		}

		case GO_CALIB:  {

		     return Q_TRAN(&Lab2A_stateA);
			}

	}

	return Q_SUPER(&Lab2A_on);

}

void dma_sw_tone_gen_calib(int flag)
{
	 if (flag)  // 10 ft
	 {
		 	uncalib_dist[1] = dist_func();
		 	calibration_mod();
	 }
	 else {      //5ft
		 	 uncalib_dist[0] = dist_func();
	 }
	 ButtonFSM(3);
}

void calibration_mod()
{
    // calculate m and c
	corr_constants[0] = 5/(uncalib_dist[1]- uncalib_dist[0]);
    corr_constants[1] = (5*uncalib_dist[1]) - (10*uncalib_dist[0]);
}
void dma_sw_tone_gen_test()
{
	int i,rand = 0;
	double rough_dist;
	double actual_dist;

    rough_dist = dist_func();

    actual_dist = (corr_constants[0]*rough_dist) + corr_constants[1];


    	double correction = 3.23; // calculated based on running 100 trials

    	char distanceString[50];
    	char correctionString[50];


    	sprintf(distanceString, "%f", actual_dist); // typecast double to String
    	sprintf(correctionString, "%f", correction);
    	clrScr();

    	setColor(255,0,127);
    	setFont(BigFont);

    	lcdPrint("Distance in ft :", 2, 50);
    	lcdPrint(distanceString, 50, 90);

    	lcdPrint("+ / -", 80, 140);
    	lcdPrint(correctionString, 50, 170);

       for(i = 0; i < 25000000;i++)
       {
    	   rand++;
        }

	ButtonFSM(0);
}

XStatus dma_send_func(Demo *p_demo_inst, UINTPTR buffer, u32 length) {
	XStatus status;

	Xil_DCacheFlushRange(buffer, length);

	status = XAxiDma_SimpleTransfer(&(p_demo_inst->dma_inst), buffer, length, XAXIDMA_DMA_TO_DEVICE);

	if (status != XST_SUCCESS)
		xil_printf("ERROR, DMA Simple transfer was not done\n");

	while (XAxiDma_Busy(&(p_demo_inst->dma_inst), XAXIDMA_DMA_TO_DEVICE));

	if ((XAxiDma_ReadReg(p_demo_inst->dma_inst.RegBase, XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET) & XAXIDMA_IRQ_ERROR_MASK) != 0) {
		xil_printf("ERROR in reading DMA\r\n");
		return XST_FAILURE;
	}

	Xil_DCacheFlushRange((UINTPTR)buffer, length);

	return XST_SUCCESS;
}

void dma_reset_func(Demo *p_demo_inst) {
	XAxiDma_Reset(&(p_demo_inst->dma_inst));
	while (!XAxiDma_ResetIsDone(&(p_demo_inst->dma_inst)));
}


double dma_sw_tone_gen_dist(Demo *p_demo_inst) {
	XStatus status;
	static const u32 buffer_size = 128;
	u32 accum = 0;
	UINTPTR buffer = (UINTPTR)malloc(buffer_size * sizeof(u8));
	memset((u32*)buffer, 0, buffer_size * sizeof(u8));
	int i,k = 0;

	float sample_f = 48828.125;
	float s = 95.36743164; //sample frequency/ number of bins

	int  place; //used for timer
	float frequency, distance;
	double phase;


	stream_grabber_start();


	while (k<5000) {
		for (u32 i=0; i<buffer_size; i++) {
			accum += 0x32AAAAAA;  //19KHz
			((u8*)buffer)[i] = accum>>24;

		}

		status = dma_send_func(p_demo_inst, (UINTPTR)buffer, buffer_size);

		k++;
	}
		accum = 0;

	for(i = 0; i < SAMPLES; i++)
          {
		q1[i] = (unsigned int)stream_grabber_read_sample(i);
		w[i] = 0;



		accum += 0x32AAAAAA;  // 19KHz
		q2[i] = accum>>24;


		q[i] = q1[i]*q2[i];
	   }

	stream_grabber_start();

	fft(q,w,SAMPLES,M,sample_f,s, &frequency, &phase, &place, &distance);
	stream_grabber_wait_enough_samples(SAMPLES);

	p_demo_inst->mode = DEMO_MODE_PAUSED;
	free((u32*)buffer);

	dma_reset_func(p_demo_inst);

	return (double)distance;

}

double dist_func()
{
	double final_dist;
	Demo device;
	XStatus status;
		status = init_call_dma(&device);
		if (status != XST_SUCCESS) {
			xil_printf("ERROR: Demo inside dist\r\n");
			sleep(1);
		} else

	device.mode = DEMO_MODE_SW_TONE_GEN;
	final_dist = dma_sw_tone_gen_dist(&device);
	return final_dist;
}


