#pragma once
#include "isosat.hpp"
#include "ikpxtree.hpp"
#include "banner.hpp"
#include "nthread.hpp"
#include "heap.hpp"
#include "../cqueue/blockingconcurrentqueue.h"

#include <chrono>
#include <unordered_set>

#define COMMAND_TERMINATE 2

struct workitem {

    u64seq initial_rows;
    int16_t exhausted_width;
    int16_t maximum_width;
    int16_t lookahead;
    int16_t direction;

};

typedef moodycamel::BlockingConcurrentQueue<workitem> WorkQueue;

struct worker_loop_obj {

    WorkQueue *from_master;
    WorkQueue *to_master;
    Velocity vel;
    std::vector<int> prime_implicants;

    worker_loop_obj(WorkQueue *from_master, WorkQueue *to_master, Velocity vel, std::vector<int> prime_implicants) :
        from_master(from_master), to_master(to_master), vel(vel), prime_implicants(prime_implicants) { }

};

void worker_loop(worker_loop_obj *obj) {

    WorkQueue *from_master = obj->from_master;
    WorkQueue *to_master = obj->to_master;
    Velocity vel = obj->vel;
    std::vector<int> prime_implicants = obj->prime_implicants;

    std::unordered_map<int, std::vector<int>> memdict;

    while (true) {

        workitem item;
        from_master->wait_dequeue(item);

        if (item.direction == COMMAND_TERMINATE) { break; }

        MetaProblem mp(item.initial_rows, vel);

        int total_solutions = 0;
        int subproblems = 0;

        do {

            subproblems += mp.find_all_solutions(item.maximum_width,
                prime_implicants, item.lookahead, [&](const u64seq &svec) {

                workitem item2;

                item2.initial_rows = svec;
                item2.exhausted_width = item.maximum_width;
                item2.maximum_width = 0;
                item2.lookahead = 0;
                item2.direction = item.direction;

                to_master->enqueue(item2);

                total_solutions += 1;

            }, memdict);

        } while ((mp.middle_bits++) == 0);

        if (total_solutions == 0) { item.maximum_width |= 0x4000; }
        item.lookahead = 1 + subproblems; // naughtily reuse field

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
    uint32_t mindepth;
    bool full_output;
    double_heap<ikpx_map::iterator> heap;

    int staleness;
    int items_in_aether;
    uint32_t record_depth;
    std::vector<int> truth_table;
    std::vector<int> prime_implicants;
    WorkQueue from_master;
    std::vector<NativeThread> workers;
    std::vector<worker_loop_obj*> wpointers;

    std::unordered_set<uint64_t> already_seen;

    semisearch(const Velocity &vel, int direction, lab32_t *lab, int search_width, int lookahead, int jumpahead, uint32_t mindepth, bool full_output) :
        vel(vel), tree(vel.vradius() * 2), direction(direction), lab(lab),
        search_width(search_width), lookahead(lookahead), jumpahead(jumpahead),
        mindepth(mindepth), full_output(full_output), heap() {

        search_width = 2;
        std::string rule = apg::get_all_rules()[0];
        truth_table = truth_table_for_rule(lab, rule);
        prime_implicants = truth_table_to_prime_implicants(truth_table);

        items_in_aether = 0;
        record_depth = 0;
        staleness = 0;
    }

    void enheap(ikpx_map::iterator it) {

        size_t depth = it->second.depth;
        if (depth < mindepth) { return; }
        int xw = it->second.exhausted_width & 0x3fff;
        if (xw >= search_width) { return; }

        uint64_t shadow = 1;
        for (auto&& x : it->first) { shadow |= x; }
        size_t breadth = floor_log2(shadow);

        heap.push(breadth, depth, it);
    }

    void enqueue_all() {

        int ideal_items = 10 * workers.size();

        while ((items_in_aether < ideal_items) && (heap.elements > 0)) {

            auto it = heap.pop();

            if (tree.is_subsumed(it, search_width)) {
                it->second.exhausted_width = 0x4000 + search_width;
                continue;
            }

            int xw = it->second.exhausted_width & 0x3fff;

            auto elem = it->first;

            workitem item;

            item.initial_rows = elem;
            item.exhausted_width = xw;
            item.maximum_width = search_width;
            item.lookahead = lookahead;
            item.direction = direction;

            from_master.enqueue(item);
            items_in_aether += 1;
        }
    }

    void rundict() {

        for (auto it = tree.preds.begin(); it != tree.preds.end(); ++it) {
            enheap(it);
        }

        enqueue_all();
    }

    void adaptive_widen() {

        search_width += 1;

        std::cout << "# Adaptive widening to width " << search_width;
        std::cout << " (treesize = " << tree.preds.size() << ")" << std::endl;

        rundict();
    }

    u64seq inject(const uint64_t *fullseq) {

        auto elem = tree.inject(fullseq);
        if (elem.empty()) { return elem; }

        auto it = tree.preds.find(elem);

        if (it->second.exhausted_width == 0) {
            // not previously in map
            it->second.exhausted_width = 1;
            enheap(it);
            // enqueue(elem, 1);
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
                tree.inject(&(results[i]));
            }
        }
    }

    void inject_partial(u64seq &results) {

        u64seq p;

        int range = jumpahead;
        int n6 = vel.vradius() * 2;

        uint64_t shadow = 0;

        for (size_t i = results.size() - n6; i < results.size(); i++) {
            shadow |= results[i];
        }

        if (shadow == 0) {
            range = results.size() - n6;
        }

        bool complete = false;

        for (int i = 0; i < range; i++) {
            auto pc = inject(&(results[i]));
            if (pc.empty()) {
                complete = true;
                int j = i - n6;
                if (j < 0) { j = 0; }
                p = inject(&(results[j]));
                results.resize(j + n6 + 1);
                break;
            }
            p = pc;
        }

        if (p.empty()) { return; }

        if ((!complete) && (!full_output) && (tree.preds[p].depth <= record_depth)) { return; }

        auto pat = tree.materialise(lab, results);

        {
            int64_t bbox[4];
            pat.getrect(bbox);

            uint64_t digest = pat.shift(0 - bbox[0], 0 - bbox[1]).digest();
            uint64_t digest2 = complete ? (digest * 3ull) : digest;

            if (already_seen.count(digest2)) { return; }
            already_seen.insert(digest);
            already_seen.insert(digest2);
        }

        pat = ikpx2golly(pat, vel);
        if (pat.empty()) { return; /* no xs0_0 please */ }

        if (complete) {
            if (pat[vel.p](vel.hd, vel.vd) == pat) {
                std::string apgcode = pat.apgcode();
                std::cout << "\n#C completed spaceship: \033[34;1m" << apgcode << "\033[0m" << std::endl;
                std::cout << "#C Catagolue URL: https://gol.hatsya.co.uk/object/" << apgcode << "/" << pat.getrule() << std::endl;
            } else {
                std::cout << "\n#C completed tail" << std::endl;
            }
        } else if (!full_output) {
            record_depth = tree.preds[p].depth;
            std::cout << "\n#C depth = " << record_depth << std::endl;
        }
        staleness = 24;
        pat.write_rle(std::cout);
        std::cout << std::endl;
    }

    void launch_thread(WorkQueue &to_master, int iters=1) {

        for (int i = 0; i < iters; i++) {
            auto wo = new worker_loop_obj(&from_master, &to_master, vel, prime_implicants);
            workers.emplace_back(worker_loop, wo);
            wpointers.push_back(wo);
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
            auto wo = wpointers[i];
            delete wo;
        }

        workers.clear();
        wpointers.clear();
    }

};

