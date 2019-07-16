import h5py

f0 = h5py.File("spikes_0.h5", "a")
f1 = h5py.File("spikes_1.h5", "a")

spikes0 = f0.create_group("spikes")
spikes1 = f1.create_group("spikes")

gids = spikes0.create_dataset('gids', (20,), dtype='i')
for i in range(0,20):
    gids[i] = i//5

times = spikes0.create_dataset('timestamps', (20,), dtype='f')
for i in range(0,20):
    times[i] = ((i+3)%5)*15 + (i//5)*15;

g2r = spikes0.create_dataset('gid_to_range', (4,2), dtype='i')
for i in range(0,4):
    g2r[i] = i*5, (i+1)*5;

gids = spikes1.create_dataset('gids', (2,), dtype='i')
gids[0] = 0
gids[1] = 0

times = spikes1.create_dataset('timestamps', (2,), dtype='f')
times[0] = 37;
times[1] = 54;

g2r = spikes1.create_dataset('gid_to_range', (1,2), dtype='i')
g2r[0] = 0, 2;

f0.close()
f1.close()
