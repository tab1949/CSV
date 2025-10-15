#ifndef CSV_HPP_
#define CSV_HPP_

#include <algorithm>
#include <any>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tabxx {

class CSV {
public:
    class Settings {
    public:
        enum LINE_ENDING {LF, CRLF, AUTO};

    public:
        Settings() : ending_(LF), separator_(','), auto_derive_type_(false), double_precision_(1) {}

        Settings(const Settings& s) : 
            ending_(s.ending_), 
            separator_(s.separator_), 
            auto_derive_type_(s.auto_derive_type_),
            double_precision_(s.double_precision_) {}

        Settings(Settings&& s) noexcept : Settings(s) {

        }

        Settings& setEnding(LINE_ENDING ending) {
            ending_ = ending;
            return *this;
        }

        LINE_ENDING getEnding() {
            return ending_;
        }

        Settings& setSeparator(char separator) {
            separator_ = separator;
            return *this;
        }

        char getSeparator() {
            return separator_;
        }

        Settings& setAutoDeriveType(bool opt) {
            auto_derive_type_ = opt;
            return *this;
        }

        Settings& setDoublePrecision(int p) {
            double_precision_ = p;
            return *this;
        }

    private:
        LINE_ENDING ending_;
        char separator_;
        bool auto_derive_type_;
        int double_precision_;
        friend class CSV;

    }; // class Settings

    class Line {
    private:
        Line() {}

    public:
        Line(const std::vector<std::any>& v) {

        }

    private:

        std::vector<std::any> data;
        friend class CSV;
    
    }; // class Line

    CSV(const Settings& settings = Settings()) : settings_(settings) {}

    CSV(std::istream& src, const Settings& s = Settings()) : CSV(s) {
        this->parse(src);
    }

    CSV(const std::string& str, const Settings& s = Settings()) : CSV(s) {
        std::istringstream iss(str);
        this->parse(iss);
    }

    CSV(const char* str, std::size_t size, const Settings& s = Settings()) : CSV(std::string(str, size), s) {}

    static CSV parse(const std::string& str, const Settings& settings = Settings()) {
        return CSV(str, settings);
    }

    static CSV parse(const char* str, std::size_t size, const Settings& settings = Settings()) {
        return CSV(str, size, settings);
    }

    std::size_t getColumnCount() {
        return title_.size();
    }

    std::size_t getRowCount() {
        return values_.size();
    }

    std::vector<std::string> getColumns() {
        return title_;
    }

    std::vector<std::any> getRow(std::size_t row) {
        return values_.at(row);
    }

    std::vector<std::any> getColumn(std::size_t column) {
        std::vector<std::any> ret;
        if (column <= title_.size())
            for (auto ite : values_)
                ret.emplace_back(ite.at(column));
        return ret;
    }

    std::vector<std::any> getColumn(const std::string& column) {
        return getColumn(searchTitle(column));
    }

    std::any& getValue(std::size_t column, std::size_t row) {
        return values_.at(row).at(column);
    }

    template <typename V>
    V getValue(std::size_t column, std::size_t row) {
        return std::any_cast<V>(getValue(column, row));
    }

    std::any& getValue(const std::string& column, std::size_t row) {
        return values_.at(row).at(searchTitle(column));
    }

    template <typename V>
    V getValue(const std::string& column, std::size_t row) {
        return std::any_cast<V>(getValue(column, row));
    }

    std::vector<std::string> getTitles() {
        return this->title_;
    }

    void setTitle(const std::vector<std::string>& t) {
        clear();
        title_ = t;
    }

    void setTitle(std::vector<std::string>&& t) {
        clear();
        title_ = std::move(t);
    }

    void addRow(const std::vector<std::any>& l) {
        if (l.size() != title_.size())
            throw std::runtime_error("tabxx::CSV::addRow(): Invalid value count");
        values_.push_back(l);
    }

    void addRow(std::vector<std::any>&& l) {
        if (l.size() != title_.size())
            throw std::runtime_error("tabxx::CSV::addRow(): Invalid value count");
        values_.emplace_back(std::move(l));
    }

    void removeRow(std::size_t index) {
        if (index >= values_.size())
            throw std::runtime_error("tabxx::CSV::removeRow(): Index out of range");
        values_.erase(values_.begin() + index);
    }

    void removeRow(std::size_t begin, std::size_t end) {
        if (begin >= values_.size() || end >= values_.size() || begin > end)
            throw std::runtime_error("tabxx::CSV::removeRow(): Invalid range");
        values_.erase(values_.begin() + begin, values_.begin() + end);
    }

    std::size_t searchTitle(const std::string& str) {
        std::size_t ret = 0;
        for (; ret < title_.size(); ++ret)
            if (title_.at(ret) == str)
                break;
        return ret;
    }

    bool empty() {
        return title_.empty();
    }

    void clear() {
        title_.clear();
        values_.clear();
    }

    Settings& settings() {
        return settings_;
    }

