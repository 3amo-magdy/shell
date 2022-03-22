#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

const int Max_words = 16;
const int Max_chars = 12;
const int Max_assignments = 32;
int ArgumentsNumber;
char* PreviousDirectory;
enum inputType{shell_builtin,executable_or_error};
enum commandType{cd, echo, export, undefined, end};

pid_t child;
char* CurrentWorkingDirectory;
char** assignments;
int assingments_count;

char** parse_input(char* CommandLine,int*a){
    *a = 0;
    int b=0;
    int n=0;

    int mode = 0;
    int quotes=0;
    char** CommandArray = (char**)malloc(Max_words*sizeof(char*));
    for (int i = 0; i < Max_words; i++)
    {
        CommandArray[i] = (char*) malloc(Max_chars*sizeof(char));
    }

    for(int i=0;i<strlen(CommandLine);i++){
        if(CommandLine[i]==' '&&mode==0){
            mode = 1;
        }
        else if(CommandLine[i]!=' '&&mode==1){
            b=0;
            mode = 0;
            *a=*a+1;
            // CommandArray = realloc(CommandArray,((*a)+1)*sizeof(char*));

            if((!strcmp(CommandArray[0],"cd"))||
            (!strcmp(CommandArray[0],"echo")&&(CommandLine[i]=='"'||CommandLine[i]=='\''))){
               CommandArray[*a]=(CommandLine+i);
               return CommandArray;
            }
        }
        if(mode==0){
            CommandArray[*a][b]=CommandLine[i];
            n++;
            b++;
            CommandArray[*a]=realloc(CommandArray[*a],(b)*sizeof(char));
        }
        else{
            //skip space characters
        }
    }
    *a=*a+1;
    ArgumentsNumber=*a;
    return CommandArray;
}


// main ()
// {
// 		signal (SIGCHLD, proc_exit);
// 		switch (fork()) {
// 			case -1:
// 				perror ("main: fork");
// 				exit (0);
// 			case 0:
// 				printf ("I'm alive (temporarily)\n");
// 				exit (rand());
// 			default:
// 				pause();
// 		}
// }

void reap_child_zombie(){
    waitpid(-1,NULL,0);
}
void write_to_log_file(char* sentence){
    // to be implemented
}

void on_child_exit(){
    // a child has terminated and it's "waitable":
    reap_child_zombie();
    write_to_log_file("Child terminated");
}



void setup_environment(){
    CurrentWorkingDirectory = (char*) malloc(Max_words*sizeof(char));
    PreviousDirectory = (char*) malloc(Max_words*sizeof(char));
    PreviousDirectory[0]='!';
    assingments_count=0;
    getcwd(CurrentWorkingDirectory,sizeof(CurrentWorkingDirectory));
    assignments=(char**)malloc(2*Max_assignments*sizeof(char*));
    for(int n=0;n<2*Max_assignments;n++){
        assignments[n]=(char*)malloc(Max_chars);
    }
}
void read_input(char* input){
    fflush(stdin);
    input = fgets(input,Max_words*Max_chars,stdin);
    return;
}

void* import(char*var){
    for (int i = 0; i <assingments_count ; i+=2){
        printf("%s",var)
        if(!strcmp(var,assignments[i])){
            return assignments[i+1];
        }
        else{
            printf("%s",assignments[i]);
        }
    }
    return NULL;
}
enum inputType get_input_type(char* command){
    if(strcasecmp(command,"echo")||strcasecmp(command,"export")||strcasecmp(command,"cd")){
        return shell_builtin;
    }
    return executable_or_error;
}
enum commandType determine_built_in_command(char* command){
    if(!strcmp(command,"echo")){
        return echo;
    }
    else if(!strcmp(command,"export")){
        return export;
    }
    else if(!strcmp(command,"cd")){
        return cd;
    }
    else if(!strcmp(command,"exit")){
        return end;
    }
    return undefined;
}
int command_is_not_exit(char* command){
    if(!strcmp(command,"exit")){
        return 0;
    }
    return 1;
}

void evaluate_expression(char* exp,char* newExpression){
    char* value;
    int var_counter=0;
    int newSize=0;
    for (int i = 0; i < strlen(exp); i++)
    {
        printf(newExpression);
        printf("\n");
        if(exp[i]=='$'){
            i++;
            while((i+var_counter)<strlen(exp)&&((exp[i+var_counter]>64&&exp[i+var_counter]<91)||(exp[i+var_counter]>96&&exp[i+var_counter]<123)||(exp[i+var_counter]>47&&exp[i+var_counter]<58))){
                var_counter++;
            }
            char* var =malloc(var_counter*sizeof(char));
            strncpy(var,exp+i,var_counter);
            printf("var : %s",var);
            i+=var_counter;
            var_counter=0;
            value = import(var);
            if(value == NULL){
                value = "";
            }
            free(var);
            for (int t = 0; t < strlen(value); t++)
            {
                newExpression[newSize]=value[t];
                newSize++;
            }
        }
        else{
            newExpression[newSize]=exp[i];
            newSize++;
        }
    }
    newExpression = (char*)realloc(newExpression,sizeof(char)*(newSize));
    return ;
}

