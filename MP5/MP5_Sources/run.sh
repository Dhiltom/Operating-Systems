make clean 
make 
./copykernel.sh 
bochs -f bochsrc.bxrc > result_terminating.txt 
