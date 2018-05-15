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
void
init_predictor()
{
  // initialize to WN (Weakly Not Taken)
  for (int i = 0; i < maxNumEntries; i++) {
    PHT[i] = WN;
  }

  // initialize global history register to NOTTAKEN (Not Taken)
  historyReg = NOTTAKEN;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  uint32_t pcMasked;
  uint32_t regMasked;
  uint32_t index;
  uint8_t prediction;

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
        // use a mask to get the lower # of bits where the # of bits = ghistoryBits
        pcMasked = pc & ((1 << ghistoryBits)-1);
        // mask the history reg
        regMasked = historyReg & ((1 << ghistoryBits)-1);
        // XOR the global history register with the masked pc to get the index into the PHT
        index = regMasked ^ pcMasked;
        // use the index to retrieve the prediction from the PHT
        prediction = PHT[index];

        if (prediction == WT || prediction == ST) {
          return TAKEN;
        } 
        else if (prediction == WN || prediction == SN) {
          return NOTTAKEN;
        }
        break;
    case TOURNAMENT:
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
train_predictor(uint32_t pc, uint8_t outcome)
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
  
  historyReg <<= outcome;
}
