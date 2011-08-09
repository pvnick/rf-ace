#include <cstdlib>
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <iomanip>
//#include <omp.h>

#include "argparse.hpp"
#include "randomforest.hpp"
#include "GBT.hpp"
#include "treedata.hpp"
#include "datadefs.hpp"

using namespace std;
using datadefs::num_t;

const bool GENERAL_DEFAULT_PRINT_HELP = false;
const bool GENERAL_DEFAULT_NO_RF = false;
const bool GENERAL_DEFAULT_NO_GBT = false; // TEMPORARY VARIABLE

const bool   RF_IS_OPTIMIZED_NODE_SPLIT = false;
const size_t RF_DEFAULT_N_TREES = 0; // zero means it will be estimated from the data by default
const size_t RF_DEFAULT_M_TRY = 0; // same here ...
const size_t RF_DEFAULT_NODE_SIZE = 0; // ... and here
const size_t RF_DEFAULT_N_PERMS = 1;
const num_t  RF_DEFAULT_P_VALUE_THRESHOLD = 0.10;

const bool   GBT_IS_OPTIMIZED_NODE_SPLIT = false;
const size_t GBT_DEFAULT_N_TREES = 100;
const size_t GBT_DEFAULT_N_MAX_LEAVES = 6;
const num_t  GBT_DEFAULT_SHRINKAGE = 0.5;
const num_t  GBT_DEFAULT_SUB_SAMPLE_SIZE = 0.5;

struct General_options {

  bool printHelp;
  string printHelp_s;
  string printHelp_l;

  string input;
  string input_s;
  string input_l;

  string output;
  string output_s;
  string output_l;

  string targetStr;
  string targetStr_s;
  string targetStr_l;

  bool noRF;
  string noRF_s;
  string noRF_l;

  bool noGBT;
  string noGBT_s;
  string noGBT_l;

  General_options():
    printHelp(GENERAL_DEFAULT_PRINT_HELP),
    printHelp_s("h"),
    printHelp_l("help"),    
    
    input(""),
    input_s("I"),
    input_l("input"),
    
    output(""),
    output_s("O"),
    output_l("output"),
    
    targetStr(""),
    targetStr_s("i"),
    targetStr_l("target"),
    
    noRF(GENERAL_DEFAULT_NO_RF),
    noRF_s("f"),
    noRF_l("noRF"),
    
    noGBT(GENERAL_DEFAULT_NO_GBT),
    noGBT_s("g"),
    noGBT_l("noGBT") {}
};

struct RF_options {
  
  bool isOptimizedNodeSplit;
  string isOptimizedNodeSplit_s;
  string isOptimizedNodeSplit_l;
  
  size_t nTrees;
  string nTrees_s;
  string nTrees_l;
  
  size_t mTry;
  string mTry_s;
  string mTry_l;

  size_t nodeSize;
  string nodeSize_s;
  string nodeSize_l;

  size_t nPerms;
  string nPerms_s;
  string nPerms_l;

  num_t pValueThreshold;
  string pValueThreshold_s;
  string pValueThreshold_l;

  RF_options():
    isOptimizedNodeSplit(RF_IS_OPTIMIZED_NODE_SPLIT),
    isOptimizedNodeSplit_s("o"),
    isOptimizedNodeSplit_l("RF_optimize"),

    nTrees(RF_DEFAULT_N_TREES),
    nTrees_s("n"),
    nTrees_l("RF_ntrees"),
    
    mTry(RF_DEFAULT_M_TRY),
    mTry_s("m"),
    mTry_l("RF_mtry"),

    nodeSize(RF_DEFAULT_NODE_SIZE),
    nodeSize_s("s"),
    nodeSize_l("RF_nodesize"),

    nPerms(RF_DEFAULT_N_PERMS),
    nPerms_s("p"),
    nPerms_l("RF_nperms"),

    pValueThreshold(RF_DEFAULT_P_VALUE_THRESHOLD),
    pValueThreshold_s("t"),
    pValueThreshold_l("RF_pthreshold") {}
  
};

struct GBT_options {

  bool isOptimizedNodeSplit;
  string isOptimizedNodeSplit_s;
  string isOptimizedNodeSplit_l;

  size_t nTrees;
  string nTrees_s;
  string nTrees_l;
  
