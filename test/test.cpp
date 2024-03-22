#include <iostream>

#include "../blocked_skiplist.hpp"

int main() {
    BlockedSkipList<int, int> list{256};
    for(int i = 1023; i >= 0; i--) {
        list.insert(i, i);
    }
    std::cout << std::endl;
    auto list2 = list;

    list.print();
    for(int i = 0; i < 1000; i++) {
        auto res = list.erase(i);
    }
    for(int i = 0; i < 896; i++) {
        std::cout << (list.find(i) != list.end()) << " ";
    }
    std::cout << std::endl;
    for(int i = 896; i < 1025; i++) {
        std::cout << (list.find(i) != list.end()) << " ";
    }
    std::cout << std::endl;
    list2 = list;
    list.print();
    list2.print();
    return 0;
}
