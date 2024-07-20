#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
using namespace std;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    // proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    unsigned long int pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    // printf("rob_size:%lu "
    //         "iq_size:%lu "
    //         "width:%lu "
    //         "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    init_pipeline(params.rob_size, params.iq_size, params.width);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //     printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    bool stalled = 0;
	int sequence = 0;
    do {
		if (!stalled)
		{
			for (uint32_t k = 0; k < params.width; k++)
			{
				if (fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
				{
					init_instruction(k);

					params.Inst[k].destReg = params.Inst[k].dest = dest;
					params.Inst[k].srcReg1 = params.Inst[k].src1 = src1;
					params.Inst[k].srcReg2 = params.Inst[k].src2 = src2;
					params.Inst[k].type = op_type;
					params.Inst[k].pc = pc;
					params.Inst[k].sequence = sequence;
					if (params.Inst[k].sequence == 0)
					{
						params.ROB->head = -1;
						params.ROB->tail = -1;
						params.ROB->count = 0;
					}
					get_operation_type(params.Inst[k].type, k);
					sequence++;

				}

				else 
				{
					params.Inst[k].sequence = -1;
					params.Inst[k].destReg = -2;
					params.Inst[k].srcReg1 = -1;
					params.Inst[k].srcReg2 = -1;

				}
			}
		}
        
        Retire();
        Writeback();
        Execute();
        Issue();
        Dispatch();
        RegRead();
        Rename();
        Decode();
        Fetch();

        stalled = params.stall ? 1 : 0;

	} while (Advance_Cycle() || (params.ROB->head!=-1));

    double IPC = (double)(sequence) / (double)(params.totalCount);

    cout << "# === Simulator Command =========" << endl;
    cout << "#";
    for (int i=0; i < argc; i++)
    {
        cout << " " << argv[i];
    }
    cout << endl;
	cout << "# === Processor Configuration ===" << endl;
	cout << "# ROB_SIZE = " << params.rob_size << endl;
	cout << "# IQ_SIZE  = " << params.iq_size << endl;
	cout << "# WIDTH    = " << params.width << endl;
	cout << "# === Simulation Results ========" << endl;
	cout << "# Dynamic Instruction Count    = " << sequence << endl;
	cout << "# Cycles                       = " << params.totalCount << endl;
	cout << "# Instructions Per Cycle (IPC) = " << setprecision(2) << fixed << IPC << endl;

    return 0;
}


void init_pipeline(int robSize, int iqSize, int width)
{
	
	// Initialize registers
	params.retireD1 = new int[width * 5];
	params.retireD2 = new int[width * 5];
	params.wbDest = new int[width * 5];
	params.RMT = new RMT[67];
	params.ROB = new ROBQueue[robSize];
	params.Inst = new inst[width];
	params.DE = new Decode_[width];
	params.RN = new Rename_[width];
	params.RR = new RegRead_[width];
	params.DI = new Dispatch_[width];
	params.IQ = new IssueQueue_[iqSize];
	params.WB = new Writeback_[width];
	exe.EX = new execute_list_*[width];
	for (int i = 0; i < width; i++)
	{
		exe.EX[i] = new execute_list_[width];
		exe.EX[i]->Inst = new inst[width * 5];
	}

	for (int i = 0; i < 67; i++)
	{
		params.RMT[i].valid = 0;
	}
	for (int i = 0; i < width * 5; i++)
	{
		params.retireD1[i] = -2;
		params.retireD2[i] = -2;
		params.wbDest[i] = -2;
	}

	// Initialize instuction at registers
	params.DE->Inst = new inst[width];
	params.RN->Inst = new inst[width];
	params.RR->Inst = new inst[width];
	params.DI->Inst = new inst[width];
	params.IQ->Inst = new inst[width * 5];
	params.IQ->IQ = new IQ_block[iqSize];
	params.IQ->IQ_sorted = new IQ_block[iqSize];
	params.WB->Inst = new inst[width * 5];
	params.IQ->tempInst = new inst[width * 5];
	params.ROB->ROB = new ROB[robSize];

	// Initialize status of registers
	params.currentCycle = 0;
	params.change = 1;
	params.stall = 0;
	params.totalCount = 0;
	params.DI->valid = 0;
	params.DI->state = 1;
	params.DI->status = 0;
	params.RN->valid = 0;
	params.RN->status = 0;
	params.RN->state = 1;
	params.RR->valid = 0;
	params.RR->status = 0;
	params.RR->state = 1;
	params.DE->valid = 0;
	params.DE->status = 0;
	params.DE->state = 1;
	params.WB->WB_valid = 0;
	params.WB->state = params.WB->WB_valid;
	params.WB->valid = new int[width * 5];
	params.ROB->valid = 0;

	for (int i = 0; i <= (robSize - 1); i++)
	{
		params.ROB->ROB[i].valid = 0;
	}

	params.IQ->IQValid = 0;
	params.IQ->state = 1;

	for (int i = 0; i < iqSize; i++)
	{
		params.IQ->IQ[i].valid = 0;
		params.IQ->IQ[i].age = 0;
	}

	for (int i = 0; i < width; i++)
	{
		exe.EX[i]->EX_valid = 0;

		for (int j = 0; j < 5; j++)
		{
			exe.EX[i]->wakeup[j] = -2;
			exe.EX[i]->valid[j] = 0;
		}
	}

	for (int i = 0; i < width*5; i++)
	{
		params.WB->valid[i] = 0;
	}
}

