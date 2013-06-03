#pragma once

#include <iterator>
#include <stack>
#include <vector>

// forward declaration
class art_node;
class artree;

namespace art{

class art_it: public std::iterator<std::forward_iterator_tag, art_node*>
{	
    friend class art_node;
    friend class artree;

public:

    art_node* node_ ;
    uint8_t   child_; 

   std::stack<std::pair<art_node*,uint8_t> > path_;



    //Default constructible
    art_it() : node_(NULL), child_(0) { }

    art_it(art_node* node, uint8_t child) : node_(node), child_(child) { }
    //Copy constructible
    art_it(const art_it &lhs) : node_(lhs.node_), child_(lhs.child_), path_(lhs.path_) { }

    ~art_it(){ }

    //Compare operators
    bool operator != (const art_it &lhs) const
    {
        return (! operator==(lhs));
    }

    bool operator == (const art_it &lhs) const
    {
        return node_ == lhs.node_ && child_ == lhs.child_;
    }

    //Reference operators
    art_node& operator*  () const
    {
        return *(getChild());
    }

    art_node* operator-> () const
    {
       return getChild();
    }

    //Increment operators
    const art_it& operator++()
    {
        advance();
        return *this;
    }

    const art_it operator++(int)
    {
        art_it tmp = *this;
        ++(*this);
        return tmp;
    }

    art_node* getChild() const
    {        
        if(!node_) return NULL;

        switch(node_->type_)
        {
             case ART_NODE_004:
                 {
                     return ((art_node_4*)node_)->child_[child_];
                 }
                 break;
             case ART_NODE_016:
                 {
                     return ((art_node_16*)node_)->child_[child_];
                 }
                 break;
             case ART_NODE_048:
                 {
                     art_node_48* tmp = (art_node_48*)node_;
                     return tmp->child_[tmp->keys_[child_]];
                 }
                 break;
             case ART_NODE_256:
                 {
                     return ((art_node_256*)node_)->child_[child_];
                 }
                 break;
            default:
                return NULL;
        }
    }

    void advance()
    {   
        if(node_ == NULL) return;  
       
        if(child_ < node_->num_children_ - 1)
        {
            ++child_;
        }else{            
            if(path_.empty()) 
            {
                ++child_;
                return;
            }else{
                node_ = path_.top().first;
                child_= path_.top().second;
                path_.pop();
            }
            advance();            
            while(! is_leaf(getChild()))
            {
                path_.push(std::make_pair(node_,child_));
                node_ = getChild();
                child_= 0;                
            }
        }
    }
};

}