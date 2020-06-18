#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/highmem.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "MD5.h"

#define PAGE_SIZE 4096	//每页大小
#define PAGEMAP_ENTRY 8	// /proc/pid/pagemap文件内的每一项的字节长度
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y   //返回位数组中,指定位的值,X:位数组，Y:位置
#define GET_PFN(X) (X & 0x7FFFFFFFFFFFFF)	//从pagemap的项中获取页框号(后面要加括号)
const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )	//判断是否为大端存储

int pid = -1;
/* operator:
 * 1:test time.
 * 2:set random maxtime & mintime
*/
int operator = 1;
int maxtime=1,mintime=10;	//用来标记随机时间间隔的最大值和最小值
unsigned long virstart,virend;	//进程代码段的虚拟地址的开始和结尾(左闭右开区间)
static char *filepath;	//可执行文件所在目录
unsigned long elftextstart=0,elftextend;	//elf文件中text段的开始和结尾地址
unsigned char elftexthash[200005][16];	//用来储存elf文件中text段的分页hash值

//用来声明模块的参数
module_param(pid,int,S_IRUGO);
module_param(operator,int,S_IRUGO);
module_param(maxtime,int,S_IRUGO);
module_param(mintime,int,S_IRUGO);
module_param(virstart,ulong,S_IRUGO);
module_param(virend,ulong,S_IRUGO);
module_param(filepath,charp,S_IRUGO);

MD5_CTX md5;	//用来计算hash值
mm_segment_t old_fs;

//用来获取ELF文件代码段的分页hash值，将结果储存到elftexthash[][16]中
void read_elf_text_hash(){
	int i,j,k;	//用于for循环中的临时变量
	unsigned char buf[PAGE_SIZE];	//储存elf代码段每页的数据
	struct file *elf_file=NULL;	//打开elf的文件指针
	loff_t pos;
	ssize_t readsize;
	int elf_text_page_num=0;

	elftextstart=0;
	elftextend=virend-virstart;		//通过虚拟地址的范围获取elf文件代码段的长度。
	
	//打开文件
	if(elf_file==NULL){
		elf_file=filp_open(filepath,O_RDONLY,0644);
	}
	//错误判断
	if (IS_ERR(elf_file)) {
		printk("opening file %s error.\n",filepath);
		return -1;
	}
	//储存旧的设置，将来再恢复
	old_fs=get_fs();
	set_fs(KERNEL_DS);
	pos=0;
	readsize=0;
	printk("elftextstart:%013x,elftextend:%013x\n",elftextstart,elftextend);
	while(1){
		if(pos>=elftextend) break;
		printk("pos:%013x ",pos);
		readsize=vfs_read(elf_file,buf,PAGE_SIZE,&pos);
		if(readsize!=PAGE_SIZE){
			buf[readsize]='\0';
		}
		printk(" elf_content:%02X~%02X ",buf[0],buf[4095]);
		/*
		for(k=0;k<readsize;k++){
			printk("%02X ",buf[k]);
			if((k+1)%16==0){
				printk("\n");
			
			}
			if((k+1)%64==0) break;
		}
		*/
		printk(" elftexthash:");
		//度量buf中的数据，将hash储存到elf_text_page_num。
		MD5Init(&md5);         		
		MD5Update(&md5,buf,readsize);
		MD5Final(&md5,elftexthash[elf_text_page_num]);   
		for(j=0;j<16;j++){
			printk("%02X",elftexthash[elf_text_page_num][j]);
		}
		printk("\n");
		elf_text_page_num++;
		//pos+=0x1000;
	}
	printk("elf_text_page_num: %d \n",elf_text_page_num);
	/*
	for(i=0;i<elf_text_page_num;i++){
		for(j=0;j<16;j++){
			printk("%02X",elftexthash[i][j]);
		}
		printk("\n");
	}
	*/
	printk(KERN_INFO "pid=%d\n",pid);
	set_fs(old_fs);
	filp_close(elf_file,NULL);

}