void init_instruction(int k)
{
	// Initialize cycle for registers
	params.Inst[k].initRT = 0;
	params.Inst[k].nextRT = 0;
	params.Inst[k].initWB = 0;
	params.Inst[k].nextWB = 0;
	params.Inst[k].initEX = 0;
	params.Inst[k].nextEX = 0;
	params.Inst[k].initIS = 0;
	params.Inst[k].nextIS = 0;
	params.Inst[k].initDI = 0;
	params.Inst[k].nextDI = 0;
	params.Inst[k].initRR = 0;
	params.Inst[k].nextRR = 0;
	params.Inst[k].initRN = 0;
	params.Inst[k].nextRN = 0;
	params.Inst[k].initDE = 0;
	params.Inst[k].nextDE = 0;
	params.Inst[k].initFE = 0;
	params.Inst[k].nextFE = 0;
	params.Inst[k].src1Ready = 0;
	params.Inst[k].src2Ready = 0;
}

void get_operation_type(int type, int k)
{
	// Acquire operation type 
	// Type 0 has a latency of 1 cycle, Type 1 has a latency of 2 cycles, and Type 2has a latency of 5 cycle
	if (type == 0)
	{
		params.Inst[k].num_cycle = 1;
	}
	else if (type == 1)
	{
		params.Inst[k].num_cycle = 2;
	}
	else if (type == 2)
	{
		params.Inst[k].num_cycle = 5;
	}
}

void ad_to_DE(inst *Inst)
{
	// Advance whole bundle of instruction from fetch to decode
	for (uint32_t i = 0; i < params.width; i++)
	{
		params.DE->Inst[i] = Inst[i];
	}

	params.DE->status = 1;
	params.DE->valid = 1;

	return;
}

void ad_to_RN(inst *Inst)
{
	// Advance whole bundle of instruction from decode to rename
	for (uint32_t i = 0; i < params.width; i++)
	{
		params.RN->Inst[i] = Inst[i];
	}

	params.RN->valid = 1;
	params.RN->status = 1;
 
	return;
}

int ROB_enqueue(ROB *data)
{
	// If rob is not empty
	if (((params.ROB->head == 0) && ((unsigned)params.ROB->tail == (params.rob_size - 1))) || (params.ROB->head == params.ROB->tail + 1))
	{
		return 0;
	}
	// If rob is empty
	else if ((params.ROB->head == -1) && (params.ROB->tail == -1))
	{
		// Initialize head and tail
		params.ROB->head = 0;
		params.ROB->tail = 0;
		params.ROB->ROB[params.ROB->tail] = *data;
		params.ROB->ROB[params.ROB->tail].valid = 1;
		params.ROB->count++;
	}
	else if ((unsigned)params.ROB->tail == (params.rob_size - 1))
	{
		// Apply FIFO if rob is full
		params.ROB->tail = 0;
		params.ROB->ROB[params.ROB->tail] = *data;
		params.ROB->ROB[params.ROB->tail].valid = 1;
		params.ROB->count++;
	}
	else
	{
		// store data
		params.ROB->tail++;
		params.ROB->ROB[params.ROB->tail] = *data;
		params.ROB->ROB[params.ROB->tail].valid = 1;
		params.ROB->count++;
	}

	return 1;
}

