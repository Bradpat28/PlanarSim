#include "planarSim.h"

double ***simulation;
int simCount = 0;
pthread_mutex_t simMutex;
pthread_cond_t readyCondP;
pthread_mutex_t readyMutex;
pthread_cond_t readyCondC;
PlanarSim *sim;

int main(int argc, char **argv) {
   sim = calloc(1, sizeof(PlanarSim));
   InitSim();
   SetupSim(argc, argv);
   CheckSim();
   SetupThreads();
   return EXIT_SUCCESS;
}

void InitSim() {
   sim->numRuns = -1;
   sim->gridNum = -1;

   sim->fixedT = 0;
   sim->fixedB = 0;
   sim->fixedL = 0;
   sim->fixedR = 0;
   sim->fixedC = 0;

   sim->tempT = 0.0;
   sim->tempB = 0.0;
   sim->tempL = 0.0;
   sim->tempR = 0.0;
   sim->tempC = 0.0;
}

void SetupSim(int argc, char **argv) {
   int ndx, val1;
   char testChar; 
   double val2;

   for (ndx = 0; ndx < argc; ndx++) {
      testChar = *(argv[ndx]);
      switch (testChar) {
         case 'G': 
            val1 = atoi(argv[ndx] + 1);
            if (val1 > 0 && val1 <= MAX_GRID) {
               if (sim->gridNum == -1) {
                  sim->gridNum = val1;
               }
            }
            break;
 
         case 'S':
            val1 = atoi(argv[ndx] + 1);
            if (val1 >= 0 && sim->numRuns == -1) {
               sim->numRuns = val1;
            }
            break;

         case 'T':
            val2 = atof(argv[ndx] + 1);
            if (!sim->fixedT) {
               sim->fixedT = 1;
               sim->tempT = val2;
            }
            break;

         case 'B':
            val2 = atof(argv[ndx] + 1);
            if (!sim->fixedB) {
               sim->fixedB = 1;
               sim->tempB = val2;
            }
            break;

         case 'L':
            val2 = atof(argv[ndx] + 1);
            if (!sim->fixedL) {
               sim->fixedL = 1;
               sim->tempL = val2;
            }
            break;

         case 'R':
            val2 = atof(argv[ndx] + 1);
            if (!sim->fixedR) {
               sim->fixedR = 1;
               sim->tempR = val2;
            }
            break;

         case 'C':
            val2 = atof(argv[ndx] + 1);
            if (!sim->fixedC) {
               sim->fixedC = 1;
               sim->tempC = val2;
            }

         default:
            ;
      }
   }
}

void CheckSim() {
   if (sim->numRuns == -1 || sim->gridNum == -1) {
      fprintf(stderr, "Usage: ./PlanarSim G S T B L R (in any order)\n");
      exit(EXIT_FAILURE);
   }
   if (sim->gridNum % 2 == 0 && sim->fixedC) {
      printf("*****Warning: No \"center\" will execute normally\n");
      sim->fixedC = 0;
   }
}