  size_t nMaxLeaves;
  string nMaxLeaves_s;
  string nMaxLeaves_l;
  
  num_t shrinkage;
  string shrinkage_s;
  string shrinkage_l;
  
  num_t subSampleSize;
  string subSampleSize_s;
  string subSampleSize_l;

  GBT_options():
    isOptimizedNodeSplit(GBT_IS_OPTIMIZED_NODE_SPLIT),
    isOptimizedNodeSplit_s("b"),
    isOptimizedNodeSplit_l("GBT_optimize"),
    
    nTrees(GBT_DEFAULT_N_TREES),
    nTrees_s("r"),
    nTrees_l("GBT_ntrees"),
    
    nMaxLeaves(GBT_DEFAULT_N_MAX_LEAVES),
    nMaxLeaves_s("l"),
    nMaxLeaves_l("GBT_maxleaves"),
    
    shrinkage(GBT_DEFAULT_SHRINKAGE),
    shrinkage_s("z"),
    shrinkage_l("GBT_shrinkage"),

    subSampleSize(GBT_DEFAULT_SUB_SAMPLE_SIZE),
    subSampleSize_s("u"),
    subSampleSize_l("GBT_samplesize") {}
};

class Progress {
public:
  Progress(): width_(3) { cout << setw(width_) << "0" << "%" << flush; }
  ~Progress() { reset(); }

  void update(const num_t fraction) { reset(); cout << setw(width_) << static_cast<size_t>(fraction*100) << "%" << flush; }

private:
  
  void reset() { for(size_t i = 0; i <= width_; ++i) { cout << "\b"; } }

  size_t width_;

};


void printHeader()
{
  cout << endl;
  cout << " --------------------------------------------------------------- " << endl;
  cout << "| RF-ACE -- efficient feature selection with heterogeneous data |" << endl;
  cout << "|                                                               |" << endl;
  cout << "|  Version:      RF-ACE v0.7.5, August 9th, 2011                |" << endl;
  cout << "|  Project page: http://code.google.com/p/rf-ace                |" << endl;
  cout << "|  Contact:      timo.p.erkkila@tut.fi                          |" << endl;
  cout << "|                kari.torkkola@gmail.com                        |" << endl;
  cout << "|                                                               |" << endl;
  cout << "|              DEVELOPMENT VERSION, BUGS EXIST!                 |" << endl;
  cout << " --------------------------------------------------------------- " << endl;
}