void ad_to_RR(inst *Inst)
{
	// Advance whole bundle of instruction from rename to regread
	for (uint32_t i = 0; i < params.width; i++)
	{
		params.RR->Inst[i] = Inst[i];
	}

	params.RR->status = 1;
	params.RR->valid = 1;

	return;
}

void ad_to_DI(inst * Inst)
{
	// Advance whole bundle of instruction from register read stage to dispatch
	for (uint32_t i = 0; i < params.width; i++)
	{
		params.DI->Inst[i] = Inst[i];
	}

	params.DI->status = 1;
	params.DI->valid = 1;

	return;
}

int IQ_status()
{
	// Check for empty space in issue queue 
	int count = 0;
	for (uint32_t i = 0; i < params.iq_size; i++)
	{
		if (params.IQ->IQ[i].valid == 0)
			count++;
	}
	if ((unsigned)count >= params.width)
		return 0;
	else 
        return 1;
}

void ad_to_IQ(inst * Inst)
{
	// Advance whole bundle of instruction from dispatch to issue
	int counter = params.width;
	int i = 0;
	params.IQ->IQValid = 1;

	while (counter != 0)
	{
		for (uint32_t j = 0; j < params.iq_size; j++)
		{
			if (params.IQ->IQ[j].valid == 0)
			{
				params.IQ->cage++;
				params.IQ->IQ[j].valid = 1;
				params.IQ->IQ[j].age = params.IQ->cage;
				params.IQ->IQ[j].Inst.src1Ready = Inst[i].src1Ready;
				params.IQ->IQ[j].Inst.src2Ready = Inst[i].src2Ready;

				params.IQ->IQ[j].Inst = *(Inst + i);
				counter--;
				i++;
			}

			if (counter == 0)
				break;
		}

	}

	return;
}

void ROB_dequeue()
{
	// Remove instructions that have been moved to ARF
	if (params.ROB->head != -1 && params.ROB->tail != -1) 
	{
		if (params.ROB->head == params.ROB->tail) {
			params.ROB->ROB[params.ROB->head].valid = 0;
			params.ROB->ROB[params.ROB->head].ready = 0;
			params.ROB->head = -1;
			params.ROB->tail = -1;
			params.ROB->count--;
		}
		else if ((unsigned)params.ROB->head == (params.rob_size - 1)) {
			params.ROB->ROB[params.ROB->head].valid = 0;
			params.ROB->ROB[params.ROB->head].ready = 0;
			params.ROB->head = 0;
			params.ROB->count--;
		}
		else {

			params.ROB->ROB[params.ROB->head].valid = 0;
			params.ROB->ROB[params.ROB->head].ready = 0;
			params.ROB->head++;
			params.ROB->count--;
		}
	}

	return;
}

void ad_to_WB(inst *Inst)
{
	for (int i = params.width * 5 - 2; i >= 0; i--)
	{
		params.WB->Inst[i + 1] = params.WB->Inst[i];	
		params.WB->valid[i + 1] = params.WB->valid[i];
	}

	params.WB->Inst[0] = *Inst;
	params.WB->valid[0] = 1;

	params.WB->WB_valid = 1;

	return;
}

void IQ_sorting()
{
	// Sort issue queue in incremental order
	IQ_block temp;
	for (uint32_t j = 0; j < params.iq_size; j++)
	{
		params.IQ->IQ_sorted[j] = params.IQ->IQ[j];
	}

	for (uint32_t j = 0; j < params.iq_size - 1; j++)
	{
		for (uint32_t i = 0; i < params.iq_size - j - 1; i++)
		{
			if (params.IQ->IQ_sorted[i + 1].age < params.IQ->IQ_sorted[i].age)
			{
				temp = params.IQ->IQ_sorted[i];
				params.IQ->IQ_sorted[i] = params.IQ->IQ_sorted[i + 1];
				params.IQ->IQ_sorted[i + 1] = temp;
			}
		}
	}

	for (uint32_t j = 0; j < params.iq_size; j++)
	{
		params.IQ->IQ[j] = params.IQ->IQ_sorted[j];
	}

	return;
}