//度量物理地址physical_page_addr处，4096字节长的数据
void measurement_page(unsigned long *physical_page_addr,size_t curr_num){
	int i;
	struct page *pp;
	void *from;
	int page_number,page_indent;
	unsigned char process_page_hash[16];
	unsigned char page_buf[4096];
	size_t count=4096;
	loff_t mem_size;

	mem_size = (loff_t)get_num_physpages()<< PAGE_SHIFT;
	if ( * physical_page_addr >= mem_size ) return 0;

	page_number = *physical_page_addr / PAGE_SIZE;
	page_indent = *physical_page_addr % PAGE_SIZE;
#if 1
	pp = pfn_to_page( page_number);
#else
	pp = &mem_map[ page_number ];
#endif
	//建立持久映射来读取物理地址
	from = kmap( pp ) + page_indent;
	if ( page_indent + count > PAGE_SIZE ) count = PAGE_SIZE - page_indent;
	
	//copy_to_user(page_buf,from,count);
	MD5Init(&md5);         		
	MD5Update(&md5,from,count);
	MD5Final(&md5, process_page_hash);   
	printk(" %02X~%02X ",*(unsigned char*)from,*( ((unsigned char*)from)+4095 ));
	//printk(" %02X~%02X ",page_buf[0],page_buf[4095]);
	printk("                              process_page_hash: ");
	for(i=0;i<16;i++){
		printk("%02X",process_page_hash[i]);
	}
	printk(" curr_num:%d ",curr_num);
	if(0==strncmp(process_page_hash,elftexthash[curr_num],16)){
		printk(" page hash right\n");
	}else{
		printk(" page hash ERROR!!!\n");
	}	
	kunmap(pp);
}

//读取/proc/pidpagemap文件，对已分配的页调用measurement_page()进行度量，与之前存储的hash进行对比
void read_pagemap_and_measurement_process(){
	int i,j,k;
	char path_buf [0x100] = {};
	struct file *f_pagemap=NULL;
	loff_t start_offset,curr_offset,end_offset;
	unsigned long curr_vir_addr;
	unsigned char c_buf[PAGEMAP_ENTRY];
	unsigned char tmp;
	ssize_t readsize=0;
	unsigned long read_val;
	unsigned long physical_page_addr=NULL;
	unsigned long pfn;

	sprintf(path_buf,"/proc/%u/pagemap",pid);
	
	if(f_pagemap==NULL){
		f_pagemap=filp_open(path_buf,O_RDONLY,0644);
	}
	if (IS_ERR(f_pagemap)) {
		printk("opening file %s error.\n",f_pagemap);
		return -1;
	}
	old_fs=get_fs();
	set_fs(KERNEL_DS);
	start_offset=virstart/PAGE_SIZE*PAGEMAP_ENTRY;
	curr_offset=virstart/PAGE_SIZE*PAGEMAP_ENTRY;
	end_offset=virend/PAGE_SIZE*PAGEMAP_ENTRY;
	curr_vir_addr=virstart;

	while(1){
		if(curr_offset>=end_offset) break;
		readsize=vfs_read(f_pagemap,c_buf,PAGEMAP_ENTRY,&curr_offset);
		if(readsize!=PAGEMAP_ENTRY){
			printk("read pagemap error !i\n");
		}
		if(!is_bigendian()){
			for(i=0;i<4;i++){
				tmp=c_buf[i];
				c_buf[i]=c_buf[PAGEMAP_ENTRY-i-1];
				c_buf[PAGEMAP_ENTRY-i-1]=tmp;
			}	
		}
		for(i=0; i < PAGEMAP_ENTRY; i++){
			read_val = (read_val << 8) + c_buf[i];
		}
		printk("curr_vir_addr:%013X   ",curr_vir_addr);
		printk("curr_offset:%d  ",curr_offset);
		if(GET_BIT(read_val,63)){
			//pfn=GET_PFN(read_val);
			physical_page_addr=GET_PFN(read_val)*PAGE_SIZE+curr_vir_addr%PAGE_SIZE;
			printk("pfn:%013X ",GET_PFN(read_val));
			printk("physical_page_addr:%013X",physical_page_addr);
			measurement_page(&physical_page_addr,(curr_offset-start_offset)/8-1);
			
		}else{
			if(GET_BIT(read_val,62)){
				printk("page swapped\n");
			}else{
				printk("page not present\n");
			}
		}
		curr_vir_addr+=PAGE_SIZE;
	}

	set_fs(old_fs);
	filp_close(f_pagemap,NULL);

	

}


static int __init memory_measurement_init(void) {
	read_elf_text_hash();

	read_pagemap_and_measurement_process();

	return 0;
}

static void __exit memory_measurement_exit(void) {
	printk(KERN_INFO "unload the memory_measurement module...!\n");
}

module_init(memory_measurement_init);
module_exit(memory_measurement_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YIFEI");
MODULE_DESCRIPTION("linux module");
MODULE_VERSION("0.01");