void SetupThreads() {
   int row, col, simCounter, tCount = 0;
   int grids = (sim->gridNum + 2) * (sim->gridNum + 2) - 4;
   pthread_t threads[grids]; 
   int retVal; 
   size_t stackSize = sizeof(double) * N_INIT * N_INIT;
   static pthread_attr_t attr;
   void *status;
   Location pos[grids];

   InitSimArr();
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_attr_setstacksize(&attr, stackSize); 
   pthread_cond_init(&readyCondP, NULL);
   pthread_cond_init(&readyCondC, NULL);
   pthread_mutex_init(&readyMutex, NULL);
   pthread_mutex_init(&simMutex, NULL);

   for (row = 0; row < sim->gridNum + 2; row++) {
      for (col = 0; col < sim->gridNum + 2; col++) {
         if (!CheckCorner(row, col)) {
            pos[tCount].row = row;
            pos[tCount].col = col; 
            retVal = pthread_create(&threads[tCount], &attr, Simulate,
             (void *) &pos[tCount]);
            tCount++;
            if (retVal) {
               fprintf(stderr, "Thread Creation Error : %d\n", retVal);
               exit(EXIT_FAILURE);
            } 
         }
      }
   }
   /*if ((csvFile = fopen("csvFile.csv", "w")) == NULL) {
      perror("FILE OPEN ERROR: ");
   }*/

   for (simCounter = 0; simCounter <= sim->numRuns; simCounter++) {
      pthread_mutex_lock(&readyMutex);
      pthread_cond_wait(&readyCondP, &readyMutex);
      PrintSim(simCounter,  NULL);
      pthread_mutex_lock(&simMutex);
      simCount = 0;
      pthread_mutex_unlock(&simMutex);
      pthread_mutex_unlock(&readyMutex);
      usleep(100);
      pthread_cond_broadcast(&readyCondC);

   }
   for (tCount = 0; tCount < grids; tCount++) {
      retVal = pthread_join(threads[tCount], &status);
      if (retVal) {
         fprintf(stderr, "Thread Join error: %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   }
   pthread_cond_destroy(&readyCondC);
   pthread_mutex_destroy(&readyMutex);
   pthread_mutex_destroy(&simMutex);
   pthread_attr_destroy(&attr);
   //fclose(csvFile);
   pthread_exit(NULL);
}

void *Simulate(void *arg) { 
   Location *pos = (Location *) arg;
   double avg = 0; 
   int numSim, fixed;
   int numCells = (sim->gridNum + 2) * (sim->gridNum + 2) - 4;
    
   avg = simulation[0][pos->row][pos->col];
   for (numSim = 0; numSim <= sim->numRuns; numSim++) {
      if (numSim == 0) {
         if (avg == 0.0)
            fixed = 1;
         else if (avg == -1) {
            simulation[0][pos->row][pos->col] = 0.0;
            avg = 0.0;
            fixed = 0;
         }
         else 
            fixed = 1;
      }
      else {
         if (!fixed)
            avg = CalcAvg(numSim, pos->row, pos->col);
         simulation[numSim][pos->row][pos->col] = avg;
      }
      pthread_mutex_lock(&simMutex);
      simCount += 1;
      pthread_mutex_unlock(&simMutex);
      if (simCount == numCells) {
         pthread_cond_signal(&readyCondP);
      }
      if (numSim != sim->numRuns) {
         pthread_mutex_lock(&readyMutex);
         pthread_cond_wait(&readyCondC, &readyMutex);
         pthread_mutex_unlock(&readyMutex);
      }
   }
   pthread_exit(NULL);
}

void InitSimArr() {
   int simNum, col, row;
   int left = 0;
   int right = sim->gridNum + 1;
   int top = 0;
   int bot = sim->gridNum + 1;
   int rowC = 1;
   int colC = 1;

   if (sim->fixedC) {
      rowC = colC += (sim->gridNum - 1) / 2;
   }
 
   simulation = (double ***) calloc(sim->numRuns + 1, sizeof(double **));

   for (simNum = 0; simNum < sim->numRuns + 1; simNum++) {
      simulation[simNum] = (double **) calloc(sim->gridNum + 2, 
       sizeof(double *));

      for (row = 0; row < sim->gridNum + 2; row++) {
         simulation[simNum][row] = (double *) calloc(sim->gridNum + 2,
          sizeof(double));

         for (col = 0; col < sim->gridNum + 2; col++) {
            simulation[simNum][row][col] = -1;
            if (!CheckCorner(row, col)) {
               if (row == top) 
                  simulation[simNum][row][col] = sim->tempT;
               else if (row == bot) 
                  simulation[simNum][row][col] = sim->tempB;
               else if (col == left) 
                  simulation[simNum][row][col] = sim->tempL;
               else if (col == right)
                  simulation[simNum][row][col] = sim->tempR;
               else if (row == rowC && col == colC) {
                  if (sim->fixedC) {
                     simulation[simNum][row][col] = sim->tempC;
                  }
               }

            }
         }
      } 
   }
}

int CheckCorner(int row, int col) {
   int corner = 0;
   int max = sim->gridNum + 1;

   if (row == 0 && col == 0)
      corner = 1;
   else if (row == 0 && col == max)
      corner = 1;
   else if (row == max && col == 0)
      corner = 1;
   else if (row == max && col == max)
      corner = 1;
 
   return corner;
}

void PrintSim(int simNum, FILE *csvFile) {
   int row, col;
   int grid = (sim->gridNum + 2) * (sim->gridNum + 2);

   printf("----------------------------------------\n");
   printf("Simulation %d/%d Total Cells %d\n", simNum, sim->numRuns, grid);
   printf("----------------------------------------\n");
   for (row = 0; row < sim->gridNum + 2; row++) {
      for (col = 0; col < sim->gridNum + 2; col++) {
         if (!CheckCorner(row, col)) {
            if (simulation[simNum][row][col] == 0)
               printf("%*d", 10, 0);
            else
               printf("%*.3f", 10, simulation[simNum][row][col]);
            //fprintf(csvFile, "%.3f,", simulation[simNum][row][col]);
         }
         else {
            if (col == 0) {
               //fprintf(csvFile, "%d,", -1);
               printf("%*s", 10, "X");
            }
            else {
               //fprintf(csvFile, "%d", -1);
               printf("%*s", 10, "X");
            }
         }
      }
      printf("\n");
      //fprintf(csvFile, "\n");
      if (row != sim->gridNum + 1)
         printf("\n");
   }
}

double CalcAvg(int numSim, int row, int col) {
   double aboveVal = simulation[numSim - 1][row - 1][col];
   double belowVal = simulation[numSim - 1][row + 1][col];
   double rightVal = simulation[numSim - 1][row][col + 1];
   double leftVal = simulation[numSim - 1][row][col - 1];

   return (aboveVal + belowVal + rightVal + leftVal) / 4;
}

