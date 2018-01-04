#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PAGE_SIZE     256
#define PAGE_ENTRIES  256
#define PAGE_NUM_BITS 8 
#define FRAME_SIZE    256 
#define FRAME_ENTRIES 256
#define MEM_SIZE      (FRAME_SIZE * FRAME_ENTRIES)
#define TLB_ENTRIES   16 
#define OFFSET_MASK   0xFF

typedef struct {
	unsigned int page;
	unsigned int frame;
} TLB_Data;

typedef struct  {
	unsigned int frame;
	bool valid;
} Page_Table;
//
TLB_Data TLB[TLB_ENTRIES];
Page_Table page_table[PAGE_ENTRIES];
int physicalMemory[MEM_SIZE];
int tlb_size = TLB_ENTRIES;
int tlb_pointer = 0;
// records
int page_fault_count = 0;
int tlb_hit_count = 0;
int address_count = 0;

int main(int argc, char const *argv[])
{
	int virt_addr, offset, page_number;
	int phys_addr = 0;
	char buff[256];
	bool hit;
	int frame = 0;
	int i;
	int content;
	double tlbHitRate;
	double pageFaultRate;
	// open files
	FILE *f   = fopen(argv[1], "r");  // request address list
	// FILE *log = fopen(argv[2], "w");  // output file
	// FILE *bks = fopen(argv[3], "rb"); // backing store file
	FILE *bks = fopen(argv[2], "rb"); // backing store file
	FILE *v_log = fopen("0310128_value.txt", "w"); // backing store file
	FILE *a_log = fopen("0310128_address.txt", "w"); // backing store file
	//
	// initialize
	for (i = 0; i < PAGE_ENTRIES; ++i) {
		page_table[i].valid = false;
	}
	// accept request
	while (fscanf(f, "%s", buff) != EOF) {
		// new request
		virt_addr = atoi(buff);
		++address_count;
		// parse virtual address
		// virtual address = [page number] [offset]
		page_number = virt_addr >> PAGE_NUM_BITS; 
		offset      = virt_addr & OFFSET_MASK; 
		// Search on TLB
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
			// printf("TLB Hit %*d\n", 3, tlb_hit_count);
			++tlb_hit_count;
		}
		// TLB miss + page fault
		else if (page_table[page_number].valid == false) {
			// printf("[PageFault] on request page %*d\n", 3, page_number);
			// load frame from backing store
			fseek(bks, page_number * PAGE_SIZE, SEEK_SET);
			fread(buff, sizeof(char), PAGE_SIZE, bks);
			// update entry in page table
			page_table[page_number].frame = frame;
			page_table[page_number].valid = true;
			// save the data to physical memory
			for (i = 0; i < FRAME_SIZE; ++i) {
				physicalMemory[frame * FRAME_SIZE+ i] = buff[i];
			}
			//
			++page_fault_count;
			++frame;
			// update FIFO
			printf("Filled entry in TLB #%*d\n", 2, tlb_pointer);
			TLB[tlb_pointer].page = page_number;
			TLB[tlb_pointer].frame = page_table[page_number].frame;
			// update TLB using FIFO
			tlb_pointer = (tlb_pointer + 1) % TLB_ENTRIES;
			// translate
			phys_addr = page_table[page_number].frame * PAGE_SIZE + offset;
		}
		// only TLB miss
		else {
			// printf("Page Table HIT\n");
			// translate using page table
			phys_addr = page_table[page_number].frame * PAGE_SIZE + offset;
			// update FIFO
			printf("Filled entry in TLB #%*d\n", 2, tlb_pointer);
			TLB[tlb_pointer].page = page_number;
			TLB[tlb_pointer].frame = page_table[page_number].frame;
			// update TLB using FIFO
			tlb_pointer = (tlb_pointer + 1) % TLB_ENTRIES;
		}
		//
		content = physicalMemory[phys_addr];
		// 
		//printf("Virtual Address: %*d | ", 5, virt_addr);
		//printf("Page Number: %*d | Offset: %*d | ", 3, page_number, 3, offset);
		//printf("Physical Address: %*d | ", 5, phys_addr);
		//printf("Value: %*d\n", 5, content);
		/*/ write to file
		fprintf(log, "Virtual Address: %*d | ", 5, virt_addr);
		fprintf(log, "Physical Address: %*d | ", 5, phys_addr);
		fprintf(log, "Value: %*d\n", 5, content);
		*/
		fprintf(v_log, "%d\n", content);
		fprintf(a_log, "%d\n", phys_addr);
	}
	fclose(f);
	fclose(bks);
	fclose(v_log);
	fclose(a_log);
	//
	tlbHitRate    = 100.0 * tlb_hit_count / address_count;
	pageFaultRate = 100.0 * page_fault_count / address_count;
	/*/ summary
	fprintf(log, "\n");
	fprintf(log, "Total address translated: %*d\n", 5, address_count);
	fprintf(log, "Total TLB Hit count     : %*d\n", 5, tlb_hit_count);
	fprintf(log, "Total Page Fault count  : %*d\n", 5, page_fault_count);
	fprintf(log, "TLB Hit rate   : %.2f%%\n", tlbHitRate);
	fprintf(log, "Page Fault rate: %.2f%%\n", pageFaultRate);
	fclose(log);
	*/
	printf("\n");
	printf("Total address translated: %*d\n", 5, address_count);
	printf("Total TLB Hit count     : %*d\n", 5, tlb_hit_count);
	printf("Total Page Fault count  : %*d\n", 5, page_fault_count);
	printf("TLB Hit rate   : %.2f%%\n", tlbHitRate);
	printf("Page Fault rate: %.2f%%\n", pageFaultRate);
	return 0;
}