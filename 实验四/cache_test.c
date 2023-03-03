#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#define ARRAY_SIZE (1 << 30)                                    // test array size is 2^28

#define B 1
#define B_MAX (1 << 8) - 1
#define KB 1024
#define MB 1048576 
#define cacheLine 64 

// KB
const int cache_sizes[] = {1,2,4,8,16,32,64,128,256,512,1024,2048};
const int cache_sizes_length = sizeof(cache_sizes) / sizeof(int);

//B
const int block_sizes[] = {1,2,4,8,16,32,64,128,256,512};
const int block_sizes_length = sizeof(block_sizes) / sizeof(int);

const int group_nums[] = {2,4,8,16,32,64,128};
const int group_nums_length = sizeof(group_nums) / sizeof(int);

const int TLB_entries[] = {1,2,4,8,16,32,64,128,256,512};
const int TLB_entries_length = sizeof(TLB_entries) / sizeof(int);

typedef unsigned char BYTE;										// define BYTE as one-byte type

BYTE array[ARRAY_SIZE];	
const int L1_cache_size = 1 << 15;
const int L1_cache_group = 8;										// test array
const int L2_cache_size = 1 << 18;
const int L2_cache_group = 4;

double get_usec(const struct timeval tp0, const struct timeval tp1)
{
    return 1000000 * (tp1.tv_sec - tp0.tv_sec) + tp1.tv_usec - tp0.tv_usec;
}

// have an access to arrays with L2 Data Cache'size to clear the L1 cache
void Clear_L1_Cache()
{
	memset(array, 0, L2_cache_size);
}

// have an access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache()
{
	memset(array, 0, ARRAY_SIZE);
}

void Test_Cache_Size()
{
	printf("**************************************************************\n");
	printf("Cache Size Test\n");
	/**
	 * struct timeval tp[2];
	 * gettimeofday(&tp[0], NULL);
	 * func();
	 * gettimeofday(&tp[1], NULL);
	 * time_used = getusec(tp[0], tp[1]);
	 */
	// TODO
	double sum_time[cache_sizes_length];
	for(int cur = 0; cur < cache_sizes_length; cur++)
	{
		//time记录访存时间
		double time = 0;
		//固定使用的array范围为需要的数组大小
		int length = cache_sizes[cur] * KB - 1;
		struct timeval tp[2];
		//利用两次循环的时间差值来计算出纯访存时间
		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i * cacheLine) & length;
			array[x] = 0;
		}
		gettimeofday(&tp[1], NULL);
		time += get_usec(tp[0], tp[1]);

		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i * cacheLine) & length;
			x = 0;
		}
		gettimeofday(&tp[1], NULL);
		time -= get_usec(tp[0], tp[1]);

		sum_time[cur] = time;
		printf("[Test_Array_size = %4dKB]      Average access time: %7.1lfms\n",cache_sizes[cur],time / 1000);
	}
	//寻找访存时间变化最大的两个数组大小
	double time_change[cache_sizes_length];
	time_change[0] = 0;
	//计算不同数组大小访存时间变化率
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		time_change[cur] = (sum_time[cur] - sum_time[cur - 1]) / sum_time[cur - 1];
	}
	//找到访存时间变化率最大的两个数组大小
	int first = 0;
	int second = 0;
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		if(time_change[cur] > time_change[first])
		{
			second = first;
			first = cur;
		}
		else if(time_change[cur] > time_change[second])
		{
			second = cur;
		}
	}
	// 将变化最大的两个数组大小排好顺序
	if(first > second)
	{
		int temp = first;
		first = second;
		second = temp;
	}
	//变化率最大的两个点之前的数组大小即为L1_Dcache容量和L2_cache容量
	printf("L1_DCache_Size is %dKB\n",cache_sizes[first - 1]);
	printf("L2_Cache_Size is %dKB\n",cache_sizes[second - 1]);
}

