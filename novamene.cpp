// TODO CHANGE FILE PATHS IN CODE ACCORDING TO YOUR ENVIRONMENT. THE REST OF NOVAMENE CODE IS PORTABLE, JUST COPY THE PROJECT FOLDER AND RUN PREFERABLY IN CLION
// INCLUDING NECESSARY HEADER FILES
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include "bf/all.hpp"
#include "rapidjson/ostreamwrapper.h"
#include "helpers/primitives.h"

//NAMESPACE SCOPE
using namespace std;
using namespace bf;

//GLOBAL VARIABLES DEFINED, MOST NAMES ARE SELF EXPLANATORY
//COUNTERS FOR SANITY CHECKING, DEBUGGING AND RESULTS CALCULATION
int init=1, last_debug_time=0, last_expiry_time=0, last_clear_time=0, last_rotate_time=0, network_time=0, txpool_time=0;
int hour=0, rbf_missed=0, rbf_replaced=0, total_rbf, tp=0, tn=0, fp=0, fn=0, fp_entry=0, fn_entry=0, tp_entry=0, tn_entry=0,
    fp_exit=0, fn_exit=0, tp_exit=0, tp_net=0, tn_net=0, fp_net=0, fn_net=0, tp_ro=0, fp_ro=0, fn_ro=0, tn_ro=0;
int h_rbf_missed=0, h_rbf_replaced=0, h_rbf=0, h_tp=0, h_tn=0, h_fp=0, h_fn=0, h_fp_entry=0, h_fn_entry=0, h_tp_entry=0, h_tn_entry=0,
    h_fp_exit=0, h_fn_exit=0, h_tp_exit=0, h_tp_net=0, h_tn_net=0, h_fp_net=0, h_fn_net=0, h_tp_ro=0, h_fp_ro=0, h_fn_ro=0, h_tn_ro=0;
int h_unique_tx=0, h_total_tx=0, h_entry_tx=0, h_added_tx=0, h_exit_tx=0, h_block_tx=0, h_non_block=0, h_non_block_not_removed=0, h_old_tx=0, h_removed_tx=0, unique_network_fp=0, unique_network_fn=0;
int old_tx=0, entry_tx=0, exit_tx=0, current_txpool_tx=0, removed_tx=0, added_tx=0, total_tx=0, current_txpool_one=0, current_txpool_two=0, day=0, h_unique_network_fp=0, h_unique_network_fn=0;;
int ro_count=0, replace_tx=0, conflict_tx=0, evict_tx=0, reorg_tx=0, expire_tx=0, block_tx=0, non_block=0, unique_tx=0;
int max_bucket_count_cbf=0, max_bucket_count_txin=0, width=4, min_rbf_check=0 , h_network_tx=0;
int rbf_check, cbf_txpool_lookup, expiry_interval=48, non_block_not_removed=0;
int unique_fn=0, unique_fp=0, h_unique_fn=0, h_unique_fp=0, fn_index=0, h_fn_index=0, fn_index_net=0, fn_index_entry=0, fn_index_exit=0, h_fn_index_net=0, h_fn_index_entry=0, h_fn_index_exit=0;
string status;
string cm = "";
std::map <string, int> entry_positives, network_positives, network_negatives;
int hours_since_expiry=0, fn_expiry_net=0, fn_expiry_entry=0, fn_expiry_exit=0, fn_expiry=0, h_fn_expiry_net=0, h_fn_expiry_entry=0, h_fn_expiry_exit=0, h_fn_expiry=0;
int network_tx=0;
int cm_net=0, cm_entry=0, cm_exit=0, total=0;
int h_cm_net=0, h_cm_entry=0, h_cm_exit=0, h_total=0;
int  already_known_add=0, h_already_known_add=0, invalid_add=0, h_invalid_add=0, underpriced_add=0, h_underpriced_add=0, replaced_add=0, h_replaced_add=0;
int fairness_exceed_tp=0, h_fairness_exceed_tp=0, size_limit_tq=0, h_size_limit_tq=0, expired_du=0, h_expired_du=0, low_balance_du=0, h_low_balance_du=0;
int   pe=0, h_pe=0;

ofstream debug_hourly ("/debug/debug_hourly.csv" );
ofstream debug_cumulative ("debug/debug_cumulative.csv" );

