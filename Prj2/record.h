/* record.h */

#ifndef RECORD_H
#define RECORD_H

#define SIZEofBUFF 20
#define SSizeofBUFF 6
#define RECSIZE 90


typedef struct{
	long custid;
	char FirstName[SIZEofBUFF];
	char LastName[SIZEofBUFF];
	char Street[SIZEofBUFF];
	int  HouseID;
	char City[SIZEofBUFF];
	char postcode[SSizeofBUFF];
	float amount;
} MyRecord;

#endif
