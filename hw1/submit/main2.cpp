#include <stdio.h>
#include <gmpxx.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
using namespace std;
typedef vector<char> Set;
const int charCount = 163999; //number of characters in the file

//see if the character has already appeared in the set f
bool examineF(Set &f, char c)
{
	for(int i = 0; i < f.size(); i ++)
	{
		if(f[i] == c)
		{
			return true;
		}
	}
	
	return false;
}

//prints the set of all characters appearing in the file
void printF(Set &f)
{
	for(int i = 0; i < f.size(); i++)
	{
		cout << f[i] << " ";
	}
	
	cout << "\nnumber of individual chars: " << f.size() << endl;
}

//obtain every type of character in the cyphertext
void chara(char *filename, Set &f)
{
	ifstream Fin(filename);
	 
	char c;
	int n = 0;
	if (Fin.is_open())
	{
		while(Fin.get(c))
		{
			n++;
			if(examineF(f, c))
			{
				continue;
			}
			else if(c != '\n') //maybe also c!= ''
			{
				f.push_back(c);
			}
		}
	}
	
	else
	{
		cout << "unable to open file.\n";
		exit(-1);
	}
	
	Fin.close();
}

//initialize the array to 0's
void initArr(int fArr[], int s)
{
	for(int i = 0; i < s; i++)
	{
		fArr[i] = 0;
	}
}

//count the number of times each individual character appears
void frequency(char *filename, Set &f, int fArr[])
{
	ifstream Fin(filename);
	 
	char c;
	int fSize = f.size();
	if (Fin.is_open())
	{
		while(Fin.get(c))
		{
			for(int i = 0; i < fSize; i++)
			{
				if (f[i] == c)
				{
					++fArr[i];
				}
			}
		}
	}
	
	else
	{
		cout << "unable to open file.\n";
		exit(-1);
	}
	
	Fin.close();	
}

int printFreq(Set &f, int fArr[])
{
	int tot = 0;
	
	int fSize = f.size();
	for(int i = 0; i < fSize; i++)
	{
		cout << f[i] << ": " << fArr[i] << endl;
		tot = tot + fArr[i];
	}
	//skip i = 30?
	cout << "total number of chars: " << tot << endl;
	return tot;
}

void printRatio(Set &f, int fArr[], int tot)
{
	int fSize = f.size();
	float ratio;
	for(int i = 0; i < fSize; i++)
	{
		ratio = float(fArr[i] / tot);
		cout << f[i] << ": " << ratio << endl;
	}	
}

void vectInit(vector<int> &sf, int fArr[])
{
	int s = sf.size();
	for(int i = 0; i < s; i++)
	{
		sf[i] = fArr[i];
	}
}

int main()
{
	char c[] = "book2.txt";
	Set f;
	
	chara(c, f);
	printF(f);
	
	int *fArr = new int[f.size()];
	vector<int> sf(f.size());
	initArr(fArr, f.size());
	frequency(c, f, fArr);
	
	int tot = printFreq(f, fArr);
	cout << "tot: " << tot << endl;
	printRatio(f, fArr, tot);
	/*vectInit(sf, fArr);
	sort(sf.begin(), sf.end());
	
	cout << endl;
	cout << "Sorted: \n";*/
	
	return 0;
}