//DETERMINISTIC
std::map <string, std::pair<int, double>> shadow_txpool;  // tx hash, unix time
std::map <string, int> shadow_TxID_cbfilter;  // tx hash, 1/2
std::map <string, int> shadow_network; // tx hash, unix time
std::map <string, int> shadow_address; // tx hash, unix time
//std::map <string, double> shadow_size; // tx hash, unix time
//PROBABILISTIC 
bf::counting_bloom_filter TxID_cbfilter_one(make_hasher(8), 4000000, width);
bf::counting_bloom_filter TxID_cbfilter_two(make_hasher(8), 4000000, width);
bf::counting_bloom_filter address(make_hasher(7), 2000000, width);

double sizeinbytes;

string confusion_matrix_txpool_entry(const string& txid, int shadow, int cbf ) {
string cm_return;

    if (shadow>0 && cbf>0) {
        tp_entry++;
        h_tp_entry++;
        cm_return="TP";
    }

    if (shadow==0 && cbf==0) {
            tn_entry++;
            h_tn_entry++;
            cm_return="TN";
    }

    if (shadow==0 && cbf>0) {
        fp_entry++;
        h_fp_entry++;
        cm_return="FP";
    }

    if (shadow>0 && cbf==0) {
        fn_entry++;
        h_fn_entry++;
        if (shadow_TxID_cbfilter.count(txid)>=1) {
             fn_index_entry++;
             h_fn_index_entry++;
        }
        cm_return="FN";
    }

    return cm_return;
}

string confusion_matrix_txpool_exit(const string& tx_reason, const string& txid, int shadow, int cbf ) {
string cm_return;

    if (shadow>0 && cbf>0) {
        tp_exit++;
        h_tp_exit++;
        cm_return="TP";
    }

    if (shadow==0 && cbf==0) {
        old_tx++;
        h_old_tx++;
        cm_return = "OLD";
    }

    if (shadow==0 && cbf>0) {
        if (tx_reason=="BLOCK") {
            fp_exit++;
            h_fp_exit++;
            cm_return = "FP";
        }
        else
            cm_return="TP";
    }

    if (shadow>0 && cbf==0) {
        if (tx_reason=="BLOCK") {
            fn_exit++;
            h_fn_exit++;
            if (shadow_txpool.count(txid)>=1) {
             fn_index_exit++;
             h_fn_index_exit++;
            }
            cm_return = "FN";
        }
        else
            cm_return="TP";
    }

    return cm_return;
}

string confusion_matrix_txpool_network(const string& txid, int shadow, int cbf ) {
    string cm_return;
// cm METRICS UPDATED
    if (shadow>0 && cbf>0) {
        tp_net++;
        h_tp_net++;
        cm_return="TP";
    }

    if (shadow==0 && cbf==0) {
        tn_net++;
        h_tn_net++;
        cm_return = "TN";
    }

    if (shadow==0 && cbf>0) {
        fp_net++;
        h_fp_net++;
        cm_return="FP";
        if (network_positives.count(txid) == 0) {
             unique_network_fp++;
             h_unique_network_fp++;
       }
       network_positives.insert(make_pair(txid, network_time));
        
    }
    if (shadow>0 && cbf==0) {
        fn_net++;
        h_fn_net++;
     
        cm_return="FN";
  if (network_negatives.count(txid) == 0) {
             unique_network_fn++;
             h_unique_network_fn++;
      if (shadow_TxID_cbfilter.count(txid)>=1) {
             fn_index_net++;
             h_fn_index_net++;
        }
       }
       network_negatives.insert(make_pair(txid, network_time));
    }

    return cm_return;
}

int last_shadow_txpool_expiry_time=0;

void initialize(int sim_time) {
    if (init == 1) {                              //initialize last expiry and debug time
        last_debug_time = sim_time;
        last_rotate_time=sim_time;
        last_clear_time=sim_time;
        last_expiry_time=sim_time;
        last_shadow_txpool_expiry_time=sim_time;
        init = 0;
    }
}

