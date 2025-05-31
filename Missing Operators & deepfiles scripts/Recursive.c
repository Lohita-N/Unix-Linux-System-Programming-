#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_VALUES 100

#define NUM_OPERATIONS 5
char operations[] = {'+', '-', '*', '/', '%'};

//void find_expressions(char expr[], int index, int result);
//      printf("Current expression: %s | Result: %d | Index: %d\n", expr, result, index);
/*      if(index == numValues - 1){
                if(result == values[numValues - 1]){
                        printf("%s = %d\n", expr, result);
                        foundSolution = true;
                }
                return;
        }
*/
int compute(int value1, char oper, int value2) {
        switch (oper) {
                case '+': return value1 + value2;
                case '-': return value1 - value2;
                case '*': return value1 * value2;
                case '/': return (value2 != 0) ? value1 / value2 : 0;
                case '%': return (value2 != 0) ? value1 % value2 : 0;
        }
        return 0;
}
int numValues = 0;
int values[MAX_VALUES];
bool foundSolution = false;

void find_expressions(char expr[], int index, int result, int target) {
        if(index == numValues) {
                if(result == target){
        //      if(result == values[numValues - 1]){
                        printf("%s = %d\n", expr, result);
                        foundSolution = true;
                }
                return;
        }

        char tempExpr[256];
        for(int i = 0; i < NUM_OPERATIONS; i++) {
                if((operations[i] == '/' || operations[i] == '%') && values[index] == 0){
                        continue;
                }

        //      int newResult = compute(result, operations[i], values[index + 1]);

        //      if((operations[i] == '+' || operations[i] == '-') && newResult > values[numValues - 1]){
        //              continue;
        //      }

           snprintf(tempExpr, sizeof(tempExpr), "%s %c %d", expr, operations[i], values[index]);
           find_expressions(tempExpr, index + 1, compute(result, operations[i], values[index]),target);
                }

        }
int main(int argc, char *argv[]){
        if(argc < 3){
                printf("Usage: %s <num1> <num2> ...<numN> <result>\n", argv[0]);
                return 1;
        }
        numValues = argc - 2;
        for(int i = 0; i < numValues; i++){
                values[i] = atoi(argv[i + 1]);
        }
        int target = atoi(argv[argc - 1]);
//      int result = values[numValues - 1];
//      values[numValues - 1] = 0;

        char expr[256];
        snprintf(expr, sizeof(expr), "%d", values[0]);
        find_expressions(expr, 1, values[0], target);

        if(!foundSolution){
                printf("No solutions!\n");
        }
        return 0;
}
   
