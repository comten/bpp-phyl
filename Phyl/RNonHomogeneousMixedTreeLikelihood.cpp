//
// File: RNonHomogeneousMixedTreeLikelihood.cpp
// Created by: Laurent Gueguen
// Created on: December 2009
// From file: RHomogeneousMixedTreeLikelihood.cpp
//

/*
   Copyright or © or Copr. CNRS, (November 16, 2004)

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

#include "RNonHomogeneousMixedTreeLikelihood.h"
#include "PatternTools.h"
#include "MixedSubstitutionModel.h"
#include "models"

// From Utils:
#include <Utils/TextTools.h>
#include <Utils/ApplicationTools.h>

using namespace bpp;

// From the STL:
#include <iostream>

using namespace std;

/******************************************************************************/

RNonHomogeneousMixedTreeLikelihood::RNonHomogeneousMixedTreeLikelihood(
  const Tree& tree,
  SubstitutionModelSet* modelSet,
  DiscreteDistribution* rDist,
  bool verbose,
  bool usePatterns)
throw (Exception) :
  RNonHomogeneousTreeLikelihood(tree, modelSet, rDist, verbose, usePatterns),
  treeLikelihoodsContainer_(), internParam_()
{
  if (!modelSet->isFullySetUpFor(tree))
    throw Exception("RNonHomogeneousMixedTreeLikelihood(constructor). Model set is not fully specified.");

  map<int, int> mapmodels;
  unsigned int ttmodels = 1;
  unsigned int nbmodels = modelSet->getNumberOfModels();
  MixedSubstitutionModel* pmsm;
  SubstitutionModel* psm;
  for (unsigned int i = 0; i < nbmodels; i++)
  {
    if ((pmsm = dynamic_cast< MixedSubstitutionModel*>(modelSet->getModel(i))) != 0)
      mapmodels[i] = pmsm->getNumberOfModels();
    else
      mapmodels[i] = 1;
    ttmodels *= mapmodels[i];
  }

  SubstitutionModelSet* psms;
  int s;
  vector<string> vn;

  for (unsigned int i = 0; i < ttmodels; i++)
  {
    s = i;

    psms = new SubstitutionModelSet(modelSet->getAlphabet(),modelSet->getRootFrequenciesSet()->clone());

    for (unsigned int j = 0; j < nbmodels; j++)
    {
      if (mapmodels[j] == 1)
        psms->addModel(modelSet->getModel(j),modelSet->getNodesWithModel(j),
                       modelSet->getModel(j)->getParameters().getParameterNames());
      else
      {
        pmsm = dynamic_cast< MixedSubstitutionModel*>(modelSet->getModel(j));
        psm = pmsm->getNModel(s % mapmodels[j]);
        psms->addModel(psm,modelSet->getNodesWithModel(j),
                       psm->getParameters().getParameterNames());

        vn = psms->getModelParameters(j).getParameterNames();
        for (unsigned int i2 = 0; i2 < vn.size(); i2++)
        {
          Parameter p = psm->getParameter(psm->getParameterNameWithoutNamespace(psms->getParameterModelName(vn[i2])));
          if (!internParam_.hasParameter(vn[i2]))
            internParam_.addParameter(
                Parameter(vn[i2], p.getValue(), p.getConstraint()->clone(), true));
        }

        s /= mapmodels[j];
      }
    }
    treeLikelihoodsContainer_.push_back(
        new RNonHomogeneousTreeLikelihood(tree, psms, rDist, false, usePatterns));
  }
}

/******************************************************************************/

RNonHomogeneousMixedTreeLikelihood::RNonHomogeneousMixedTreeLikelihood(
  const Tree& tree,
  const SiteContainer& data,
  SubstitutionModelSet* modelSet,
  DiscreteDistribution* rDist,
  bool verbose,
  bool usePatterns)
