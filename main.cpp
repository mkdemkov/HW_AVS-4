#include <fstream>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <string>
#include <unistd.h>
#include <vector>

class Seller {      //класс продавца
public:
    int index{};

    Seller() = default;
};

class Customer {        //класс покупателя
public:
    int index{};
    int time_start{};

    std::queue<int> plan;

    Customer() = default;
};

bool global_flag = true;       //при false - закончились покупатели

pthread_mutex_t mutex_0;
pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;

std::string file_string;

std::vector<Customer *> customers;      //список покупателей

void *CustomerFunction(void *shared_arg) {          //функция потоков
    Customer *all_customers = (Customer *) shared_arg; // список покупателей

    sleep(all_customers->time_start);       //чтобы все покупатели не обслуживались в одно время, каждый из них "ждет" какое-то время
    std::cout << "\nCustomer #" << all_customers->index + 1 << " has came to the shop";
    file_string += "\nCustomer #" + std::to_string(all_customers->index + 1) + " has came to the shop";

    while (!all_customers->plan.empty()) {      //покупатель ходит по магазинам
        std::cout << "\nCustomer #" << all_customers->index + 1 << " has came to the seller #" << all_customers->plan.front() + 1;
        file_string += "\nCustomer #" + std::to_string(all_customers->index + 1) + " has came to the seller #" +
                       std::to_string(all_customers->plan.front() + 1);
        sleep(2);       //покупатель 2 секунды идет к нужному продавцу
        if (all_customers->plan.front() == 0) {
            pthread_mutex_lock(&mutex_0);
            break;
        } else if (all_customers->plan.front() == 1) {
            pthread_mutex_lock(&mutex_1);
            break;
        } else if (all_customers->plan.front() == 2) {
            pthread_mutex_lock(&mutex_2);
            break;
        } else {
            std::cout << "\nCan not lock the mutex";
        }
        customers[all_customers->plan.front()] = all_customers; //покупатель подходит к кассе
        sleep(1);       //1 секунда на обслуживание
        while (customers[all_customers->plan.front()] != nullptr) {     //так как продавец занят, всем остальным нужно подождать нужно
            usleep(200 * 1000);
        }
        if (all_customers->plan.front() == 0) {     //следующий покупатель
            pthread_mutex_unlock(&mutex_0);
            break;
        } else if (all_customers->plan.front() == 1) {
            pthread_mutex_unlock(&mutex_1);
            break;
        } else if (all_customers->plan.front() == 2) {
            pthread_mutex_unlock(&mutex_2);
            break;
        } else {
            std::cout << "\nCan not unlock the mutex";
        }
        all_customers->plan.pop();      //вычеркиваем сделанную покупку
    }
    //покупатель уходит из магазина
    std::cout << "\nCustomer #" << all_customers->index + 1 << " has left the shop";
    file_string += "\nCustomer #" + std::to_string(all_customers->index + 1) + " has left the shop";

    return nullptr;
}

void *SellerFunction(void *shared_arg) {            //функция потоков
    Seller *seller = (Seller *) shared_arg;
    while (global_flag) {
        if (customers[seller->index] == nullptr) {
            continue;
        }
        std::cout << "\nSeller #" << seller->index + 1 << " is now serving the customer #" << customers[seller->index]->index + 1;
        file_string += "\nSeller #" + std::to_string(seller->index + 1) + " is now serving the customer #" +
                        std::to_string(customers[seller->index]->index + 1);

        sleep(3);       //3 секунды на обслуживание
        std::cout << "\nSeller #" << seller->index + 1 << " has finished serving the customer #" << customers[seller->index]->index + 1;
        file_string += "\nSeller #" + std::to_string(seller->index + 1) + "is finished serving the customer #" +
                       std::to_string(customers[seller->index]->index + 1);

        customers[seller->index] = nullptr;
    }
    std::cout << "\n";      //как только покупатели закончились, магазин закрывается (логично)
    std::cout << "\nSeller #" << seller->index + 1 << " is now closing his shop";
    file_string += "\nSeller #" + std::to_string(seller->index + 1) + " is closing his shop";

    return nullptr;
}

