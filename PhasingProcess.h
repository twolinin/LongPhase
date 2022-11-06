#ifndef PHASINGPROCESS_H
#define PHASINGPROCESS_H
#include "Util.h"



struct PhasingParameters
{
    int numThreads;
    int distance;
    int crossSNP;
    std::string snpFile;
    std::string svFile;
    std::string bamFile;
    std::string fastaFile;
    std::string resultPrefix;
    bool generateDot;
    bool isONT;
    bool isPB;
    
    int connectAdjacent;
    int mappingQuality;
    
    double confidentHaplotype;
    double judgeInconsistent;
    int inconsistentThreshold;
    
    double alleleConsistentRatio;
    double maxAlleleRatio;
    
    double readsThreshold;
    
    std::string version;
    std::string command;
};

class PhasingProcess
{

    public:
        PhasingProcess(PhasingParameters params);
        ~PhasingProcess();

};


#endif