#include "cachelab.h"
//#define _POSIX_C_SOURCE 2
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdarg.h>
#include<inttypes.h>
#include<assert.h>
#include<math.h>

enum operation_type {OP_LOAD, OP_WRITE, OP_MODIFY, OP_UNKNOWN};
typedef struct cache_line {
	uint32_t tag_bit;
	unsigned access_time_stamp;
	unsigned valid_bit : 1;
	unsigned dirty_bit : 1;
} cache_line;

typedef struct operation_info {
	enum operation_type operation_type;
	uint64_t virt_addr;
} operation_info;

typedef struct virt_addr_split_result {
	uint32_t CI, CT, CO;
} virt_addr_split_result;

int index_bits, lines_per_set, block_offset_bits, verbose;
int sets_cnt;
int hits_cnt, misses_cnt, evictions_cnt;
unsigned time_stamp; // this value is automatically incremented each round and will be assigned to the cache line that was accessed
char *trace_filename;
const char help_message[] = "\
Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\
\n\
Examples:\n\
  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n\
";
cache_line **global_cache;

void get_options(int, char *[]);
void print_usage(const char *);

void init_cache(void); // initialize a global stimulated cache
void replay_cache(void); // replay the whole emulation process, output info and update global vars

// sub functions for replay_cache()
static operation_info read_and_print_operation(void); // read an operation form stdin and print it to the screen, set operation_type to OP_UNKNOWN if any error occured
static virt_addr_split_result virt_addr_split(uint64_t); // split a 64-bit virtual address into 3 parts
static cache_line *find_desired_cache_line(cache_line *, uint64_t, enum operation_type); // find index of the cache_line which satisfy the given CT or the cache line that should be evicted;
static int printf_if_verbose(const char *, ...); // print to stdout if verbose is enabled.

void free_cache(void);

FILE *Fopen(const char *, const char *); // wrapper function for fopen()
void *Malloc(size_t); // wrapper function for malloc()
ssize_t Getline(char **, size_t *, FILE *); // wrapper function for Getline()

int main(int argc, char *argv[])
{
	get_options(argc, argv);
	init_cache();
	replay_cache();
	free_cache();
	printSummary(hits_cnt, misses_cnt, evictions_cnt);
	return 0;
}

void get_options(int argc, char *argv[])
{
	int finished = 0;
	while(!finished)
	{
		switch(getopt(argc, argv, "hvs:E:b:t:"))
		{
			case '?':
			case 'h':
				print_usage(argv[0]);
				exit(0);
			case 'v':
				verbose = 1;
				break;
			case 's':
				index_bits = atoi(optarg);
				break;
			case 'E':
				lines_per_set = atoi(optarg);
				break;
			case 'b':
				block_offset_bits = atoi(optarg);
				break;
			case 't':
				trace_filename = optarg;
				break;
			case -1:
				finished = 1;
				break;
		}
	}

	if(index_bits <= 0 || lines_per_set <= 0 || block_offset_bits <= 0)
	{
		fprintf(stderr, "%s: Missing required command line argument\n", argv[0]);
		print_usage(argv[0]);
		exit(1);
	}
	stdin = Fopen(trace_filename, "r");
}

void print_usage(const char *progname)
{
	fprintf(stderr, help_message, progname, progname, progname);
}

void init_cache(void)
{
	sets_cnt = pow(2, index_bits);
	global_cache = Malloc(sizeof(cache_line *) * sets_cnt);
	for(int i = 0; i < sets_cnt; i++)
	{
		global_cache[i] = Malloc(sizeof(cache_line) * lines_per_set);
		for(int j = 0; j < lines_per_set; j++)
			global_cache[i][j].valid_bit = 0;
	}
}

