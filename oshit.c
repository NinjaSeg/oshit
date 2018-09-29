// given a known offset, search for where an overlapping partition move failed
// 64bit required for partitions larger than ~3gb

// gcc -O2 oshit.c -o oshit
// ./oshit /dev/sdXX <offset in bytes>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

static size_t BS=4096; // block size

int main(int argc,char** argv){
	if(argc<=2){
		printf("usage: ./oshit /dev/sdXX <offset in bytes>\n");
		return -1;
	}
	size_t dist=atol(argv[2]); // search distance in bytes
	assert(dist>0);

	int fd=open(argv[1],O_RDONLY,0);
	if(fd==-1){
		perror("open");
		return -1;
	}

	size_t size=0;
	struct stat st;
	fstat(fd,&st);
	if((st.st_mode&S_IFMT)==S_IFBLK){
		if(ioctl(fd,BLKGETSIZE64,&size)==-1){
			perror("ioctl");
			return -1;
		}
	}else{
		size=st.st_size;
	}
	assert(size>dist);

	uint8_t* part=mmap(NULL,size,PROT_READ,
			MAP_PRIVATE | MAP_NORESERVE
			,fd,0);
	if(part==MAP_FAILED){
		perror ("mmap");
		return -1;
	}

	printf("using block size %zu\n",BS);
	printf("partition size %zu blocks\n",size/BS);
	printf("search distance %zu bytes\n",dist);

	size_t run=0;
	for(size_t block=0;block<size/BS-dist/BS;++block){
		if(block%1000L==0){
			printf("%zu",block);
			fflush(stdout);
			printf("   \r");
		}
		if(!memcmp(&part[block*BS],&part[block*BS+dist],BS)){
			if(run==0){
				printf("block %zu match\n",block);
				//for(size_t i=0;i<BS;++i){
				//	printf("%02X ",part[i]);
				//}
				//printf("\n");
			}
			++run;
		}else if(run>0){
			printf("run %zu   \n",run);
			run=0;
		}
	}
	printf("\ndone.\n");

	int rc=munmap(part,size);
	assert(rc==0);
	close(fd);
	return 0;
}
