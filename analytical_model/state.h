#ifndef state_h
#define state_h

class state{
	public:
		state(int);
		state(int,int[10]);
		void show();
		bool isES();
		int* getState(int[10]);	
	private:
		int blocks[10];
		int numMiners;
}; 

#endif