int pip_empty()
{
	// Check if pipline is empty
    if (((params.ROB->head == 0) && ((unsigned)params.ROB->tail == (params.rob_size - 1))) || (params.ROB->head == params.ROB->tail + 1)) 
    	return 1;

    if (params.ROB->head == -1 && params.ROB->tail == -1) 
    	return 0;

    if (params.ROB->tail > params.ROB->head)
    {
    	if ((params.rob_size - 1) - params.ROB->tail + params.ROB->head >= params.width)
    		return 0;
    	else
    		return 1;
    }

    if (params.ROB->tail < params.ROB->head)
    {
    	if ((unsigned)(params.ROB->head - params.ROB->tail - 1) >= params.width)
    		return 0;
    	else
    		return 1;
    }
	return 0;
}

int Advance_Cycle()
{
	params.currentCycle++;

	if (params.currentCycle > 2 && pip_empty() == 0)
		return 0;
	else
		return 1;
}

void Retire()
{
	int counter = 0;

	// Check if retire is valid
	if (params.ROB->valid == 1)
	{
		for (uint32_t i = 0; i < (params.rob_size - 1); i++)
		{
			if (params.ROB->ROB[params.ROB->head].valid == 1 && params.ROB->ROB[params.ROB->head].ready == 1)
			{
				// Retire if it's valid and ready
				params.ROB->ROB[params.ROB->head].Inst.nextRT = params.currentCycle + 1 - params.ROB->ROB[params.ROB->head].Inst.initRT;
				counter++;

				// Upate ARF if RMT has latest tag
				if (params.ROB->ROB[params.ROB->head].Inst.robTag == params.RMT[params.ROB->ROB[params.ROB->head].Inst.dest].tag)
				{
					params.RMT[params.ROB->ROB[params.ROB->head].Inst.dest].valid = 0;
				}

                // Print data of the instruction
				if (params.ROB->ROB[params.ROB->head].Inst.sequence != -1)
				{
                    cout
                        << params.ROB->ROB[params.ROB->head].Inst.sequence << " "
                        << "fu{" << params.ROB->ROB[params.ROB->head].Inst.type << "} "
                        << "src{" << params.ROB->ROB[params.ROB->head].Inst.src1 << "," << params.ROB->ROB[params.ROB->head].Inst.src2 << "} "
                        << "dst{" << params.ROB->ROB[params.ROB->head].Inst.dest << "} "
                        << "FE{" << params.ROB->ROB[params.ROB->head].Inst.initFE << "," << params.ROB->ROB[params.ROB->head].Inst.nextFE << "} "
                        << "DE{" << params.ROB->ROB[params.ROB->head].Inst.initDE << "," << params.ROB->ROB[params.ROB->head].Inst.nextDE << "} "
                        << "RN{" << params.ROB->ROB[params.ROB->head].Inst.initRN << "," << params.ROB->ROB[params.ROB->head].Inst.nextRN << "} "
                        << "RR{" << params.ROB->ROB[params.ROB->head].Inst.initRR << "," << params.ROB->ROB[params.ROB->head].Inst.nextRR << "} "
                        << "DI{" << params.ROB->ROB[params.ROB->head].Inst.initDI << "," << params.ROB->ROB[params.ROB->head].Inst.nextDI << "} "
                        << "IS{" << params.ROB->ROB[params.ROB->head].Inst.initIS << "," << params.ROB->ROB[params.ROB->head].Inst.nextIS << "} "
                        << "EX{" << params.ROB->ROB[params.ROB->head].Inst.initEX << "," << params.ROB->ROB[params.ROB->head].Inst.nextEX << "} "
                        << "WB{" << params.ROB->ROB[params.ROB->head].Inst.initWB << "," << params.ROB->ROB[params.ROB->head].Inst.nextWB << "} "
                        << "RT{" << params.ROB->ROB[params.ROB->head].Inst.initRT << "," << params.ROB->ROB[params.ROB->head].Inst.nextRT << "} "
                        << endl;

                    params.totalCount = params.ROB->ROB[params.ROB->head].Inst.initRT + params.ROB->ROB[params.ROB->head].Inst.nextRT;
				}

				ROB_dequeue();
			}

			if ((unsigned)counter == params.width)
				break;
		}
	}

}

