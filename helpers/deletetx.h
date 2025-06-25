#pragma once
#include <cstdint>
#include <string>
#include <functional> //for std::hash
#include <vector>
#include <iostream>
#include "uint256.h"
using namespace std;


void delete_tx(string del_tx) 
{ 
  
    // Open FIle pointers 
    fstream fin, fout; 
  
    // Open the existing file 
    fin.open("debug_forensics.csv", ios::in); 
  
    // Create a new file to store the non-deleted data 
    fout.open("debug_forensicsnew.csv", ios::out); 
  
    string tx; 
    char sub; 
    int index, new_marks; 
    string line, word; 
    vector<string> row; 
   
    // Check if this record exists 
    // If exists, leave it and 
    // add all other data to the new file 
    while (!fin.eof()) { 
  
        row.clear(); 
        getline(fin, line); 
        stringstream s(line); 
  
        while (getline(s, word, ', ')) { 
            row.push_back(word); 
        } 
  
        int row_size = row.size(); 
        tx = stoi(row[0]); 
  
        // writing all records, 
        // except the record to be deleted, 
        // into the new file 'reportcardnew.csv' 
        // using fout pointer 
        if (tx != del_tx) { 
            if (!fin.eof()) { 
                for (i = 0; i < row_size - 1; i++) { 
                    fout << row[i] << ", "; 
                } 
                fout << row[row_size - 1] << "\n"; 
            } 
        } 
        else { 
            count = 1; 
        } 
        if (fin.eof()) 
            break; 
    } 
    if (count == 1) 
        cout << "Transaction deleted\n"; 
    else
        cout << "Transaction not found\n"; 
  
    // Close the pointers 
    fin.close(); 
    fout.close(); 
  
    // removing the existing file 
    remove("debug_forensics.csv"); 
  
    // renaming the new file with the existing file name 
    rename("debug_forensicsnew.csv", "debug_forensics.csv"); 
} 

