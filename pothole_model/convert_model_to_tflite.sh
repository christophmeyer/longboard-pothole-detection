#!/bin/bash

python -m model.convert_to_tflite
xxd -i ./models/pothole/pothole.tflite > ./models/pothole/pothole_model.cc
sed -i 's/__models_pothole_pothole_tflite/pothole_model/g' ./models/pothole/pothole_model.cc
sed -i 's/unsigned/const unsigned/g' ./models/pothole/pothole_model.cc
sed -i '1s/^/#include "pothole_model.h"\n/' ./models/pothole/pothole_model.cc