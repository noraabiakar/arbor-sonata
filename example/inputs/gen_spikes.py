import h5py

f0 = h5py.File("input_spikes.h5", "a")

spikes = f0.create_group('spikes')

gids = spikes.create_dataset('gids', (10,), dtype='i')
for i in range(0,10):
    gids[i] = 1

times = spikes.create_dataset('timestamps', (10,), dtype='f')
for i in range(0,10):
    times[i] = i + 0.1

f0.close()
