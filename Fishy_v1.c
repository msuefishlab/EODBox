
/*Fishy_v1.c written by Bailey Winter February 2016. Derived from SpikeDetector_v8.c written by Bailey Winter and 
*  Stanislav Mircic July 2015.Initial source code from the High Speed ADC example in Chapter 13 of Exploring
*  BeagleBone by Derek Molloy. It loads Fishy.p and PRUClock.p into the PRU-ICSS, transfers the configuration to 
*  the PRU memory spaces, and starts the execution of both PRU programs. The data is continuously fetched from the 
*  PRU memory, either the raw data is saved or the data is analysed to determine if a spike has occurred by determining 
*  if a sample has reached a threshold value. If a spike has been detected, it will copy one thousand samples before and 
*  after the triggering sample into a file. The file is saved as an .EOD file, and the file name will be the date and time
*  the file was created.
*/

/*	HOW TO COMPILE AND EXECUTE
*	To compile this code, type the following command into the command line.
*	
*	gcc Fishy_v1.c -o Fishy_v1 -lpthread -lprussdrv -lm
*
*	To execute this program, type the following command into the command line.
*
*	./Fishy_v1
*/

#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>


#define ADC_PRU_NUM	   0   // using PRU0 for the ADC Save.CAPTURE
#define CLK_PRU_NUM	   1   // using PRU1 for the sample clock
#define MMAP0_LOC     "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC     "/sys/class/uio/uio0/maps/map1/" //"root/exploringBB/Ch13" 

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)
#define MMAP_LOC   "/sys/class/uio/uio0/maps/map1/"

#define MAX_MEMORY 3999996
#define DELAY_COUNT 500000
//#define Save.CAPTURE 200
#define THRESHOLD 20

enum FREQUENCY {    // measured and calibrated, but can be calculated
	//Commented out frequencies have not been calibrated. ADS7883 is not capable of sampling past 1MHz  using this set up
	//FREQ_12_5MHz =  1  
	//FREQ_6_25MHz =  6,
	//FREQ_5MHz    =  7,
	//FREQ_3_85MHz = 10,
	FREQ_1MHz   =  5,
	FREQ_500kHz =  127,
	FREQ_250kHz = 255,
	FREQ_100kHz = 495,
	//NOT ABLE TO SELECT BELOW 100kHz FOR 1M ADC
	FREQ_25kHz = 1995,
	FREQ_10kHz = 4995,
	FREQ_5kHz =  9995,
	FREQ_2kHz = 24995,
	FREQ_1kHz = 49995
};

enum CONTROL {
	PAUSED = 0,
	RUNNING = 1,
	UPDATE = 3
};

struct Header{
		char header[2048];
		int RunMode;
		float RunTime;
		float RunTime_ms;
		char	SpecimenNo[5];
		char	SpecimenLocal[256];
		char	RecordingTemp[20];
		int	Polarity;
		char	Coupling[20];
		int	Gain;
		char	Species[256];
		char	Comments[100000];
		char Experimenter_name[100];
		char Date[11];
		char Time[6];
		unsigned int Code_Version;
		unsigned int Header_Version;
		char Sample_Rate[20]; 
		int  Samp_Rate;
		int CAPTURE;
		int x_value;
	
			
};

struct Header Save = {.Code_Version = 1, .Header_Version = 1};	
	
	