void Writeback()
{
	params.change = params.change * (-1);

	if (params.WB->WB_valid)
	{
		// For each instruction in WB, mark the instruction as “ready” in its entry in the ROB
		for (uint32_t i = 0; i < params.width * 5; i++)
		{
			if (params.WB->valid[i] == 1)
			{
				// Send the data to retire if valid
				params.WB->valid[i] = 0;
				params.WB->Inst[i].initRT = params.currentCycle + 1;
				params.WB->Inst[i].nextWB = params.currentCycle + 1 - params.WB->Inst[i].initWB;

				if (params.change == 1)
				{
					params.retireD1[i] = params.WB->Inst[i].destReg;
				}

				if (params.change == -1)
				{
					params.retireD2[i] = params.WB->Inst[i].destReg;
				}

				params.wbDest[i] = params.WB->Inst[i].destReg;

				params.ROB->valid = 1;
				params.ROB->ROB[params.WB->Inst[i].robTag].valid = 1;
				params.ROB->ROB[params.WB->Inst[i].robTag].ready = 1;
				params.ROB->ROB[params.WB->Inst[i].robTag].Inst = params.WB->Inst[i];
			}
			else 
			{
				params.wbDest[i] = -2;
			}
		}
	}
}

void Execute()
{
	if (exe.EX[0]->EX_valid == 1)
	{
		for (uint32_t k = 0; k < params.width; k++)
		{
			for (int i = 0; i < 5; i++)
			{
				if (exe.EX[k]->EX_valid == 1)
				{
					if (exe.EX[k]->valid[i] == 1)
					{	
						// Remove the instruction from the execute_list
						exe.EX[k]->Inst[i].num_cycle--;
						if (exe.EX[k]->Inst[i].num_cycle== 0) 
						{
							exe.EX[k]->valid[i] = 0;
							exe.EX[k]->wakeup[i] = exe.EX[k]->Inst[i].destReg;
							// Calculate cycles
							exe.EX[k]->Inst[i].initWB = params.currentCycle + 1;
							exe.EX[k]->Inst[i].nextEX = params.currentCycle + 1 - exe.EX[k]->Inst[i].initEX;
							// Advance the instruction to WB
							ad_to_WB(&(exe.EX[k]->Inst[i]));
						}
						// Wakeup dependent instructions
						else exe.EX[k]->wakeup[i] = -2;
					}
					// Wakeup dependent instructions
					else exe.EX[k]->wakeup[i] = -2;
				}
			}
		}
	}
}

void Issue()
{
	// Sort IQ at incremental order
    IQ_sorting();

    int count = 0;
    int p = 0;

	if (params.IQ->state == 1)
	{
		for (uint32_t i = 0; i < params.iq_size; i++)
		{
			if (params.IQ->IQ[i].valid == 1)
			{
				for (uint32_t j = 0; j < params.width; j++)
				{
					for (int k = 0; k < 5; k++)
					{
						// Change to ready if src1 has data
						if ((exe.EX[j]->wakeup[k] == params.IQ->IQ[i].Inst.srcReg1) || (params.IQ->IQ[i].Inst.srcReg1 == -1))
						{
							params.IQ->IQ[i].Inst.src1Ready = 1;
						}

						// Change to ready if src2 has data
						if ((exe.EX[j]->wakeup[k] == params.IQ->IQ[i].Inst.srcReg2) || (params.IQ->IQ[i].Inst.srcReg2 == -1))
						{
							params.IQ->IQ[i].Inst.src2Ready = 1;
						}
					}
				}
			
				if (params.ROB->ROB[params.IQ->IQ[i].Inst.srcReg1].ready == 1)
				{
					params.IQ->IQ[i].Inst.src1Ready = 1;
				}
		
				if (params.ROB->ROB[params.IQ->IQ[i].Inst.srcReg2].ready == 1)
				{
					params.IQ->IQ[i].Inst.src2Ready = 1;
				}
			}

			for (uint32_t k = 0; k < params.width * 5; k++)
			{
				if (params.wbDest[k] == params.IQ->IQ[i].Inst.srcReg1)
				{
					params.IQ->IQ[i].Inst.src1Ready = 1;
				}
			
				if (params.wbDest[k] == params.IQ->IQ[i].Inst.srcReg2)
				{
					params.IQ->IQ[i].Inst.src2Ready = 1;
				}
			}
		}

		// Remove the instruction from the IQ
		for (uint32_t i = 0; i < params.iq_size; i++)
		{
			if ((params.IQ->IQ[i].valid == 1) && (params.IQ->IQ[i].Inst.src1Ready == 1) && (params.IQ->IQ[i].Inst.src2Ready == 1))
			{

				params.IQ->tempInst[p] = params.IQ->IQ[i].Inst;
				p++;
				count++;
				params.IQ->IQ[i].valid = 0;
			}

			if ((unsigned)count == params.width)
				break;
		}

		// Advance the instruction to the execute_list
		for (int i = 0; i < count; i++)
		{
			params.IQ->tempInst[i].initEX = params.currentCycle + 1;
			params.IQ->tempInst[i].nextIS = params.currentCycle + 1 - params.IQ->tempInst[i].initIS;
			for (int j = 3; j >= 0; j--)
			{
				exe.EX[i]->Inst[j + 1] = exe.EX[i]->Inst[j];
				exe.EX[i]->valid[j + 1] = exe.EX[i]->valid[j];
			}
            //Advance to execute stage
			exe.EX[i]->Inst[0] = params.IQ->tempInst[i];
			exe.EX[i]->valid[0] = 1;
			exe.EX[i]->EX_valid = 1;
		}


	}
}

