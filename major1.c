/*
        BUG LIST:

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
enum { READ, WRITE };


int main(int argc, char* argv[])
{
int exitStatus = 0;;
char* commands[50]; //array of full on commands
char* builtins[20];
char raw_input[512];
raw_input[0] = '\0';

char* token = NULL;
char* cdDirTok = NULL;
char* pathTok = NULL;
int cmdCount;
int builtinsCount;
int i;

char cwd[256];

//	executable variables
char *list[50];
int argCount;

// redirection variables
int saveSTDIN = dup(0);
int saveSTDOUT = dup(1);



while(exitStatus == 0 && feof(stdin) == 0)
{
    //  counter variables
    cmdCount = 0;
    builtinsCount = 0;
    argCount = 0;			
	
	if (argc > 1)			 // batch mode
	{
		char line[256];
		
		FILE* fp = fopen(argv[1], "r"); // Open whatever is right after ./a.out
		if (fp == NULL)
		{
			perror("file");
			exit(EXIT_FAILURE);
		}
		else 
		{
			fscanf(fp, "%[^\n]%*c", line); //Its regex that basically translates to a getline. Why we didnt use a getline... well...
			token = strtok(line, ";");

		//	token = strtok(raw_input, ";"); //split commands into args
		    while(token != NULL)
		    {
        		if(strstr(token, "cd") != NULL || strstr(token, "exit") != NULL || strstr(token, "path") != NULL)  //put builtins into a different array of toks than real commands
        		{
        		    builtins[builtinsCount] = token;
        		    printf("current token (built in): %s\n", token);
        		    token = strtok(NULL, ";");
        		    builtinsCount++;
        		}
        		else//get commands into single array
        		{
        		    commands[cmdCount] = token;
        		    token = strtok(NULL, ";");
        		    cmdCount++;
        		}

    		}
    			for(i = 0; i < builtinsCount; i++) //run through the builtins
    			{
        			printf("command : %s\n" , builtins[i]);
        			if(strstr(builtins[i], "exit") != NULL)
        			{
        			    printf("exiting...\n");
        			    exitStatus = 1;
        			}
        			if(strstr(builtins[i], "cd") != NULL)
        			{
        			    cdDirTok = strtok(builtins[i], " ");
        			    cdDirTok = strtok(NULL, " ");
        			    if(chdir(cdDirTok) == 0)
        			    {
        			        printf("changed dir to %s", cdDirTok);
        			        getcwd(cwd, sizeof(cwd));
        			        printf("%s\n", cwd);
        			    }

    	    			}
				if(strstr(builtins[i], "path") != NULL && strstr(builtins[i], "path +") == NULL && strstr(builtins[i], "path -") == NULL)
        			{
        			    printf("%s\n", getenv("PATH")); //print path
        			}
        			if(strstr(builtins[i], "path +") != NULL) //append to path in a really complicated way
        			{
        			    pathTok = strtok(builtins[i], "+");
        			    pathTok = strtok(NULL, "+");
	
        			    if(pathTok != NULL)
        			    {
        			        printf("%s\n", pathTok);
        			        char envarr[1024];
        			        strcat(envarr, getenv("PATH"));
        			        strcat(envarr, ":");
        			        strcat(envarr, pathTok);
        			        setenv("PATH", envarr, 1);
        			    }
        			    printf("%s\n", getenv("PATH"));


        			}
        			if(strstr(builtins[i], "path -") != NULL) //remove a found token from the PATH
        			{
        			    printf("-\n");
        			    pathTok = strtok(builtins[i], "-");
        			    pathTok = strtok(NULL, "-");
        			    if(pathTok != NULL)
        			    {
        			        unsetenv(pathTok);
        			    }
        			}
    			}		

			char* command = NULL;



			if(!feof(stdin)) //make sure we're not eof already
			{
			    for(i = 0; i<cmdCount; i++)
			    {
		
			        argCount = 0;
			        char* cmdToken = NULL;

			        cmdToken = strtok(commands[i], " ");    // split command[i] into command part (first word "cat")
			        command = cmdToken; //store the command here for ease of access

			        while (cmdToken != NULL)
			        {
        			    list[argCount] = cmdToken;  //list[0] will be *command, but execvp() seems to ignore that so w/e
        			    cmdToken = strtok(NULL, " ");               // split new cmdToken into arguments (2nd word. "txt.txt")
        			        argCount++;
        			}

        			list[argCount] = NULL; // NULL terminate the last for execvp

        			int cnt;
        			char first[64], second[64];
        			for(cnt = 0; list[cnt] != NULL; cnt++) //Piping
        			{
                			if(strcmp(list[cnt], "|") == 0)
                			{
                        			list[cnt] = NULL;
                        			strcpy(first, list[cnt-1]);
                        			strcpy(second, list[cnt+1]);
		
        			                int fd[2];
                        			if(pipe(fd) == -1)//error
                        			{
                        			        perror("Pipe");
                        			}
                        			switch(fork()) {
                        			        case -1: perror("Fork");//error
                                			case 0: // child
                                        			dup2(fd[1], fileno(stdout));
                                        			close(fd[0]);
                                        			close(fd[1]);
                                        			int ret = execvp(first, list);//execute first command
                                        			if(ret == -1)
                                        			{
                                                			perror("execvp:");
                                                			exitStatus = 1;
                                        			}
                                			default:        // parent
                                        		if(fork() == 0)
                                        		{
                                                		dup2(fd[0], fileno(stdin));
                                                		close(fd[0]);
                                                		close(fd[1]);
                                                		int ret2 = execvp(second, list);//execute second command
                                                		if(ret2 == -1)
                                                		{
                                                		        perror("execvp:");
                                                        		exitStatus = 1;
                                                		}
                                        		}
                                        		wait(0);
                        			}
                			}
				}

				int r;
    				char tempIn[64];
    				char tempOut[64];
    				int in = 0;
    				int out = 0;
    				int append = 0;


    				pid_t pid2 = fork();
	
			    	if(pid2 == 0)
    				{
        				for(r = 0; list[r] != NULL; r++)
        				{
        	    				if(strcmp(list[r], "<") == 0) //Comparing the commands to < > and >>. If any of them are there, then we go into the if statement that deals with it
            					{
                					list[r] = NULL;
                					strcpy(tempIn, list[r+1]);
                					in = 1;
            					}
	
        					if(strcmp(list[r], ">>") == 0)//append to file
            					{
                					list[r] = NULL;
                					strcpy(tempOut, list[r+1]);
                					append = 1;
            					}
            					else if(strcmp(list[r], ">") == 0)//write to file
            					{
                					list[r] = NULL;
                					strcpy(tempOut, list[r+1]);
                					out = 1;
            					}
  //      				}
				}
	
				if(in)
        			{			
            				int fd0;

            				if((fd0 = open(tempIn, O_RDONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //Open the file as read only so we can use it for our command
            				{
                				perror("input file");
            				}

            				dup2(fd0, STDIN_FILENO); //create a dup to handle it
	
        			    	close(fd0); //close the pathways
        			}

        			if(append)
        			{
            				int fd4;

            				if((fd4 = open(tempOut, O_RDWR | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //Read, write, and append to append to a file
            				{
                				perror("append file");
            				}

            				dup2(fd4, STDOUT_FILENO);

            				close(fd4);
        			}

        			if(out)
        			{
            				int fd3;
            				if((fd3 = open(tempOut, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //We can only write to the file but if the specified file isnt there, then we create it
            				{
                				perror("output file");
            				}

            				dup2(fd3, STDOUT_FILENO);
            				close(fd3);
        			}

				if(command != NULL) //If theres any commands that arent the arrows, then execute them
        			{
            				int ret = execvp(command, list);
            				if(ret == -1)
            				{
                				perror("execvp:");
                				exitStatus = 1; //if execvp failed, exit this child process
            				}
        			}

        			//  restore stdin AND stout
        			dup2(saveSTDIN, 0);
        			dup2(saveSTDOUT, 1);
        		}
        			else if(pid2 > 0)    // parent, assume no error for now
        			{
            				wait(0);
        			}
        			else if (pid2 < 0)
        			{
                			perror("fork");
        			}

            		}

    		}

				fclose(fp);			
		}
		break;
	}

	/*
			INTERACTIVE MODE 
    */
    
    int clear;
    while(printf(">") && scanf("%[^\n]%*c", raw_input) < 1 ) //If you just hit enter at the prompt, REPAIR SELF
    {
        while((clear = getchar()) != '\n' && clear != EOF);
    }
     

    token = strtok(raw_input, ";"); //split commands into args
    while(token != NULL)
    {
        if(strstr(token, "cd") != NULL || strstr(token, "exit") != NULL || strstr(token, "path") != NULL)  //put builtins into a different array of toks than real commands
        {
            builtins[builtinsCount] = token;
            printf("current token (built in): %s\n", token);
            token = strtok(NULL, ";");
            builtinsCount++;
        }
        else
        {
            commands[cmdCount] = token;
            token = strtok(NULL, ";");
            cmdCount++;
        }

    }
    for(i = 0; i < builtinsCount; i++) //run through the builtins
    {
        printf("command : %s\n" , builtins[i]);
        if(strstr(builtins[i], "exit") != NULL)
        {
            printf("exiting...\n");
            exitStatus = 1;
        }
        if(strstr(builtins[i], "cd") != NULL)
        {
            cdDirTok = strtok(builtins[i], " ");
            cdDirTok = strtok(NULL, " ");
            if(chdir(cdDirTok) == 0)
            {
                printf("changed dir to %s", cdDirTok);
                getcwd(cwd, sizeof(cwd));
                printf("%s\n", cwd);
            }

        }
        if(strstr(builtins[i], "path") != NULL && strstr(builtins[i], "path +") == NULL && strstr(builtins[i], "path -") == NULL)
        {
            printf("%s\n", getenv("PATH")); //print path
        }
        if(strstr(builtins[i], "path +") != NULL) //append to path in a really complicated way
        {
            pathTok = strtok(builtins[i], "+");
            pathTok = strtok(NULL, "+");

            if(pathTok != NULL)
            {
                printf("%s\n", pathTok);
                char envarr[1024];
                strcat(envarr, getenv("PATH"));
                strcat(envarr, ":");
                strcat(envarr, pathTok);
                setenv("PATH", envarr, 1);
            }
            printf("%s\n", getenv("PATH"));
            
            
        }
        if(strstr(builtins[i], "path -") != NULL) //remove a found token from the PATH
        {
            printf("-\n");
            pathTok = strtok(builtins[i], "-");
            pathTok = strtok(NULL, "-");
            if(pathTok != NULL)
            {
                unsetenv(pathTok);                    
            }
        }
    }

    /*
		execvp() part
    */

	char* command = NULL;



