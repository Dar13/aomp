#include <cstdint>
#include <limits>

#include <cstdio>

// A Psuedo-Lazily-Synchronized Linked List
// Based on this implementation: https://github.com/jserv/concurrent-ll/
// Derived from: "A Pragmatic Implementation of Non-Blocking Linked-Lists" by Timothy L. Harris

class Node
{
    public:
        Node(uintptr_t v) : next(nullptr), value(v) {}

        Node* next;

        uintptr_t value;
};

class LazyList
{
    public:
        LazyList()
            : head(new Node(0)),
              tail(new Node(std::numeric_limits<uintptr_t>::max()))
        {
            head->next = tail;
        }

        ~LazyList()
        {
            Node* c = head;
            while(c)
            {
                Node* n = get_unmarked(c->next);
                delete c;
                c = n;
            }
        }

        // This is basically a re-implementation of MarkableReference
        inline bool is_marked(Node* n) { return (uintptr_t)n & 0x1; }
        inline Node* clear_mark(Node* n)
        {
            uintptr_t t = reinterpret_cast<uintptr_t>(n) & ~0x1ull;
            return reinterpret_cast<Node*>(t);
        }

        inline Node* set_mark(Node* n)
        {
            uintptr_t t = reinterpret_cast<uintptr_t>(n) | 0x1ull;
            return reinterpret_cast<Node*>(t);
        }

        inline Node* get_unmarked(Node* n)
        {
            return reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(n) & ~0x1ull);
        }

        inline Node* get_marked(Node* n)
        {
            return reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(n) | 0x1ull);
        }

        Node* search(uintptr_t value, Node **left)
        {
            Node* left_next = nullptr, *right = nullptr;

            while(true)
            {
                Node* pred = head;
                Node* curr = head->next;

                while(is_marked(curr) || (pred->value < value))
                {
                    if(!is_marked(curr))
                    {
                        *left = pred;
                        left_next = curr;
                    }

                    pred = get_unmarked(curr);
                    if(pred == tail)
                        break;

                    curr = pred->next;
                }

                right = pred;

                if(left_next == right)
                {
                    if(!is_marked(right->next))
                        return right;
                }
                else
                {
                    // If you reach here, you're in trouble.
                    // This is due to an un-pruned node sticking around,
                    // which means you're misusing the data structure.
                }
            }
        }

        // Searches for given value in list, and can return the found node
        bool contains(uintptr_t value, Node **node = nullptr)
        {
            Node* itr = get_unmarked(head->next);
            while(itr != tail)
            {
                if(!is_marked(itr->next) && itr->value >= value)
                {
                    if(itr->value == value)
                    {
                        if(node)
                            *node = itr;

                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                itr = get_unmarked(itr->next);
            }

            return false;
        }

        bool add(uintptr_t value)
        {
            Node* right = nullptr, *left = nullptr;
            Node* node = new Node(value);

            while(true)
            {
                right = search(value, &left);
                if(right != tail && right->value == value)
                {
                    delete node;
                    return false;
                }

                // TODO: Is this worth it? Only construct if we know we're gonna insert
                //Node* node = new Node(value);
                node->next = right;
                if(__sync_val_compare_and_swap(&(left->next), right, node) == right)
                {
                    return true;
                }
                //delete node;
            }
        }

        // Logically and/or physically removes node from list
        //
        // If you choose not to physically remove (via passing in a valid
        // 'removed'), a consequent operation may stall forever until the
        // node is pruned. You have been warned.
        bool remove(uintptr_t value, Node **removed = nullptr)
        {
            Node *right = nullptr, *left = nullptr, *right_next = nullptr;

            while(true)
            {
                right = search(value, &left);

                if(right == tail || right->value != value)
                    return false;

                right_next = right->next;
                if(!is_marked(right_next))
                {
                    // Logically remove node
                    if(__sync_val_compare_and_swap(&(right->next), right_next, get_marked(right_next)))
                    {
                        break;
                    }
                }
            }

            if(removed)
            {
                prune(value, removed);
            }

            return true;
        }

        // Search for marked node with this value
        bool prune(uintptr_t value, Node **pruned)
        {
            Node *right = nullptr, *left_next = nullptr, *left = nullptr;
            while(true)
            {
                Node* pred = head;
                Node* curr = head->next;

                while(is_marked(curr) || (pred->value < value))
                {
                    if(!is_marked(curr))
                    {
                        left = pred;
                        left_next = curr;
                    }

                    pred = get_unmarked(curr);
                    if(pred == tail)
                        break;

                    curr = pred->next;
                }

                right = pred;

                if(left_next != right)
                {
                    // Physically remove logically-removed node
                    if(__sync_val_compare_and_swap(&(left->next), left_next, right) == left_next)
                    {
                        *pruned = left_next;
                        return true;
                    }
                }
            }
        }


        void print()
        {
            auto c = head->next;
            while(c != tail)
            {
                if(!is_marked(c->next))
                    printf("[Valid] ");
                else
                    printf("[Marked] ");

                printf("(%p)->value = %#lx\n", c, c->value);

                c = get_unmarked(c->next);
            }
        }

    private:
        Node* head;
        Node* tail;
};
