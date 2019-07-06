#!/bin/bash

#The script below checks if the results given by "myfind" are identical 
#with those created by the combination of grep and sort system programs
#applied to the result created by the "Verify" program given.

#It requires a few things in order to run properly

#First, it needs execution permissions ($ chmod +x compare.sh)

#Second, it needs 2 arguments:
#the binary input file as its first argument,
#and the search pattern as its second.

#Last, it needs to be in the same directory with your "myfind" executable,
#along with the "Data4Project2" directory, given by the tutor, containing the "Verify" executable

#Note 1: If "myfind" writes additional info to stdout along with the result tuples, 
#	this script will mark the results as non-identical to those created by grep and sort,
#	and will print out the additional info as the difference.
#	This doesn't mean the results are wrong, unless a tuple is printed as difference, 
#	along with your extra info.

#Note 2: If all of the results are printed as difference, 
#	try printing them both in "myfind" and "Verify" in the exact same way. 


#check if arguments are correct
if [[ "$#" != "2" ]]; then
 	#statements
	echo "./compare.sh <file.bin> <pattern>";
	exit -1;
fi

BINARY_FILE=$1
PATTERN=$2

TEST1="testout1.txt"
TEST2="testout2.txt"

#check if binary exists
if [[ ! -f "$BINARY_FILE" ]]; then
	echo "Binary file \"$BINARY_FILE\" doesn't exist.";
	exit -1;
fi

#Verify binary file, find the tuples containing the pattern, and sort them
#save the result to TEST2 ASCII file
./Data4Project2/Verify "$BINARY_FILE" | grep "$PATTERN" | sort > "$TEST2";

echo "Executing without \"-s\".."
#for each process hierarchy Height
for (( i = 1; i <= 5; i++ )); do
	echo " Comparing with Height = $i...";

	#run myfind with given patterns and save the output to the ASCII file TEST1
	./myfind -h "$i" -d "$BINARY_FILE" -p "$PATTERN" > "$TEST1";


	#compare TEST1 and TEST2 files
	if [[ -z $(diff $TEST1 $TEST2) ]]; then
		echo " Identical";
	else
		echo " Non-identical";
		diff "$TEST1" "$TEST2";
	fi
done

#do the same as above but in skew mode (-s)
echo "Executing with \"-s\".."
for (( i = 1; i <= 5; i++ )); do
	echo " Comparing with Height = $i...";
	./myfind -h "$i" -d "$BINARY_FILE" -p "$PATTERN" -s > "$TEST1";

	if [[ -z $(diff $TEST1 $TEST2) ]]; then
		echo " Identical";
	else
		echo " Non-identical";
		diff "$TEST1" "$TEST2";
	fi
done

rm -f "$TEST1" "$TEST2"
