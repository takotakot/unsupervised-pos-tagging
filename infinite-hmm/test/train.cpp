#include  <iostream>
#include  <string>
#include "../src/ihmm/utils.h"
#include "../src/ihmm/ihmm.h"
using namespace ihmm;

int main(){
	InfiniteHMM* ihmm = new InfiniteHMM(10, 100);
	return 0;
}