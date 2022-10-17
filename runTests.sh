#!/bin/bash

if [[ $# != 3 ]] ; then
    echo "Usage: $0 <inputdir> <outputdir> <maxthreads>"
    exit 1
fi

MAX_THREADS=${3}
input_dir=$(echo "${1}" | cut -d '/' -f1);
output_dir=$(echo "${2}" | cut -d '/' -f1);

if [ ${MAX_THREADS} -lt 1 ]; then
    echo -e "ERROR: The number of threads must be equal or greater than 1!"
    exit 1
fi

if [ ! -d "${input_dir}" ]; then
    echo -e "ERROR: The directory ${input_dir} does not exist!"
    exit 1
fi

if [ ! -d "${output_dir}" ]; then
    echo -e "ERROR: The directory ${output_dir} does not exist!"
    exit 1    
fi


for test_in in `ls ${input_dir}/*`; do

    input_name=$(echo "${test_in}" | cut -d '/' -f2);
    input_preffix=$(echo ${input_name} | cut -d '.' -f1);

    for i in `seq ${MAX_THREADS}`
    do
    test_out="${output_dir}/${input_preffix}-${i}.txt"
    echo "InputFile=${input_name} NumThreads=${i}"
    bash -c "./tecnicofs ${test_in} ${test_out} ${i}" | tail -1
    rv_student=$?

    if [ ! -f "${test_out}" ]; then
        echo -e "ERROR: The output of the exercise was not created (file ${test_out})!"
        exit 1
    fi

    if [ ${rv_student} != 0 ]; then
        echo -e "ERROR: Program did not return 0!\n"
        bash -c "make clean"
        exit 1
    fi
    done

done