    std::size_t write(std::ostream& des) {
        std::size_t ret = 0;
        if (!empty()) {
            std::size_t lim = title_.size();
            for (std::size_t i = 0; i < lim; ++i) {
                des << title_[i];
                ret += title_[i].size();
                if (i < (lim - 1)) {
                    des << settings_.separator_;
                    ++ ret;
                }
            }
            if (settings_.ending_ == Settings::LF) {
                des << '\n';
                ret += 1;
            }
            else {
                des << "\r\n";
                ret += 2;
            }
            lim = values_.size();
            if (!settings_.auto_derive_type_) {
                for (std::size_t i = 0; i < lim; ++i) {
                    auto&& line = values_.at(i);
                    for (std::size_t j = 0; j < line.size(); ++j) {
                        auto&& str = std::any_cast<std::string>(line.at(j));
                        des << str;
                        ret += str.size();
                        if (j < line.size() - 1) {
                            des << settings_.separator_;
                            ret += 1;
                        }
                    }
                    if (settings_.ending_ == Settings::LF) {
                        des << '\n';
                        ret += 1;
                    }
                    else {
                        des << "\r\n";
                        ret += 2;
                    }
                }
            }
            else {
                for (std::size_t i = 0; i < lim; ++i) {
                    auto&& line = values_.at(i);
                    for (std::size_t j = 0; j < line.size(); ++j) {
                        std::string str(anyToString(line[j]));
                        des << str;
                        ret += str.size();
                        if (j < line.size() - 1) {
                            des << settings_.separator_;
                            ret += 1;
                        }
                    }
                    if (settings_.ending_ == Settings::LF) {
                        des << '\n';
                        ret += 1;
                    }
                    else {
                        des << "\r\n";
                        ret += 2;
                    }
                }
            }
        }
        return ret;
    }

private:
    std::any detectType(std::string&& str) {
        std::any ret;
        if (str.at(0) == '"' && str.at(str.size() - 1) == '"') {
            str.erase(str.size() - 1, 1);
            str.erase(0, 1);
        }
        bool isFloat = false;
        for (auto ch = str.begin(); ch != str.end(); ++ch) { // STRING (std::string)
            if (!('0' <= *ch && *ch <= '9')) {
                if (*ch == '.') {
                    if (!isFloat) {
                        isFloat = true;
                        continue;
                    }
                    else {
                        ret = std::move(str);
                        return ret;
                    }
                }
                else if (ch == str.begin() && (*ch == '+' || *ch == '-'))
                    continue;
                ret = std::move(str);
                return ret;
            }
        }
        if (isFloat) { // FLOAT (double)
            ret = std::stod(str);
        }
        else { // INT (long long)
            ret = std::stoll(str);
        }
        return ret;
    }

    std::string anyToString(std::any v) {
        if (v.type() == typeid(std::string))
            return std::any_cast<std::string>(v);
        if (v.type() == typeid(double)) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(settings_.double_precision_) << std::any_cast<double>(v);
            return ss.str();
        }
        if (v.type() == typeid(long long))
            return std::to_string(std::any_cast<long long>(v));
        if (v.type() == typeid(int))
            return std::to_string(std::any_cast<int>(v));
        if (v.type() == typeid(short))
            return std::to_string(std::any_cast<short>(v));
        if (v.type() == typeid(char))
            return std::to_string(std::any_cast<char>(v));
    }

    int parse(std::istream& src) {
        std::string line;
        if (settings_.ending_ == Settings::AUTO) {
            for (char ch = 0; !src.eof(); ) {
                ch = src.get();
                if (ch == '\r') {
                    settings_.ending_ = Settings::CRLF;
                    break;
                }
                else if (ch == '\n') {
                    settings_.ending_ = Settings::LF;
                    break;
                }
                else {
                    line.push_back(ch);
                }
            }
        }
        char delim_line = (settings_.ending_ == Settings::LF ? '\n' : '\r');

        if (line.empty())
            return -1;
        else if (delim_line == '\r')
            src.get(); // CRLF

        title_.clear();
        values_.clear();
        std::stringstream ss(line);
        for (; ; ) {
            std::string name;
            std::getline(ss, name, settings_.separator_);
            if (!ss.eof() || !name.empty())
                title_.emplace_back(name);
            else
                break;
        }

        for (; ; ) {
            std::getline(src, line, delim_line);
            if (line.empty())
                if (!src.eof())
                    throw std::runtime_error("tabxx::CSV::parse(std::istream&): Invalid Dataline (Empty Line)");
                else
                    break;
            if (delim_line == '\r')
                src.get();
            std::stringstream s(line);
            std::vector<std::any> row;
            for (; ; ) {
                std::string val;
                std::getline(s, val, settings_.separator_);
                if (settings_.auto_derive_type_)
                    row.emplace_back(detectType(std::move(val)));
                else
                    row.emplace_back(std::move(val));
                if (s.eof())
                    break;
            }
            if (row.size() != getColumnCount())
                throw std::runtime_error("tabxx::CSV::parse(std::istream&): Invalid Dataline");
            values_.emplace_back(std::move(row));
            if (src.eof())
                break;
        }
            
        return 0;
    }

private:
    Settings settings_;
    std::vector<std::string> title_;
    std::vector<std::vector<std::any>> values_;

}; // class CSV

} // namespace tabxx

#endif // CSV_HPP_
