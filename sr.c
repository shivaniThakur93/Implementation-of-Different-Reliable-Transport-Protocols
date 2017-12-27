#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
#include<stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#define MAXPACKETS 1000
int nextSeqA;
int timeOutSeqA;
int minUnackedSeqA;
int minAbsExpSeqA;
int flagTimerA;
int stopFlagA;
int baseA;
int A_State;
int B_State;
int maxSeqA;
int buffMsgNo;
int indexA;
int winSizeA;
int winSizeB;
int buffFlagB;
int prevAckBtoA;
int recvSeqB;
int prevExpSeqB;
int seqFieldB;
int lastIndex;
char dataB;
float rtt;
float alphaOneA;
float alphaTwoA;
float betaOneA;
float betaTwoA;
float sRTT;
float dRTT;
float eRTT;
struct buffMsg{
 struct pkt pkt_AtoB;
 int passType;//1 indicates msg from A upper layer,odrwise 0
 int sent;//1 means packet is sent
 float sendTime;
 float absExpTime;
};
struct recvBuffMsg{
 char message[20];
 int recv;
 int delivered;
};
struct buffMsg buffmsgsA[MAXPACKETS];
struct recvBuffMsg buffmsgsB[MAXPACKETS];
//deliver buffered messages if any B
void deliverBuffMsgsB(int seq){
    int i;
    int j=0;
    B_State=0;
    for(i=seq+1;i<seq+winSizeA;i++){
    if(buffmsgsB[i].recv){
        printf("PKT WITH SEQ NO::%d IN BUFFER DELIVERED \n",i);
        printf("BUFFERED PKT DELIVERED TO UPPER LAYER::%s\n",buffmsgsB[i].message);
        buffmsgsB[i].delivered=1;
        tolayer5(1,buffmsgsB[i].message);
        j++;
        }
    else{
        break;
        }
    }
    B_State=1;
    recvSeqB=recvSeqB+j; 
    printf("DELIVER BUFF MSGS B RECV NO::%d \n",recvSeqB);
}
//create checksum method for sender
int createCheckSumA(struct pkt packet){
int checksum;
int i;
checksum=packet.acknum+packet.seqnum;
size_t len=strlen(packet.payload);
if(len>20){
	len=20;
}
for(i=0;i<len;i++){
 checksum+=(int)(packet.payload[i]);
}
return checksum;
}
//send packet method from A to B
void sendPktAtoB(){
printf("SEND PKT A TO B \n");
printf("LATEST SEND INDEX::%d\n",indexA);
printf("SEND BASE::%d\n",baseA);
printf("NEXT AVAL SEQ NO::%d\n",nextSeqA);
printf("LAST INDEX::%d\n",lastIndex);
int checksumA;
A_State=0;
int i;
for(i=lastIndex;i<baseA+winSizeA;i++){
   if(buffmsgsA[i].passType && !buffmsgsA[i].sent && !buffmsgsA[i].pkt_AtoB.acknum){//msg in buffer
	printf("Sending Message::%s,Seq No::%d\n",buffmsgsA[i].pkt_AtoB.payload,nextSeqA);
		buffmsgsA[i].pkt_AtoB.acknum=0;
		buffmsgsA[i].pkt_AtoB.seqnum=nextSeqA;
		checksumA=createCheckSumA(buffmsgsA[i].pkt_AtoB);	
		buffmsgsA[i].pkt_AtoB.checksum=checksumA;
		buffmsgsA[i].sent=1;
		buffmsgsA[i].sendTime=get_sim_time();
		buffmsgsA[i].absExpTime=rtt+get_sim_time();
		tolayer3(0,buffmsgsA[i].pkt_AtoB);
		if(!flagTimerA){//1 means timer started, 0 means stopped
		printf("STARTING TIMER FOR::%s SEQ NO::%d RTT::%f \n",buffmsgsA[i].pkt_AtoB.payload,i,rtt);
		starttimer(0,rtt);
		timeOutSeqA=i;// need to check
		minAbsExpSeqA=i+1;
		flagTimerA=1;
		}
		indexA++;
		nextSeqA++;
		printf("NEXT SEQ A::%d \n",nextSeqA,baseA);
	}
	else{
		 break;
	}
  }	
   if(nextSeqA==baseA+winSizeA){
      printf("WINDOW FULL ::NEXT SEQ A ::%d",nextSeqA);
      A_State=0;
     }
    else if(nextSeqA<baseA+winSizeA){
      printf("WINDOW EMPTIED ::NEXT SEQ A ::%d",nextSeqA);
      A_State=1;
    }
}
// return minimum unacknowledged sequence number
int findMinimumUnackedSeqNo(){
    int i;
    int findSeq;
    int j=0;
    printf("findSeq::%d \n",findSeq);
    for(i=baseA+1;i<baseA+winSizeA;i++){
        printf("PASSTYPE::%d SENT::%d ACKED::%d \n",buffmsgsA[i].passType,buffmsgsA[i].sent,buffmsgsA[i].pkt_AtoB.acknum);
        if(!buffmsgsA[i].passType){
            findSeq=i;
            j=1;
            break;
        }
        else if(buffmsgsA[i].passType && !buffmsgsA[i].sent){
            findSeq=i;
            j=1;
            break;
        } 
        else if(buffmsgsA[i].passType && buffmsgsA[i].sent && !buffmsgsA[i].pkt_AtoB.acknum){
            printf("Unacked element with min seq no::%d \n",i);
            findSeq=i;
            j=1;
            break;
        }    
     }
     if(j==0){
         printf("J is zero \n");
         findSeq=i;
     }
     printf("Out of loop:: Unacked element with min seq no::%d i::%d\n",findSeq,i);
     return findSeq;
}
// reset timer 
void resetTimer(){
    float rttNew;
    int findSeq;
    int i;
    int ackFlag=1;
    float min;
    float now;
    findSeq=baseA;
    min=buffmsgsA[baseA].absExpTime;
    printf("RESET TIMER BASE A::%d \n",baseA);
    for(i=baseA;i<baseA+winSizeA;i++){
    if(buffmsgsA[i].sent && !buffmsgsA[i].pkt_AtoB.acknum){
        printf("Unacked element found at %d \n",i);
        ackFlag=0;
        if(min>buffmsgsA[i].absExpTime){
          min=buffmsgsA[i].absExpTime;
          findSeq=i;
         }
     }
    else if(!buffmsgsA[i].sent){
        break;
     }
    }
    if(ackFlag){
        printf("STOPPING TIMER :: NO UNACKED ELEMENT FOUND \n");
        stoptimer(0);
        flagTimerA=0;
        }
    else{
        minAbsExpSeqA=findSeq;
        printf("MINIMUM ABS EXP TIME FOR::%s SEQ NO::%d \n",buffmsgsA[minAbsExpSeqA].pkt_AtoB.payload,minAbsExpSeqA);
        now=get_sim_time();
        rttNew=buffmsgsA[minAbsExpSeqA].absExpTime-now;
        timeOutSeqA=minAbsExpSeqA;  
        printf("SEND TIME :: %f RTTNEW::%f PREVIOUS RTT: %f \n",buffmsgsA[minAbsExpSeqA].sendTime,rttNew,rtt);
        printf("NEW TIMER SET FOR SEQ NO ::%d ABS EXP TIME::%f \n",minAbsExpSeqA,buffmsgsA[minAbsExpSeqA].absExpTime);
        if(stopFlagA){
           stoptimer(0);
          }
        starttimer(0,rttNew);
        flagTimerA=1;
        }
    
}
// send the packet for which timeout occured
void sendPktOnTimeout(){
    float rttNew;
    float now;
    printf("RETRANSMISSION FOR::%s Seq No::%d \n",buffmsgsA[timeOutSeqA].pkt_AtoB.payload,buffmsgsA[timeOutSeqA].pkt_AtoB.seqnum);
	tolayer3(0,buffmsgsA[timeOutSeqA].pkt_AtoB);
	now=get_sim_time();
	buffmsgsA[timeOutSeqA].sendTime=now;
	buffmsgsA[timeOutSeqA].absExpTime=now+rtt;
	stopFlagA=0;
	resetTimer();
}
void A_output(message)
  struct msg message;
{
if(A_State){
	printf("IN A_OUTPUT data from above::%s INDEX::%d \n",message.data,buffMsgNo);
	strcpy(buffmsgsA[buffMsgNo].pkt_AtoB.payload,message.data);
	buffmsgsA[buffMsgNo].passType=1;
	indexA=buffMsgNo;
	lastIndex=indexA;
	sendPktAtoB();
	buffMsgNo++;
}
else{
	//window full,buffer message
	printf("IN A_OUTPUT, window full, buf msg : AT INDEX:: %d \n",buffMsgNo);
	printf("IN A_OUTPUT, window full, buf msg ::%s\n",message.data);
	strcpy(buffmsgsA[buffMsgNo].pkt_AtoB.payload,message.data);
	buffmsgsA[buffMsgNo].passType=1;
	buffMsgNo++;
}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int ack_BToA=packet.acknum;
    int seq_BtoA=packet.seqnum;
    int checksum_BtoA=packet.checksum;
    int checkSumVal;//expected checksum
    checkSumVal=seq_BtoA+ack_BToA+(int)(packet.payload[0]);
    printf("in A_input B to A::Ack::%d Seq::%d Char::%c \n",ack_BToA,seq_BtoA,packet.payload[0]);
    printf("CS from B to A::%d and Expected CS at A::%d\n",checksum_BtoA,checkSumVal);
    if(checksum_BtoA==checkSumVal && ack_BToA>=baseA && ack_BToA<=indexA){
    printf("ACK RECV FOR SEQ::%d MSG::%s \n",ack_BToA,buffmsgsA[ack_BToA].pkt_AtoB.payload);
    if(!buffmsgsA[ack_BToA].pkt_AtoB.acknum){
            sRTT=get_sim_time()-buffmsgsA[ack_BToA].sendTime;
            printf("SAMPLE RTT::%f \n",sRTT);
            eRTT=alphaOneA*eRTT + alphaTwoA*sRTT;
            printf("EST RTT RTT::%f \n",eRTT);
            dRTT=betaOneA*dRTT + betaTwoA*(abs(eRTT-sRTT));
            printf("DEVIATION RTT::%f \n",dRTT);
            rtt=eRTT+(4.0*dRTT);
            printf("RTT::%f \n",rtt);
        }
    printf("Element with seq no::%d acked \n",ack_BToA);
    buffmsgsA[ack_BToA].pkt_AtoB.acknum=1;
    printf("Base Value::%d \n",baseA);
    if(ack_BToA==baseA){
        printf("Ack received for base no::%d \n",ack_BToA);
        baseA=findMinimumUnackedSeqNo();//packet acked
    }
    prevAckBtoA=ack_BToA;
    lastIndex=indexA;
    printf("BASE A::%d & NEXT SEQ NO::%d Current Index::%d \n",baseA,nextSeqA,indexA);
    //if pkt is already acked-> dont consider sample RTT to estimate RTT
              
    printf("TIMOUT VALUE::%f \n",rtt);
    stopFlagA=1;
    if(ack_BToA==timeOutSeqA){
        resetTimer();//reset timer only when the ack for the element for which the timer is set is acked
    }
    sendPktAtoB();
    }
    else{//make adaptive, if corrupted checksum 
    printf("CORR/DUP ACK DID NOTHING \n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf("TIMEOUT OCCURED \n");
    sendPktOnTimeout();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    rtt=17.0;  
    timeOutSeqA=0;
    flagTimerA=0;
    minAbsExpSeqA=0;
    winSizeA=getwinsize();
    A_State=1;
    alphaOneA=0.875;
    alphaTwoA=0.125;
    betaOneA=0.75;
    betaTwoA=0.25;
    dRTT=0.0;
    eRTT=rtt;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
    struct pkt pkt_BtoA;
	int checkSumAtoB;
	int expCheckSumAtoB;
	int seqAtoB;
	int recvBMax;
	seqAtoB=packet.seqnum;
	checkSumAtoB=packet.checksum;
	expCheckSumAtoB=createCheckSumA(packet);
	printf("Packet Arrived at B::%s\n",packet.payload);
	printf("Expected checksum::%d and Arrived checksum::%d\n",expCheckSumAtoB,checkSumAtoB);
	printf("EXP Seq No::%d Recv Seq No::%d\n",recvSeqB,seqAtoB);
	if(checkSumAtoB==expCheckSumAtoB){
	    if(seqAtoB==recvSeqB && B_State){
	         printf("ORDERED PKT DELIVERED TO UPPER LAYER::%s\n",packet.payload);
	         buffmsgsB[seqAtoB].delivered=1;
	         buffmsgsB[seqAtoB].recv=1;
             tolayer5(1,packet.payload);
             deliverBuffMsgsB(seqAtoB);       
             recvSeqB++;   
             printf("NEXT SMALLEST RECV NO::%d \n",recvSeqB);
	    }
	    else if(seqAtoB>recvSeqB || seqAtoB<recvSeqB){
	        if(buffmsgsB[seqAtoB].delivered || buffmsgsB[seqAtoB].recv){
	            printf("DISCARDING PACKET WITH SEQ NO::%d ALREADY DELIVERED/BUFFERED|\n",seqAtoB);
	        }
	        else{
	             printf("OUT OF ORDER PKT SEQ NO::%d BUFFERED::%s\n",seqAtoB,packet.payload);
	             buffFlagB=1;
	             buffmsgsB[seqAtoB].recv=1;
	             strcpy(buffmsgsB[seqAtoB].message,packet.payload);
	            }
	       
	    }
	    pkt_BtoA.acknum=seqAtoB;
        pkt_BtoA.seqnum=seqFieldB;
        pkt_BtoA.payload[0]=dataB;
        pkt_BtoA.checksum=seqAtoB+seqFieldB+(int)(dataB);
        printf("B to A ACK::%d Checksum::%d\n",pkt_BtoA.acknum,pkt_BtoA.checksum);
	    tolayer3(1,pkt_BtoA);
	}
    else{
        printf("CORR PKT NOT DEL TO UPPER LAYER B \n");
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	recvSeqB=0;
	seqFieldB=1;
	dataB='B';
	winSizeB=getwinsize();
	B_State=1;
	buffFlagB=0;
}
