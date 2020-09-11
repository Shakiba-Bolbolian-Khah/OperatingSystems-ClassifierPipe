#include <vector>
#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iomanip>
#include <errno.h>

#define CLASSIFIERNAME "/classifier_"
#define TYPE ".csv"
#define DATASET "/dataset.csv"
#define DELIMETER "-"
#define NAMEDPIPEADD "/tmp/pipeClassifier"
#define VOTERNAMEDPIPE "/tmp/pipeVoter"
#define MSGSIZE 2048

using namespace std;

char* strToChar(string s){
    char * str;
    str = (char*) malloc(MSGSIZE * sizeof(int));
    strcpy(str, s.c_str());
    return str;
}

void makeNamedPipe(string fifoName){
    const char* name = strToChar(fifoName);
    int fd = open(name, O_RDWR | O_CREAT , 0666);
    if(fd < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }
    close(fd);
    mkfifo(name , 0666);
}

void sendDataForVoter(string message , string namedPipeName){
    int fd;
    char* name = strToChar(namedPipeName);
    fd = open(name, O_WRONLY);
    char* msg = (char*)(message.c_str());
    if(write(fd, msg , strlen(msg)) < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }
    close(fd);
}

vector<char*> split(char *s){
    vector <char*> args;
    char* temp;
    temp = strtok(s, "-");
    args.push_back(temp);
    while(temp != NULL){
        temp = strtok(NULL, "-");
        args.push_back(temp);
    }
    return args;
}

vector<int> readFromPipe(char* pipeName){
    char msg[MSGSIZE]="";
    int fd = open(pipeName, O_RDONLY);
    if(read(fd, msg, MSGSIZE) < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }
    vector<char*> datas = split(msg);
    vector<int> calculatedLabels;

    for(int i = 0;i <datas.size()-1 ;i++){
        int k = atoi(datas[i]);
        calculatedLabels.push_back(k);
    }
    return calculatedLabels;
}

vector<int> readFiles(string address){
    
    string line;
    fstream fin;
    fin.open(address, ios::in);
    getline(fin, line);
    vector<int> data;

    while (getline(fin, line))
        data.push_back(stoi(line));
    fin.close();
    return data;   
}


double calculateAccuracy(vector<int> labels, vector<int> calculatedResult){
    float numOfData = calculatedResult.size();
    float error = 0;
    for(int i = 0; i < numOfData; i++){
        if(calculatedResult[i] != labels[i])
            error++;
    }
    float accuracy = ((numOfData - error)/numOfData)*100;
    return accuracy;
}


int main(int argc, char* argv[]){
    if(argc < 3){
        cout<<"Ensemble Classifier: Invalid num of arguments" << endl;
        exit(0);
    }

    int pid, fileNum = 0, nbytes;
    string validationAdd = argv[1], weightAdd = argv[2];
    string fileName = weightAdd + CLASSIFIERNAME + to_string(fileNum) + TYPE;
    string dataSetFileName =  validationAdd + DATASET;
    vector<char*> pipesForVoter;
    FILE *classifierFile;

    while( classifierFile = fopen(fileName.c_str(), "r")){
        int p[2];

        if( pipe(p) < 0){
            cout << "Creating pipe failed for classifier: " << fileName << endl;
            exit(1);
        }
        string fifoName = NAMEDPIPEADD + to_string(fileNum);
        pipesForVoter.push_back(strToChar(fifoName));


        if( (pid = fork()) == 0 ){
            //Named Pipe for voter

            
            makeNamedPipe(fifoName);
            


            char message[MSGSIZE];
            close(p[1]);
            if( (nbytes = read(p[0], message, MSGSIZE))> 0){
                char* args[] = {"./linearClassifier" , message, strToChar(fifoName), NULL};
                execv(args[0], args);
            }
            else if( nbytes < 0){
                cout << "Error in reading from pipe in classifier: " << fileName << endl;
                exit(1);
            }
            close(p[0]);

        }
        else if(pid > 0){
            close(p[0]);
            string message = fileName + DELIMETER + dataSetFileName + DELIMETER;

            if( (nbytes = write(p[1], message.c_str(), message.size() )) < 0){
                cout << "Error in wrting from ensemble classifier in pipe for classifier: " << fileName << endl;
                exit(1);
            }
            close(p[1]);
        }   
        else{
            cout<<"Error in fork for file: "<< fileName << endl;
            exit(1);
        }

        fclose(classifierFile);
        fileNum++;
        fileName = weightAdd + CLASSIFIERNAME + to_string(fileNum) + TYPE;
    }

    for( int i = 0; i < fileNum ; i++)
        wait(NULL);


    string voterFifoName = VOTERNAMEDPIPE;
    makeNamedPipe(voterFifoName);
    string message = validationAdd + "/labels.csv"+ DELIMETER + voterFifoName + DELIMETER;
    sendDataForVoter(message, voterFifoName);
    char** args = new char*[3+ pipesForVoter.size()];
    
    int pidVoter = fork();
    if(pidVoter > 0){
        wait(NULL);
        vector<int> calculatedResult = readFromPipe(strToChar(voterFifoName));
        string labelsAddr = validationAdd + "/labels.csv";
        vector<int> realLabels = readFiles(labelsAddr);
        double accuracy = calculateAccuracy(realLabels, calculatedResult);
        cout << "Accuracy: " ;
        cout.setf(ios::fixed, ios::floatfield);
        cout.precision(2);
        cout << accuracy<<"%"<<endl;
        
    }
    else if(pidVoter == 0)
    {
        args[0] = strToChar("./voter");
        char msg[MSGSIZE]="";
        char* name = strToChar(voterFifoName);
        int fd = open(name, O_RDONLY);
        if(read(fd, msg, MSGSIZE) < 0){
            cout << strerror(errno) << endl;
            exit(1);
        }
        close(fd);
        args[1] = msg;
        
        for(int i = 0; i < pipesForVoter.size(); i++)
            args[2+i] = pipesForVoter[i];
        
        args[2+pipesForVoter.size()] = NULL;

        execv(args[0],args);
        
    }
    
    
}