//MAIN FUNCTION BEGINS HERE
int main() {

    debug_cumulative << "hour"
                     << "," << "inventory_time"
                     << "," << "inventory_tx"
                     << "," << "cm inv"
                     << "," << "unique_tx"
                     << "," << "unique_fp"
                     << "," << "unique_fn"
                     << "," << "tp_inv"
                     << "," << "tn_inv"
                     << "," << "fp_inv"
                     << "," << "fn_inv"
                     << "," << "fn_index_inv"
                     << "," << "fn_expiry_inv"
                     << "," << "FPR inv"
                     << "," << "FNR inv"
                     << "," << "Accuracy % inv"
                     << "," << "Reprocess % inv"

                     << "," << "total_tx"
                     << "," << "cm total"
                     << "," << "entry_tx"
                     << "," << "cm entry"
                     << "," << "added_tx"
                     << "," << "tp_entry"
                     << "," << "tn_entry"
                     << "," << "fp_entry"
                     << "," << "fn_entry"
                     << "," << "fn_index_entry"
                     << "," << "fn_expiry_entry"
                     << "," << "FPR Entry"
                     << "," << "FNR Entry"
                     << "," << "Accuracy % Entry"
                     << "," << "Reprocess % Entry"

                     << "," << "exit_tx"
                     << "," << "cm exit"
                     << "," << "exited_tx"
                     << "," << "block_tx"
                     << "," << "debris"
                     << "," << "tp_exit"
                     << "," << "tn_exit"
                     << "," << "fp_exit"
                     << "," << "fn_exit"
                     << "," << "fn_index_exit"
                     << "," << "fn_expiry_exit"
                     << "," << "fn_index"
                     << "," << "fn_expiry"
                     << "," << "FPR Exit"
                     << "," << "FNR Exit"
                     << "," << "Accuracy % Exit"
                     << "," << "Reprocess % Exit"

                     << "," << "FPR"
                     << "," << "FNR"
                     << "," << "Accuracy %"
                     << "," << "Reprocess %"
                     << "," << "current_mempool_tx"
                     << "," << "shadow_mempool.size()"
                     << "," << "(fp_entry-fn_entry+fp_exit-fn_exit)"
                     << "," << "non_block"
                     << "," << "shadow check"
                     << "," << "debris check"
                     << endl;

    debug_hourly << "hour"
                 << "," << "inventory_time"
                 << "," << "inventory_tx"
                 << "," << "cm inv"
                 << "," << "unique_tx"
                 << "," << "unique_fp"
                 << "," << "unique_fn"
                 << "," << "tp_inv"
                 << "," << "tn_inv"
                 << "," << "fp_inv"
                 << "," << "fn_inv"
                 << "," << "fn_index_inv"
                 << "," << "fn_expiry_inv"
                 << "," << "FPR inv"
                 << "," << "FNR inv"
                 << "," << "Accuracy % inv"
                 << "," << "Reprocess % inv"


                 << "," << "total_tx"
                 << "," << "cm total"
                 << "," << "entry_tx"
                 << "," << "cm entry"
                 << "," << "added_tx"
                 << "," << "tp_entry"
                 << "," << "tn_entry"
                 << "," << "fp_entry"
                 << "," << "fn_entry"
                 << "," << "fn_index_entry"
                 << "," << "fn_expiry_entry"
                 << "," << "FPR Entry"
                 << "," << "FNR Entry"
                 << "," << "Accuracy % Entry"
                 << "," << "Reprocess % Entry"

                 << "," << "exit_tx"
                 << "," << "cm exit"
                 << "," << "exited_tx"
                 << "," << "block_tx"
                 << "," << "non_block"
                 << "," << "tp_exit"
                 << "," << "tn_exit"
                 << "," << "fp_exit"
                 << "," << "fn_exit"
                 << "," << "fn_index_exit"
                 << "," << "fn_expiry_exit"
                 << "," << "fn_index"
                 << "," << "fn_expiry"
                 << "," << "FPR Exit"
                 << "," << "FNR Exit"
                 << "," << "Accuracy % Exit"
                 << "," << "Reprocess % Exit"

                 << "," << "FPR"
                 << "," << "FNR"
                 << "," << "Accuracy %"
                 << "," << "Reprocess %"
                 << "," << "current_mempool_tx"
                 << "," << "shadow_mempool.size()"
                 << "," << "shadow size in bytes"
                 << endl;

    std::cout << "Begin." << endl;

    fstream entry;
    fstream exit;
    string csv_entry = string("/dataset/entry.csv");
    string csv_exit = string("/dataset/exit.csv");
    entry.open(csv_entry, ios::in); //open a file to perform read operation using file object
    exit.open(csv_exit, ios::in);

    string entry_line, exit_line;
    string entry_type, exit_type;
    string entry_hash, exit_hash;
    double entry_size, exit_size;
    string entry_sender, exit_reason;
    int entry_nonce, exit_nonce;
    int entry_time, exit_time;
    int active_filter = 1;

    getline(entry, entry_line); // read data from file object and put it into string.
    getline(exit, exit_line); // read data from file object and put it into string.

    // C++ snippet to parse a comma-separated string

    vector<string> vec_entry, vec_exit;
    stringstream ss_entry(entry_line);
    stringstream ss_exit(exit_line);

    while (ss_entry.good()) {
        string substr;
        getline(ss_entry, substr, ',');
        vec_entry.push_back(substr);
    }

    //   for (size_t i = 0; i < vec_entry.size(); i++)
    //     cout << vec_entry[i] << endl;

    while (ss_exit.good()) {
        string substr;
        getline(ss_exit, substr, ',');
        vec_exit.push_back(substr);
    }

    //  for (size_t i = 0; i < vec_exit.size(); i++)
    //      cout << vec_exit[i] << endl;

    entry_time = stoi(vec_entry[0]);
    cout << entry_time << endl;
    exit_time = stoi(vec_exit[0]);
    cout << exit_time << endl;
    initialize(entry_time);

    while (entry.is_open() && exit.is_open()) {   //checking whether the file is open


        if (txpool_time >= (last_rotate_time + 3600)) {  // rotate
            last_rotate_time = txpool_time;

            if ((hour % (expiry_interval*2) == 0) ){

                    TxID_cbfilter_one.clear();
                    current_txpool_one = 0;

                    auto sc = shadow_TxID_cbfilter.begin();
                    while (sc != shadow_TxID_cbfilter.end()) {
                        if (sc->second == 1)   // Check if value of this entry matches with given value
                            shadow_TxID_cbfilter.erase(sc->first); // erase
                        sc++;
                    }

                    active_filter=1;


            } else if ((hour % expiry_interval) == 0 ) {

                    TxID_cbfilter_two.clear();
                    current_txpool_two = 0;

                    auto sc = shadow_TxID_cbfilter.begin();
                    while (sc != shadow_TxID_cbfilter.end()) {
                        if (sc->second == 2)   // Check if value of this entry matches with given value
                            shadow_TxID_cbfilter.erase(sc->first); // erase
                        sc++;
                    }

                    active_filter=2;
            }

        }

      /*  if (txpool_time - (3600 * expiry_interval * 2) > last_expiry_time) {

            auto ro = shadow_address.begin();
            while (ro != shadow_address.end()) {
                if (ro->second < (txpool_time - (3600 * expiry_interval * 2)))   // Check if value of this entry matches with given value
                    shadow_address.erase(ro->first); // erase

                ro++;
            }

            auto si = shadow_network.begin();
            while (si != shadow_network.end()) {
                if (si->second < (txpool_time - (3600 * expiry_interval * 2)))   // Check if value of this entry matches with given value
                    shadow_network.erase(si->first); // erase
                si++;
            }

        } */

       if (txpool_time > last_shadow_txpool_expiry_time + 3600) {
            auto sm = shadow_txpool.begin();
            while (sm != shadow_txpool.end()) {
                if (sm->second.first < (txpool_time - 3600 * expiry_interval*2 )) {  // Check if value of this entry matches with given value

                            exit_size = sm->second.second;
                            sizeinbytes = sizeinbytes - exit_size;
                            shadow_txpool.erase(sm->first); // erase
                }
                sm++;
            }

            last_shadow_txpool_expiry_time=txpool_time;
        }


        if (entry_time <= exit_time) {
            getline(entry, entry_line);

            // C++ program to parse a comma-separated string ENTRY
            vector<string> vec_entry;
            stringstream ss_entry(entry_line);

            while (ss_entry.good()) {
                string substr;
                getline(ss_entry, substr, ',');
                vec_entry.push_back(substr);
            }

            //        for (size_t i = 0; i < vec_entry.size(); i++)
            //          cout << vec_entry[i] << endl;

            entry_time = stoi(vec_entry[0]);
            entry_type = vec_entry[1];
            entry_hash = vec_entry[3];
            entry_sender = vec_entry[4];
            entry_size = stod(vec_entry[5]);
            entry_nonce = stoi(vec_entry[6]);

            // C++ program to parse a comma-separated string ENTRY


            cout << entry_time << " " << entry_type << " " << entry_hash << " " << entry_sender << " " << entry_nonce
                 << " " << entry_size << endl;
         //   cout << total_tx << endl;


            if (entry_type == "DISCARD") {

                total_tx++;
                h_total_tx++;
                txpool_time = entry_time;
                network_tx++;
                h_network_tx++;

                //   if (entry_ty == "ALREADY_KNOWN_add") {
                already_known_add++;
                h_already_known_add++;

                int TxID_cbfilter_lookup;
                int TxID_cbfilter_lookup_one = TxID_cbfilter_one.lookup(entry_hash);
                int TxID_cbfilter_lookup_two = TxID_cbfilter_two.lookup(entry_hash);

                if (TxID_cbfilter_lookup_one >= TxID_cbfilter_lookup_two)
                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_one;
                else
                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_two;

                int shadow_txpool_lookup = shadow_txpool.count(entry_hash);
                int shadow_network_lookup = shadow_network.count(entry_hash);

                if (shadow_network_lookup == 0) {
                    shadow_network.insert(make_pair(entry_hash, entry_time));
                    unique_tx++;
                    h_unique_tx++;
                }

                confusion_matrix_txpool_network(entry_hash, shadow_txpool_lookup, TxID_cbfilter_lookup);


            } else if (entry_type == "ENTRY") {

                txpool_time = entry_time;
                total_tx++;
                h_total_tx++;
                entry_tx++;
                h_entry_tx++;

                int TxID_cbfilter_lookup;
                int TxID_cbfilter_lookup_one = TxID_cbfilter_one.lookup(entry_hash);
                int TxID_cbfilter_lookup_two = TxID_cbfilter_two.lookup(entry_hash);

                if (TxID_cbfilter_lookup_one >= TxID_cbfilter_lookup_two)
                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_one;
                else
                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_two;

                int shadow_txpool_lookup = shadow_txpool.count(entry_hash);

                if (shadow_txpool_lookup==0) {
                    sizeinbytes = sizeinbytes + entry_size;
                }

                confusion_matrix_txpool_entry(entry_hash, shadow_txpool_lookup, TxID_cbfilter_lookup);

                shadow_txpool.insert(make_pair(entry_hash, make_pair(entry_time, entry_size)));

                if (TxID_cbfilter_lookup == 0) {

                    if (active_filter == 1) {                  //14
                        TxID_cbfilter_one.add(entry_hash);
                        current_txpool_one++;
                        shadow_TxID_cbfilter.insert(make_pair(entry_hash, 1));

                    } else if (active_filter == 2) {        //14
                        TxID_cbfilter_two.add(entry_hash);
                        current_txpool_two++;
                        shadow_TxID_cbfilter.insert(make_pair(entry_hash, 2));
                    }

                    added_tx++;
                    h_added_tx++;

                }
            }
        } else {

            getline(exit, exit_line);

            // C++ program to parse a comma-separated string EXIT

            stringstream ss_exit(exit_line);
            vector<string> vec_exit;

            while (ss_exit.good()) {
                string substr;
                getline(ss_exit, substr, ',');
                vec_exit.push_back(substr);
            }

            //    for (size_t i = 0; i < vec_exit.size(); i++)
            //   cout << vec_exit[i] << endl;

            exit_time = stoi(vec_exit[0]);
            exit_type = vec_exit[1];
            exit_reason = vec_exit[2];
            exit_hash = vec_exit[3];
            exit_size = stod(vec_exit[5]);
            exit_nonce = stoi(vec_exit[6]);

            cout << exit_time << " " << exit_type << " " << exit_hash << " " << exit_reason << " " << exit_nonce << " "
                 << exit_size << endl;

            if (rand() % 6   !=0) {

                exit_tx++;                                //INCREMENT EXIT TRANSACTIONS COUNTER
                total_tx++;
                h_total_tx++;
                h_exit_tx++;

                txpool_time = exit_time;

                int TxID_cbfilter_lookup = 0;
                int TxID_cbfilter_lookup_one = TxID_cbfilter_one.lookup(exit_hash);
                int TxID_cbfilter_lookup_two = TxID_cbfilter_two.lookup(exit_hash);

                if (TxID_cbfilter_lookup_one >= TxID_cbfilter_lookup_two)
                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_one;
                else

                    TxID_cbfilter_lookup = TxID_cbfilter_lookup_two;

                int shadow_txpool_lookup = shadow_txpool.count(exit_hash);

                confusion_matrix_txpool_exit(exit_reason, exit_hash, shadow_txpool_lookup,
                                             TxID_cbfilter_lookup);

                if (shadow_txpool_lookup > 0) {
                    sizeinbytes = sizeinbytes - exit_size;
                    shadow_txpool.erase(exit_hash);
                }

                //  if (TxID_cbfilter_lookup > 0) {

                //IF REASON IS BLOCK THEN
                //    if (exit_reason == "EXPIRED_DU" || exit_reason == "LOW_BALANC" || exit_reason=="FAIRNESS_E") {
                if (TxID_cbfilter_lookup_one > 0) {
                    TxID_cbfilter_one.remove(exit_hash);
                    current_txpool_one--;
                    removed_tx++;
                    h_removed_tx++;

                } else if (TxID_cbfilter_lookup_two > 0) {
                    TxID_cbfilter_two.remove(exit_hash);
                    current_txpool_two--;
                    removed_tx++;
                    h_removed_tx++;
                }
            }
        }

        if (txpool_time - last_debug_time >= 3600) {

            tp = tp_entry + tp_exit + tp_net;
            h_tp = h_tp_entry + h_tp_exit + h_tp_net;

            tn = tn_entry + tn_net;
            h_tn = h_tn_entry + h_tn_net;

            fp = fp_entry + fp_exit + fp_net;
            h_fp = h_fp_entry + h_fp_exit + h_fp_net;

            fn = fn_entry + fn_exit + fn_net;
            h_fn = h_fn_entry + h_fn_exit + h_fn_net;


            // unique_fp=unique_network_fp+fp_entry+fp_exit;
            // unique_fn= unique_network_fn+fn_entry+fn_exit;
            // h_unique_fp=h_unique_network_fp+h_fp_entry+h_fp_exit;
            // h_unique_fn= h_unique_network_fn+h_fn_entry+h_fn_exit;

            fn_index = fn_index_net + fn_index_exit + fn_index_entry;
            h_fn_index = h_fn_index_net + h_fn_index_exit + h_fn_index_entry;

            fn_expiry_net = unique_network_fn - fn_index_net;
            fn_expiry_entry = fn_entry - fn_index_entry;
            fn_expiry_exit = fn_exit - fn_index_exit;
            fn_expiry = fn_expiry_entry + fn_expiry_exit + fn_expiry_net;

            h_fn_expiry_net = h_unique_network_fn - h_fn_index_net;
            h_fn_expiry_entry = h_fn_entry - h_fn_index_entry;
            h_fn_expiry_exit = h_fn_exit - h_fn_index_exit;
            h_fn_expiry = h_fn_expiry_entry + h_fn_expiry_exit + h_fn_expiry_net;

            cm_net = tp_net + tn_net + fp_net + fn_net;
            cm_entry = tp_entry + tn_entry + fp_entry + fn_entry;
            cm_exit = tp_exit + fp_exit + fn_exit;
            total = (total_tx - old_tx);

            h_cm_net = h_tp_net + h_tn_net + h_fp_net + h_fn_net;
            h_cm_entry = h_tp_entry + h_tn_entry + h_fp_entry + h_fn_entry;
            h_cm_exit = h_tp_exit + h_fp_exit + h_fn_exit;
            h_total = (h_total_tx - h_old_tx);

            current_txpool_tx = current_txpool_two + current_txpool_one;

            last_debug_time = txpool_time;
            hour++;

            debug_cumulative << hour << "," << (3600 * hour)
                             << "," << network_tx
                             << "," << cm_net
                             << "," << unique_tx
                             << "," << unique_network_fp
                             << "," << unique_network_fn
                             << "," << tp_net
                             << "," << tn_net
                             << "," << fp_net
                             << "," << fn_net
                             << "," << fn_index_net
                             << "," << fn_expiry_net
                             << "," << float(fp_net) / float(cm_net)  //fpr
                             << "," << float(fn_net) / float(cm_net)//fnr
                             << "," << float(cm_net - fp_net) / float(cm_net) * 100// accuracy
                             << "," << float(fn_net) / float(cm_net) * 100//reprocess

                             << "," << total_tx
                             << "," << cm_entry
                             << "," << entry_tx
                             << "," << tp_entry + tn_entry + fp_entry + fn_entry
                             << "," << added_tx
                             << "," << tp_entry
                             << "," << tn_entry
                             << "," << fp_entry
                             << "," << fn_entry
                             << "," << fn_index_entry
                             << "," << fn_expiry_entry
                             << "," << float(fp_entry) / float(cm_entry) //fpr
                             << "," << float(fn_entry) / float(cm_entry)//fnr
                             << "," << float(cm_entry - fp_entry) / float(cm_entry) * 100// accuracy
                             << "," << float(fn_entry) / float(cm_entry) * 100//reprocess

                             << "," << exit_tx
                             << "," << cm_exit
                             << "," << removed_tx
                             << "," << block_tx
                             << "," << non_block
                             << "," << tp_exit
                             << "," << old_tx
                             << "," << fp_exit
                             << "," << fn_exit
                             << "," << fn_index_exit
                             << "," << fn_expiry_exit
                             << "," << fn_index
                             << "," << fn_expiry
                             << "," << float(fp_exit) / float(cm_exit) //fpr
                             << "," << float(fn_exit) / float(cm_exit) //fnr
                             << "," << float(cm_exit - fp_exit) / float(cm_exit) * 100// accuracy
                             << "," << float(fn_exit) / float(cm_exit) * 100//reprocess

                             << "," << float(fp) / float(total)  //fpr
                             << "," << float(fn) / float(total) //fnr
                             << "," << float(total - fp_net - fp_entry) / float(total) * 100// accuracy
                             << "," << float(fn_net + fn_entry) / float(cm_net + cm_entry) * 100//reprocess

                             << "," << current_txpool_tx
                             << "," << shadow_txpool.size()
                             << "," << (fp_entry - fn_entry + fp_exit - fn_exit)
                             << "," << non_block
                             << "," << current_txpool_tx - non_block + (fp_entry - fn_entry + fp_exit - fn_exit)
                             << "," << current_txpool_tx - shadow_txpool.size() +
                                       (fp_entry - fn_entry + fp_exit - fn_exit)
                             << "," << sizeinbytes
                             << "," << current_txpool_one
                             << "," << current_txpool_two
                             << "," << active_filter

                             << endl;

            rbf_replaced = total_rbf - rbf_missed;
            h_rbf_replaced = h_rbf - h_rbf_missed;

            ro_count = 0;

            debug_hourly << hour << "," << (3600 * hour)
                         << "," << network_tx
                         << "," << h_tp_net + h_tn_net + h_fp_net + h_fn_net
                         << "," << h_unique_tx
                         << "," << h_unique_network_fp
                         << "," << h_unique_network_fn
                         << "," << h_tp_net
                         << "," << h_tn_net
                         << "," << h_fp_net
                         << "," << h_fn_net
                         << "," << h_fn_index_net
                         << "," << h_fn_expiry_net
                         << "," << float(h_fp_net) / float(h_cm_net)  //fpr
                         << "," << float(h_fn_net) / float(h_cm_net)//fnr
                         << "," << float(h_cm_net - h_fp_net) / float(h_cm_net) * 100// accuracy
                         << "," << float(h_fn_net) / float(h_cm_net) * 100//reprocess

                         << "," << h_total_tx
                         << "," << cm_entry + cm_exit
                         << "," << h_entry_tx
                         << "," << cm_entry
                         << "," << h_added_tx
                         << "," << h_tp_entry
                         << "," << h_tn_entry
                         << "," << h_fp_entry
                         << "," << h_fn_entry
                         << "," << h_fn_index_entry
                         << "," << h_fn_expiry_entry
                         << "," << float(h_fp_entry) / float(h_cm_entry) //fpr
                         << "," << float(h_fn_entry) / float(h_cm_entry)//fnr
                         << "," << float(h_cm_entry - h_fp_entry) / float(h_cm_entry) * 100// accuracy
                         << "," << float(h_fn_entry) / float(h_cm_entry) * 100//reprocess

                         << "," << h_exit_tx
                         << "," << h_cm_exit
                         << "," << h_removed_tx
                         << "," << h_block_tx
                         << "," << h_non_block
                         << "," << h_tp_exit
                         << "," << h_old_tx
                         << "," << h_fp_exit
                         << "," << h_fn_exit
                         << "," << h_fn_index_exit
                         << "," << h_fn_expiry_exit
                         << "," << h_fn_index
                         << "," << h_fn_expiry
                         << "," << float(h_fp_exit) / float(h_cm_exit) //fpr
                         << "," << float(h_fn_exit) / float(h_cm_exit) //fnr
                         << "," << float(h_cm_exit - h_fp_exit) / float(h_cm_exit) * 100// accuracy
                         << "," << float(h_fn_exit) / float(h_cm_exit) * 100//reprocess


                         << "," << float(h_fp) / float(h_cm_net + h_cm_entry + h_cm_exit)  //fpr
                         << "," << float(h_fn) / float(h_cm_net + h_cm_entry + h_cm_exit) //fnr
                         << ","
                         << float(h_cm_net - h_fp_net - h_fp_entry) / float(h_cm_net) * 100// accuracy
                         << ","
                         << float(h_fn_net + h_fn_entry) / float(h_cm_net + h_cm_entry) * 100//reprocess
                         << "," << current_txpool_tx
                         << "," << shadow_txpool.size()
                         << "," << sizeinbytes
                         << endl;

            h_rbf_missed = 0, h_rbf_replaced = 0, h_rbf = 0, h_tp = 0, h_tn = 0, h_fp = 0, h_fn = 0, h_fp_entry = 0, h_fn_entry = 0, h_tp_entry = 0, h_tn_entry = 0,
            h_fp_exit = 0, h_fn_exit = 0, h_tp_exit = 0, h_tp_net = 0, h_tn_net = 0, h_fp_net = 0, h_fn_net = 0, h_tp_ro = 0, h_fp_ro = 0, h_fn_ro = 0, h_tn_ro = 0,
            h_unique_tx = 0, h_total_tx = 0, h_old_tx = 0, h_entry_tx = 0, h_added_tx = 0, h_entry_tx = 0, h_added_tx = 0, h_exit_tx = 0, h_network_tx = 0,
            h_block_tx = 0, h_removed_tx = 0, h_exit_tx = 0, h_removed_tx = 0, h_old_tx = 0, h_non_block = 0, h_non_block_not_removed = 0, h_fn_index = 0;
            h_fn_expiry_net = 0, h_fn_expiry_entry = 0, h_fn_expiry_exit = 0, h_fn_expiry = 0;

            h_fn_index = 0, h_fn_index_net = 0, h_fn_index_exit = 0, h_fn_index_entry = 0;
            h_fn_expiry_net = 0, h_unique_network_fn = 0, h_fn_index_net = 0;
            h_fn_expiry_entry = 0, h_fn_entry = 0, h_fn_index_entry = 0;
            h_fn_expiry_exit = 0, h_fn_exit = 0, h_fn_index_exit = 0;
            h_fn_expiry = 0, h_fn = 0, h_fn_index = 0;

            h_unique_fp = 0, h_unique_network_fp = 0, h_fp_entry = 0, h_fp_exit = 0;
            h_unique_fn = 0, h_unique_network_fn = 0, h_fn_entry = 0, h_fn_exit = 0;

            h_invalid_add = 0, h_already_known_add = 0, h_underpriced_add = 0, h_replaced_add = 0;
            h_fairness_exceed_tp = 0, h_size_limit_tq = 0, h_expired_du = 0, h_low_balance_du = 0, h_pe = 0;
         }
    }

    return 0;
}
