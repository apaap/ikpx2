#pragma once
#include "isosat.hpp"
#include "ikpxtree.hpp"
#include "banner.hpp"
#include "../cqueue/blockingconcurrentqueue.h"

#include <chrono>

#define COMMAND_TERMINATE 2

struct workitem {

    u64seq initial_rows;
    int16_t exhausted_width;
    int16_t maximum_width;
    int16_t lookahead;
    int16_t direction;

};

typedef moodycamel::BlockingConcurrentQueue<workitem> WorkQueue;

void worker_loop(WorkQueue *from_master, WorkQueue *to_master, Velocity vel, std::vector<int> prime_implicants) {

    while (true) {

        workitem item;
        from_master->wait_dequeue(item);

        if (item.direction == COMMAND_TERMINATE) { break; }

        MetaProblem mp(item.initial_rows, vel);

        mp.find_all_solutions(item.maximum_width, item.exhausted_width,
            prime_implicants, item.lookahead, [&](const u64seq &svec) {

            workitem item2;

            item2.initial_rows = svec;
            item2.exhausted_width = item.maximum_width;
            item2.maximum_width = 0;
            item2.lookahead = 0;
            item2.direction = item.direction;

            to_master->enqueue(item2);

        });

        // Inform the master that the work has been completed:
        to_master->enqueue(item);
    }

}


struct semisearch {

    Velocity vel;
    ikpxtree tree;
    int direction;
    lab32_t *lab;
    int search_width;
    int lookahead;
    int jumpahead;

    int items_in_aether;
    uint32_t record_depth;
    std::vector<int> truth_table;
    std::vector<int> prime_implicants;
    WorkQueue from_master;
    std::vector<std::thread> workers;

    semisearch(const Velocity &vel, int direction, lab32_t *lab, int search_width, int lookahead, int jumpahead) :
        vel(vel), tree(vel.vradius() * 2), direction(direction), lab(lab),
        search_width(search_width), lookahead(lookahead), jumpahead(jumpahead) {

        search_width = 2;
        std::string rule = apg::get_all_rules()[0];
        truth_table = truth_table_for_rule(lab, rule);
        prime_implicants = truth_table_to_prime_implicants(truth_table);

        items_in_aether = 0;
        record_depth = 0;
    }

    void enqueue(const u64seq &elem, int exhausted_width) {

        if (exhausted_width >= search_width) { return; }

        workitem item;

        item.initial_rows = elem;
        item.exhausted_width = exhausted_width;
        item.maximum_width = search_width;
        item.lookahead = lookahead;
        item.direction = direction;

        from_master.enqueue(item);
        items_in_aether += 1;
    }

    void adaptive_widen() {

        search_width += 1;

        std::cout << "Adaptive widening to width " << search_width << std::endl;

        for (auto it = tree.preds.begin(); it != tree.preds.end(); ++it) {

            enqueue(it->first, it->second.exhausted_width);

        }
    }

    u64seq inject(const uint64_t *fullseq) {

        auto elem = tree.inject(fullseq);
        if (elem.empty()) { return elem; }

        auto it = tree.preds.find(elem);

        if (it->second.exhausted_width == 0) {
            // not previously in map
            it->second.exhausted_width = 1;
            enqueue(elem, 1);
        }

        return elem;
    }

    void load_file(std::string filename) {

        FILE* fptr = fopen(filename.c_str(), "rb");

        if (fptr == NULL) {
            ERREXIT("cannot open " << filename << " for reading.");
        }

        uint64_t bigheader = 0;
        fread(&bigheader, 8, 1, fptr);

        if (bigheader == 216768998249ull) {
            tree.read_from_file(fptr);
            fclose(fptr);
        } else {
            fclose(fptr);

            apg::pattern robin(lab, filename);
            std::vector<uint64_t> results;
            int n7 = ltransform(robin, vel, results);

            for (uint64_t i = 0; i < results.size(); i += n7) {
                inject(&(results[i]));
            }
        }
    }

    void inject_partial(const u64seq &results) {

        u64seq p;

        int range = jumpahead;
        int n6 = vel.vradius() * 2;

        uint64_t shadow = 0;

        for (size_t i = results.size() - n6; i < results.size(); i++) {
            shadow |= results[i];
        }

        if (shadow == 0) { range = results.size() - n6; }

        bool complete = false;

        for (int i = 0; i < range; i++) {
            auto pc = inject(&(results[i]));
            if (pc.empty()) { complete = true; break; }
            p = pc;
        }

        if (p.empty()) { return; }

        // guarantee that we don't print the same partial twice:
        if (tree.preds[p].exhausted_width >= 2) { return; }
        tree.preds[p].exhausted_width = 2;

        if (complete) {
            auto pat = tree.materialise(lab, p.data());
            pat = ikpx2golly(pat, vel);

            if (pat[vel.p](vel.hd, vel.vd) == pat) {
                std::cout << "\n#C completed spaceship: \033[34;1m" << pat.apgcode() << "\033[0m" << std::endl;
            } else {
                std::cout << "\n#C completed tail" << std::endl;
            }
            pat.write_rle(std::cout);

        } else if (tree.preds[p].depth > record_depth) {
            record_depth = tree.preds[p].depth;
            auto pat = tree.materialise(lab, p.data());
            std::cout << "\n#C depth = " << record_depth << std::endl;
            ikpx2golly(pat, vel).write_rle(std::cout);
        }
    }

