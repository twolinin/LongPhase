#include "PhasingProcess.h"
#include "PhasingGraph.h"
#include "ParsingBam.h"

PhasingProcess::PhasingProcess(PhasingParameters params)
{
    // load SNP vcf file
    std::time_t begin = time(NULL);
    std::cerr<< "parsing VCF ... ";
    SnpParser snpFile(params);
    std::cerr<< difftime(time(NULL), begin) << "s\n";
    
    // load SV vcf file
    begin = time(NULL);
    std::cerr<< "parsing SV VCF ... ";
    SVParser svFile(params, snpFile);
    std::cerr<< difftime(time(NULL), begin) << "s\n";
 
    //Parse mod vcf file
	begin = time(NULL);
	std::cerr<< "parsing Meth VCF ... ";
	METHParser modFile(params, snpFile);
	std::cerr<< difftime(time(NULL), begin) << "s\n";
 
    // parsing ref fasta 
    begin = time(NULL);
    std::cerr<< "reading reference ... ";
    std::vector<int> last_pos;
    for(auto chr :snpFile.getChrVec()){
        last_pos.push_back(snpFile.getLastSNP(chr));
    }
    FastaParser fastaParser(params.fastaFile , snpFile.getChrVec(), last_pos);
    std::cerr<< difftime(time(NULL), begin) << "s\n";

    // get all detected chromosome
    std::vector<std::string> chrName = snpFile.getChrVec();

    // record all phasing result
    ChrPhasingResult chrPhasingResult;
    // Initialize an empty map in chrPhasingResult to store all phasing results.
    // This is done to prevent issues with multi-threading, by defining an empty map first.
    for (std::vector<std::string>::iterator chrIter = chrName.begin(); chrIter != chrName.end(); chrIter++)    {
        chrPhasingResult[*chrIter] = PhasingResult();
    }
    
    // set chrNumThreads and bamParserNumThreads based on parameters
    int chrNumThreads,bamParserNumThreads;
    setNumThreads(chrName.size(), params.numThreads, chrNumThreads, bamParserNumThreads);
    begin = time(NULL);
    
    // loop all chromosome
    #pragma omp parallel for schedule(dynamic) num_threads(chrNumThreads)
    for(std::vector<std::string>::iterator chrIter = chrName.begin(); chrIter != chrName.end() ; chrIter++ ){
        
        std::time_t chrbegin = time(NULL);
        
        // get lase SNP variant position
        int lastSNPpos = snpFile.getLastSNP((*chrIter));
        // therer is no variant on SNP file. 
        if( lastSNPpos == -1 ){
            continue;
        }

        // create a bam parser object and prepare to fetch varint from each vcf file
        BamParser *bamParser = new BamParser((*chrIter), params.bamFile, snpFile, svFile, modFile);
        // fetch chromosome string
        std::string chr_reference = fastaParser.chrString.at(*chrIter);
        // use to store variant
        std::vector<ReadVariant> readVariantVec;
        // run fetch variant process
        bamParser->direct_detect_alleles(lastSNPpos, bamParserNumThreads, params, readVariantVec ,chr_reference);
        // free memory
        delete bamParser;
        
        // filter variants prone to switch errors in ONT sequencing.
        if(params.isONT){
            snpFile.filterSNP((*chrIter), readVariantVec, chr_reference);
        }

        // bam files are partial file or no read support this chromosome's SNP
        if( readVariantVec.size() == 0 ){
            continue;
        }
        
        // create a graph object and prepare to phasing.
        VairiantGraph *vGraph = new VairiantGraph(chr_reference, params);
        // trans read-snp info to edge info
        vGraph->addEdge(readVariantVec);
        // run main algorithm
        vGraph->phasingProcess();
        // push result to phasingResult
        vGraph->exportResult((*chrIter), chrPhasingResult[*chrIter]);
        // generate dot file
        if(params.generateDot){
            vGraph->writingDotFile((*chrIter));
        }
        
        // release the memory used by the object.
        vGraph->destroy();
        
        // free memory
        readVariantVec.clear();
        readVariantVec.shrink_to_fit();
        delete vGraph;
        
        std::cerr<< "(" << (*chrIter) << "," << difftime(time(NULL), chrbegin) << "s)";
    }
    
    std::cerr<< "\nparsing total:  " << difftime(time(NULL), begin) << "s\n";
    
    begin = time(NULL);
    std::cerr<< "merge results ... ";
    // Create a container for merged phasing results.
    PhasingResult mergedPhasingResult;
    // Merge phasing results from all chromosomes.
    mergeAllChrPhasingResult(chrPhasingResult, mergedPhasingResult);
    std::cerr<< difftime(time(NULL), begin) << "s\n";
    
    begin = time(NULL);
    std::cerr<< "writeResult SNP ... ";
    snpFile.writeResult(mergedPhasingResult);
    std::cerr<< difftime(time(NULL), begin) << "s\n";
    
    if(params.svFile!=""){
        begin = time(NULL);
        std::cerr<< "write SV Result ... ";
        svFile.writeResult(mergedPhasingResult);
        std::cerr<< difftime(time(NULL), begin) << "s\n";
    }
    
    if(params.modFile!=""){
        begin = time(NULL);
        std::cerr<< "write mod Result ... ";
        modFile.writeResult(mergedPhasingResult);
        std::cerr<< difftime(time(NULL), begin) << "s\n";
    }

    return;
};

PhasingProcess::~PhasingProcess(){
};

