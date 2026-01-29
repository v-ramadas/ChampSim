/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs
 *  and could serve as the starting point for developing your first PIN tool
 */

#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "../../inc/trace_instruction.h"
#include "pin.H"

using trace_instr_format_t = input_instr;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 instrCount = 0;

std::ofstream outfile;

trace_instr_format_t curr_instr;

const CHAR * ROI_BEGIN = "__parsec_roi_begin";
const CHAR * ROI_END = "__parsec_roi_end";

FILE * trace;
ADDRINT ROIStartAddress = 0x0;
ADDRINT ROIEndAddress = 0xffffffffffffffff;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "champsim.trace", "specify file name for Champsim tracer output");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "1000000", "How many instructions to trace");

KNOB<UINT64> KnobROIStartAddress(KNOB_MODE_WRITEONCE, "pintool", "roiStart", "0", "Offset of the ROI function within the text segment");

KNOB<UINT64> KnobROIEndAddress(KNOB_MODE_WRITEONCE, "pintool", "roiEnd", "0x1000", "Offset of the ROI function within the text segment");

KNOB<std::string> KnobBMKName(KNOB_MODE_WRITEONCE, "pintool", "bmkName", "stream", "Name of the benchmark to be profiled.");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
  std::cerr << "This tool creates a register and memory access trace" << std::endl
            << "Specify the output trace file with -o" << std::endl
            << "Specify the number of instructions to skip before tracing with -s" << std::endl
            << "Specify the number of instructions to trace with -t" << std::endl
            << std::endl;

  std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;

  return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void ResetCurrentInstruction(VOID* ip)
{
  curr_instr = {};
  curr_instr.ip = (unsigned long long int)ip;
}

BOOL ShouldWrite(void* ip)
{
  ADDRINT addr = (ADDRINT)ip;
  bool shouldWrite = false;

  if (addr >= ROIStartAddress && addr <= ROIEndAddress) {// && addr <= ROIEndAddress) {
    ++instrCount;
    //std::cerr << "Addr " << std::hex << addr << " ROIStartAddress " << ROIStartAddress << " ROIEndAddress " << ROIEndAddress << std::dec << std::endl;
    shouldWrite = ((instrCount > KnobSkipInstructions.Value()) && (instrCount <= (KnobSkipInstructions.Value() + KnobTraceInstructions.Value())));
  }

  if (shouldWrite && (instrCount % 100000000) == 0) {
    std::cerr << " Another 100 million instructions gone by. instrCount = " << instrCount << std::endl << std::flush;
  }  

  return shouldWrite;
}

void WriteCurrentInstruction()
{
  typename decltype(outfile)::char_type buf[sizeof(trace_instr_format_t)];
  std::memcpy(buf, &curr_instr, sizeof(trace_instr_format_t));
  outfile.write(buf, sizeof(trace_instr_format_t));
}

void BranchOrNot(UINT32 taken)
{
  curr_instr.is_branch = 1;
  curr_instr.branch_taken = taken;
}

template <typename T>
void WriteToSet(T* begin, T* end, UINT32 r)
{
  auto set_end = std::find(begin, end, 0);
  auto found_reg = std::find(begin, set_end, r); // check to see if this register is already in the list
  *found_reg = r;
}

