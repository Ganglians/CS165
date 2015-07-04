#include <stdio.h>
#include <iostream>
#include <fstream>//file io
#include <stdlib.h>//exit
#include <gmpxx.h>

using namespace std;

//g++ main.cpp -lgmpxx -lgmp

#define posLong unsigned long long//use when number is really big
#define veryLong long long//to make code shorter

//open the file and get the values
void getVals(char *fileName, veryLong s[], int arrSize)
{
	ifstream Fin(fileName);
	
	if (Fin.is_open())
	{
		veryLong tmp;
		Fin >> tmp;//exclude the first character

		for(int i = 0; i < arrSize; i ++)
		{
			Fin >> s[i];
		}
			
		Fin.close();
	}	
	
	else
	{
		cout << "unable to open file.\n";
		exit(-1);
	}
	
	return;	
}

/*filter out the word 'Project ' from the first eight
characters of the file */
void filter(veryLong s[], int arrs_Size, char f[], int arrf_Size)
{
	
	if(arrs_Size != arrf_Size)
	{
		cout << "the cyphertext length is longer than the crib phrase.\n";
		exit(-1);
	}
	
	for(int i = 0; i < arrs_Size; i++)
	{
		s[i] = s[i] - f[i];
	}
	
	return;
}

//similar to a determinant method
veryLong calcDet(veryLong val1, veryLong val2, veryLong val3, veryLong val4)
{	
	return  abs((val3-val2)*(val3 - val2) - (val4 - val3)*(val2-val1));
}

//gcd of two numbers a and b
veryLong GCD (veryLong a, veryLong b)
{
  veryLong tmp;
  while ( a != 0 ) 
  {
     tmp = a; 
     a = b%a;  
     b = tmp;
  }
  
  return b;
}

//take the gcd of all the calculated multiples of m
veryLong calcMod(veryLong detlist[], int det_size)
{
	veryLong GCDtemp[det_size];
	
	for(int i = 0;i<det_size;i++)
	{
		GCDtemp[i] = detlist[i];
	}
	
	for(int k = 1;k<det_size;k++)
	{
		for(int i = 0;i<det_size-k;i++)
		{
			GCDtemp[i] = GCD(GCDtemp[i],GCDtemp[i+1]);
		}
	}
	
	return GCDtemp[0];
}

veryLong calcSeed(posLong a, posLong b, posLong m, posLong val, int nth_val)
{
	posLong sol;
	sol = val%m;
	
	for(int i = nth_val-1; i >= 0; --i)
	{
		sol -= b%m;
		sol /= a%m;
	}
	
	return sol;
}

//translate the file
void Translate(veryLong a, veryLong b, veryLong m, veryLong x0, char *inFile, char *outFile)
{
	ifstream Fin(inFile);
	ofstream Fout(outFile);
	
	if (Fin.is_open() && Fout.is_open())
	{
		posLong tmp;
		posLong x_p = x0;
		posLong x_n;
		posLong ot;
		
		Fin >> tmp;//exclude the first character
		Fin >> tmp;
		
		while(!Fin.eof())
		{
			Fin >> tmp;
		
			x_n = (a*x_p + b) %m;
			x_p = x_n;
			
			ot = tmp - x_n;
			Fout << (char) ot;
		}
		
		Fout.close();	
		Fin.close();
	}	
	
	else
	{
		cout << "unable to open file.\n";
		exit(-1);
	}	
}

int main()
{
	veryLong m, a, b, s0;
	
	//where the values from the book are stored
	int vals = 8;
	veryLong s[vals];
	char c[] = "book1.txt";
	
	//where the solutions equivalent to 0modm are stored
	int det_size = vals - 3;
	veryLong detlist[det_size];	
	
	getVals(c, s, vals);
	
	//check values came from file properly
	/*for(int i = 0; i < vals; i ++)
		cout << s[i] << endl;*/
	
	char f[] = "Project ";
	filter(s, vals, f, vals);
	
	//check values were filtered properly
	/*for(int i = 0; i < vals; i ++)
		cout << s[i] << endl;*/
	
	//collect solutions equivalent to 0modm	
	for(int i = 0;i<det_size;i++)
	{
		detlist[i] = calcDet(s[i],s[i+1],s[i+2],s[i+3]);
		cout << detlist[i] << " ";		
	}		
	cout << endl;
	
	/*after taking the GCD of 5 different cases congruent to 0modm
	 * the value has a high chance of being the real m*/
	 
	m = calcMod(detlist, det_size);
	cout << "their GCD's (possibly m): " << m << endl;
	
	//solve for a exhaustively
	
	/*veryLong k2 = abs((s[1] - s[0])*(s[2] - s[1]));
	veryLong k1 = abs((s[2] - s[1])*(s[2] - s[1]));
	(for(veryLong i = 0; i < m; i++)
	{
		//cout << i << endl;
		if(((k1%m) == ((i*k2)%m)))
		{
			cout << i << endl << endl << endl;
			a = i;
			break;
		}
	}*/
	
	a = 4276115653;//hardcoded since took a while to solve for
	
	//solving for b is trivial
	b = ((abs(s[1] - a*s[0]))%m);	
	
	s0 = calcSeed(a,b,m,s[vals-1],vals);
	
	cout << "The LCG function is : x[n] = ( " << a;
	cout << "*x[n-1] + " << b << " ) mod(" << m << ")" << endl;
	cout << "Seed: " << s0;		

	char c2[] = "decrypt.txt";
	Translate(a, b, m, s[0], c, c2);

	return 0;
}
