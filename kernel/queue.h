#ifndef _QUEUE_H_
#define _QUEUE_H_

template<typename T> class Queue {
public:
    virtual void addTail(T v) = 0;
    virtual T removeHead()= 0;
    virtual bool isEmpty() = 0;
    virtual unsigned long size() = 0;
};

template<typename T> class SimpleQueue : public Queue<T> {
private:
    class Node {
    public:
        Node* next;
        Node* prev;
        T value;
    };

    Node *first;
    Node *last;
    unsigned long nodes; // node count (size of queue)
public:
    SimpleQueue() : first(0), last(0) {}
    virtual ~SimpleQueue() {}
    void addTail(T v) {
        Node *n = new Node();
        n->value = v;
        n->prev = last;
        n->next = 0;
        if (last != 0) {
            last->next = n;
        }
        last = n;
        if (first == 0) {
            first = n;
        }
        nodes++;
    }
    bool isEmpty() {
        return first == 0;
    }
    T removeHead() {
        Node *p = first;
        first = p->next;
        if (first == 0) {
            last = 0;
        } else {
            first->prev = 0;
        }
        T v = p->value;
        delete p;
        nodes--;
        return v;
    }
    unsigned long size() {
        return nodes;
    }

};

#endif