void Test_L1C_Block_Size()
{
	printf("**************************************************************\n");
	printf("L1 DCache Block Size Test\n");
	Clear_L1_Cache();
	// TODO
	double sum_time[block_sizes_length];
	for(int cur = 0; cur < block_sizes_length; cur++)
	{
		double time = 0;
		struct timeval tp[2];
		//利用两次循环的时间差值来计算出纯访存时间

		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i % (ARRAY_SIZE / block_sizes[cur])) * block_sizes[cur];
			array[x] = 0;
		}
		gettimeofday(&tp[1], NULL);
		time += get_usec(tp[0], tp[1]);

		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i % (ARRAY_SIZE / block_sizes[cur])) * block_sizes[cur];
			x = 0;
		}
		gettimeofday(&tp[1], NULL);
		time -= get_usec(tp[0], tp[1]);

		sum_time[cur] = time;
		printf("[Test_Array_Jump = %5dB]      Average access time: %7.1lfms\n",block_sizes[cur],time / 1000);
	}
	//寻找访存时间开始变化较大的访问步长，即为cache块大小
	double time_change[block_sizes_length];
	time_change[0] = 0;
	int change = 0;
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		time_change[cur] = (sum_time[cur] - sum_time[cur - 1]) / sum_time[cur - 1];
		//设置找到第一个访存时间变化率大于50%的访问步长为cache块大小
		if(time_change[cur] > 0.5)
		{
			change = cur;
			break;
		}
	}
	printf("L1_DCache_Block is %dB\n",block_sizes[change]);
}

void Test_L2C_Block_Size()
{
	printf("**************************************************************\n");
	printf("L2 Cache Block Size Test\n");
	Clear_L2_Cache();											// Clear L2 Cache
	
	// TODO
	double sum_time[block_sizes_length];
	for(int cur = 0; cur < block_sizes_length; cur++)
	{
		double time = 0;
		struct timeval tp[2];
		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i % (ARRAY_SIZE / block_sizes[cur])) * block_sizes[cur];
			array[x] = 0;
		}
		gettimeofday(&tp[1], NULL);
		time += get_usec(tp[0], tp[1]);

		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < ARRAY_SIZE; i++)
		{
			int x = (i % (ARRAY_SIZE / block_sizes[cur])) * block_sizes[cur];
			x = 0;
		}
		gettimeofday(&tp[1], NULL);
		time -= get_usec(tp[0], tp[1]);

		sum_time[cur] = time;
		printf("[Test_Array_Jump = %5dB]      Average access time: %7.1lfms\n",block_sizes[cur],time / 1000);
	}
	//寻找访存时间开始变化较大的访问步长，即为cache块大小
	double time_change[block_sizes_length];
	time_change[0] = 0;
	int change = 0;
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		time_change[cur] = (sum_time[cur] - sum_time[cur - 1]) / sum_time[cur - 1];
		if(time_change[cur] > 0.5)
		{
			change = cur;
			break;
		}
	}
	printf("L2_Cache_Block is %dB\n",block_sizes[change]);
}

void Test_L1C_Way_Count()
{
	printf("**************************************************************\n");
	printf("L1 DCache Way Count Test\n");

	// TODO
	double sum_time[group_nums_length];
	for(int cur = 0; cur < group_nums_length; cur++)
	{
		struct timeval tp[2];

		Clear_L1_Cache();
		gettimeofday(&tp[0], NULL);
		for(int loop = 0; loop < ARRAY_SIZE >> 10; loop++)
		{
			//访问奇数块
			for(int i = 1; i < group_nums[cur]; i += 2)
			{
				//按照cacheLine << 2步长访问array数组2 * L1_cache_size大小部分的每个块
				for(int j = 0; j < 2 * L1_cache_size / group_nums[cur]; j += (cacheLine << 2))
				{
					array[i * (2 * L1_cache_size / group_nums[cur]) + j] = 0;
				}
			}
		}
		gettimeofday(&tp[1], NULL);

		sum_time[cur] = get_usec(tp[0], tp[1]);

		printf("[Test_Split_Groups = %5d]     Average differ time: %7.1lfms\n",group_nums[cur],sum_time[cur] / 1000);
	}
	//寻找访存时间变化最大的分块数量n，2^(n-2)即为cache相联度
	int max = 1;
	for(int cur = 1; cur < group_nums_length; cur++)
	{
		if(sum_time[cur] - sum_time[cur - 1] > sum_time[max] - sum_time[max - 1])
		{
			max = cur;
		}
	}
	printf("L1_DCache_Way_Count is %d Way\n",group_nums[max] / 4);
}

