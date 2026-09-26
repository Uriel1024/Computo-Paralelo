#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<wait.h>
#include<sys/types.h>

int main()
{
   pid_t pid1, pid2, pid3;
   pid1 = fork();
   
   if(pid1==-1)
   {
      perror("\nError al crear el proceso\n");
      exit(-1);
   }
   
   if(pid1==0)
   {
      printf("\nHola soy el proceso hijo\n");
      printf("\nMi identificador es: %d\n",getpid());
      printf("\nMi proceso padre es: %d\n",getppid());
      sleep(15);
      exit(0);
   }
   pid2 = fork();
   if(pid2==-1)
   {
      perror("\nError al crear el proceso\n");
      exit(-1);
   }
   
   if(pid2==0)
   {
      printf("\nHola soy el proceso hijo\n");
      printf("\nMi identificador es: %d\n",getpid());
      printf("\nMi proceso padre es: %d\n",getppid());
      sleep(15);
      exit(0);
   }
   pid3 = fork();
   if(pid3==-1)
   {
      perror("\nError al crear el proceso\n");
      exit(-1);
   }
   if(pid3==0)
   {
      printf("\nHola soy el proceso hijo\n");
      printf("\nMi identificador es: %d\n",getpid());
      printf("\nMi proceso padre es: %d\n",getppid());
      sleep(15);
      exit(0);
   }
   sleep(30);
   return 0;
}