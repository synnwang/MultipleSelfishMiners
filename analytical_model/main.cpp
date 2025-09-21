#include <iostream>
#include "state.h"
#include <list>
#include <math.h>

//Maximum number of selfish miners=0;
using namespace std;
int miners=3; //selfish miners
double r_s=0.4;
double r_h=1-r_s;
double per_rs=r_s/miners;
list<state> ES;
list<state> confirmES;

bool isRepeat(state);
bool isESRepeat(state);
bool equal(state,state);
double calculateESProbability(state);
double op=0.0;
double total_sp=0;
long long comb(int,int);
double srewards[10]={0};
double hrewards=0;
void calculateRewards(state,double);
void calculateORewards(double);
void normalize();

int main(int argc, char** argv) {

    if (argc >= 2) miners  = atoi(argv[1]);
    if (argc >= 3) r_s   = atof(argv[2]);
    

	r_h=1-r_s;
	per_rs=r_s/miners;
	int CurrentState[10]={0};
	state s(miners,CurrentState);
	ES.push_back(s);
	int x=1;
	cout<<miners<<" "<<r_s<<" ";
	while(!ES.empty())
	{
		state front=ES.front();
		ES.pop_front();	
		
		double sp=calculateESProbability(front);
		total_sp=total_sp+sp;
		calculateRewards(front,sp);
		//if(x%1000==0)
		//{
		//	cout<<x<<" "<<sp<<" ";
		//	front.show();
		//}
		x++;		
		if(1)  //Transit to next state 
		{
			//confirmES.push_back(front);	
			//front.show();
			int cs[10];
			int* ptr=front.getState(cs);
			for(int i=0;i<miners;i++)
			{
				int newState[10];
				for(int j=0;j<miners;j++)
				{
					newState[j]=ptr[j];
				}
				newState[i]++;
				state NewES(miners,newState);
				if(NewES.isES() && !isESRepeat(NewES))
				{
					ES.push_back(NewES);
				}
				else if (!NewES.isES())
				{
					op=op+sp*r_s/miners;	
					//to \Omega state
					
				}
			}
		}
	}
	

/*	while(!confirmES.empty())
	{
		state f=confirmES.front();
		cout<<i<<" ";
		f.show();
		confirmES.pop_front();
		i++;
	}
*/	
	//cout<<x-1<<endl;
//	cout<<"Omega "<<op<<endl;
//	cout<<"SP "<<total_sp<<endl;
	calculateORewards(op);
	normalize();
	return 0;
}

bool isRepeat(state s)
{
	list<state> temp=confirmES;
	while(!temp.empty())
	{
		if(equal(s,temp.front()))
			return true;
		temp.pop_front();
	}
	return false;
}

bool isESRepeat(state s)
{
	list<state> temp=ES;
	while(!temp.empty())
	{
		if(equal(s,temp.front()))
			return true;
		temp.pop_front();
	}
	return false;
}


bool equal(state a,state b)
{
	int ab[10];
	int* pa=a.getState(ab);
	int bb[10];
	int* pb=b.getState(bb);
	
	bool isequal=true;
	for(int i=0;i<10;i++)
	{
		if(ab[i]!=bb[i])
			isequal=false;
	}
	return isequal;
}

double calculateESProbability(state s)
{
	int x[10];
	int* state=s.getState(x);
	int sum=0;

	for(int i=0;i<miners;i++)
	{
		sum=sum+state[i];
	}
	double sp=pow(per_rs,sum);
	for(int i=0;i<miners;i++)
	{
		sp=sp*comb(sum,state[i]);
		sum=sum-state[i];
	}
	if(sp<0)
		sp=0;
	return sp;
	
}



long long comb(int n,int x)
{
	
	int y=x;
	if(n-x<x)
	{
		y=n-x;
	}
	long long result=1;
	for(int i=n;i>n-y;i--)
	{
		result=result*i;
	}
	for(int i=1;i<=y;i++)
	{
		result=result/i;
	}
	//cout<<"comb "<<n<<" "<<x<<" "<<result<<endl;
	return result;
}

