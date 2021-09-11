#include "common.h"

int str2int(char* str){
	int ans = 0; 
	for(int i = 0 ; str[i] ; i++){
		ans *= 10;
		if (str[i] > '9' || str[i] < '0') return -1;  
		ans += str[i]-'0';
	}
	return ans; 
}

int getFactorial(int x){
	int ans = 1; 
	for (int i = 1 ; i <= x ; i++){
		ans *= i;
	}
	return ans;
}

int main(int argc, char** argv)
{
	if(argc < 2){
		printf("Huh?\n");
	} else {
		int numInput = str2int(argv[1]);
		// printf("input: %d\n",numInput);
		if(numInput < 1){
			printf("Huh?\n");
		}else if(numInput > 12){
			printf("Overflow\n");
		}else{
			printf("%d\n",getFactorial(numInput));
		}
	}
	return 0;
}