if(!feof(stdin)) //make sure we're not eof already
{
    for(i = 0; i<cmdCount; i++)
    {

        argCount = 0;
    	char* cmdToken = NULL;
    	
        cmdToken = strtok(commands[i], " ");	// split command[i] into command part (first word "cat")
        command = cmdToken; //store the command here for ease of access
        
    	while (cmdToken != NULL)				
    	{
            list[argCount] = cmdToken;  //list[0] will be *command, but execvp() seems to ignore that so w/e
            cmdToken = strtok(NULL, " ");		// split new cmdToken into arguments (2nd word. "txt.txt")
    		argCount++;
       	}

    	list[argCount] = NULL; // NULL terminate the last for execvp

	int cnt;
	char first[64], second[64];
	for(cnt = 0; list[cnt] != NULL; cnt++) //Piping
	{
		if(strcmp(list[cnt], "|") == 0)
		{
			list[cnt] = NULL;
			strcpy(first, list[cnt-1]);
			strcpy(second, list[cnt+1]);
		
			int fd[2];
	    		if(pipe(fd) == -1)
		    	{
	    			perror("Pipe");
	    		}
	    		switch(fork()) {
	    			case -1: perror("Fork");//error
	    			case 0: // child
	    				dup2(fd[1], fileno(stdout));
	    				close(fd[0]);
	    				close(fd[1]);
		    			int ret = execvp(first, list);
					if(ret == -1)
					{
						perror("execvp:");
						exitStatus = 1;
					}
					printf("after exeec child\n");
    				default:	// parent
					if(fork() == 0)
					{
    						dup2(fd[0], fileno(stdin));
    						close(fd[0]);
    						close(fd[1]);
    						int ret2 = execvp(second, list);
						if(ret2 == -1)
                                        	{
                                        	        perror("execvp:");
                                        	        exitStatus = 1;
                                        	}
					}
					printf("after exeec parent\n");
					wait(0);
    			}
		}
	}
        
        // pipe() before fork()

    int r;
    char tempIn[64];
    char tempOut[64];
    int in = 0;
    int out = 0;
    int append = 0;

    
    pid_t pid2 = fork();

    if(pid2 == 0)
    {
        for(r = 0; list[r] != NULL; r++)
        {
            if(strcmp(list[r], "<") == 0) //Comparing the commands to < > and >>. If any of them are there, then we go into the if statement that deals with it
            {
                list[r] = NULL;
                strcpy(tempIn, list[r+1]);
                in = 1;
            }

            if(strcmp(list[r], ">>") == 0)
            {
                list[r] = NULL;
                strcpy(tempOut, list[r+1]);
                append = 1;
            }
            else if(strcmp(list[r], ">") == 0)
            {
                list[r] = NULL;
                strcpy(tempOut, list[r+1]);
                out = 1;
            }
        }

        if(in)
        {
            int fd0;

            if((fd0 = open(tempIn, O_RDONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //Open the file as read only so we can use it for our command
            {
                perror("input file");
            }

            dup2(fd0, STDIN_FILENO); //create a dup to handle it

            close(fd0); //close the pathways
        }

        if(append)
        {
            int fd4;

            if((fd4 = open(tempOut, O_RDWR | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //Read, write, and append to append to a file
            {
                perror("append file");
            }
            
            dup2(fd4, STDOUT_FILENO);

            close(fd4);
        }

        if(out)
        {
            int fd3;
            if((fd3 = open(tempOut, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0) //We can only write to the file but if the specified file isnt there, then we create it
            {
                perror("output file");
            }

            dup2(fd3, STDOUT_FILENO);
            close(fd3);
        }
 
        if(command != NULL) //If theres any commands that arent the arrows, then execute them
        {
            int ret = execvp(command, list);
            if(ret == -1)
            {
                perror("execvp:");  
                exitStatus = 1; //if execvp failed, exit this child process
            }
        }

        //  restore stdin AND stout
        dup2(saveSTDIN, 0);
        dup2(saveSTDOUT, 1); 
        }
        else if(pid2 > 0)    // parent, assume no error for now
        {
            wait(0);
        }
    	else if (pid2 < 0)
    	{
    		perror("fork");
    	}

            }

    }
        
}
	return 0; 
}