throw (Exception) :
  RNonHomogeneousTreeLikelihood(tree, modelSet, rDist, verbose, usePatterns),
  treeLikelihoodsContainer_(), internParam_()
{
  if (!modelSet->isFullySetUpFor(tree))
    throw Exception("RNonHomogeneousMixedTreeLikelihood(constructor). Model set is not fully specified.");

  map<int, int> mapmodels;
  unsigned int ttmodels = 1;
  unsigned int nbmodels = modelSet->getNumberOfModels();
  MixedSubstitutionModel* pmsm;
  SubstitutionModel* psm;
  for (unsigned int i = 0; i < nbmodels; i++)
  {
    if ((pmsm = dynamic_cast< MixedSubstitutionModel*>(modelSet->getModel(i))) != 0)
      mapmodels[i] = pmsm->getNumberOfModels();
    else
      mapmodels[i] = 1;
    ttmodels *= mapmodels[i];
  }

  SubstitutionModelSet* psms;
  int s;
  vector<string> vn;

  for (unsigned int i = 0; i < ttmodels; i++)
  {
    s = i;

    psms = new SubstitutionModelSet(modelSet->getAlphabet(),modelSet->getRootFrequenciesSet()->clone());

    for (unsigned int j = 0; j < nbmodels; j++)
    {
      if (mapmodels[j] == 1)
        psms->addModel(modelSet->getModel(j),modelSet->getNodesWithModel(j),
                       modelSet->getModel(j)->getParameters().getParameterNames());
      else
      {
        pmsm = dynamic_cast< MixedSubstitutionModel*>(modelSet->getModel(j));
        psm = pmsm->getNModel(s % mapmodels[j]);
        psms->addModel(psm,modelSet->getNodesWithModel(j),
                       psm->getParameters().getParameterNames());

        vn = psms->getModelParameters(j).getParameterNames();
        for (unsigned int i2 = 0; i2 < vn.size(); i2++)
        {
          Parameter p = psm->getParameter(psm->getParameterNameWithoutNamespace(psms->getParameterModelName(vn[i2])));
          if (!internParam_.hasParameter(vn[i2]))
            internParam_.addParameter(
                Parameter(vn[i2], p.getValue(), p.getConstraint()->clone(), true));
        }
        s /= mapmodels[j];
      }
    }
    treeLikelihoodsContainer_.push_back(
      new RNonHomogeneousTreeLikelihood(tree, psms, rDist, false, usePatterns));
  }

  setData(data);
}

/******************************************************************************/

RNonHomogeneousMixedTreeLikelihood::RNonHomogeneousMixedTreeLikelihood(
  const RNonHomogeneousMixedTreeLikelihood& lik) :
  RNonHomogeneousTreeLikelihood(lik),
  treeLikelihoodsContainer_(lik.treeLikelihoodsContainer_.size()),
  internParam_(lik.internParam_)
{
  for (unsigned int i = 0; i < lik.treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i] =lik.treeLikelihoodsContainer_[i]->clone();
  }
}

/******************************************************************************/

RNonHomogeneousMixedTreeLikelihood& RNonHomogeneousMixedTreeLikelihood::operator=(
  const RNonHomogeneousMixedTreeLikelihood& lik)
{
  RNonHomogeneousTreeLikelihood::operator=(lik);

  internParam_ = lik.internParam_;
  
  for (unsigned int i = 0; i < lik.treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_.push_back(lik.treeLikelihoodsContainer_[i]->clone());
  }
  return *this;
}

/******************************************************************************/

RNonHomogeneousMixedTreeLikelihood::~RNonHomogeneousMixedTreeLikelihood()
{
  unsigned s = treeLikelihoodsContainer_.size();
  for (unsigned int i = 0; i < s; i++)
  {
    delete treeLikelihoodsContainer_[i];
  }
}


/******************************************************************************/
void RNonHomogeneousMixedTreeLikelihood::initialize() throw (Exception)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->initialize();
  }
  RNonHomogeneousTreeLikelihood::initialize();
}


