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
#define MAXPACKETS 1000
int nextSeqA;
int baseA;
int A_State;
int buffMsgNo;
int indexA;
int winSizeA;
int expSeqB;
int prevExpSeqB;
int seqFieldB;
float rtt;
int lastIndex;
char dataB;
int timeout;
struct buffMsg{
 struct pkt pkt_AtoB;
 int passType;//1 indicates msg from A upper layer,odrwise 0
 int sent;//1 indicates packet already sent,0 otherwise
 float sendTime;
 int acked;
};
struct buffMsg buffmsgsA[MAXPACKETS];
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
//
void sendPacketsOnTimeOut(){
printf("sendPacketsOnTimeOut:: \n");
printf("LATEST SEND INDEX::%d\n",indexA);
printf("SEND BASE::%d\n",baseA);
printf("NEXT AVAL SEQ NO::%d\n",nextSeqA);
printf("LAST INDEX::%d\n",lastIndex);
int checksumA;
int i;
A_State=0;
timeout=1;
for(i=baseA;i<baseA+winSizeA;i++){
   if(buffmsgsA[i].passType && !buffmsgsA[i].sent){//msg in buffer
		buffmsgsA[i].pkt_AtoB.acknum=0;
		buffmsgsA[i].pkt_AtoB.seqnum=nextSeqA;
		checksumA=createCheckSumA(buffmsgsA[i].pkt_AtoB);
		if(i==baseA){
		printf("STARTING TIMER ::PLACE 1 \n");
		starttimer(0,rtt);
		}	
		buffmsgsA[i].pkt_AtoB.checksum=checksumA;
		buffmsgsA[i].sent=1;
		printf("Sending msg from buffer::%s,Seq No::%d\n",buffmsgsA[i].pkt_AtoB.payload,buffmsgsA[i].pkt_AtoB.seqnum);
		tolayer3(0,buffmsgsA[i].pkt_AtoB);	
		indexA++;
		nextSeqA++;
		if(i==nextSeqA){
		 printf("STARTING TIMER ::PLACE 2 \n");
		 starttimer(0,rtt);
		}
		printf("NEXT SEQ A::%d \n",nextSeqA);
	}
	else if(buffmsgsA[i].passType && buffmsgsA[i].sent){
	    printf("Packet Already Sent:Retransmission::%s Seq No::%d\n",buffmsgsA[i].pkt_AtoB.payload,buffmsgsA[i].pkt_AtoB.seqnum);
	    tolayer3(0,buffmsgsA[i].pkt_AtoB);
	}
	else{
		 break;
	}
}
timeout=0;
 if(nextSeqA==baseA+winSizeA){
      printf("WINDOW FULL ::NEXT SEQ A ::%d",nextSeqA);
      A_State=0;
 }
 else if(nextSeqA<baseA+winSizeA){
      A_State=1;
 }
 }
