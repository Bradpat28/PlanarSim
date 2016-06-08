#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include "Report.h"

#define MAX_GRID 20
#define N_INIT 100

typedef struct PlanarSim {
   int numRuns;
   int gridNum;
   int fixedT;
   int fixedB;
   int fixedL;
   int fixedR;
   int fixedC;
   double tempT;
   double tempB;
   double tempL;
   double tempR;
   double tempC;
} PlanarSim;

typedef struct Location {
   int row;
   int col;
} Location;

void InitSim();
void SetupSim(int argc, char **argv);
void CheckSim();
void SetupThreads();
void InitSimArr();
int CheckCorner(int row, int col);
void *Simulate(void *arg);
void LockAll(int row, int col);
void DestroyMutex();
int CheckStart();
void PrintSim(int simNum, FILE *csvFile);
double CalcAvg(int numSim, int row, int col);
