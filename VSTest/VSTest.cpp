#include <iostream>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>

using boost::uuids::detail::md5;

// класс MD5 хеша по блоку
std::string md5hash(char* block)
{
	md5 hash;
	md5::digest_type digest;

	hash.process_bytes(block, sizeof(block));
	hash.get_digest(digest);

	const auto charDigest = reinterpret_cast<const char*>(&digest);
	std::string result;
	boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));
	return result;
}

//класс блока для обработки
class fileCache {
	char* buff;
public:
	fileCache(int size) {
		buff = new char[size];
	}
	~fileCache() {
		delete[] buff;
	}
	char* getCache() { return buff; }
	void setCache(int pos, char val) 
	{
		buff[pos] = val;
	}
};

//класс входного файла
class fileInput {
	FILE* fin;
	errno_t err;
public:
	fileInput(std::string name) {
		err = fopen_s(&fin, name.c_str(), "rb"); // открытие входного файла
		if (err == 0) file_name = name;
	}
	~fileInput() {
		try
		{
			if (err <= 0) fclose(fin);
		}
		catch (...)
		{

		}
	}
	errno_t get_err() { return err; }
	std::string get_file_hash(int block_size)
	{
		//получение размера файла
		int fs;
		try
		{
			fs = getFileSize();
		}
		catch (...)
		{
			err = -1;
			return "Error on filesize calculation";
		}

		std::string result = "";

		//цикл по входному файлу
		for (int i = 0; i <= fs / block_size; i++)
		{
			fileCache file_cache = fileCache(block_size); // буфер промежуточного хранения считываемого из файла текста

			int rs = read_file_block(block_size, file_cache.getCache()); //чтение блока
			if (rs < 0)
			{
				err = -2;
				return "Error read from input file";
			}

			//если прочитано меньше блока, то дополняем нулями
			if (rs < block_size)
			{
				for (int j = rs; j < block_size; j++)
				{
					file_cache.setCache(j, 0);
				}
			}

			//вычисление хеша блока
			result += md5hash(file_cache.getCache());
		}

		return result;
	}
private:
	std::string file_name;
	
	//чтение блока
	int read_file_block(int block_size, char* cache)
	{
		try
		{
			int rs = fread_s(cache, block_size, sizeof(char), block_size, fin);
			return rs;
		}
		catch (...)
		{
			return -1;
		}
	}

	//получение размера файла
	long getFileSize()
	{
		struct stat stat_buf;
		int rc = stat(file_name.c_str(), &stat_buf);
		return rc == 0 ? stat_buf.st_size : -1;
	}
};

//класс выходного файла
class fileOutput {
	FILE* fout;
	errno_t err;
public:
	fileOutput(std::string name) {
		err = fopen_s(&fout, name.c_str(), "wb"); // открытие или создание выходного файла
	}
	~fileOutput() {
		try
		{
			if (err == 0) fclose(fout);
		}
		catch (...)
		{

		}
	}

	errno_t get_err() { return err; }

	//запись результата в файл
	int write_result_to_file(std::string val)
	{
		try
		{
			fwrite(val.c_str(), sizeof(char), val.length(), fout);
			return 0;
		}
		catch (...)
		{
			return -1;
		}
	}
};

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "rus"); // корректное отображение Кириллицы

	std::string input = argv[1]; // имя и путь входного файла
	std::string output = argv[2]; // имя и путь выходного файла
	
	int block_size = 1048576; // размер блока по умолчанию 1Мб
	try
	{
		block_size = std::stoi(argv[3]); // размер блока
	}
	catch (...)
	{
		std::cout << "Blocksize is not numeric. Size " + std::to_string(block_size) + "bytes will be used.";
	}

	int err;

	fileOutput file_output = fileOutput(output); // создание или открытие выходного файла
	err = file_output.get_err();
	if (err != 0)
	{
		std::cout << "Cannot create output file with error " + std::to_string(err);
		return -2;
	}

	fileInput file_input = fileInput(input); // открытие входного файла
	err = file_input.get_err();
	if (err != 0)
	{
		std::cout << "Cannot open input file with error " + std::to_string(err);
		return -1;
	}

	std::string md5str = file_input.get_file_hash(block_size); // подсчет хеша с разбивкой по блокам
	if (file_input.get_err() != 0)
	{
		std::cout << md5str;
		return -4;
	}

	if (file_output.write_result_to_file(md5str) != 0) //запись хеша в файл
	{
		std::cout << "Error write to output file";
		return -5;
	}

	std::cout << "Complete";
	return 0;
}