void printHelp(const General_options& geno, const RF_options& rfo, const GBT_options& gbto)
{

  size_t maxwidth = 20;

  cout << endl;
  cout << "REQUIRED ARGUMENTS:" << endl;
  cout << " -" << geno.input_s << " / --" << geno.input_l << setw( maxwidth - geno.input_l.size() ) << " " << "Input feature file (AFM or ARFF)" << endl;
  cout << " -" << geno.targetStr_s << " / --" << geno.targetStr_l << setw( maxwidth - geno.targetStr_l.size() ) << " " << "Target, specified as integer or string that is to be matched with the content of input" << endl;
  cout << " -" << geno.output_s << " / --" << geno.output_l << setw( maxwidth - geno.output_l.size() ) << " " << "Output association file" << endl;
  cout << endl;
  cout << "OPTIONAL ARGUMENTS -- RF FILTER:" << endl;
  cout << " -" << geno.noRF_s << " / --" << geno.noRF_l << setw( maxwidth - geno.noRF_l.size() ) << " " << "Set this flag to turn RF filtering OFF (default ON)" << endl; 
  cout << " -" << rfo.isOptimizedNodeSplit_s << " / --" << rfo.isOptimizedNodeSplit_l << setw( maxwidth - rfo.isOptimizedNodeSplit_l.size() ) << " " << "Perform optimized node splitting with Random Forests (default NO)" << endl;
  cout << " -" << rfo.nTrees_s << " / --" << rfo.nTrees_l << setw( maxwidth - rfo.nTrees_l.size() ) << " " << "Number of trees per RF (default 10*nsamples/nrealsamples)" << endl;
  cout << " -" << rfo.mTry_s << " / --" << rfo.mTry_l << setw( maxwidth - rfo.mTry_l.size() ) << " " << "Number of randomly drawn features per node split (default sqrt(nfeatures))" << endl;
  cout << " -" << rfo.nodeSize_s << " / --" << rfo.nodeSize_l << setw( maxwidth - rfo.nodeSize_l.size() ) << " " << "Minimum number of train samples per node, affects tree depth (default max{5,nsamples/100})" << endl;
  cout << " -" << rfo.nPerms_s << " / --" << rfo.nPerms_l << setw( maxwidth - rfo.nPerms_l.size() ) << " " << "Number of Random Forests (default " << RF_DEFAULT_N_PERMS << ")" << endl;
  cout << " -" << rfo.pValueThreshold_s << " / --" << rfo.pValueThreshold_l << setw( maxwidth - rfo.pValueThreshold_l.size() ) << " " << "p-value threshold below which associations are listed (default " << RF_DEFAULT_P_VALUE_THRESHOLD << ")" << endl;
  cout << endl;
  cout << "OPTIONAL ARGUMENTS -- GBT:" << endl;
  cout << " -" << geno.noGBT_s << " / --" << geno.noGBT_l << setw( maxwidth - geno.noGBT_l.size() ) << " " << "Enable Gradient Boosting Trees (default ON)" << endl;
  cout << " -" << gbto.isOptimizedNodeSplit_s << " / --" << gbto.isOptimizedNodeSplit_l << setw( maxwidth - gbto.isOptimizedNodeSplit_l.size() ) << " " << "Perform optimized node splitting with Gradient Boosting Trees (default NO)" << endl;
  cout << " -" << gbto.nTrees_s << " / --" << gbto.nTrees_l << setw( maxwidth - gbto.nTrees_l.size() ) << " " << "Number of trees in the GBT (default " << GBT_DEFAULT_N_TREES << ")" << endl; 
  cout << " -" << gbto.nMaxLeaves_s << " / --" << gbto.nMaxLeaves_l << setw( maxwidth - gbto.nMaxLeaves_l.size() ) << " " << "Maximum number of leaves per tree (default " << GBT_DEFAULT_N_MAX_LEAVES << ")" << endl;
  cout << " -" << gbto.shrinkage_s << " / --" << gbto.shrinkage_l << setw( maxwidth - gbto.shrinkage_l.size() ) << " " << "Shrinkage applied to evolving the residual (default " << GBT_DEFAULT_SHRINKAGE << ")" << endl;
  cout << " -" << gbto.subSampleSize_s << " / --" << gbto.subSampleSize_l << setw( maxwidth - gbto.subSampleSize_l.size() ) << " " << "Sample size fraction for training the trees (default " << GBT_DEFAULT_SUB_SAMPLE_SIZE << ")" << endl;
  cout << endl;
}

void printHelpHint()
{
  cout << endl;
  cout << "To get started, type \"-h\" or \"--help\"" << endl;
}

void executeRandomForestFilter(Treedata& treedata,
                               const size_t targetIdx,
                               const RF_options& op,
                               vector<num_t>& pValues,
                               vector<num_t>& importanceValues);


