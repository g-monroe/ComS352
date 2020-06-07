#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    char c;
    int i;
} letter;

char *infile, *outfile;
char *inBuffer, *outBuffer;
unsigned bufferSize;
sem_t inMutex, outMutex;
sem_t encryptFull, encryptEmpty, readEmpty, writeFull, inFull, outFull;
pthread_t tRead, tEncrypt, tWrite, tInCounter, tOutCounter;


int charToInt(char letter) {
    if (letter >= 'A' && letter <= 'Z') return letter - 'A';
    else if (letter >= 'a' && letter <= 'z') return letter - 'a';
    else return -1;
}
void countChars(letter* occurrences) {
    for (int i = 0; i < 26; i++) {
        if (charToInt(occurrences[i].c) != -1)
            printf("%c: %d\n", occurrences[i].c, occurrences[i].i);
    }
}

void *reader(void *args) {
    FILE* in = fopen(infile, "r");
    int index = 0;
    while (1) {
        char c = fgetc(in);
        sem_wait(&readEmpty);
        sem_wait(&inMutex);
        inBuffer[index] = c;
        sem_post(&inMutex);
        sem_post(&encryptFull);
        sem_post(&inFull);
        if (c == EOF) break;
        index = (index + 1) % bufferSize;
    }
    fclose(in);
    return NULL;
}

void *writer(void *args) {
    FILE* out = fopen(outfile, "w");
    int index = 0;
    while (1) {
        sem_wait(&writeFull);
        sem_wait(&outMutex);
        char c = outBuffer[index];
        sem_post(&outMutex);
        sem_post(&encryptEmpty);
        index = (index + 1) % bufferSize;
        if (c == EOF) break;
        fputc(c, out);
    }
    fclose(out);
    return NULL;
}

char encrypt_char(char c, int *s) {
    if (charToInt(c) == -1) return c;
    if (*s == 1) {
        *s = -1;
        if (c == 'Z' || c == 'z') return c - 25;
        else return c + 1;
    }
    else if (*s == -1) {
        *s = 0;
        if (c == 'A' || c == 'a') return c + 25;
        else return c - 1;
    }
    else {
        *s = 1;
        return c;
    }
}

void *encryption(void *args) {
    int s = 1;
    int index = 0;
    while (1) {
        sem_wait(&encryptFull);
        sem_wait(&inMutex);
        char c = inBuffer[index];
        printf("ENCRYPTION: Access char '%c' at index '%d'\n", c, index);
        sem_post(&inMutex);
        sem_post(&readEmpty);
        c = encrypt_char(c, &s);
        sem_wait(&encryptEmpty);
        sem_wait(&outMutex);
        outBuffer[index] = c;
        sem_post(&outMutex);
        sem_post(&outFull);
        sem_post(&writeFull);
        index = (index + 1) % bufferSize;
        if (c == EOF) break;
    }
    return NULL;
}
void *inCounter(void *args) {
    letter ch[26];
    for (int i = 0; i < 26; i++) { ch[i].c = '.'; ch[i].i = 0; }
    int index = 0;
    while (1) {
        //Mutex wait
        sem_wait(&inFull);
        sem_wait(&inMutex);
        //Mutex  post
        char c = inBuffer[index];
        sem_post(&inMutex);
        sem_post(&readEmpty);
        index = (index + 1) % bufferSize;
        if (c == EOF) break;
        if (charToInt(c) != -1) {
            ch[charToInt(c)].c = toupper(c);
            ch[charToInt(c)].i++;
        }
    }
    //Print
    printf("Input file contains:\n");
    countChars(ch);
    return NULL;
}

void *outCounter(void *args) {
    letter ch[26];
    for (int i = 0; i < 26; i++) { ch[i].c = '.'; ch[i].i = 0; }
    int index = 0;
    while (1) {
        //wait to mutex
        sem_wait(&outFull);
        sem_wait(&outMutex);
        //add
        char c = outBuffer[index];
        //post to mutex
        sem_post(&outMutex);
        sem_post(&encryptEmpty);
        index = (index + 1) % bufferSize;
        if (c == EOF) break;//End of file.
        if (charToInt(c) != -1) {
            ch[charToInt(c)].c = toupper(c);
            ch[charToInt(c)].i++;
        }
    }
    //Print
    printf("Output file contains: \n");
    countChars(ch);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "ERR: Arg for 'Infile' and Arg for outfile needed!\n"
                        "Form: \"encrypt <input file> <output file>\"\n");
        exit(1);
    }

    infile = argv[1];
    outfile = argv[2];
    if (access(infile, F_OK) == -1) {
        fprintf(stderr, "ERR: Given input file does not exist: %s\n", infile);
        exit(1);
    }

    if (access(outfile, F_OK) == -1) {
        fprintf(stderr, "ERR: Given output file does not exist: %s\n", outfile);
        exit(1);
    }

    printf("Enter buffer size: ");
    fscanf(stdin, "%d", &bufferSize);
    inBuffer = malloc(sizeof(char) * (bufferSize + 1));
    outBuffer = malloc(sizeof(char) * (bufferSize + 1));
    //Initilizer
    sem_init(&encryptFull, 0, 0);
    sem_init(&encryptEmpty, 0, bufferSize);
    sem_init(&readEmpty, 0, bufferSize);
    sem_init(&writeFull, 0, 0);
    sem_init(&inFull, 0, 0);
    sem_init(&outFull, 0, 0);
    sem_init(&inMutex, 0, 1);
    sem_init(&outMutex, 0, 1);
    //Create threads
    pthread_create(&tRead, NULL, reader, NULL);
    pthread_create(&tWrite, NULL, writer, NULL);
    pthread_create(&tInCounter, NULL, inCounter, NULL);
    pthread_create(&tOutCounter, NULL, outCounter, NULL);
    pthread_create(&tEncrypt, NULL, encryption, NULL);
    //Join back together
    pthread_join(tRead, NULL);
    pthread_join(tEncrypt, NULL);
    pthread_join(tWrite, NULL);
    pthread_join(tInCounter, NULL);
    pthread_join(tOutCounter, NULL);
    //Kill trheads
    sem_destroy(&encryptFull);
    sem_destroy(&encryptEmpty);
    sem_destroy(&readEmpty);
    sem_destroy(&writeFull);
    sem_destroy(&inFull);
    sem_destroy(&outFull);
    sem_destroy(&inMutex);
    sem_destroy(&outMutex);
    //Die
    return 0;
}