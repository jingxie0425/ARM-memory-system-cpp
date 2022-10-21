/*ENEE646 Mid-term by Jing Xie 116276009*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <list>
#include <algorithm>
#include <iomanip>

using namespace std;

class Trace{
	public:
		Trace();
		Trace(string type_str, string addr_str);
		unsigned short type;
		unsigned int addr;
		unsigned int get_index(int block_sz, int nset);
		unsigned int get_tag(int block_sz, int nset);
		unsigned int get_virtual_page_num();
		unsigned int get_page_offset();
		unsigned int get_virtual_address();
};

Trace::Trace(){
	type = 0;
	addr = 0;
}

Trace::Trace(string type_str, string addr_str){
	type = (unsigned short) stoi(type_str);
	addr = (unsigned int) strtoul(addr_str.c_str(), NULL, 16);	
}

/* get index of given address */
unsigned int Trace::get_index(int block_sz, int nset){
	
	unsigned int index = addr;
	index >>= (int) log2(block_sz);
	index %= nset;

	return index;

}

/* get tag bit of given address */
unsigned int Trace::get_tag(int block_sz, int nset){

	unsigned int tag = addr;
	tag >>= (int) log2(block_sz);
	tag >>= (int) log2(nset);

	return tag;

}

unsigned int Trace::get_virtual_page_num(){

	unsigned int vpn = addr;
	vpn >>= 16;//get left 16 bits

	return vpn;

}

unsigned int Trace::get_page_offset(){

	unsigned int pgo = addr;
	pgo %= 65536;//get right 16 bits

	return pgo;

}

unsigned int Trace::get_virtual_address(){

	return addr;

}
class CacheBlock{
	public:
		CacheBlock();
		unsigned int tag;
		bool dirty;
		bool operator == (const CacheBlock& block);
};

CacheBlock::CacheBlock(){
	tag = 0;
	dirty = false;
}

bool CacheBlock:: operator == (const CacheBlock& block){
	return tag == block.tag;
}





vector<Trace> traces_1;
vector<Trace> traces_2;
//list<CacheBlock> DPT[1];
list<CacheBlock> TLB_I[1];//define instruction TLB
list<CacheBlock> TLB_D[1];//define data TLB
list<CacheBlock> TLB_L2[128];//define level 2 TLB for data access
list<CacheBlock> MPT[1];//define memory page table
list<CacheBlock> DPT[1];//define disk page table
list<CacheBlock> L1_I[256];//define level1 cache for instruction search, 256 set
list<CacheBlock> L2[1024];//define level2 cache for instruction search, 1024 set
list<CacheBlock> L1_D[128];//define level1 cache for data search, 128 set
const size_t TLB_I_MAX_SZ = 10;
const size_t MPT_MAX_SZ = 100;
unsigned int real_page_num = 0;
unsigned int physical_addr = 0;
unsigned int page_offset = 0;

int TLB_D_miss_TLB_L2_miss = 0;
int TLB_D_miss_TLB_L2_hit = 0;
int TLB_D_hit = 0;
int L1_D_hit = 0;
int L1_D_miss_L2_hit = 0;
int L1_D_miss_L2_miss = 0;

int TLB_I_miss = 0;
int TLB_I_hit = 0;
int L1_I_hit = 0;
int L1_I_miss_L2_hit = 0;
int L1_I_miss_L2_miss = 0;

int write_back=0;

