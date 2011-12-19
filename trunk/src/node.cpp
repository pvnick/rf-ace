#include<iostream>
#include<cassert>
#include "node.hpp"

Node::Node():
  splitter_(NULL),
  //isTrainPredictionSet_(false),
  trainPrediction_(datadefs::NUM_NAN),
  nTestSamples_(0),
  testPredictionError_(0.0),
  //hasChildren_(false),
  leftChild_(NULL),
  rightChild_(NULL) {
  
}

// !! Documentation: establishes a recursive relationship that deletes all
// !! children, unless their pointers are orphaned.

Node::~Node() {

  if ( leftChild_ || rightChild_ ) {

    if ( leftChild_ && rightChild_ ) {
      delete leftChild_;
      delete rightChild_;
      delete splitter_;
    } else {
      cerr << "Node has only one child!" << endl;
      exit(1);
    }
    
  }

}

// !! Documentation: consider combining with the documentation in the header
// !! file, fleshing it out a bit. Ideally, implementation notes should fall
// !! here; notes on the abstraction should fall in the header file.
void Node::setSplitter(size_t splitterIdx, const string& splitterName, num_t splitLeftLeqValue) {
  
  if ( leftChild_ || rightChild_ ) {
    cerr << "Cannot set a splitter to a node twice!" << endl;
    exit(1);
  }
  
  splitterIdx_ = splitterIdx;
  splitter_ = new Splitter(splitterName,splitLeftLeqValue);

  leftChild_ = new Node;
  rightChild_ = new Node;

}

// !! Documentation: consider combining with the documentation in the header
// !! file, fleshing it out a bit. Ideally, implementation notes should fall
// !! here; notes on the abstraction should fall in the header file.
void Node::setSplitter(size_t splitterIdx, const string& splitterName, const set<num_t>& leftSplitValues, const set<num_t>& rightSplitValues) {

  if ( leftChild_ || rightChild_ ) {
    cerr << "Cannot set a splitter to a node twice!" << endl;
    exit(1);
  }

  splitterIdx_ = splitterIdx;
  splitter_ = new Splitter(splitterName,leftSplitValues,rightSplitValues);

  leftChild_ = new Node;
  rightChild_ = new Node;

}

Node* Node::percolateData(num_t value) {

  if ( datadefs::isNAN( value ) ) {
    cerr << "Node class does not accept NaNs to be percolated!" << endl;
    exit(1);
  }

  if ( !splitter_ ) {
    return( this );
  }

  if ( splitter_->splitsLeft( value ) ) {
    return( leftChild_ );
  }

  if ( splitter_->splitsRight( value ) ) {
    return( rightChild_ );
  }

  return( this );

}

Node* Node::leftChild() {
  return( leftChild_ );
}


Node* Node::rightChild() {
  return( rightChild_ );
}

/** 
 * Counts the number of (descending) nodes the tree has. 
 * If called from the root node, will return the size of 
 * the whole tree.
 */
size_t Node::nNodes() {
  
  size_t n = 1;
  
  // Recursive call to dig out the number of descending nodes
  this->recursiveNDescendantNodes(n);
  return(n);
}

/**
 * Recursive function for actually counting the number of nodes
 */
void Node::recursiveNDescendantNodes(size_t& n) {
  if ( !this->hasChildren() ) {
    return;
  } else {
    n += 2;
    leftChild_->recursiveNDescendantNodes( n );
    rightChild_->recursiveNDescendantNodes( n );
  }
}

void Node::print(ofstream& toFile) {
  
  if ( !this->hasChildren() ) {
    return;
  }
  
  toFile << this->splitter_->name() << "   SPLITTER_NAME   SPLIT_LEFT   SPLIT_RIGHT" << endl;
  
  this->leftChild()->print(toFile);
  this->rightChild()->print(toFile);
}


// !! Documentation: just your usual accessor, returning a copy of
// !! trainPrediction_.
num_t Node::getLeafTrainPrediction() {
  //assert( isTrainPredictionSet_ );
  return( trainPrediction_ );
}

