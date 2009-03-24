/*
 * @author Andrew Rambaut
 * @author Marc Suchard
 */

#ifndef __beagle__
#define __beagle__

//#define DYNAMIC_SCALING
//#define SCALING_REFRESH	0

/* Definition of REAL can be switched between 'double' and 'float' */
#ifdef DOUBLE_PRECISION
#define REAL		double
#else
#define REAL		float
#endif

// initialize the library
//
// nodeCount the number of nodes in the tree
// tipCount the number of tips in the tree
// patternCount the number of site patterns
// categoryCount the number of rate categories
// matrixCount the number of substitution matrices (should be 1 or categoryCount)
void initialize(
				int nodeCount,
				int tipCount,
				int patternCount,
				int categoryCount,
				int matrixCount);

// finalize and dispose of memory allocation if needed
void finalize();

// set the partials for a given tip
//
// tipIndex the index of the tip
// inPartials the array of partials, stateCount x patternCount
void setTipPartials(
					int tipIndex,
					REAL* inPartials);

// set the states for a given tip
//
// tipIndex the index of the tip
// inStates the array of states: 0 to stateCount - 1, missing = stateCount
void setTipStates(
				  int tipIndex,
				  int* inStates);

// set the vector of state frequencies
//
// stateFrequencies an array containing the state frequencies
void setStateFrequencies(REAL* inStateFrequencies);

// sets the Eigen decomposition for a given matrix
//
// matrixIndex the matrix index to update
// eigenVectors an array containing the Eigen Vectors
// inverseEigenVectors an array containing the inverse Eigen Vectors
// eigenValues an array containing the Eigen Values
void setEigenDecomposition(
						   int matrixIndex,
						   REAL** inEigenVectors,
						   REAL** inInverseEigenVectors,
						   REAL* inEigenValues);

// set the vector of category rates
//
// categoryRates an array containing categoryCount rate scalers
void setCategoryRates(REAL* inCategoryRates);

// set the vector of category proportions
//
// categoryProportions an array containing categoryCount proportions (which sum to 1.0)
void setCategoryProportions(REAL* inCategoryProportions);

// calculate a transition probability matrices for a given list of node. This will
// calculate for all categories (and all matrices if more than one is being used).
//
// nodeIndices an array of node indices that require transition probability matrices
// branchLengths an array of expected lengths in substitutions per site
// count the number of elements in the above arrays
void calculateProbabilityTransitionMatrices(
                                            int* nodeIndices,
                                            REAL* branchLengths,
                                            int count);

// calculate partials using an array of operations
//
// operations an array of triplets of indices: the two source partials and the destination
// dependencies an array of indices specify which operations are dependent on which (optional)
// count the number of operations
// rescale indicate if partials should be rescaled during peeling
void calculatePartials(
					   int* operations,
					   int* dependencies,
					   int count,
					   int rescale);

// calculate the site log likelihoods at a particular node
//
// rootNodeIndex the index of the root
// outLogLikelihoods an array into which the site log likelihoods will be put
void calculateLogLikelihoods(
			int rootNodeIndex,
			REAL* outLogLikelihoods);

// store the current state of all partials and matrices
void storeState();

// restore the stored state after a rejected move
void restoreState();

#endif // __beagle__