// In this function, there's a page faults simulation
void instruction_find_previous_edition(void){
	cout << "execute instruction find previous edition" << endl;
	
	for(size_t k = 0; k < traces_1.size(); k++){

		Trace t = traces_1[k];
		CacheBlock block; // cache block for L1
		cout << endl << "virtual_address " << k << ":";
		cout << hex << t.get_virtual_address() << endl;
		if(t.type == 0){ // instruction access delete
			bool flag_found_TLB_I = false;
			block.tag = t.get_virtual_page_num();
			//cout << hex << block.tag;

			// find in intruction TLB (TLB-I)
			list<CacheBlock>::iterator it_TLB_I;
			for (it_TLB_I =TLB_I[0].begin(); it_TLB_I!=TLB_I[0].end(); ++it_TLB_I) { 
				
				unsigned int TLB_I_tag = (*it_TLB_I).tag;
				TLB_I_tag >>= 16;//should be 16
        			if(TLB_I_tag == block.tag){
					flag_found_TLB_I = true;
					break;
				}
    			} 
			CacheBlock block_for_push = block;
			if(flag_found_TLB_I == false){ // not found (TLB_I miss)
				cout << "TLB_I miss" << endl;
				if(TLB_I[0].size() == TLB_I_MAX_SZ){ // TLB_I is full
					TLB_I[0].pop_front(); // block evicted from TLB_I
					cout<< "TLB_I this set is full" << endl;
				}

				//find in memory level page table (MPT)
				bool flag_found_MPT = false;
				list<CacheBlock>::iterator it_MPT;
				for (it_MPT =MPT[0].begin(); it_MPT!=MPT[0].end(); ++it_MPT) { 
					unsigned int MPT_tag = (*it_MPT).tag;
					MPT_tag >>=16;
					if (MPT_tag == block.tag){
						flag_found_MPT = true;
						break;
					}
				}
				//cout << flag_found_MPT;

				if(flag_found_MPT == false){ // not found in memory level page table
					cout << "Page fault-MPT miss" << endl;
					if(MPT[0].size() == MPT_MAX_SZ){ // MPT is full
						MPT[0].pop_front(); // block evicted from TLB_I
						cout << "MPT this set is full" << endl;
					}
					
					//find in disk level page table (DPT)
					list<CacheBlock>::iterator it_DPT;
					for (it_DPT =DPT[0].begin(); it_DPT!=DPT[0].end(); ++it_DPT) { 
						unsigned int DPT_tag = (*it_DPT).tag;
						DPT_tag >>=16;
						if (DPT_tag == block.tag){
							break;
						}
					}
					// put in MPT and TLB_I
					block_for_push.tag = (*it_DPT).tag;
					MPT[0].push_back(block_for_push);
					TLB_I[0].push_back(block_for_push);

					real_page_num = (*it_DPT).tag;
					real_page_num  %= 65536;//get right 16-bit real page number
					cout << "real page number from DPT:" << hex << real_page_num << endl;
					physical_addr = real_page_num << 16;
					physical_addr += t.get_page_offset();//check
					cout << "physical address:" << hex << physical_addr << endl;
				}

				if(flag_found_MPT == true){// TLB_I miss, MPT hit
					cout << "TLB_I miss, MPT hit" << endl;
					block_for_push.tag = (*it_MPT).tag;
					
					MPT[0].erase(it_MPT);
					MPT[0].push_back(block_for_push);//update memory level page table
					//block_for_push = block;
					//block_for_push.tag <<= 16;
					//block_for_push.tag += real_page_num_ex;
					TLB_I[0].push_back(block_for_push); // add what found in MPT to TLB_I
					
					// generate physical address
					real_page_num = block_for_push.tag;//32-bit
					real_page_num  %= 65536;//get right 16-bit real page number
					cout << "real page number from MPT:" << hex << real_page_num << endl;
					physical_addr = real_page_num << 16;
					physical_addr += t.get_page_offset();//check
					cout << "physical address:" << hex << physical_addr << endl;
				}
				

			}
			//cout << physical_addr << endl;
			//cout << real_page_num << endl;

			if(flag_found_TLB_I == true){ // found (TLB_I hit) 
				cout << "TLB_I hit" << endl;
				block_for_push.tag = (*it_TLB_I).tag;
				TLB_I[0].erase(it_TLB_I);
				TLB_I[0].push_back(block_for_push);
				real_page_num = block_for_push.tag;
				cout << "real page number from TLB_I:" << hex << real_page_num << endl;
				physical_addr = real_page_num << 16;
				physical_addr += t.get_page_offset();//check
				cout << "physical address:" << hex << physical_addr << endl;
			}

			// get set index and tag to search L1 cache
			unsigned int index_L1_I = physical_addr>>6;
			index_L1_I %= 256;
			cout << "index_L1_I:" << hex << index_L1_I << endl;
			block.tag = physical_addr>>14;
			cout << "block.tag:" << hex << block.tag << endl;

			list<CacheBlock>::iterator it_L1_I = find(L1_I[index_L1_I].begin(), L1_I[index_L1_I].end(), block);
			if(it_L1_I == L1_I[index_L1_I].end()){ // not found (L1_I miss)
				cout << "L1_I miss" << endl;
				if(L1_I[index_L1_I].size() == 2){ // set is full, two-way associative check
					L1_I[index_L1_I].pop_front(); // delete by LRU policy
					cout << "L1_I this set is full" << endl;
				}
				L1_I[index_L1_I].push_back(block);
				
				// try finding block in L2 cache
				unsigned int index_L2 = physical_addr>>6;
				index_L2 %= 1024;
				cout  << "index_L2:" << hex << index_L2 << endl;
				block.tag = physical_addr>>16;
				cout  << "block.tag:" << hex << block.tag << endl;
				list<CacheBlock>::iterator it_L2 = find(L2[index_L2].begin(), L2[index_L2].end(), block);
				if(it_L2 == L2[index_L2].end()){ // not found (L1 miss, L2 miss)
					cout << "L1_I miss, L2 miss" << endl;
					if(L2[index_L2].size() == 16){ // set is full, L2 cache is 16-way associative
						L2[index_L2].pop_front(); // block evicted from L2 cache
						cout << "L2 cache this set is full" << endl;
					}
					L2[index_L2].push_back(block);
				}// not found (L1 miss, L2 miss)

				else{ // found (L1 miss, L2 hit)
				cout << "L1 miss, L2 hit" << endl;
				// update L2 by LRU
				L2[index_L2].erase(it_L2);
				L2[index_L2].push_back(block);
			}
			}// not found (L1_I miss)
			else{ // found (L1_I hit)
				cout << "L1_I hit" << endl;
				// update L1 by LRU
				L1_I[index_L1_I].erase(it_L1_I);
				L1_I[index_L1_I].push_back(block);
			}

		}


	}
	
	
}


