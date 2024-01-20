#pragma once

template <typename T>
struct Node {
    Node(T value) : data(value), prev(nullptr), next(nullptr) {}
    T data;
    Node* prev;
    Node* next;
};

template <typename T>
class DoubleLinkedList {
protected:
    Node<T>* head;
    Node<T>* tail;
    size_t size;

    // Helper function to perform deep copy
    void copyFrom(const DoubleLinkedList& other) {
        if (!other.head)
            return;

        head = new Node<T>(other.head->data);
        Node<T>* current_this = head;
        Node<T>* current_other = other.head->next;

        while (current_other) {
            current_this->next = new Node<T>(current_other->data);
            current_this->next->prev = current_this;
            current_this = current_this->next;
            current_other = current_other->next;
        }

        tail = current_this;
        size = other.size;
    }

     // Helper function for merging two sorted linked lists
    Node<T>* merge(Node<T>* left, Node<T>* right, bool (*compare)(const T&, const T&)) {
        Node<T>* result = nullptr;

        if (!left)
            return right;
        if (!right)
            return left;

        if (compare(left->data, right->data)) {
            result = left;
            result->next = merge(left->next, right, compare);
            result->next->prev = result;
        } else {
            result = right;
            result->next = merge(left, right->next, compare);
            result->next->prev = result;
        }

        return result;
    }

    // Recursive merge sort function
    Node<T>* mergeSort(Node<T>* head, bool (*compare)(const T&, const T&)) {
        if (!head || !head->next)
            return head;

        Node<T>* middle = getMiddle(head);
        Node<T>* nextToMiddle = middle->next;

        middle->next = nullptr;

        Node<T>* left = mergeSort(head, compare);
        Node<T>* right = mergeSort(nextToMiddle, compare);

        return merge(left, right, compare);
    }

    // Helper function to find the middle node of a linked list
    Node<T>* getMiddle(Node<T>* head) {
        if (!head)
            return head;

        Node<T>* slow = head;
        Node<T>* fast = head->next;

        while (fast) {
            fast = fast->next;
            if (fast) {
                slow = slow->next;
                fast = fast->next;
            }
        }

        return slow;
    }

public:
    DoubleLinkedList() : head(nullptr), tail(nullptr), size(0) {}

    // Copy constructor
    DoubleLinkedList(const DoubleLinkedList& other) : head(nullptr), tail(nullptr), size(0) {
        copyFrom(other);
    }

    ~DoubleLinkedList() {
        clear();
    }

    void sort(bool (*compare)(const T&, const T&)) {
        if (size <= 1)
            return;

        head = mergeSort(head, compare);

        // Update the tail pointer
        tail = head;
        while (tail->next)
            tail = tail->next;
    }


    // Helper function to deallocate memory
    void clear() {
        while (head) {
            Node<T>* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        size = 0;
    }

    T& get(size_t index) {
        if(index >= size) {
            throw std::out_of_range("Index out of bounds");
        }
        Node<T>* current = head;
        size_t i = 0;
        while(current) {
            if(i == index) {
                return current->data;
            }
            current = current->next;
            i++;
        }
        throw std::out_of_range("Index out of bounds");
    }

    T& getLast() const {
        if(tail) return tail->data;
        throw std::out_of_range("Index out of bounds");
    }

    T& getFirst() const {
        if(head) return head->data;
        throw std::out_of_range("Index out of bounds");
    }

    size_t pushBack(const T& value) {
        Node<T>* newNode = new Node<T>(value);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size++;
        return size - 1;
    }

    void pushFront(const T& value) {
        Node<T>* newNode = new Node<T>(value);
        if (!head) {
            head = tail = newNode;
        } else {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        }
        size++;
    }

    void remove(const T& value) {
        Node<T>* current = head;
        while (current) {
            if (current->data == value) {
                if (current == head) {
                    head = current->next;
                    if (head) head->prev = nullptr;
                } else if (current == tail) {
                    tail = current->prev;
                    if (tail) tail->next = nullptr;
                } else {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }

                delete current;
                size--;
                return;
            }
            current = current->next;
        }
    }

    void removeIndex(size_t index) {
        if(index >= size) return;
        Node<T>* current = head;
        size_t currentIndex = 0;
        while (current) {
            if (currentIndex == index) {
                if (current == head) {
                    head = current->next;
                    if (head) head->prev = nullptr;
                } else if (current == tail) {
                    tail = current->prev;
                    if (tail) tail->next = nullptr;
                } else {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }
                delete current;
                size--;
                return;
            }
            current = current->next;
            currentIndex++;
        }
    }

    bool includes(const T& cmp) {
        for (auto element : *this) {
            if(element == cmp) {
                return true;
            }
        }
        return false;
    }

    size_t getSize() const {
        return size;
    }

    // Copy assignment operator
    DoubleLinkedList& operator=(const DoubleLinkedList& other) {
        if (this == &other)
            return *this;

        clear();
        copyFrom(other);
        return *this;
    }

    // Iterator for range-based for loop
    class Iterator {
    private:
        Node<T>* current;

    public:
        Iterator(Node<T>* node) : current(node) {}

        T& operator*() const {
            return current->data;
        }

        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }

        void operator++() {
            current = current->next;
        }
    };

    class ReverseIterator {
    private:
        Node<T>* current;

    public:
        ReverseIterator(Node<T>* node) : current(node) {}

        T& operator*() const {
            return current->data;
        }

        bool operator!=(const ReverseIterator& other) const {
            return current != other.current;
        }

        void operator++() {
            current = current->prev;
        }
    };


    Iterator begin() const {
        return Iterator(head);
    }

    Iterator end() const {
        return Iterator(nullptr);
    }

    ReverseIterator rbegin() const {
        return ReverseIterator(tail);
    }

    ReverseIterator rend() const {
        return ReverseIterator(nullptr);
    }
};