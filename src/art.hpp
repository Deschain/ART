#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

//x86 SSE intrinsics
#include <emmintrin.h>

#ifdef _MSC_VER

	//#include <intrin.h>

	static uint32_t __inline ctz(unsigned long x )
	{
	   unsigned long r = 0;
	   _BitScanReverse(&r, x);
	   return r;
	}
#else
	#define ctz(x) __builtin_ctz(x)
#endif

#include "art_node.hpp"
#include "art_it.hpp"

namespace art{

class artree
{

	friend class art_node;
	friend class art_it;

public: 

	typedef art_it iterator;

public:

	std::vector<std::string> dictionary;
	art_node* root_;

public:

	artree() : root_(NULL) { }

	~artree()
	{
		delete root_;
	}

private:

	/**
	 * Check if the node is already full
	 * @arg node
	**/
	bool isFull(const art_node* node) const
	{
		switch(node->type_){
		case ART_NODE_004:
			return ((art_node_4*)node)->num_children_   == 4;
			break;
		case ART_NODE_016:
			return ((art_node_16*)node)->num_children_  == 16;
			break;
		case ART_NODE_048:
			return ((art_node_48*)node)->num_children_  == 48;
			break;
		case ART_NODE_256:
			return ((art_node_256*)node)->num_children_ == 256;
			break;
		default:
			return false;
		}
	}

   const  art_node* minimum(const art_node* node) const
	{
		if(!node) return NULL;
		if(is_leaf(node)) return node;

		switch(node->type_)
		{
			case ART_NODE_004:
				{
					art_node_4* node_4 = (art_node_4*)node;
					return minimum(node_4->child_[0]);
				}
			case ART_NODE_016:
				{
					art_node_16* node_16 = (art_node_16*)node;
					return minimum(node_16->child_[0]);
				}
			case ART_NODE_048:
				{
					uint8_t i = 0;
					while(!((art_node_48*)node)->keys_[i]) ++i;
					i = ((art_node_48*)node)->keys_[i] - 1;
					return minimum(((art_node_48*)node)->child_[i]);
				}
			case ART_NODE_256:
				{
					int8_t i = 0;
					while(!((art_node_256*)node)->child_[i]) ++i;                    
					return minimum(((art_node_256*)node)->child_[i]);
				}
		}
	}

	/**
	 * Returns the number of matched characteres in both the node and the key	 
	 */
	uint8_t checkPrefix(const art_node* node, const char* key, const uint8_t& key_length, const uint8_t& depth) const
	{		        
		uint8_t counter = 0;

		if(node->prefix_length_ > MAX_PREFIX_LENGTH)
		{                     
			const art_node* first_leaf = minimum(node);
			std::string key2 = dictionary[recover_leaf(first_leaf)];
			uint8_t cmp = std::min<uint8_t>(key_length, key2.size());
			for(; counter < cmp; ++counter)
			{
				if(key[counter+depth] != key2[counter+depth]) return counter;
			}            
		}else{
			uint8_t cmp = std::min<uint8_t>(std::min<uint8_t>(node->prefix_length_,MAX_PREFIX_LENGTH), key_length - depth);
			for(; counter < cmp; ++counter)
			{
				if(node->prefix_[counter] != key[depth + counter])
				{			
					return counter;
				}
			}
		}

	  
		
		return counter;
	}

