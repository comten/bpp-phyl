//
// File: HomogeneousSequenceSimulator.cpp
//       (previously SequenceSimulator.cpp)
// Created by: Julien Dutheil
// Created on: Wed Feb  4 16:30:51 2004
//

/*
Copyright or � or Copr. CNRS, (November 16, 2004)

This software is a computer program whose purpose is to provide classes
for phylogenetic data analysis.

This software is governed by the CeCILL  license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
*/

#include "HomogeneousSequenceSimulator.h"

// From Utils:
#include <Utils/ApplicationTools.h>

// From SeqLib:
#include <Seq/VectorSiteContainer.h>

// From NumCalc:
#include <NumCalc/VectorTools.h>
using namespace VectorOperators;
#include <NumCalc/MatrixTools.h>

/******************************************************************************/

HomogeneousSequenceSimulator::HomogeneousSequenceSimulator(
	const MutationProcess * process,
	const DiscreteDistribution * rate,
	const TreeTemplate<Node> * tree,
	bool verbose):
	_process(process),
	_rate(rate),
	_tree(tree)
{
	_leaves   = _tree   ->getLeaves();
	_model    = _process->getSubstitutionModel();
	_alphabet = _model  ->getAlphabet();
	_seqNames.resize(_leaves.size());
	for(unsigned int i = 0; i < _seqNames.size(); i++)
    _seqNames[i] = _leaves[i]->getName();
	// Initialize cumulative pxy:
	if(verbose) ApplicationTools::displayTask("Initializing probabilities");
	vector<const Node *> nodes = _tree->getNodes();
	nodes.pop_back(); //remove root
	_nbNodes = nodes.size();
	_nbClasses = _rate->getNumberOfCategories();
	_nbStates = _model->getNumberOfStates();
  _continuousRates = false;
	for(unsigned int i = 0; i < nodes.size(); i++)
  {
		const Node * node = nodes[i];
		double d = node->getDistanceToFather();
		VVVdouble * _cumpxy_node = & _cumpxy[node];
		_cumpxy_node->resize(_nbClasses);
		for(unsigned int c = 0; c < _nbClasses; c++)
    {
			VVdouble * _cumpxy_node_c = & (* _cumpxy_node)[c];
			_cumpxy_node_c->resize(_nbStates);
			RowMatrix<double> P = _model->getPij_t(d * _rate->getCategory(c));
			for(unsigned int x = 0; x < _nbStates; x++)
      {
				Vdouble * _cumpxy_node_c_x = & (* _cumpxy_node_c)[x];
				_cumpxy_node_c_x->resize(_nbStates);
				(* _cumpxy_node_c_x)[0] = P(x, 0);
				for(unsigned int y = 1; y < _nbStates; y++)
        {
					(* _cumpxy_node_c_x)[y] = (* _cumpxy_node_c_x)[y - 1] + P(x, y);
				}
			}
		}
	}
	if(verbose) ApplicationTools::displayTaskDone();
}

/******************************************************************************/

Site * HomogeneousSequenceSimulator::simulate() const
{
	// Draw an initial state randomly according to equilibrum frequencies:
	int initialState = 0;
	double r = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	double cumprob = 0;
	for(unsigned int i = 0; i < _nbStates; i++)
  {
		cumprob += _model->freq(i);
		if(r <= cumprob)
    {
			initialState = (int)i;
			break;
		}
	}

	return simulate(initialState);
}

/******************************************************************************/
	
Site * HomogeneousSequenceSimulator::simulate(int initialState) const
{
  if(_continuousRates)
  {
	  // Draw a random rate:
	  double rate = _rate->randC();
	  // Make this state evolve:
	  return simulate(initialState, rate);
  }
  else
  {
	  // Draw a random rate:
	  unsigned int rateClass = (unsigned int)RandomTools::giveIntRandomNumberBetweenZeroAndEntry(_rate->getNumberOfCategories());
	  // Make this state evolve:
	  return simulate(initialState, rateClass);
  }
}

