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
#define maxPTHeight 1 << 15
#define maxPTWidth 1 << 15
#define SL  0			// predict local, strong not local
#define WL  1			// predict local, weak not local
#define WG  2			// predict global, weak global
#define SG  3			// predict global, strong global

//
// TODO:Student Information
//
const char *studentName = "Kellian Hunt, Angelique Taylor";
const char *studentID   = "A53244070, A53230147";
const char *email       = "kchunt@eng.ucsd.edu, amt062@eng.ucsd.edu";
int counter = 0;

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

//Custom data structures:
int8_t PT[maxPTWidth][maxPTHeight]; //Perceptron Table
int theta;
int8_t globalHR[maxPTWidth]; // global history reg

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void 
gshare_init_predictor() 
{
  // initialize to WN (Weakly Not Taken)
  for (int i = 0; i < maxEntriesGshare; i++) {
    PHT[i] = WN;
  }

  // initialize global history register to NOTTAKEN (Not Taken)
  historyReg = NOTTAKEN;
}

void 
tournament_init_predictor() 
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
custom_init_predictor()
{
  // initialize theta
  theta = 100;

  // initialize Percetron table and global history reg
  for (int i = 0; i < maxPTWidth; i++) {
    globalHR[i] = -1;
    for (int j = 0; j < maxPTHeight; j++){
      PT[i][j] = 0;
    }
  }

  globalHR[1] = 1;
  globalHR[2] = -1;
  globalHR[3] = 1;
  globalHR[4] = 1;

  // bias
  globalHR[0] = 1;
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
    case CUSTOM:
      custom_init_predictor();
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

  // XOR the global history register with the masked pc to get the index into the PHT
  uint32_t index = historyReg ^ pcMasked;
  // use the index to retrieve the prediction from the PHT
  uint8_t prediction = PHT[index];

  if (prediction == WT || prediction == ST) 
    return TAKEN;

  // otherwise, return NOTTAKEN (for WN and SN)
  return NOTTAKEN;
}

uint8_t
tournament_make_prediction(uint32_t pc)
{
  // local prediction
  uint32_t local_hist_index = pc & ((1 << pcIndexBits)-1);
  uint32_t local_hist = localHistTable[local_hist_index];
  uint8_t local_prediction = localPrediction[local_hist];

  // global prediction
  uint8_t global_prediction = globalPrediction[historyReg];

  // choice prediction
  uint8_t choice = choicePrediction[historyReg];

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
custom_make_prediction(uint32_t pc)
{
  uint32_t pcMasked;
  int8_t weights[ghistoryBits];
  int y = 0;

  // use a mask to get the lower # of bits where the # of bits = ghistoryBits
  pcMasked = pc & ((1 << pcIndexBits)-1);

  // index into PT for weight vector
  for (int i = 0; i < ghistoryBits; i++){
    weights[i] = PT[pcMasked][i];
  }

  // Make prediction
  for (int i = 0; i < ghistoryBits; i++){
    y = y + weights[i] * globalHR[i];
  }

  if( y < 0 ){
    return NOTTAKEN;
  }
  return TAKEN;
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
      return custom_make_prediction(pc);
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

  historyReg <<= 1;
  historyReg |= outcome;
  historyReg = historyReg & ((1 << ghistoryBits)-1);
}

void
tournament_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t pcMasked;
  uint32_t index;
  uint8_t local_prediction, global_prediction;

  //**** Train Local Predictor ****
  // use a mask to get the lower # of bits where the # of bits = lhistoryBits
  pcMasked = pc & ((1 << pcIndexBits)-1);

  // use the last lhistoryBits of pc to index into LHT
  index = localHistTable[pcMasked];

  // use the bits in LHT[pc] to index into localPrediction or BHT
  local_prediction = localPrediction[index];

  // Check whether the localPrediction is correct and update using 2-bit predictor
  if (local_prediction == SN && outcome == TAKEN) {
    localPrediction[index] = WN;
  } else if (local_prediction == WN && outcome == NOTTAKEN) {
    localPrediction[index] = SN;
  } else if (local_prediction == WN && outcome == TAKEN) {
    localPrediction[index] = WT;
  } else if (local_prediction == WT && outcome == NOTTAKEN) {
    localPrediction[index] = WN;
  } else if (local_prediction == WT && outcome == TAKEN) {
    localPrediction[index] = ST;
  } else if (local_prediction == ST && outcome == NOTTAKEN) {
    localPrediction[index] = WT;
  }

  // update localHistTable with outcome
  localHistTable[pcMasked] <<= 1;
  localHistTable[pcMasked] |= outcome;
  localHistTable[pcMasked] = localHistTable[pcMasked] & ((1 << lhistoryBits)-1);

  //**** Train Global Predictor ****
  // use the index to retrieve the prediction from the PHT
  global_prediction = globalPrediction[historyReg];

  // Check whether the localPrediction is correct and update using 2-bit predictor
  index = historyReg;
  if (global_prediction == SN && outcome == TAKEN) {
    globalPrediction[index] = WN;
  } else if (global_prediction == WN && outcome == NOTTAKEN) {
    globalPrediction[index] = SN;
  } else if (global_prediction == WN && outcome == TAKEN) {
    globalPrediction[index] = WT;
  } else if (global_prediction == WT && outcome == NOTTAKEN) {
    globalPrediction[index] = WN;
  } else if (global_prediction == WT && outcome == TAKEN) {
    globalPrediction[index] = ST;
  } else if (global_prediction == ST && outcome == NOTTAKEN) {
    globalPrediction[index] = WT;
  }

  //**** Train Choice Predictor ****
  // get bits in choice prediction table
  uint8_t choice = choicePrediction[historyReg];

  // update historyReg with the outcome
  historyReg <<= 1;
  historyReg |= outcome;
  historyReg = historyReg & ((1 << ghistoryBits)-1);

  uint8_t global_choice;
  if (global_prediction == WT || global_prediction == ST) {
    global_choice = TAKEN;
  } else {
    global_choice = NOTTAKEN;
  }

  uint8_t local_choice;
  if (local_prediction == WT || local_prediction == ST) {
    local_choice = TAKEN;
  } else {
    local_choice = NOTTAKEN;
  }  

  // if the global and local predictors disagree, then train the choice predictor
  if (global_choice != local_choice) {
    // state machine
    if (choice == SG && global_choice != outcome) {
      choicePrediction[index] = WG;
    } else if (choice == WG && global_choice == outcome) {
      choicePrediction[index] = SG;
    } else if (choice == WG && global_choice != outcome) {
      choicePrediction[index] = WL;
    } else if (choice == WL && local_choice != outcome) {
      choicePrediction[index] = WG;
    } else if (choice == WL && local_choice == outcome) {
      choicePrediction[index] = SL;
    } else if (choice == SL && local_choice != outcome) {
      choicePrediction[index] = WL;
    }
  }
}