void Node::recursiveNodeSplit(Treedata* treeData,
                              const size_t targetIdx,
                              const vector<size_t>& sampleIcs,
                              const GrowInstructions& GI,
                              set<size_t>& featuresInTree,
                              size_t& nNodes) {

  size_t nSamples = sampleIcs.size();

  if(nSamples < 2 * GI.minNodeSizeToStop || nNodes >= GI.maxNodesToStop) {
    //cout << "Too few samples to start with, quitting" << endl;
    vector<num_t> leafTrainData = treeData->getFeatureData(targetIdx,sampleIcs);
    //Node::setLeafTrainPrediction(leafTrainData,GI);

    // !! Potential Crash: This is unsafe. Add asserts or runtime checks.
    // !! Correctness: Violates the Principle of Least Knowledge. Refactor.
    (this->*GI.leafPredictionFunction)(leafTrainData,GI.numClasses);
    return;
  }
  
  vector<size_t> featureSampleIcs(GI.nFeaturesForSplit);

  if(GI.isRandomSplit) {
    if(GI.useContrasts) {
      for(size_t i = 0; i < GI.nFeaturesForSplit; ++i) {
	
        featureSampleIcs[i] = treeData->getRandomIndex(treeData->nFeatures());
	// If the sampled feature is a contrast... 
	if( treeData->getRandomIndex( 35535 ) / 35535.0 < 0.01 ) { // CHANGE TO 0.5 and see what's the difference in time consumption
	  //cout << "Generated CONTRAST \n" << endl;
	  featureSampleIcs[i] += treeData->nFeatures();
	}
      }
    } else {
      for(size_t i = 0; i < GI.nFeaturesForSplit; ++i) {
        featureSampleIcs[i] = treeData->getRandomIndex(treeData->nFeatures());
      }
    }
  } else {
    for(size_t i = 0; i < GI.nFeaturesForSplit; ++i) {
      featureSampleIcs[i] = i;
    }
  }

  //vector<num_t> targetData;
  //vector<num_t> featureData;
  
  //const bool isTargetNumerical = rootNode->isTargetNumerical();
  //treeData->getFeatureData(targetIdx,sampleIcs,targetData);

  vector<size_t> sampleIcs_left,sampleIcs_right;
  num_t splitValue;
  set<num_t> splitValues_left, splitValues_right;
  //size_t bestSplitterIdx;
  //Splitter bestSplitter;

  num_t splitFitness;
  size_t splitFeatureIdx;

  bool isSplitSuccessful;
  
  
  isSplitSuccessful = Node::regularSplitterSeek(treeData,
						targetIdx,
						sampleIcs,
						featureSampleIcs,
						GI,
						splitFeatureIdx,
						sampleIcs_left,
						sampleIcs_right,
						splitValue,
						splitValues_left,
						splitValues_right,
						splitFitness);
  
  
  if(!isSplitSuccessful) {

    vector<num_t> leafTrainData = treeData->getFeatureData(targetIdx,sampleIcs);

    // !! Potential Crash: This is unsafe. Add asserts or runtime checks.
    // !! Correctness: Violates the Principle of Least Knowledge. Refactor.
    (this->*GI.leafPredictionFunction)(leafTrainData,GI.numClasses);
    //cout << "Stopping tree generation after creation of " << nNodes << " nodes" << endl;
    return;
  }
  
  if ( true ) {
    //cout << "Out of ";
    //for ( size_t i = 0; i < featureSampleIcs.size(); ++i ) {
    //cout << " " << treeData->getFeatureName(featureSampleIcs[i]);
    //}
    //cout << endl;
    
    //cout << " ---- Feature " << treeData->getFeatureName(splitFeatureIdx) << " splits the data with ";

    //for ( size_t i = 0; i < sampleIcs_left.size(); ++i ) {
    //  cout << " " << treeData->getRawFeatureData(targetIdx,sampleIcs_left[i]);
    //}
    //cout << " ] <==> [ ";
    //for ( size_t i = 0; i < sampleIcs_right.size(); ++i ) {
    //  cout << " " << treeData->getRawFeatureData(targetIdx,sampleIcs_right[i]);
    //}
    
    //cout << "FITNESS " << splitFitness << endl;
  }

  if ( treeData->isFeatureNumerical(splitFeatureIdx) ) {
    //cout << "num splitter" << endl;
    Node::setSplitter(splitFeatureIdx,treeData->getFeatureName(splitFeatureIdx),splitValue);
  } else {
    //cout << "cat splitter" << endl;
    Node::setSplitter(splitFeatureIdx,treeData->getFeatureName(splitFeatureIdx),splitValues_left,splitValues_right);
  }

  vector<num_t> trainData = treeData->getFeatureData(targetIdx,sampleIcs);

  // !! Potential Crash: This is unsafe. Add asserts or runtime checks.
  // !! Correctness: Violates the Principle of Least Knowledge. Refactor.
  (this->*GI.leafPredictionFunction)(trainData,GI.numClasses);

  featuresInTree.insert(splitFeatureIdx);
  nNodes += 2;

  leftChild_->recursiveNodeSplit(treeData,targetIdx,sampleIcs_left,GI,featuresInTree,nNodes);
  rightChild_->recursiveNodeSplit(treeData,targetIdx,sampleIcs_right,GI,featuresInTree,nNodes);
  
}

