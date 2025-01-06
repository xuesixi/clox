#! /bin/bash

touch liblox_core liblox_iter liblox_data_structure
xxd -i liblox_core liblox_core.h
xxd -i liblox_iter liblox_iter.h
xxd -i liblox_data_structure liblox_data_structure.h
rm liblox_core liblox_iter liblox_data_structure
