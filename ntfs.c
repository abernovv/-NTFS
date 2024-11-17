#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdint.h>
#include <string.h>
#include <sys/stat.h>
       #include <sys/types.h>


uint64_t cluster_size;
int fd;
uint8_t *cluster; 
uint8_t n,s;
char File_name[256];


void name_and_sectors(uint64_t move_start){
	uint32_t offset;
	uint32_t len ;
	if(n==1) len = *((uint8_t*)(cluster+move_start+1));
	if(n==2) len = *((uint16_t*)(cluster+move_start+1));
	if(n==4) len = *((uint32_t*)(cluster+move_start+1));
	if(s==1) offset = *((uint8_t*)(cluster+move_start+1+n));
	if(s==2) offset = *((uint16_t*)(cluster+move_start+1+n));
	if(s==4) offset = *((uint32_t*)(cluster+move_start+1+n));
	printf("		len = %x  offset = %x\n",len ,offset);

	if (File_name[0]>=97 && File_name[0]<=122) {
		FILE *fp=fopen(File_name,"a+");//создает файл
		fprintf(fp,"hi");
		fclose(fp);//закрыть файл
		memset(File_name,0,255);
		}				
}


void resident(uint64_t nomer_struct){

}

void non_residents(uint64_t nomer_struct){
	uint64_t offset_series_list = *((uint64_t*)(cluster+0x20+nomer_struct));
	//printf("	offset_series_list: %x\n",offset_series_list);
	uint8_t size_file =0;int next=0;
	do{
		size_file = *((uint8_t*)(cluster+offset_series_list+nomer_struct+next));if(size_file==0)break;
		//printf("size_file %x\n",size_file);
		n=size_file&0x0f;
		s=size_file>>4;
		//printf("	len = %x  offset = %x %x %x\n",n ,s,*((uint8_t*)(cluster+offset_series_list+nomer_struct+next+1)),*((uint8_t*)(cluster+offset_series_list+nomer_struct+next+1+n))  );
		name_and_sectors(offset_series_list+nomer_struct+next);
		next+=n+s+1;
	}while(size_file!=0);

}

void structure(uint64_t nomer_struct,int i){
	uint16_t attribute=*((uint16_t*)(cluster+nomer_struct)),
	len=*((uint8_t*)(cluster+0x4+nomer_struct));
	if(attribute==0xffff || len ==0)  return;
	uint8_t flag=*((uint8_t*)(cluster+0x8+nomer_struct)),
	name_len=*((uint8_t*)(cluster+0x9+nomer_struct));
	//printf("attribute: %x   len: %x   flag : %x     name_len :%x\n",attribute,len,flag,name_len);
	if(attribute==0x80) flag ? non_residents(nomer_struct) : resident(nomer_struct);
	if(attribute==0x30 && flag==0){
		for(int i=0;i<len-90-1;i++){
			File_name[i]=*((uint16_t*)(cluster+nomer_struct+90+i*2)); if(File_name[i]==0) break;
		}
	}	
	if(attribute==0x90){
		if (File_name[0]>=97 && File_name[0]<=122) {
			int p =  mkdir(File_name,S_IRWXU);
			memset(File_name,0,255);
			}	
	}
	
	if(i==0)len-=16;
		structure(nomer_struct+(uint64_t)len,1);
}

void open_image(char *name){
	if((-1) == (fd = open(name,O_RDONLY)) ){
		perror(__func__);
		exit(-1);
	}
	uint16_t sector_size;
	uint8_t cluster_per_sector;
	uint8_t sector[512];	
	if(512 != read(fd,sector,512)){
		exit(-1);
	}
	cluster_per_sector = sector[0xd];
	//printf("sector %x\n",cluster_per_sector);
	sector_size = *((uint16_t*)(sector + 0xb));
	//printf("sector_size %x\n",sector_size);
	cluster_size = (( uint64_t)cluster_per_sector)*((uint64_t)sector_size);
	//printf("cluster_size %x\n",cluster_size);
	if(NULL == (cluster = malloc(cluster_size))){
	exit(-1);
	}		
}


void get_cluster(uint64_t num){

	uint64_t offset = num * cluster_size;
	if(offset !=lseek(fd,offset,SEEK_SET)){
		perror(__func__);
		exit(-1);
	}
	
	if(cluster_size != read(fd,cluster,cluster_size)){
	perror(__func__);
	exit(-1);
	}
}


void __attribute__((destructor)) gc(void){
	if(fd>0)   close(fd);
	if(NULL!=cluster)  free(cluster);
}


int main(int argc,char* argv[])
{
	open_image(argv[1]);
	get_cluster(0);
	//write(1,cluster,512);
	uint8_t first_claster= *((uint8_t*)cluster+0x30);
	uint8_t record_size_MFT = *((uint8_t*)cluster+0x40);
	int i;
	for( i=first_claster;;i++){
		printf("nomer fail %i\n",i);
		get_cluster(i);
		//write(1,cluster,512*record_size_MFT);
		if(*((uint32_t*)cluster)!=0x454c4946 )break;
		n=0;s=0;
		structure(16,0);
		
	}
	close(fd);
	return 0;
}