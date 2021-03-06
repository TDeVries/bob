/**
 * @file trainer/cxx/MLPBaseTrainer.cc
 * @date Tue May 14 12:04:51 CEST 2013
 * @author Andre Anjos <andre.anjos@idiap.ch>
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
 * Copyright (C) 2011-2013 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <bob/core/assert.h>
#include <bob/core/check.h>
#include <bob/core/Exception.h>
#include <bob/math/linear.h>
#include <bob/machine/MLPException.h>
#include <bob/trainer/Exception.h>
#include <bob/trainer/MLPBaseTrainer.h>

bob::trainer::MLPBaseTrainer::MLPBaseTrainer(size_t batch_size):
  m_batch_size(batch_size),
  m_train_bias(true),
  m_H(0), ///< handy!
  m_delta(1),
  m_delta_bias(1),
  m_error(1),
  m_output(1)
{
  m_delta[0].reference(blitz::Array<double,2>(0,0));
  m_delta_bias[0].reference(blitz::Array<double,1>(0));
  m_error[0].reference(blitz::Array<double,2>(0,0));
  m_output[0].reference(blitz::Array<double,2>(0,0));

  reset();
}

bob::trainer::MLPBaseTrainer::MLPBaseTrainer(const bob::machine::MLP& machine,
    size_t batch_size):
  m_batch_size(batch_size),
  m_train_bias(true),
  m_H(machine.numOfHiddenLayers()), ///< handy!
  m_delta(m_H + 1),
  m_delta_bias(m_H + 1),
  m_error(m_H + 1),
  m_output(m_H + 1)
{
  const std::vector<blitz::Array<double,2> >& machine_weight =
    machine.getWeights();
  const std::vector<blitz::Array<double,1> >& machine_bias =
    machine.getBiases();

  for (size_t k=0; k<(m_H + 1); ++k) {
    m_delta[k].reference(blitz::Array<double,2>(machine_weight[k].shape()));
    m_delta_bias[k].reference(blitz::Array<double,1>(machine_bias[k].shape()));
  }

  reset();

  setBatchSize(batch_size);
}

bob::trainer::MLPBaseTrainer::~MLPBaseTrainer() { }

bob::trainer::MLPBaseTrainer::MLPBaseTrainer(const MLPBaseTrainer& other):
  m_batch_size(other.m_batch_size),
  m_train_bias(other.m_train_bias),
  m_H(other.m_H)
{
  bob::core::array::ccopy(other.m_delta, m_delta);
  bob::core::array::ccopy(other.m_delta_bias, m_delta_bias);
  bob::core::array::ccopy(other.m_error, m_error);
  bob::core::array::ccopy(other.m_output, m_output);
}

bob::trainer::MLPBaseTrainer& bob::trainer::MLPBaseTrainer::operator=
(const bob::trainer::MLPBaseTrainer& other) {
  if (this != &other)
  {
    m_batch_size = other.m_batch_size;
    m_train_bias = other.m_train_bias;
    m_H = other.m_H;

    bob::core::array::ccopy(other.m_delta, m_delta);
    bob::core::array::ccopy(other.m_delta_bias, m_delta_bias);
    bob::core::array::ccopy(other.m_error, m_error);
    bob::core::array::ccopy(other.m_output, m_output);
  }
  return *this;
}

void bob::trainer::MLPBaseTrainer::setBatchSize (size_t batch_size) {
  // m_output: values after the activation function
  // m_error: error values;
 
  m_batch_size = batch_size;
   
  for (size_t k=0; k<m_output.size(); ++k) {
    m_output[k].resize(batch_size, m_delta[k].extent(1));
    m_output[k] = 0.;
  }

  for (size_t k=0; k<m_error.size(); ++k) {
    m_error[k].resize(batch_size, m_delta[k].extent(1));
    m_error[k] = 0.;
  }
}

bool bob::trainer::MLPBaseTrainer::isCompatible(const bob::machine::MLP& machine) const 
{
  if (m_H != machine.numOfHiddenLayers()) return false;
  
  if (m_delta.back().extent(1) != (int)machine.outputSize()) return false;

  if (m_delta[0].extent(0) != (int)machine.inputSize()) return false;

  //also, each layer should be of the same size
  for (size_t k=0; k<(m_H + 1); ++k) {
    if (!bob::core::array::hasSameShape(m_delta[k], machine.getWeights()[k])) return false;
  }

  //if you get to this point, you can only return true
  return true;
}

void bob::trainer::MLPBaseTrainer::forward_step(const bob::machine::MLP& machine, 
  const blitz::Array<double,2>& input)
{
  const std::vector<blitz::Array<double,2> >& machine_weight = machine.getWeights();
  const std::vector<blitz::Array<double,1> >& machine_bias = machine.getBiases();

  bob::machine::MLP::actfun_t actfun = machine.getActivationFunction();
  bob::machine::MLP::actfun_t output_actfun = machine.getOutputActivationFunction();

  for (size_t k=0; k<machine_weight.size(); ++k) { //for all layers
    if (k == 0) bob::math::prod_(input, machine_weight[k], m_output[k]);
    else bob::math::prod_(m_output[k-1], machine_weight[k], m_output[k]);
    bob::machine::MLP::actfun_t cur_actfun = 
      (k == (machine_weight.size()-1) ? output_actfun : actfun );
    for (int i=0; i<(int)m_batch_size; ++i) { //for every example
      for (int j=0; j<m_output[k].extent(1); ++j) { //for all variables
        m_output[k](i,j) = cur_actfun(m_output[k](i,j) + machine_bias[k](j));
      }
    }
  }
}

void bob::trainer::MLPBaseTrainer::backward_step(const bob::machine::MLP& machine,
  const blitz::Array<double,2>& target)
{
  const std::vector<blitz::Array<double,2> >& machine_weight = machine.getWeights();

  bob::machine::MLP::actfun_t bwdfun = getDerivative(machine.getActivation());
  bob::machine::MLP::actfun_t output_bwdfun = getDerivative(machine.getOutputActivation());

  //last layer
  m_error[m_H] = m_output.back() - target;
  for (int i=0; i<(int)m_batch_size; ++i) { //for every example
    for (int j=0; j<m_error[m_H].extent(1); ++j) { //for all variables
      m_error[m_H](i,j) *= output_bwdfun(m_output[m_H](i,j));
    }
  }

  //all other layers
  for (size_t k=m_H; k>0; --k) {
    bob::math::prod_(m_error[k], machine_weight[k].transpose(1,0), m_error[k-1]);
    for (int i=0; i<(int)m_batch_size; ++i) { //for every example
      for (int j=0; j<m_error[k-1].extent(1); ++j) { //for all variables
        m_error[k-1](i,j) *= bwdfun(m_output[k-1](i,j));
      }
    }
  }
}

void bob::trainer::MLPBaseTrainer::initialize(const bob::machine::MLP& machine)
{
  const std::vector<blitz::Array<double,2> >& machine_weight =
    machine.getWeights();
  const std::vector<blitz::Array<double,1> >& machine_bias =
    machine.getBiases();

  m_H = machine.numOfHiddenLayers();
  m_delta.resize(m_H + 1);
  m_delta_bias.resize(m_H + 1);
  m_output.resize(m_H + 1);
  m_error.resize(m_H + 1);
  for (size_t k=0; k<(m_H + 1); ++k) {
    m_delta[k].reference(blitz::Array<double,2>(machine_weight[k].shape()));
    m_delta_bias[k].reference(blitz::Array<double,1>(machine_bias[k].shape()));
    m_output[k].resize(m_batch_size, m_delta[k].extent(1));
    m_error[k].resize(m_batch_size, m_delta[k].extent(1));
  }

  reset();
}

void bob::trainer::MLPBaseTrainer::setError(const std::vector<blitz::Array<double,2> >& error) {
  bob::core::array::assertSameDimensionLength(error.size(), m_error.size());
  for (size_t k=0; k<error.size(); ++k)
  {
    bob::core::array::assertSameShape(error[k], m_error[k]);
    m_error[k] = error[k];
  }
}

void bob::trainer::MLPBaseTrainer::setError(const blitz::Array<double,2>& error, const size_t id) {
  if (id >= m_error.size())
    throw bob::core::InvalidArgumentException("MLPBaseTrainer: Index in error array",
      (int)id, 0, (int)(m_error.size()-1));
  bob::core::array::assertSameShape(error, m_error[id]);
  m_error[id] = error;
}

void bob::trainer::MLPBaseTrainer::setOutput(const std::vector<blitz::Array<double,2> >& output) {
  bob::core::array::assertSameDimensionLength(output.size(), m_output.size());
  for (size_t k=0; k<output.size(); ++k)
  {
    bob::core::array::assertSameShape(output[k], m_output[k]);
    m_output[k] = output[k];
  }
}

void bob::trainer::MLPBaseTrainer::setOutput(const blitz::Array<double,2>& output, const size_t id) {
  if (id >= m_output.size())
    throw bob::core::InvalidArgumentException("MLPBaseTrainer: Index in output array", 
      (int)id, 0, (int)(m_output.size()-1));
  bob::core::array::assertSameShape(output, m_output[id]);
  m_output[id] = output;
}

bob::machine::MLP::actfun_t 
bob::trainer::MLPBaseTrainer::getDerivative(bob::machine::Activation f)
{
  bob::machine::MLP::actfun_t bwdfun;
  switch (f) {
    case bob::machine::LINEAR:
      bwdfun = bob::machine::linear_derivative;
      break;
    case bob::machine::TANH:
      bwdfun = bob::machine::tanh_derivative;
      break;
    case bob::machine::LOG:
      bwdfun = bob::machine::logistic_derivative;
      break;
    default:
      throw bob::machine::UnsupportedActivation(f);
  }
  return bwdfun;
}

void bob::trainer::MLPBaseTrainer::reset() {
  for (size_t k=0; k<(m_H + 1); ++k) {
    m_delta[k] = 0.;
    m_delta_bias[k] = 0.;
    m_error[k] = 0.;
    m_output[k] = 0.;
  }
}

