//
// File: WordReversibleSubstitutionModel.cpp
// Created by:  Laurent Gueguen
// Created on: Jan 2009
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

#include "WordReversibleSubstitutionModel.h"
#include "FrequenciesSet.h"

#include <Bpp/Numeric/Matrix/MatrixTools.h>
#include <Bpp/Numeric/VectorTools.h>
#include <Bpp/Numeric/Matrix/EigenValue.h>

// From SeqLib:
#include <Bpp/Seq/Alphabet/WordAlphabet.h>
#include <Bpp/Seq/Container/SequenceContainerTools.h>

using namespace bpp;

// From the STL:
#include <cmath>

using namespace std;

/******************************************************************************/

WordReversibleSubstitutionModel::WordReversibleSubstitutionModel(
  const std::vector<SubstitutionModel*>& modelVector,
  const std::string& st) :
  AbstractWordReversibleSubstitutionModel(modelVector, (st == "") ? "Word." : st)
{
   unsigned int i, nbmod = VSubMod_.size();

  // relative rates
  for (i = 0; i < nbmod - 1; i++)
  {
    addParameter_(Parameter("Word.relrate" + TextTools::toString(i+1), 1.0 / (nbmod - i), &Parameter::PROP_CONSTRAINT_EX));
  }

  WordReversibleSubstitutionModel::updateMatrices();
}

WordReversibleSubstitutionModel::WordReversibleSubstitutionModel(
  const Alphabet* alph,
  const std::string& st) :
  AbstractWordReversibleSubstitutionModel(alph, (st == "") ? "Word." : st)
{}

WordReversibleSubstitutionModel::WordReversibleSubstitutionModel(
  SubstitutionModel* pmodel,
  unsigned int num,
  const std::string& st) :
  AbstractWordReversibleSubstitutionModel(pmodel, num,  (st == "") ? "Word." : st)
{
   unsigned int i;

  // relative rates
  for (i = 0; i < num - 1; i++)
  {
    addParameter_(Parameter("Word.relrate" + TextTools::toString(i+1), 1.0 / (num - i ), &Parameter::PROP_CONSTRAINT_EX));
  }

  WordReversibleSubstitutionModel::updateMatrices();
}

void WordReversibleSubstitutionModel::updateMatrices()
{
   int i,k, nbmod = VSubMod_.size();
   double x;
  for (k = nbmod - 1; k >= 0; k--)
  {
    x = 1.0;
    for (i = 0; i < k; i++)
    {
      x *= 1 - getParameterValue("relrate" + TextTools::toString(i+1));
    }
    if (k != nbmod - 1)
      x *= getParameterValue("relrate" + TextTools::toString(k+1));
    rate_[k] = x;
  }

  AbstractWordReversibleSubstitutionModel::updateMatrices();
}

void WordReversibleSubstitutionModel::completeMatrices()
{
   int nbmod = VSubMod_.size();
   int i,p,j,m;
   int salph = getAlphabet()->getSize();

  // freq_ for this generator

  for (i = 0; i < salph; i++)
  {
    freq_[i] = 1;
    j = i;
    for (p = nbmod - 1; p >= 0; p--)
    {
      m = VSubMod_[p]->getNumberOfStates();
      freq_[i] *= VSubMod_[p]->getFrequencies()[j % m];
      j /= m;
    }
  }
}

double WordReversibleSubstitutionModel::Pij_t(unsigned int i, unsigned int j, double d) const
{
   double x = 1;
   unsigned int nbmod = VSubMod_.size();
   unsigned int t;
   int p;

   unsigned int i2 = i;
   unsigned int j2 = j;

  for (p = nbmod - 1; p >= 0; p--)
  {
    t = VSubMod_[p]->getNumberOfStates();
    x *= VSubMod_[p]->Pij_t(i2 % t, j2 % t, d * rate_[p]);
    i2 /= t;
    j2 /= t;
  }

  return x;
}