void instruction_find(unsigned int virtual_address){
	cout << "execute instruction find" << endl;
	cout << "virtual address:" << hex << virtual_address << endl;
	//for(size_t k = 0; k < traces_1.size(); k++){

	//Trace t = traces_1[k];
	CacheBlock block; // cache block for L1
	//cout << endl << "virtual_address " << k << ":";
	//cout << hex << t.get_virtual_address() << endl;
	//if(t.type == 0){ // instruction access delete
	bool flag_found_TLB_I = false;
	block.tag = virtual_address>>16;
	cout << "block.tag virtual page number:" << hex << block.tag;

	// find in intruction TLB (TLB-I)
	list<CacheBlock>::iterator it_TLB_I;
	for (it_TLB_I =TLB_I[0].begin(); it_TLB_I!=TLB_I[0].end(); ++it_TLB_I) { 
		
		unsigned int TLB_I_tag = (*it_TLB_I).tag;
		TLB_I_tag >>= 16;//should be 16
		if(TLB_I_tag == block.tag){
			flag_found_TLB_I = true;
			break;
		}
	} 
	CacheBlock block_for_push = block;
	if(flag_found_TLB_I == false){ // not found (TLB_I miss)
		TLB_I_miss++;
		cout << "TLB_I miss" << endl;
		if(TLB_I[0].size() == TLB_I_MAX_SZ){ // TLB_I is full
			TLB_I[0].pop_front(); // block evicted from TLB_I
			cout<< "TLB_I this set is full" << endl;
		}

		// generate a 16-bit physical address (real page number)
		real_page_num = rand()%65536;
		cout << "get real_page_num:" << hex << real_page_num << endl;

		// update TLB_I
		unsigned int TLB_I_tag_and_rpm = virtual_address >> 16; // get left 16-bit TLB_I tag
		TLB_I_tag_and_rpm <<= 16;
		TLB_I_tag_and_rpm += real_page_num;
		cout << "push this 16+16 bit number to TLB_I:" << hex << TLB_I_tag_and_rpm << endl;
		block_for_push.tag = TLB_I_tag_and_rpm; // 16+16 bit num
		TLB_I[0].push_back(block_for_push); // add what "found" in MPT to TLB_I (level 1 TLB) It's a 16+16 bit num

		page_offset = virtual_address % 65536; // 2^16 = 65536
		physical_addr = real_page_num << 16;
		physical_addr += page_offset;
		cout << "get physical address from MPT/DPT:" << hex << physical_addr << endl;
		// not simulate find in memory/disk level page table (MPT/DPT)

	}


	if(flag_found_TLB_I == true){ // found (TLB_I hit) 
		TLB_I_hit++;
		cout << "TLB_I hit" << endl;
		block_for_push.tag = (*it_TLB_I).tag;
		TLB_I[0].erase(it_TLB_I);
		TLB_I[0].push_back(block_for_push);

		// get physical address
		real_page_num = block_for_push.tag;
		real_page_num %= 65536; // get right 16 bits from 32 bits number stored in TLB_I
		page_offset = virtual_address % 65536; // 2^16 = 65536 get right 16 bits from 32-bit virtual address
		physical_addr = real_page_num << 16;
		physical_addr += page_offset; // get physical address
		cout << "get physical address from TLB_I:" << hex << physical_addr << endl;
	}

	// get set index and tag to search L1 cache
	unsigned int index_L1_I = physical_addr>>6;
	index_L1_I %= 256;
	cout << "index_L1_I:" << hex << index_L1_I << endl;
	block.tag = physical_addr>>14;
	cout << "block.tag:" << hex << block.tag << endl;

	list<CacheBlock>::iterator it_L1_I = find(L1_I[index_L1_I].begin(), L1_I[index_L1_I].end(), block);
	if(it_L1_I == L1_I[index_L1_I].end()){ // not found (L1_I miss)
		cout << "L1_I miss" << endl;
		if(L1_I[index_L1_I].size() == 2){ // set is full, two-way associative 
			unsigned int recover_tag_index = L1_I[index_L1_I].front().tag;//the .front() is what will be swapped from L1_I cache
			recover_tag_index <<= 8;
			recover_tag_index += index_L1_I; // a 26-bit number
			unsigned int recover_index_L2 = recover_tag_index % 1024; 
			
			//search L2 cache write back policy  put what will be swapped into L2, and if L2 already has, do nothing
			block.tag = recover_tag_index>>10; //L2 has 10-bit index
			list<CacheBlock>::iterator it_L2_wb = find(L2[recover_index_L2].begin(), L2[recover_index_L2].end(), block);
			if(it_L2_wb == L2[recover_index_L2].end()){// if there's no same tag as L1 what will be swapped, put what will be swapped into L2
				write_back++;
				if(L2[recover_index_L2].size() == 16){
					L2[recover_index_L2].pop_front();
				}
				L2[recover_index_L2].push_back(block);
			}
			L1_I[index_L1_I].pop_front(); // delete by LRU policy
			cout << "L1_I this set is full, execute write back policy" << endl;
		}
		L1_I[index_L1_I].push_back(block);
		
		// try finding block in L2 cache
		unsigned int index_L2 = physical_addr>>6;
		index_L2 %= 1024;
		cout  << "index_L2:" << hex << index_L2 << endl;
		block.tag = physical_addr>>16;
		cout  << "block.tag:" << hex << block.tag << endl;
		list<CacheBlock>::iterator it_L2 = find(L2[index_L2].begin(), L2[index_L2].end(), block);
		if(it_L2 == L2[index_L2].end()){ // not found (L1 miss, L2 miss)
			L1_I_miss_L2_miss++;
			cout << "L1_I miss, L2 miss" << endl;
			if(L2[index_L2].size() == 16){ // set is full, L2 cache is 16-way associative
				L2[index_L2].pop_front(); // block evicted from L2 cache
				cout << "L2 cache this set is full" << endl;
			}
			L2[index_L2].push_back(block);
		}// not found (L1 miss, L2 miss)

		else{ // found (L1 miss, L2 hit)
			L1_I_miss_L2_hit++;
			cout << "L1 miss, L2 hit" << endl;
			// update L2 by LRU
			L2[index_L2].erase(it_L2);
			L2[index_L2].push_back(block);
		}
	}// not found (L1_I miss)
	else{ // found (L1_I hit)
		L1_I_hit++;
		cout << "L1_I hit" << endl;
		// update L1 by LRU
		L1_I[index_L1_I].erase(it_L1_I);
		L1_I[index_L1_I].push_back(block);
	}
	
}