int main(int argc, char* argv[])
{

  General_options gen_op;
  RF_options RF_op_copy; // We need a copy (backup) since the options will differ between RF permutations, if enabled ...
  GBT_options GBT_op;

  //Print the intro header
  printHeader();

  //With no input arguments the help is printed
  if(argc == 1)
    {
      printHelp(gen_op,RF_op_copy,GBT_op);
      return(EXIT_SUCCESS);
    }

  //Read the user parameters ... 
  ArgParse parser(argc,argv);

  // first General Options
  parser.getArgument<bool>(gen_op.printHelp_s, gen_op.printHelp_l, gen_op.printHelp);
  parser.getArgument<string>(gen_op.input_s, gen_op.input_l, gen_op.input); 
  parser.getArgument<string>(gen_op.targetStr_s, gen_op.targetStr_l, gen_op.targetStr); 
  parser.getArgument<string>(gen_op.output_s, gen_op.output_l, gen_op.output);
  parser.getArgument<bool>(gen_op.noRF_s, gen_op.noRF_l, gen_op.noRF);
  parser.getArgument<bool>(gen_op.noGBT_s, gen_op.noGBT_l, gen_op.noGBT);

  parser.getArgument<bool>(RF_op_copy.isOptimizedNodeSplit_s, RF_op_copy.isOptimizedNodeSplit_l, RF_op_copy.isOptimizedNodeSplit);
  parser.getArgument<size_t>(RF_op_copy.nTrees_s,RF_op_copy.nTrees_l,RF_op_copy.nTrees);
  parser.getArgument<size_t>(RF_op_copy.mTry_s, RF_op_copy.mTry_l, RF_op_copy.mTry); 
  parser.getArgument<size_t>(RF_op_copy.nodeSize_s, RF_op_copy.nodeSize_l, RF_op_copy.nodeSize); 
  parser.getArgument<size_t>(RF_op_copy.nPerms_s, RF_op_copy.nPerms_l, RF_op_copy.nPerms); 
  parser.getArgument<num_t>(RF_op_copy.pValueThreshold_s, RF_op_copy.pValueThreshold_l, RF_op_copy.pValueThreshold); 

  parser.getArgument<bool>(GBT_op.isOptimizedNodeSplit_s, GBT_op.isOptimizedNodeSplit_l, GBT_op.isOptimizedNodeSplit);
  parser.getArgument<size_t>(GBT_op.nTrees_s, GBT_op.nTrees_l, GBT_op.nTrees);
  parser.getArgument<size_t>(GBT_op.nMaxLeaves_s, GBT_op.nMaxLeaves_l, GBT_op.nMaxLeaves);
  parser.getArgument<num_t>(GBT_op.shrinkage_s, GBT_op.shrinkage_l, GBT_op.shrinkage);
  parser.getArgument<num_t>(GBT_op.subSampleSize_s, GBT_op.subSampleSize_l, GBT_op.subSampleSize);

  if(gen_op.printHelp)
    {
      printHelp(gen_op,RF_op_copy,GBT_op);
      return(EXIT_SUCCESS);
    }

  //Print help and exit if input file is not specified
  if(gen_op.input == "")
    {
      cerr << "Input file not specified" << endl;
      printHelpHint();
      return EXIT_FAILURE;
    }

  //Print help and exit if target index is not specified
  if(gen_op.targetStr == "")
    {
      cerr << "target(s) (-i/--target) not specified" << endl;
      printHelpHint();
      return EXIT_FAILURE;
    }

  //Print help and exit if output file is not specified
  if(gen_op.output == "")
    {
      cerr << "Output file not specified" << endl;
      printHelpHint();
      return EXIT_FAILURE;
    }

  //Read data into Treedata object
  cout << endl << "Reading file '" << gen_op.input << "' please wait..." << endl;
  Treedata treedata_copy(gen_op.input);

  //Check which feature names match with the specified target identifier
  set<size_t> targetIcs;
  treedata_copy.getMatchingTargetIcs(gen_op.targetStr,targetIcs);
  if(targetIcs.size() == 0)
    {
      cerr << "No features match the specified target identifier '" << gen_op.targetStr << "'" << endl;
      return EXIT_FAILURE;
    }

  cout << endl << "[list of parameter values will be added here soon]" << endl << endl;


  //The program starts a loop in which an RF-ACE model will be built for each spcified target feature
  size_t iter = 1;
  for(set<size_t>::const_iterator it(targetIcs.begin()); it != targetIcs.end(); ++it, ++iter)
    {

      //Copy the data and options into new objects, which allows the program to alter the other copy without losing data
      Treedata treedata = treedata_copy;
      RF_options RF_op = RF_op_copy;

      //Extract the target index from the pointer and the number of real samples from the treedata object
      size_t targetIdx = *it;
      size_t nRealSamples = treedata.nRealSamples(targetIdx);
      num_t realFraction = 1.0*nRealSamples / treedata.nSamples();

      size_t maxwidth = 1 + static_cast<int>(targetIcs.size()) / 10;
      cout << setw(maxwidth) << iter << "/" << setw(maxwidth) << targetIcs.size() << " target " << treedata.getFeatureName(targetIdx) << ", " << flush;

      //If the target has no real samples, the program will just exit
      if(nRealSamples == 0)
	{
	  cout << "omitting for it has no real samples." << endl;
	  continue;
	}

      if(treedata.isFeatureNumerical(targetIdx))
        {
          cout << "regression ";
        }
      else
        {
          cout << treedata.nCategories(targetIdx) << "-class ";
        }
      cout << "CART. " << flush;
      
      //If default nTrees is to be used...
      if(RF_op.nTrees == RF_DEFAULT_N_TREES)
	{
	  RF_op.nTrees = static_cast<size_t>( 10.0*treedata.nSamples() / realFraction );
	}
      
      //If default mTry is to be used...
      if(RF_op.mTry == RF_DEFAULT_M_TRY)
	{
	  RF_op.mTry = static_cast<size_t>( floor( sqrt( 1.0*treedata.nFeatures() ) ) );   
	}
      
      //If default nodeSize is to be used...
      if(RF_op.nodeSize == RF_DEFAULT_NODE_SIZE)
	{
	  RF_op.nodeSize = 5;
	  size_t altNodeSize = static_cast<size_t>( ceil( 1.0*nRealSamples/100 ) );
	  if(altNodeSize > RF_op.nodeSize)
	    {
	      RF_op.nodeSize = altNodeSize;
	    }
	}
            
      //if(!op.applyFilter)
      //	{
	  // NOTE: THIS WILL TAKE EFFECT AFTER GBT IS IN PLACE
	  //if(optimizedRF)
	  //  {
	  //    cout << "NOTE: setting filter optimizations ON does not help when the filter is OFF" << endl;
	  //  }
      //	}
      
      if(treedata.nFeatures() < RF_op.mTry)
	{
	  cerr << "Not enough features (" << treedata.nFeatures()-1 << ") to test with mtry = " << RF_op.mTry << " features per split" << endl;
	  return EXIT_FAILURE;
	}
      
      if(treedata.nSamples() < 2 * RF_op.nodeSize)
	{
	  cerr << "Not enough samples (" << treedata.nSamples() << ") to perform a single split" << endl;
	  return EXIT_FAILURE;
	}

      
      //////////////////////////////////////////////////////////////////////
      //  ANALYSIS 1 -- Random Forest ensemble with artificial contrasts  //
      //////////////////////////////////////////////////////////////////////
      
      vector<num_t> pValues; //(treedata.nFeatures());
      vector<num_t> importanceValues; //(treedata.nFeatures());
      
      if(!gen_op.noRF)
	{
	  //cout << "1/3 Random Forest filter *ENABLED*. Options:" << endl;
	  //cout << "  --optimize       = "; if(RF_op.isOptimizedNodeSplit) { cout << "YES" << endl; } else { cout << "NO" << endl; }
	  //cout << "  --ntrees         = " << RF_op.nTrees << endl;
	  //cout << "  --mtry           = " << RF_op.mTry << endl;
	  //cout << "  --nodesize       = " << RF_op.nodeSize << endl;
	  //cout << "  --nperms         = " << RF_op.nPerms << endl;
	  //cout << "  --pthresold      = " << RF_op.pValueThreshold << endl;
	  //cout << endl;

	  cout << "Filtering " << flush;

	  executeRandomForestFilter(treedata,targetIdx,RF_op,pValues,importanceValues);
	   
	  size_t nFeatures = treedata.nFeatures();
	  vector<size_t> keepFeatureIcs(1);
	  keepFeatureIcs[0] = targetIdx;
	  vector<string> removedFeatures;
	  vector<size_t> removedFeatureIcs;
	  for(size_t featureIdx = 0; featureIdx < nFeatures; ++featureIdx)
	    {
	      if(featureIdx != targetIdx && importanceValues[featureIdx] > datadefs::EPS)
		{
		  keepFeatureIcs.push_back(featureIdx);
		}
	      else
		{
		  removedFeatureIcs.push_back(featureIdx);
		  removedFeatures.push_back(treedata.getFeatureName(featureIdx));
		}
	    }
	  
	  treedata.keepFeatures(keepFeatureIcs);
	  targetIdx = 0;
	  //cout << endl;
	  cout << "done, " << treedata.nFeatures() << " / " << treedata.nFeatures() + removedFeatureIcs.size() << " features left. " << flush;
	  //cout << "TEST: target is '" << treedata.getFeatureName(targetIdx) << "'" << endl;
	}
      else
	{
	  //cout << "1/3 Random Forest filter *DISABLED*" << endl << endl;
	}
      
      //cout << "2/3 Random Forest feature selection *ENABLED* (will become obsolete). Options:" << endl;
      
      cout << "Analyzing with RF " << flush;

      // THIS WILL BE REPLACED BY GBT
      executeRandomForestFilter(treedata,targetIdx,RF_op,pValues,importanceValues);
    	  
      cout << "done. " << flush;
      
      /////////////////////////////////////////////
      //  ANALYSIS 2 -- Gradient Boosting Trees  //
      /////////////////////////////////////////////
      
      if(!gen_op.noGBT)
	{
	  //cout << "3/3 Gradient Boosting Trees *ENABLED* (NON-FUNCTIONAL, WILL NOT AFFECT OUTPUT). Options:" << endl;
	  //cout << "GBT ";if(gen_op.noGBT) { cout << "DISABLED:" << endl; } else { cout << "ENABLED:" << endl; }
	  //cout << "  --optimize        = "; if(GBT_op.isOptimizedNodeSplit) { cout << "YES" << endl; } else { cout << "NO" << endl; }
	  //cout << "  --ntrees          = " << GBT_op.nTrees << endl;
	  //cout << "  --maxleaves       = " << GBT_op.nMaxLeaves << endl;
	  //cout << "  --shrinkage       = " << GBT_op.shrinkage << endl;
	  //cout << "  --samplesize      = " << GBT_op.subSampleSize << endl;

	  cout << "Analyzing with GBT ";
	 	  
	  GBT gbt(&treedata, targetIdx, GBT_op.nTrees, GBT_op.nMaxLeaves, GBT_op.shrinkage, GBT_op.subSampleSize);
	  
	  
	  cout <<endl<< "PREDICTION:" << endl;
	  vector<num_t> prediction(treedata.nSamples());
	  gbt.predictForest(&treedata, prediction);
	  cout << "done." << endl;
	}
      else
	{
	  cout << endl;
	}
      //else
      //	{
      //cout << "3/3 Gradient Boosting Trees *DISABLED*" << endl << endl;
      //}
      
      //cout << "done." << endl;
      
      
      ///////////////////////
      //  GENERATE OUTPUT  //
      ///////////////////////  
      vector<size_t> refIcs(treedata.nFeatures());
      //vector<string> fnames = treedata.featureheaders();
      bool isIncreasingOrder = false;
      datadefs::sortDataAndMakeRef(isIncreasingOrder,importanceValues,refIcs); // BUG
      datadefs::sortFromRef<num_t>(pValues,refIcs);
      //targetIdx = refIcs[targetIdx];
      
      string targetName = treedata.getFeatureName(targetIdx);
      
      //MODIFICATION: ALL ASSOCIATIONS WILL BE PRINTED
      if(true)
	{
	  //cout << "Writing associations to file '" << output << "'" << endl;
	  //ofstream os(output.c_str());
	  FILE* file;
	  if(iter == 1)
	    {
	      file = fopen(gen_op.output.c_str(),"w");
	    }
	  else
	    {
	      file = fopen(gen_op.output.c_str(),"a");
	    }
	  
	  for(size_t featureIdx = 0; featureIdx < treedata.nFeatures(); ++featureIdx)
	    {
	      
	      //MODIFICATION: ALL ASSOCIATIONS WILL BE PRINTED
	      /*
		if(importanceValues[featureIdx] < datadefs::EPS)
		{
		break;
		}
	      */
	      
	      if(refIcs[featureIdx] == targetIdx)
		{
		  //cout << refIcs[featureIdx] << " == " << targetIdx << " (" << targetHeader << ")" << endl;
		  continue;
		}
	      
	      if(RF_op.nPerms > 1)
		{
		  fprintf(file,"%s\t%s\t%9.8f\t%9.8f\t%9.8f\n",targetName.c_str(),treedata.getFeatureName(refIcs[featureIdx]).c_str(),pValues[featureIdx],importanceValues[featureIdx],treedata.pearsonCorrelation(targetIdx,refIcs[featureIdx]));
		}
	      else
		{
		  fprintf(file,"%s\t%s\tNaN\t%9.8f\t%9.8f\n",targetName.c_str(),treedata.getFeatureName(refIcs[featureIdx]).c_str(),importanceValues[featureIdx],treedata.pearsonCorrelation(targetIdx,refIcs[featureIdx]));
		}
	      //os << target_str << "\t" << treedata.get_featureheader(ref_ics[i]) << "\t" 
	      //   << pvalues[i] << "\t" << ivalues[i] << "\t" << treedata.corr(targetidx,ref_ics[i]) << endl;
	    }
	  
	  //CURRENTLY NOT POSSIBLE TO REPORT FILTERED FEATURES
	  /*
	    if(reportFiltered)
	    {
	    for(size_t featureIdx = 0; featureIdx < removedFeatureIcs.size(); ++featureIdx)
	    {
	    fprintf(po,"%s\t%s\tNaN\tNaN\t%9.8f\n",targetName.c_str(),removedFeatures[featureIdx].c_str(),treedata.pearsonCorrelation(oldTargetIdx,removedFeatureIcs[featureIdx]));
	    }
	    }
	  */


	  fclose(file);
	  //cout << endl << "Association file created. Format:" << endl;
	  //cout << "TARGET   PREDICTOR   P-VALUE   IMPORTANCE   CORRELATION" << endl << endl;
	  //cout << "Done." << endl;
	}
      else
	{
	  //cout << endl << "No significant associations found..." << endl;
	}
    }
      
  cout << endl << "Association file created. Format:" << endl;
  cout << "TARGET   PREDICTOR   P-VALUE   IMPORTANCE   CORRELATION" << endl << endl;
  cout << "RF-ACE completed successfully." << endl;

      
  return(EXIT_SUCCESS);
}


