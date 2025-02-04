#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#define freef(x) if(x)free(x)
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define COL_SEP ',' // ? will be used in answers.txt to seperate values, questions arent answers lol
#define ROW_SEP 0xa /*newline/seperator*/
// csv

long getFileLength(FILE* fp) {
    long n;
    fseek(fp, 0L, SEEK_END);
    n = ftell(fp);
    rewind(fp);
    return n;
}

int getCode(char** code) {
    if (code == NULL) {
        return 1;
    }

    while (1) {
        printf("Enter your 6-character quiz code: ");
        char* res = fgets(*code, 10, stdin); // +2 for nl

        if (res == NULL) {
            fprintf(stderr, "%sFailed to read code. Try again.%s\n", KRED, KNRM);
            continue;
        }

        if (strlen(*code) < 6) {
            fprintf(stderr, "%sInvalid input length, Got: %ld\n%s", KRED, strlen(*code), KNRM);
            continue;
        }
        
        if (fflush(stdin)) {
            fprintf(stderr, "%sfflush() failed\n%s", KRED, KNRM);
            return 1;
        }
        break;
    }
    return 0;
}

int getQnA(char*** qp, char*** ap) {
    FILE *ansF = NULL, *quesF = NULL;
    char *ansBuf = NULL, *quesBuf = NULL, **answers = NULL, **questions = NULL;
    long ansFL=0, quesFL=0;
    
    char* code = malloc(10);
    while (1) {
        int r = getCode(&code);
        code[strlen(code) - 1] = 0;
        if (r)
            exit(1);
        
        int len = strlen("qnas/123456/questions.txt0");
        char* tmpstr = malloc(len);
        sprintf(tmpstr, "qnas/%s/", code);
        DIR* dir = opendir(tmpstr);
        if (dir) {
            closedir(dir);
        } else if (ENOENT == errno) {
            fprintf(stderr, "%sThe code is not in database.\n%s", KRED, KNRM);
            continue;
        }
        memset(tmpstr, 0, len);
        sprintf(tmpstr, "qnas/%s/answers.txt", code);
        ansF = fopen(tmpstr, "r");
        memset(tmpstr, 0, len);
        sprintf(tmpstr, "qnas/%s/questions.txt", code); 
        quesF = fopen(tmpstr, "r");
        free(tmpstr);
        tmpstr = NULL;
        break;
    }

    // check if files were successfully opened
    if (ansF == NULL) {
        fprintf(stderr, "answers.txt could not be opened.\n");
        exit(1);
    }
    if (quesF == NULL) {
        fprintf(stderr, "questions.txt could not be opened.\n");
        fclose(ansF);
        exit(1);
    }

    // allocate file length to string buffer, read into the buffer, 1 chunk only
    ansFL = getFileLength(ansF); quesFL = getFileLength(quesF);
    ansBuf = malloc(ansFL); quesBuf = malloc(quesFL);

    // safety
    if (ansBuf == NULL || quesBuf == NULL) {
        goto safe_exit;
    }

    int na = fread(ansBuf, ansFL, 1, ansF);
    int nq = fread(quesBuf, quesFL, 1, quesF);

    // if zero bytes or error, safe exit.
    if (na < 1 || nq < 1) {
        fprintf(stderr, "%sUnable to read from files or File could be empty\nans_n: %d, ques_n: %d\n%s", KRED, na, nq, KNRM);
        goto safe_exit;
    }
    
    // close fp's
    if (ansF != NULL && quesF != NULL) {
        fclose(ansF);
        fclose(quesF);
        ansF = NULL;
        quesF = NULL;
    }

    // future: maybe use strchr instead, its faster i believe
    // ansN_Nl = len(answers); quesN_Nl = len(questions)
    int ansNL = 1, quesNL = 1; // number of lines
    // loops until the bigger length of the two
    long gFL = (ansFL > quesFL) ? ansFL : quesFL; // greatest file length
    for (long i=0; i < gFL; i++) {
        if (i < ansFL && ansBuf[i] == ROW_SEP) ansNL++;
        if (i < quesFL && quesBuf[i] == ROW_SEP) quesNL++;
    }
    
    if (ansNL != quesNL) {
        fprintf(stderr, "%sSome questions might not have answers to them or vice-versa.\n%s", KRED, KNRM);
        goto safe_exit;
    }

    // in case theres only one line with everything
    // allocate answers and questions memory
    answers = malloc(ansNL); questions = malloc(quesNL);
    if (answers == NULL || questions == NULL) {
        fprintf(stderr, "malloc() failed\n");
        goto safe_exit;
    }

    // allocate mem for each line
    for (int i=0; i<ansNL; i++) {
        answers[i] = malloc(ansFL);
        questions[i] = malloc(quesFL);
    }

    // create local scope for clarity
    if (1) {
        // fill in the memory
        long tmp1=0, tmp2=0; // to store the address of the first char
        int ansIndex = 0, quesIndex = 0;
        for (long i=0; i < gFL; i++) {
            if (ansBuf == NULL || quesBuf == NULL || answers == NULL || questions == NULL) {
                fprintf(stderr, "Memory error\n");
                goto safe_exit;
            }
            if (i < ansFL && ansBuf[i] == ROW_SEP) {
                // reached new line in answers.txt
                answers[ansIndex] = (ansBuf + tmp1); // tmp1 is the offset
                ansBuf[i] = 0;
                tmp1 = i+1;
                ansIndex++;
            }
            if (i < quesFL && quesBuf[i] == ROW_SEP) {
                questions[quesIndex] = (quesBuf + tmp2); // tmp2 is the offset
                quesBuf[i] = 0;
                tmp2 = i+1;
                quesIndex++;
            }
            if (i == ansFL-1) {
                // last character
                answers[ansIndex] = (ansBuf + tmp1);
                tmp1 = -1;
                ansIndex = -1;
            }
            if (i == quesFL-1) {
                questions[quesIndex] = (quesBuf + tmp2);
                tmp2 = -1;
                quesIndex = -1;
            }
        }
    }
    if (ansF != NULL)
        fclose(ansF);
    if (quesF != NULL)
        fclose(quesF);
    *ap = answers;
    *qp = questions;
    return quesNL;

    safe_exit:
    if (ansF != NULL)
        fclose(ansF);
    if (quesF != NULL)
        fclose(quesF);
    freef(answers);
    freef(questions);
    freef(ansBuf);
    freef(quesBuf);
    return -1;
}


