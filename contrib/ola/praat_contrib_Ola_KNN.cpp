/* praat_contrib_Ola_KNN.cpp
 *
 * Copyright (C) 2007-2009 Ola Söder, 2010-2011,2015,2016 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * os 2007/05/29 Initial release?
 * os 2009/01/23 Bugfix: Removed MUX:ing (KNN_learn) incompatible with the scripting engine. Thanks to Paul Boersma for spotting this problem.
 * pb 2010/12/28 in messages: typos, English, interpunction
 * pb 2011/07/12 C++ and removed several errors
 * pb 2011/09/18 made a start with handling dataChanged at all
 */

#include "KNN.h"
#include "KNN_threads.h"
#include "KNN_prune.h"
#include "Pattern_to_Categories_cluster.h"
#include "FeatureWeights.h"
#include "praat.h"

static const char32 *QUERY_BUTTON   = U"Query -";
static const char32 *MODIFY_BUTTON  = U"Modify -";
static const char32 *EXTRACT_BUTTON = U"Extract -";

/////////////////////////////////////////////////////////////////////////////////////////
// KNN creations                                                                      //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (KNN_create, U"Create kNN Classifier", U"kNN classifiers 1. What is a kNN classifier?") {
	WORD (U"Name", U"Classifier")
	OK2
DO
	autoKNN knn = KNN_create ();
	praat_new (knn.move(), GET_STRING (U"Name"));
END2 }

FORM (KNN_Pattern_Categories_to_KNN, U"Create kNN classifier", U"kNN classifiers 1. What is a kNN classifier?") {
	WORD (U"Name", U"Classifier")
	RADIO (U"Ordering", 1)
	RADIOBUTTON (U"Random")
	RADIOBUTTON (U"Sequential")
	OK2
DO
	iam_ONLY (PatternList);
	youare_ONLY (Categories);
	int ordering = GET_INTEGER (U"Ordering");
	autoKNN knn = KNN_create ();
	switch (ordering) {
		case 1:
			ordering = kOla_SHUFFLE;
			break;
		case 2:
			ordering = kOla_SEQUENTIAL;
	}
	int result = KNN_learn (knn.get(), me, you, kOla_REPLACE, ordering);
	switch (result) {
		case kOla_PATTERN_CATEGORIES_MISMATCH:
			Melder_throw (U"The number of Categories should be equal to the number of rows in PatternList.");
		case kOla_DIMENSIONALITY_MISMATCH:
			Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
		default:
			praat_new (knn.move(), GET_STRING(U"Name"));
	}
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// KNN extractions, queries and modifications                                         //
/////////////////////////////////////////////////////////////////////////////////////////

DIRECT2 (KNN_getNumberOfInstances) {
    iam_ONLY (KNN);
    Melder_information (my nInstances, U" units");
END2 }

FORM (KNN_getOptimumModel, U"kNN model selection", U"kNN classifiers 1.1.2. Model selection") {
	RADIO (U"Evaluation method", 1)
	RADIOBUTTON (U"Leave one out")
	RADIOBUTTON (U"10-fold cross-validation")
	INTEGER (U"k max", U"50")
	INTEGER (U"Number of seeds", U"10")
	POSITIVE (U"Learning rate", U"0.2")
	OK2
DO
	iam_ONLY (KNN);
	long k = GET_INTEGER (U"k max");
	double lrate = GET_REAL (U"Learning rate");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k max such that 0 < k max < ", my nInstances + 1, U".");
	long nseeds = GET_INTEGER (U"Number of seeds");
	if (nseeds < 1)
		Melder_throw (U"The number of seeds should exceed 1.");
	int mode = GET_INTEGER (U"Evaluation method");
	switch (mode) {
		case 2:
			mode = kOla_TEN_FOLD_CROSS_VALIDATION;
			break;
		case 1:
			mode = kOla_LEAVE_ONE_OUT;
			break;
	}
	autoFeatureWeights fws = FeatureWeights_create ((my input) -> nx);
	int dist;
	KNN_modelSearch (me, fws.get(), &k, &dist, mode, lrate, nseeds);
	switch (dist) {
		case kOla_SQUARED_DISTANCE_WEIGHTED_VOTING:
			Melder_information (U"Vote weighting: Inversed squared distance\n", U"k: ", k);
			break;
		case kOla_DISTANCE_WEIGHTED_VOTING:
			Melder_information (U"Vote weighting: Inversed distance\n", U"k: ", k);
			break;
		case kOla_FLAT_VOTING:
			Melder_information (U"Vote weighting: Flat\n", U"k: ", k);
			break;
	}
END2 }

FORM (KNN_evaluate, U"Evaluation", U"KNN: Get accuracy estimate...") {
	RADIO (U"Evaluation method", 1)
	RADIOBUTTON (U"Leave one out")
	RADIOBUTTON (U"10-fold cross-validation")
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances < 1)
		Melder_throw (U"Instance base is empty.");
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	int mode = GET_INTEGER (U"Evaluation method");
	switch (mode) {
		case 2:
			mode = kOla_TEN_FOLD_CROSS_VALIDATION;
			break;
		case 1: 
			mode = kOla_LEAVE_ONE_OUT;
			break;
	}
	autoFeatureWeights fws = FeatureWeights_create (my input -> nx);
	double result = KNN_evaluate (me, fws.get(), k, vt, mode);
	if (lround (result) == kOla_FWEIGHTS_MISMATCH)
		Melder_throw (U"The number of feature weights should be equal to the dimensionality of the PatternList.");
	Melder_information (100 * result, U" percent of the instances correctly classified.");   // BUG: use Melder_percent
END2 }