void Dispatch()
{	
	// If DI contains a dispatch bundle
	if ((params.DI->state == 1) && (params.DI->status == 1))
	{
        for (uint32_t i = 0; i < params.width; i++)
        {
            if (params.DI->Inst[i].srcReg1 != -1)
            {
                for (uint32_t j = 0; j < params.width; j++)
                {
                    for (int k = 0; k < 5; k++)
                    {
                        if (exe.EX[j]->wakeup[k] == params.DI->Inst[i].srcReg1)
                        {
                            params.DI->Inst[i].src1Ready = 1;
                        }
                    }
                }
            }

            if (params.DI->Inst[i].srcReg2 != -1)
            {
                for (uint32_t j = 0; j < params.width; j++)
                {
                    for (int k = 0; k < 5; k++)
                    {
                        if (exe.EX[j]->wakeup[k] == params.DI->Inst[i].srcReg2)
                        {
                            params.DI->Inst[i].src2Ready = 1;
                        }
                    }
                }
            }

            if (params.ROB->ROB[params.DI->Inst[i].srcReg1].ready == 1)
            {
                params.DI->Inst[i].src1Ready = 1;
            }

            if (params.ROB->ROB[params.DI->Inst[i].srcReg2].ready == 1)
            {
                params.DI->Inst[i].src2Ready = 1;
            }

			// Wakeup if src has data from writeback
            for (uint32_t k = 0; k < params.width * 5; k++)
            {
                if (params.wbDest[k] == params.DI->Inst[i].srcReg1)
                {
                    params.DI->Inst[i].src1Ready = 1;
                }
            }

            for (uint32_t k = 0; k < params.width * 5; k++)
            {
                if (params.wbDest[k] == params.DI->Inst[i].srcReg2)
                {
                    params.DI->Inst[i].src2Ready = 1;
                }
            }
        }

		// If the number of free IQ entries is greater than or equal to the size of the dispatch bundle in DI
        if (IQ_status() == 0)
        {	
			// Dispatch all instructions from DI to the IQ
            for (uint32_t i = 0; i < params.width; i++)
            {
                params.DI->Inst[i].initIS = params.currentCycle + 1;
                params.DI->Inst[i].nextDI = params.currentCycle + 1 - params.DI->Inst[i].initDI;
            }
            ad_to_IQ(params.DI->Inst);
            params.DI->status = 0;
            params.DI->valid = 0;
        }
	}
}