bool Node::regularSplitterSeek(Treedata* treeData,
			       const size_t targetIdx,
			       const vector<size_t>& sampleIcs,
			       const vector<size_t>& featureSampleIcs,
			       const GrowInstructions& GI,
			       size_t& splitFeatureIdx,
			       vector<size_t>& sampleIcs_left,
			       vector<size_t>& sampleIcs_right,
			       num_t& splitValue,
			       set<num_t>& splitValues_left,
			       set<num_t>& splitValues_right,
			       num_t& splitFitness) {

  //bool isTargetNumerical = treeData->isFeatureNumerical(targetIdx);
  //vector<num_t> targetData = treeData->getFeatureData(targetIdx,sampleIcs);

  size_t nFeaturesForSplit = featureSampleIcs.size();
  splitFeatureIdx = nFeaturesForSplit;
  splitFitness = 0.0;
  
  num_t newSplitFitness;
  //vector<size_t> newSampleIcs_left;
  //vector<size_t> newSampleIcs_right;
  num_t newSplitValue;
  set<num_t> newSplitValues_left, newSplitValues_right;

  splitFitness = 0.0;
  
  for ( size_t i = 0; i < nFeaturesForSplit; ++i ) {
    
    //vector<num_t> newSplitFeatureData;
    size_t newSplitFeatureIdx = featureSampleIcs[i];
    bool isFeatureNumerical = treeData->isFeatureNumerical(newSplitFeatureIdx);

    //Neither the real nor the contrast feature can appear in the tree as splitter
    if ( newSplitFeatureIdx == targetIdx ) {
      continue;
    }

    // vector<num_t> featureData = treeData->getFeatureData(newSplitFeatureIdx,sampleIcs);

    vector<size_t> newSampleIcs_left(0);
    vector<size_t> newSampleIcs_right = sampleIcs;

    if ( isFeatureNumerical ) {
      Node::numericalFeatureSplit(treeData,
				  targetIdx,
				  newSplitFeatureIdx,
				  GI,
				  newSampleIcs_left,
				  newSampleIcs_right,
				  newSplitValue,
				  newSplitFitness);
    } else {
      Node::categoricalFeatureSplit(treeData,
				    targetIdx,
				    newSplitFeatureIdx,
				    GI,
				    newSampleIcs_left,
				    newSampleIcs_right,
				    newSplitValues_left,
				    newSplitValues_right,
				    newSplitFitness);
    }

    if( newSplitFitness > splitFitness &&
	newSampleIcs_left.size() >= GI.minNodeSizeToStop &&
	newSampleIcs_right.size() >= GI.minNodeSizeToStop ) {
      
      splitFitness = newSplitFitness;
      splitFeatureIdx = newSplitFeatureIdx;
      splitValue = newSplitValue;
      splitValues_left = newSplitValues_left;
      splitValues_right = newSplitValues_right;
      sampleIcs_left = newSampleIcs_left;
      sampleIcs_right = newSampleIcs_right;
    }    

  }
  
  if ( splitFeatureIdx == nFeaturesForSplit ) {
    return(false);
  }

  return(true);

}

