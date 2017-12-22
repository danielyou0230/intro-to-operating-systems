#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
//#include <types.h>

/* Define struct to pass parameters*/
typedef struct 
{
	int row;
	int column;
} parameters;
//
void *check_row(void* param);
void *check_col(void* param);
void *check_block(void* param);
//
int arr[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
int table[9][9];
int valid = 1;
//
int main(int argc, char const *argv[])
{
	//
	int x, y, i;
	char c;
	//parameters *param = (parameters *) malloc(sizeof(parameters));
	parameters param[27];
	// open sudoku file
	FILE *f = fopen(argv[1], "r");
	for (x = 0; x < 9; ++x){
		for (y = 0; y < 9; ++y){
			c = fgetc(f);
			while (c == '\r' || c == '\n')
				c = fgetc(f);
			table[x][y] = c - '0';
		}
	}
	fclose(f);
	for (y = 0; y < 9; ++y) {
		printf("%d %d %d %d %d %d %d %d %d\n", \
			   table[0][y], table[1][y], table[2][y], \
			   table[3][y], table[4][y], table[5][y], \
			   table[6][y], table[7][y], table[8][y]);
	}
	//
	pthread_t tid[27];
	for (i = 0; i < 9; ++i){
		param[i * 3 + 0].row = i;
		param[i * 3 + 1].column = i;
		param[i * 3 + 2].row = (i / 3) * 3;
		param[i * 3 + 2].column = (i % 3) * 3;
		//
		pthread_create(&tid[i * 3 + 0], NULL, check_row, &param[i * 3 + 0]);
		pthread_create(&tid[i * 3 + 1], NULL, check_col, &param[i * 3 + 1]);
		pthread_create(&tid[i * 3 + 2], NULL, check_block, &param[i * 3 + 2]);
	}
	for (i = 0; i < 27; ++i){
		pthread_join(tid[i], NULL);
	}
	
	printf("valid: %d\n", valid);
	return 0;
}

void *check_row(void* param){
	int itr, i;
	int chk[9] = {0};
	int current_idx;
	//
	parameters *p = (parameters *) param;
	int row = p->row;
	for (itr = 0; itr < 9; ++itr){
		current_idx = table[row][itr] - 1;
		//printf("[%d] current = %d\n", itr, current_idx);
		if (chk[current_idx]){
			printf("[ROW] row = %d col = %d, current_idx = %d\n", row, itr, current_idx);
			valid = 0;
			break;//pthread_exit(0);
		}
		else{
			chk[current_idx] = 1;
		}
	}
	pthread_exit(0);
}

void *check_col(void* param){
	int itr, i;
	int chk[9] = {0};
	int current_idx;
	//
	parameters *p = (parameters *) param;
	int col = p->column;
	for (itr = 0; itr < 9; ++itr){
		current_idx = table[itr][col] - 1;
		//printf("[%d] current = %d\n", itr, current_idx);
		if (chk[current_idx]){
			printf("[COL] row = %d col = %d, current_idx = %d\n", itr, col, current_idx);
			valid = 0;
			break;//pthread_exit(0);
		}
		else
			chk[current_idx] = 1;
	}
	pthread_exit(0);
}

void *check_block(void* param){
	int itr_row, itr_col, i;
	int chk[9] = {0};
	int current_idx;
	//
	parameters *p = (parameters *) param;
	int row = p->row;
	int col = p->column;
	for (itr_row = row; itr_row < row + 3; ++itr_row){
		for (itr_col = col; itr_col < col + 3; ++itr_col){
			current_idx = table[itr_row][itr_col] - 1;
			if (chk[current_idx]){
				valid = 0;
				goto EXIT; //pthread_exit(0);
			}
			else
				chk[current_idx] = 1;
		}
	}

EXIT:
	pthread_exit(0);
}