make clean 
make 
./copykernel.sh 
bochs -f bochsrc.bxrc > results.txt 
