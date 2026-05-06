### Testing Speed
    make profile
    make gprof > gprof.txt

    
rm -rf generated_files/ Png/ Html/ Jpeg/ Others/ create_files
gcc -pthread create_files.c -o create_files
./create_files

make profile
make gprof > gprof.txt

./bin/file-cleaner ./generated_files/
