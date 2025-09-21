#include <iostream>
#include "state.h"

using namespace std;

state::state(int n)
{
	numMiners=n;
	for(int i=0;i<n;i++)
		blocks[i]=0;
}

state::state(int n,int b[10])
{
	numMiners=n;
	for(int i=0;i<n;i++)
		blocks[i]=b[i];
}
 


void state::show()
{
	for(int i=0;i<numMiners;i++)
		cout<<blocks[i]<<" ";
	cout<<endl;
//	cout<<isES()<<endl;
}

bool state::isES()
{
	bool isMark[10]={false};
	int max=0;
	for(int i=0;i<numMiners;i++)
	{
		if(blocks[i]==0 || blocks[i]==1 || blocks[i]==2)
		{
			isMark[i]=true;
			if(blocks[i]>max)
				max=blocks[i];
		}
	}
	bool ctd=false;
	do{
		int current_max=max;
		ctd=false;
		for(int i=0;i<numMiners;i++)
		{
			if(blocks[i]==current_max+1)
			{
				isMark[i]=true;
				ctd=true;
			}
		}
		if(ctd==true)
			max++;
	}while(ctd);
	
	int mark=0;
	for(int i=0;i<numMiners;i++)
	{
		if(isMark[i])
			mark++;
	}
	if(mark==numMiners)
		return true;
	else
		return false;
}

int* state::getState(int b[10])
{
	for(int i=0;i<10;i++)
		b[i]=blocks[i];
	return b;
}
