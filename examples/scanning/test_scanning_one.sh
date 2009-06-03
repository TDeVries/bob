#!/bin/sh

image="/idiap/common_vision/visidiap/databases/cmu-test/images_gray/addams-family.pgm"
model="../data/models/facedetection/frontal/mct4.cascade"

params_prune=""	#-prune_use_mean -prune_use_stdev"
params_select="-select_type 1 -select_merge_type 1 -select_min_surf_overlap 60" #-select_overlap_iterative
params_general="-dx 0.2 -dy 0.2 -ds 1.5 -min_patt_w 0 -max_patt_w 10000 -min_patt_h 0 -max_patt_h 10000"
params=$params_prune" "$params_select" "$params_general" -verbose"

# Multiscale
valgrind --leak-check=full ./`uname -s`_`uname -m`/scanning $image $model -explorer_type 1 -scale_explorer_type 0 $params
#gdb --args

# Pyramid
#./`uname -s`_`uname -m`/scanning $image $model -explorer_type 0 -scale_explorer_type 0 $params
