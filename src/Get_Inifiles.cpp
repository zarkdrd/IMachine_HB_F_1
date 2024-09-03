/*************************************************************************
	> File Name: src/Get_Inifiles.cpp
	> Author: ARTC
	> Descripttion: 获取INI配置文件程序源文件
	> Created Time: 2024-04-11
 ************************************************************************/

#include "../include/Get_Inifiles.h"

 /**获取配置文件信息时错误代码的含义**/
#define RET_OK 0                        // 成功
#define ERR_UNMATCHED_BRACKETS 2        // 没有找到匹配的]
#define ERR_SECTION_EMPTY 3             // 段为空
#define ERR_SECTION_ALREADY_EXISTS 4    // 段名已经存在
#define ERR_PARSE_KEY_VALUE_FAILED 5    // 解析key value失败
#define ERR_OPEN_FILE_FAILED 6          // 打开文件失败
#define ERR_NO_ENOUGH_MEMORY 7          // 内存不足
#define ERR_NOT_FOUND_KEY 8             // 没有找到对应的key
#define ERR_NOT_FOUND_SECTION 9         // 没有找到对应的section

/**行尾符号LF**/
static const char delim[] = "\n";

IniFile* IniFile::m_Instance = NULL;

IniFile::IniFile()
{
	commentDelimiter = "#";
}

IniFile::~IniFile()
{
	release();
}

IniFile *IniFile::getInstance()
{
	if(m_Instance == NULL) {
		 m_Instance = new IniFile();
	}
	return m_Instance;
}

bool IniFile::parse(const string &content, string *key, string *value)
{
	return split(content, "=", key, value);
}

int IniFile::UpdateSection(const string &cleanLine, const string &comment, const string &rightComment, IniSection **section)
{
	IniSection *newSection;
	size_t index = cleanLine.find_first_of(']');
	if (index == string::npos) {
		//std::cout << "no matched ] found" << std::endl;
		return ERR_UNMATCHED_BRACKETS;
	}

	int len = index - 1;
	if (len <= 0) {
		//std::cout << "section name is empty" << std::endl;
		return ERR_SECTION_EMPTY;
	}

	string s(cleanLine, 1, len);
	trim(s);

	if (getSection(s) != NULL) {
		//std::cout << "section [" << s << "] already exist" << std::endl;
		return ERR_SECTION_ALREADY_EXISTS;
	}

	newSection = new IniSection();
	newSection->name = s;
	newSection->comment = comment;
	newSection->rightComment = rightComment;

	sections_vt.push_back(newSection);
	*section = newSection;
	return 0;
}

int IniFile::AddKeyValuePair(const string &cleanLine, const string &comment, const string &rightComment, IniSection *section)
{
	string key, value;

	if (!parse(cleanLine, &key, &value)) {
		//std::cout << "parse line failed:  " << cleanLine << std::endl;
		return ERR_PARSE_KEY_VALUE_FAILED;
	}

	IniItem item;
	item.key = key;
	item.value = value;
	item.comment = comment;
	item.rightComment = rightComment;

	section->items.push_back(item);
	return 0;
}

int IniFile::Load(const string &filePath)
{
	int err;
	string line;
	string cleanLine;
	string comment;
	string rightComment;
	IniSection *currSection = NULL;
	release();

	iniFilePath = filePath;
	std::ifstream ifs(iniFilePath);
	if (!ifs.is_open()) {
		//std::cout << "Open [" << iniFilePath << "] file error" << std::endl;
		return ERR_OPEN_FILE_FAILED;
	}

	currSection = new IniSection();
	currSection->name = "";
	sections_vt.push_back(currSection);

	while (std::getline(ifs, line)) {
		trim(line);
		if (line.length() <= 0) {
			comment += delim;
			continue;
		}

		if (IsCommentLine(line)) {
			comment += line + delim;
			continue;
		}

		split(line, commentDelimiter, &cleanLine, &rightComment);
		if (cleanLine[0] == '[') {
			err = UpdateSection(cleanLine, comment, rightComment, &currSection);
		} else {
			err = AddKeyValuePair(cleanLine, comment, rightComment, currSection);
		}

		if (err != 0) {
			ifs.close();
			return err;
		}

		comment = "";
		rightComment = "";
	}

	ifs.close();

	return 0;
}

bool IniFile::Save()
{
	return SaveAs(iniFilePath);
}

bool IniFile::SaveAs(const string &filePath)
{
	string data = "";

	for (IniSection_it sect = sections_vt.begin(); sect != sections_vt.end(); ++sect) {
		if ((*sect)->comment != "") {
			data += (*sect)->comment;
		}

		if ((*sect)->name != "") {
			data += string("[") + (*sect)->name + string("]");
			data += delim;
		}

		if ((*sect)->rightComment != "") {
			data += " " + commentDelimiter +(*sect)->rightComment;
		}

		for (IniSection::IniItem_it item = (*sect)->items.begin(); item != (*sect)->items.end(); ++item) {
			if (item->comment != "") {
				data += item->comment;
				if (data[data.length()-1] != '\n') {
					data += delim;
				}
			}

			data += item->key + "=" + item->value;

			if (item->rightComment != "") {
				data += " " + commentDelimiter + item->rightComment;
			}

			if (data[data.length()-1] != '\n') {
				data += delim;
			}
		}
	}

	std::ofstream ofs;
	ofs.open(filePath, ios::out|ios::trunc);
	if(ofs.is_open()==false){
		return false;
	}
	ofs << data <<  flush;
	ofs.close();

	return true;
}

