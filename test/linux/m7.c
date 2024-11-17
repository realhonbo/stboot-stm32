#include <stdio.h>
#include <unistd.h>

int main()
{
	int i;
	for( i = 0; i < 1024; i++ ) {
		printf("Hello Linux in STM32H7\n");
		printf("\n");
		sleep(1);
	}
}