// credit for this function goes to: https://stackoverflow.com/a/6127606/17196870
void shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

int getSubAns(char* ans, char** subAns) {
    int subI = 0;
    char* tmp = ans;
    int l = strlen(ans);
    // split string with ans_sep
    for (int ansI=0; ansI<l; ansI++) {
        if (ans[ansI] == COL_SEP) {
            subAns[subI] = tmp;
            ans[ansI] = 0;
            subI++;
            tmp = (ans + ansI+1);
        }
    }
    // handle last case
    subAns[subI] = tmp;
    subI++;
    return subI;
}

int main(void) {
    srand(time(0));
    char*** qp = malloc(sizeof(char**));
    char*** ap = malloc(sizeof(char**));
    int nQ = getQnA(qp, ap);

    if (nQ < 0) {
        fprintf(stderr, "%sgetQnA() failed%s\n", KRED, KNRM);
        return 1;
    }

    char** questions = *qp;
    char** answers = *ap;
    free(ap); free(qp); ap = 0; qp = 0;

    unsigned short score = 0; // make sure no overflow happens though
    char* in = malloc(200); // declared outside so no realloc for no reason
    // loop through the questions (main loop)
    for (int i=0; i<nQ; i++) {
        char* ques = questions[i];
        char* ans = answers[i];

        // todo seperate answers with , and ask questions/input
        fprintf(stdout, "%s%s\n", KCYN, ques);
        char** subAns = malloc(sizeof(char*) * nQ);
        int nSubAns = getSubAns(ans, subAns);

        int* randInd = malloc(sizeof(nSubAns)); // contains the corresponding indexes subAns[randInd[i]] to decode

        // fill array and shuffle it
        for (int i=0; i<nSubAns; i++) randInd[i] = i;
        shuffle(randInd, nSubAns);

        for (int i=0; i<nSubAns; i++) {
            printf("\t%s%c: %s\n", KYEL, 'a'+i, subAns[randInd[i]]);
        }
        printf("\t%sYour answer: ", KCYN); // cyan fn

        memset(in, 0, 200); // zero it out to avoid mem leaks
        if (fgets(in, 200, stdin) == NULL) {
            fprintf(stderr, "%sFailed to read from stdin.\n%s", KRED, KNRM);
            exit(1);
        }
        in[strlen(in) - 1] = 0; // remove \n
    
        if (in == NULL) {
            fprintf(stderr, "%sError reading from stdin\n%s", KRED, KNRM);
            exit(1);
        }
        
        unsigned char success = 0; // bool (i know char is already unsiged, just making clear)
        int index = 0;
        while (!success) {
            // delete and compare to abc, not the acc answer lol
            if (strlen(in) > 1) {
                invalid:
                printf("%sInvalid input, please enter only [a,b,c...] or [1,2,3...]\n%s", KRED, KNRM);
                continue;
            }
            char c = in[0];
            c = (char)tolower(c);
            if (!isalnum(c) || c == '0') {
                goto invalid;
            }

            if (isalpha(c)) {
                index = c - 'a';
            } else {
                index = c - '1';
            }

            if (index > nSubAns) {
                printf("answer is out of range\nAcceptable range is [a-%c] or [1-%d]", 'a'+nSubAns-1, nSubAns);
                goto invalid;
            }
            success = 1;
        }
        success = 0;
        // zero is the index of the correct answer
        // think of randInd as a function that gets the corresponding index for the shuffled one
        if (randInd[index] == 0) {
            // handle buffer overflow since were using uint16, unless a bug happens or 2^16 questions are available... XD
            if (++score == 0) {
                fprintf(stderr, "%sScore counter overflow detected. Shutting down...%s\n", KRED, KNRM);
                exit(1);
            }

            printf("%sCorrect!\tscore +1\n\tCurrent score:%d/%d%s\n", KGRN, score, nQ, KNRM);
        } else {
            printf("%sIncorrect.\n\tCurrent score:%d/%d\n%s", KRED, score, nQ, KNRM);
        }
        
        // no memory leaks!!
        if (subAns != NULL) {
            free(subAns);
            subAns = NULL;
        }
        if (randInd != NULL) {
            //free(randInd);
            randInd = NULL;
        }
    }
    float grade = (float)score/(float)nQ;
    grade *= 100.f;
    printf("%sFINAL SCORE: %d/%d which is %.2f%%\n", KNRM, score, nQ, grade);
    
    printf("%s", KGRN);
    if (grade > 95.f) {
        printf("ACED IT!\n");
    } else if (grade > 80.f) {
        printf("Good Job!\n");
    } else if (grade > 60.f) {
        printf("%sYou should study a bit more..\n", KYEL);
    } else if (grade > 40.f) {
        printf("%sFailed. Go study.\n", KRED);
    } else {
        printf("%sYOU'RE COOKED BRO 😭😭😭\n", KRED);
    }
    printf("%s", KNRM);
    return 0;
}