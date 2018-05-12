/*
 * hzklab4-2.c
 *
 *  Created on: Mar 13, 2016
 *      Author: zhqk6
 */


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <fcntl.h>
#include<rtai.h>
#include<rtai_sched.h>
#include<rtai_fifos.h>
#include"serial_ece4220.h"

unsigned char x;
ssize_t num_bytes;
int fd_fifo_out;
sem_t sem;
unsigned int Buff[2];        //this buffer is used between main() and T0()
struct Buffer{               //structure Buffer
	unsigned int i,j,k;
};

void ch0(void *buffer){      //child threads which may be used 0-4 times per 250ms
	int counter = 0;
	unsigned int x2,t2;
	double xe;
	struct Buffer *child = (struct Buffer*) buffer;  //structure passed from T0()
	while((*child).i==Buff[0]){                //wait for the next value(data x and its corresponding timestamp)
		counter++;
		fflush(stdout);
	}
	x2=Buff[0];
	t2=Buff[1];                                //give x2,t2 next value
	xe = (double)(x2-(*child).i)*((*child).k-(*child).j)/(t2-(*child).j)+(*child).i;
	//xe = (x2-x1)(te-t1)/(t2-te)+x1
	sem_wait(&sem);
	printf("[x1=%u,t1=%ums]\n",(*child).i,(*child).j);
	printf("[x2=%u,t2=%ums]\n",x2,t2);
	printf("[xe=%f,te=%ums]\n",xe,(*child).k);            //print x1,x2,t1,t2,xe,te
	printf("\n");
	sem_post(&sem);
	pthread_exit(0);

}

void T0(void * ptr0){
	unsigned int x1,t1,te,timestamp;
	pthread_t child[4];                         //create 4(at most) child threads
	struct Buffer buffer[4];
	if((fd_fifo_out = open("/dev/rtf/0",O_RDWR))<0){    //open fifo from button event
		printf("open fifo error");
		exit(-1);
	}
	int i;
	while(1){
		for(i=0;i<4;i++){                          //do 4 times
			if(read(fd_fifo_out,&timestamp,sizeof(timestamp))<0){
				printf("read fifo error");         //read fifo from button event
				exit(-1);
			}
			x1=Buff[0];
			t1=Buff[1];                            //read from buffer(from main())
			te=timestamp;
			(buffer[i]).i=x1;
			(buffer[i]).j=t1;
			(buffer[i]).k=te;                      //give these 3 values to structure buffer
			pthread_create(&child[i],NULL,(void*)&ch0,(void*)&buffer[i]);
			//create child threads (4 times at most)
		}
		fflush(stdout);
	}
}



int main(){
	unsigned int timestamp1;
	struct timeval TS1;
	sem_init(&sem,0,1);                   //initial semaphore
	int prt_id = serial_open(0,5,5);      //open serial port
	usleep(10);
	pthread_t thread0;
	pthread_create(&thread0,NULL,(void *)&T0,NULL);   //create thread T0
	while(1){
		read(prt_id,&x,sizeof(x));        // read from serial port
		gettimeofday(&TS1,NULL);          // get timestamp of button event
		timestamp1 = TS1.tv_sec*1000+(TS1.tv_usec)/1000;  // calculate timestamp
		Buff [0]= (unsigned int)x;
		Buff [1]= (unsigned int)timestamp1;    //give these two values to Buff[2]
		fflush(stdout);
	}
	return 0;
}