IniSection *IniFile::getSection(const string &section)
{
	for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
		if ((*it)->name == section) {
			return *it;
		}
	}

	return NULL;
}

int IniFile::GetSections(vector<string> *sections)
{
	for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
		sections->push_back((*it)->name);
	}
	return sections->size();
}

int IniFile::GetSectionNum()
{
	return sections_vt.size();
}

string IniFile::GetStringValue(const string &section, const string &key, const string defaultValue)
{
	string strValue;
	if (getValue(section, key, &strValue) != 0) {
		return defaultValue;
	}
	return strValue;
}

int IniFile::GetIntValue(const string &section, const string &key, const int defaultValue)
{
	string strValue;
	if (getValue(section, key, &strValue) != 0) {
		return defaultValue;
	}
	return atoi(strValue.c_str());
}

double IniFile::GetDoubleValue(const string &section, const string &key, const double defaultValue)
{
	string strValue;
	if (getValue(section, key, &strValue) != 0) {
		return defaultValue;
	}
	return atof(strValue.c_str());
}

bool IniFile::GetBoolValue(const string &section, const string &key, const bool defaultValue)
{
	string strValue;
	if (getValue(section, key, &strValue) != 0) {
		return defaultValue;
	}

	if (StringCmpIgnoreCase(strValue, "true") || StringCmpIgnoreCase(strValue, "1")) {
		return true;
	} else if (StringCmpIgnoreCase(strValue, "false") || StringCmpIgnoreCase(strValue, "0")) {
		return false;
	}

	return defaultValue;
}

int IniFile::GetComment(const string &section, const string &key, string *comment)
{
	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	if (key == "") {
		*comment = sect->comment;
		return RET_OK;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			*comment = it->comment;
			return RET_OK;
		}
	}

	//std::cout << "not find the key [" << key << "]" << std::endl;
	return ERR_NOT_FOUND_KEY;
}

int IniFile::GetRightComment(const string &section, const string &key, string *rightComment)
{
	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	if (key == "") {
		*rightComment = sect->rightComment;
		return RET_OK;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			*rightComment = it->rightComment;
			return RET_OK;
		}
	}

	//std::cout << "not find the key [" << key << "]" << std::endl;
	return ERR_NOT_FOUND_KEY;
}

int IniFile::getValue(const string &section, const string &key, string *value)
{
	string comment;
	return getValue(section, key, value, &comment);
}

int IniFile::getValue(const string &section, const string &key, string *value, string *comment)
{
	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			*value = it->value;
			*comment = it->comment;
			return RET_OK;
		}
	}

	//std::cout << "not find the key [" << key << "]" << std::endl;
	return ERR_NOT_FOUND_KEY;
}

int IniFile::GetValues(const string &section, const string &key, vector<string> *values)
{
	vector<string> comments;
	return GetValues(section, key, values, &comments);
}

int IniFile::GetValues(const string &section, const string &key, vector<string> *values, vector<string> *comments)
{
	string value, comment;

	values->clear();
	comments->clear();

	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			value = it->value;
			comment = it->comment;

			values->push_back(value);
			comments->push_back(comment);
		}
	}

	if (values->size() == 0) {
		//std::cout << "not find the key [" << key << "]" << std::endl;
		return ERR_NOT_FOUND_KEY;
	}

	return RET_OK;
}

bool IniFile::HasSection(const string &section)
{
	return (getSection(section) != NULL);
}

bool IniFile::HasKey(const string &section, const string &key)
{
	IniSection *sect = getSection(section);
	if (sect != NULL) {
		for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
			if (it->key == key) {
				return true;
			}
		}
	}

	return false;
}

int IniFile::setValue(const string &section, const string &key, const string &value, const string &comment /*=""*/)
{
	IniSection *sect = getSection(section);
	string comt = comment;
	if (comt != "") {
		comt = commentDelimiter + comt;
	}

	if (sect == NULL) {
		sect = new IniSection();
		if (sect == NULL) {
			//std::cout << "new IniSection error: " << std::endl;
			return ERR_NO_ENOUGH_MEMORY;
		}

		sect->name = section;
		if (sect->name == "") {
			sections_vt.insert(sections_vt.begin(), sect);
		} else {
			sections_vt.push_back(sect);
		}
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			it->value = value;
			it->comment = comt;
			return RET_OK;
		}
	}

	IniItem item;
	item.key = key;
	item.value = value;
	item.comment = comt;
	sect->items.push_back(item);

	return RET_OK;
}

int IniFile::SetStringValue(const string &section, const string &key, const string &value)
{
	return setValue(section, key, value);
}

