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


node_group_id = pop_i.create_dataset("node_group_id", (100,), dtype='i')
for i in range(0,100):
    node_group_id[i] = 0

node_group_index = pop_i.create_dataset("node_group_index", (100,), dtype='i')
for i in range(0,100):
    node_group_index[i] = i

node_type_id = pop_i.create_dataset("node_type_id", (100,), dtype='i')
for i in range(0,100):
    node_type_id[i] = 101

f0.close()
f1.close()