	void addChild(art_node* parent, unsigned char byte, art_node* child)
	{
		switch(parent->type_)
		{
			case ART_NODE_004:
			{
				art_node_4* node = (art_node_4*)parent;
				uint8_t idx;
				for(idx = 0; idx < node->num_children_; ++idx)
				{
					if(byte < node->keys_[idx]) break;
				}

				memmove(node->keys_ + idx + 1, node->keys_  + idx,  node->num_children_ - idx);
				memmove(node->child_+ idx + 1, node->child_ + idx, (node->num_children_ - idx) * sizeof(art_node*));

				node->keys_ [idx] = byte;
				node->child_[idx] = child;
			}
			break;
			case ART_NODE_016:
			{
				art_node_16* node = (art_node_16*)parent;
				uint8_t idx;
				for(idx = 0; idx < node->num_children_; ++idx)
				{
					if(byte < node->keys_[idx]) break;
				}

				memmove(node->keys_ + idx + 1, node->keys_  + idx,  node->num_children_ - idx);
				memmove(node->child_+ idx + 1, node->child_ + idx, (node->num_children_ - idx) * sizeof(art_node*));

				node->keys_ [idx] = byte;
				node->child_[idx] = child;
			}
			break;
			case ART_NODE_048:
			{
				art_node_48* node  = (art_node_48*)parent;
				node->keys_ [byte] = node->num_children_;
				node->child_[node->num_children_] = child;
			}
			break;
			case ART_NODE_256:
			{
				art_node_256* node = (art_node_256*)parent;
				node->child_[byte] = child;
			}
			break;
		}		
		parent->num_children_++;
	}

	void grow(art_node*& node) 
	{		
		switch(node->type_)
		{
			case ART_NODE_004:
			{
				art_node_4  * oldNode = (art_node_4*)node;
				art_node_16 * newNode = new art_node_16();
			
				newNode->num_children_ = oldNode->num_children_;
				newNode->prefix_length_ = oldNode->prefix_length_;

				memcpy(newNode->prefix_,oldNode->prefix_,MAX_PREFIX_LENGTH);
				memcpy(newNode->keys_,oldNode->keys_,4);
				memcpy(newNode->child_,oldNode->child_,4*sizeof(art_node*));
			
				//delete old node
				delete node;
			
				node = (art_node*)newNode;
			}
			break;
			case ART_NODE_016:
			{
				art_node_16  * oldNode = (art_node_16*)node;
				art_node_48  * newNode = new art_node_48();

				newNode->num_children_ = oldNode->num_children_;
				newNode->prefix_length_ = oldNode->prefix_length_;

				memcpy(newNode->prefix_,oldNode->prefix_,MAX_PREFIX_LENGTH);
				for(uint8_t i = 0; i < 16; i++){
					newNode->keys_[oldNode->keys_[i]] = i;
					newNode->child_[i] = oldNode->child_[i];
				}

				//delete old node
				delete node;

				node = (art_node*)newNode;
			}
			break;
			case ART_NODE_048:
			{
				art_node_48  * oldNode = (art_node_48*)node;
				art_node_256 * newNode = new art_node_256();

				newNode->num_children_ = oldNode->num_children_;
				newNode->prefix_length_ = oldNode->prefix_length_;
				memcpy(newNode->prefix_,oldNode->prefix_,MAX_PREFIX_LENGTH);
				//TODO: copy data of the children

				//Delete old node
				delete node;

				node = (art_node*)newNode;
			}
			break;
			case ART_NODE_256:
				//How to grow the biggest node?
				//No need, there are no more than 256 values for char
			break;
		}
	}

	art_node** findChild(const art_node* node, unsigned char byte) const
	{

		switch(node->type_)
		{
			case ART_NODE_004:
			{
				art_node_4 *node_4 = (art_node_4*)node;

				for(uint8_t i = 0; i < node->num_children_; ++i)
				{
					if(node_4->keys_[i] == byte)
					{
						return &node_4->child_[i];
					}
				}	
			}
			break;
			case ART_NODE_016:
			{
				art_node_16 *node_16 = (art_node_16*)node;		

				//SIMD comparation using SEE instructions
				// Sets the 16 signed 8bit integer values to byte
				// Loads 128 bit value (16 8 bits keys)
				// And compare the two 128 values
				__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(byte) , _mm_loadu_si128((__m128i*)node_16->keys_));             
				int bitfield = _mm_movemask_epi8(cmp) & ((1 << node->num_children_) - 1);				
				
				if(bitfield)
				{

					return &node_16->child_[ctz(bitfield)];
				}
			}
			break;
			case ART_NODE_048:
			{
				art_node_48 *node_48 = (art_node_48*)node;
				//Direct access to child using key of parameter 'byte' index
				if(node_48->child_[node_48->keys_[byte]])
					return &node_48->child_[node_48->keys_[byte]];
			}
			break;
			case ART_NODE_256:
			{
				art_node_256 *node_256 = (art_node_256*)node;
				//Direct access to child using parameter 'byte' as index                
				return &node_256->child_[byte];
			}
			break;			
		}
		return NULL;	
	}