/******************************************************************************/

Site * HomogeneousSequenceSimulator::simulate(int initialState, unsigned int rateClass) const
{
	// Launch recursion:
	const Node * root = _tree->getRootNode();
	_states[root] = initialState;
	for(unsigned int i = 0; i < root->getNumberOfSons(); i++)
  {
		evolveInternal(root->getSon(i), rateClass);
	}
	// Now create a Site object:
	Vint site(_leaves.size());
	for(unsigned int i = 0; i < _leaves.size(); i++)
  {
		site[i] = _model->getState(_states[_leaves[i]]);
	}
	return new Site(site, _alphabet);
}

/******************************************************************************/
	
Site * HomogeneousSequenceSimulator::simulate(int initialState, double rate) const
{
	// Launch recursion:
	const Node * root = _tree->getRootNode();
	_states[root] = initialState;
	for(unsigned int i = 0; i < root->getNumberOfSons(); i++)
  {
		evolveInternal(root->getSon(i), rate);
	}
	// Now create a Site object:
	Vint site(_leaves.size());
	for(unsigned int i = 0; i < _leaves.size(); i++)
  {
		site[i] = _model->getState(_states[_leaves[i]]);
	}
	return new Site(site, _alphabet);
}

/******************************************************************************/

Site * HomogeneousSequenceSimulator::simulate(double rate) const
{
	// Draw an initial state randomly according to equilibrum frequencies:
	int initialState = 0;
	double r = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	double cumprob = 0;
	for(unsigned int i = 0; i < _nbStates; i++)
  {
		cumprob += _model->freq(i);
		if(r <= cumprob)
    {
			initialState = (int)i;
			break;
		}
	}
	// Make this state evolve:
	return simulate(initialState, rate);
}

/******************************************************************************/
		
SiteContainer * HomogeneousSequenceSimulator::simulate(unsigned int numberOfSites) const
{
	Vint * initialStates = new Vint(numberOfSites, 0);
	for(unsigned int j = 0; j < numberOfSites; j++)
  { 
		double r = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
		double cumprob = 0;
		for(unsigned int i = 0; i < _nbStates; i++)
    {
      cumprob += _model->freq(i);
			if(r <= cumprob)
      {
        (* initialStates)[j] = (int)i;
				break;
			}
		}
	}
  if(_continuousRates)
  {
	  VectorSiteContainer * sites = new VectorSiteContainer(_seqNames.size(), _alphabet);
    sites->setSequencesNames(_seqNames);
    for(unsigned int j = 0; j < numberOfSites; j++)
    {
      Site * site = simulate();
      site->setPosition(j);
      sites->addSite(*site);
      delete site;
    }
    return sites;
  }
  else
  { //More efficient to do site this way:
	  // Draw random rates:
	  vector<unsigned int> rateClasses(numberOfSites);
	  unsigned int nCat = _rate->getNumberOfCategories();
	  for(unsigned int j = 0; j < numberOfSites; j++)
      rateClasses[j] = (unsigned int)RandomTools::giveIntRandomNumberBetweenZeroAndEntry(nCat);
	  // Make these states evolve:
	  SiteContainer * sites = multipleEvolve(* initialStates, rateClasses);
	  return sites;
  }
}

/******************************************************************************/

HomogeneousSiteSimulationResult * HomogeneousSequenceSimulator::dSimulate() const
{
	// Draw an initial state randomly according to equilibrum frequencies:
	int initialState = 0;
	double r = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	double cumprob = 0;
	for(unsigned int i = 0; i < _nbStates; i++)
  {
		cumprob += _model->freq(i);
		if(r <= cumprob)
    {
			initialState = i;
			break;
		}
	}
  
	return dSimulate(initialState);
}

/******************************************************************************/