void Test_L2C_Way_Count()
{
	printf("**************************************************************\n");
	printf("L2 Cache Way Count Test\n");
	
	// TODO
	double sum_time[group_nums_length];
	for(int cur = 0; cur < group_nums_length; cur++)
	{
		struct timeval tp[2];

		Clear_L2_Cache();
		gettimeofday(&tp[0], NULL);
		for(int loop = 0; loop < ARRAY_SIZE >> 10; loop++)
		{
			//访问奇数块
			for(int i = 1; i < group_nums[cur]; i += 2)
			{
				//按照cacheLine << 2步长访问array数组2 * L1_cache_size大小部分的每个块
				for(int j = 0; j < 2 * L2_cache_size / group_nums[cur]; j += (cacheLine << 2))
				{
					array[i * (2 * L2_cache_size / group_nums[cur]) + j] = 0;
				}
			}
		}
		gettimeofday(&tp[1], NULL);

		sum_time[cur] = get_usec(tp[0], tp[1]);

		printf("[Test_Split_Groups = %5d]     Average access time: %7.1lfms\n",group_nums[cur],sum_time[cur] / 1000);
	}
	//寻找访存时间变化最大的分块数量n，2^(n-2)即为cache相联度
	int max = 1;
	for(int cur = 1; cur < group_nums_length; cur++)
	{
		if(sum_time[cur] - sum_time[cur - 1] > sum_time[max] - sum_time[max - 1])
		{
			max = cur;
		}
	}
	printf("L2_Cache_Way_Count is %d Way\n",group_nums[max] / 4);
}

void Test_Cache_Write_Policy()
{
	printf("**************************************************************\n");
	printf("Cache Write Policy Test\n");
	struct timeval tp[2];
	// TODO
	double change_time = 0;
	double lost_time = 0;
	int length = L1_cache_size - 1;
	gettimeofday(&tp[0], NULL);
	//循环对数组赋值修改
	for(int i = 0; i < ARRAY_SIZE; i++)
	{
		array[i & length] = 0;
	}
	gettimeofday(&tp[1], NULL);
	change_time = get_usec(tp[0], tp[1]);
	
	gettimeofday(&tp[0], NULL);
	//循环访问数组缺失
	for(int i = 0; i < ARRAY_SIZE; i++)
	{
		array[(i % (ARRAY_SIZE / cacheLine)) * cacheLine] = 0;
	}
	gettimeofday(&tp[1], NULL);
	lost_time = get_usec(tp[0], tp[1]);
	
	printf("The time of array changed is %7.1lfms\n",change_time / 1000);
	printf("The time of array accessd lost is %7.1lfms\n",lost_time / 1000);
	
	//若两种情况下访存时间差距较大，则为写回法
	if(change_time - lost_time > 0.5 * lost_time || lost_time - change_time > 0.5 * change_time )
	{
		printf("The method of cache write is write back method\n");
	}
	//若两种情况下访存时间差距较小，则为写直达法
	else
	{
		printf("The method of cache write is write through method\n");
	}
}

