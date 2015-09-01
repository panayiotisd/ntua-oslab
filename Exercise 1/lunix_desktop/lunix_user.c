#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	FILE *fp = fopen(argv[1],"r");
	float x;
	fork();
	pid_t mypid = getpid();
	while(1)
	{
		fscanf(fp,"%f",&x);
		printf("(%ld) %.3f\n",(long)mypid,x);
	}
	return 0;
}