void replay_cache(void)
{
	operation_info opi;
	virt_addr_split_result vasres;
	cache_line *target_set;
	while(1)
	{
		opi = read_and_print_operation();
		if(opi.operation_type == OP_UNKNOWN)
			break;
		vasres = virt_addr_split(opi.virt_addr);
		target_set = find_desired_cache_line(global_cache[vasres.CI], vasres.CT, opi.operation_type);
		switch(opi.operation_type)
		{
			case OP_LOAD:
				if(target_set->valid_bit == 0) // an invalid cache line, simply treat it missing
				{
					printf_if_verbose(" miss\n");
					target_set->valid_bit = 1;
					target_set->dirty_bit = 0;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
				}
				else if(target_set->tag_bit == vasres.CT) // an valid cache line and tag bits match to CT
				{
					printf_if_verbose(" hit\n");
					target_set->access_time_stamp = time_stamp++;

					hits_cnt++;
				}
				else // an valid cache line but tag bits don't match, need to be evicted
				{
					printf_if_verbose(" miss eviction\n");
					target_set->dirty_bit = 0;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
					evictions_cnt++;
				}
				break;
			
			case OP_WRITE:
				if(target_set->valid_bit == 0) // an invalid cache line, simply treat it missing
				{
					printf_if_verbose(" miss\n");
					target_set->valid_bit = 1;
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
				}
				else if(target_set->tag_bit == vasres.CT) // an valid cache line and tag bits match to CT
				{
					printf_if_verbose(" hit\n");
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;

					hits_cnt++;
				}
				else // an valid cache line but tag bits don't match, need to be evicted
				{
					printf_if_verbose(" miss eviction\n");
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
					evictions_cnt++;
				}
				break;
			
			case OP_MODIFY:
				if(target_set->valid_bit == 0) // an invalid cache line, simply treat it missing
				{
					printf_if_verbose(" miss hit\n");
					target_set->valid_bit = 1;
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
					hits_cnt++;
				}
				else if(target_set->tag_bit == vasres.CT) // an valid cache line and tag bits match to CT
				{
					printf_if_verbose(" hit hit\n");
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;

					hits_cnt += 2;
				}
				else // an valid cache line but tag bits don't match, need to be evicted
				{
					printf_if_verbose(" miss eviction hit\n");
					target_set->dirty_bit = 1;
					target_set->access_time_stamp = time_stamp++;
					target_set->tag_bit = vasres.CT;

					misses_cnt++;
					evictions_cnt++;
					hits_cnt++;
				}
				break;
			
			default:
				assert(0);
		}

	}
}

static operation_info read_and_print_operation(void)
{
	operation_info ret;
	char opt_ch;
	size_t line_sz = 0;
	ssize_t lfpos = 0;
	char *read_line = NULL;
	lfpos = Getline(&read_line, &line_sz, stdin);
	if(lfpos != -1) // if getline() reaches EOF, don't strip CR
		read_line[lfpos - 1] = '\0';
	if(lfpos == -1 || sscanf(read_line, " %c %" SCNx64 ",%*x", &opt_ch, &ret.virt_addr) != 2) // throw the size away.
	{
		ret.operation_type = OP_UNKNOWN;
	}
	else
	{
		switch(opt_ch)
		{
			case 'I': ret = read_and_print_operation(); break; // for instruction operations, we skip it and read the next line
			case 'L': ret.operation_type = OP_LOAD; break;
			case 'S': ret.operation_type = OP_WRITE; break;
			case 'M': ret.operation_type = OP_MODIFY; break;
			default : ret.operation_type = OP_UNKNOWN;
		}
		printf_if_verbose("%s", read_line + 1); //skip the leading space
	}
	free(read_line);
	return ret;
}

static virt_addr_split_result virt_addr_split(uint64_t virt_addr)
{
	virt_addr_split_result ret;
	uint64_t CO_mask = (1 << block_offset_bits) - 1;
	uint64_t CI_mask = (1 << index_bits) - 1;
	uint64_t CT_mask = (1LLU << (64 - block_offset_bits - index_bits)) - 1;

	ret.CO = virt_addr & CO_mask;
	ret.CI = (virt_addr >> block_offset_bits) & CI_mask;
	ret.CT = (virt_addr >> block_offset_bits >> index_bits) & CT_mask;

	return ret;
}

static cache_line *find_desired_cache_line(cache_line *set, uint64_t tag_bits, enum operation_type opt)
{
	cache_line *ret = set;
	for(int i = 0; i < lines_per_set; i++) // search for the line that is both valid and correspondant
	{
		if(set[i].tag_bit == tag_bits && set[i].valid_bit == 1)
			return set + i;
	}
	for(int i = 0; i < lines_per_set; i++) // no such line is found, return a line that is invalid(empty)
	{
		if(set[i].valid_bit == 0)
			return set + i;
	}
	for(int i = 0; i < lines_per_set; i++) // the set is full, return the least recently used line(LRU)
	{
		if(set[i].access_time_stamp < ret->access_time_stamp)
			ret = set + i;
	}
	return ret;
}

static int printf_if_verbose(const char *fmt, ...)
{
	int ret;
	if(!verbose)
		return 0;
	va_list arg;
	va_start(arg, fmt);
	ret = vprintf(fmt, arg);
	va_end(arg);
	return ret;
}

void free_cache(void)
{
	sets_cnt = pow(2, index_bits);
	for(int i = 0; i < sets_cnt; i++)
	{
		free(global_cache[i]);
	}
	free(global_cache);
}

FILE *Fopen(const char *filename, const char *mode)
{
	FILE *ret;
	ret = fopen(filename, mode);
	if(ret == NULL)
	{
		perror(filename);
		exit(1);
	}
	return ret;
}

void *Malloc(size_t sz)
{
	void *ret = NULL;
	if(!(ret = malloc(sz)))
	{
		perror("malloc failed");
		exit(2);
	}
	return ret;
}

ssize_t Getline(char **lineptr, size_t *n, FILE *stream)
{
	ssize_t ret;
	ret = getline(lineptr, n, stream);
	if(ret == -1)
	{
		if (!feof(stream))
		{
			perror("getline failed");
			exit(2);
		}
	}
	return ret;
}