template<typename T>
void master_loop(semisearch &searcher, WorkQueue &to_master, std::string directory,
                    int backup_duration, bool last_iteration, T &t1) {

    bool save_at_end = last_iteration;
    std::vector<std::string> checkpoint_names;

    {
        std::ostringstream ss;
        std::string rule = apg::get_all_rules()[0];
        ss << directory << "/backup_" << rule;
        ss << "_velocity_" << searcher.vel.vd << "_" << searcher.vel.hd << "_" << searcher.vel.p;
        ss << "_width_" << searcher.search_width;
        checkpoint_names.push_back(ss.str() + "_odd.bin");
        checkpoint_names.push_back(ss.str() + "_even.bin");
        checkpoint_names.push_back(ss.str() + "_final.bin");
    }

    uint64_t xcount = 0;
    uint64_t scount = 0;
    uint64_t bcount = 0;
    uint64_t pcount = 0;
    uint64_t checkpoint_number = 0;

    while (searcher.items_in_aether) {

        workitem item;
        to_master.wait_dequeue(item);

        if (item.lookahead) {
            searcher.tree.preds[item.initial_rows].exhausted_width = item.maximum_width;
            searcher.items_in_aether -= 1;
            pcount += 1;
            bcount += (item.lookahead - 1);
        } else {
            searcher.inject_partial(item.initial_rows);
            scount += 1;
        }

        xcount += 1;

        if ((xcount & 255) == 0) {
            std::cout << "# " << xcount << " iterations (" << pcount << " problems, " << bcount;
            std::cout << " subproblems, " << scount << " solutions) completed: queuesize = ";
            std::cout << searcher.items_in_aether << "; heapsize = " << searcher.heap.elements;
            std::cout << "; treesize = " << searcher.tree.preds.size() << std::endl;

            if (searcher.staleness > 0) {
                searcher.staleness -= 1;
            } else if (searcher.record_depth > 0) {
                searcher.record_depth -= 1;
            }
        }

        searcher.enqueue_all();

        auto t2 = std::chrono::steady_clock::now();
        bool finished_queue = (searcher.items_in_aether == 0);
        bool hour_elapsed = (std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() > backup_duration);
        save_at_end |= hour_elapsed;
        finished_queue &= save_at_end;

        if (finished_queue || hour_elapsed) {

            std::cout << "# Performing backup..." << std::endl;

            auto bfname = checkpoint_names[finished_queue ? 2 : checkpoint_number];
            FILE* fptr = fopen(bfname.c_str(), "wb");

            if (fptr == NULL) {
                std::cout << "# ...cannot open file " << bfname << " for writing." << std::endl;
            } else {
                checkpoint_number ^= 1;

                uint64_t bigheader = 216768998249ull;

                fwrite(&bigheader, 8, 1, fptr);
                searcher.tree.write_to_file(fptr);

                fclose(fptr);

                std::cout << "# ...saved backup file " << bfname << " successfully." << std::endl;
            }

            t1 = std::chrono::steady_clock::now();
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
    int maximum_width = 60;
    int lookahead = 0;
    int jumpahead = 0;
    int backup_duration = 3600;
    int threads = 8;
    int minimum_depth = 0;
    bool full_output = false;

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
            } else if ((command == "-x") || (command == "--maximum-width")) {
                maximum_width = std::stoll(arguments[++i]);
            } else if ((command == "-m") || (command == "--minimum-depth")) {
                minimum_depth = std::stoll(arguments[++i]);
            } else if ((command == "-p") || (command == "--threads")) {
                threads = std::stoll(arguments[++i]);
            } else if ((command == "-f") || (command == "--full-output")) {
                full_output = true;
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

    std::cout << "# Valid velocity: \033[32;1m(" << vel.vd << "," << vel.hd << ")c/" << vel.p << "\033[0m" << std::endl;

    std::cout << "# Jacobian: [(" << vel.jacobian[0] << ", " << vel.jacobian[1] << "), (" <<
                                    vel.jacobian[2] << ", " << vel.jacobian[3] << "), (" <<
                                    vel.jacobian[4] << ", " << vel.jacobian[5] << ")]" << std::endl;

    if (lookahead == 0) { lookahead = vel.jacobian[1] << 3; }
    if (jumpahead == 0) { jumpahead = lookahead >> 2; }

    std::cout << "# lookahead = " << lookahead << "; jumpahead = " << jumpahead << std::endl;

    apg::lifetree<uint32_t, 1> lt(100);

    WorkQueue to_master;
    semisearch hs(vel, 0, &lt, width, lookahead, jumpahead, minimum_depth, full_output);

    // load search tree:
    for (auto&& filename : filenames) { hs.load_file(filename); }
    if (hs.tree.preds.size() == 0) { hs.tree.inject_base_element(); }

    // launch worker threads:
    hs.launch_thread(to_master, threads);

    // enqueue the work:
    hs.rundict();

    // start backup timer:
    auto t1 = std::chrono::steady_clock::now();

    while (true) {
        bool last_iteration = (hs.search_width >= maximum_width);
        master_loop(hs, to_master, directory, backup_duration, last_iteration, t1);
        if (last_iteration) { break; }
        hs.adaptive_widen();
    }

    hs.join_threads();

    return 0;

}

