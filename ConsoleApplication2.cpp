#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <map>
#include <stdexcept>

// ==================== 1. Базовый класс FileManager ====================
class FileManager {
public:
    virtual ~FileManager() = default;
    
    /// Проверка существования файла
    bool fileExists(const std::string& path) const {
        std::ifstream file(path);
        return file.good();
    }
    
    /// Чтение файла в бинарном режиме
    virtual std::vector<uint8_t> readFile(const std::string& path) const {
        if (!fileExists(path)) {
            throw std::runtime_error("File not found: " + path);
        }
        
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), {}};
    }
    
    /// Запись файла в бинарном режиме
    virtual void writeFile(const std::string& path, const std::vector<uint8_t>& data) const {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
};

// ==================== 2. Абстрактный класс Compressor ====================
class Compressor {
public:
    virtual ~Compressor() = default;
    
    /// Сжатие данных (абстрактный метод)
    virtual std::vector<uint8_t> compress(const std::vector<uint8_t>& data) = 0;
    
    /// Распаковка данных (абстрактный метод)
    virtual std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) = 0;
    
    /// Получение коэффициента сжатия
    virtual float getCompressionRatio() const = 0;
};

// ==================== 3. Реализация RLE сжатия ====================
class RLECompressor : public Compressor {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override {
        std::vector<uint8_t> compressed;
        if (data.empty()) return compressed;
        
        uint8_t current = data[0];
        int count = 1;
        
        for (size_t i = 1; i < data.size(); i++) {
            if (data[i] == current && count < 255) {
                count++;
            } else {
                compressed.push_back(current);
                compressed.push_back(static_cast<uint8_t>(count));
                current = data[i];
                count = 1;
            }
        }
        
        compressed.push_back(current);
        compressed.push_back(static_cast<uint8_t>(count));
        
        return compressed;
    }
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData) override {
        std::vector<uint8_t> decompressed;
        
        for (size_t i = 0; i < compressedData.size(); i += 2) {
            uint8_t value = compressedData[i];
            uint8_t count = compressedData[i+1];
            
            for (int j = 0; j < count; j++) {
                decompressed.push_back(value);
            }
        }
        
        return decompressed;
    }
    
    float getCompressionRatio() const override { return 1.5f; }
};

// ==================== 4. Главный класс Archiver ====================
class Archiver : public FileManager {
    std::unique_ptr<Compressor> compressor;
    
public:
    /// Конструктор с выбором алгоритма сжатия
    explicit Archiver(int type) {
        switch (type) {
            case 0: compressor = std::make_unique<RLECompressor>(); break;
            // Можно добавить другие алгоритмы
            default: throw std::invalid_argument("Unknown compressor type");
        }
    }
    
    /// Создание архива
    bool createArchive(const std::string& outputPath, const std::vector<std::string>& inputFiles) {
        try {
            std::vector<uint8_t> archiveData;
            
            // Простая реализация - просто объединяем файлы
            for (const auto& file : inputFiles) {
                auto fileData = readFile(file);
                auto compressed = compressor->compress(fileData);
                archiveData.insert(archiveData.end(), compressed.begin(), compressed.end());
            }
            
            writeFile(outputPath, archiveData);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /// Извлечение архива
    bool extractArchive(const std::string& inputPath, const std::string& outputDir) {
        try {
            auto archiveData = readFile(inputPath);
            auto decompressed = compressor->decompress(archiveData);
            
            // В реальной реализации нужно парсить структуру архива
            std::string outputPath = outputDir + "/extracted_file.bin";
            writeFile(outputPath, decompressed);
            
            return true;
        } catch (...) {
            return false;
        }
    }
};

// ==================== 5. Пример использования ====================
int main() {
    try {
        // 1. Создаем тестовый файл
        std::ofstream testFile("test.txt");
        testFile << "Hello World! This is a test file for RLE compression.";
        testFile.close();
        
        // 2. Создаем архиватор с RLE сжатием
        Archiver archiver(0);
        
        // 3. Создаем архив
        if (archiver.createArchive("archive.rle", {"test.txt"})) {
            std::cout << "Archive created successfully!\n";
        }
        
        // 4. Извлекаем архив
        if (archiver.extractArchive("archive.rle", ".")) {
            std::cout << "Archive extracted successfully!\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}