void RegRead()
{
	if ((params.RR->state == 1) && (params.RR->status == 1))
	{
		// If RR contains a register-read bundle
        for (uint32_t i = 0; i < params.width; i++)
        {
            if (params.RR->Inst[i].srcReg1 == -1)
            {
                params.RR->Inst[i].src1Ready = 1;
            }
            else
            {
                if (params.ROB->ROB[params.RR->Inst[i].srcReg1].ready == 1)
                {
                    params.RR->Inst[i].src1Ready = 1;
                }
                
                for (uint32_t k = 0; k < params.width * 5; k++)
                {
                    if (params.change == -1)
                    {
                        if (params.RR->Inst[i].srcReg1 == params.retireD1[k])
                        {
                            params.RR->Inst[i].src1Ready = 1;
                        }
                    }
                    else if (params.change == 1)
                    {
                        if (params.RR->Inst[i].srcReg1 == params.retireD2[k])
                        {
                            params.RR->Inst[i].src1Ready = 1;
                        }
                    }

                    if (params.wbDest[k] == params.RR->Inst[i].srcReg1)
                    {
                        params.RR->Inst[i].src1Ready = 1;
                    }
                }

                for (uint32_t j = 0; j < params.width; j++)
                {
                    for (uint32_t k = 0; k < 5; k++)
                    {
                        if (exe.EX[j]->wakeup[k] == params.RR->Inst[i].srcReg1)
                        {
                            params.RR->Inst[i].src1Ready = 1;
                        }
  
                        if (exe.EX[j]->wakeup[k] == params.RR->Inst[i].srcReg1)
                        {
                            params.RR->Inst[i].src1Ready = 1;
                        }
                    }
                }

                if (params.ROB->ROB[params.RR->Inst[i].srcReg1].ready == 1)
                {
                    params.RR->Inst[i].src1Ready = 1;
                }
            }

            if (params.RR->Inst[i].srcReg2 == -1)
            {
                params.RR->Inst[i].src2Ready = 1;
            }
            else
            {
                if ((params.ROB->ROB[params.RR->Inst[i].srcReg2].ready == 1))
                {
                    params.RR->Inst[i].src2Ready = 1;
                }

                // Check wakeup at retire
                for (uint32_t k = 0; k < params.width * 5; k++)
                {
                    if (params.change == -1)
                    {
                        if (params.RR->Inst[i].srcReg2 == params.retireD2[k])
                        {
                            params.RR->Inst[i].src2Ready = 1;
                        }
                    }
                    else if (params.change == 1)
                    {
                        if (params.RR->Inst[i].srcReg2 == params.retireD1[k])
                        {
                            params.RR->Inst[i].src2Ready = 1;
                        }
                    }

                    // Check wakeup at writeback
                    if (params.wbDest[k] == params.RR->Inst[i].srcReg2)
                    {
                        params.RR->Inst[i].src2Ready = 1;
                    }
                }

                // Check wakeup at execute
                for (uint32_t j = 0; j < params.width; j++)
                {
                    for (int k = 0; k < 5; k++)
                    {
                        if (exe.EX[j]->wakeup[k] == params.RR->Inst[i].srcReg2)
                        {
                            params.RR->Inst[i].src2Ready = 1;
                        }
                    }
                }
			}
		}   

		// If DI is empty then process the register-read bundle and advance it from RR to DI
		if (params.DI->status == 0 || params.DI->state == 0)
		{
            //calculate teh cycle time in register read for the instruction
            for (uint32_t i = 0; i < params.width; i++)
            {
                params.RR->Inst[i].initDI = params.currentCycle + 1;
                params.RR->Inst[i].nextRR = params.currentCycle + 1 - params.RR->Inst[i].initRR;
            }

            ad_to_DI(params.RR->Inst);
            params.RR->status = 0;
		}
		// If DI is not empty then do nothing
	}
}


