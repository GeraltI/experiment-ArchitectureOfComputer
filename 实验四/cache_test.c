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
		//time��¼�ô�ʱ��
		double time = 0;
		//�̶�ʹ�õ�array��ΧΪ��Ҫ�������С
		int length = cache_sizes[cur] * KB - 1;
		struct timeval tp[2];
		//��������ѭ����ʱ���ֵ����������ô�ʱ��
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
	//Ѱ�ҷô�ʱ��仯�������������С
	double time_change[cache_sizes_length];
	time_change[0] = 0;
	//���㲻ͬ�����С�ô�ʱ��仯��
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		time_change[cur] = (sum_time[cur] - sum_time[cur - 1]) / sum_time[cur - 1];
	}
	//�ҵ��ô�ʱ��仯���������������С
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
	// ���仯�������������С�ź�˳��
	if(first > second)
	{
		int temp = first;
		first = second;
		second = temp;
	}
	//�仯������������֮ǰ�������С��ΪL1_Dcache������L2_cache����
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
		//��������ѭ����ʱ���ֵ����������ô�ʱ��

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
	//Ѱ�ҷô�ʱ�俪ʼ�仯�ϴ�ķ��ʲ�������Ϊcache���С
	double time_change[block_sizes_length];
	time_change[0] = 0;
	int change = 0;
	for(int cur = 1; cur < cache_sizes_length; cur++)
	{
		time_change[cur] = (sum_time[cur] - sum_time[cur - 1]) / sum_time[cur - 1];
		//�����ҵ���һ���ô�ʱ��仯�ʴ���50%�ķ��ʲ���Ϊcache���С
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
	//Ѱ�ҷô�ʱ�俪ʼ�仯�ϴ�ķ��ʲ�������Ϊcache���С
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
			//����������
			for(int i = 1; i < group_nums[cur]; i += 2)
			{
				//����cacheLine << 2��������array����2 * L1_cache_size��С���ֵ�ÿ����
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
	//Ѱ�ҷô�ʱ��仯���ķֿ�����n��2^(n-2)��Ϊcache������
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
			//����������
			for(int i = 1; i < group_nums[cur]; i += 2)
			{
				//����cacheLine << 2��������array����2 * L1_cache_size��С���ֵ�ÿ����
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
	//Ѱ�ҷô�ʱ��仯���ķֿ�����n��2^(n-2)��Ϊcache������
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
	//ѭ�������鸳ֵ�޸�
	for(int i = 0; i < ARRAY_SIZE; i++)
	{
		array[i & length] = 0;
	}
	gettimeofday(&tp[1], NULL);
	change_time = get_usec(tp[0], tp[1]);
	
	gettimeofday(&tp[0], NULL);
	//ѭ����������ȱʧ
	for(int i = 0; i < ARRAY_SIZE; i++)
	{
		array[(i % (ARRAY_SIZE / cacheLine)) * cacheLine] = 0;
	}
	gettimeofday(&tp[1], NULL);
	lost_time = get_usec(tp[0], tp[1]);
	
	printf("The time of array changed is %7.1lfms\n",change_time / 1000);
	printf("The time of array accessd lost is %7.1lfms\n",lost_time / 1000);
	
	//����������·ô�ʱ����ϴ���Ϊд�ط�
	if(change_time - lost_time > 0.5 * lost_time || lost_time - change_time > 0.5 * change_time )
	{
		printf("The method of cache write is write back method\n");
	}
	//����������·ô�ʱ�����С����Ϊдֱ�﷨
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
		// �Ƚ�cache����(��1-8�����ݿ�����)
		for(int i = 0; i < L1_cache_group; i++) 
		{
			curArray[i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[0], NULL);
	
		// �ٵ������һ�飬��LRU����˳������ת
		for(int i = L1_cache_group - 1; i >= 0; i--) 
		{
			curArray[i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[1], NULL);
	
		//ȫ���з���ÿ�����������cache��ʱ�� 
		all_hit_time += get_usec(tp[0], tp[1]);
	
		//�ٷ����������ݿ����滻cache�д�ŵ�һ�����ݿ�(����9-12) 
		gettimeofday(&tp[0], NULL);
		for(int i = 0; i < L1_cache_group / 2; i++) 
		{
			curArray[L1_cache_size + i * cache_group_num * cacheLine] = 0;
		}
		gettimeofday(&tp[1], NULL);
		
		//ȫ�滻����ÿ����������cache��ʱ�� 
		all_replace_time += (2 * get_usec(tp[0], tp[1]));
	
		//����LRU�㷨,��cache��5-8�����ݿ鱻����,����1-4�����ݿ�����ʱ�����=all_hit_time
		//����FIFO�㷨,��cache��1-4�����ݿ鱻����,����1-4�����ݿ�����ʱ���=all_repalce_time
		//��������㷨,��cache�����4�����ݿ鱻����,����1-4�����ݿ�����ʱ��λ�����
	
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
		//��֤ÿ������ô�������
		for(int loop = 0; loop < (ARRAY_SIZE / TLB_entries[cur]) >> 10; loop++)
		{
			//���ղ�ͬҳ������
			for(int i = 0; i <  TLB_entries[cur]; i++)
			{
				//����cacheLine��������ÿ��ҳ
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
	//Ѱ�ҷô�ʱ�俪ʼ�����ϴ�仯��ҳ��(>70)����ΪTLBentry����
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