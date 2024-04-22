#!/bin/bash
#SBATCH --time=00:05:00
#SBATCH --account=mcs

bash -c "mpic++ -fopenmp -std=c++11 encode_mpi_openmp.cpp -o encode_mpi_openmp" &>/dev/null
bash -c "g++ -std=c++11 decode_serial.cpp -o decode_serial" &>/dev/null

echo -e ">>>> MPI+OpenMP Algorithm Time Analysis >>>>"

N=10
M=$((5 * 2 * 2 * 2 * 2 * 2))
file1=text_sample.txt
file2=input.txt

for ((i = 1; i <= 32 * 2 * 2 * 2 * 2 * 2 * 2; i += 1)); do
	cat $file1 >>$file2
done

for ((i = 0; i <= 10; i += 1)); do
	for ((j = 1; j <= M; j += 1)); do
		cat $file1 >>$file2
	done
done

for ((i = 1; i <= N; i += 1)); do
	for ((j = 1; j <= M; j += 1)); do
		cat $file1 >>$file2
	done

	numChars=$(wc -c "./${file2}")
	t1=$(date +%s)
	bash -c "mpirun -np 40 ./encode_mpi_openmp ./${file2} ./huffman_tree.txt ./output.bin" &>/dev/null
	t2=$(date +%s)
	secs=$((t2 - t1))
	echo -e "N = ${numChars} -> Encode seconds run = ${secs}"

	numBytes=$(wc -c ./output.bin)
	t1=$(date +%s)
	./decode_serial ./output.bin ./huffman_tree.txt plain.txt
	t2=$(date +%s)
	secs=$((t2 - t1))
	echo -e "N = ${numBytes} -> Decode seconds run = ${secs}"
	echo -e "--"
done
