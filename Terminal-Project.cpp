#include <iostream>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

string trim(string input){
    int i = 0;
    while(i < input.size() && input[i] == ' '){
        i++;
    }
    if(i < input.size()){
        input = input.substr(i);
    } else {
        return "";
        
    }

    i = input.size()-1;
    while(i >= 0 && input[i] == ' '){
        i--;
    }
    if(i >= 0){
        input = input.substr(0,i+1);
    } else {
        return "";
    }
    
    return input;
    
}

char** vec_to_char_array(vector<string>& parts){
    char** result = new char * [parts.size() + 1];
    for (int i = 0; i < parts.size(); i++){
        result[i] = (char*) parts[i].c_str();
    }
    result[parts.size()] = NULL;
    return result;
    
}

vector<string> split(string input, string seperator = " "){
    vector<string> result;
    bool has_quotations = false;

    int quote_count = 0;
    int quotation_start = 0;
    int quotation_end = 0;
    int prev_quote_count = 0;

    for(int i = 0; i < input.size(); i++){
        if(input[i] == '"' || input[i] == '\''){
            quote_count++;
            quotation_end = i;
            
            if(quote_count == 1 && prev_quote_count != 1){
                quotation_start = i;
                prev_quote_count++;
                }
            if(quote_count % 2 == 0){
                has_quotations = true;
            } else {
                has_quotations = false;
            }
        }
        
    }
    
    
    while(input.size()){
        int index = input.find(seperator);
       
        if(index != input.npos){
            result.push_back(input.substr(0, index));
            
            if(has_quotations){
                
                input = input.substr(quotation_start+1, quotation_end-(quotation_start+1));
                result.push_back(input);
            
                break;
                
            } else {
                input = input.substr(index + seperator.size());
               
            }
           
            if(input.size() == 0){
                result.push_back(input);
            }

        }else{
            
            result.push_back(input);
            input = "";
        }
    }

    
    return result;
}

void execute(string input){
    vector<string> split_input;
    char** arguments;
    cout << "in execute" << endl;
    input = trim(input);
    split_input = split(input);
    arguments = vec_to_char_array(split_input);

    execvp(arguments[0], arguments);


}

string IO_process_output(string input_line){
    string name = "";
    string file_name = "";

    name = input_line.substr(input_line.find('>'), input_line.size()-1);
    input_line = input_line.substr(0,input_line.find('>'));

    name = trim(name);
    input_line = trim(input_line);

    for(int i = 0; i < name.size(); i++){//geting stuck in here
        
        if(isalpha(name[i]) || name[i] == '.'){
            file_name += name[i];
            
        }
    }
    
    file_name = trim(file_name);
    
    
    int fd = open((char*) file_name.c_str(), O_CREAT| O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(fd,1);
    return input_line;
}

string IO_process_input(string input_line){
    string name = "";
    string file_name = "";

    
    name = input_line.substr(input_line.find('<'), input_line.size()-1);
    input_line = input_line.substr(0,input_line.find('<'));

    name = trim(name);
    input_line = trim(input_line);

    for(int i = 0; i < name.size(); i++){//geting stuck in here
        if(isalpha(name[i]) || name[i] == '.'){
            file_name += name[i];
            
        }
    }
    file_name = trim(file_name);
    
    int fd = open((char*) file_name.c_str(), O_CREAT| O_RDONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(fd,0);
    return input_line;
}


int main(){

    /*
    * dont use pattern for 4 use something like cpp, there are no files with pattern in them
    *
    * 
    */

    int in_def = dup(0);
    int out_def = dup(1);

    vector <int> bgs; //list of background processes
    
    while(true){
        dup2(in_def, 0);
        dup2(out_def, 1);
        /*
        * Zombie Slaya
        */
        

        for(int i = 0; i < bgs.size(); i++){
            if(waitpid(bgs[i], 0, WNOHANG) == bgs[i]){
                cout << "Process: " << bgs[i] << " ended" << endl;
                bgs.erase(bgs.begin() + i);
                i--;
            }
        }

        /*
        * Shell output
        */
        cout << getlogin() << "$ ";
        string input_line;
        getline(cin, input_line);
        if(input_line == string("exit")){
            cout << "BYE! End of Shell, Thanks and Gig 'em" << endl;
            break;
        }

        input_line = trim(input_line);



        /*
        * int chdir(const char* path)
        * to use for cd, do not let exec handle cd
        * 
        * 
        */
        bool cd = false;
        if(input_line.find("cd") != input_line.npos){
            cd = true;
        }
        
        
        /*
        * Background Processes
        */
        bool bg = false;
        if(input_line[input_line.size()-1] == '&'){
            bg = true;
            input_line = input_line.substr(0,input_line.size()-1);
            input_line = trim(input_line);
        }

        bool has_quotations = false;
        for(int i = 0; i < input_line.size(); i++){
            if(input_line[i] == '"' || input_line[i] == '\''){
                has_quotations = true;
            }
        }
        

        /*
        * IPC Piping
        * after the if add them to the background process, look at slides
        * look at campus wire as well for resetting stdin and stout
        * 
        */
        bool has_pipe = false;
        for(int i = 0; i < input_line.size(); i++){
            if(input_line[i] == '|'){
                has_pipe = true;
            }
        }


        if(cd){ 
            string path_string;
            char* path;
            vector<string> prev_paths;
            int position_aftercd = 0;
            //char* prev_path;
            
            
            if(input_line.find("-") != input_line.npos){
                //prev_path = (char*) prev_paths[prev_paths.size()-1].c_str();
                chdir(path);
            } else {

                position_aftercd = input_line.find("cd") + 3;
            
                path_string = input_line.substr(position_aftercd, input_line.size());
                prev_paths.push_back(path_string);
                path = (char*) path_string.c_str();

                chdir(path);
            }
            

        } else if(has_pipe && has_quotations == false && cd == false){

           vector<string> c = split(input_line, " | ");
    
            for(int i = 0; i < c.size(); i++){
                int fd[2];
                pipe(fd);
                int cid = fork();
                if(!cid){
                    if(i < c.size() - 1){
                        dup2(fd[1], 1);
                    }
                    execute(c[i]);
                    bgs.push_back(cid);
                } else {
                    waitpid(cid, 0, 0);
                    dup2(fd[0], 0);
                    close(fd[1]);
                }
            }

        } else {
            /*
            * Child Processes 
            */
            int pid = fork();

            if (pid == 0){//child process

                /*
                * I/O Processing
                * 
                */
                if((input_line.find('>') != input_line.npos || input_line.find('<') != input_line.npos)  && has_quotations == false && cd == false) { 
                
                    if(input_line.find('>') != input_line.npos){
                        input_line = IO_process_output(input_line);
                    }
                    if(input_line.find('<') != input_line.npos){
                        input_line = IO_process_input(input_line);
                    }

                   
                } 
                
            
                    
                vector<string> parts = split(input_line);
                char** args = vec_to_char_array(parts);
                execvp(args[0], args);
                
                
            } else {
                if(!bg){
                    waitpid(pid, 0, 0);//wait for the child process
                } else {
                    bgs.push_back(pid);
                }
                
            }
            
        }


       
    }

}