void sendPktAtoB(){
printf("SEND PKT A TO B \n");
printf("LATEST SEND INDEX::%d\n",indexA);
printf("SEND BASE::%d\n",baseA);
printf("NEXT AVAL SEQ NO::%d\n",nextSeqA);
printf("LAST INDEX::%d\n",lastIndex);
int checksumA;
int i;
A_State=0;
if(!timeout){
for(i=lastIndex;i<baseA+winSizeA;i++){
    if(timeout){
        break;
        }
   if(buffmsgsA[i].passType){//msg in buffer
		buffmsgsA[i].pkt_AtoB.acknum=0;
		buffmsgsA[i].pkt_AtoB.seqnum=nextSeqA;
		checksumA=createCheckSumA(buffmsgsA[i].pkt_AtoB);
		if(i==baseA){
		printf("STARTING TIMER ::PLACE 1 \n");
		starttimer(0,rtt);
		}	
		buffmsgsA[i].pkt_AtoB.checksum=checksumA;
		buffmsgsA[i].sent=1;
		printf("Sending msg from buffer::%s,Seq No::%d\n",buffmsgsA[i].pkt_AtoB.payload,buffmsgsA[i].pkt_AtoB.seqnum);
		tolayer3(0,buffmsgsA[i].pkt_AtoB);	
		indexA++;
		nextSeqA++;
		if(i==nextSeqA){
		 printf("STARTING TIMER ::PLACE 2 \n");
		 starttimer(0,rtt);
		}
		printf("NEXT SEQ A::%d \n",nextSeqA);
	}
	else{
		 break;
	}
}
}
if(nextSeqA==baseA+winSizeA){
      printf("WINDOW FULL ::NEXT SEQ A ::%d",nextSeqA);
      A_State=0;
 }
 else if(nextSeqA<baseA+winSizeA){
      A_State=1;
 }
}
void A_output(message)
  struct msg message;
{
if(A_State){
	printf("IN A_OUTPUT data from above::%s\n",message.data);
	strcpy(buffmsgsA[buffMsgNo].pkt_AtoB.payload,message.data);
	buffmsgsA[buffMsgNo].passType=1;
	indexA=buffMsgNo;
	lastIndex=indexA;
	sendPktAtoB();
	buffMsgNo++;
}
else{
	//window full,buffer message
	printf("IN A_OUTPUT,WINDOW FULL,BUFFER MESSAGE::%s\n",message.data);
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
    if(checksum_BtoA==checkSumVal && ack_BToA>=baseA && ack_BToA<indexA){
    baseA=ack_BToA+1;
    lastIndex=indexA;
    printf("BASE A::%d & NEXT SEQ NO::%d \n",baseA,nextSeqA);
    if(baseA==nextSeqA){
       stoptimer(0);
      }
    else{
     printf("Restarting timer after receiving ack \n"); 
     stoptimer(0);
     printf("STARTING TIMER ::PLACE 3 \n");
     starttimer(0,rtt);
     }
     sendPktAtoB();
     printf("NEW BASE::%d\n",baseA);
     printf("LAST AND LATEST INDEX::%d\n",indexA);
    }
    else{
    printf("DUP/OLD ACK DID NOTHING \n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
 printf("STARTING TIMER ::PLACE 4 \n");
 starttimer(0,rtt);
 printf("TIMEOUT OCCURED::STOPPING TIMER::RETRANSMISSION %s \n",buffmsgsA[baseA].pkt_AtoB.payload);
 printf("BASE:%d\n",baseA);
 sendPacketsOnTimeOut();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
 nextSeqA=0;
 baseA=0;
 A_State=1;
 buffMsgNo=0;
 winSizeA=getwinsize();
 rtt=27.0;
 lastIndex=0;
 timeout=0;
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
seqAtoB=packet.seqnum;
checkSumAtoB=packet.checksum;
expCheckSumAtoB=createCheckSumA(packet);
printf("Packet Arrived at B::%s\n",packet.payload);
printf("Expected checksum::%d and Arrived checksum::%d\n",expCheckSumAtoB,checkSumAtoB);
printf("EXP Seq No::%d Recv Seq No::%d\n",expSeqB,seqAtoB);
if(checkSumAtoB==expCheckSumAtoB && seqAtoB==expSeqB){
 printf("B::DATA DELIVERED TO UPPER LAYER::%s\n",packet.payload);
 tolayer5(1,packet.payload);
 pkt_BtoA.acknum=expSeqB;
 pkt_BtoA.seqnum=seqFieldB;
 pkt_BtoA.payload[0]=dataB;
 pkt_BtoA.checksum=expSeqB+seqFieldB+(int)(pkt_BtoA.payload[0]);
 prevExpSeqB=expSeqB;
 expSeqB++;
 printf("B to A ACK::%d\n",pkt_BtoA.acknum); 
}
else {//packet is corrupted or out of order//send prev ack
 printf("DUP CORR DATA NOT DELIVERED TO UPPER LAYER:: PREV ACK::%d\n",prevExpSeqB);
 pkt_BtoA.acknum=prevExpSeqB;
 pkt_BtoA.seqnum=seqFieldB;
 pkt_BtoA.payload[0]=dataB;
 pkt_BtoA.checksum=prevExpSeqB+seqFieldB+(int)(pkt_BtoA.payload[0]);
 printf("B to A ACK::%d\n",pkt_BtoA.acknum);
}
 tolayer3(1,pkt_BtoA);
 
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
expSeqB=0;
seqFieldB=1;
prevExpSeqB=9999;
dataB='B';
}