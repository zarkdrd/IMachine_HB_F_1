/*************************************************************************
    > File Name: include/Get_Inifiles.h
    > Author: ARTC
    > Descripttion: 获取INI配置文件程序头文件
    > Created Time: 2024-04-11
 ************************************************************************/

#ifndef _INCLUDE_GET_PROFILES_LIST_H_
#define _INCLUDE_GET_PROFILES_LIST_H_

#include <vector>
#include <algorithm>
#include <string>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fstream>
#include <sstream>

using namespace std;

/**存放一个键值项内容结构体**/
struct IniItem{
    string key;             // 键名
    string value;           // 键值
    string comment;         // 键值注释，包括空行
    string rightComment;    // 行尾注释
};

/**存放一个段信息结构体**/
struct IniSection{
    string name;            // 段名
    string comment;         // 段注释，包括空行
    string rightComment;    // 行尾注释
    vector<IniItem> items;  // 键值项数组，一个段可以有多个键值，所有的键值项用vector来储存

    /**定义一个迭代器，即指向键值项元素的指针**/
    typedef vector<IniItem>::iterator IniItem_it;
    IniItem_it begin(){
        /**指向 items 元素向量的头指针**/
        return items.begin();
    }
    IniItem_it end(){
        /**指向 items 元素向量的尾指针**/
        return items.end();
    }
};

/**ini 配置文件类**/
class IniFile{
    public:
        IniFile();
        ~IniFile();
        static IniFile * getInstance();
    public:
        /* 打开并解析INI文件 */
        int Load(const string &fname);

        /* 打印当前INI配置文件所有的信息 */
        void PrintIniFile();

        /* 将缓冲区内容写入文件中 */
        bool Save();
        bool SaveAs(const string &fname);

        /* 将一个段名为section, 键名为key的值改为value， 若不存在对应的节或键名，则增加一个键值项，成功返回0，否则返回错误码 */
        int SetIntValue(const string &section, const string &key, int value);
        int SetBoolValue(const string &section, const string &key, bool value);
        int SetDoubleValue(const string &section, const string &key, double value);
        int SetStringValue(const string &section, const string &key, const string &value);

        /* 获取section段第一个键为key的值，成功返回读取到的值，否则返回默认值 */
        int GetIntValue(const string &section, const string &key, const int defaultValue);
        bool GetBoolValue(const string &section, const string &key, const bool defaultValue);
        double GetDoubleValue(const string &section, const string &key, const double defaultValue);
        string GetStringValue(const string &section, const string &key, const string defaultValue);

        /* 设置注释分隔符，默认为"#" */
        void SetCommentDelimiter(const string &delimiter);
        /* 设置键值项注释 */
        int SetComment(const string &section, const string &key, const string &comment);
        int SetRightComment(const string &section, const string &key, const string &rightComment);

        /* 删除对应段或者对应段的特定键值项 */
        void DeleteSection(const string &section);
        void DeleteKey(const string &section, const string &key);


    private:
        int GetSectionNum();
        bool HasSection(const string &section);
        int GetSections(vector<string> *sections);
        IniSection *getSection(const string &section = "");
        bool HasKey(const string &section, const string &key);
        int GetComment(const string &section, const string &key, string *comment);
        int GetValues(const string &section, const string &key, vector<string> *values);
        int GetRightComment(const string &section, const string &key, string *rightComment);

        void release();
        void trim(string &str);
        void trimleft(string &str, char c = ' ');
        void trimright(string &str, char c = ' ');
        bool IsCommentLine(const string &str);
        bool StartWith(const string &str, const string &prefix);
        bool parse(const string &content, string *key, string *value);
        bool StringCmpIgnoreCase(const string &str1, const string &str2);
        bool split(const string &str, const string &sep, string *left, string *right);

        int getValue(const string &section, const string &key, string *value);
        int getValue(const string &section, const string &key, string *value, string *comment);
        int setValue(const string &section, const string &key, const string &value, const string &comment = "");
        int GetValues(const string &section, const string &key, vector<string> *value, vector<string> *comments);
        int UpdateSection(const string &cleanLine, const string &comment, const string &rightComment, IniSection **section);
        int AddKeyValuePair(const string &cleanLine, const string &comment, const string &rightComment, IniSection *section);
    public:
        static IniFile* m_Instance;         // INI配置文件类单例
    private:
        string errMsg;                      // 保存出错时的错误信息
        string iniFilePath;                 // INI配置文件名
        string commentDelimiter;            // 设置注释的分隔符
        vector<IniSection *> sections_vt;   // 用于存储节集合的vector容器
        typedef vector<IniSection *>::iterator IniSection_it;
};

#endif //_INCLUDE_GET_PROFILES_LIST_H_
