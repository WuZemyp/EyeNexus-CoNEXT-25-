#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
int main() {
    std::ifstream file("C:\\Users\\13513\\Desktop\\eyegaze.csv"); 

    if (!file) {
        std::cout << "can not open" << std::endl;
        return 1;
    }

    std::vector<std::vector<std::string>> data; 

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row; // 保存每一行的数据

        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, ',')) {
            row.push_back(token);
        }

        data.push_back(row);
    }

    
    int columnToExtract = 0; 

    for (const auto& row : data) {
        if (columnToExtract < row.size()) {
            std::string value = row[columnToExtract];
            std::cout << value << std::endl;
        }
    }

    return 0;
}