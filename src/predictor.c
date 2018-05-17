//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#define maxEntriesGshare 1 << 13
#define maxEntriesLocalHistTable 1 << 10
#define maxEntriesLocalPrediction 1 << 10
#define maxEntriesGlobalPrediction 1 << 9
#define maxEntriesChoicePrediction 1 << 10
#define SL  0			// predict local, strong not local
#define WL  1			// predict local, weak not local
#define WG  2			// predict global, weak global
#define SG  3			// predict global, strong global

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
//int counter = 0;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// gshare data structures:
// in our PHT, 00 = 0 (SN, Strongly Not Taken), 01 = 1 (WN, Weakly Not Taken)
//             10 = 2 (WT, Weakly Taken), and 11 = 3 (ST, Strongly Taken)
uint8_t PHT[maxEntriesGshare];
// also used by tournament:
uint32_t historyReg;

// tournament data structures:
uint32_t localHistTable[maxEntriesLocalHistTable];
uint8_t localPrediction[maxEntriesLocalPrediction];
uint8_t globalPrediction[maxEntriesGlobalPrediction];
uint8_t choicePrediction[maxEntriesChoicePrediction];

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void gshare_init_predictor() 
{
  // initialize to WN (Weakly Not Taken)
  for (int i = 0; i < maxEntriesGshare; i++) {
    PHT[i] = WN;
  }

  // initialize global history register to NOTTAKEN (Not Taken)
  historyReg = NOTTAKEN;
}

void tournament_init_predictor() 
{
  // initialize local predictors
  for (int i = 0; i < maxEntriesLocalHistTable; i++) {
    localHistTable[i] = NOTTAKEN;
  }
  for (int i = 0; i < maxEntriesLocalPrediction; i++) {
    localPrediction[i] = WN;
  }
  // initialize global predictors
  historyReg = NOTTAKEN;
  for (int i = 0; i < maxEntriesGlobalPrediction; i++) {
    globalPrediction[i] = WN;
  }
  // initialize choice predictor
  for (int i = 0; i < maxEntriesChoicePrediction; i++) {
    choicePrediction[i] = WG;
  }
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
  //if(counter < 20){
  //  printf("Make prediction: PC = %d, PC masked = %d\n", pc, pcMasked); 
  //  printf("Make prediction: Reg = %d\n", historyReg);
  //}

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
  // local prediction
  uint32_t local_pcMasked = pc & ((1 << lhistoryBits)-1);
  uint32_t local_hist = localHistTable[local_pcMasked];
  uint8_t local_prediction = localPrediction[local_hist];

  // global prediction
  uint8_t global_prediction = globalPrediction[historyReg];

  // choice prediction
  uint32_t choice_pcMasked = pc & ((1 << pcIndexBits)-1);
  uint8_t choice = choicePrediction[choice_pcMasked];

  // if the choice leans towards the local predictor:
  if (choice == WL || choice == SL) {
    if (local_prediction == WT || local_prediction == ST) 
      return TAKEN;
    return NOTTAKEN;  
  } 

  // if the choice leans towards the global predictor:
  if (global_prediction == WT || global_prediction == ST)
    return TAKEN;
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
  //if(counter < 20){
  //  printf("\t Training: outcome    = %d\n", outcome);
  //  printf("\t Training: reg before = %d\n", historyReg);
  //}

  historyReg <<= 1;
  historyReg |= outcome;
  historyReg = historyReg & ((1 << ghistoryBits)-1);

  //TESTING
  //if(counter < 20){ printf("\t Training: reg after  = %d\n\n", historyReg);}
  //counter++;
}

void
tournament_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t pcMasked;
  uint32_t index;
  uint32_t index;
  uint8_t prediction;

  //**** Train Local Predictor ****
  // use a mask to get the lower # of bits where the # of bits = ghistoryBits
  pcMasked = pc & ((1 << lhistoryBits)-1);

  // use the last lhistoryBits of pc to index into LHT
  index = localHistTable[pcMasked];

  // use the bits in LHT[pc] to index into localPrediction or BHT
  prediction = localPrediction[index];

  // Check whether the localPrediction is correct and update using 2-bit predictor
  if (prediction == SN && outcome == TAKEN) {
    localPrediction[index] = WN;
  } else if (prediction == WN && outcome == NOTTAKEN) {
    localPrediction[index] = SN;
  } else if (prediction == WN && outcome == TAKEN) {
    localPrediction[index] = WT;
  } else if (prediction == WT && outcome == NOTTAKEN) {
    localPrediction[index] = WN;
  } else if (prediction == WT && outcome == TAKEN) {
    localPrediction[index] = ST;
  } else if (prediction == ST && outcome == NOTTAKEN) {
    localPrediction[index] = WT;
  }

  // update localHistTable with outcome
  localHistTable[pcMasked] <<= 1;
  localHistTable[pcMasked] |= outcome;
  localHistTable[pcMasked] = localHistTable[pcMasked] & ((1 << lhistoryBits)-1);

  //**** Train Global Predictor ****
  // use the index to retrieve the prediction from the PHT
  prediction = globalPrediction[historyReg];

  // Check whether the localPrediction is correct and update using 2-bit predictor
  if (prediction == SN && outcome == TAKEN) {
    globalPrediction[index] = WN;
  } else if (prediction == WN && outcome == NOTTAKEN) {
    globalPrediction[index] = SN;
  } else if (prediction == WN && outcome == TAKEN) {
    globalPrediction[index] = WT;
  } else if (prediction == WT && outcome == NOTTAKEN) {
    globalPrediction[index] = WN;
  } else if (prediction == WT && outcome == TAKEN) {
    globalPrediction[index] = ST;
  } else if (prediction == ST && outcome == NOTTAKEN) {
    globalPrediction[index] = WT;
  }

  historyReg <<= 1;
  historyReg |= outcome;
  historyReg = historyReg & ((1 << ghistoryBits)-1);

  //**** Train Choice Predictor ****
  //choicePrediction
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