    void launch_thread(WorkQueue &to_master, int iters=1) {
        for (int i = 0; i < iters; i++) {
            workers.emplace_back(worker_loop, &from_master, &to_master, vel, prime_implicants);
        }
    }

    void join_threads() {

        workitem item2;
        item2.exhausted_width = 0;
        item2.maximum_width = 0;
        item2.lookahead = 0;
        item2.direction = COMMAND_TERMINATE;

        for (size_t i = 0; i < workers.size(); i++) {
            from_master.enqueue(item2);
        }

        for (size_t i = 0; i < workers.size(); i++) {
            workers[i].join();
        }

        workers.clear();
    }

};

void master_loop(semisearch &searcher, WorkQueue &to_master, std::string directory, int backup_duration) {

    std::vector<std::string> checkpoint_names;

    checkpoint_names.push_back(directory + "/backup_odd.bin");
    checkpoint_names.push_back(directory + "/backup_even.bin");

    uint64_t xcount = 0;
    uint64_t checkpoint_number = 0;

    auto t1 = std::chrono::steady_clock::now();

    while (searcher.items_in_aether) {

        workitem item;
        to_master.wait_dequeue(item);

        if (item.lookahead) {
            searcher.tree.preds[item.initial_rows].exhausted_width = item.maximum_width;
            searcher.items_in_aether -= 1;
        } else {
            searcher.inject_partial(item.initial_rows);
        }

        xcount += 1;

        if ((xcount & 255) == 0) {
            std::cout << xcount << " iterations completed; qsize = " << searcher.items_in_aether;
            std::cout << "; treesize = " << searcher.tree.preds.size() << std::endl;
        }

        auto t2 = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() > backup_duration) {
            t1 = t2;
            std::cout << "Performing backup..." << std::endl;

            auto bfname = checkpoint_names[checkpoint_number];
            FILE* fptr = fopen(bfname.c_str(), "wb");

            if (fptr == NULL) {
                std::cout << "...cannot open file " << bfname << " for writing." << std::endl;
            } else {
                checkpoint_number += 1;
                if (checkpoint_number >= checkpoint_names.size()) {
                    checkpoint_number = 0;
                }

                uint64_t bigheader = 216768998249ull;

                fwrite(&bigheader, 8, 1, fptr);
                searcher.tree.write_to_file(fptr);

                fclose(fptr);

                std::cout << "...saved backup file " << bfname << " successfully." << std::endl;
            }
        }
    }

}

int run_ikpx(const std::vector<std::string> &arguments) {

    if (arguments.size() == 0) {
        print_help();
        ERREXIT("zero command-line arguments were provided.");
    }

    std::cerr << "sizeof(workitem) = " << sizeof(workitem) << std::endl;

    std::string velocity = "";
    std::string directory = ".";

    // DEFAULTS:
    int width = 3;
    int lookahead = 0;
    int jumpahead = 0;
    int backup_duration = 3600;
    int threads = 8;

    std::vector<std::string> filenames;

    for (size_t i = 0; i < arguments.size(); i++) {

        std::string command = arguments[i];
        if (command == "") { continue; }

        if (command[0] == '-') {
            if ((command == "-v") || (command == "--velocity")) {
                velocity = arguments[++i];
            } else if ((command == "-d") || (command == "--directory")) {
                directory = arguments[++i];
            } else if ((command == "-b") || (command == "--backup")) {
                backup_duration = std::stoll(arguments[++i]);
            } else if ((command == "-k") || (command == "--lookahead")) {
                lookahead = std::stoll(arguments[++i]);
            } else if ((command == "-j") || (command == "--jumpahead")) {
                jumpahead = std::stoll(arguments[++i]);
            } else if ((command == "-w") || (command == "--width")) {
                width = std::stoll(arguments[++i]);
            } else if ((command == "-p") || (command == "--threads")) {
                threads = std::stoll(arguments[++i]);
            } else {
                ERREXIT("unknown command: " << command);
            }
        } else {
            filenames.push_back(command);
        }
    }

    if (velocity == "") {
        ERREXIT("velocity must be specified.");
    }

    Velocity vel(velocity);

    std::cout << "Valid velocity: \033[32;1m(" << vel.vd << "," << vel.hd << ")c/" << vel.p << "\033[0m" << std::endl;

    std::cout << "Jacobian: [(" << vel.jacobian[0] << ", " << vel.jacobian[1] << "), (" <<
                                    vel.jacobian[2] << ", " << vel.jacobian[3] << "), (" <<
                                    vel.jacobian[4] << ", " << vel.jacobian[5] << ")]" << std::endl;

    if (lookahead == 0) { lookahead = 9 * vel.jacobian[1]; }
    if (jumpahead == 0) { jumpahead = lookahead >> 1; }

    std::cout << "lookahead = " << lookahead << "; jumpahead = " << jumpahead << std::endl;

    apg::lifetree<uint32_t, 1> lt(100);

    WorkQueue to_master;
    semisearch hs(vel, 0, &lt, width, lookahead, jumpahead);
    for (auto&& filename : filenames) { hs.load_file(filename); }

    if (hs.tree.preds.size() == 0) {
        hs.tree.inject_base_element();
    }

    hs.launch_thread(to_master, threads);

    while (true) {
        master_loop(hs, to_master, directory, backup_duration);
        hs.adaptive_widen();
    }

    hs.join_threads();

    return 0;

}