// !! Correctness, Inadequate Abstraction: kill this method with fire. Refactor, REFACTOR, _*REFACTOR*_.
void Node::numericalFeatureSplit(Treedata* treedata,
				 const size_t targetIdx,
                                 const size_t featureIdx,
                                 const GrowInstructions& GI,
                                 vector<size_t>& sampleIcs_left,
                                 vector<size_t>& sampleIcs_right,
                                 num_t& splitValue,
                                 num_t& splitFitness) {


  vector<num_t> tv,fv;

  sampleIcs_left.clear();
  treedata->getFilteredDataPair(targetIdx,featureIdx,sampleIcs_right,tv,fv);

  size_t n_tot = tv.size();
  size_t n_right = n_tot;
  size_t n_left = 0;

  if(n_tot < 2 * GI.minNodeSizeToStop) {
    splitFitness = datadefs::NUM_NAN;
    return;
  }

  //Make reference indices that define the sorting wrt. feature
  bool isIncreasingOrder = true;

  //Sort feature vector and collect reference indices
  vector<size_t> refIcs;
  datadefs::sortDataAndMakeRef(isIncreasingOrder,fv,refIcs);

  //Use the reference indices to sort sample indices
  datadefs::sortFromRef<num_t>(tv,refIcs);
  datadefs::sortFromRef<size_t>(sampleIcs_right,refIcs);

  int bestSplitIdx = -1;
  
  //If the target is numerical, we use the iterative squared error formula to update impurity scores while we traverse "right"
  if ( treedata->isFeatureNumerical(targetIdx) ) {
    num_t mu_right = 0.0;
    num_t se_right = 0.0;
    num_t mu_left = 0.0;
    num_t se_left = 0.0;
    num_t se_best = 0.0;
    num_t se_tot = 0.0;

    datadefs::sqerr(tv,mu_right,se_right,n_right);
    assert(n_tot == n_right);
    se_best = se_right;
    se_tot = se_right;

    size_t idx = 0;
    while(n_left < n_tot - GI.minNodeSizeToStop) {
      datadefs::forward_backward_sqerr(tv[idx],n_left,mu_left,se_left,n_right,mu_right,se_right);
      if( se_left + se_right < se_best && n_left >= GI.minNodeSizeToStop) {
        bestSplitIdx = idx;
        se_best = se_left + se_right;
      }
      ++idx;
    }
    splitFitness = (se_tot - se_best) / se_tot;
  } else { //Otherwise we use the iterative gini index formula to update impurity scores while we traverse "right"
    map<num_t,size_t> freq_left;
    map<num_t,size_t> freq_right;
    size_t sf_left = 0;
    size_t sf_right = 0;

    datadefs::sqfreq(tv,freq_right,sf_right,n_right);
    num_t sf_tot = sf_right;
    num_t nsf_best = 1.0 * sf_right / n_right;
    assert(n_tot == n_right);

    size_t idx = 0;
    while(n_left < n_tot - GI.minNodeSizeToStop) {
      datadefs::forward_backward_sqfreq(tv[idx],n_left,freq_left,sf_left,n_right,freq_right,sf_right);
      if(1.0 * n_right * sf_left + 1.0 * n_left * sf_right > n_left * n_right * nsf_best && n_left >= GI.minNodeSizeToStop) {
        bestSplitIdx = idx;
        nsf_best = 1.0 * sf_left / n_left + 1.0 * sf_right / n_right;
	splitFitness = ( -1.0 * n_left*n_right*sf_tot + n_tot*n_right*sf_left + n_tot*n_left*sf_right ) / ( n_left*n_right * (1.0*n_tot*n_tot - sf_tot) );
      }
      ++idx;
    }
    //splitFitness = ( -1.0 * n_left*n_right*sf_tot + n_tot*n_right*sf_left + n_tot*n_left*sf_right ) / ( n_left*n_right * (1.0*n_tot*n_tot - sf_tot) );
  }

  if(bestSplitIdx == -1) {
    splitFitness = 0.0;
    return;
  }

  splitValue = fv[bestSplitIdx];
  n_left = bestSplitIdx + 1;
  sampleIcs_left.resize(n_left);

  for(size_t i = 0; i < n_left; ++i) {
    sampleIcs_left[i] = sampleIcs_right[i];
  }
  sampleIcs_right.erase(sampleIcs_right.begin(),sampleIcs_right.begin() + n_left);
  n_right = sampleIcs_right.size();
  //for(size_t i = 0; i < n_right; ++i) {
  //  sampleIcs_right[i] = sampleIcs_right[i];
  //}

  //cout << sampleIcs_left.size() << " " << sampleIcs_right.size() << " == " << n_tot << endl;
  assert(n_left + n_right == n_tot);

 
  /*
    if(false) {
    cout << "Numerical feature splits target [";
    for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
    cout << " " << sampleIcs_left[i] << ":" << tv[sampleIcs_left[i]];
    }
    cout << " ] <==> [";
    for(size_t i = 0; i < sampleIcs_right.size(); ++i) {
    cout << " " << sampleIcs_right[i] << ":" << tv[sampleIcs_right[i]];
    }
    cout << " ]" << endl;
    }
  */
  
}

