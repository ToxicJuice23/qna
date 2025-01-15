#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define NL ';' /*newline/seperator*/
#define freef(x) if(x)free(x)
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define ANS_SEP '?' // ? will be used in answers.txt to seperate values, questions arent answers lol

long getFileLength(FILE* fp) {
    long n;
    fseek(fp, 0L, SEEK_END);
    n = ftell(fp);
    rewind(fp);
    return n;
}

int getQnA(char*** qp, char*** ap) {
    FILE *ansF = NULL, *quesF = NULL;
    char *ansBuf = NULL, *quesBuf = NULL, **answers = NULL, **questions = NULL;
    long ansFL=0, quesFL=0;

    ansF = fopen("answers.txt", "r");
    quesF = fopen("questions.txt", "r");

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
        fprintf(stderr, "Unable to read from files or File could be empty\nans_n: %d, ques_n: %d\n", na, nq);
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
        if (i < ansFL && ansBuf[i] == NL) ansNL++;
        if (i < quesFL && quesBuf[i] == NL) quesNL++;
    }
    
    if (ansNL != quesNL) {
        fprintf(stderr, "Some questions might not have answers to them or vice-versa.\n");
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
            if (i < ansFL && ansBuf[i] == NL) {
                // reached new line in answers.txt
                answers[ansIndex] = (ansBuf + tmp1); // tmp1 is the offset
                ansBuf[i] = 0;
                tmp1 = i+1;
                ansIndex++;
            }
            if (i < quesFL && quesBuf[i] == NL) {
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

int getSubAns(char* ans, char** subAns) {
    int subI = 0;
    char* tmp = ans;
    int l = strlen(ans);
    for (int ansI=0; ansI<l; ansI++) {
        if (ans[ansI] == ANS_SEP) {
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

int main(int argc, char** argv) {
    char*** qp = malloc(sizeof(char**));
    char*** ap = malloc(sizeof(char**));
    int nQ = getQnA(qp, ap);

    if (nQ < 0) {
        return 1;
    }

    char** questions = *qp;
    char** answers = *ap;
    free(ap); free(qp); ap = 0; qp = 0;

    for (int i=0; i<nQ; i++) {
        char* ques = questions[i];
        char* ans = answers[i];
        // todo seperate answers with , and ask questions/input
        fprintf(stdout, "%s%s\n", KCYN, ques);
        char** subAns = malloc(sizeof(char*) * nQ);
        int nSubAns = getSubAns(ans, subAns);
        for (int i=0; i<nSubAns; i++) {
            printf("\t%sAnswer #%d: %s\n", KGRN, i+1, subAns[i]);
        }
    }

    free(answers); free(questions);
    return 0;
}