//
//  Translate.cpp
//  TestParsing
//
//  Original program created by Stanislav Mircic on 6/9/15.
//	Modified by Bailey Winter March 2016 for the Electric Fish Project
//  Copyright (c) 2015 BackyardBrains. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring> // for std::strlen
#include <cstddef> // for std::size_t -> is a typedef on an unsinged int

using namespace std ;

int main(int argc, char* argv[]) {
   /* cout << "argc = " << argc << endl;
    for(int i = 0; i < argc; i++)
        cout << "argv[" << i << "] = " << argv[i] << endl;
    */
    cout<<"Usage: TestParsing [name-of-input-file] [optional-offset(default 5120)]\n";
    if(argc<2)
    {
        cout << "Error: Specify input file.\n";
        exit(1);
    }
    
    ofstream myfile;
    myfile.open ("Translate.csv", ios::out|ios::trunc);
    
    int initialOffset = 101*1024;
    if(argc>2)
    {
        initialOffset = strtol (argv[2],NULL,10);
    }
    
    ifstream inFile;
    inFile.open( argv[1], ios::in|ios::binary|ios::ate );
    if (!inFile) {
        cerr << "Can't open input file " << argv[1] << endl;
        exit(1);
    }
    
    string oData = " ";
    
    size_t size = 0;     
    inFile.seekg(0, ios::end); // set the pointer to the end
    size = inFile.tellg() ; // get the length of the file
    cout << "Size of file: " << size<<"\n";
    inFile.seekg(0, ios::beg); // set the pointer to the beginning
	
	std::stringstream tempFile;
	tempFile << inFile.rdbuf();
	oData = tempFile.str();
    
   
    std::cout<<"Reading file...\n";

    if (inFile)
        std::cout << "all characters read successfully.\n";
    else
        std::cout << "error: only " << inFile.gcount() << " could be read\n";
    
    cout << " oData size: " << oData.size() << "\n";

	long int timestamp;//3 bytes
    long int wave;//2 bytes
	

    
    int index;
    string buffer = " ";
	string bufferShort = " ";

    long displayIndex = 0;

	int flag = 1;
	int i;
	std::stringstream tempTime;
	std::stringstream tempWave;
	int run_mode;
	printf("Enter the run mode: ");
	scanf("%d", &run_mode);

	
	
	
	
    for (  i = initialOffset; i < size; i= i++)
    {

		
		 if(oData.at(i) == '\n'){
			
			myfile << '\n';
			flag =1;
		}
		
		
		
		else{
			
				if( run_mode = 1) {
					
					if(flag >0){
				
				
						buffer = oData.substr(i,6);
						timestamp = strtol(buffer.c_str(),NULL,16);
						
				
						  myfile<<timestamp<<",";
						
						  flag = 0;
						  i+=5;
						
					}
					
				
							
							
						   
					 else{
								   
								   
						   bufferShort = oData.substr(i,4);
						   wave =strtol(bufferShort.c_str(),NULL,16);
							
							myfile<<wave<<",";
							
							i+=3;
								   
								   
					} 
										
				
				}
			
			/*if(flag >0){
			
			
			buffer = oData.substr(i,6);
			timestamp = strtol(buffer.c_str(),NULL,16);
			
	
			  myfile<<timestamp<<",";
			
			  flag = 0;
			  i+=5;
			}*/
		
		
       
		  else{
			   
			   
			   bufferShort = oData.substr(i,4);
			   wave =strtol(bufferShort.c_str(),NULL,16);
		
				myfile<<wave<<",";
		
				i+=3;
			   
			   
		   } 
			
		} 
        
              
        displayIndex++;
      /*  if(displayIndex%1000 == 0)
        {
            cout<<(timestamp/60000)/60<<":"<<(timestamp/60000)%60<<":"<<(timestamp/1000)%60<<"\n";
        }*/
	
		

        
    }
    myfile.close();
    cout<<"\n File converted and saved in Translate.csv\n";
    return 0;
}