/******************************************************************************/
void RNonHomogeneousMixedTreeLikelihood::setData(const SiteContainer& sites) throw (Exception)
{
   RNonHomogeneousTreeLikelihood::setData(sites);
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->setData(sites);
  }
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLikelihood() const
{
  return exp(getLogLikelihood());
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLogLikelihood() const
{
   vector<double> reslog;

  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   reslog.push_back(treeLikelihoodsContainer_[i]->getLogLikelihood());
  }

  return VectorTools::logmeanexp(reslog);
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLikelihoodForASite(unsigned int site) const
{
  return exp(getLogLikelihoodForASite(site));
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLogLikelihoodForASite(unsigned int site) const
{
   vector<double> reslog;

  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   reslog.push_back(treeLikelihoodsContainer_[i]->getLogLikelihoodForASite(site));
  }

  return VectorTools::logmeanexp(reslog);
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
  return exp(getLogLikelihoodForASiteForARateClass(site, rateClass));
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLogLikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
   vector<double> reslog;

  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   reslog.push_back(treeLikelihoodsContainer_[i]->getLogLikelihoodForASiteForARateClass(site, rateClass));
  }

  return VectorTools::logmeanexp(reslog);
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLikelihoodForASiteForARateClassForAState(unsigned int site, unsigned int rateClass, int state) const
{
  return exp(getLogLikelihoodForASiteForARateClassForAState(site, rateClass, state));
}

/******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getLogLikelihoodForASiteForARateClassForAState(unsigned int site, unsigned int rateClass, int state) const
{
   vector<double> reslog;

  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   reslog.push_back(treeLikelihoodsContainer_[i]->getLogLikelihoodForASiteForARateClassForAState(site, rateClass, state));
  }

  return VectorTools::logmeanexp(reslog);
}

/******************************************************************************/
void RNonHomogeneousMixedTreeLikelihood::fireParameterChanged(const ParameterList& params)
{
  applyParameters();

  unsigned int s;
  const SubstitutionModel* psm;
  MixedSubstitutionModel* pm;
  SubstitutionModelSet* modelSet = getSubstitutionModelSet();
  SubstitutionModelSet* psms;

  ParameterList par;
  vector<string> vp;

  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    s = i;
    for (unsigned int j = 0; j < modelSet->getNumberOfModels(); j++)
    {
      pm = dynamic_cast<MixedSubstitutionModel*>(modelSet->getModel(j));

      if (pm != NULL)
      {
        psm = pm->getNModel(s % pm->getNumberOfModels());

        psms = treeLikelihoodsContainer_[i]->getSubstitutionModelSet();
        par = psms->getNodeParameters();
        vp = par.getParameterNames();

        for (unsigned int i2 = 0; i2 < vp.size(); i2++)
        {
          if (internParam_.hasParameter(vp[i2]))
            internParam_.setParameterValue(vp[i2],
                                           psm->getParameterValue(psm->getParameterNameWithoutNamespace(psms->getParameterModelName(vp[i2]))));
        }


        treeLikelihoodsContainer_[i]->matchParametersValues(internParam_);

        s /= pm->getNumberOfModels();
      }
    }
    treeLikelihoodsContainer_[i]->matchParametersValues(getParameters());
  }
  minusLogLik_ = -getLogLikelihood();
}

/******************************************************************************/
void RNonHomogeneousMixedTreeLikelihood::computeTreeLikelihood()
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeTreeLikelihood();
  }
}

void RNonHomogeneousMixedTreeLikelihood::computeSubtreeLikelihood(const Node* node)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeSubtreeLikelihood(node);
  }
}

/******************************************************************************
*                           First Order Derivatives                          *
******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getDLikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getDLikelihoodForASiteForARateClass(site,rateClass));
  }

  return VectorTools::mean<double, double>(rescontainer);
}

double RNonHomogeneousMixedTreeLikelihood::getDLikelihoodForASite(unsigned int site) const
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getDLikelihoodForASite(site));
  }

  return VectorTools::mean<double, double>(rescontainer);
}

void RNonHomogeneousMixedTreeLikelihood::computeTreeDLikelihood(const string& variable)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeTreeDLikelihood(variable);
  }
}

double RNonHomogeneousMixedTreeLikelihood::getFirstOrderDerivative(const string& variable) const
throw (Exception)
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getFirstOrderDerivative(variable));
  }

  return VectorTools::mean<double, double>(rescontainer);
}

void RNonHomogeneousMixedTreeLikelihood::computeDownSubtreeDLikelihood(const Node* node)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeDownSubtreeDLikelihood(node);
  }
}


/******************************************************************************
*                           Second Order Derivatives                         *
******************************************************************************/
double RNonHomogeneousMixedTreeLikelihood::getSecondOrderDerivative(const string& variable) const throw (Exception)
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getSecondOrderDerivative(variable));
  }

  return VectorTools::mean<double, double>(rescontainer);
}


double RNonHomogeneousMixedTreeLikelihood::getD2LikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getD2LikelihoodForASiteForARateClass(site,rateClass));
  }

  return VectorTools::mean<double, double>(rescontainer);
}

double RNonHomogeneousMixedTreeLikelihood::getD2LikelihoodForASite(unsigned int site) const
{
   vector<double> rescontainer;
   unsigned int i;

  for (i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
   rescontainer.push_back(treeLikelihoodsContainer_[i]->getD2LikelihoodForASite(site));
  }

  return VectorTools::mean<double, double>(rescontainer);
}

void RNonHomogeneousMixedTreeLikelihood::computeTreeD2Likelihood(const string& variable)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeTreeD2Likelihood(variable);
  }
}

void RNonHomogeneousMixedTreeLikelihood::computeDownSubtreeD2Likelihood(const Node* node)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->computeDownSubtreeD2Likelihood(node);
  }
}


/******************************************************************************/
void RNonHomogeneousMixedTreeLikelihood::displayLikelihood(const Node* node)
{
  for (unsigned int i = 0; i < treeLikelihoodsContainer_.size(); i++)
  {
    treeLikelihoodsContainer_[i]->displayLikelihood(node);
  }
}

/*******************************************************************************/
