#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

//SIMD libreries
#include <emmintrin.h>

#ifdef _MSC_VER

    #include <intrin.h>

    uint32_t __inline ctz( uint32_t x )
    {
        unsigned long r = 0;
        _BitScanReverse(&r, x);
        return r;
    };

#endif

#ifdef __GNUC__
    int ctz(unsigned x )
    {		
        return __builtin_ctz(x);
    };
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

    /**
     * Returns the number of matched characteres in both the node and the key	 
     */
    uint8_t checkPrefix(const art_node* node, const char* key, uint8_t depth) const
    {		 
        uint8_t counter;
        for(counter = 0; counter < node->prefix_length_; ++counter)
        {
            if(node->prefix_[counter] != key[depth + counter])
            {			
                return counter;
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
                art_node_48* node = (art_node_48*)parent;
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

                int mask = (1 << node->num_children_) - 1;                
                int bitfield = _mm_movemask_epi8(cmp) & mask;				
                
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
                if(node_256->child_[byte])
                    return &node_256->child_[byte];
            }
            break;			
        }
        return NULL;	
    }

    bool leafMatches(const art_node* node, const std::string key, short depth) const
    {
        std::string nodeKey = loadKey(node);
        return key.compare(nodeKey) == 0;       
    }
    
    std::string loadKey(const art_node* leaf) const
    {
        return dictionary.at(recover_leaf(leaf));
    }

public:

    const art_node* search(const art_node* node, const char* key, uint8_t depth = 0) const 
    {
        if(node == NULL)
        {
            return NULL;
        }

        if(is_leaf(node))
        {
            if(leafMatches(node,key,depth))
            {
                return node;
            }
            return NULL;
        }

        short prefix_length = checkPrefix(node,key,depth);

        if(prefix_length != node->prefix_length_)
        {
            return NULL;
        }

        depth += node->prefix_length_;
        art_node** next = findChild(node,key[depth]);

        return search(*next,key,depth+1);
    }

    void insert(art_node** node__, const char* key, unsigned key_length, art_node* leaf = NULL, uint8_t depth = 0) 
    {	
        art_node* node_ = *node__;
        //Handle empty tree
        if(!node_)
        {
            dictionary.push_back(key);			         	
            *node__ = (art_node*)(create_leaf(dictionary.size()-1));
            return;
        }
        //Expand node
        if(is_leaf(node_))
        {
            //Handle duplicate value
            if(leafMatches(node_,key,depth)) return;
            
            art::art_node_4* newNode = new art::art_node_4();	
            std::string key2 = loadKey(node_);
                        
            uint8_t i = depth;
            for(; key[i] == key2[i]; ++i)
            {
                newNode->prefix_[i-depth] = key[i];
            }
            newNode->prefix_length_ = i-depth;
            depth += newNode->prefix_length_;
            addChild(newNode, key2[depth],node_);
            dictionary.push_back(key);
            addChild(newNode, key[depth], create_leaf(dictionary.size()-1));			
            //Update node
            *node__ = (art_node*)newNode;
            return;
        }
        //Prefix mismatch
        uint8_t p = checkPrefix(node_,key,depth);
        if(p != node_->prefix_length_)
        {
            art::art_node_4 *newNode = new art::art_node_4();
            dictionary.push_back(key);
            addChild(newNode, key[depth+p], create_leaf(dictionary.size()-1));
            addChild(newNode, node_->prefix_[p], node_);
            newNode->prefix_length_ = p;
            memcpy(newNode->prefix_, node_->prefix_, p);
            //Update prefix of the old node
            node_->prefix_length_ -= (p+1);
            memmove(node_->prefix_, node_->prefix_+p+1, node_->prefix_length_);
            //Update node
            *node__ = (art_node*)newNode;
            return;
        }
        //Find child to insert in
        depth += node_->prefix_length_;
        art_node** child = findChild(node_,key[depth]);
        if(child){
            //Recursive insert
            insert(child, key, key_length, leaf,depth+1);
            return;
        }else{
            //Add to inner node
            if(isFull(node_))
            {
                grow(*node__);
            }			
            dictionary.push_back(key);
            addChild(*node__, key[depth], create_leaf(dictionary.size()-1));
        }
    }

    art_it begin()
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

    art_it end()
    {
        art_it it(root_,root_->num_children_ -1);
        while(! is_leaf(it.getChild()))
        {
            it.path_.push(std::make_pair(it.node_,it.child_));
            it.child_ = it.node_->num_children_ - 1;
            it.node_  = it.getChild();            
        }
        it.child_++;
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
};
}