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
    if(temp != NULL)
        temp = strtok(NULL, "-");
    args.push_back(temp);
    return args;
}

vector<vector<double> > readFiles(string address){
    
    string line, word;
    fstream fin;
    fin.open(address, ios::in);
    getline(fin, line);
    vector< vector<double> > data;

    while (getline(fin, line))
    {
        stringstream str(line);
        vector<double> row;
        while(getline(str, word, ',')){
            // cout<< word << endl;
            row.push_back(stod(word));
        }        
        if(row.size() != 0)
            data.push_back(row);

    }
    fin.close();
    return data;   
}

vector<double> findScore(vector<double> data, vector<vector<double>> weights){
    vector<double> scores;
    
    for(int j = 0; j<weights.size(); j++){
        double score = 0;
        for(int i = 0; i<data.size(); i++){
            score += data[i]* weights[j][i];
        }
        score += weights[j][(weights[j].size() - 1)];
        scores.push_back(score);
    }
    return scores;
}

int findMax(vector<double> scores){
    double max = scores[0];
    int label = 0;
    for(int i = 1; i<scores.size(); i++){
        if(scores[i] > max){
            max = scores[i];
            label = i;
        }
    }
    return label;
}


vector<int> findlabels(vector<vector<double> > weights, vector<vector<double> >dataset){
    vector<int> labels;
    for(int i = 0; i<dataset.size();i++){
        vector<double> scores = findScore(dataset[i], weights);
        
        // cout << "SIZE:"<<scores.size() <<"---" << scores[0] <<"---"<<scores[1] << "---" << scores[2] << endl;
        
        int label = findMax(scores);
        labels.push_back(label);
        
        // cout << label;
    }
    
    // cout<<endl;
    // cout<<labels.size() << endl;
    
    return labels;
}

void sendDataForVoter(vector<int> labels, string namedPipeName){
    int fd;
    fd = open(namedPipeName.c_str(), O_WRONLY);
    for(int i = 0; i< labels.size(); i++){
        char* label = (char*)(to_string(labels[i]).c_str());
        write(fd, label, strlen(label));
        write(fd, "-", sizeof(char));
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc < 2){
        cout<<"Linear Classifier: Invalid num of arguments" << endl;
        exit(0);
    }
    // cout<<"here in linear" << endl;
    
    vector<char*> args = split( argv[1]);
    string classifierAddr = args[0];
    string datasetAddr = args[1];
    string namedPipeName = argv[2];
    vector< vector<double> > weights = readFiles( classifierAddr);
    vector< vector<double> > dataset = readFiles( datasetAddr);

    vector<int> labels = findlabels(weights, dataset);

    sendDataForVoter(labels, namedPipeName);

}   