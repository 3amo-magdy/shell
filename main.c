#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>
//global enums
enum inputType{shell_builtin,executable_or_error}; 
enum commandType{cd, echo, export, undefined, end};

//for (malloc):
const int Max_words = 16;  //max words per line
const int Max_chars = 12;  //max characters per word
const int Max_assignments = 32;  //max number of assignments 

int ArgumentsNumber;    //number of space-seperated elements in the command inputted
char* PreviousDirectory;   //path of the previous directory
char* CurrentWorkingDirectory;   //Path of the current directory
char** assignments;     //assignments table for exporting variables
int assingments_count;  //current number of assignments
int background;   //background process or not ? 1 or 0
pid_t child;    //pid_t for fork



//takes a string, outputs a pointer to the array of strings seperated by spaces of the original string
//takes int *a to store the number of words found in it
char** parse_input(char* CommandLine,int*a){
    *a = 0;    //index of a word - first dimension
    int b=0;   //index of a character - second dmension

    background=0; //setting background to another value later if found "&" as the last argument
    int mode = 0; //there are two modes : 0 for storing characters sequentially,
                                       // 1 for skipping characters sequentially
    
    char quote[16];
    int quote_num=0;
    char** CommandArray = (char**)malloc(Max_words*sizeof(char*));   //(allocate memory) for the returned 2d structure
    for (int i = 0; i < Max_words; i++)
    {
        CommandArray[i] = (char*) malloc(Max_chars*sizeof(char));
    }

    for(int i=0;i<strlen(CommandLine)&&CommandLine[i]!='\n';i++){
        if(quote_num==0&&mode==2){
            mode = 0;
        }
        else if((CommandLine[i]==' ')&&mode==0){  //switch to the skipping mode (1)
            mode = 1;
        }
        else if(CommandLine[i]=='"'||CommandLine[i]=='\''){
            if(quote_num>0 && quote[quote_num-1]==CommandLine[i]){
                quote[quote_num]=NULL;
                quote_num--;
            }
            else{
                quote[quote_num++]=CommandLine[i];
                mode = 2;
            }
        }
        else if((CommandLine[i]!=' ')&&mode==1){   //switch to the storing mode (0)
            CommandArray[*a][b]='\0'; //finish the completed word by appending a '\0'
            b=0;
            mode = 0;
            *a=*a+1;

            if((!strcmp(CommandArray[0],"cd"))||
                (!strcmp(CommandArray[0],"echo"))){
               CommandArray[*a]=(CommandLine+i);
               return CommandArray;
            }
        }
        if(mode == 2||mode ==0){
            CommandArray[*a][b]=CommandLine[i];
            b++;
            CommandArray[*a]=realloc(CommandArray[*a],(b+1)*sizeof(char));
            
        }
        else{ 
            //skip space characters
        }
    }
    if(quote_num!=0){
        *a = -1;
        return CommandArray;
    }
    CommandArray[*a][b]='\0'; //finish the last argument string (word)
   
    if(!strcmp(CommandArray[*a],"&")){ //if "&" was the last argument --> background 
        background=1;   //don't increment *a to overwrite the "&" with NULL
    }
    else{   
       // increment *a to keep the last argument 
        *a=*a+1;
        background=0;
    }
    CommandArray = realloc(CommandArray,((*a)+1)*sizeof(char*));
    CommandArray[*a]=NULL;
    ArgumentsNumber=*a;
    return CommandArray;
}



//write a sentence onto the log.txt file 
void write_to_log_file(const char *sentence,pid_t id){
    time_t t = time(NULL);
    struct tm tm =*localtime(&t);
    FILE* f = fopen("log.txt","a");
    fprintf(f,"%d-%02d-%02d %02d:%02d:%02d -> %s ",(tm.tm_year+1900),tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec,sentence);
    if(id){
        fprintf(f," %d",id);
    }
    fprintf(f,"\n");
    
    fclose(f);
    // to be implemented
}

//for handling un-waited for children that terminated but whose places in the processes table are yet to be released
void reap_zombie_children(){
    pid_t res;
    while(res = waitpid(-1,NULL,WNOHANG)){
        if(res!=-1){
            write_to_log_file("Background Child terminated :",res);
        }
        else{
            if(errno!=10){
                write_to_log_file(strerror(errno),NULL);
            }
            else{
                write_to_log_file("Foreground Child terminated",NULL);
            }
            break;
        }

    }
}
//the handler for SIGCHLD
void on_child_exit(){
    // a child has terminated and it's "waitable":
    reap_zombie_children();
}

