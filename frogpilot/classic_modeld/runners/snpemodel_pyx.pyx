# distutils: language = c++
# cython: c_string_encoding=ascii

import os
from libcpp cimport bool
from libcpp.string cimport string

from .snpemodel cimport SNPEModel as cppSNPEModel
from frogpilot.classic_modeld.models.commonmodel_pyx cimport CLContext
from frogpilot.classic_modeld.runners.runmodel_pyx cimport RunModel
from frogpilot.classic_modeld.runners.runmodel cimport RunModel as cppRunModel

os.environ['ADSP_LIBRARY_PATH'] = "/data/pythonpath/third_party/snpe/dsp/"

cdef class SNPEModel(RunModel):
  def __cinit__(self, string path, float[:] output, int runtime, bool use_tf8, CLContext context):
    self.model = <cppRunModel *> new cppSNPEModel(path, &output[0], len(output), runtime, use_tf8, context.context)