void Test_Cache_Swap_Method()
{
	printf("**************************************************************\n");
	printf("Cache Replace Method Test\n");

	// TODO
	int cache_block_num = L1_cache_size / cacheLine;//512
	int cache_group_num = cache_block_num / L1_cache_group;//64
	double all_hit_time = 0;
	double all_replace_time = 0;
	double test_time = 0;
	struct timeval tp[2];
	BYTE* curArray = (BYTE*)malloc(sizeof(BYTE) * 2 * L1_cache_size);
	for(int loop = 0; loop < ARRAY_SIZE >> 5; loop++)
	{
		Clear_L1_Cache();
		// 先将cache填满(用1-8号数据块填满)
		for(int i = 0; i < L1_cache_group; i++) 
		{
			curArray[i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[0], NULL);
	
		// 再倒序访问一遍，让LRU访问顺序发生反转
		for(int i = L1_cache_group - 1; i >= 0; i--) 
		{
			curArray[i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[1], NULL);
	
		//全命中访问每个组块数量次cache的时间 
		all_hit_time += get_usec(tp[0], tp[1]);
	
		//再访问其他数据块来替换cache中存放的一半数据块(访问9-12) 
		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < L1_cache_group / 2; i++) 
		{
			curArray[L1_cache_size + i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[1], NULL);
		
		//全替换访问每个块数量次cache的时间 
		all_replace_time += (2 * get_usec(tp[0], tp[1]));
	
		//若是LRU算法,则cache中5-8号数据块被换出,访问1-4号数据块所用时间最短=all_hit_time
		//若是FIFO算法,则cache中1-4号数据块被换出,访问1-4号数据块所用时间最长=all_repalce_time
		//若是随机算法,则cache中随机4块数据块被换出,访问1-4号数据块所用时间位于其间
	
		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < L1_cache_group / 2; i++) 
		{
			curArray[i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[1], NULL);

		test_time += (2 * get_usec(tp[0], tp[1]));
	}
	free(curArray);
	printf("The time of cache hit is %7.1lfms\n",all_hit_time / 1000);
	printf("The time of cache replace is %7.1lfms\n",all_replace_time / 1000);
	printf("The time of cache test is %7.1lfms\n",test_time / 1000);
}

void Test_TLB_Size()
{
	printf("**************************************************************\n");
	printf("TLB Size Test\n");

	const int page_size = 1 << 12;								// Execute "getconf PAGE_SIZE" under linux terminal

    // TODO
    double sum_time[TLB_entries_length];
	for(int cur = 0; cur < TLB_entries_length; cur++)
	{
		struct timeval tp[2];

		gettimeofday(&tp[0], NULL);
		//保证每组测量访存次数相等
		for(int loop = 0; loop < (ARRAY_SIZE / TLB_entries[cur]) >> 10; loop++)
		{
			//按照不同页数访问
			for(int i = 0; i <  TLB_entries[cur]; i++)
			{
				//按照cacheLine步长访问每个页
				for(int j = 0; j < page_size; j+= cacheLine)
				{
					array[i * page_size + j] = 0;
				}
			}
		}
		gettimeofday(&tp[1], NULL);

		sum_time[cur] = get_usec(tp[0], tp[1]);

		printf("[Test_TLB_Entries = %5d]     Average access time: %7.1lfms\n",TLB_entries[cur],sum_time[cur] / 1000);
	}
	//寻找访存时间开始发生较大变化的页数(>70)，即为TLBentry数量
	int change = 0;
	for(int cur = 1; cur < TLB_entries_length; cur++)
	{
		if(sum_time[cur] / 1000 > 70)
		{
			change = cur;
			break;
		}
	}
	printf("The number of TLB entries is %d\n",TLB_entries[change - 1]);	
}

int main()
{
	Test_Cache_Size();
	Test_L1C_Block_Size();
	Test_L2C_Block_Size();
	Test_L1C_Way_Count();
	Test_L2C_Way_Count();
	Test_Cache_Write_Policy();
	Test_Cache_Swap_Method();
	Test_TLB_Size();
	
	return 0;
}