import h5py

f0 = h5py.File("input_spikes.h5", "a")

spikes = f0.create_group("spikes")

gids = spikes.create_dataset('gids', (1,), dtype='i')
for i in range(0,1):
    gids[i] = 1

times = spikes.create_dataset('timestamps', (1,), dtype='f')
for i in range(0,1):
    times[i] = 0;

g2r = spikes.create_dataset('gid_to_range', (1,2), dtype='i')
for i in range(0,1):
    g2r[i] = 0,1;

f0.close()
