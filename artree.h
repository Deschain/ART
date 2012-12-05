/**
 * @file 	artree.h
 *
 * @date 	Dec 5, 2012
 * @author 	D059455
 */

#ifndef ARTREE_H_
#define ARTREE_H_

#include <cstring>
#include <emmintrin.h>

class artree{

	static const short node4  = 0;
	static const short node16 = 1;
	static const short node48 = 3;
	static const short node256= 4;
	static const short leaf		  = 5;
	static const short leaf4	  = 6;
	static const short leaf16	  = 7;
	static const short leaf48	  = 8;
	static const short leaf256	  = 9;


	struct node{
		unsigned short type;
		unsigned short count;
		unsigned short prefixLen;
		char prefix[8];
	};

	struct inner_node4 :public node{
		char key[4];
		node *child[4];

		inner_node4()
		{
			type = node4;
			count = 0;
			prefixLen = 0;
		}
	};

	struct inner_node16: public node {
		char key[16];
		node *child[16];

		inner_node16()
		{
			type = node16;
			count = 0;
			prefixLen = 0;
		}

	};

	struct inner_node48: public node {
		char key[256];
		node *child[48];

		inner_node48()
		{
			type = node48;
			count = 0;
			prefixLen = 0;
		}
	};

	struct inner_node256: public node{
		node* child[256];

		inner_node256()
		{
			type = node256;
			count = 0;
			prefixLen = 0;
		}
	};

	struct leaf_node_single : public node{
		char value;

		leaf_node_single()
		{
			type = leaf;
			count = 0;
			prefixLen = 0;
		}
	};

	struct leaf_node4 :public node{
		char value[4];

		leaf_node4()
		{
			type = leaf4;
			count = 0;
			prefixLen = 0;
		}

	};

	struct leaf_node16: public node {
		char value[16];

		leaf_node16()
		{
			type = leaf16;
			count = 0;
			prefixLen = 0;
		}

	};

	struct leaf__node48: public node {
		char value[48];

		leaf__node48()
		{
			type = leaf48;
			count = 0;
			prefixLen = 0;
		}

	};

	struct leaf__node256: public node{
		char value[256];

		leaf__node256()
		{
			type = leaf256;
			count = 0;
			prefixLen = 0;
		}
	};

public:

	bool isLeaf(node* node){
		return node->type == leaf || node->type == leaf4 || node->type == leaf16 || node->type == leaf48 || node->type == leaf256;
	}

	bool isFull(node* node_){
		switch(node_->type){
		case node4:
			return ((inner_node4*)node_)->count == 4;
			break;
		case node16:
			return ((inner_node16*)node_)->count == 16;
			break;
		case node48:
			return ((inner_node48*)node_)->count == 48;
			break;
		case node256:
			return ((inner_node256*)node_)->count == 256;
			break;
		default:
			return false;
		}
	}

	short checkPrefix(node* node,char* key, short depth) {
		return 0;
	}

	void addChild(node* parent, char byte, node* child){

	}

	void grow(node* node_) {

	}

	node* findChild(node* node, char byte) {
		if(node->type == node4)
		{
			inner_node4 *inner_node = (inner_node4*)node;
			for(int i; i < node->count; i++)
			{
				if(inner_node->key[i] == byte)
				{
					return inner_node->child[i];
				}
			}
			return NULL;
		}
		if(node->type == node16)
		{
			//SSE comparation
			int key = _mm_set1_epi8(byte);
			int cmp = _mm_cmpeq_epi8(key,node->prefixLen);
			int mask = (1<<node->count) -1;
			int bitfield = _mm_movemask_epi8(cmp) & mask;
			if(bitfield)
			{
				((inner_node16*)node)->child[/*ctz(bitfield)*/];
			}
		}
		if(node->type == node48)
		{
			inner_node48 *inner_node = (inner_node48*)node ;
			if(inner_node->key[byte] != 0)
			{
				return inner_node->child[inner_node->key[byte]];
			}
			return NULL;
		}
		if(node->type == node256)
		{
			return ((inner_node256*)node)->child[byte];
		}
	}

	bool leafMatches(node* node_, char* key, short depth) {
		return true;
	}

	node* search(node* node_, char* key, short depth) {
		if(node == NULL)
		{
			return NULL;
		}
		if(isLeaf(node_))
		{
			if(leafMatches(node_,key,depth))
			{
				return node_;
			}
			return NULL;
		}
		if(checkPrefix(node_,key,depth) != node_->prefixLen)
		{
			return NULL;
		}
		depth += node_->prefixLen;
		node* next = findChild(node_,key[depth]);
		return search(next,key,depth+1);
	}

	void insert(node* node_, char* key, node* leaf, short depth) {
		if(node_ == NULL)
		{
			node_ = leaf;
			return;
		}
		if(isLeaf(node))
		{
			inner_node4* newNode = new inner_node4();
			char* key2 = loadKey(node_);
			short i = depth;
			for(; key[i] == key2[i]; i++)
			{
				newNode->prefix[i-depth] = key[i];
			}
			newNode->prefixLen = i-depth;
			depth += newNode->prefixLen;
			addChild(newNode, key[depth],leaf);
			addChild(newNode, key2[depth],node_);
			//TODO: Free node
			node_ = newNode;
			return;
		}
		short p = checkPrefix(node_,key,depth);
		if(p != node_->prefixLen)
		{
			inner_node4 *newNode = new inner_node4();
			addChild(newNode, key[depth+p], leaf);
			addChild(newNode, node_->prefix[p], node_);
			newNode->prefix = p;
			memcpy(newNode->prefix,node_->prefix,p);
			node_->prefixLen = node_->prefixLen-(p+1);
			memmove(node_->prefix,node_->prefix+p+1,node_->prefixLen);
			//Free node_!!
			node_ = newNode;
			return;
		}
		depth=depth+node_->prefixLen;
		node *next = findChild(node_,key[depth]);
		if(next){
			insert(next,key,leaf,depth+1);
		}else{
			if(isFull(node_))
			{
				grow(node_);
			}
			addChild(node_, key[depth], leaf);
		}
	}

};


#endif /* ARTREE_H_ */