HomogeneousSiteSimulationResult * HomogeneousSequenceSimulator::dSimulate(int initialState) const
{
	// Draw a random rate:
	if(_continuousRates)
  {
    double rate = _rate->randC();
	  return dSimulate(initialState, rate);
  }
  else
  {
    unsigned int rateClass = (unsigned int)RandomTools::giveIntRandomNumberBetweenZeroAndEntry(_rate->getNumberOfCategories());
	  return dSimulate(initialState, rateClass);
    //NB: this is more efficient than dSimulate(initialState, _rDist->rand())
  }
}

/******************************************************************************/

HomogeneousSiteSimulationResult * HomogeneousSequenceSimulator::dSimulate(int initialState, double rate) const
{
	// Make this state evolve:
	HomogeneousSiteSimulationResult * hssr = new HomogeneousSiteSimulationResult(_tree, _process->getSubstitutionModel()->getAlphabet(), initialState, rate);
	dEvolve(initialState, rate, *hssr);
	return hssr;
}

/******************************************************************************/

HomogeneousSiteSimulationResult * HomogeneousSequenceSimulator::dSimulate(int initialState, unsigned int rateClass) const
{
	return dSimulate(initialState, _rate->getCategory(rateClass));
}

/******************************************************************************/

HomogeneousSiteSimulationResult * HomogeneousSequenceSimulator::dSimulate(double rate) const
{
	// Draw an initial state randomly according to equilibrum frequencies:
	int initialState = 0;
	double r = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	double cumprob = 0;
	for(unsigned int i = 0; i < _nbStates; i++)
  {
		cumprob += _model->freq(i);
		if(r <= cumprob)
    {
			initialState = i;
			break;
		}
	}
  
	return dSimulate(initialState, rate);
}

/******************************************************************************/

int HomogeneousSequenceSimulator::evolve(const Node * node, int initialState, unsigned int rateClass) const
{
	Vdouble * _cumpxy_node_c_x = & _cumpxy[node][rateClass][initialState];
	double rand = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	for(int y = 0; y < (int)_nbStates; y++)
  {
		if(rand < (* _cumpxy_node_c_x)[y]) return y;
	}
	cerr << "DEBUG: This message should never happen! (HomogeneousSequenceSimulator::evolve)" << endl;
	cout << "   rand = " << rand << endl;
  return -1;
}

/******************************************************************************/

int HomogeneousSequenceSimulator::evolve(const Node * node, int initialState, double rate) const
{
	double _cumpxy = 0;
	double rand = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
	double l = rate * node->getDistanceToFather();
	for(int y = 0; y < (int)_nbStates; y++)
  {
		_cumpxy += _model->Pij_t(initialState, y, l);
		if(rand < _cumpxy) return y;
	}
	cerr << "DEBUG: This message should never happen! (HomogeneousSequenceSimulator::evolve)" << endl;
	cout << "   rand = " << rand << endl;
	cout << "_cumpxy = " << _cumpxy << endl;
	MatrixTools::print(_model->getPij_t(l));
  return -1;
}

/******************************************************************************/

void HomogeneousSequenceSimulator::multipleEvolve(const Node * node, const Vint & initialStates, const vector<unsigned int> & rateClasses, Vint & finalStates) const
{
	VVVdouble * _cumpxy_node = & _cumpxy[node];
	for(unsigned int i = 0; i < initialStates.size(); i++)
  {
		Vdouble * _cumpxy_node_c_x = & (* _cumpxy_node)[rateClasses[i]][initialStates[i]];
		double rand = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.);
		for(unsigned int y = 0; y < _nbStates; y++)
    {
			if(rand < (* _cumpxy_node_c_x)[y])
      {
        finalStates[i] = (int)y;
				break;
			}
		}
	}
}

/******************************************************************************/

void HomogeneousSequenceSimulator::evolveInternal(const Node * node, unsigned int rateClass) const
{
	if(!node->hasFather())
  { 
    cerr << "DEBUG: HomogeneousSequenceSimulator::evolveInternal. Forbidden call of method on root node." << endl;
    return;
  }
	int initialState = _states[node->getFather()];
	int   finalState = evolve(node, initialState, rateClass);
	_states[node] = finalState;
	for(unsigned int i = 0; i < node->getNumberOfSons(); i++)
  {
		evolveInternal(node->getSon(i), rateClass);
	}
}

