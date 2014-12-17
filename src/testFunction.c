#include "testFunction.h"
#include "host.h"
#include "clib.h"


int fib_test(int count)
{

 	int result = 1,i = 0;
 	int temp = 0;
 	for (i = 0; i < count; i++){
 		int pre = result;
 		result += temp ;
 		temp = pre;
 	}
 return result;
}
void systemTestLogger(char *message){
 //int handle = 0;
 //host_action(SYS_SYSTEM, "touch output/testLog");
 //handle = host_action(SYS_OPEN, "output/testLog", 4);
 	host_action(SYS_WRITE, testLogfile, (void *)message, strlen(message));
 //host_action(SYS_CLOSE, handle);
}

char *intToCharArray(int number){
 	char numbox[10] = {'0','1','2','3','4','5','6','7','8','9'};
 	char *num = itoa(numbox,number,10);

 return num;

}