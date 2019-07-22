import h5py

f0 = h5py.File("input_spikes.h5", "a")

spikes = f0.create_group("spikes")

gids = spikes.create_dataset('gids', (25,), dtype='i')
for i in range(0,25):
    gids[i] = i//5

times = spikes.create_dataset('timestamps', (25,), dtype='f')
for i in range(0,25):
    times[i] = ((i+4)%5)*20 + (i//5)*4;

g2r = spikes.create_dataset('gid_to_range', (5,2), dtype='i')
for i in range(0,5):
    g2r[i] = i*5, (i+1)*5;

f0.close()