int main(int argc, char *argv[]) {
    std::vector<Seller> sellers(3); //продавцы
    std::vector<pthread_t> threads_sellers(sellers.size());

    customers = std::vector<Customer *>(3);
    std::vector<Customer> customers;    //покупатели
    std::vector<pthread_t> threads_customers;

    int choice, number_of_customers, number_of_shops, order, max_number_of_shops, seed;
    std::string input, output;
    //инициализация мьютексов
    pthread_mutex_init(&mutex_0, nullptr);
    pthread_mutex_init(&mutex_1, nullptr);
    pthread_mutex_init(&mutex_2, nullptr);

    choice = 0;
    if (argc == 3) {    //файловый ввод командной строкой
        choice = 2;
        input = argv[1];
        output = argv[2];
    } else if (argc == 4) {     //рандомная генерация чисел командной строкой
        choice = 3;
        number_of_customers = std::stoi(argv[1]);
        max_number_of_shops = std::stoi(argv[2]);
        seed = std::stoi(argv[3]);
    }

    if (choice == 0) {
        std::cout << "Please, make a choice: \n1. Console input\n2. File input\n3. Random generating:\n";
        std::cin >> choice;
    }

    if (choice == 1) {      //консольный ввод
        std::cout << "Enter the amount of customers:";
        std::cin >> number_of_customers;
        customers = std::vector<Customer>(number_of_customers);
        threads_customers = std::vector<pthread_t>(customers.size());
        for (int i = 0; i < customers.size(); ++i) {
            customers[i].index = i;
            std::cout << "Customer #" << i + 1 << "\nEnter how many departments customer should go through from 1 to 3:";
            std::cin >> number_of_shops;
            if (number_of_shops < 1 or number_of_shops > 3) {
                continue;
            }
            std::queue<int> plan;
            std::cout << "Enter the departments in order:\n";
            for (int j = 0; j < number_of_shops; ++j) {
                std::cin >> order;
                plan.push(std::max(0, order - 1) % 3);
            }
            customers[i].plan = plan;
        }
    } else if (choice == 2) {      //консольный файловый ввод
        if (argc != 3) {
            std::cout << "Name the input file:";
            std::cin >> input;
            std::cout << "Name the output file:";
            std::cin >> output;
        }
        std::ifstream in(input);
        if (!in.is_open()) {
            std::cout << "\nFile can not be open for several reasons\n";
            return 1;
        }
        in >> number_of_customers;
        customers = std::vector<Customer>(number_of_customers);
        threads_customers = std::vector<pthread_t>(customers.size());
        for (int i = 0; i < customers.size(); ++i) {
            customers[i].index = i;
            in >> number_of_shops;
            std::queue<int> plan;
            for (int j = 0; j < number_of_shops; ++j) {
                in >> order;
                plan.push(std::max(0, order - 1) % 3);
            }
            customers[i].plan = plan;
        }
        in.close();
    } else if (choice == 3) {       //рандомная генерация чисел с консоля
        if (argc != 4) {
            std::cout << "Enter the amount of customers:";
            std::cin >> number_of_customers;
            std::cout << "Enter the maximum amount of departments:";
            std::cin >> max_number_of_shops;
            std::cout << "Enter a special value for random generation:";
            std::cin >> seed;
        }
        customers = std::vector<Customer>(number_of_customers);
        threads_customers = std::vector<pthread_t>(customers.size());
        srand(seed);
        for (int i = 0; i < customers.size(); ++i) {
            customers[i].index = i;
            number_of_shops = 1 + rand() % std::max(1, max_number_of_shops);
            std::queue<int> plan;
            for (int j = 0; j < number_of_shops; ++j) {
                order = rand() % 3;
                plan.push(order);
            }
            customers[i].plan = plan;
        }
    } else {
        std::cout << "There is no command you have typed";
        return 0;
    }

    std::cout << "\nShop is opened!\n";
    file_string += "\nShop is opened!\n";
    //Начало работы - 3 продавца "подходят" к своему отделу
    for (int i = 0; i < sellers.size(); i++) {
        sellers[i].index = i;
        pthread_create(&threads_sellers[i], nullptr, SellerFunction, &sellers[i]); //запуск потока
        std::cout << "\nSeller #" << i + 1 << " has came to his workplace";
        file_string += "\nSeller #" + std::to_string(i + 1) + " has came to his workplace";
    }
    std::cout << "\n";
    for (int i = 0; i < customers.size(); ++i) {
        customers[i].index = i;
        customers[i].time_start = rand() % 15;  //начальное время прихода в магазин
        std::cout << "\nCustomer #" << i + 1 << " is now making a shopping list";  //покупатель составляет план
        file_string += "\nCustomer #" + std::to_string(i + 1) + " is making a shopping list";
    }
    std::cout << "\n";
    //Все покупатели идут к своему отделу
    for (int i = 0; i < customers.size(); ++i) {
        pthread_create(&threads_customers[i], nullptr, CustomerFunction, &customers[i]);  //запуск потока
    }
    //Параллельное взаимодействие покупателей и продавцов
    for (unsigned long long threads_customer: threads_customers) {
        pthread_join(threads_customer, nullptr);
    }
    global_flag = false;   //все покупатели закончили ходить по магазинам
    for (unsigned long long threads_seller: threads_sellers) {
        pthread_join(threads_seller, nullptr);      //все продавцы "уходят с работы"
    }
    std::cout << "\n\nShop is closed!";
    file_string += "\n\nShop is closed!";

    if (choice == 2) {      //записываем в файл все, что передавалось в консоль
        std::ofstream out(output);
        if (!out.is_open()) {
            std::cout << "\nFile can not be open\n";
            return 0;
        }
        out << file_string;
        out.close();
    }

    pthread_mutex_destroy(&mutex_0);
    pthread_mutex_destroy(&mutex_1);
    pthread_mutex_destroy(&mutex_2);

    return 0;
}
