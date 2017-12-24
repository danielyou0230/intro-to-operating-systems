#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

#define PAGE_SIZE 256
#define PAGE_ENTRIES 256
#define PAGE_NUM_BITS 8 
#define FRAME_SIZE 256 
#define FRAME_ENTRIES 256
#define MEM_SIZE (FRAME_SIZE * FRAME_ENTRIES)
#define TLB_ENTRIES 16 

typedef struct {
	unsigned int page;
	unsigned int frame;
} TLB_Data;

int tlb_size = TLB_ENTRIES;

typedef struct  {
	unsigned int frame;
	bool valid;
} Page_Table;

TLB_Data TLB[TLB_ENTRIES];
// int page_table[PAGE_ENTRIES];
Page_Table page_table[PAGE_ENTRIES];
char memory[MEM_SIZE];
int mem_index = 0;
int physicalMemory[MEM_SIZE];
// records
int page_fault_count = 0;
int tlb_hit_count = 0;
int address_count = 0;

int main(int argc, char const *argv[])
{
	char* addr_file;
	char* outfile;
	char* store_file;
	char* store_data;
	char buff[256];
	int virt_addr, offset, page_number;
	int page_addr = 0;
	int phys_addr = 0;
	int store_fd;
	int hit;
	int frame = 0;
	int i, j, k;
	int tlb_index;
	int content;
	double tlbHitRate;
	double pageFaultRate;
	//
	FILE *f = fopen(argv[1], "r");
	FILE *log = fopen(argv[2], "w");
	FILE *bks = fopen(argv[3], "rb");
	// initialize
	// memset(page_table, -1, PAGE_SIZE * sizeof(int));
	for (i = 0; i < PAGE_ENTRIES; ++i) {
		page_table[i].valid = false;
	}
	//
	while (fscanf(f, "%s", buff) != EOF) {
		virt_addr = atoi(buff);
		++address_count;
		page_number = virt_addr >> PAGE_NUM_BITS;
		offset      = virt_addr & 0xFF;
		// Search for TLB
		hit = false;
		for (i = 0; i < TLB_ENTRIES; ++i) {
			if (TLB[i].page == page_number) {
				phys_addr = TLB[i].frame * FRAME_SIZE + offset;
				hit = true;
				break;
			}
		}
		// TLB hit
		if (hit) {
			printf("TLB HIT\n");
			++tlb_hit_count;
		}
		// TLB miss + page fault
		else if (page_table[page_number].valid == false) {
			fseek(bks, page_number * PAGE_SIZE, SEEK_SET);
			fread(buff, sizeof(char), 256, bks);
			// update entry in page table
			page_table[page_number].frame = frame;
			page_table[page_number].valid == true;
			//
			for (i = 0; i < FRAME_SIZE; ++i) {
				physicalMemory[frame * FRAME_SIZE+ i] = buff[i];
			}
			//
			++page_fault_count;
			++frame;
			// update TLB
			if (tlb_size == TLB_ENTRIES) // full
				--tlb_size;
			// FIFO TLB
			for (tlb_index = tlb_size; tlb_index > 0; --tlb_index) {
				TLB[tlb_index].page = TLB[tlb_index - 1].page;
				TLB[tlb_index].frame = TLB[tlb_index - 1].frame;
			}
			// add entry to TLB
			TLB[0].page = page_number;
			TLB[0].frame = page_table[page_number].frame;
			//
			if (tlb_size <= TLB_ENTRIES - 1)
				++tlb_size;
			//
			phys_addr = page_table[page_number].frame * PAGE_SIZE + offset;
		}
		// only TLB miss
		else {
			printf("page_table HIT\n");
			phys_addr = page_table[page_number].frame * PAGE_SIZE + offset;
		}
		//
		content = physicalMemory[phys_addr];
		// write to file
		printf("Virtual Address: %*d | ", 5, virt_addr);
		printf("Page Number: %*d | Offset: %*d | ", 3, page_number, 3, offset);
		printf("Physical Address: %*d | ", 5, phys_addr);
		printf("Value: %*d\n", 3, content);
	}
	fclose(f);
	fclose(bks);
	//
	tlbHitRate    = 100.0 * tlb_hit_count / address_count;
	pageFaultRate = 100.0 * page_fault_count / address_count;
	// summary
	fprintf(log, "Total address translated: %*d\n", 5, address_count);
	fprintf(log, "Total TLB Hit count     : %*d\n", 5, tlb_hit_count);
	fprintf(log, "Total Page Fault count  : %*d\n", 5, page_fault_count);
	fprintf(log, "TLB Hit rate   : %f%%\n", tlbHitRate);
	fprintf(log, "Page Fault rate: %f%%\n", pageFaultRate);
	fclose(log);
	return 0;
}