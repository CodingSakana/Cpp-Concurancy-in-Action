#include <atomic>

template<typename T>
class lock_free_stack
{
private:
    std::atomic<node*> to_be_deleted;
    static void delete_nodes(node* nodes)
    {
        while(nodes)
        {
            node* next=nodes->next;
            delete nodes;
            nodes=next;
        }
    }
    void try_reclaim(node* old_head)
    {
        if(threads_in_pop==1)   // 这个时候就已经保证了head已经修改过了，所以在确认此时 threads_in_pop==1 后，old_head 已经永远不会被另外一个线程访问了
        {
            node* nodes_to_delete=to_be_deleted.exchange(nullptr);
            if(!--threads_in_pop)
            {
                delete_nodes(nodes_to_delete);
            }
            else if(nodes_to_delete)
            {
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;    // 所以这个地方可以直接 delete
        }
        else
        {
            chain_pending_node(old_head);
            --threads_in_pop;
        }
    }
    void chain_pending_nodes(node* nodes)       // 找到 nodes (即 nodes_to_delete) 的最后一个节点
    {
        node* last=nodes;
        while(node* const next=last->next)
        {
            last=next;
        }
        chain_pending_nodes(nodes,last);        // 相当于 nodes 的 first 和 last 作为参数
    }
    void chain_pending_nodes(node* first,node* last)    // 把一个节点链表 [first, ..., last] 链接到 to_be_deleted 的前面。
    {
        last->next=to_be_deleted;
        while(!to_be_deleted.compare_exchange_weak(     // 1. last->next = to_be_deleted    2. ti_be_deleted = first
                  last->next,first));
    }
    void chain_pending_node(node* n)
    {
        chain_pending_nodes(n,n);       // 把节点 n 接在 to_be_deleted 的前面
    }
};