	bool leafMatches(const art_node* leaf, const std::string key) const
	{
		std::string nodeKey = loadKey(leaf);
		return key.compare(nodeKey) == 0;       
	}
	
	std::string loadKey(const art_node* leaf) const
	{
		return dictionary.at(recover_leaf(leaf));
	}

	size_t storeKey(const char* key)
	{		
		dictionary.push_back(key);
		return dictionary.size() -1;
	}

public:

	const art_node* search(art_node* node, const char* key, uint8_t key_length, uint8_t depth = 0) const 
	{
		while(node != NULL)
		{
			if(is_leaf(node))
			{
				if(leafMatches(node,key))
				{
					return node;
				}
				return NULL;
			}
			
			if(node->prefix_length_)
			{
				if(node->prefix_length_ < MAX_PREFIX_LENGTH)
				{
					short prefix_length = checkPrefix(node,key,key_length,depth);
					if(prefix_length != node->prefix_length_)
					{
						return NULL;
					}									
				}
				depth += node->prefix_length_;
			}						
			node = *(findChild(node,key[depth]));
			depth++;
		}

		return NULL;
	}

	void insert(art_node** node__, const char* key,  uint8_t key_length, art_node* leaf = NULL, uint8_t depth = 0) 
	{	
		art_node* node_ = *node__;
		//Handle empty tree
		if(!node_)
		{            	
			*node__ = (art_node*)(create_leaf(storeKey(key)));
			return;
		}
		//Expand node
		if(is_leaf(node_))
		{
			//Handle duplicate value
			if(leafMatches(node_,key)) return;
			
			art::art_node_4* newNode = new art::art_node_4();	
			std::string key2 = loadKey(node_);
			
				
			int cmp = std::min<uint8_t>(key_length, key2.size()) - depth;
			int i   = 0;
			for(; i < cmp; ++i)
			{
				if(key[i+depth] != key2[i+depth]) break;
			}
		   
			newNode->prefix_length_ = i;
			memcpy(newNode->prefix_, key + depth, std::min<uint8_t>(i,MAX_PREFIX_LENGTH));                        
			addChild(newNode, key2[i+depth],node_);
			dictionary.push_back(key);
			addChild(newNode, key[i+depth], create_leaf(dictionary.size()-1));			
			//Update node
			*node__ = (art_node*)newNode;
			return;
		}
		//Prefix mismatch
		uint8_t p = checkPrefix(node_,key,key_length,depth);
		if(p != node_->prefix_length_)
		{
			//Create new node
			art::art_node_4 *newNode = new art::art_node_4();
			newNode->prefix_length_ = p;
			memcpy(newNode->prefix_, node_->prefix_, std::min<uint8_t>(MAX_PREFIX_LENGTH,p));                      

			//Update prefix of the old node
			if(node_->prefix_length_ > MAX_PREFIX_LENGTH)
			{
				node_->prefix_length_ -= (p+1);
				const art_node* first_leaf = minimum(node_);
				addChild(newNode,dictionary[recover_leaf(first_leaf)][depth+p],node_);
				memmove(node_->prefix_, node_->prefix_+p+1,std::min<uint8_t>(MAX_PREFIX_LENGTH,node_->prefix_length_));
			}else{
				addChild(newNode, node_->prefix_[p], node_);              
				node_->prefix_length_ -= (p+1);
				memmove(node_->prefix_, node_->prefix_+p+1, std::min<uint8_t>(MAX_PREFIX_LENGTH,node_->prefix_length_));
			}
			
			addChild(newNode, key[depth+p], create_leaf(storeKey(key)));

			//Update node
			*node__ = (art_node*)newNode;
			return;
		}
		//Find child to insert in
		depth += node_->prefix_length_;
		art_node** child = findChild(node_,key[depth]);
		if(child){
			//Recursive insert
			insert(child, key, key_length, leaf, depth+1);
			return;
		}else{
			//Add to inner node
			if(isFull(node_))
			{
				//Grow full node
				grow(*node__);
			}		
			//Create new leaf with the index of the key in the dictionary
			art_node* leaf = create_leaf(storeKey(key));
			addChild(*node__, key[depth], leaf);
		}
	}

