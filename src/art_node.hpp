#pragma once

#include <cstdint>

namespace art{

    const uint8_t ART_NODE_004 = 0;
    const uint8_t ART_NODE_016 = 1;
    const uint8_t ART_NODE_048 = 2;
    const uint8_t ART_NODE_256 = 3;

    const uint8_t MAX_PREFIX_LENGTH = 8;

class art_node
{

 public:

    uint8_t type_;
    uint8_t num_children_;
    uint8_t prefix_length_;
    unsigned char prefix_[MAX_PREFIX_LENGTH];

 public:
    
    art_node() : type_(0), num_children_(0), prefix_length_(0)
    {
        memset(prefix_, 0, MAX_PREFIX_LENGTH);
    }

};

class art_node_4: public art_node
{

public:

    unsigned char keys_[4];
    art_node*     child_[4];

public:
    art_node_4()
    {
        type_ = art::ART_NODE_004;
        memset(keys_,  0, 4);
        memset(child_, 0, 4*sizeof(art_node*));
    }

    ~art_node_4()
    {
        for(uint8_t idx = 0; idx < 4; ++idx)
        {
            delete child_[idx];
        }
    }
};

class art_node_16: public art_node
{	

public:

    unsigned char keys_ [16];
    art_node*     child_[16];

public:
    art_node_16()
    {
        type_ = art::ART_NODE_016;
        memset(keys_,  0, 16);
        memset(child_, 0, 16 * sizeof(art_node*));
    }

    ~art_node_16()
    {
        for(uint8_t idx = 0; idx < 16; ++idx)
        {
            delete child_[idx];
        }
    }
};

class art_node_48: public art_node
{

public:

    unsigned char keys_ [256];
    art_node*     child_[48];

public:

    art_node_48()
    {
        type_ = art::ART_NODE_048;
        memset(keys_,  0, 256);
        memset(child_, 0, 48 * sizeof(art_node*));
    }

    ~art_node_48()
    {
        for(uint8_t idx = 0; idx < 48; ++idx)
        {
            delete child_[idx];
        }
    }
};

class art_node_256: public art_node
{

public:

    art_node*     child_[256];

public:

    art_node_256()
    {		
        type_ = ART_NODE_256;
        memset(child_, 0, 256 * sizeof(art_node*));
    }

    ~art_node_256()
    {
        for(uint8_t idx = 0; idx < 256; ++idx)
        {
            delete child_[idx];
        }
    }
};
    
    //Pointer tagged values (assuming at least 4 bytes alignment) using only the lsb
    //Warning! Not applicable to all architecures

    /** 
     * Any pointer with the last bit set is pointer tagged as leaf
    **/
    art_node* create_leaf(const size_t index)
    {
        return (art_node*) ((index << 1) | 1);
    }

    bool is_leaf(const art_node* node)
    {
        return (((intptr_t)node) & 1);
    }

    /**
     * The value of the leaf node is calculated by shifting rigth the pointer tag bit
     * NOTE: The maximun value a leaf can store would be uintptr_t/2 (with 32 bits lenght uintptr_t it would be 2^31)
    **/
    uintptr_t recover_leaf(const art_node* leaf)
    {		
        return (size_t) ((uintptr_t)leaf >> 1);
    }
}