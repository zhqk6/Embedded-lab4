/*
 * hzklab4-1.c
 *
 *  Created on: Mar 1, 2016
 *      Author: zhqk6
 */

#ifndef MODULE
#define  MODULE
#endif

#ifndef __KERNEL__
#define  __KERNEL__
#endif

#include<linux/module.h>
#include<linux/kernel.h>
#include<asm/io.h>
#include<linux/time.h>
#include<rtai.h>
#include<rtai_sched.h>                  //  scheduler
#include<rtai_fifos.h>//for fifos
#include"ece4220lab3.h"

MODULE_LICENSE("GPL");        //  avoid issues

static RT_TASK mytask;
RTIME period;
unsigned long *pbdr,*pbddr,*ptr;
struct timeval start1;
unsigned int timestamp;

static void rt_process(int t){
	ptr = (unsigned long*)__ioremap(0x80840000,4096,0);
	pbdr = ptr+1;
	pbddr = ptr+5;
	*pbddr|= 0x00;
	*pbdr|=0xFF;   //initialize pbdr value 11111111 (the first button)
	while(1){
		if(*pbdr==0xFE)   // Judge that if pbdr is 11111110, which means the first button is pressed
		{
		do_gettimeofday(&start1);                               // get timestamp of button event
	    timestamp = 1000*(start1.tv_sec)+(start1.tv_usec)/1000; // calculate timestamp
        rtf_put(0,&timestamp,sizeof(timestamp));                // put the value of timestamp into fifo
        *pbdr|=0xFF;                                            // give value 11111111 to pbdr
		}
		rt_task_wait_period();
	}
}

int init_module(void){
	int timestamp;
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));
	rt_task_init(&mytask,rt_process,0,256,0,0,0);              //initialize realtime task
	rt_task_make_periodic(&mytask,rt_get_time()+10*period,75*period); //make it periodic
    if(rtf_create(0,1*sizeof(timestamp))<0){                   // create fifo
		printk("error");
	}
return 0;
}

void cleanup_module(void){
	rt_task_delete(&mytask);                                   //delete realtime task
	stop_rt_timer();
	rtf_destroy(0);
}

