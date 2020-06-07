// encryptiondecryptionoffiles.cpp : Defines the entry point for //the console application.
//header files
#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <semaphore.h>
using namespace std;
//declaration of function prototypes
char ceaserDecipher(char c, int key);
char ceaserCipher(char c, int key);
void* tReadFile(void *infile);
string readFile(char *infile);
void encrypt(string str, char *outfile);
void decrypt(char *infile, char *outfile);
void displayMenu();
void updateS();
void updateSDecrypt();
void countStr(string s);
/**************************************************************
* This is the main function of the program.                   *
* @return a value to terminate the program successfully       *
**************************************************************/
char *input_buffer, *output_buffer;
unsigned buffer_size;
sem_t encryption_full, encryption_empty, reader_empty, writer_full, input_counter_full, output_counter_full;
sem_t input_mutex, output_mutex;

int main()
{
     //call the display menu
     displayMenu();
     system("pause");
     return 0;
}
string readFile(char *infile){
     ifstream in(infile);
     string str;
     char ch;
     if (!in)
     {
          cout << "<!!!> Cannot open 'Input File'.\n";
          return str;
     }
     while (in)
     {
          in.get(ch);
          if (in)
          {
               str += ch;
          }
     }
     return str;
}
void countStr(string str){
     int numberOfLetters = 26;
     int counts[numberOfLetters];
     for (int i = 0; i < numberOfLetters; i++)
	{
		counts[i] = 0;
	}

	for (std::string::size_type i = 0; i < str.size(); i++) {
		{
			str[i] = tolower(str[i]);
			counts[str[i] - 'a'] ++;
		}
	}
     for (int i = 0; i < numberOfLetters; i++)
	{
          if (counts[i] != 0){
               char letter = toupper(static_cast<char>(i + 'a'));
               cout << letter << ": " << counts[i] << "\n";
          }
	}
     return;
}
/**************************************************************
* This function displays the menu options as in the handout   *
**************************************************************/
void displayMenu()
{
     //declare local variables
     int choice;
     char option;
     char inputFile[20];
     char outputFile[20];
     pthread_t threads[3];
     do
     {
          printf("\033c");
          cout << "Please enter your selection: \n";
          cout << "1.  Encrypt\n";
          cout << "2.  Decrypt \n";
          cin >> choice;
          if (choice == 1)
          {
               pthread_t threads;
               cout << "[!] Enter your 'Input File':\n";
               cin >> inputFile;
               cout << "[!] Enter your 'Output File':\n";
               cin >> outputFile;
               //pthread_create(&threads, NULL, reader, (void*)inputFile);
               //pthread_join(threads, NULL); 
               //cout << input_buffer << "\n";
               string input = readFile(inputFile);
               encrypt(input, outputFile);
               cout << "[?] Input file contains:\n";
               countStr(input);
               cout << "[?] Output file contains:\n";
               string output = readFile(outputFile);
               countStr(output);
               cout << "-- Encrypted Successful --\n";
          }
          else if (choice == 2)
          {
               cout << "[!] Enter your 'Input File':\n";
               cin >> inputFile;
               cout << "[!]Enter your 'Output File':\n";
               cin >> outputFile;
               decrypt(inputFile, outputFile);
               cout << "-- Decrypted Successful --\n";
              // ofstream out(outputFile);
               //countStr(out);
          }
          cout << "\n[!] Enter your selection:\n\n";
          cout << " 1. Continue\n";
          cout << " 2. Exit\n";
          cin >> option;
     } while (option != '2');
}
/**************************************************************
* This function encrypts the content of infile and saves the *
* encrypted text into outfile                                 *  
* @param infile string (file that has raw text)              *
* @param outfile string (file that will have encrypted text) *
**************************************************************/
int s = 1;
void encrypt(string str, char *outfile)
{
     ofstream out(outfile);
     char ch;
     int key;
     s = 1;
     cout << "[!] Enter Buffer: \n";
     cin >> key;
     if (!out)
     {
          cout << "<!!!> Cannot open 'Output File'.\n";
          return;
     }
     for (std::string::size_type i = 0; i < str.size(); i++) {
          char item = str[i];
          ch = ceaserCipher(item, key);
          out << ch;
     }
     return;
}
/**************************************************************
* This function decrypts the content of infile and saves the *
* decrypted text into outfile                                    *
* @param infile string (file that has encrypted text)        *
* @param outfile string (file that will have decrypted text) *
**************************************************************/
void decrypt(char *infile, char *outfile)
{
     ofstream out(outfile);
     ifstream in(infile);
     char ch;
     int key;
     cout << "[!] Enter Key: \n";
     cin >> key;
     s = -1;
     if (!out)
     {
          cout << "<!!!> Cannot open 'Output File'.\n";
          return;
     }
     if (!in)
     {
          cout << "<!!!> Cannot open 'Input File'.\n";
          return;
     }
     while (in)
     {
          in.get(ch);
          if (in)
          {
               char tempChar = ceaserDecipher(ch, key);
               cout << "(" << s << ")" << ch << " = " << tempChar << "\n"; 
               ch = tempChar;
               out << ch;
          }
     }
}
/**************************************************************
* This function takes an character and a cipher key to return *
* an encrypted character.                                     *
* @param c is a char (the character to be encrypted)          *
* @param key is an integer (cipher key given in the handout) *
**************************************************************/
char ceaserCipher(char c, int key)
{
     if ((c >= 'A') && (c <= 'Z')){
           if (s == 1){
               c = 'A' + (((c - 'A') + key) % 26);
          }else if (s == -1){
               c = 'A' + (((c - 'A') + (key + 24)) % 26);
          }
          updateS();
     }else if ((c >= 'a') && (c <= 'z')){
          if (s == 1){
               c = 'a' + (((c - 'a') + key) % 26);
          }else if (s == -1){
               c = 'a' + (((c - 'a') + (key + 24)) % 26);
          }
          updateS();
     }
     return c;
}
void updateS(){
     if (s == 1){
          s = -1;
     }else if (s == -1){
          s= 0;
     }else if (s == 0){
          s= 1;
     }
}
/**************************************************************
* This function takes an encrypted character and a cipher key *
* to return a decrypted character.                            *
* @param c is a char (the character to be decrypted)          *
* @param key is an integer (cipher key given in the handout) *
**************************************************************/
char ceaserDecipher(char c, int key)
{
     cout << c;
     if (((c >= 'A') && (c <= 'Z')) || ((c >= 'A') && (c <= 'Z'))){
          if (s == 1){
               c = 'Z' - ('Z' - c - (key - 26)) % 26;
          }else if (s == -1){
               c = 'Z' - ('Z' - c + key) % 26;
          }
          updateSDecrypt();
     }else if ((c >= 'a') && (c <= 'z')){
          if (s == 1){
               c = 'z' - ('z' - c - (key - 26)) % 26;
          }else if (s == -1){
               c = 'z' - ('z' - c + key) % 26;
          }
          updateSDecrypt();
     }
     return c;
}
void updateSDecrypt(){
     if (s == -1){
          s = 1;
     }else if (s == 1){
          s= 0;
     }else if (s == 0){
          s= -1;
     }
}