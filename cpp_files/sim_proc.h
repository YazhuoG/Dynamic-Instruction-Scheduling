#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <stdlib.h>


typedef struct {
	int sequence;
	unsigned int pc;
	int type;

	int dest;
	int src1;
	int src2;

	int destReg;
	int srcReg1;
	int srcReg2;

	int src1Ready;
	int src2Ready;

    int initRT;
	int nextRT;
    int initWB;
	int nextWB;
    int initEX;
	int nextEX;
    int initIS;
	int nextIS;
    int initDI;
	int nextDI;
    int initRR;
	int nextRR;
    int initRN;
	int nextRN;
    int initDE;
	int nextDE;
	int initFE;
	int nextFE;
	
	int robTag;
	int num_cycle;

} inst;

typedef struct IQ_block{
	int valid;
	long int age;
	inst Inst;

} IQ_block;

typedef struct RMT{
	int valid;
	int tag;

} RMT_;

typedef struct ROB{
	int valid;
	int tag;
	int ready;
	inst Inst;

} ROB_;

typedef struct Writeback{
	int width;
	int WB_valid;
	int *valid;	
	int state;
	inst *Inst;

} Writeback_;

typedef struct ROBQueue{
	int head, tail, size, count;
	int valid;
	ROB_ *ROB;

} ROBQueue;

typedef struct Decode{
	int valid;
	int status;
	bool state;
	int width;
	inst *Inst;

} Decode_;

typedef struct Rename{
	int valid;
	int status;
	int state;
	int width;
	inst *Inst;

} Rename_;

typedef struct RegRead{
	int valid;
	int status;
	int state;
	int width;
	inst *Inst;

} RegRead_;

typedef struct Dispatch{
	int valid;
	int status;
	int state;
	int width;
 	inst *Inst;

} Dispatch_;

typedef struct IssueQueue{
	int size;
	int width;
	int IQValid;
	long int cage;
	int state;
	IQ_block *IQ;
	IQ_block *IQ_sorted;
	inst *tempInst;
	inst *Inst;

} IssueQueue_;


typedef struct execute_list {
	int width;
	int EX_valid;
	int valid[5];
	int wakeup[5];
	inst *Inst;

} execute_list_;

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
	int change;
	int stall;
	int totalCount;
	int currentCycle;
	int *retireD1;
	int *retireD2;
	int *wbDest;
	RMT_ *RMT;
	ROBQueue *ROB;
	Writeback_ *WB;
    IssueQueue_ *IQ;
    Dispatch_ *DI;
    RegRead_ *RR;
    Rename_ *RN;
    Decode_ *DE;
	inst *Inst;
	
}proc_params;

typedef struct execute{
	execute_list_ **EX;
} execute;

proc_params params;
execute exe;

// Put additional data structures here as per your requirement

void init_pipeline(int, int, int);
void init_instruction(int);
void get_operation_type(int, int);
void ad_to_DE(inst *);
void ad_to_RN(inst *);
int ROB_enqueue(ROB *);
void ad_to_RR(inst *);
void ad_to_DI(inst *);
int IQ_status();
void ad_to_IQ(inst *);
void ROB_dequeue();
void ad_to_WB(inst *);
void IQ_sorting();
int pip_empty();
int Advance_Cycle();
void Retire();
void Writeback();
void Execute();
void Issue();
void Dispatch();
void RegRead();
void Rename();
void Decode();
void Fetch();

#endif
