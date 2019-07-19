import h5py

f0 = h5py.File("edges_0.h5", "a")
f1 = h5py.File("edges_1.h5", "a")
f2 = h5py.File("edges_2.h5", "a")

edges0 = f0.create_group("edges")
edges1 = f1.create_group("edges")
edges2 = f2.create_group("edges")

pop_e_i = edges0.create_group("pop_e_i")
pop_i_e = edges1.create_group("pop_i_e")
pop_e_e = edges2.create_group("pop_e_e")

##################################################################################
nedges = 2
nsrcs = 4
ntgts = 1

edge_group_id = pop_e_i.create_dataset("edge_group_id", (nedges,), dtype="i")
edge_group_id[0] = 0
edge_group_id[1] = 1

edge_group_index = pop_e_i.create_dataset("edge_group_index", (nedges,), dtype="i")
edge_group_index[0] = 0
edge_group_index[1] = 0

edge_type_id = pop_e_i.create_dataset("edge_type_id", (nedges,), dtype="i")
edge_type_id[0] = 103
edge_type_id[1] = 103

source_node_id = pop_e_i.create_dataset("source_node_id", (nedges,), dtype="i")
source_node_id[0] = 0
source_node_id[1] = 2

target_node_id = pop_e_i.create_dataset("target_node_id", (nedges,), dtype="i")
target_node_id[0] = 0
target_node_id[1] = 0


ind = pop_e_i.create_group("indicies")
source_to_target = ind.create_group("source_to_target")

node_id_to_ranges = source_to_target.create_dataset("node_id_to_ranges", (nsrcs,2), dtype="i")
node_id_to_ranges[0] = 0,1
node_id_to_ranges[1] = 1,1
node_id_to_ranges[2] = 1,2
node_id_to_ranges[3] = 2,2

range_to_edge_id = source_to_target.create_dataset("range_to_edge_id", (2,2), dtype="i")
range_to_edge_id[0] = 0,1
range_to_edge_id[1] = 1,2

target_to_source = ind.create_group("target_to_source")

node_id_to_ranges = target_to_source.create_dataset("node_id_to_ranges", (ntgts,2), dtype="i")
node_id_to_ranges[0] = 0,1

range_to_edge_id = target_to_source.create_dataset("range_to_edge_id", (1,2), dtype="i")
range_to_edge_id[0] = 0,2


g0 = pop_e_i.create_group("0")
g1 = pop_e_i.create_group("1")

afferent_id = g0.create_dataset("afferent_section_id", (1,), dtype="i")
afferent_pos = g0.create_dataset("afferent_section_pos", (1,), dtype="f")
efferent_id = g0.create_dataset("efferent_section_id", (1,), dtype="i")
efferent_pos = g0.create_dataset("efferent_section_pos", (1,), dtype="f")

afferent_id[0] = 0
afferent_pos[0] = 0.4
efferent_id[0] = 1
efferent_pos[0] = 0.3

afferent_id = g1.create_dataset("afferent_section_id", (1,), dtype="i")
afferent_pos = g1.create_dataset("afferent_section_pos", (1,), dtype="f")
efferent_id = g1.create_dataset("efferent_section_id", (1,), dtype="i")
efferent_pos = g1.create_dataset("efferent_section_pos", (1,), dtype="f")

afferent_id[0] = 2
afferent_pos[0] = 0.1
efferent_id[0] = 3
efferent_pos[0] = 0.2


#################################################################################
nedges = 1
nsrcs = 1
ntgts = 4

edge_group_id = pop_i_e.create_dataset("edge_group_id", (nedges,), dtype="i")
edge_group_id[0] = 0

edge_group_index = pop_i_e.create_dataset("edge_group_index", (nedges,), dtype="i")
edge_group_index[0] = 0

edge_type_id = pop_i_e.create_dataset("edge_type_id", (nedges,), dtype="i")
edge_type_id[0] = 101

source_node_id = pop_i_e.create_dataset("source_node_id", (nedges,), dtype="i")
source_node_id[0] = 0

