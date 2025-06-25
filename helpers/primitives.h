#pragma once
#include <cstdint>
#include <string>
#include <functional> //for std::hash
#include <vector>
#include <iostream>
#include "uint256.h"
using namespace std;

class Entry_Tx{
    string type;
    string hash;
    string sender;
    float size;
    double value;
    uint64_t gas;
    uint64_t gasPrice;
    uint64_t nonce;
    uint64_t time;

public:
    Entry_Tx(){}

    Entry_Tx(string type,
             string hash,
             string sender,
             float size,
             double value,
             uint64_t gasPrice,
             uint64_t gas,
             uint64_t nonce,
             uint64_t time) : type(type),
                        hash(hash),
                        sender(sender),
                        size(size),
        value(value),
        gasPrice(gasPrice),
        gas(gas),
        nonce(nonce),
        time(time)
        {

    }

    const string &getType() const {
        return type;
    }

  string getHash() const {
        return hash;
    }
    string getSender() const {

        return sender;
    }
    float getSize() const {
        return size;
    }

    uint64_t getValue() const {
        return value;
    }

    uint64_t getGas() const {
        return gas;
    }

    uint64_t getGasPrice() const {
        return gasPrice;
    }

    uint64_t getTime() const {
        return time;
    }
    uint64_t getNonce() const {
        return nonce;
    }


    string toString(){
        std::string log = "{\n"
                          + string("\t\"type\": ") + "\""+this->getType()+"\"" + ",\n"
                          + string("\t\"sender\": ") + "\"" + this->getSender() + "\"" + ",\n"
                          + string("\t\"hash\": ") + "\"" + this->getHash() + "\"" + ",\n"
                          + string("\t\"time\": ") + to_string(this->getTime()) + ",\n"
                          + string("\t\"size\": ") + to_string(this->getSize()) + ",\n"
                          + string("\t\"value\": ") + to_string(this->getValue()) + ",\n"
                          + string("\t\"gas\":") + to_string(this->getGas()) + ",\n"
                          + string("\t\"fee\":") + to_string(this->getGasPrice()) + ",\n"
                          + string("\t\"nonce\":") + to_string(this->getNonce()) + ",\n"
                          + "}\n\n";
        return log;
    }

    bool operator < (const Entry_Tx& othertx) const
    {

        return this->getHash()< othertx.getHash();

    }

};


class Discard_Tx{
    string type;
    string hash;
    string reason;
    string sender;
    float size;
    double value;
    uint64_t gas;
    uint64_t gasPrice;
    uint64_t time;
    uint64_t nonce;

public:
    Discard_Tx(){}

    Discard_Tx(
       string type,
       string hash,
       string reason,
       string sender,
       float size,
       double value,
       uint64_t gasPrice,
       uint64_t gas,
       uint64_t nonce,
       uint64_t time) : type(type),
                        hash(hash),
                        reason(reason),
                        sender(sender),
                        size(size),
                        value(value),
                        gasPrice(gasPrice),
                        gas(gas),
                        nonce(nonce),
                        time(time)
    {

    }

    const string &getType() const {
        return type;
    }

    string getHash() const {
        return hash;
    }

    //function for reason of EXIT Transaction
    string getReason() const{
        return reason;
    }

    string getSender() const{
        return sender;
    }

    float getSize() const {
        return size;
    }

    uint64_t getValue() const {
        return value;
    }

    uint64_t getGas() const {
        return gas;
    }

    uint64_t getGasPrice() const {
        return gasPrice;
    }

    uint64_t getTime() const {
        return time;
    }
    uint64_t getNonce() const {
        return nonce;
    }

    string toString(){
        std::string log = "{\n"
                          + string("\t\"type\": ") + "\""+this->getType()+"\"" + ",\n"
                          + string("\t\"hash\": ") + "\"" + this->getHash() + "\"" + ",\n"

                          + string("\t\"reason\": ") + "\"" + this->getReason() + "\"" + ",\n"
                          + string("\t\"sender\": ") + "\"" + this->getSender() + "\"" + ",\n"
                          + string("\t\"time\": ") + to_string(this->getTime()) + ",\n"
                          + string("\t\"size\": ") + to_string(this->getSize()) + ",\n"
                          + string("\t\"value\": ") + to_string(this->getValue()) + ",\n"
                          + string("\t\"gas\":") + to_string(this->getGas()) + ",\n"
                          + string("\t\"fee\":") + to_string(this->getGasPrice()) + ",\n"
                          + string("\t\"nonce\":") + to_string(this->getNonce()) + ",\n"
                          + "}\n\n";
        return log;
    }

    bool operator < (const Discard_Tx& othertx) const
    {

        return this->getHash()< othertx.getHash();

    }

};


class Exit_Tx{
    string type;
    string hash;
    string reason;
    float size;
    double value;
    uint64_t gasPrice;
    uint64_t gas;
    uint64_t nonce;
    uint64_t time;


public:
    Exit_Tx(){}

    Exit_Tx(string type,
       string hash,
       string reason,
       float size,
       double value,
       uint64_t gasPrice,
       uint64_t gas,
       uint64_t nonce,
       uint64_t time) : type(type),
                        hash(hash),
                        reason(reason),
                        size(size),
                        value(value),
                        gasPrice(gasPrice),
                        gas(gas),
                        nonce(nonce),
		                time(time)
    {

    }

    const string &getType() const {
        return type;
    }

    string getHash() const {
        return hash;
    }

    //function for reason of EXIT Transaction
    string getReason() const{
        return reason;
    }
    float getSize() const {
        return size;
    }

    uint64_t getValue() const {
        return value;
    }

    uint64_t getGas() const {
        return gas;
    }

    uint64_t getGasPrice() const {
        return gasPrice;
    }

    uint64_t getTime() const {
        return time;
    }
    uint64_t getNonce() const {
        return nonce;
    }

    string toString(){
        std::string log = "{\n"
                          + string("\t\"type\": ") + "\""+this->getType()+"\"" + ",\n"
                          + string("\t\"hash\": ") + "\"" + this->getHash() + "\"" + ",\n"
                          + string("\t\"time\": ") + to_string(this->getTime()) + ",\n"
                          + string("\t\"size\": ") + to_string(this->getSize()) + ",\n"
                          + string("\t\"value\": ") + to_string(this->getValue()) + ",\n"
                          + string("\t\"gas\":") + to_string(this->getGas()) + ",\n"
                          + string("\t\"gasprice\":") + to_string(this->getGasPrice()) + ",\n"
                          + string("\t\"nonce\":") + to_string(this->getNonce()) + ",\n"
                          + "}\n\n";
        return log;
    }

    bool operator < (const Exit_Tx& othertx) const
    {

        return this->getHash()< othertx.getHash();

    }

};
