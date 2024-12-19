#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include <map>
#include <fstream>
class Config {
public:
	std::map<std::string, std::map<std::string, std::string> > records;
	Config(const char* path) {
		std::string section = "default";

		std::ifstream file(path);

		if (!file.is_open()) {
			return;
		}
		while (!file.eof()) {
			char buff[1024];
			file.getline(buff, sizeof(buff) - 1);

			for (char* ch = buff; *ch; ch++) {
				if (*ch == '#') {
					*ch = '\0';
					break;
				}
				if (*ch == '\r' || *ch == '\n') {
					*ch = '\0';
					break;
				}
			}

			if (buff[0] == '[') {
				int i = 0;
				while (buff[i])
					i++;
				i--;
				if (i > 0 && buff[i] == ']') {
					buff[i] = '\0';
					section = buff + 1;
					continue;
				}
			}

			{
				char* end = buff;
				while (*end)end++;
				while (buff < end && *(end - 1) == ' ')
					*--end = '\0';
			}

			char* key = buff;
			char* value = buff;
			while (*value) {
				if (*value == '=') {
					*value = '\0';
					value++;
					while (*value == ' ') value++;
					break;
				}
				value++;
			}
			if (*key && *value) {
				if (records.count(section) == 0) {
					records[section] = {};
				}
				records[section][key] = value;
			}
		}
	}

	std::string GetOrDefault(std::string section, std::string key, std::string default = "") {
		if (records.count(section)) {
			if (records[section].count(key))
				return records[section][key];
		}
		return default;
	}
	int GetOrDefaultInt(std::string section, std::string key, int default = -1) {
		if (records.count(section)) {
			if (records[section].count(key)) {
				std::string r = records[section][key];
				return atoi(r.c_str());
			}
		}
		return default;
	}

};
#endif