void data_find(unsigned long long int instruction){
	cout << "execute data find" << endl;
	cout << "instruction:" << hex << instruction << endl;
	
	CacheBlock block; // cache block for L1

	
	bool flag_found_TLB_D = false;
	
	unsigned long long int get_vir_addr = instruction >> 6;
	unsigned int vir_addr = (unsigned int) get_vir_addr % 4294967296; // 4294967296 = 2^32
	cout << "virtual address:" << hex << vir_addr << endl;
	block.tag = vir_addr >> 16; // get virtual page number, compared with Data TLB tag
	
	cout << "block_tag:" << hex << block.tag << endl;

	// find in Data TLB (TLB-D)
	list<CacheBlock>::iterator it_TLB_D;
	for (it_TLB_D =TLB_D[0].begin(); it_TLB_D!=TLB_D[0].end(); ++it_TLB_D) { 
		
		unsigned int TLB_D_tag = (*it_TLB_D).tag;
		TLB_D_tag >>= 16;// extract 16-bit TLB tag
		if(TLB_D_tag == block.tag){
			flag_found_TLB_D = true;
			break;
		}
	} 
	
	CacheBlock block_for_push = block;
	if(flag_found_TLB_D == false){ // not found (TLB_D miss)
		cout << "TLB_D miss" << endl;
		if(TLB_D[0].size() == 10){ // TLB_D is full 10-way associative
			TLB_D[0].pop_front(); // block evicted from TLB_D
			cout<< "TLB_D this set is full" << endl;
		}

		// find in TLB_L2
		unsigned int index_TLB_L2 = vir_addr >> 16;
		index_TLB_L2 %= 128; // 2^7 = 128 get 7-bit TLB_L2 index
		cout << "index_TLB_L2:" << hex << index_TLB_L2 << endl;
		block.tag = vir_addr >> 23; // get left 9-bit TLB_L2 tag
		cout << "block.tag:" << hex << block.tag << endl;
		bool flag_found_TLB_L2 = false;
		list<CacheBlock>::iterator it_TLB_L2;
		for (it_TLB_L2 =TLB_L2[index_TLB_L2].begin(); it_TLB_L2!=TLB_L2[index_TLB_L2].end(); ++it_TLB_L2) { 
			
			unsigned int TLB_L2_tag = (*it_TLB_L2).tag;
			TLB_L2_tag >>= 16;//should be 16
			if(TLB_L2_tag == block.tag){
				flag_found_TLB_L2 = true;
				break;
			}
		} 
		if(flag_found_TLB_L2 == false){ // not found (TLB_D miss, TLB_L2 miss)
			TLB_D_miss_TLB_L2_miss++;
			cout << "TLB_D miss, TLB_L2 miss" << endl;
			if(TLB_L2[index_TLB_L2].size() == 4){ // TLB_L2 is 4-way associative
				TLB_L2[index_TLB_L2].pop_front(); // block evicted from TLB_L2
				cout<< "TLB_L2 this set is full" << endl;
			}
			// generate a 16-bit physical address (real page number)
			real_page_num = rand()%65536;
			cout << "get real_page_num:" << hex << real_page_num << endl;

			// update TLB_L2
			unsigned int TLB_L2_tag_and_rpm = vir_addr >> 23; // get left 9-bit TLB_L2 tag (23 = 16+7)
			TLB_L2_tag_and_rpm <<= 16;
			TLB_L2_tag_and_rpm += real_page_num;
			cout << "push this 9+16 bit number to TLB_L2:" << hex << TLB_L2_tag_and_rpm << endl;
			block_for_push.tag = TLB_L2_tag_and_rpm; // 9+16 bit num
			TLB_L2[index_TLB_L2].push_back(block_for_push); // add what "found" in MPT to TLB_L2 (level 2 TLB) It's a 9+16 bit num

			// update TLB_D
			unsigned int TLB_D_tag_and_rpm = vir_addr >> 16; // get left 16-bit TLB_D tag
			TLB_D_tag_and_rpm <<= 16;
			TLB_D_tag_and_rpm += real_page_num;
			cout << "push this 16+16 bit number to TLB_D:" << hex << TLB_D_tag_and_rpm << endl;
			block_for_push.tag = TLB_D_tag_and_rpm; // 16+16 bit num
			TLB_D[0].push_back(block_for_push); // add what "found" in MPT to TLB_D (level 1 TLB) It's a 16+16 bit num

			page_offset = vir_addr % 65536; // 2^16 = 65536
			physical_addr = real_page_num << 16;
			physical_addr += page_offset;
			cout << "get physical address from MPT/DPT:" << hex << physical_addr << endl;
			// not simulate find in memory/disk level page table (MPT/DPT)
			
		}// not found (TLB_D miss, TLB_L2 miss)

		if(flag_found_TLB_L2 == true){ // found in TLB_L2 (TLB_D miss, TLB_L2 hit)
			TLB_D_miss_TLB_L2_hit++;
			cout << "TLB_D miss, TLB_L2 hit" << endl;
			// update TLB_L2 (LRU)
			block_for_push.tag = (*it_TLB_L2).tag;
			TLB_L2[index_TLB_L2].erase(it_TLB_L2);
			TLB_L2[index_TLB_L2].push_back(block_for_push);

			// get physical address
			real_page_num = block_for_push.tag;
			real_page_num %= 65536; // get right 16 bits from 23 bits 
			page_offset = vir_addr % 65536; // 2^16 = 65536 get right 16 bits from 32-bit virtual address
			physical_addr = real_page_num << 16;
			physical_addr += page_offset; // get physical address
			cout << "get physical address from TLB_L2:" << hex << physical_addr << endl;

			// update TLB_D
			unsigned int TLB_D_tag_and_rpm = vir_addr >> 16; // get left 16-bit TLB_D tag
			TLB_D_tag_and_rpm <<= 16;
			TLB_D_tag_and_rpm += real_page_num;
			cout << "push this 16+16 bit number to TLB_D:" << hex << TLB_D_tag_and_rpm << endl;
			block_for_push.tag = TLB_D_tag_and_rpm; // 16+16 bit num
			TLB_D[0].push_back(block_for_push); // add what "found" in MPT to TLB_D (level 1 TLB) It's a 16+16 bit num
		}// found in TLB_L2 (TLB_L2 hit)

	}//if not found (TLB_D miss)

	if(flag_found_TLB_D == true){ // found (TLB_D hit)
		TLB_D_hit++;
		cout << "TLB_D hit" << endl;
		// update TLB_D (LRU)
		block_for_push.tag = (*it_TLB_D).tag;
		TLB_D[0].erase(it_TLB_D);
		TLB_D[0].push_back(block_for_push);

		// get physical address
		real_page_num = block_for_push.tag;
		real_page_num %= 65536; // get right 16 bits from 32 bits 
		page_offset = vir_addr % 65536; // 2^16 = 65536 get right 16 bits from 32-bit virtual address
		physical_addr = real_page_num << 16;
		physical_addr += page_offset; // get physical address
		cout << "get physical address from TLB_D:" << hex << physical_addr << endl;
	}// found (TLB_D hit)

	// get set index and tag to search L1 cache
	unsigned int index_L1_D = physical_addr>>6;
	index_L1_D %= 128; // 7-bit index
	block.tag = physical_addr>>13; // 32-13=19 bit block tag number
	cout << "physical address for cache find:" << hex << physical_addr << endl;
	cout << "index_L1_D and block.tag" << hex << index_L1_D << "  " << block.tag << endl;
	list<CacheBlock>::iterator it_L1_D = find(L1_D[index_L1_D].begin(), L1_D[index_L1_D].end(), block);
	if(it_L1_D == L1_D[index_L1_D].end()){ // not found (L1_D miss)
		cout << "L1_D miss" << endl;
		if(L1_D[index_L1_D].size() == 4){ // set is full, 4-way associative check
			
			unsigned int recover_tag_index = L1_D[index_L1_D].front().tag;//the .front() is what will be swapped from L1_D cache
			
			recover_tag_index <<= 7; //L1_D has 7-bit index
			
			recover_tag_index += index_L1_D; // a 26-bit number tag+index
			cout << "recover_tag_index:" <<hex <<recover_tag_index <<endl;
			unsigned int recover_index_L2 = recover_tag_index % 1024; 
			cout << "recover_index_L2:" <<hex<<recover_index_L2 <<endl;
			//search L2 cache write back policy  put what will be swapped into L2, and if L2 already has, do nothing
			block.tag = recover_tag_index>>10; //L2 has 10-bit index
			cout << "block.tag:" <<hex<<block.tag <<endl;
			list<CacheBlock>::iterator it_L2_wb2 = find(L2[recover_index_L2].begin(), L2[recover_index_L2].end(), block);
			if(it_L2_wb2 == L2[recover_index_L2].end()){// if there's no same tag as L1 what will be swapped, put what will be swapped into L2
				write_back++;
				if(L2[recover_index_L2].size() == 16){
					L2[recover_index_L2].pop_front();
				}
				cout << "push this number into L2:" << block.tag << endl<< endl<< endl<< endl<< endl<< endl;
				L2[recover_index_L2].push_back(block);
			}
			L1_D[index_L1_D].pop_front(); // delete by LRU policy
			cout << "L1_D this set is full, execute write back policy" << endl;
		}

		L1_D[index_L1_D].push_back(block);
		
		// try finding block in L2 cache
		unsigned int index_L2 = physical_addr>>6;
		index_L2 %= 1024; // 10-bit index
		cout << "index_L2:" << hex << index_L2 << endl;
		block.tag = physical_addr>>16; // 16-bit block tag
		cout << "block.tag L2:" << hex << block.tag << endl;
		list<CacheBlock>::iterator it_L2 = find(L2[index_L2].begin(), L2[index_L2].end(), block);
		if(it_L2 == L2[index_L2].end()){ // not found (L1 miss, L2 miss)
			L1_D_miss_L2_miss++;
			cout << "L1 miss, L2 miss" << endl;
			if(L2[index_L2].size() == 16){ // set is full, L2 cache is 16-way associative
				L2[index_L2].pop_front(); // block evicted from L2 cache
				cout << "cache L2 this set is full" << endl;
			}
			L2[index_L2].push_back(block);
		}
		else{ // found (L1 miss, L2 hit)
			L1_D_miss_L2_hit++;
			cout << "L1 miss, L2 hit" << endl;
			L2[index_L2].erase(it_L2);
			L2[index_L2].push_back(block);
		}
	}// not found (L1_D miss)
	else{ // found (L1_D hit)
		L1_D_hit++;
		cout << "L1_D hit" << endl;
		L1_D[index_L1_D].erase(it_L1_D);
		L1_D[index_L1_D].push_back(block);
	}
	
		
}

