import h5py

f0 = h5py.File("nodes_0.h5", "a")
f1 = h5py.File("nodes_1.h5", "a")

nodes0 = f0.create_group('nodes')
nodes1 = f1.create_group('nodes')

pop_e = nodes0.create_group('pop_e')
pop_i = nodes1.create_group('pop_i')

node_group_id = pop_e.create_dataset("node_group_id", (400,), dtype='i')
for i in range(0,400):
    node_group_id[i] = 0

node_group_index = pop_e.create_dataset("node_group_index", (400,), dtype='i')
for i in range(0,400):
    node_group_index[i] = i

node_type_id = pop_e.create_dataset("node_type_id", (400,), dtype='i')
for i in range(0,400):
    node_type_id[i] = 100

g0 = pop_e.create_group("0")

g_pas = g0.create_dataset("pas_0.g_pas", (400,), dtype="f")
e_pas = g0.create_dataset("pas_0.e_pas", (400,), dtype="f")
gl_hh = g0.create_dataset("hh_0.gl_hh", (400,), dtype="f")
el_hh = g0.create_dataset("hh_0.el_hh", (400,), dtype="f")

for i in range(0,400):
    g_pas[i] = .001
    e_pas[i] = -65
    gl_hh[i] = 0.0003
    el_hh[i] = -54.3

############################################################################

node_group_id = pop_i.create_dataset("node_group_id", (100,), dtype='i')
for i in range(0,100):
    node_group_id[i] = 0

node_group_index = pop_i.create_dataset("node_group_index", (100,), dtype='i')
for i in range(0,100):
    node_group_index[i] = i

node_type_id = pop_i.create_dataset("node_type_id", (100,), dtype='i')
for i in range(0,100):
    node_type_id[i] = 101

g0 = pop_i.create_group("0")

g_pas = g0.create_dataset("pas_0.g_pas", (400,), dtype="f")
e_pas = g0.create_dataset("pas_0.e_pas", (400,), dtype="f")
gl_hh = g0.create_dataset("hh_0.gl_hh", (400,), dtype="f")
el_hh = g0.create_dataset("hh_0.el_hh", (400,), dtype="f")

for i in range(0,100):
    g_pas[i] = .001
    e_pas[i] = -65
    gl_hh[i] = 0.0003
    el_hh[i] = -54.3

f0.close()
f1.close()