// !! Inadequate Abstraction: Refactor me.
void Node::categoricalFeatureSplit(Treedata* treedata,
                                   const size_t targetIdx,
                                   const size_t featureIdx,
				   const GrowInstructions& GI,
                                   vector<size_t>& sampleIcs_left,
                                   vector<size_t>& sampleIcs_right,
				   set<num_t>& splitValues_left,
				   set<num_t>& splitValues_right,
                                   num_t& splitFitness) {

  vector<num_t> tv,fv;

  sampleIcs_left.clear();
  treedata->getFilteredDataPair(targetIdx,featureIdx,sampleIcs_right,tv,fv);

  // Map all feature categories to the corresponding samples and represent it as map. The map is used to assign samples to left and right branches
  map<num_t,vector<size_t> > fmap_right;
  map<num_t,vector<size_t> > fmap_left;
  map<num_t,vector<size_t> > fmap_right_best;
  map<num_t,vector<size_t> > fmap_left_best;

  size_t n_tot = 0;
  datadefs::map_data(fv,fmap_right,n_tot);
  size_t n_right = n_tot;
  size_t n_left = 0;

  if(n_tot < GI.minNodeSizeToStop) {
    splitFitness = datadefs::NUM_NAN;
    return;
  }
  
  map<size_t,num_t> int2num;
  size_t iter = 0;
  for ( map<num_t,vector<size_t> >::const_iterator it( fmap_right.begin() ); it != fmap_right.end(); ++it ) {
    int2num.insert( pair<size_t,num_t>( iter, it->first ) );
    //cout << iter << "->" << it->first << endl;
    ++iter;
  }

  // A variable to determine the index for the last sample in the partition sequence: lastSample = GI.partitionSequence->at(psMax)
  size_t psMax = 0;
  if ( fmap_right.size() > 2 ) {
    psMax = ( 1 << (fmap_right.size() - 2) ); // 2^( fmap_right.size() - 2 )
  }

  //cout << "Splitter has " << fmap_right.size() << " categories => psMax is " << psMax << endl;

  bool foundSplit = false;

  if ( treedata->isFeatureNumerical(targetIdx) ) {

    //cout << "Target is numerical" << endl;

    num_t mu_right;
    num_t mu_left = 0.0;
    num_t se_right;
    num_t se_left = 0.0;
    datadefs::sqerr(tv,mu_right,se_right,n_right);
    assert(n_tot == n_right);
    num_t se_best = se_right;
    num_t se_tot = se_right;

    for ( size_t psIdx = 0; psIdx <= psMax; ++psIdx ) {

      //cout << "psIdx = " << psIdx << " <= " << psMax << ", PS(psIdx) = " << GI.partitionSequence->at(psIdx) << " is thrown " << flush;
            
      // If the category is added from right to left
      if ( GI.partitionSequence->isAdded(psIdx) ) {

	//cout << "from right to left: ics [";
      
	//Take samples from right and put them left
	map<num_t,vector<size_t> >::iterator it( fmap_right.find( int2num[ GI.partitionSequence->at(psIdx) ] ) );
	for(size_t i = 0; i < it->second.size(); ++i) {
	  //cout << " " << it->second[i];
	  datadefs::forward_backward_sqerr(tv[ it->second[i] ],n_left,mu_left,se_left,n_right,mu_right,se_right);
	  //cout << n_left << "\t" << n_right << "\t" << se_left << "\t" << se_right << endl;
	}
	//cout << " ]" << endl;

	fmap_left.insert( *it );
	fmap_right.erase( it->first );

      } else {
	
	//cout << "from left to right: ics [";

        //Take samples from left back to right
	map<num_t,vector<size_t> >::iterator it( fmap_left.find( int2num[ GI.partitionSequence->at(psIdx) ] ) );
        for(size_t i = 0; i < it->second.size(); ++i) {
	  //cout << " " << it->second[i];
          //cout << tv[it->second[i]] << ": ";
          datadefs::forward_backward_sqerr(tv[ it->second[i] ],n_right,mu_right,se_right,n_left,mu_left,se_left);
          //cout << n_left << "\t" << n_right << "\t" << se_left << "\t" << se_right << endl;
        }
	//cout << " ]" << endl;

	fmap_right.insert( *it );
	fmap_left.erase( it->first );

      }

      if ( se_left+se_right < se_best && n_left >= GI.minNodeSizeToStop && n_right >= GI.minNodeSizeToStop ) {
	foundSplit = true;
	fmap_left_best = fmap_left;
	fmap_right_best = fmap_right;
	se_best = se_left + se_right;
      }
    }

    splitFitness = ( se_tot - se_best ) / se_tot;

  } else {

    //cout << "Target is categorical" << endl;
    
    map<num_t,size_t> freq_left,freq_right;
    size_t sf_left = 0;
    size_t sf_right;
    datadefs::sqfreq(tv,freq_right,sf_right,n_right);
    assert(n_tot == n_right);
    
    num_t sf_tot = sf_right;
    num_t nsf_best = 1.0 * sf_right / n_right;
    
    for ( size_t psIdx = 0; psIdx <= psMax; ++psIdx ) {

      //cout << "psIdx = " << psIdx << ", PS(psIdx) = " << GI.partitionSequence->at(psIdx) << " is thrown " << flush;
      
      // If the samples corresponding to the next shifted category is from right to left 
      if ( GI.partitionSequence->isAdded(psIdx) ) {
	
	//cout << "from right to left: ics [";

	// Take samples from right and put them left
	map<num_t,vector<size_t> >::iterator it( fmap_right.find( int2num[ GI.partitionSequence->at(psIdx) ] ) );
	for(size_t i = 0; i < it->second.size(); ++i) {
	  //cout << " " << tv[it->second[i]];
	  //cout << " " << it->second[i];
	  datadefs::forward_backward_sqfreq(tv[ it->second[i] ],n_left,freq_left,sf_left,n_right,freq_right,sf_right);
	  //cout << "<-" << tv[it->second[i]] << "   :" << n_left << "," << n_right << "," << sf_left << "," << sf_right << endl;
	}
	//cout << " ]" << endl;

	fmap_left.insert( *it );
        fmap_right.erase( it->first );
	
      } else {

	//cout << "from left to right: ics [";

        //Take samples from left back to right
	map<num_t,vector<size_t> >::iterator it( fmap_left.find( int2num[ GI.partitionSequence->at(psIdx) ] ) );
        for(size_t i = 0; i < it->second.size(); ++i) {
          //cout << " " << tv[it->second[i]];
	  //cout << " " << it->second[i];
          datadefs::forward_backward_sqfreq(tv[ it->second[i] ],n_right,freq_right,sf_right,n_left,freq_left,sf_left);
          //cout << "  " << tv[it->second[i]] << "-> :" << n_left << "," << n_right << "," << sf_left << "," << sf_right << endl;
        }
        //cout << " ]" << endl;

	fmap_right.insert( *it );
        fmap_left.erase( it->first );

      }
      
      if ( 1.0*n_right*sf_left + n_left*sf_right > n_left*n_right*nsf_best && n_left >= GI.minNodeSizeToStop && n_right >= GI.minNodeSizeToStop ) {
	foundSplit = true;
	fmap_left_best = fmap_left;
	fmap_right_best = fmap_right;
	nsf_best = 1.0*sf_left/n_left + 1.0*sf_right/n_right;
	splitFitness = ( -1.0 * n_left*n_right*sf_tot + n_tot*n_right*sf_left + n_tot*n_left*sf_right ) / ( n_left*n_right * (1.0*n_tot*n_tot - sf_tot) );
      }
            
    }
    //cout << n_left << "," << sf_left << " <-> " << n_right << "," << "," << sf_right << endl;
    //splitFitness = ( -1.0 * n_left*n_right*sf_tot + n_tot*n_right*sf_left + n_tot*n_left*sf_right ) / ( n_left*n_right * (1.0*n_tot*n_tot - sf_tot) );
  }
  
  if(!foundSplit) {
    splitFitness = 0.0;
    return;
  }

  // Assign samples and categories on the left. First store the original sample indices 
  vector<size_t> sampleIcs = sampleIcs_right;

  sampleIcs_left.resize(n_tot);
  splitValues_left.clear();

  // Then populate the left side (sample indices and split values)
  iter = 0;
  for ( map<num_t,vector<size_t> >::const_iterator it(fmap_left_best.begin()); it != fmap_left_best.end(); ++it ) {
    for ( size_t i = 0; i < it->second.size(); ++i ) {
      sampleIcs_left[iter] = sampleIcs[it->second[i]];
      ++iter;
    }
    splitValues_left.insert( it->first );
  }
  sampleIcs_left.resize(iter);

  // Last populate the right side (sample indices and split values) 
  iter = 0;
  for ( map<num_t,vector<size_t> >::const_iterator it(fmap_right_best.begin()); it != fmap_right_best.end(); ++it ) {
    for ( size_t i = 0; i < it->second.size(); ++i ) {
      sampleIcs_right[iter] = sampleIcs[it->second[i]];
      ++iter;
    }
    splitValues_right.insert( it->first );
  }
  sampleIcs_right.resize(iter);

}

