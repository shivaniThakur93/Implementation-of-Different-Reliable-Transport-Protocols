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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
#include<stddef.h>>
#define MAXPACKETS 1000
#define MSGSIZE 20
int seqA;
int expAckA;
float rtt;
int A_State;
char dataB;
struct pkt pkt_AtoB;
struct buffMsg{
 struct msg message;
 int passType;//1 indicates msg from A upper layer,odrwise 0
};
struct buffMsg buffmsgsA[MAXPACKETS];
int buffMsgNo;
int sendBufMsgNo;
struct msg prev_message;
int expSeqB;
struct pkt pkt_BtoA;
int checkTimer;
int expAckRecv;
int acknNumA;
int seqNumB;
void sendPktAtoB(struct msg message){
    printf("A to B MSG::%s \n",message.data);
    printf("A to B MSG Seq Num:%d\n",seqA);
    prev_message=message;
    A_State=0;
    pkt_AtoB.seqnum=seqA;
    pkt_AtoB.acknum=acknNumA;
    pkt_AtoB.checksum=seqA+acknNumA;
    int i;
    int len=strlen(message.data);
    if(len>20){
      len=20;
    }
    strcpy(pkt_AtoB.payload,message.data);
    for(i=0;i<len;i++){
      pkt_AtoB.checksum+=(int)(pkt_AtoB.payload[i]);
    }
    printf("A to B MSG CheckSum:%d\n",pkt_AtoB.checksum);
    expAckA=seqA;
    tolayer3(0,pkt_AtoB);
    expAckRecv=0;
    starttimer(0,rtt); 
    checkTimer=1;//timer started
    printf("Timer started::\n"); 
}
void stepsAfterAck(){
	  expAckRecv=1;
      A_State=1;
      seqA=flipNumber(expAckA);
      printf("Next seq no::%d\n",seqA);
      stoptimer(0);
      printf("Timer Stopped::\n"); 
      size_t len=strlen(buffmsgsA[sendBufMsgNo].message.data);
      printf("Length buff msg::%d & Msg is::%s & Msg No::%d",len,buffmsgsA[sendBufMsgNo].message.data,sendBufMsgNo);
      if(len>0 && buffmsgsA[sendBufMsgNo].passType==1){
      	printf("Sending buffered message \n");
        sendPktAtoB(buffmsgsA[sendBufMsgNo].message);   
        memset(buffmsgsA[sendBufMsgNo].message.data, '\0', sizeof buffmsgsA[sendBufMsgNo].message.data);
        sendBufMsgNo++;
        if(sendBufMsgNo>999){
        return -1;
        }
        printf("Buffered messages count::%d\n",sendBufMsgNo);
      }
}
void sendPktBtoA(struct pkt packetAtoB){
  struct pkt pkt_BtoA;

}

int flipNumber(int no){
  if(no==0){
    return 1;
  }
  else if(no==1){
    return 0;
  }
}
void A_output(struct msg message) 
 {
  //ack not awaited,layer 4 ready to send msgs
  if(A_State && buffmsgsA[sendBufMsgNo].passType==0){
    printf(" \n");
    sendPktAtoB(message);
  }
  else{
  	printf("BUFFERING MESSAGE::%s\n",message.data);
    strcpy(buffmsgsA[buffMsgNo].message.data,message.data);
    buffmsgsA[buffMsgNo].passType=1;
    buffMsgNo++;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    int checkSumVal;//expected checksum
    int ack_BToA=packet.acknum;
    int seq_BtoA=packet.seqnum;
    int checksum_BtoA=packet.checksum;
    char data_BtoA=packet.payload[0];
    checkSumVal=seq_BtoA+ack_BToA+(int)(data_BtoA);
    printf("Ack from B to A::%d and Expected ack at A::%d\n",ack_BToA,expAckA);
    printf("Seq from B to A::%d and Expected Seq at A::1\n",seq_BtoA);
    printf("CS from B to A::%d and Expected CS at A::%d data B to A::%c\n",checksum_BtoA,checkSumVal,data_BtoA);
    if(ack_BToA==expAckA && checksum_BtoA==checkSumVal){
    stepsAfterAck();
    }
    else if(expAckRecv){
	stepsAfterAck();
	}
    else{
    printf("CheckTimer val::%d\n",checkTimer);
      if(checkTimer){
            stoptimer(0);
      }
      printf("RETRANSMISSION\n");
      printf("Fast retransmit:Message %s\n",prev_message.data);
      sendPktAtoB(prev_message);
    }   
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  checkTimer=0;//timer stopped
  printf("RETRANSMISSION\n");
  printf("Timeout Occured : Prev Msg::%s\n",prev_message.data);
  sendPktAtoB(prev_message);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  seqA=0;
  expAckA=0;
  A_State=1;
  acknNumA=0;
  rtt=10.0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input( struct pkt packet)
{
  //deliver arrived pck to the layer 5 and send ack packet to layer 3
  printf("Packet Arrived at B\n");
  struct pkt pkt_BtoA;
  char dataAtoB[20];
  int seqAtoB;
  int ackAtoB;
  int checksumAtoB;//actual checksum
  int checkSumVal;//expected checksum
  size_t len=strlen(packet.payload);
  if(len>20){
    len=20;
  }
  int i;
  int dupAck=0;
  checksumAtoB=packet.checksum;
  seqAtoB=packet.seqnum;
  ackAtoB=packet.acknum;
  checkSumVal=seqAtoB+ackAtoB;
  //check for packet : corrupted or not
  strcpy(dataAtoB,packet.payload);
  for(i=0;i<len;i++){
    checkSumVal+=(int)(dataAtoB[i]);
  }
  //packet not corrupted and exp seq no received
  printf("CS at B::%d CS sent by A::%d seqAtoB::%d expSeqB::%d data::%s\n",checkSumVal,checksumAtoB,seqAtoB,expSeqB,dataAtoB);
  if(checkSumVal==checksumAtoB && seqAtoB==expSeqB){
    printf("B::DATA DELIVERED TO UPPER LAYER::%s\n",dataAtoB);
    tolayer5(1,dataAtoB);
    pkt_BtoA.acknum=expSeqB;
    pkt_BtoA.seqnum=seqNumB;
    pkt_BtoA.payload[0]=dataB;
    pkt_BtoA.checksum=expSeqB+seqNumB+(int)(pkt_BtoA.payload[0]);
    expSeqB=flipNumber(expSeqB);
  }
  //either packet corrupted or duplicate:send ack for last correct pkt received
  else{
  	printf("B::DUPLICATE OR CORRUPTED PACKET NOT DELIVERED TO UPPER LAYER \n");
    dupAck=flipNumber(expSeqB);
    pkt_BtoA.acknum=dupAck;
    pkt_BtoA.seqnum=seqNumB;
    pkt_BtoA.payload[0]=dataB;
    pkt_BtoA.checksum=dupAck+seqNumB+(int)(pkt_BtoA.payload[0]);
  }
  printf("B to A ACK::%d\n",pkt_BtoA.acknum);
  tolayer3(1,pkt_BtoA);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  expSeqB=0;
  seqNumB=1;
  dataB='B';
}
