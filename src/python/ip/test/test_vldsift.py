#!/usr/bin/env python
# vim: set fileencoding=utf-8 :
# Laurent El Shafey <Laurent.El-Shafey.ch>
# Mon  23 Jan 2012

"""Tests our Dense SIFT features extractor based on VLFeat
"""

import os, sys
import unittest
import bob
import numpy

def load_image(relative_filename):
  # Please note our PNG loader will always load in RGB, but since that is a
  # grayscaled version of the image, I just select one of the planes. 
  filename = os.path.join("data", "sift", relative_filename)
  array = bob.io.load(filename)
  return array.astype('float32')

def equal(x, y, epsilon):
  return (abs(x - y) < epsilon)

def equals(x, y, epsilon):
  return (abs(x - y) < epsilon).all()

class VLDSiftTest(unittest.TestCase):
  """Performs various tests"""

  def test01_VLDSiftPython(self):
    # Dense SIFT reference using VLFeat 0.9.13 
    # (First 3 descriptors, Gaussian window)
    filename = os.path.join("data", "sift", "vldsift_gref.hdf5")
    ref_vl = bob.io.load(filename)

    # Computes dense SIFT feature using VLFeat binding
    img = load_image('vlimg_ref.pgm')
    mydsift1 = bob.ip.VLDSIFT(img.shape[0],img.shape[1])
    out_vl = mydsift1(img)
    # Compare to reference (first 200 descriptors)
    for i in range(200):
      self.assertTrue(equals(out_vl[i,:], ref_vl[i,:], 2e-6))
      
    
if __name__ == '__main__':
  sys.argv.append('-v')
  if os.environ.has_key('BOB_PROFILE') and \
      os.environ['BOB_PROFILE'] and \
      hasattr(torch.core, 'ProfilerStart'):
    torch.core.ProfilerStart(os.environ['BOB_PROFILE'])
  os.chdir(os.path.realpath(os.path.dirname(sys.argv[0])))
  unittest.main()
  if os.environ.has_key('BOB_PROFILE') and \
      os.environ['BOB_PROFILE'] and \
      hasattr(torch.core, 'ProfilerStop'):
    torch.core.ProfilerStop()