target_node_id = pop_i_e.create_dataset("target_node_id", (nedges,), dtype="i")
target_node_id[0] = 1


ind = pop_i_e.create_group("indicies")
source_to_target = ind.create_group("source_to_target")

node_id_to_ranges = source_to_target.create_dataset("node_id_to_ranges", (nsrcs,2), dtype="i")
node_id_to_ranges[0] = 0, 1

range_to_edge_id = source_to_target.create_dataset("range_to_edge_id", (1,2), dtype="i")
range_to_edge_id[0] = 0, 1
         
target_to_source = ind.create_group("target_to_source")

node_id_to_ranges = target_to_source.create_dataset("node_id_to_ranges", (ntgts,2), dtype="i")
node_id_to_ranges[0] = 0, 0
node_id_to_ranges[1] = 0, 1
node_id_to_ranges[2] = 1, 1
node_id_to_ranges[3] = 1, 1

range_to_edge_id = target_to_source.create_dataset("range_to_edge_id", (1,2), dtype="i")
range_to_edge_id[0] = 0, 1


g0 = pop_i_e.create_group("0")

afferent_id = g0.create_dataset("afferent_section_id", (nedges,), dtype="i")
afferent_pos = g0.create_dataset("afferent_section_pos", (nedges,), dtype="f")
efferent_id = g0.create_dataset("efferent_section_id", (nedges,), dtype="i")
efferent_pos = g0.create_dataset("efferent_section_pos", (nedges,), dtype="f")

afferent_id[0] = 0
afferent_pos[0] = 0.5
efferent_id[0] = 0
efferent_pos[0] = 0.9

##################################################################################

nedges = 1
nsrcs = 4
ntgts = 4

edge_group_id = pop_e_e.create_dataset("edge_group_id", (nedges,), dtype="i")
edge_group_id[0] = 0

edge_group_index = pop_e_e.create_dataset("edge_group_index", (nedges,), dtype="i")
edge_group_index[0] = 0

edge_type_id = pop_e_e.create_dataset("edge_type_id", (nedges,), dtype="i")
edge_type_id[0] = 105

source_node_id = pop_e_e.create_dataset("source_node_id", (nedges,), dtype="i")
source_node_id[0] = 1

target_node_id = pop_e_e.create_dataset("target_node_id", (nedges,), dtype="i")
target_node_id[0] = 3

ind = pop_e_e.create_group("indicies")
source_to_target = ind.create_group("source_to_target")

node_id_to_ranges = source_to_target.create_dataset("node_id_to_ranges", (nsrcs,2), dtype="i")
node_id_to_ranges[0] = 0,0
node_id_to_ranges[1] = 0,1
node_id_to_ranges[2] = 1,1
node_id_to_ranges[3] = 1,1

range_to_edge_id = source_to_target.create_dataset("range_to_edge_id", (1,2), dtype="i")
range_to_edge_id[0] = 0,1

target_to_source = ind.create_group("target_to_source")

node_id_to_ranges = target_to_source.create_dataset("node_id_to_ranges", (ntgts,2), dtype="i")
node_id_to_ranges[0] = 0,0
node_id_to_ranges[1] = 0,0
node_id_to_ranges[2] = 0,0
node_id_to_ranges[3] = 0,1

range_to_edge_id = target_to_source.create_dataset("range_to_edge_id", (1,2), dtype="i")
range_to_edge_id[0] = 0,1


g0 = pop_e_e.create_group("0")

afferent_id = g0.create_dataset("afferent_section_id", (nedges,), dtype="i")
afferent_pos = g0.create_dataset("afferent_section_pos", (nedges,), dtype="f")
efferent_id = g0.create_dataset("efferent_section_id", (nedges,), dtype="i")
efferent_pos = g0.create_dataset("efferent_section_pos", (nedges,), dtype="f")

afferent_id[0] = 5
afferent_pos[0] = 0.6
efferent_id[0] = 1
efferent_pos[0] = 0.2

f0.close()
f1.close()
f1.close()
