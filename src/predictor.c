//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#define maxNumEntries 1 << 13
//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;
//TESTING
int counter = 0;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// in our PHT, 00 = 0 (SN, Strongly Not Taken), 01 = 1 (WN, Weakly Not Taken)
//             10 = 2 (WT, Weakly Taken), and 11 = 3 (ST, Strongly Taken)
uint8_t PHT[maxNumEntries];
uint32_t historyReg;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void gshare_init_predictor() 
{
  // initialize to WN (Weakly Not Taken)
  for (int i = 0; i < maxNumEntries; i++) {
    PHT[i] = WN;
  }

  // initialize global history register to NOTTAKEN (Not Taken)
  historyReg = NOTTAKEN;
}

void tournament_init_predictor() 
{

}

void
init_predictor()
{
  switch (bpType) {
    case GSHARE:
      gshare_init_predictor();
      break;
    case TOURNAMENT:
      tournament_init_predictor();
      break;
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
gshare_make_prediction(uint32_t pc)
{
  // use a mask to get the lower # of bits where the # of bits = ghistoryBits
  uint32_t pcMasked = pc & ((1 << ghistoryBits)-1);

  //TESTING
  if(counter < 20){
    printf("Make prediction: PC = %d, PC masked = %d\n", pc, pcMasked); 
    printf("Make prediction: Reg = %d\n", historyReg);
  }

  // XOR the global history register with the masked pc to get the index into the PHT
  uint32_t index = historyReg ^ pcMasked;
  // use the index to retrieve the prediction from the PHT
  uint8_t prediction = PHT[index];

  if (prediction == WT || prediction == ST) {
    return TAKEN;
  } 
  // otherwise, return NOTTAKEN (for WN and SN)
  return NOTTAKEN;
}

uint8_t
tournament_make_prediction(uint32_t pc)
{
  return NOTTAKEN;
}

uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_make_prediction(pc);
    case TOURNAMENT:
      return tournament_make_prediction(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void 
gshare_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t pcMasked;
  uint32_t index;
  uint8_t prediction;

  // use a mask to get the lower # of bits where the # of bits = ghistoryBits
  pcMasked = pc & ((1 << ghistoryBits)-1);
  // XOR the global history register with the masked pc to get the index into the PHT
  index = historyReg ^ pcMasked;
  // use the index to retrieve the prediction from the PHT
  prediction = PHT[index];

  if (prediction == SN && outcome == TAKEN) {
    PHT[index] = WN;
  } else if (prediction == WN && outcome == NOTTAKEN) {
    PHT[index] = SN;
  } else if (prediction == WN && outcome == TAKEN) {
    PHT[index] = WT;
  } else if (prediction == WT && outcome == NOTTAKEN) {
    PHT[index] = WN;
  } else if (prediction == WT && outcome == TAKEN) {
    PHT[index] = ST;
  } else if (prediction == ST && outcome == NOTTAKEN) {
    PHT[index] = WT;
  }
  
  //TESTING
  if(counter < 20){
    printf("\t Training: outcome    = %d\n", outcome);
    printf("\t Training: reg before = %d\n", historyReg);
  }

  historyReg <<= 1;
  historyReg |= outcome;
  historyReg = historyReg & ((1 << ghistoryBits)-1);

  //TESTING
  if(counter < 20){ printf("\t Training: reg after  = %d\n\n", historyReg);}
  counter++;
}

void
tournament_train_predictor(uint32_t pc, uint8_t outcome)
{

}

void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    case GSHARE:
      gshare_train_predictor(pc, outcome);
      break;
    case TOURNAMENT:
      tournament_train_predictor(pc, outcome);
      break;
    default:
      break;
  }
}