int IniFile::SetIntValue(const string &section, const string &key, int value)
{
	char buf[64] = {0};
	snprintf(buf, sizeof(buf), "%d", value);
	return setValue(section, key, buf);
}

int IniFile::SetDoubleValue(const string &section, const string &key, double value)
{
	char buf[64] = {0};
	snprintf(buf, sizeof(buf), "%f", value);
	return setValue(section, key, buf);
}

int IniFile::SetBoolValue(const string &section, const string &key, bool value)
{
	if (value) {
		return setValue(section, key, "true");
	} else {
		return setValue(section, key, "false");
	}
}

int IniFile::SetComment(const string &section, const string &key, const string &comment)
{
	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	if (key == "") {
		sect->comment = comment;
		return RET_OK;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			it->comment = comment;
			return RET_OK;
		}
	}

	//std::cout << "not find the key [" << key << "]" << std::endl;
	return ERR_NOT_FOUND_KEY;
}

int IniFile::SetRightComment(const string &section, const string &key, const string &rightComment)
{
	IniSection *sect = getSection(section);
	if (sect == NULL) {
		//std::cout << "not find the section [" << section << "]" << std::endl;
		return ERR_NOT_FOUND_SECTION;
	}

	if (key == "") {
		sect->rightComment = rightComment;
		return RET_OK;
	}

	for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
		if (it->key == key) {
			it->rightComment = rightComment;
			return RET_OK;
		}
	}

	//std::cout << "not find the key [" << key << "]" << std::endl;
	return ERR_NOT_FOUND_KEY;
}

void IniFile::SetCommentDelimiter(const string &delimiter)
{
	commentDelimiter = delimiter;
}

void IniFile::DeleteSection(const string &section)
{
	for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ) {
		if ((*it)->name == section) {
			delete *it;
			it = sections_vt.erase(it);
			break;
		} else {
			it++;
		}
	}
}

void IniFile::DeleteKey(const string &section, const string &key)
{
	IniSection *sect = getSection(section);
	if (sect != NULL) {
		for (IniSection::IniItem_it it = sect->begin(); it != sect->end();) {
			if (it->key == key) {
				it = sect->items.erase(it);
				break;
			} else {
				it++;
			}
		}
	}
}

void IniFile::release()
{
	iniFilePath = "";
	for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
		delete (*it);
	}
	sections_vt.clear();
}

bool IniFile::IsCommentLine(const string &str)
{
	return StartWith(str, commentDelimiter);
}

void IniFile::PrintIniFile()
{
	std::cout << "############ [" << iniFilePath << "] ############" << std::endl;

	for (IniSection_it it = sections_vt.begin() + 1; it != sections_vt.end(); ++it) {
		std::cout << (*it)->comment;
		std::cout << "[" << (*it)->name << "]" << std::endl;
		if ((*it)->rightComment != "") {
			//std::cout << (*it)->rightComment << std::endl;
		}

		for (IniSection::IniItem_it i = (*it)->items.begin(); i != (*it)->items.end(); ++i) {
			std::cout << i->comment;
			std::cout << i->key << "=" << i->value << std::endl;
			if (i->rightComment != "") {
				//std::cout << i->rightComment << std::endl;
			}
		}
	}
	std::cout << "################ end ################" << std::endl;
	return;
}

bool IniFile::StartWith(const string &str, const string &prefix)
{
	if (strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0) {
		return true;
	}
	return false;
}

void IniFile::trimleft(string &str, char c /*=' '*/)
{
	int i = 0;
	int len = str.length();

	while (str[i] == c && str[i] != '\0') {
		i++;
	}

	if (i != 0) {
		str = string(str, i, len - i);
	}
}

void IniFile::trimright(string &str, char c /*=' '*/)
{
	int i = 0;
	int len = str.length();

	for (i = len - 1; i >= 0; --i) {
		if (str[i] != c) {
			break;
		}
	}

	str = string(str, 0, i + 1);
}

void IniFile::trim(string &str)
{
	int i = 0;
	int len = str.length();

	while ((i < len) && iswspace(str[i]) && (str[i] != '\0')) {
		i++;
	}

	if (i != 0) {
		str = string(str, i, len - i);
	}

	len = str.length();

	for (i = len - 1; i >= 0; --i) {
		if (!iswspace(str[i])) {
			break;
		}
	}

	str = string(str, 0, i + 1);
}

bool IniFile::split(const string &str, const string &sep, string *pleft, string *pright)
{
	size_t pos = str.find(sep);
	string left, right;

	if (pos != string::npos) {
		left = string(str, 0, pos);
		right = string(str, pos+1);

		trim(left);
		trim(right);
		*pleft = left;
		*pright = right;
		return true;
	} else {
		left = str;
		right = "";

		trim(left);
		*pleft = left;
		*pright = right;
		return false;
	}
}

bool IniFile::StringCmpIgnoreCase(const string &str1, const string &str2)
{
	string a = str1;
	string b = str2;
	transform(a.begin(), a.end(), a.begin(), ::toupper);
	transform(b.begin(), b.end(), b.begin(), ::toupper);

	return (a == b);
}