FORM (KNN_evaluateWithFeatureWeights, U"Evaluation", U"KNN & FeatureWeights: Get accuracy estimate...") {
	RADIO (U"Evaluation method", 1)
	RADIOBUTTON (U"Leave one out")
	RADIOBUTTON (U"10-fold cross-validation")
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances < 1)
		Melder_throw (U"Instance base is empty");
	youare_ONLY (FeatureWeights);
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}   
	int mode = GET_INTEGER (U"Evaluation method");
	switch (mode) {
		case 2:
			mode = kOla_TEN_FOLD_CROSS_VALIDATION;
			break;
		case 1:
			mode = kOla_LEAVE_ONE_OUT;
			break;
	}
	double result = KNN_evaluate (me, you, k, vt, mode);
	if (lround (result) == kOla_FWEIGHTS_MISMATCH)
		Melder_throw (U"The number of feature weights should be equal to the dimensionality of the PatternList.");
	Melder_information (100 * result, U" percent of the instances correctly classified.");
END2 }

DIRECT2 (KNN_extractInputPatterns) {
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	autoPatternList input = Data_copy (my input.get());
	praat_new (input.move(), U"Input Patterns");
END2 }

DIRECT2 (KNN_extractOutputCategories) {
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	autoCategories output = Data_copy (my output.get());
	praat_new (output.move(), U"Output Categories");
END2 }

FORM (KNN_reset, U"Reset", U"KNN: Reset...") {
    LABEL (U"", U"Warning: this command destroys all previous learning.")
    OK2
DO
	iam_ONLY (KNN);
	my input.reset();
	my output.reset();
	my nInstances = 0;
	praat_dataChanged (me);   // BUG: this should be inserted much more often
END2 }

DIRECT2 (KNN_shuffle) {
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	KNN_shuffleInstances (me);
END2 }

