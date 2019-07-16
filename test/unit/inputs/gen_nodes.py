import h5py
import numpy as np

f0 = h5py.File("nodes_0.h5", "a")
f1 = h5py.File("nodes_1.h5", "a")
f2 = h5py.File("nodes_2.h5", "a")

nodes0 = f0.create_group('nodes')
nodes1 = f1.create_group('nodes')
nodes2 = f2.create_group('nodes')

pop_e = nodes0.create_group('pop_e')
pop_i = nodes1.create_group('pop_i')
pop_ext = nodes2.create_group('pop_ext')

node_group_id = pop_e.create_dataset("node_group_id", (4,), dtype='i')
for i in range(0,4):
    node_group_id[i] = 0

node_group_index = pop_e.create_dataset("node_group_index", (4,), dtype='i')
for i in range(0,4):
    node_group_index[i] = i

node_type_id = pop_e.create_dataset("node_type_id", (4,), dtype='i')
for i in range(0,4):
    node_type_id[i] = 100

g0 = pop_e.create_group("0")
dyn_param = g0.create_group("dynamics_params")

g_pas = dyn_param.create_dataset("pas_0.g_pas", (4,), dtype="f")
e_pas = dyn_param.create_dataset("pas_0.e_pas", (4,), dtype="f")
gl_hh = dyn_param.create_dataset("hh_0.gl_hh", (4,), dtype="f")
el_hh = dyn_param.create_dataset("hh_0.el_hh", (4,), dtype="f")

for i in range(0,4):
    g_pas[i] = 0.001
    e_pas[i] = -65.1
    gl_hh[i] = 0.0003
    el_hh[i] = -54.3

############################################################################

node_group_id = pop_i.create_dataset("node_group_id", (1,), dtype='i')
for i in range(0,1):
    node_group_id[i] = 0

node_group_index = pop_i.create_dataset("node_group_index", (1,), dtype='i')
for i in range(0,1):
    node_group_index[i] = i

node_type_id = pop_i.create_dataset("node_type_id", (1,), dtype='i')
for i in range(0,1):
    node_type_id[i] = 101

g0 = pop_i.create_group("0")
dyn_param = g0.create_group("dynamics_params")

dt = h5py.special_dtype(vlen=bytes)
morph = g0.create_dataset("morphology", (1,), dtype='S100')

for i in range(0,1):
    morph[i] = np.string_("/home/abiakarn/git/arbor-sonata/test/unit/inputs/soma.swc")

############################################################################

node_group_id = pop_ext.create_dataset("node_group_id", (1,), dtype='i')
for i in range(0,1):
    node_group_id[i] = 0

node_group_index = pop_ext.create_dataset("node_group_index", (1,), dtype='i')
for i in range(0,1):
    node_group_index[i] = i

node_type_id = pop_ext.create_dataset("node_type_id", (1,), dtype='i')
for i in range(0,1):
    node_type_id[i] = 200

g0 = pop_ext.create_group("0")

f0.close()
f1.close()
f2.close()