void setup_environment(){
    //the two Directories initialization
    CurrentWorkingDirectory = (char*) malloc(Max_words*Max_chars*sizeof(char));
    PreviousDirectory = (char*) malloc(Max_words*sizeof(char));
    PreviousDirectory[0]='!';
    CurrentWorkingDirectory=  getcwd(CurrentWorkingDirectory,Max_words*Max_chars*sizeof(char));
    //chdir(CurrentWorkingDirectory);
   
    assignments=(char**)malloc(2*Max_assignments*sizeof(char*));
    for(int n=0;n<2*Max_assignments;n++){
        assignments[n]=(char*)malloc(Max_chars);
    }
    assingments_count=0;

    background=0;
    write_to_log_file("New session started !",NULL);
}
//to read the next command line
void read_input(char* input){
    fflush(stdin);
    input = fgets(input,Max_words*Max_chars,stdin);
    return;
}
//to import the value of a variable in the assignment table
void* import(char*var){
    for (int i = 0; i <assingments_count ; i+=2){
        if(!strcmp(var,assignments[i])){
            return assignments[i+1];
        }
    }
    return NULL;
}
//to determine whether the command is built-in or external
enum inputType get_input_type(char* command){
    if(!strcmp(command,"echo")||!strcmp(command,"export")||!strcmp(command,"cd")||!strcmp(command,"exit")){
        return shell_builtin;
    }
    return executable_or_error;
}
//in case it was built-in, then which one is it ?:
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
//replaces each ($variable) with the corresponding (value) 
//[takes apointer *exp to the original string and another pointer to store in the new resulting string]
void evaluate_expression(char* exp,char* newExpression){
    char* value;
    int var_counter=0;
    int newSize=0;
    for (int i = 0; i < strlen(exp)&&exp[i]!='\n'; i++)
    {
        if(exp[i]=='$'){
            i++;
            while((i+var_counter)<strlen(exp)&&((exp[i+var_counter]>64&&exp[i+var_counter]<91)||(exp[i+var_counter]>96&&exp[i+var_counter]<123)||(exp[i+var_counter]>47&&exp[i+var_counter]<58))){
                var_counter++;
            }
            // char* var =malloc(var_counter*sizeof(char)); --->found to be not necessary cuz it's just a local temporary variable
            char var [var_counter+1];
            //gather the variable into var : char*
            for(int j=0;j<var_counter;j++){
                var[j] = exp[i+j];
            }
            var[var_counter]='\0';
            
            if(var_counter){
                i+=var_counter-1;
            }

            var_counter=0;
            //import the corresponding value from the table:
            value = import(var);
            //if not found, replace it with empty string (a.k.a remove it)
            if(value == NULL){
                value = "";
            }
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
    newExpression[newSize]='\0';
    newSize++;
    newExpression = (char*)realloc(newExpression,sizeof(char)*(newSize));
    return ;
}
//for cd command
void changeDirectory(char** arguments_string){
   //if [cd]: go to HOME
    if(!arguments_string[1]){
        PreviousDirectory = CurrentWorkingDirectory;
        CurrentWorkingDirectory = getenv("HOME");
        chdir(CurrentWorkingDirectory);
        return;
    }
    int n=0;
    char** arguments = parse_input(arguments_string[1],&n);
    //if there was 1 argument:
    if(n==1){
        //for [cd -]
        if(strcmp(arguments[0],"-")==0){
            if(PreviousDirectory[0]=='!'){
                //do nothing as no previous directory was set yet
            }
            else{
                //go to the previous directory and swap current with previous
                chdir(PreviousDirectory);
                char* temp = (PreviousDirectory);
                PreviousDirectory = CurrentWorkingDirectory;
                CurrentWorkingDirectory = temp;

            }
        }
        //for[cd ~] go to HOME
        else if(strcmp(arguments[0],"~")==0){
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            strcpy(CurrentWorkingDirectory , getenv("HOME"));
            chdir(CurrentWorkingDirectory);        
        }
        // for [cd ..]
        else if(strcmp(arguments[0],"..")==0||strcmp(arguments[0],"../")==0){
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            chdir("..");
            CurrentWorkingDirectory= getcwd(CurrentWorkingDirectory,sizeof(CurrentWorkingDirectory));
        }
        // for [cd <path>]
        else {
            strcpy(PreviousDirectory,CurrentWorkingDirectory);
            if(!chdir(arguments[0])){
               perror("Error");
            };
            CurrentWorkingDirectory= getcwd(CurrentWorkingDirectory,sizeof(CurrentWorkingDirectory));
        }
    }
}
//to store a pair of variable and value into the table:
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

    assignments[assingments_count-2]=var;
    assignments[assingments_count-1]=value;

}
//to print the assignment table --> for when export got no arguments & also for testing purposes
void printAssingments(){
    for (int i = 0; i <assingments_count ; i+=2){
        printf("%s : %s\n",assignments[i],assignments[i+1]);            
    }
}
//takes an argument of export and processes it (ex : "x=sad"), returns -1 if something was wrong
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
    variable=realloc(variable,(var_c+1)*sizeof(char));
    for(;i<strlen(export_argument)&&export_argument[i]!='\n';i++){
        if(export_argument[i]!='"'&&export_argument[i]!='\''){
            value[val_c]=export_argument[i];
            val_c++;
        }
    }
    if(val_c==0||var_c==0){
        return -1;
    }
    value=realloc(value,(val_c+1)*sizeof(char));
    variable[var_c]='\0';
    value[val_c]='\0';
    assign(variable,value);
}
//takes the whole export command (ex: 'export x=sad y=423 z="yes"')
void Export(char** CommandArray,int count){
    if(count < 2){
        printAssingments();
    }
    for (int i = 1; i < count; i++)
    {
        int res = ExportArg(CommandArray[i]);
        if(res == -1){
            printf("%s is not a valid export identifier\n",CommandArray[i]);
        }
    }
}
//if the command was built-in:
int execute_shell_builtin(char** CommandArray){
    char* command = CommandArray[0];
    enum commandType command_type = determine_built_in_command(command);
    switch(command_type){
        case cd:
            changeDirectory(CommandArray);
            break;
        case echo:
            for(int c=0;c<strlen(CommandArray[1]);c++){
                if(CommandArray[1][c] != '"'){
                    printf("%c",CommandArray[1][c]);
                }
            }
            printf("\n");
            break;
        case export:
            Export(CommandArray,ArgumentsNumber);
            break;
        case undefined:
            return -1;
        case end:
            return -2;
    }
    return 0;
}
//if it was an external command:
int execute_command(char** command){
    pid_t id = fork();
    //child process:
    if(id==0&&id!=-1){
        child = id;
        execvp(command[0],command);
        printf("Error processing the command, make sure the command %s is correct\n",command[0]);
        return(-1);
    }
    //parent process:
        //foreground:
    else if (id!=0&&!background){
        //wait on the child
        //we can use first parameter as -1 cuz we only would have one foreground child at a time
        waitpid(-1,NULL,0);
    }
        //background:
    else if(id ==0&&background){
        //don't wait on the child .. keep going in ur flow
    }
    else if(id==-1){
        printf("Error Occured");
    }
}
//contains the loop of the shell 
//makes use of the above methods to parse, evaluate and execute commands
void shell(){
    char* Input = malloc(Max_chars*Max_words*sizeof(char));
    char** CommandArray;
    int a=0;
    enum inputType input_type=shell_builtin;
    int status=0;
    do{
        printf("..%s$ ",CurrentWorkingDirectory);
        read_input(Input);
        a=0;
        char* evaluatedInput = (char*) malloc(10*strlen(Input)*sizeof(char));
        evaluate_expression(Input,evaluatedInput);
        CommandArray = parse_input(evaluatedInput,&a);
        if(a==-1){
            printf("Parsing Error\n");
        }
        else{
            input_type = get_input_type(CommandArray[0]);
            switch(input_type){
                case shell_builtin:
                    status = execute_shell_builtin(CommandArray);
                    break;
                case executable_or_error:
                    status = execute_command(CommandArray);
                    break;
            }
        }
        // for (int i = 0; i < ArgumentsNumber; i++)
        // {
        //     free(CommandArray[i]);
        // }
        free(CommandArray);
        free(evaluatedInput);
    }
    while (status>=0);
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


//the main function:
int main(){
    fflush(stdout);
    fflush(stdin);
    //register SIGCHLD handler (on_child_exit):
        // struct sigaction SA;
        // SA.sa_handler=&on_child_exit;
        // sigaction(SIGCHLD,&SA,NULL);
        signal(SIGCHLD,on_child_exit);
    //setup environment and start taking commands;
        setup_environment();
    shell();
    return 0;
}