void executeRandomForestFilter(Treedata& treedata,
                               const size_t targetIdx,
                               const RF_options& op,
                               vector<num_t>& pValues,
                               vector<num_t>& importanceValues)
{

  vector<vector<num_t> > importanceMat(op.nPerms);
  pValues.resize(treedata.nFeatures());
  importanceValues.resize(treedata.nFeatures());
  size_t nNodesInAllForests = 0;

  //clock_t time_start(clock());

  //cout << "Growing " << op.nPerms << " Random Forests (RFs), please wait..." << endl;
  //#pragma omp parallel for
  Progress progress;
  for(int permIdx = 0; permIdx < static_cast<int>(op.nPerms); ++permIdx)
    {
      //cout << "  RF " << permIdx + 1 << ": ";
      //Treedata td_thread = treedata;

      progress.update(1.0*permIdx/op.nPerms);
  
      bool useContrasts;
      if(op.nPerms > 1)
	{
	  useContrasts = true;
	}
      else
	{
	  useContrasts = false;
	}

      Randomforest RF(&treedata,targetIdx,op.nTrees,op.mTry,op.nodeSize,useContrasts,op.isOptimizedNodeSplit);
      size_t nNodesInForest = RF.nNodes();
      nNodesInAllForests += nNodesInForest;
      importanceMat[permIdx] = RF.featureImportance();
      //printf("  RF %i: %i nodes (avg. %6.3f nodes/tree)\n",permIdx+1,static_cast<int>(nNodesInForest),1.0*nNodesInForest/op.nTrees);
      progress.update( 1.0 * ( 1 + permIdx ) / op.nPerms );
    }

  //num_t time_diff = 1.0*(clock() - time_start) / CLOCKS_PER_SEC;
  //cout << op.nPerms << " RFs, " << op.nPerms * op.nTrees << " trees, and " << nNodesInAllForests
  //     << " nodes generated in " << time_diff << " seconds (" << 1.0*nNodesInAllForests / time_diff
  //     << " nodes per second)" << endl;

  if(op.nPerms > 1)
    {
      for(size_t featureIdx = 0; featureIdx < treedata.nFeatures(); ++featureIdx)
	{
	  
	  size_t nRealSamples;
	  vector<num_t> fSample(op.nPerms);
	  vector<num_t> cSample(op.nPerms);
	  for(size_t permIdx = 0; permIdx < op.nPerms; ++permIdx)
	    {
	      fSample[permIdx] = importanceMat[permIdx][featureIdx];
	      cSample[permIdx] = importanceMat[permIdx][featureIdx + treedata.nFeatures()];
	    }
	  datadefs::utest(fSample,cSample,pValues[featureIdx]);
	  datadefs::meanVals(fSample,importanceValues[featureIdx],nRealSamples);
	}
    }
  else
    {
      importanceValues = importanceMat[0];
    }

  importanceValues.resize(treedata.nFeatures());
  
}
