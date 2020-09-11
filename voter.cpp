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

#define CLASSIFIERNAME "/classifier_"
#define TYPE ".csv"
#define DATASET "/dataset.csv"
#define MSGSIZE 2048

using namespace std;


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

int findNumberOfClass(vector<int> classifierResult){
    int num = 0;
    for( int i = 0; i < classifierResult.size(); i++){
        if(classifierResult[i] > num)
            num = classifierResult[i];
    }
    return num;
}

int findMax(vector<int> frequency){
    int max = 0;
    int maxIndex = 0;
    for(int i =0; i < frequency.size(); i++){
        if(frequency[i] > max){
            max = frequency[i];
            maxIndex = i;
        }
    }
    return maxIndex;
}


int vote(vector<vector<int> > calculatedLabels, int sample){
    int classNumbers = findNumberOfClass(calculatedLabels[0])+1;
    vector<int> frequency( classNumbers,0);
    for(int i = 0; i < calculatedLabels.size();i++){
        frequency[ (calculatedLabels[i][sample])]++;
    }
    return findMax(frequency);
}

void sendDataForEnsmbler(vector<int> result, string namedPipeName){
    int fd;
    fd = open(namedPipeName.c_str(), O_WRONLY);
    for(int i = 0; i< result.size(); i++){
        char* res = (char*)(to_string(result[i]).c_str());
        if(write(fd, res, strlen(res))<0){
            cout<< strerror(errno) << endl;
            exit(1);
        }
        if(write(fd, "-", sizeof(char))<0){
            cout<< strerror(errno) << endl;
            exit(1);
        }
    }
    close(fd);
}


int main(int argc, char* argv[]){
    if(argc < 2){
        cout<<"Voter: Invalid num of arguments" << endl;
        exit(0);
    }
    vector<char*> args=split(argv[1]);
    string labelAdd = args[0], namedPipeName = args[1];

    vector<vector<int> > calculatedLabels;
    for(int i = 2; i < argc; i++){
        calculatedLabels.push_back(readFromPipe(argv[i]));    
    }

    vector<int> result;

    for (int i =0; i < calculatedLabels[0].size() ;i++){
        result.push_back(vote(calculatedLabels, i));
    }

    sendDataForEnsmbler(result, namedPipeName);
    

    return 0;
}