// Short function to load a single unsigned int from a sysfs entry
unsigned int readFileValue(char filename[]){
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

//FUNCTION DECLARATIONS
double MeanCalculator(double *mean,  double *prev_mean, void* tail);
double StandardDeviation(double *mean, double*currentValue,void* tail, double *SD);
void WritetoFile(void* tail,void* map_base, off_t startofBuffer, off_t currentTailPosition, FILE* ofp,int *first_record,unsigned long long int elapsed, struct timeval start, struct timeval sample,FILE* dummy);
FILE* Header();
int run (FILE* ofp);
void PlotGraph(FILE* ofp, FILE* dummy);

int main(){
	
	FILE* ofp = Header();
	run(ofp);
	
}

int run (FILE* ofp)
{
   if(getuid()!=0){
      printf("You must run this program as root. Exiting.\n");
      exit(EXIT_FAILURE);
   }
   // Initialize structure used by prussdrv_pruintc_intc
   // PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
   tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

   // Read in the location and address of the shared memory. This value changes
   // each time a new block of memory is allocated.
   unsigned int timerData[2];
   //ADD SELECTABLE SAMPLING RATE HERE
   timerData[0] = FREQ_1MHz;
   timerData[1] = RUNNING;
   printf("The PRU clock state is set as period: %04X (0x%x) and state: %04X\n", timerData[0], timerData[0], timerData[1]);
   unsigned int PRU_data_addr = readFileValue(MMAP0_LOC "addr");
   printf("-> the PRUClock memory is mapped at the base address: %x\n", (PRU_data_addr + 0x2000));
   printf("-> the PRUClock on/off state is mapped at address: %x\n", (PRU_data_addr + 0x10000));

   // data for PRU0 based on the MCPXXXX datasheet
   unsigned int spiData[3];
   spiData[0] = 0x01800000;
   spiData[1] = readFileValue(MMAP1_LOC "addr");
   spiData[2] = readFileValue(MMAP1_LOC "size");
   printf("Sending the SPI Control Data: 0x%x\n", spiData[0]);
   printf("The DDR External Memory pool has location: 0x%x and size: 0x%x bytes\n", spiData[1], spiData[2]);
   int numberSamples = spiData[2]/2;
   printf("-> this space has capacity to store %04X 16-bit samples (max)\n", numberSamples);

   // Allocate and initialize memory
   prussdrv_init ();
   prussdrv_open (PRU_EVTOUT_0);
   

				
   // Write the address and size into PRU0 Data RAM0. You can edit the value to
   // PRUSS0_PRU1_DATARAM if you wish to write to PRU1
   prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, spiData, 12);  // spi code
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, timerData, 8); // sample clock

   // Map the PRU's interrupts
   prussdrv_pruintc_init(&pruss_intc_initdata);

   // Load and execute the PRU program on the PRU
   prussdrv_exec_program (ADC_PRU_NUM, "./Fishy.bin");
   prussdrv_exec_program (CLK_PRU_NUM, "./PRUClock.bin");
 
   printf("EBBClock PRU1 program now running (%04X)\n", timerData[0]);
 
   
   
   //VARIABLE DECLARATIONS
   
    void *map_base, *virt_addr, *tail;
	unsigned int temp;
	unsigned int tempSize;
	unsigned int addr = readFileValue(MMAP_LOC "addr");   
	
	off_t target = addr;
    off_t startofBuffer = target +4;
	off_t currentTailPosition = startofBuffer;
	uint32_t last_read_result=0;
	uint32_t i;
	int j;
	int count =0;
	int wrap_around = 0;
	uint32_t read_result, writeval;

	int first_record = 0;
	
	int fd;
	double mean=0;
	double SD = 0;
	double meansq =0;
		
	off_t collectPosition = currentTailPosition;
	uint32_t samplePosition = (uint32_t)tail;
	
	struct timeval start;
	struct timeval sample;
	unsigned long long int elapsed;
	
	


	// Initialize variables
	
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
			printf("Failed to open memory!");
			return -1;
	}    fflush(stdout);

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
       printf("Failed to map base address");
       return -1;
    }
    fflush(stdout);
	
	virt_addr = map_base + (target & MAP_MASK);	
	tail = map_base + (currentTailPosition & MAP_MASK);
	
	
	
	int delay_count;
	float init_sum =0.0;
	float counter_limit = 1000.0;
	
	usleep(1*1000);

	FILE* dummy;
	dummy = fopen("dummy.dat","w+");

	
	
	//calculate an initial mean
	read_result = (*((uint32_t *) virt_addr))-4;
	if(read_result < counter_limit){
		
		counter_limit =read_result;
	}

	for(delay_count =0; delay_count<counter_limit; delay_count ++){
		
		init_sum = (int)init_sum + *((uint16_t*)tail);
	
		
		currentTailPosition = currentTailPosition + 2;
		tail = map_base + (currentTailPosition & MAP_MASK);
		
	
	}
	
	float init_mean = init_sum/counter_limit;

	
	mean = (double)init_mean;

	double prev_mean = mean;

	double currentValue = 0;
	
	currentTailPosition = startofBuffer;
	tail = map_base + (currentTailPosition & MAP_MASK);
	
	gettimeofday(&start, 0);
	
	//printf("run time = %f\n", Save.RunTime);
	//printf("run time ms = %f\n", Save.RunTime_ms);

	//Read through the memory
	while(1){

			
        read_result = (*((uint32_t *) virt_addr))-4;
		
		
		
		
		if(read_result == last_read_result){
			continue;
		}
		
		//If the end memory count is greater than the start of memory count read through the memory normally
		if(read_result>last_read_result){
			
		
			for(i = last_read_result; i< read_result; i+=2){
				
				if(Save.RunMode == 0){
					
					gettimeofday(&sample, NULL); 
					elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
					if(elapsed >= Save.RunTime_ms){
						
						PlotGraph(ofp,dummy);
					}
					else{
						
						
						fprintf(ofp, "%04X", *((uint16_t*)tail));
						//fprintf(dummy,"%d,", *((uint16_t*)tail));
						fprintf(dummy,"%d\n", *((uint16_t*)tail));
						
						//Move to the next sample
						currentTailPosition = currentTailPosition + 2;
						tail = map_base + (currentTailPosition & MAP_MASK);
					}
					
					
				}
				
				else{
					//Read through the data chunk and determine if there is a spike.
					//The threshold is set by the standard deviation from the mean.
					MeanCalculator(&mean,  &prev_mean, tail);
					StandardDeviation(&mean, &currentValue,tail,&SD);
						gettimeofday(&sample, NULL); 
						elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
						if(elapsed >= Save.RunTime_ms){
						
							PlotGraph(ofp,dummy);
						}
					
					//If a spike has been detected record the previous data points and the time the spike occured
					if(abs(*((uint16_t*)tail)-(uint16_t)mean) >= THRESHOLD*SD && count ==0){
						
						gettimeofday(&sample, NULL); 
						elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
						if(elapsed >= Save.RunTime_ms){
						
							PlotGraph(ofp,dummy);
						}
						else{
							fprintf(ofp, "%06X",elapsed);
						
							WritetoFile(tail,map_base, startofBuffer, currentTailPosition, ofp, &first_record,elapsed,start,sample, dummy);
						
						
							count = (Save.CAPTURE*0.6);
						}
						
						
						
					}
					
					//Record the following data points
					else if(count > 0){
						
						
						count --;
						
						
						//End the line when a full wave has been recorded.	
						if(count == 0){
							
								Save.x_value++;
								fprintf(ofp, "%04X\n", *((uint16_t*)tail));
								//fprintf(dummy, "%d\n\n\n", *((uint16_t*)tail));
								
								
								fprintf(dummy,"%f %d\n\n\n",((float)Save.x_value/Save.Samp_Rate),*((uint16_t*)tail));
								gettimeofday(&sample, NULL); 
							elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
							if(elapsed >= Save.RunTime_ms){
						
								PlotGraph(ofp,dummy);
							}
							
						}
						else{
								
								
								Save.x_value++;
								fprintf(ofp,"%04X", *((uint16_t*)tail));
								
								//fprintf(dummy, "%d\n", *((uint16_t*)tail));
								fprintf(dummy,"%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
															
								
						}
							
					
					}
				
					//Move to the next sample
					currentTailPosition = currentTailPosition + 2;
					tail = map_base + (currentTailPosition & MAP_MASK);
				}
		
				
			}
			
			
				
				
				
		}
   
		else{
			// If the previous condition is false, make a two part copy 
			if(MAX_MEMORY-last_read_result>0){
				
				
					
				for(i = last_read_result; i <MAX_MEMORY; i+=2){
					
					if(Save.RunMode == 0){
						
						gettimeofday(&sample, NULL); 
						elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
						if(elapsed >= Save.RunTime_ms){
							
							PlotGraph(ofp,dummy);
						}
						else{
							
							fprintf(ofp, "%04X", *((uint16_t*)tail));
							
							fprintf(dummy,"%d\n", *((uint16_t*)tail));
							
							//Move to the next sample
							currentTailPosition = currentTailPosition + 2;
							tail = map_base + (currentTailPosition & MAP_MASK);
						}
						
						
					}
					
					else{
						MeanCalculator(&mean, &prev_mean, tail);
						StandardDeviation(&mean, &currentValue,tail,&SD);
							gettimeofday(&sample, NULL); 
							elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
							if(elapsed >= Save.RunTime_ms){
						
								PlotGraph(ofp,dummy);
							}
						
						
						if( abs(*((uint16_t*)tail)-(uint16_t)mean) >= THRESHOLD*SD && count ==0){
						
							gettimeofday(&sample, NULL); 
							elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
							if(elapsed >= Save.RunTime_ms){
							
								PlotGraph(ofp,dummy);
							}
							
							else{
								
								fprintf(ofp, "%06X",elapsed);
								WritetoFile(tail,map_base, startofBuffer, currentTailPosition, ofp, &first_record,elapsed,start,sample, dummy);
							
							
								if(((uint32_t)tail-startofBuffer)/2 < (Save.CAPTURE*0.6)){
									
											count = (MAX_MEMORY - samplePosition)/2; 
											wrap_around = (Save.CAPTURE*0.6) -((MAX_MEMORY - samplePosition)/2 );
								}
								else{
									
									
									count = (Save.CAPTURE*0.6);
								}
								
							}
							
							
						
						}
						
						else if(count > 0){
								
								
								count --;
							
							if(count == 0){
								
								
								Save.x_value++;
								fprintf(ofp, "%04X\n", *((uint16_t*)tail));
								
								//fprintf(dummy, "%d\n\n\n", *((uint16_t*)tail));
								fprintf(dummy,"%f %d\n\n\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
								
								//If program is past run time, do not look for more EODs
								gettimeofday(&sample, NULL); 
								elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
									
								if(elapsed >= Save.RunTime_ms){
								
									PlotGraph(ofp,dummy);
								}
							
							}
							else{
								
								
								Save.x_value++;
								fprintf(ofp,"%04X", *((uint16_t*)tail));
								
								//fprintf(dummy, "%d\n", *((uint16_t*)tail));
								fprintf(dummy,"%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
								
								
								
									
							}
								
								
						}
									
					
							currentTailPosition = currentTailPosition + 2;
							tail = map_base + (currentTailPosition & MAP_MASK);
					}	
					
				}
				
			}
			
			
			currentTailPosition = startofBuffer;
			tail = map_base + (currentTailPosition & MAP_MASK);
			
			last_read_result =0;
			
			
			
			for(i = 0; i < read_result; i+=2 ){
				
					if(Save.RunMode == 0){
						
						
						gettimeofday(&sample, NULL); 
						elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
						if(elapsed >= Save.RunTime_ms){
							
							PlotGraph(ofp,dummy);
						}
						else{
							
							fprintf(ofp, "%04X", *((uint16_t*)tail));
							//fprintf(dummy,"%d,", *((uint16_t*)tail));
							fprintf(dummy,"%d\n", *((uint16_t*)tail));
							//Move to the next sample
							currentTailPosition = currentTailPosition + 2;
							tail = map_base + (currentTailPosition & MAP_MASK);
						}
						
					}
					
					else{
						
						
						MeanCalculator(&mean,  &prev_mean, tail);
						StandardDeviation(&mean, &currentValue,tail,&SD);
							
						
						
						if(wrap_around > 0){
					
							
							wrap_around --;
							
							if(wrap_around== 0){
													
									Save.x_value++;				
									fprintf(ofp, "%04X\n", *((uint16_t*)tail));
									
									//fprintf(dummy, "%d\n\n\n", *((uint16_t*)tail));
									fprintf(dummy,"%f %d\n\n\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
									
									//If program is past run time, do not look for more EODs
									gettimeofday(&sample, NULL); 
									elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
									
									if(elapsed >= Save.RunTime_ms){
								
										PlotGraph(ofp,dummy);
									}
									
								}
								else{
									
									Save.x_value++;
									fprintf(ofp,"%04X", *((uint16_t*)tail));
									
									//fprintf(dummy, "%d\n", *((uint16_t*)tail));
									fprintf(dummy,"%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
									
								
								}
							
							
						}
						
						else{
							MeanCalculator(&mean,  &prev_mean, tail);
							StandardDeviation(&mean, &currentValue,tail,&SD);
							
							gettimeofday(&sample, NULL); 
							elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
							if(elapsed >= Save.RunTime_ms){
						
								PlotGraph(ofp,dummy);
							}
							
							if(abs(*((uint16_t*)tail)-(uint16_t)mean)>= THRESHOLD*SD && count ==0){
								
								gettimeofday(&sample, NULL); 
								elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
								if(elapsed >= Save.RunTime_ms){
							
									PlotGraph(ofp,dummy);
								}	
								else{
									fprintf(ofp, "%06X",elapsed);
									WritetoFile(tail,map_base, startofBuffer, currentTailPosition, ofp, &first_record,elapsed,start,sample, dummy);
									count = (Save.CAPTURE*0.6);
								}
								
								
							}
							
							else if(count > 0){
								
						
								count --;
								
								if(count == 0){
									
									Save.x_value++;
									fprintf(ofp, "%04X\n", *((uint16_t*)tail));
									
									//fprintf(dummy, "%d\n\n\n", *((uint16_t*)tail));
									fprintf(dummy,"%f %d\n\n\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
									
									//If program is past run time, do not look for more EODs
									gettimeofday(&sample, NULL); 
									elapsed = (sample.tv_sec - start.tv_sec) * 1000.0f + (sample.tv_usec - start.tv_usec) / 1000.0f;
									if(elapsed >= Save.RunTime_ms){
								
										PlotGraph(ofp,dummy);
									}
								
								}
								else{
									
									Save.x_value++;
									fprintf(ofp,"%04X", *((uint16_t*)tail));
									
									//fprintf(dummy, "%d\n", *((uint16_t*)tail));
									fprintf(dummy,"%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)tail));
									
									
								}
								
								
								
							}	
						
						}
						
						
						currentTailPosition = currentTailPosition + 2;
						tail = map_base + (currentTailPosition & MAP_MASK);
						
					}
				
			}
		
			
			
		}
		
		last_read_result = read_result;
		
		
		temp = read_result;
   
   
	}   
	    fflush(stdout);
   // Wait for event completion from PRU, returns the PRU_EVTOUT_0 number
   int n = prussdrv_pru_wait_event (PRU_EVTOUT_0);
   printf("EBBADC PRU0 program completed, event number %04X.\n", n);

// Disable PRU and close memory mappings 
   prussdrv_pru_disable(ADC_PRU_NUM);
   prussdrv_pru_disable(CLK_PRU_NUM);

   prussdrv_exit ();
   return EXIT_SUCCESS;

	
   
}

double MeanCalculator(double *mean,  double *prev_mean, void* tail){
	
	

	uint16_t saveTail = *((uint16_t*) tail);
	
	double alpha = 0.000001;
	
	*mean = (*prev_mean)*(1.0-alpha) + saveTail*alpha;
	
	
	*prev_mean = *mean;

	
	return *mean;
	
}

double StandardDeviation(double *mean, double*currentValue,void* tail, double *SD){
	
	uint16_t value = *((uint16_t*)tail);

	
	double beta = 0.000001;
	*currentValue = *currentValue*(1.0-beta) + beta*(value-*mean)*(value-*mean);
	
	*SD = sqrt(*currentValue);
	return *SD;
	
}

void WritetoFile(void* tail,void* map_base, off_t startofBuffer, off_t currentTailPosition, FILE* ofp,int *first_record,unsigned long long int elapsed, struct timeval start, struct timeval sample, FILE* dummy){
	
		off_t collectPosition = currentTailPosition;
		void* samplePosition = tail;
		
		off_t wrap_back = 0;
		int collect_count;
		int back_count;
		int count_limit;
		int sample_numer;
	
		//If there are not enough samples before our current position, record the number of available samples
		 if((collectPosition - startofBuffer)/2 < (Save.CAPTURE*0.4)){
			 
			 
			 //Flag to determine if this is the first run through the memory buffer
			 if(*first_record == 0){
			 
			 
				count_limit = (int)((collectPosition - startofBuffer)/2);
				
				
				collectPosition = startofBuffer;
				samplePosition =(map_base + (collectPosition & MAP_MASK));	
				
				if(count_limit >= (Save.CAPTURE*0.4)){
					
					
					*first_record =1;
				}
				
				else{
					
					//*first_record =0;
					
				
				
						
					for(collect_count = 0; collect_count < count_limit; collect_count++){
						
						
						Save.x_value = collect_count;
						fprintf(ofp, "%04X", *((uint16_t*)samplePosition));
						//fprintf(dummy, "%d\n", *((uint16_t*)samplePosition));
						//printf("%f", ((float)Save.x_value/Save.Samp_Rate));
						fprintf(dummy, "%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)samplePosition));
						
						
						collectPosition = collectPosition +2;
						samplePosition = (map_base + (collectPosition & MAP_MASK));	
						
						
						
					}
				}
				
				/*if(count_limit >= (Save.CAPTURE*0.6)){
					
					
					*first_record =1;
				}
				
				else{
					
					*first_record =0;
					
				}*/
					
			 
			}
			
			//Record values from end of the memory buffer
			else{
				
			
				wrap_back = (Save.CAPTURE*0.4) -((collectPosition - startofBuffer)/2);
				collectPosition = collectPosition + 2*(MAX_MEMORY-wrap_back);
				samplePosition =(map_base + (collectPosition & MAP_MASK));	
				
				
				for(back_count =0; back_count < wrap_back; back_count ++ ){
					
					
				
					Save.x_value = back_count;
					fprintf(ofp, "%04X", *((uint16_t*)samplePosition));
					//fprintf(dummy, "%d\n", *((uint16_t*)samplePosition));
					
					fprintf(dummy, "%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)samplePosition));
					
					collectPosition = collectPosition +2;
					samplePosition = (map_base + (collectPosition & MAP_MASK));	
					
					
				}
				
				collectPosition = startofBuffer;
				samplePosition =(map_base + (collectPosition & MAP_MASK));	
				
				
				for(collect_count =0 ; collect_count < ((Save.CAPTURE*0.4)-wrap_back); collect_count++){
					
					
					Save.x_value++;
					fprintf(ofp, "%04X", *((uint16_t*)samplePosition));
					//fprintf(dummy, "%d\n", *((uint16_t*)samplePosition));
					
					fprintf(dummy, "%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)samplePosition));
					
					
					collectPosition = collectPosition +2;
					samplePosition = (map_base + (collectPosition & MAP_MASK));	
					
				}
					
			}
			
			
			
		}
		
		//Record the values prior to the trigger
		else{
			
			collectPosition = collectPosition - 2*(Save.CAPTURE*0.4);
			samplePosition =(map_base + (collectPosition & MAP_MASK));	
			
			for(collect_count =0; collect_count < (Save.CAPTURE*0.4); collect_count++){
				
				
				Save.x_value = collect_count;
				fprintf(ofp, "%04X", *((uint16_t*)samplePosition));
				//fprintf(dummy, "%d\n", *((uint16_t*)samplePosition));
				
				fprintf(dummy, "%f %d\n",((float)Save.x_value/Save.Samp_Rate), *((uint16_t*)samplePosition));
					
				
				
				
				collectPosition = collectPosition +2;
				samplePosition = (map_base + (collectPosition & MAP_MASK));	
				
			}
			
			
		}
		
	
	
		
	
}

FILE* Header(){
	
	int i;
	FILE *ofp;
	char Filename[256];
	time_t start_t = time(NULL);
	struct tm start = *localtime(&start_t);
	//EDIT FILE LOCATION HERE
	sprintf(Filename, "/root/Data/%02d-%02d-%02d_%02d:%02d:%02d.EOD", start.tm_year+1900, start.tm_mon + 1, start.tm_mday, start.tm_hour, start.tm_min, start.tm_sec);
	
	ofp = fopen(Filename,"w");
	
	strcpy(Save.header,"Data format for data collection from a BeagleBone Black. Written  February 2016 by\n");
	strcat(Save.header,"Bailey Winter for the Electric Fish Project.  If run mode 0, Raw Data, is chosen, the Data\n");
	strcat(Save.header,"consists of the continuous change in voltage levels over the course of the code's run time.\n");
	strcat(Save.header,"If run mode 1, Spikes Only, is chosen, the data will consist of the electric organ discharges(EODs)\n"); 
	strcat(Save.header,"and a timestamp for each EOD.\n");
	strcat(Save.header,"This text description begins the file and describes version 1.\n");
	strcat(Save.header,"The main header fields begin at 2*1024 bytes.\n");
	strcat(Save.header,"The data begins at 101*1024 bytes.\n");
	strcat(Save.header,"At the beginning of the file.\n\n");
	strcat(Save.header,"uint16		Header version number\n");
	strcat(Save.header,"uint16		EOD reading software version number\n");
	strcat(Save.header,"char[256]	Name of file (format YYYY-MM-DD_HH:MM:SS)\n");
	strcat(Save.header,"char[256]	Experimenter Name\n");
	strcat(Save.header,"char[11]	Date (format YYYY-MM-DD)\n");
	strcat(Save.header,"char[6]		Time (format HH:MM)\n");
	strcat(Save.header,"char[256]	Species\n");
	strcat(Save.header,"char[5]		Specimen No.\n");
	strcat(Save.header,"char[256]	Specimen Locality Code\n");
	strcat(Save.header,"char[20]	Recording Temperature\n");
	strcat(Save.header,"int		Polarity(0 = normal, 1 = inverted)\n");
	strcat(Save.header,"char[20]	Coupling(AC or DC)\n");
	strcat(Save.header,"int		Gain\n");
	strcat(Save.header,"float		Sampling Rate(Hz)\n");
	strcat(Save.header,"int		Run mode (0 or 1)\n");
	strcat(Save.header,"float		Run time(min)\n");
	strcat(Save.header,"char[100000]	Comments\n");
	strcat(Save.header,"char[823]	PADDING\n\n");
	strcat(Save.header,"Data description:\n\n");
	strcat(Save.header,"Sample Timestamp(3 bytes):	Records how many milliseconds from the\n");
	strcat(Save.header,"					execution of the program the EOD occurs.\n");
	strcat(Save.header,"Waveform(800 bytes):		These values represent the various sample values \n");
	strcat(Save.header,"					that form the complete EOD.\n");
	
	
	fprintf(ofp,Save.header);
	
	//PADDING
	for(i=0;i<731;i++){
		
		fprintf(ofp," ");
	}
	fprintf(ofp,"\n");
	
	fprintf(ofp,"%d\n",Save.Header_Version);
	fprintf(ofp,"%d\n",Save.Code_Version);

	fprintf(ofp,"%s",Filename);
	
	//Padding onto the rest of Filename
	for(i = 0; i < 256-strlen(Filename); i++)
	{
		fprintf(ofp," ");	
	}
	fprintf(ofp,"\n");
		
	printf("Enter the Experimenter's name: ");
	
	scanf("%s",&Save.Experimenter_name);
	fprintf(ofp,"%s",Save.Experimenter_name);
	
	for(i = 0; i < 100-strlen(Save.Experimenter_name); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
			
	sprintf(Save.Date, "%02d-%02d-%02d", start.tm_year+1900, start.tm_mon + 1, start.tm_mday);
	fprintf(ofp, "%s", Save.Date);
	fprintf(ofp,"\n");

	sprintf(Save.Time, "%02d:%02d",start.tm_hour, start.tm_min);
	fprintf(ofp, "%s", Save.Time);
	fprintf(ofp,"\n");
	
	printf("Enter the species' name:");
	scanf("%s",&Save.Species);
	fprintf(ofp,"%s",Save.Species);
	for(i = 0; i < 256-strlen(Save.Species); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
	printf("Enter the Specimen Number:");
	scanf("%s",&Save.SpecimenNo);
	fprintf(ofp,"%s",Save.SpecimenNo);
	for(i = 0; i < 5-strlen(Save.SpecimenNo); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
	printf("Enter the Specimen Locality Code:");
	scanf("%s",&Save.SpecimenLocal);
	fprintf(ofp,"%s",Save.SpecimenLocal);
	for(i = 0; i < 256-strlen(Save.SpecimenLocal); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
	printf("Enter the recording temperature:");
	scanf("%s",&Save.RecordingTemp);
	fprintf(ofp,"%s",Save.RecordingTemp);
	for(i = 0; i < 20-strlen(Save.RecordingTemp); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
	printf("Enter the Polarity(0 = normal, 1 = inverted):");
	scanf("%d",&Save.Polarity);
	fprintf(ofp,"%d",Save.Polarity);
	fprintf(ofp,"\n");
	
	printf("Enter the coupling type:");
	scanf("%s",&Save.Coupling);
	fprintf(ofp,"%s",Save.Coupling);
	
	for(i = 0; i < 20-strlen(Save.Coupling); i++)
	{
		fprintf(ofp," ");	
	}	
	fprintf(ofp,"\n");
	
	printf("Enter the gain:");
	scanf("%d",&Save.Gain);
	fprintf(ofp,"%d",Save.Gain);
	fprintf(ofp,"\n");
	
	
	printf("Enter the sampling rate as listed:\n");
	//TABLE OF SAMPLING RATE INPUTS
	printf("1Msps   = FREQ_1MHz\n");
	printf("500ksps = FREQ_500kHz\n");
	printf("250ksps = FREQ_250kHz\n");
	printf("100ksps = FREQ_100kHz\n");
	scanf("%s",&Save.Sample_Rate);
	
	
	
	if(strcmp(Save.Sample_Rate, "FREQ_1MHz") == 0){
		
		Save.Samp_Rate = 1000000;
		fprintf(ofp,"%d",Save.Samp_Rate);
	}
	else if(strcmp(Save.Sample_Rate, "FREQ_500kHz")==0){
		Save.Samp_Rate = 500000;
		fprintf(ofp,"%d",Save.Samp_Rate);
	}
	else if(strcmp(Save.Sample_Rate, "FREQ_250kHz")==0){
		Save.Samp_Rate = 250000;
		fprintf(ofp,"%d",Save.Samp_Rate);
	}
	else if(strcmp(Save.Sample_Rate, "FREQ_100kHz")==0){
		Save.Samp_Rate = 100000;
		fprintf(ofp,"%d",Save.Samp_Rate);
	}
	
	/*for(i = 0; i < 20-strlen(Save.Sample_Rate); i++)
	{
		fprintf(ofp," ");	
	}*/	
	fprintf(ofp,"\n");
	/*else{
		printf("ERROR: Sample rate option does not exist.\n ")
		printf("Removing file and restarting program.")
	}*/

	Save.CAPTURE = Save.Samp_Rate*0.004;
	printf("Enter the run time(min):\n");
	scanf("%f",&Save.RunTime);
	fprintf(ofp,"%f\n",Save.RunTime);
	
	Save.RunTime_ms = Save.RunTime*60*1000;
	
	printf("Enter the run mode(0 = Raw Data, 1 = Spikes Only):\n");
	scanf("%d",&Save.RunMode);
	fprintf(ofp,"%d\n",Save.RunMode);
	
	
	
	printf("Comments(100000 character limit):\n");
	scanf("%s",&Save.Comments);
	fprintf(ofp,"%s",Save.Comments);
	for(i = 0; i < 100000-strlen(Save.Comments); i++)
	{
		fprintf(ofp," ");	
	}	
	
	fprintf(ofp,"\n");
	
	//PADDING
	for(i = 0; i < 410; i++)
	{
		fprintf(ofp, " ");	
	}
	fprintf(ofp,"\n");
	
	return ofp;
	
}

void PlotGraph(FILE* ofp, FILE* dummy){
	
	fclose(ofp);
	fclose(dummy);
	if (Save.RunMode == 1){
		
		
		system("gnuplot -p 'plotscript.gp'");
		remove("dummy.dat");
		printf("Hit return while in the command line to close  the program\n");
		printf("Exiting program.\n");
	
		exit(0);
	}
	//system("gnuplot -persist 'plotscript.gp'");
	
	printf("Exiting program.\n");
	
	exit(0);
	
	
}