void calculateRewards(state s, double sp)
{
	int x[10];
	int* state=s.getState(x);
	int maxPrivate=-1;
	int isCompetitive[10]={0};
	int numComp=0;
	double per_rs=r_s/miners;
			
	for(int i=0;i<miners;i++)  //confirm the 
	{
		if(state[i]>maxPrivate)
		{
			maxPrivate=state[i];
			for(int j=0;j<miners;j++)
			{
				isCompetitive[j]=0;  //Clear
			}
			numComp=0;
			isCompetitive[i]=1;
			numComp++;
		}
		else if(state[i]==maxPrivate)
		{
			isCompetitive[i]=1;
			numComp++;
		}
	} 
	
	//discuss by case
	if(maxPrivate==0)
	{
//		cout<<"R "<<1<<" "; //honest
//		for(int i=0;i<miners;i++)
//			cout<<0<<" "; //selfish
//		cout<<endl;
		hrewards=hrewards+sp*1; // 1 means the honest miner wins from (0,0,...,0)
	}
	else if(maxPrivate>=2 && numComp==1)
	{
//		cout<<"R "<<0<<" ";//honest
		for(int i=0;i<miners;i++)
		{
			if(isCompetitive[i]==1)
			{
				srewards[i]=srewards[i]+sp*maxPrivate;
//				cout<<maxPrivate<<" ";
			}
			else
			{
//				cout<<0<<" ";
			}
		}
//		cout<<endl;
	}
	else if(maxPrivate==1)
	{
		//Competitive situation including honest miner
		double noncomp_rate=(miners-numComp)*per_rs;

		double hr=(r_h/(1+numComp)*2+noncomp_rate*1/(1+numComp)+numComp*r_h/(1+numComp));
//		cout<<"R "<<hr<<" ";
		hrewards=hrewards+hr*sp;
		double compreward=per_rs*2+r_h/(1+numComp)+1*per_rs/(1+numComp)*(miners-numComp);
		double noncompreward=noncomp_rate/(1+numComp)*1+noncomp_rate/(1+numComp)*1*numComp;
		for(int i=0;i<miners;i++)
		{
			if(isCompetitive[i]==1)
			{
				srewards[i]=srewards[i]+compreward*sp;
//				cout<<compreward<<" ";
			}
			else
			{
				srewards[i]=srewards[i]+noncompreward*sp;	
//				cout<<noncompreward<<" ";		
			}
		}
//		cout<<endl;
		
	}
	else
	{

		double noncomp_rate=(miners-numComp)*per_rs;
		double ts=(r_h+noncomp_rate)/(numComp)+per_rs;
	
		double hr=r_h/numComp*numComp;
//		cout<<"R "<<hr<<" ";
		hrewards=hrewards+hr*sp;
			
		double compreward=per_rs*(maxPrivate+1)+r_h/(numComp)*maxPrivate+per_rs/(numComp)*(miners-numComp)*maxPrivate;
		double noncompreward=per_rs;	

		for(int i=0;i<miners;i++)
		{
			if(isCompetitive[i]==1)
			{
				srewards[i]=srewards[i]+compreward*sp;
//				cout<<compreward<<" ";
			}
			else
			{
				srewards[i]=srewards[i]+noncompreward*sp;	
//				cout<<noncompreward<<" ";		
			}	
		}
//		cout<<endl;
	}
	
}

void calculateORewards(double op)
{
	double orewards=((miners+1)/(1-per_rs)+per_rs/(1-per_rs)/(1-per_rs))*1/miners;
//	double orewards=((3)/(1-per_rs)+per_rs/(1-per_rs)/(1-per_rs))*1/miners;
	for(int i=0;i<miners;i++)
	{
		srewards[i]=srewards[i]+orewards*op;
	}
}

void normalize()
{
	double totalRewards=hrewards;
	for(int i=0;i<miners;i++)
	{
		totalRewards=totalRewards+srewards[i];
	}
	cout<<hrewards/totalRewards<<" "<<srewards[0]/totalRewards<<" "<<miners*srewards[0]/totalRewards<<endl;
/*	for(int j=0;j<miners;j++)
	{
		cout<<srewards[j]/totalRewards<<" ";		
	}
	cout<<endl;*/
}