template <typename T>
void PrintInstAddr(T begin, void* ip, UINT32 r) {
  ADDRINT addr = (ADDRINT)ip;
  if (addr >= ROIStartAddress && addr < ROIEndAddress)
  std::cout << "At inst: 0x" << std::hex << addr << " Source Addr: 0x" <<  (ADDRINT)begin << " Operand: 0x" << r << std::dec << std::endl;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID* v)
{
    // begin each instruction with this function
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ResetCurrentInstruction, IARG_INST_PTR, IARG_END);

  // instrument branch instructions
  if (INS_IsBranch(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BranchOrNot, IARG_BRANCH_TAKEN, IARG_END);

  // instrument register reads
  UINT32 readRegCount = INS_MaxNumRRegs(ins);
  for (UINT32 i = 0; i < readRegCount; i++) {
    UINT32 regNum = INS_RegR(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.source_registers, IARG_PTR,
                   curr_instr.source_registers + NUM_INSTR_SOURCES, IARG_UINT32, regNum, IARG_END);
  }

  // instrument register writes
  UINT32 writeRegCount = INS_MaxNumWRegs(ins);
  for (UINT32 i = 0; i < writeRegCount; i++) {
    UINT32 regNum = INS_RegW(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.destination_registers, IARG_PTR,
                   curr_instr.destination_registers + NUM_INSTR_DESTINATIONS, IARG_UINT32, regNum, IARG_END);
  }

  // instrument memory reads and writes
  UINT32 memOperands = INS_MemoryOperandCount(ins);
  //const xed_decoded_inst_t xedd = *INS_XedDec(ins);
  //if (xed_decoded_inst_get_attribute(&xedd, XED_ATTRIBUTE_SIMD_SCALAR)) {
  //  if (INS_Address(ins) >= ROIStartAddress && INS_Address(ins) < ROIEndAddress)
  //  std::cerr << "Scalar instruction encountered at ins " << std::hex << INS_Address(ins) << ". ROI Start is " << ROIStartAddress <<std::dec << std::endl;
  //}
  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
    if (INS_MemoryOperandIsRead(ins, memOp)) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.source_memory, IARG_PTR,
                     curr_instr.source_memory + NUM_INSTR_SOURCES, IARG_MEMORYOP_EA, memOp, IARG_END);
//      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintInstAddr<unsigned long long int>, IARG_PTR, curr_instr.source_memory, IARG_INST_PTR,
//                    IARG_MEMORYOP_EA, memOp, IARG_END);
    }
    if (INS_MemoryOperandIsWritten(ins, memOp))
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.destination_memory, IARG_PTR,
                     curr_instr.destination_memory + NUM_INSTR_DESTINATIONS, IARG_MEMORYOP_EA, memOp, IARG_END);
  }

  // finalize each instruction with this function
  ADDRINT addr = INS_Address(ins);
  if (addr >= ROIStartAddress && addr < ROIEndAddress) {
//      std::cerr << "addr : 0x" << std::hex << addr << " ROI start: 0x" << ROIStartAddress << " ROI end: 0x" << ROIEndAddress << std::dec << std::endl << std::flush;
      INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)ShouldWrite, IARG_INST_PTR, IARG_END);
      INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteCurrentInstruction, IARG_END);
  }

}

VOID ImageLoad(IMG img, VOID *v)
{
    if (IMG_Name(img).find(KnobBMKName) != std::string::npos) {
        std::cerr << "Image name " << IMG_Name(img) << std::endl;
        for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        {
            for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
            {
                if (RTN_Name(rtn) == "_init") {
                    RTN_Open(rtn);
                    std::cout << "Found routine: " << RTN_Name(rtn) << " at 0x" <<std::hex << RTN_Address(rtn) <<std::dec<< " in section " << SEC_Name(sec) << std::endl;
                    ROIStartAddress = RTN_Address(rtn) + KnobROIStartAddress.Value();
                    ROIEndAddress = RTN_Address(rtn) + KnobROIEndAddress.Value();
                    std::cerr << std::hex << " ROI start: 0x" << ROIStartAddress << " ROI end: 0x" << ROIEndAddress << std::dec << std::endl;

                    RTN_Close(rtn);
                }
            }
        }
    }
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v) { outfile.close(); }

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid
  if (PIN_Init(argc, argv))
    return Usage();

  outfile.open(KnobOutputFile.Value().c_str(), std::ios_base::binary | std::ios_base::trunc);
  if (!outfile) {
    std::cout << "Couldn't open output trace file. Exiting." << std::endl;
    exit(1);
  }

  IMG_AddInstrumentFunction(ImageLoad,0);
  // Register function to be called to instrucment ROI

  // Register function to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, 0);

  // Register function to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