/******************************************************************************/

void HomogeneousSequenceSimulator::evolveInternal(const Node * node, double rate) const
{
	if(!node->hasFather())
  { 
    cerr << "DEBUG: HomogeneousSequenceSimulator::evolveInternal. Forbidden call of method on root node." << endl;
    return;
  }
	int initialState = _states[node->getFather()];
	int finalState = evolve(node, initialState, rate);
	_states[node] = finalState;
	for(unsigned int i = 0; i < node->getNumberOfSons(); i++)
  {
		evolveInternal(node->getSon(i), rate);
	}
}

/******************************************************************************/

void HomogeneousSequenceSimulator::multipleEvolveInternal(const Node * node, const vector<unsigned int> & rateClasses) const
{
	if(!node->hasFather())
  { 
    cerr << "DEBUG: HomogeneousSequenceSimulator::multipleEvolveInternal. Forbidden call of method on root node." << endl;
    return;
  }
	const vector<int> * initialStates = &_multipleStates[node->getFather()];
	unsigned int n = initialStates->size();
	_multipleStates[node].resize(n); //allocation.
	multipleEvolve(node, *initialStates, rateClasses, _multipleStates[node]);
	for(unsigned int i = 0; i < node->getNumberOfSons(); i++)
  {
		multipleEvolveInternal(node->getSon(i), rateClasses);
	}
}

/******************************************************************************/

SiteContainer * HomogeneousSequenceSimulator::multipleEvolve(const Vint & initialStates, const vector<unsigned int> & rateClasses) const
{
	// Launch recursion:
	const Node * root = _tree->getRootNode();
	_multipleStates[root] = initialStates;
	for(unsigned int i = 0; i < root->getNumberOfSons(); i++)
  {
		multipleEvolveInternal(root->getSon(i), rateClasses);
	}
	// Now create a SiteContainer object:
	AlignedSequenceContainer * sites = new AlignedSequenceContainer(_alphabet);
	unsigned int n = _leaves.size();
  unsigned int nbSites = initialStates.size();
	for(unsigned int i = 0; i < n; i++)
  {
    vector<int> content(nbSites);
    vector<int> * states = & _multipleStates[_leaves[i]];
    for(unsigned int j = 0; j < nbSites; j++)
    {
      content[j] = _model->getState((*states)[j]);
    }
		sites->addSequence(Sequence(_leaves[i]->getName(), content, _alphabet), false);
	}
	return sites;
}

/******************************************************************************/
	
void HomogeneousSequenceSimulator::dEvolve(int initialState, double rate, HomogeneousSiteSimulationResult & hssr) const
{
	// Launch recursion:
	const Node * root = _tree->getRootNode();
	_states[root] = initialState;
	for(unsigned int i = 0; i < root->getNumberOfSons(); i++)
  {
		dEvolveInternal(root->getSon(i), rate, hssr);
	}
}

/******************************************************************************/

void HomogeneousSequenceSimulator::dEvolveInternal(const Node * node, double rate, HomogeneousSiteSimulationResult & hssr) const
{
	if(!node->hasFather())
  { 
    cerr << "DEBUG: HomogeneousSequenceSimulator::evolveInternal. Forbidden call of method on root node." << endl;
    return;
  }
	int initialState = _states[node->getFather()];
	MutationPath mp = _process->detailedEvolve(initialState, node->getDistanceToFather() * rate);
	_states[node] = mp.getFinalState();

	// Now append infos in _ssr:
	hssr.addNode(node, mp);

	// Now jump to son nodes:
	for(unsigned int i = 0; i < node->getNumberOfSons(); i++)
  {
		dEvolveInternal(node->getSon(i), rate, hssr);
	}
}

/******************************************************************************/