FORM (KNN_prune, U"Pruning", U"KNN: Prune...") {
	POSITIVE (U"Noise pruning degree", U"1")
	POSITIVE (U"Redundancy pruning degree", U"1")
	INTEGER (U"k neighbours", U"1")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	long oldn = my nInstances;   // save before it changes!
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	double n = GET_REAL (U"Noise pruning degree");
	double r = GET_REAL (U"Redundancy pruning degree");
	if (n <= 0 || n > 1 || r <= 0 || r > 1)
		Melder_throw (U"Please select a pruning degree d such that 0 < d <= 1.");
	long npruned = KNN_prune_prune (me, n, r, k);
	Melder_information (npruned, U" instances discarded. \n", U"Size of new instance base: ", oldn - npruned);
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Learning                                                                            //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (KNN_learn, U"Learning", U"kNN classifiers 1. What is a kNN classifier?") {
	RADIO (U"Learning method", 1)
	RADIOBUTTON (U"Append new information")
	RADIOBUTTON (U"Replace current intancebase")
	RADIO (U"Ordering", 1)
	RADIOBUTTON (U"Random")
	RADIOBUTTON (U"Sequential")
	OK2
DO
	iam_ONLY (KNN);
	youare_ONLY (PatternList);
	heis_ONLY (Categories);
	int ordering = GET_INTEGER (U"Ordering");
	switch (ordering) {
		case 1:
			ordering = kOla_SHUFFLE;
			break;
		case 2:
			ordering = kOla_SEQUENTIAL;
	}
	int method = GET_INTEGER (U"Learning method");
	int result = kOla_ERROR;
	switch (method) {
		case 1:
			result = KNN_learn (me, you, him, my nInstances == 0 ? kOla_REPLACE : kOla_APPEND, ordering);
			break;
		case 2:
			result = KNN_learn (me, you, him, kOla_REPLACE, ordering);
			break;
	}
	switch (result) {
		case kOla_PATTERN_CATEGORIES_MISMATCH:  
			Melder_throw (U"The number of Categories should be equal to the number of rows in PatternList.");
		case kOla_DIMENSIONALITY_MISMATCH:
			Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
	}
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Evaluation                                                                          //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (KNN_evaluateWithTestSet, U"Evaluation", U"KNN & PatternList & Categories: Evaluate...") {
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty");
	youare_ONLY (PatternList);
	heis_ONLY (Categories);
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	if (your ny != his size)
		Melder_throw (U"The number of Categories should be equal to the number of rows in PatternList.");
	if (your nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
	autoFeatureWeights fws = FeatureWeights_create (your nx);
	double result = KNN_evaluateWithTestSet (me, you, him, fws.get(), k, vt);
	Melder_information (100 * result, U" percent of the instances correctly classified.");
END2 }

FORM (KNN_evaluateWithTestSetAndFeatureWeights, U"Evaluation", U"KNN & PatternList & Categories & FeatureWeights: Evaluate...") {
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty");
	PatternList p = (PatternList) ONLY (classPatternList);
	Categories c = (Categories) ONLY (classCategories);
	FeatureWeights fws = (FeatureWeights) ONLY (classFeatureWeights);
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	if (p -> ny != c->size)
		Melder_throw (U"The number of Categories should be equal to the number of rows in PatternList.");
	if (p -> nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
	if (p->nx != fws -> fweights -> numberOfColumns)
		Melder_throw (U"The number of feature weights should be equal to the dimensionality of the PatternList.");
	double result = KNN_evaluateWithTestSet (me, p, c, fws, k, vt);
	Melder_information (100 * result, U" percent of the instances correctly classified.");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Classification                                                                      //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (KNN_toCategories, U"Classification", U"KNN & PatternList: To Categories...") {
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	youare_ONLY (PatternList);
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	if (your nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should match that of the instance base.");
	autoFeatureWeights fws = FeatureWeights_create (your nx);
	autoCategories result = KNN_classifyToCategories (me, you, fws.get(), k, vt);
	praat_new (result.move(), U"Output");
END2 }

FORM (KNN_toTableOfReal, U"Classification", U"KNN & PatternList: To TabelOfReal...") {
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	youare_ONLY (PatternList);
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	autoFeatureWeights fws = FeatureWeights_create (your nx);
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	if (your nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should match that of the instance base.");
	autoTableOfReal result = KNN_classifyToTableOfReal (me, you, fws.get(), k, vt);
	praat_new (result.move(), U"Output");
END2 }

FORM (KNN_toCategoriesWithFeatureWeights, U"Classification", U"KNN & PatternList & FeatureWeights: To Categories...") {
	INTEGER (U"k neighbours", U"KNN & PatternList & FeatureWeights: To Categories...")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")  
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	youare_ONLY (PatternList);
	heis_ONLY (FeatureWeights);
	int vt = GET_INTEGER (U"Vote weighting");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;  
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	if (your nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
	if (your nx != his fweights -> numberOfColumns)
		Melder_throw (U"The number of feature weights should be equal to the dimensionality of the PatternList.");
	autoCategories result = KNN_classifyToCategories (me, you, him, k, vt);
	praat_new (result.move(), U"Output");
END2 }

FORM (KNN_toTableOfRealWithFeatureWeights, U"Classification", U"KNN & PatternList & FeatureWeights: To TableOfReal...") {
	INTEGER (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 1)
	RADIOBUTTON (U"Inversed squared distance")
	RADIOBUTTON (U"Inversed distance")
	RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty.");
	youare_ONLY (PatternList);
	heis_ONLY (FeatureWeights);
	long k = GET_INTEGER (U"k neighbours");
	int vt = GET_INTEGER (U"Vote weighting");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U"\n");
	if (your nx != his fweights -> numberOfColumns)
		Melder_throw (U"The number of features and the number of feature weights should be equal.");
	switch (vt) {
		case 1:
			vt = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			vt = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			vt = kOla_FLAT_VOTING;
			break;
	}
	autoTableOfReal result = KNN_classifyToTableOfReal (me, you, him, k, vt);
	praat_new (result.move(), U"Output");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Clustering                                                                          //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (Pattern_to_Categories_cluster, U"k-means clustering", U"PatternList: To Categories...") {
	INTEGER (U"k clusters", U"1")
	POSITIVE (U"Cluster size ratio constraint", U"0.0000001");
	INTEGER (U"Maximum number of reseeds", U"1000")
	OK2
DO
	iam_ONLY (PatternList);
	if (my nx > 0 && my ny > 0) {
		long k = GET_INTEGER (U"k clusters");
		if (k < 1 || k > my ny)
			Melder_throw (U"Please select a value of k such that 0 < k <= ", my ny, U".");
		long rs =  GET_INTEGER (U"Maximum number of reseeds");
		if (rs < 0)
			Melder_throw (U"The maximum number of reseeds should not be negative.");
		double rc =  GET_REAL (U"Cluster size ratio constraint");
		if (rc > 1 || rc <= 0)
			Melder_throw (U"Please select a value of the cluster size ratio constraint c such that 0 < c <= 1.");
		autoFeatureWeights fws = FeatureWeights_create (my nx);
		autoCategories result = PatternList_to_Categories_cluster (me, fws.get(), k, rc, rs);
		praat_new (result.move(), U"Output");
	} else {
		Melder_throw (U"PatternList is empty.");
	}
END2 }

FORM (Pattern_to_Categories_clusterWithFeatureWeights, U"k-means clustering", U"PatternList & FeatureWeights: To Categories...") {
	INTEGER (U"k clusters", U"1")
	POSITIVE (U"Cluster size ratio constraint", U"0.0000001");
	INTEGER (U"Maximum number of reseeds", U"1000")
	OK2
DO
	iam_ONLY (PatternList);
	if (my nx > 0 && my ny > 0) {
		youare_ONLY (FeatureWeights);
		if (my nx != your fweights -> numberOfColumns)
			Melder_throw (U"The number of features and the number of feature weights should be equal.");
		long k = GET_INTEGER(U"k clusters");
		if (k < 1 || k > my ny)
			Melder_throw (U"Please select a value of k such that 0 < k <= ", my ny, U".");
		long rs =  GET_INTEGER(U"Maximum number of reseeds");
		if (rs < 0)
			Melder_throw (U"The maximum number of reseeds should not be negative.");
		double rc =  GET_REAL(U"Cluster size ratio constraint");
		if (rc > 1 || rc <= 0)
			Melder_throw (U"Please select a value of the cluster size ratio constraint c such that 0 < c <= 1.");
		autoCategories result = PatternList_to_Categories_cluster (me, you, k, rc, rs);
		praat_new (result.move(), U"Output");
	} else {
		Melder_throw (U"PatternList is empty.");
	}
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Dissimilarity computations                                                          //
/////////////////////////////////////////////////////////////////////////////////////////

DIRECT2 (KNN_patternToDissimilarity) {
	iam_ONLY (PatternList);
	autoFeatureWeights fws = FeatureWeights_create (my nx);
	autoDissimilarity result = KNN_patternToDissimilarity (me, fws.get());
	praat_new (result.move(), U"Output");
END2 }

DIRECT2 (KNN_patternToDissimilarityWithFeatureWeights) {
	iam_ONLY (PatternList);
	youare_ONLY (FeatureWeights);
	if (my nx != your fweights -> numberOfColumns)
		Melder_throw (U"The number of features and the number of feature weights should be equal.");
	autoDissimilarity result = KNN_patternToDissimilarity (me, you);
	praat_new (result.move(), U"Output");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Computation of permutation                                                          //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (KNN_SA_computePermutation, U"To Permutation...", U"PatternList & Categories: To FeatureWeights...") {
	NATURAL (U"Tries per step", U"200")
	NATURAL (U"Iterations", U"10")
	POSITIVE (U"Step size", U"10")
	POSITIVE (U"Boltzmann constant", U"1.0")
	POSITIVE (U"Initial temperature", U"0.002")
	POSITIVE (U"Damping factor", U"1.005")
	POSITIVE (U"Final temperature", U"0.000002")
	OK2
DO
	iam_ONLY (KNN);
	long tries = GET_INTEGER (U"Tries per step");
	long iterations = GET_INTEGER (U"Iterations");
	double step_size = GET_REAL (U"Step size");
	double bolzmann_c = GET_REAL (U"Boltzmann constant");
	double temp_start = GET_REAL (U"Initial temperature");
	double temp_damp = GET_REAL (U"Damping factor");
	double temp_stop = GET_REAL (U"Final temperature");
	autoPermutation result = KNN_SA_ToPermutation (me, tries, iterations, step_size, bolzmann_c, temp_start, temp_damp, temp_stop);
	praat_new (result.move(), U"Output");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Computation of feature weights                                                      //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (FeatureWeights_computeRELIEF, U"Feature weights", U"PatternList & Categories: To FeatureWeights...") {
	INTEGER (U"Number of neighbours", U"1")
	OK2
DO
	iam_ONLY (PatternList);
	youare_ONLY (Categories);
	if (my ny < 2)
		Melder_throw (U"The PatternList object should contain at least two rows.");
	if (my ny != your size)
		Melder_throw (U"The number of rows in the PatternList object should equal the number of categories in the Categories object.");
	autoFeatureWeights result = FeatureWeights_compute (me, you, GET_INTEGER (U"Number of neighbours"));
	praat_new (result.move(), U"Output");
END2 }

FORM (FeatureWeights_computeWrapperExt, U"Feature weights", U"KNN & PatternList & Categories: To FeatureWeights..") {
	POSITIVE (U"Learning rate", U"0.02")
	NATURAL (U"Number of seeds", U"20")
	POSITIVE (U"Stop at", U"1")
	RADIO (U"Optimization", 1)
		RADIOBUTTON (U"Co-optimization")
		RADIOBUTTON (U"Single feature")
	NATURAL (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 3)
		RADIOBUTTON (U"Inversed squared distance")
		RADIOBUTTON (U"Inversed distance")
		RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances <= 0)
		Melder_throw (U"Instance base is empty");
	youare_ONLY (PatternList);
	heis_ONLY (Categories);
	int mode = GET_INTEGER (U"Vote weighting");
	switch (mode) {
		case 1:
			mode = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			mode = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			mode = kOla_FLAT_VOTING;
			break;
	}
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	if (your nx != my input -> nx)
		Melder_throw (U"The dimensionality of PatternList should be equal to that of the instance base.");
	autoFeatureWeights result = FeatureWeights_computeWrapperExt (me, you, him, k, mode, GET_INTEGER (U"Number of seeds"),
		GET_REAL (U"Learning rate"), GET_REAL (U"Stop at"), (int) GET_INTEGER (U"Optimization"));
	praat_new (result.move(), U"Output");
END2 }

FORM (FeatureWeights_computeWrapperInt, U"Feature weights", U"KNN: To FeatureWeights...") {
	POSITIVE (U"Learning rate", U"0.02")
	NATURAL (U"Number of seeds", U"10")
	POSITIVE (U"Stop at", U"1")
	RADIO (U"Optimization", 1)
		RADIOBUTTON (U"Co-optimization")
		RADIOBUTTON (U"Single feature")
	RADIO (U"Evaluation method", 1)
		RADIOBUTTON (U"Leave one out")
		RADIOBUTTON (U"10-fold cross-validation")
	NATURAL (U"k neighbours", U"1")
	RADIO (U"Vote weighting", 3)
		RADIOBUTTON (U"Inversed squared distance")
		RADIOBUTTON (U"Inversed distance")
		RADIOBUTTON (U"Flat")
	OK2
DO
	iam_ONLY (KNN);
	if (my nInstances < 1)
		Melder_throw (U"Instance base is empty");
	int emode = GET_INTEGER (U"Evaluation method");
	switch (emode) {
		case 2:
			emode = kOla_TEN_FOLD_CROSS_VALIDATION;
			break;
		case 1:
			emode = kOla_LEAVE_ONE_OUT;
			break;
	}
	int mode = GET_INTEGER (U"Vote weighting");
	switch (mode) {
		case 1:
			mode = kOla_SQUARED_DISTANCE_WEIGHTED_VOTING;
			break;
		case 2:
			mode = kOla_DISTANCE_WEIGHTED_VOTING;
			break;
		case 3:
			mode = kOla_FLAT_VOTING;
			break;
	}
	long k = GET_INTEGER (U"k neighbours");
	if (k < 1 || k > my nInstances)
		Melder_throw (U"Please select a value of k such that 0 < k < ", my nInstances + 1, U".");
	autoFeatureWeights result = FeatureWeights_computeWrapperInt (me, k, mode, GET_INTEGER (U"Number of seeds"), GET_REAL (U"Learning rate"),
		GET_REAL (U"Stop at"), (int) GET_INTEGER (U"Optimization"), emode);
	praat_new (result.move(), U"Output");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Creation and processing of auxiliary datatypes                                      //
/////////////////////////////////////////////////////////////////////////////////////////

FORM (Pattern_create, U"Create PatternList", nullptr) {
	WORD (U"Name", U"1x1")
	NATURAL (U"Dimension of a pattern", U"1")
	NATURAL (U"Number of patterns", U"1")
	OK2
DO
	autoPatternList result = PatternList_create (GET_INTEGER (U"Number of patterns"), GET_INTEGER (U"Dimension of a pattern"));
	praat_new (result.move(), GET_STRING (U"Name"));
END2 }

FORM (FeatureWeights_create, U"Create FeatureWeights", nullptr) {
	WORD (U"Name", U"empty")
	NATURAL (U"Number of weights", U"1")
	OK2
DO
	autoFeatureWeights result = FeatureWeights_create (GET_INTEGER (U"Number of weights"));
	praat_new (result.move(), GET_STRING (U"Name"));
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// DEBUG                                                                               //
/////////////////////////////////////////////////////////////////////////////////////////

// Disabled
/*
#ifdef _DEBUG

DIRECT (KNN_debug_KNN_SA_partition) {
    PatternList p = ONLY (classPatternList);
    autoPatternList output = PatternList_create (p->ny, p->nx);
    autoNUMvector <long> result (0, p->ny);
    KNN_SA_partition (p, 1, p->ny, result);

    for (long k = 1, c = 1; k <= output->ny; ++k, ++c)
        for (long i = 1; i <= p->ny && k <= output->ny; ++i)
            if(result [i] == c)
            {
                for(long j = 1; j <= output->nx; ++j)
                    output->z[k][j] = p->z[i][j];
                ++k;
            }

    praat_new (output.move(), U"Output");

END }

DIRECT (KNN_debug_KNN_getNumberOfCPUs) {
    Melder_information (KNN_getNumberOfCPUs(), U" CPUs available");
END }

DIRECT (KNN_debug_KNN_threadTest) {
    KNN_threadTest();
END }

#endif
*/

/////////////////////////////////////////////////////////////////////////////////////////
// Help                                                                                //
/////////////////////////////////////////////////////////////////////////////////////////

DIRECT2 (KNN_help) {
	Melder_help (U"KNN classifiers");
END2 }

DIRECT2 (hint_KNN_and_FeatureWeights_evaluate) {
	Melder_information (U"The accuracy of a KNN can be estimated by selecting a KNN and a FeatureWeights object and choosing \"Evaluate...\".");
END2 }

DIRECT2 (hint_KNN_and_Pattern_classify) {
	Melder_information (U"You can use the KNN as a classifier by selecting a KNN and a PatternList and choosing \"To Categories...\" or \"To TableOfReal...\".");
END2 }

DIRECT2 (hint_KNN_and_Pattern_and_FeatureWeights_classify) {
	Melder_information (U"You can use the KNN as a classifier by selecting a KNN, a PatternList and an FeatureWeights object and choosing \"To Categories...\" or \"To TableOfReal...\".");
END2 }

DIRECT2 (hint_KNN_and_Pattern_and_Categories_learn) {
	Melder_information (U"You can train a KNN by selecting a KNN, a PatternList and a Categories object together and choosing \"Learn...\".");
END2 }

DIRECT2 (hint_KNN_and_Pattern_and_Categories_evaluate) {
	Melder_information (U"The accuracy of a KNN can be estimated by selecting a KNN, a test PatternList and the corresponding Categories object and choosing \"Evaluate...\".");
END2 }

DIRECT2 (hint_KNN_and_Pattern_and_Categories_and_FeatureWeights_evaluate) {
	Melder_information (U"The accuracy of a KNN can be estimated by selecting a KNN, a test PatternList, an FeatureWeights object, and the corresponding Categories object and choosing \"Evaluate...\".");
END2 }

DIRECT2 (hint_Pattern_and_FeatureWeights_to_Categories) {
	Melder_information (U"A PatternList object and a FeatureWeights object can be used to compute a fixed number of clusters using the k-means clustering clustering algorithm.");
END2 }

DIRECT2 (hint_Pattern_and_FeatureWeights_to_Dissimilarity) {
	Melder_information (U"A Dissimilarity matrix can be generated from a PatternList and a FeatureWeights object.");
END2 }

/////////////////////////////////////////////////////////////////////////////////////////
// Setting callbacks                                                                   //
/////////////////////////////////////////////////////////////////////////////////////////

void praat_contrib_Ola_KNN_init ();
void praat_contrib_Ola_KNN_init ()
{
    Thing_recognizeClassesByName (classKNN, classFeatureWeights, nullptr);

//////////
// Menu //
//////////

    praat_addMenuCommand (U"Objects", U"New", U"kNN classifiers", nullptr, 0, nullptr);

    praat_addMenuCommand (U"Objects", U"New", U"kNN classifiers", nullptr, praat_DEPTH_1 | praat_NO_API, DO_KNN_help);
    praat_addMenuCommand (U"Objects", U"New", U"-- KNN --", nullptr, 1, nullptr);

    praat_addMenuCommand (U"Objects", U"New", U"Create kNN classifier...", nullptr, 1, DO_KNN_create);

    praat_addMenuCommand (U"Objects", U"New", U"Advanced", nullptr, 1, nullptr);
    praat_addMenuCommand (U"Objects", U"New", U"Create Pattern...", nullptr, 2, DO_Pattern_create);
    //praat_addMenuCommand (U"Objects", U"New", U"Create Categories...", nullptr, 2, DO_Categories_create);
    praat_addMenuCommand (U"Objects", U"New", U"Create FeatureWeights...", nullptr, 2, DO_FeatureWeights_create);

/////////////
// Actions //
/////////////

    praat_addAction1 (classKNN, 0, U"kNN help", nullptr, 0, DO_KNN_help);
    praat_addAction1 (classKNN, 0, QUERY_BUTTON, nullptr, 0, nullptr);
    praat_addAction1 (classKNN, 1, U"Get optimized parameters...", nullptr, 2, DO_KNN_getOptimumModel);
    praat_addAction1 (classKNN, 1, U"Get accuracy estimate...", nullptr, 2, DO_KNN_evaluate);
    praat_addAction1 (classKNN, 1, U"Get size of instancebase", nullptr, 2, DO_KNN_getNumberOfInstances);

    praat_addAction1 (classKNN, 0, MODIFY_BUTTON, nullptr, 0, nullptr);
    praat_addAction1 (classKNN, 1, U"Shuffle", nullptr, 1, DO_KNN_shuffle);
    praat_addAction1 (classKNN, 1, U"Prune...", nullptr, 1, DO_KNN_prune);
    praat_addAction1 (classKNN, 1, U"Reset...", nullptr, 1, DO_KNN_reset);
    praat_addAction1 (classKNN, 0, EXTRACT_BUTTON, nullptr, 0, nullptr);
    praat_addAction1 (classKNN, 0, U"Extract input Patterns", nullptr, 1, DO_KNN_extractInputPatterns);
    praat_addAction1 (classKNN, 0, U"Extract output Categories", nullptr, 1, DO_KNN_extractOutputCategories);

    praat_addAction1 (classKNN, 0, U"To FeatureWeights...", nullptr, 0, DO_FeatureWeights_computeWrapperInt);

 // praat_addAction1 (classKNN, 0, U"To Permutation...", nullptr, 0, DO_KNN_SA_computePermutation);
 // praat_addAction2 (classKNN, 1, classFeatureWeights, 1, U"To Permutation...", nullptr, 0, DO_KNN_evaluateWithFeatureWeights);

    praat_addAction3 (classKNN, 1, classPatternList, 1, classCategories, 1, U"Learn...", nullptr, 0, DO_KNN_learn);
    praat_addAction2 (classKNN, 1, classFeatureWeights, 1, U"Evaluate...", nullptr, 0, DO_KNN_evaluateWithFeatureWeights);
    praat_addAction3 (classKNN, 1, classPatternList, 1, classCategories, 1, U"Evaluate...", nullptr, 0, DO_KNN_evaluateWithTestSet);
    praat_addAction4 (classKNN, 1, classPatternList, 1, classCategories, 1, classFeatureWeights, 1, U"Evaluate...", nullptr, 0, DO_KNN_evaluateWithTestSetAndFeatureWeights);
    praat_addAction3 (classKNN, 1, classPatternList, 1, classCategories, 1, U"To FeatureWeights...", nullptr, 0, DO_FeatureWeights_computeWrapperExt);
    praat_addAction2 (classKNN, 1, classPatternList, 1, U"To Categories...", nullptr, 0, DO_KNN_toCategories);
    praat_addAction2 (classKNN, 1, classPatternList, 1, U"To TableOfReal...", nullptr, 0, DO_KNN_toTableOfReal);

    praat_addAction3 (classKNN, 1, classPatternList, 1, classFeatureWeights, 1, U"To Categories...", nullptr, 0, DO_KNN_toCategoriesWithFeatureWeights);
    praat_addAction3 (classKNN, 1, classPatternList, 1, classFeatureWeights, 1, U"To TableOfReal...", nullptr, 0, DO_KNN_toTableOfRealWithFeatureWeights);

    praat_addAction1 (classPatternList, 1, U"To Dissimilarity", nullptr, 1, DO_KNN_patternToDissimilarity);
    praat_addAction1 (classPatternList, 1, U"To Categories...", nullptr, 1, DO_Pattern_to_Categories_cluster);
    praat_addAction2 (classPatternList, 1, classFeatureWeights, 1, U"To Dissimilarity", nullptr, 0, DO_KNN_patternToDissimilarityWithFeatureWeights);
    praat_addAction2 (classPatternList, 1, classFeatureWeights, 1, U"To Categories...", nullptr, 0, DO_Pattern_to_Categories_clusterWithFeatureWeights);

    praat_addAction2 (classPatternList, 1, classCategories, 1, U"To FeatureWeights...", nullptr, 0, DO_FeatureWeights_computeRELIEF);
    praat_addAction2 (classPatternList, 1, classCategories, 1, U"To KNN Classifier...", nullptr, 0, DO_KNN_Pattern_Categories_to_KNN);

///////////
// DEBUG //
///////////

/*
#ifdef _DEBUG

    praat_addAction1 (classKNN, 0, U"_DEBUG: KNN_getNumberOfCPUs", 0, 0, DO_KNN_debug_KNN_getNumberOfCPUs);
    praat_addAction1 (classKNN, 0, U"_DEBUG: KNN_threadTest", 0, 0, DO_KNN_debug_KNN_threadTest);
    praat_addAction1 (classPattern, 1, U"_DEBUG: KNN_SA_partition", 0, 1, DO_KNN_debug_KNN_SA_partition);

#endif
*/

///////////
// Hints //
///////////

    praat_addAction1 (classPatternList, 0, U"& FeatureWeights: To Categories?", nullptr, 0, DO_hint_Pattern_and_FeatureWeights_to_Categories);
    praat_addAction1 (classPatternList, 0, U"& FeatureWeights: To Dissimilarity?", nullptr, 0, DO_hint_Pattern_and_FeatureWeights_to_Dissimilarity);

    praat_addAction1 (classKNN, 0, U"& FeatureWeights: Evaluate?", nullptr, 0, DO_hint_KNN_and_FeatureWeights_evaluate);
//  praat_addAction1 (classKNN, 0, U"& FeatureWeights: To Permutation?", nullptr, 0, DO_hint_Pattern_and_FeatureWeights_to_Dissimilarity);
    praat_addAction1 (classKNN, 0, U"& Pattern: Classify?", nullptr, 0, DO_hint_KNN_and_Pattern_classify);
    praat_addAction1 (classKNN, 0, U"& Pattern & FeatureWeights: Classify?", nullptr, 0, DO_hint_KNN_and_Pattern_and_FeatureWeights_classify);
    praat_addAction1 (classKNN, 0, U"& Pattern & Categories: Learn?", nullptr, 0, DO_hint_KNN_and_Pattern_and_Categories_learn);
    praat_addAction1 (classKNN, 0, U"& Pattern & Categories: Evaluate?", nullptr, 0, DO_hint_KNN_and_Pattern_and_Categories_evaluate);
    praat_addAction1 (classKNN, 0, U"& Pattern & Categories & FeatureWeights: Evaluate?", nullptr, 0, DO_hint_KNN_and_Pattern_and_Categories_and_FeatureWeights_evaluate);

    INCLUDE_MANPAGES (manual_KNN_init)
}

/* End of file praat_contrib_Ola_KNN.cpp */
