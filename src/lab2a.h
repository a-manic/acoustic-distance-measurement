/*****************************************************************************
* lab2a.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#ifndef lab2a_h
#define lab2a_h

enum Lab2ASignals {
	FIVE_FT_CALIB = Q_USER_SIG,
	TEN_FT_CALIB,
	DO_CALIB,
	GO_CALIB,
	GO_TEST,
	DO_TEST,
	CALC_DIST
};


extern struct Lab2ATag AO_Lab2A;


void Lab2A_ctor(void);
void dma_sw_tone_gen_calib(int flag);
void calibration_mod();
void dma_sw_tone_gen_test();
double dist_func();

//Global Definitions
double uncalib_dist[2];
double corr_constants[2];

int calib_done;
int ten_ft_flag;
int dist_done;


#endif  