// !! Legibility: Clean out all of the print statements.
/*
  num_t Node::splitFitness(vector<num_t> const& data,
  bool const& isFeatureNumerical,
  size_t const& minSplit,
  vector<size_t> const& sampleIcs_left,
  vector<size_t> const& sampleIcs_right) {
  
  //assert(data.size() == sampleIcs_left.size() + sampleIcs_right.size());
  
  size_t n_left = 0;
  size_t n_right = 0;
  if(isFeatureNumerical) {
  num_t mu_left = 0.0;
  num_t se_left = 0.0;
  num_t mu_right = 0.0;
  num_t se_right = 0.0;
  
  for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
  datadefs::forward_sqerr(data[sampleIcs_left[i]],n_right,mu_right,se_right);
  //cout << "forward sqerr: " << featurematrix_[featureidx][sampleics_left[i]] << " " << n_right << " " << mu_right << " " << se_right << endl;
  }
  
  for(size_t i = 0; i < sampleIcs_right.size(); ++i) {
  datadefs::forward_sqerr(data[sampleIcs_right[i]],n_right,mu_right,se_right);
  //cout << "forward sqerr: " << featurematrix_[featureidx][sampleics_right[i]] << " " << n_right << " " << mu_right << " " << se_right << endl;
  }
  
  if(n_right < 2*minSplit) {
  return(0.0);
  }
  
  num_t se_tot = se_right;
  
  for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
  datadefs::forward_backward_sqerr(data[sampleIcs_left[i]],n_left,mu_left,se_left,n_right,mu_right,se_right);
  //cout << "fw bw sqerr: " << featurematrix_[featureidx][sampleics_left[i]] << " " << n_left << " " << mu_left << " " << se_left << " " << n_right
  //  << " " << mu_right << " " << se_right << endl;
  }
  
  if(n_left < minSplit || n_right < minSplit) {
  return(0.0);
  }
  
  return(( se_tot - se_left - se_right ) / se_tot);
  
  } else {
  map<num_t,size_t> freq_left,freq_right;
  size_t sf_left = 0;
  size_t sf_right = 0;
  
  for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
  datadefs::forward_sqfreq(data[sampleIcs_left[i]],n_right,freq_right,sf_right);
  //cout << "forward sqfreq: " << featurematrix_[featureidx][sampleics_left[i]] << " " << n_right << " " << sf_right << endl;
  }
  
  for(size_t i = 0; i < sampleIcs_right.size(); ++i) {
  datadefs::forward_sqfreq(data[sampleIcs_right[i]],n_right,freq_right,sf_right);
  //cout << "forward sqfreq: " << featurematrix_[featureidx][sampleics_right[i]] << " " << n_right << " " << sf_right << endl;
  }
  
  if(n_right < 2*minSplit) {
  return(0.0);
  }
  
  size_t n_tot = n_right;
  size_t sf_tot = sf_right;
  
  for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
  datadefs::forward_backward_sqfreq(data[sampleIcs_left[i]],n_left,freq_left,sf_left,n_right,freq_right,sf_right);
  //cout << "fw bw sqfreq: " << featurematrix_[featureidx][sampleics_left[i]] << " " << n_left << " "<< sf_left << " " << n_right << " " << sf_right << endl;
  }
  
  if(n_left < minSplit || n_right < minSplit) {
  return(0.0);
  }
  
  //cout << n_left << " " << n_right << " " << sf_tot << " " << n_tot << " " << n_right << " " << sf_left << " " << n_tot*n_left*sf_right << " " << n_left*n_right << " " << pow(n_tot,2) - sf_tot << endl;
  
  //num_t fitness = (-1.0*(n_left*n_right*sf_tot) + n_tot*n_right*sf_left + n_tot*n_left*sf_right) / (n_left*n_right*(pow(n_tot,2) - sf_tot));
  //cout << "Fitness " << fitness << endl;
  
  return( ( -1.0 * n_left*n_right*sf_tot + n_tot*n_right*sf_left + n_tot*n_left*sf_right ) / ( n_left*n_right * (1.0*n_tot*n_tot - sf_tot) ) ) ;
  
  }
  
  }
*/

