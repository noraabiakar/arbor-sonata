# SONATA for Arbor

### Build the example and unit tests
#### Clone and build the arbor library
```
$ ./install-local.sh
```
#### Generate unit test input (hdf5 files)
```
$ cd test/unit/inputs
$ python generate/gen_edges.py
$ python generate/gen_nodes.py
$ python generate/gen_spikes.py
```

#### Generate example input (hdf5 files)
```
$ cd example/network
$ python generate/gen_edges.py
$ python generate/gen_nodes.py
$ cd ../inputs
$ python generate/gen_spikes.py
```

#### Build the sonata library, unit tests and example
```
$ mkdir build && cd build
$ cmake .. -Darbor_DIR=/path/to/arbor-sonata/install/lib/cmake/arbor -DCMAKE_BUILD_TYPE=release
$ make
```

#### Run the unit tests
```
$ cd build
$ ./bin/unit
```

#### Run the example
```
$ cd build
$ ./bin/sonata-example ../example/simulation_config.json
```
#####Expected outcome of running the example:
* **2805** spikes generated
* Output spikes report: `output_spikes.h5`
* Voltage and current probe reports: `voltage_report.h5` and `current_report.h5`