int main(int argc, char* argv[]){

	ifstream fread_1(argv[1]); // open file
	string line;
	stringstream ss;

	/* read file and push each entry into vector */
	while(getline(fread_1, line)){	

		ss.clear();
		ss << line;
		string type, addr;
		ss >> type >> addr;
		
		//cout << "type:" << type << endl;
		//cout << "addr:" << addr << endl;

		Trace trace(type, addr);
		traces_1.push_back(trace);
		//need to be done: create a hard-disk page table
	}

	ifstream fread_2(argv[2]);
	string line2;
	stringstream ss2;
	

	while(getline(fread_2, line2)){
		ss2.clear();
		ss2 << line2;
		string pt;
		ss2 >> pt;
		CacheBlock blockread;
		blockread.tag = (unsigned int) strtoul(pt.c_str(), NULL, 16);
		
		DPT[0].push_back(blockread);
	}

	unsigned int pc = rand()%0x100000000;//define a program counter (32-dit number) and initialize it
	for(int i=0;i<100000;i++){//you can decide how many loops to be excuted by changing i range
		cout << endl << endl << "loop: " << i << endl;
		pc += 16;//after fetch one instruction, pc = pc+16
		cout << hex << "program counter = " << pc << endl;
		instruction_find(pc);
		for(int ti = 0;ti<2;ti++){// fetch two instructions at the same time
			cout << endl << "sub-loop:" << ti << endl;
			//generate an instruction
			unsigned long long int a = rand()%4;//opcode = 0/1/2/3
			a <<=38;
			unsigned long long int opcount = rand()%4 + 1;// 1 <= opcount <= 4
			//unsigned long long int ran_num = (rand() % (b-a))+a;
			unsigned long long int ran_num = rand();
			ran_num <<= 6;
			unsigned long long int instruction = a + ran_num + opcount;
			cout << "instruction fetched: " << hex << instruction << endl;
			unsigned long long int opcode = instruction >> 38;
			cout << "opcode:"<< opcode << endl;
			if(opcode == 0||opcode == 1){
				cout << "decode the instruction---load/store"<< endl;
				for(unsigned long long int j=1;j<=opcount;j++){
					instruction = instruction+16*64*(j-1);
					data_find(instruction);//64 means 2^6(shift left 6 bits) because for the next oprand, the address should be incremented by 16.
				}
			}
			if(opcode == 2){
				cout << "decode the instruction---branch"<< endl;
				//check the branch condition
				if(opcount == 1){
					cout << "direct branch" << endl;
					//get branch address
					pc = ran_num;
					pc = pc - 16;
				}
				else{
					cout << "conditional branch  won't be executed here" << endl;
				}//not simulate other conditions... It's not the main point of this question.
				
			}
		}
	}

	//statistic collection of data access
	cout << endl << endl << "Result:" << endl;
	cout << "TLB_D_miss_TLB_L2_miss:" << TLB_D_miss_TLB_L2_miss << endl;
	cout << "TLB_D_miss_TLB_L2_hit:" << TLB_D_miss_TLB_L2_hit << endl;
	cout <<"TLB_D_hit:"<< TLB_D_hit << endl;
	cout <<"L1_D_hit:"<< L1_D_hit << endl;
	cout <<"L1_D_miss_L2_hit:"<< L1_D_miss_L2_hit << endl;
	cout <<"L1_D_miss_L2_miss:"<< L1_D_miss_L2_miss << endl;
	
	//statistic collection of instruction fetch
	cout << "TLB_I_miss:" << TLB_I_miss << endl;
	cout << "TLB_I_hit:" << TLB_I_hit << endl;
	cout << "L1_I_hit:" << L1_I_hit << endl;
	cout << "L1_I_miss_L2_hit:" << L1_I_miss_L2_hit << endl;
	cout << "L1_I_miss_L2_miss:" << L1_I_miss_L2_miss << endl;

	cout << "write_back:" << write_back << endl;

	//calculate total clock cycles
	int total_clk_cycles = 100*(TLB_D_miss_TLB_L2_miss+L1_D_miss_L2_miss+TLB_I_miss+L1_I_miss_L2_miss)+8*(TLB_D_miss_TLB_L2_hit+L1_D_miss_L2_hit+L1_I_miss_L2_hit)+4*(TLB_D_hit+L1_D_hit+L1_I_hit+TLB_I_hit)+8*write_back;
	cout << endl << "total_clk_cycles:" << total_clk_cycles << endl;

	// this step is for checking a cache
	cout << endl << "display one cache---TLB_I Cache:";
	list<CacheBlock>::iterator it2;
	for (it2 =TLB_I[0].begin(); it2!=TLB_I[0].end(); ++it2) { 
		//(*it2).tag <<= 12;
		//CacheBlock TLB_output = *it2;
		//TLB_output <<= 10;
        	cout << hex << (*it2).tag << " "; 
    	} 
    	cout << endl; 
	

	// this part is fage fault simulation and it needs ./input_file/trace3.din ./input_file/trace4.din as input files!
	// the command will be "./find_test ./input_file/trace3.din ./input_file/trace4.din"
	instruction_find_previous_edition();//you can commit this line if you don't execute previous edition (page fault simulation)


	//display more caches/TLBs
/*
	list<CacheBlock>::iterator it3;
	for (it3 =TLB_2[0].begin(); it3!=TLB_2[0].end(); ++it3) { 
		//(*it2).tag <<= 12;
		//CacheBlock TLB_output = *it2;
		//TLB_output <<= 10;
        	cout << hex << (*it3).tag << " "; 
    	} 
    	cout << endl; 
*/
}