void changeDirectory(char* arguments_string){
    if(strlen(arguments_string)==0){
        PreviousDirectory = CurrentWorkingDirectory;
        CurrentWorkingDirectory = getenv("HOME");
        chdir(CurrentWorkingDirectory);
    }
    
    int n=0;
    char** arguments = parse_input(arguments_string,&n);
    if(n==1){
        if(strcmp(arguments[0],"-")==0){
            if(PreviousDirectory[0]=='!'){
                //do nothing
            }
            else{
                chdir(PreviousDirectory);
                char* temp = PreviousDirectory;
                PreviousDirectory = CurrentWorkingDirectory;
                CurrentWorkingDirectory = temp;
            }
        }
        else if(strcmp(arguments[0],"~")==0){
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            strcpy(CurrentWorkingDirectory , getenv("HOME"));
            chdir(CurrentWorkingDirectory);        }
        else if(strcmp(arguments[0],"..")==0||strcmp(arguments[0],"../")==0){
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            chdir("..");
            getcwd(CurrentWorkingDirectory,sizeof(CurrentWorkingDirectory));
        }
        else {
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            chdir(arguments[0]);
            getcwd(CurrentWorkingDirectory,sizeof(CurrentWorkingDirectory));
        }
    }




}

void assign(char* var,char* value){
    for (int i = 0; i <assingments_count ; i+=2){
        if(!strcmp(var,assignments[i])){
            assignments[i+1]=realloc(assignments[i+1],strlen(value)*sizeof(char));
            assignments[i+1]=value;
            return;
        }
    }
    assingments_count+=2;

    assignments=realloc(assignments,(assingments_count)*sizeof(char*));
    printf("%i",assingments_count);
    printf(" here  ");

    assignments[assingments_count-1]=var;
    assignments[assingments_count]=value;
}
int ExportArg(char* export_argument){
    char* variable = malloc(Max_chars*sizeof(char));
    char* value = malloc(Max_chars*sizeof(char));
    int var_c=0,val_c=0,i = 0;
    for (; i < strlen(export_argument); i++){
        if(export_argument[i]==' '||export_argument[i]=='$'){
            return -1;
        }
        if(export_argument[i]=='='){
            i++;
            break;
        }
        variable[var_c]=export_argument[i];
        var_c++;
    }
    variable=realloc(variable,var_c*sizeof(char));
    for(;i<strlen(export_argument)&&export_argument[i]!='\n';i++){
        value[val_c]=export_argument[i];
        val_c++;
    }
    variable=realloc(variable,val_c*sizeof(char));

    assign(variable,value);
}
void Export(char** CommandArray,int count){
    for (int i = 1; i < count; i++)
    {
        int res = ExportArg(CommandArray[i]);
        if(res == -1){
            printf("%s is not a valid exportation\n",CommandArray[i]);
        }
    }

}
int execute_shell_builtin(char** CommandArray){
    char* command = CommandArray[0];
    enum commandType command_type = determine_built_in_command(command);
    switch(command_type){
        case cd:
            changeDirectory(CommandArray[1]);
            break;
        case echo:
            for(int c=0;c<strlen(CommandArray[1]);c++){
                if(CommandArray[1][c] != '"'){
                    printf("%c",CommandArray[1][c]);
                }
            }
            break;
        case export:
            printf("exporting");
            Export(CommandArray,ArgumentsNumber);
            break;
        case undefined:
            return -1;
        case end:
            return -2;
    }
    return 0;
}

int execute_command(char** command){
    pid_t id = fork();
    //child process:
    if(id!=0&&id!=-1){
        child = id;
        execvp(command[0],command+1);
        printf("Error processing the command, make sure the command is correct\n");
        return(-1);
    }
    //parent process:
    else if (id==0&&strcmp(command[ArgumentsNumber-1],"&")){
        //foreground:
        //wait on the child
        //we can use first parameter as -1 cuz we only have one child at a time
        waitpid(-1,NULL,0);
    }
    else if(id ==0&&!strcmp(command[ArgumentsNumber-1],"&")){
        //background:
        //don't wait on the child .. do nothing
    }
}

void shell(){

    char* Input = malloc(Max_chars*Max_words*sizeof(char));
    char** CommandArray;
    int a=0;
    enum inputType input_type=shell_builtin;
    int status=0;
    do{
        read_input(Input);
        printf(Input);
        a=0;
        char* evaluatedInput = (char*) malloc(10*strlen(Input)*sizeof(char));
        evaluate_expression(Input,evaluatedInput);
        printf(evaluatedInput);
        CommandArray = parse_input(evaluatedInput,&a);
        printf("first:");
        printf(CommandArray[0]);
        
        printf("\n");
        printf("second:");
        printf(CommandArray[1]);
        
        printf("\n");
        input_type = get_input_type(CommandArray[0]);
        switch(input_type){
            case shell_builtin:
                status = execute_shell_builtin(CommandArray);
                break;
            case executable_or_error:
                status = execute_command(CommandArray);
                break;
        }
        for (int i = 0; i < ArgumentsNumber; i++)
        {
            free(CommandArray[i]);
        }
        free(CommandArray);
        free(evaluatedInput);
    }
    while (status>=0);
    printf("%d",status);
    //free resources & exit;
    for (int i = 0; i < assingments_count; i++)
    {
        free(assignments[i]);
    }
    free(assignments);
    free(CurrentWorkingDirectory);
    free(PreviousDirectory);
    free(Input);
    return;
}



int main(){
    printf("hi\n");
    fflush(stdout);
    fflush(stdin);
    struct sigaction SA;
    SA.sa_handler=&on_child_exit;
    sigaction(SIGCHLD,&SA,NULL);

    setup_environment();
    shell();
    return 0;
}
