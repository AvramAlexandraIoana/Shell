
///se ruleaza cu flag lreadline
//// gcc main.c -L/usr/include -lreadline -o 
//// ./a.out

#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h>  //folosim libraria readline, trebuie instalata in prealabil
#include<readline/history.h> 
  
#define MAXCOM 1000 //nr maxim de litere al unei comenzi
#define MAXLIST 100 // nr maxim de comenzi ce pot fi retinutes
  
//functie pentru eliberarea ecranului/ CTRL + L
#define clear() printf("\033[H\033[J") 
  
//initializare, mesaj de start + numele utilizatorului etc
void init_shell() 
{ 
    clear(); 
    printf("\n\n\n\n******************"
        "************************"); 
    printf("\n\n\n\t****SHELL****"); 
    printf("\n\n\n\n*******************"
        "***********************"); 
    char* username = getenv("USER"); 
    printf("\n\n\nUSER is: @%s", username); 
    printf("\n"); 
    sleep(1); 
    clear(); 
} 
  
// Functie pentru citirea datelor de intrare
int takeInput(char* str) 
{ 
    char* buf; 
  
    buf = readline("\n>>> "); //citim comanda
    if (strlen(buf) != 0) { 
        add_history(buf); // daca exista, o adaugam in istoric
        strcpy(str, buf); 
        return 0; 
    } else { 
        return 1; 
    } 
} 
  
// afisarea folderului curent (pwd)
void printDir() 
{ 
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); //preluam numele directorului curent cu functia de sistem getcwd 
    printf("\nDir: %s", cwd); 
} 
  
// Functia care executa comenzile de sistem simple
void execArgs(char** parsed) 
{ 
    // cream un nou proces copil
    pid_t pid = fork();  
  
    if (pid == -1) { 
        printf("\nFailed forking child..");  // eroare la creare
        return; 
    } else if (pid == 0) { 
        if (execvp(parsed[0], parsed) < 0) {  //executam comanda
            printf("\nCould not execute command..");  //daca intoarece -1, eroare la executie
        } 
        exit(0); 
    } else { 
        wait(NULL);  
        return; 
    } 
} 
  
// Functia in care executam comenzile pipe
void execArgsPiped(char** parsed, char** parsedpipe) 
{ 
    int pipefd[2];  
    pid_t p1, p2; 
  
    if (pipe(pipefd) < 0) { 
        printf("\nPipe could not be initialized"); 
        return; 
    } 
    p1 = fork(); //cream un nou proces
    if (p1 < 0) { 
        printf("\nCould not fork"); 
        return; 
    } 
  
    if (p1 == 0) { 
   
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); //facem o copie a file descriptorului 
        close(pipefd[1]); 
  
        if (execvp(parsed[0], parsed) < 0) { 
            printf("\nCould not execute command 1.."); //eroare la executia comenzii
            exit(0); 
        } 
    } else { 

        p2 = fork(); 
  
        if (p2 < 0) { 
            printf("\nCould not fork"); 
            return; 
        } 
  
        if (p2 == 0) { 
            close(pipefd[1]); 
            dup2(pipefd[0], STDIN_FILENO);  //facem o copie a file descriptorului 
            close(pipefd[0]); 
            if (execvp(parsedpipe[0], parsedpipe) < 0) { 
                printf("\nCould not execute command 2.."); 
                exit(0); 
            } 
        } else { 

            wait(NULL); 
            wait(NULL); 
        } 
    } 
} 
  

void openHelp() 
{ 
    puts("\n***WELCOME TO MY SHELL HELP***"
        "\nCopyright @ Suprotik Dey"
        "\n-Use the shell at your own risk..."
        "\nList of Commands supported:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>all other general commands available in UNIX shell"
        "\n>pipe handling"
        "\n>improper space handling"); 
  
    return; 
} 
  
// Lista cu cateva comenzi implementate manual -> comenzi speciale
int ownCmdHandler(char** parsed) 
{ 
    int NoOfOwnCmds = 4, i, switchOwnArg = 0; 
    char* ListOfOwnCmds[NoOfOwnCmds]; 
    char* username; 
  
    ListOfOwnCmds[0] = "exit"; 
    ListOfOwnCmds[1] = "cd"; 
    ListOfOwnCmds[2] = "help"; 
    ListOfOwnCmds[3] = "hello"; 
  
    for (i = 0; i < NoOfOwnCmds; i++) { 
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) { 
            switchOwnArg = i + 1; 
            break; 
        } 
    } 
  
    switch (switchOwnArg) { 
    case 1: 
        printf("\nGoodbye\n"); 
        exit(0); 
    case 2: 
        chdir(parsed[1]); 
        return 1; 
    case 3: 
        openHelp(); 
        return 1; 
    case 4: 
        username = getenv("USER"); 
        printf("\nHello %s.\nMind that this is "
            "not a place to play around."
            "\nUse help to know more..\n", 
            username); 
        return 1; 
    default: 
        break; 
    } 
  
    return 0; 

    //se returneaza 1 daca s-a detectat o comanda valida
} 
  
// verificam daca avem o comanda simpla sau piped
int parsePipe(char* str, char** strpiped) 
{ 
    int i; 
    for (i = 0; i < 2; i++) { 
        strpiped[i] = strsep(&str, "|"); //incercam split dupa caracterul |
        if (strpiped[i] == NULL) 
            break; 
    } 
  
    if (strpiped[1] == NULL) //daca e doar un element -> comanda simpla
        return 0; //returnam 0 (comanda e simpla)
    else { 
        return 1; //returnam 1 (comanda e de tip pipe)
    } 
} 
  
// functie pentru splitarea comenzii introduse dupa spatii
void parseSpace(char* str, char** parsed) 
{ 
    int i; 
  	// se creeaza o matrice de cuvinte ( comenzi )
    for (i = 0; i < MAXLIST; i++) { 
        parsed[i] = strsep(&str, " "); 
  
        if (parsed[i] == NULL) 
            break; 
        if (strlen(parsed[i]) == 0) 
            i--; 
    } 
} 


int processString(char* str, char** parsed, char** parsedpipe) 
{ 
  
    char* strpiped[2]; 
    int piped = 0; 
  
    piped = parsePipe(str, strpiped); //verificam daca expresia e piped sau nu
 

 	//eliminam eventualele spatii 
    if (piped) { 
        parseSpace(strpiped[0], parsed); 
        parseSpace(strpiped[1], parsedpipe); 
  
    } else { 
  
        parseSpace(str, parsed); 
    } 
  
    if (ownCmdHandler(parsed)) //daca a fost una din comenzile speciale, s-a executat deja
        return 0; 
    else
        return 1 + piped; //returnam 1 daca comanda de sistem e simpla si 2 daca e piped
} 
  
int main() 
{ 
    char inputString[MAXCOM], *parsedArgs[MAXLIST]; 
    char* parsedArgsPiped[MAXLIST]; 
    int execFlag = 0; 
    init_shell(); 
  
    while (1) { 
        printDir(); 


        // preluam comanda de la utilizator
        if (takeInput(inputString)) 
            continue; 

        execFlag = processString(inputString, 
        parsedArgs, parsedArgsPiped); //verificam ce tip de comanda avem


        // executie
        if (execFlag == 1) 
            execArgs(parsedArgs); // comanda simpla 
  
        if (execFlag == 2) 
            execArgsPiped(parsedArgs, parsedArgsPiped); //expresie pipe
    } 
    return 0; 
} 