void Rename()
{
	// If RN contains a rename bundle
	if ((params.RN->state == 1) && (params.RN->status == 1))
	{
		// If either RR is empty and the ROB have enough free entries to accept the entire rename bundle		
        if ((params.RR->status == 0) && (pip_empty() == 0))
        {	
			// process the rename bundle and advance it from RN to RR.
            for (uint32_t i = 0; i < params.width; i++)
            {
                ROB *data;
                data = new ROB;
                data->ready = 0;
                data->valid = 1;
                data->Inst = *(params.RN->Inst + i);
                data->tag = (params.ROB->tail + 1);
                ROB_enqueue(data);

				// Rename its source registers
                if (params.RN->Inst[i].srcReg1 != -1)
                {
                    if (params.RMT[params.RN->Inst[i].src1].valid == 1)
                    {
                        params.RN->Inst[i].srcReg1 = params.RMT[params.RN->Inst[i].src1].tag;
                    }
                    else
                    {
                        params.RN->Inst[i].src1Ready = 1;
                    }

                }
                else
                {
                    params.RN->Inst[i].src1Ready = 1;
                }
                
                if (params.RN->Inst[i].srcReg2 != -1)
                {
                    if (params.RMT[params.RN->Inst[i].src2].valid == 1)
                    {
                        params.RN->Inst[i].srcReg2 = params.RMT[params.RN->Inst[i].src2].tag;
                    }
                    else
                    {
                        params.RN->Inst[i].src2Ready = 1;
                    }

                }
                else
                {
                    params.RN->Inst[i].src2Ready = 1;
                }

				// Rename its destination register
                if (params.RN->Inst[i].destReg != -1)
                {
                    params.RMT[params.RN->Inst[i].dest].tag = params.ROB->tail;
                    params.RMT[params.RN->Inst[i].dest].valid = 1;
                    params.RN->Inst[i].robTag = params.ROB->tail;

					if (params.RMT[params.RN->Inst[i].dest].valid == 1)
                    {
                        params.RN->Inst[i].destReg = params.RMT[params.RN->Inst[i].dest].tag;
                    }
                }
                else
                {
                    params.RN->Inst[i].robTag = params.ROB->tail;
                }
                
                if (params.ROB->ROB[params.RN->Inst[i].srcReg1].ready == 1)
                    params.RN->Inst[i].src1Ready = 1;
                if (params.ROB->ROB[params.RN->Inst[i].srcReg2].ready == 1)
                    params.RN->Inst[i].src2Ready = 1;
            }

            // Compute the cycle of instruction in rename
            for (uint32_t i = 0; i < params.width; i++)
            {
                params.RN->Inst[i].initRR = params.currentCycle + 1;
                params.RN->Inst[i].nextRN = params.currentCycle + 1 - params.RN->Inst[i].initRN;
            }
            // Advance from rename to regread
            ad_to_RR(params.RN->Inst);
            params.RN->status = 0;
        }
    }
}

void Decode()
{	
	// If DE contains a decode bundle
	if ((params.DE->state == 1) && (params.DE->status == 1))
	{
        params.RN->valid = 1;
		// If RN is empty then advance the decode bundle from DE to RN.
        if (params.RN->status == 0 && params.RN->valid == 1)
        {
            for (uint32_t i = 0; i < params.width; i++)
            {
                params.DE->Inst[i].initRN = params.currentCycle + 1;
                params.DE->Inst[i].nextDE = params.currentCycle + 1 - params.DE->Inst[i].initDE;
            }
            // Advance to rename
            ad_to_RN(params.DE->Inst);
            params.DE->status = 0;
        }
    }
}

void Fetch()
{
	int flag = 0;
	for (uint32_t i = 0; i < params.width; i++)
	{
		params.Inst[i].initFE = params.currentCycle;
	}

	// Check cycle of instruction
	if (params.currentCycle % 2 == 1) 
	{
		
		for (uint32_t i = 0; i < params.width * 5; i++)
		{
			params.retireD2[i] = -2;
		}
	}
	else
	{
		for (uint32_t i = 0; i < params.width * 5; i++)
		{
			params.retireD1[i] = -2;
		}

	}

	// If DE is not empty then fetch up to WIDTH instructions from the trace file into DE
	if (params.DE->status == 0)
	{
		params.stall = 0;

		for (uint32_t i = 0; i < params.width; i++)
		{
			//calculate the duration instruction stalled in fetch
			params.Inst[i].initDE = params.currentCycle + 1;
			params.Inst[i].nextFE = params.currentCycle + 1 - params.Inst[i].initFE;

			if (params.Inst[i].sequence == -1)
				flag = flag + 1;
		}
		//if decode is empty and valid forward the instruction bundle to decode stage
		if (flag == 0)
		{
			ad_to_DE(params.Inst);
		}
		else if ((unsigned)flag < params.width)
		{
			ad_to_DE(params.Inst);
		}
	}
	else if (params.DE->status == 1 && params.currentCycle > 2)
	{
		params.stall = 1;
	}
	else params.stall = 0;

	return;
}