void
custom_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t pcMasked;
  int8_t weights[ghistoryBits];
  int y = 0;
  uint8_t prediction;
  int outcome_temp;

  if(outcome == TAKEN){
    outcome_temp = 1;
  }
  else{
    outcome_temp = -1;
  }

  // use a mask to get the lower # of bits where the # of bits = ghistoryBits
  pcMasked = pc & ((1 << pcIndexBits)-1);

  // index into PT for weight vector
  for (int i = 0; i < ghistoryBits; i++){
    weights[i] = PT[pcMasked][i];
  }

  // Make prediction
  for (int i = 0; i < ghistoryBits; i++){
    y = y + weights[i] * globalHR[i];
  }

  if( y < 0 ){
    prediction = NOTTAKEN;
  }
  else{
    prediction = TAKEN;
  }

  // update weights
  if( prediction == outcome || abs(y) <= theta ){
    for (int i = 0; i < ghistoryBits; i++){
      weights[i] = weights[i] + outcome_temp*globalHR[i];
    }
  }

  if(counter < 20){
  printf("New weights: [");
  for (int i = 0; i < ghistoryBits -1; i++) {
      printf("%d, ", weights[i]);
  }
  printf("%d]\n", weights[ghistoryBits]);
  }

 
  // update PT
  for( int i = 0; i < ghistoryBits; i++ ){
    PT[pcMasked][i] = weights[i];
  }

  if (counter < 20) {
  printf("New PT: [");
  for (int i = 0; i < ghistoryBits - 1; i++){
    printf("%d, ", PT[pcMasked][i]);
  }
  printf("%d]\n", PT[pcMasked][ghistoryBits]);
  }

  // update ghr
  for (int i = 1; i < ghistoryBits-1; i++)
    globalHR[i] = globalHR[i+1];
  globalHR[ghistoryBits-1] = outcome_temp;

  if (counter < 20) {
  printf("New GHR: [");
  for (int i = 0; i < ghistoryBits - 1; i++){
    printf("%d, ", globalHR[i]);
  }
  printf("%d]\n", globalHR[ghistoryBits]);
  }
  counter++;
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
    case CUSTOM:
      custom_train_predictor(pc, outcome);
      break;
    default:
      break;
  }
}