void Node::leafMean(const vector<datadefs::num_t>& data, const size_t numClasses) {
  
  if ( !datadefs::isNAN(trainPrediction_) ) {
    cerr << "Tried to set node prediction twice!" << endl;
    exit(1);
  }

  size_t n = data.size();
  assert(n > 0);
  trainPrediction_ = 0.0;

  for(size_t i = 0; i < data.size(); ++i) {
    trainPrediction_ += data[i];
  }

  trainPrediction_ /= n;

}

void Node::leafMode(const vector<datadefs::num_t>& data, const size_t numClasses) {

  if ( !datadefs::isNAN(trainPrediction_) ) {
    cerr << "Tried to set node prediction twice!" << endl;
    exit(1);
  }

  size_t n = data.size();
  assert(n > 0);
  trainPrediction_ = 0.0;

  map<num_t,size_t> freq;
  
  datadefs::count_freq(data,freq,n);
  map<num_t,size_t>::iterator it(max_element(freq.begin(),freq.end(),datadefs::freqIncreasingOrder()));
  trainPrediction_ = it->first;
  //isTrainPredictionSet_ = true;

}

// !! Document
void Node::leafGamma(const vector<datadefs::num_t>& data, const size_t numClasses) {

  if ( !datadefs::isNAN(trainPrediction_) ) {
    cerr << "Tried to set node prediction twice!" << endl;
    exit(1);
  }

  size_t n = data.size();
  assert(n > 0);
  trainPrediction_ = 0.0;

  num_t numerator = 0.0;
  num_t denominator = 0.0;

  for (size_t i = 0; i < n; ++i) {
    num_t abs_data_i = fabs( data[i] );
    denominator += abs_data_i * (1.0 - abs_data_i);
    numerator   += data[i];
  }
  if ( fabs(denominator) <= datadefs::EPS ) {
    trainPrediction_ = datadefs::LOG_OF_MAX_NUM * numerator;
  } else {
    trainPrediction_ = (numClasses - 1)*numerator / (numClasses * denominator);
  }
  //isTrainPredictionSet_ = true;
}
  
