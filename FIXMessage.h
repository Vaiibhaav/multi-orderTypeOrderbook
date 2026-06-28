#pragma once
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <stdexcept>

class FIXMessage {
private:
    std::map<int, std::string> fields_;

public:
    void setField(int tag, const std::string& value) {
        fields_[tag] = value;
    }

    void setField(int tag, int value) {
        fields_[tag] = std::to_string(value);
    }

    void setField(int tag, double value) {
        fields_[tag] = std::to_string(value);
    }

    std::string getField(int tag) const {
        auto it = fields_.find(tag);
        if (it == fields_.end()) {
            throw std::runtime_error("Tag " + std::to_string(tag) + " not found in FIX message");
        }
        return it->second;
    }

    int getIntField(int tag) const {
        return std::stoi(getField(tag));
    }

    bool hasField(int tag) const {
        return fields_.find(tag) != fields_.end();
    }

    std::string getMsgType() const {
        return getField(35);
    }

    std::string serialize() const {
        std::ostringstream oss;
        // Always write tag 8 first as per FIX standard (simulated)
        auto it8 = fields_.find(8);
        if (it8 != fields_.end()) {
            oss << "8=" << it8->second << "|";
        }
        
        // Write tag 35 second
        auto it35 = fields_.find(35);
        if (it35 != fields_.end()) {
            oss << "35=" << it35->second << "|";
        }

        for (const auto& [tag, val] : fields_) {
            if (tag == 8 || tag == 35) continue;
            oss << tag << "=" << val << "|";
        }
        return oss.str();
    }

    static FIXMessage parse(const std::string& raw) {
        FIXMessage msg;
        std::string token;
        std::istringstream tokenStream(raw);
        while (std::getline(tokenStream, token, '|')) {
            if (token.empty()) continue;
            size_t eq = token.find('=');
            if (eq == std::string::npos) continue;
            int tag = std::stoi(token.substr(0, eq));
            std::string val = token.substr(eq + 1);
            msg.setField(tag, val);
        }
        return msg;
    }
};