	art_it begin() const
	{
		art_it it(root_,0);
		while(! is_leaf(it.getChild()))
		{
			it.path_.push(std::make_pair(it.node_,it.child_));
			it.node_  = it.getChild();
			it.child_ = 0;
		}
		return it;
	}

	art_it end() const
	{
		art_it it;
		return it;
	}

	void print(art_node* node, std::string ident = "")
	{
		if(!node) return;
		if(is_leaf(node)) 
		{
			std::cout << ident << dictionary[recover_leaf(node)] << std::endl;
			return;
		}

		switch(node->type_)
		{
			case ART_NODE_004:
			{
				art_node_4* node_4 = (art_node_4*)node;				
				std::cout << ident << "Nodo tipo 4, " << (int)node->num_children_ << " hijos ("; 
				for(uint8_t idx = 0; idx < node->prefix_length_; ++idx)
				{			
					std::cout << (char)node->prefix_[idx];
				}
				std::cout << ")" << std::endl;
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{			
					print(node_4->child_[idx], ident + "  ");
				}
			}
			break;
			case ART_NODE_016:
			{
				art_node_16* node_16 = (art_node_16*)node;			
				std::cout << ident << "Nodo tipo 16, " << (int)node->num_children_ << " hijos (";
				for(uint8_t idx = 0; idx < node->prefix_length_; ++idx)
				{			
					std::cout << (char)node->prefix_[idx];
				}
				std::cout << ")" << std::endl;
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{										
					print(node_16->child_[idx], ident + "  ");
				}                
			}
			break;
			case ART_NODE_048:
			{
				art_node_48* node_48 = (art_node_48*)node;		
				std::cout << ident << "Nodo tipo 48, " << (int)node->num_children_ << " hijos (";
				for(uint8_t idx = 0; idx < node->prefix_length_; ++idx)
				{			
					std::cout << (char)node->prefix_[idx];
				}
				std::cout << ")" << std::endl;
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{										
					 print(node_48->child_[idx], ident + "  ");
				}                
			}
			break;
			case ART_NODE_256:
			{
				art_node_256* node_256 = (art_node_256*)node;	
				  std::cout << ident << "Nodo tipo 256, " << (int)node->num_children_ << " hijos (" << node->prefix_ << ")" << std::endl;
				for(uint8_t idx = 0; idx < 256; ++idx)
				{										
					print(node_256->child_[idx], ident + "  ");
				}               
			}
			break;
		}
	}

	void size(art_node* node, unsigned int &s)
	{
		if(!node || is_leaf(node)) return;
		switch(node->type_)
		{
			case ART_NODE_004:
			{
				art_node_4* node_4 = (art_node_4*)node;				
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{										
					size(node_4->child_[idx],s);
				}
				s += sizeof(art_node_4);
			}
			break;
			case ART_NODE_016:
			{
				art_node_16* node_16 = (art_node_16*)node;				
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{										
					size(node_16->child_[idx],s);
				}
				s += sizeof(art_node_16);
			}
			break;
			case ART_NODE_048:
			{
				art_node_48* node_48 = (art_node_48*)node;				
				for(uint8_t idx = 0; idx < node->num_children_; ++idx)
				{										
					size(node_48->child_[idx],s);
				}
				s += sizeof(art_node_48);
			}
			break;
			case ART_NODE_256:
			{
				art_node_256* node_256 = (art_node_256*)node;				
				for(uint8_t idx = 0; idx < 256; ++idx)
				{										
					size(node_256->child_[idx],s);
				}
				s += sizeof(art_node_256);
			}
			break;
		}
	}

	void insert(std::string& value)
	{
		insert(&root_, value.c_str(), value.size());
	}

	 const art_node* search(std::string& value)
	 {
		 return search(root_, value.c_str(), value.size());
	 }
};
}