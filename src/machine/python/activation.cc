/**
 * @file machine/python/activation.cc
 * @date Thu Jul 7 16:49:35 2011 +0200
 * @author Andre Anjos <andre.anjos@idiap.ch>
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

#include <boost/python.hpp>
#include <bob/machine/Activation.h>

using namespace boost::python;

void bind_machine_activation() {
  enum_<bob::machine::Activation>("Activation")
    .value("LINEAR", bob::machine::LINEAR)
    .value("TANH", bob::machine::TANH)
    .value("LOG", bob::machine::LOG)
    .value("SIGMOID", bob::machine::LOG)
    ;
}