const RowMatrix<double>& WordReversibleSubstitutionModel::getPij_t(double d) const
{
   unsigned int nbStates = getNumberOfStates();
   unsigned int i, j;

  for (i = 0; i < nbStates; i++)
  {
    for (j = 0; j < nbStates; j++)
    {
      p_(i,j) = Pij_t(i,j,d);
    }
  }
  return p_;
}

double WordReversibleSubstitutionModel::dPij_dt(unsigned int i, unsigned int j, double d) const
{
   double r, x;
   int nbmod = VSubMod_.size();
   int t;
   int p,q;

   int i2 = i;
   int j2 = j;

  r = 0;
  for (q = 0; q < nbmod; q++)
  {
    i2 = i;
    j2 = j;
    x = 1;
    for (p = nbmod - 1; p >= 0; p--)
    {
      t = VSubMod_[p]->getNumberOfStates();
      if (q != p)
        x *= VSubMod_[p]->Pij_t(i2 % t,j2 % t,d * rate_[p]);
      else
        x *= rate_[p] * VSubMod_[p]->dPij_dt(i2 % t,j2 % t,d * rate_[p]);
      i2 /= t;
      j2 /= t;
    }
    r += x;
  }
  return r;
}

const RowMatrix<double>& WordReversibleSubstitutionModel::getdPij_dt(double d) const
{
   unsigned int nbetats = getNumberOfStates();
   unsigned int i, j;

  for (i = 0; i < nbetats; i++)
  {
    for (j = 0; j < nbetats; j++)
    {
      p_(i,j) = dPij_dt(i,j,d);
    }
  }
  return p_;
}

double WordReversibleSubstitutionModel::d2Pij_dt2(unsigned int i, unsigned int j, double d) const
{
   double r, x;
   int nbmod = VSubMod_.size();
   int b, q, t;
   int p;

   int i2 = i;
   int j2 = j;

  r = 0;

  for (q = 1; q < nbmod; q++)
  {
    for (b = 0; b < q; b++)
    {
      x = 1;
      i2 = i;
      j2 = j;
      for (p = nbmod - 1; p >= 0; p--)
      {
        t = VSubMod_[p]->getNumberOfStates();
        if (p == q)
          x *= rate_[p] * VSubMod_[p]->dPij_dt(i2 % t, j2 % t, d * rate_[p]);
        else if (p == b)
          x *= rate_[p] * VSubMod_[p]->dPij_dt(i2 % t, j2 % t, d * rate_[p]);
        else
          x *= VSubMod_[p]->Pij_t(i2 % t, j2 % t, d * rate_[p]);

        i2 /= t;
        j2 /= t;
      }
      r += x;
    }
  }

  r *= 2;

  for (q = 0; q < nbmod; q++)
  {
    x = 1;
    i2 = i;
    j2 = j;
    for (p = nbmod - 1; p >= 0; p--)
    {
      t = VSubMod_[p]->getNumberOfStates();
      if (q != p)
        x *= VSubMod_[p]->Pij_t(i2 % t, j2 % t, d * rate_[p]);
      else
        x *= rate_[p] * rate_[p] * VSubMod_[p]->d2Pij_dt2(i2 % t, j2 % t, d * rate_[p]);

      i2 /= t;
      j2 /= t;
    }
    r += x;
  }

  return r;
}

const RowMatrix<double>& WordReversibleSubstitutionModel::getd2Pij_dt2(double d) const
{
   unsigned int nbetats = getNumberOfStates();
   unsigned int i,j;

  for (i = 0; i < nbetats; i++)
  {
    for (j = 0; j < nbetats; j++)
    {
      p_(i,j) = Pij_t(i,j,d);
    }
  }

  return p_;
}


string WordReversibleSubstitutionModel::getName() const
{
   unsigned int nbmod = VSubMod_.size();
   string s = "WordReversibleSubstitutionModel model: " + VSubMod_[0]->getName();
  for (unsigned int i = 1; i < nbmod - 1; i++)
  {
    s += " " + VSubMod_[i]->getName();
  }

  return s;
}

