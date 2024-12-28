#include <iostream>
#include "router.h"

void Router::run(){
    std::cout << "Router is running..." << std::endl;
    if (checkAccess()) {
        getUsers();
        createUser();
    } else {
        std::cout << "Access denied." << std::endl;
    }
}
void Router::getUsers() {
    std::cout << "Getting users..." << std::endl;
}
void Router::createUser(){
    std::cout << "Getting users..." << std::endl;
}
bool Router::checkAccess() {
    std::cout << "Checking access..." << std::endl;
    return true; //Проверка (ничего пока не делает)
}

